#include <GL/freeglut.h>
#include <cmath>
#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <iomanip>
#include "vtk_reader.h"
#include "data_structures.h"

using namespace std;

// --- VARIÁVEIS GLOBAIS ---
vector<Ponto> g_pontos;
vector<Segmento> g_segmentos;

// Câmera Orbitante (Aula 09)
float g_camDist = 4.0f;
float g_camTheta = 0.0f; 
float g_camPhi = 0.5f;

// Controle de Animação e Passos (TP2 Requisito)
bool g_animating = false;
int g_current_step = 16;
vector<int> g_validSteps = {16, 32, 48, 64, 80, 96, 112, 128}; // Exemplo para Nterm 128
int g_stepIndex = 0;

// Iluminação e Seleção
bool g_useSmoothShading = true;
int g_selectedSegment = -1;

// Caminhos (Ajuste conforme sua pasta data/)
string g_basePath = "./data/TP2_3D/Nterm_0128/";
string g_ntermPrefix = "tree3D_Nterm0128_step";

// --- FUNÇÕES AUXILIARES ---

// REUTILIZADO: Mapa de cores do seu TP1
void getColorFromValue(float v, float &r, float &g, float &b) {
    float t = (v - 0.01f) / (0.1f - 0.01f); // Normalização simples para o exemplo
    t = max(0.0f, min(1.0f, t));
    r = t; g = 1.0f - t; b = 0.2f; // Gradiente simples: Vermelho (grosso) -> Verde (fino)
}

// NOVO: Cálculo de comprimento para o relatório do TP2
float calculateLength(const Ponto &p1, const Ponto &p2) {
    return sqrt(pow(p2.x - p1.x, 2) + pow(p2.y - p1.y, 2) + pow(p2.z - p1.z, 2));
}

// --- NÚCLEO DE RENDERIZAÇÃO 3D ---

// Desenha o ramo como tronco de cone com normais para iluminação
void drawVesselSegment(const Ponto &p1, const Ponto &p2, float r1, float r2, bool pickingMode = false, int id = 0) {
    float dx = p2.x - p1.x, dy = p2.y - p1.y, dz = p2.z - p1.z;
    float len = sqrt(dx*dx + dy*dy + dz*dz);
    if (len < 1e-6f) return;

    if (pickingMode) {
        unsigned char r = (id & 0x0000FF);
        unsigned char g = (id & 0x00FF00) >> 8;
        unsigned char b = (id & 0xFF0000) >> 16;
        glColor3ub(r, g, b);
    }

    glPushMatrix();
    glTranslatef(p1.x, p1.y, p1.z);

    // Rotação para alinhar o cilindro ao vetor (p2-p1)
    float angle = acos(dz / len) * 180.0f / M_PI;
    if (abs(dz / len) < 0.999f) glRotatef(angle, -dy, dx, 0.0f);
    else if (dz < 0) glRotatef(180.0f, 1.0f, 0.0f, 0.0f);

    GLUquadric* quad = gluNewQuadric();
    gluQuadricNormals(quad, g_useSmoothShading ? GLU_SMOOTH : GLU_FLAT);
    gluCylinder(quad, r1, r2, len, 16, 1);
    gluDeleteQuadric(quad);
    glPopMatrix();
}

// --- CALLBACKS E LÓGICA ---

void loadTree() {
    readVTKFile(g_basePath, g_ntermPrefix, g_current_step, g_pontos, g_segmentos);
}

void init3D() {
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glEnable(GL_DEPTH_TEST); // Z-Buffer
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    
    // Configuração de Phong (Especular)
    float lightSpec[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpec);
    
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
}



void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, 1.0, 0.1, 100.0); // Projeção Perspectiva

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    float eyX = g_camDist * cos(g_camPhi) * sin(g_camTheta);
    float eyY = g_camDist * sin(g_camPhi);
    float eyZ = g_camDist * cos(g_camPhi) * cos(g_camTheta);
    gluLookAt(eyX, eyY, eyZ, 0, 0, 0, 0, 1, 0);

    for (int i = 0; i < g_segmentos.size(); i++) {
        const auto &s = g_segmentos[i];
        if (i == g_selectedSegment) glColor3f(1.0f, 1.0f, 0.0f); // Destaque Amarelo
        else {
            float r, g, b;
            getColorFromValue(s.raio, r, g, b);
            glColor3f(r, g, b);
        }
        drawVesselSegment(g_pontos[s.p1_index], g_pontos[s.p2_index], s.raio*0.02f, s.raio*0.02f);
    }
    glutSwapBuffers();
}



void timer(int v) {
    if (g_animating && g_stepIndex < g_validSteps.size()-1) {
        g_stepIndex++;
        g_current_step = g_validSteps[g_stepIndex];
        loadTree();
        glutPostRedisplay();
        glutTimerFunc(100, timer, 0);
    } else g_animating = false;
}

void processPicking(int x, int y) {
    glDisable(GL_LIGHTING);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    for (int i = 0; i < g_segmentos.size(); i++) {
        drawVesselSegment(g_pontos[g_segmentos[i].p1_index], g_pontos[g_segmentos[i].p2_index], 
                          g_segmentos[i].raio*0.02f, g_segmentos[i].raio*0.02f, true, i);
    }
    unsigned char res[3];
    GLint viewport[4]; glGetIntegerv(GL_VIEWPORT, viewport);
    glReadPixels(x, viewport[3]-y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, res);
    int id = res[0] + (res[1] << 8) + (res[2] << 16);
    if (id < g_segmentos.size()) {
        g_selectedSegment = id;
        cout << "Vaso: " << id << " | Raio: " << g_segmentos[id].raio << " | Comp: " 
             << calculateLength(g_pontos[g_segmentos[id].p1_index], g_pontos[g_segmentos[id].p2_index]) << endl;
    }
    glEnable(GL_LIGHTING);
    glutPostRedisplay();
}

void mouse(int b, int s, int x, int y) {
    if (b == GLUT_LEFT_BUTTON && s == GLUT_DOWN) processPicking(x, y);
}

void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case 'a': g_camTheta -= 0.1f; break;
        case 'd': g_camTheta += 0.1f; break;
        case 'w': g_camPhi += 0.1f; break;
        case 's': g_camPhi -= 0.1f; break;
        case 'p': g_animating = !g_animating; if(g_animating) timer(0); break;
        case 'l': g_useSmoothShading = !g_useSmoothShading; break;
        case 27: exit(0);
    }
    glutPostRedisplay();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 800);
    glutCreateWindow("UFOP - CG TP2: Arvore Arterial 3D");
    init3D();
    loadTree();
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMainLoop();
    return 0;
}