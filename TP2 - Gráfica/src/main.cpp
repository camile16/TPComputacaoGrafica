#include <GL/freeglut.h>
#include <cmath>
#include <iostream>
#include <vector>
#include <algorithm>
#include "vtk_reader.h"
#include "data_structures.h"

using namespace std;

// Globais
vector<Ponto> g_pontos;
vector<Segmento> g_segmentos;
float g_camDist = 4.0f, g_camTheta = 0.0f, g_camPhi = 0.5f;
bool g_animating = false, g_useSmooth = true;
int g_selectedID = -1, g_current_step = 64;
int g_stepIndex = 0;
vector<int> g_validSteps = {64, 128, 256, 320, 384, 448, 512}; 

// Caminhos configurados para a sua estrutura (Rodando da raiz do projeto)
string g_basePath = "../data/Nterm_512/";
string g_ntermPrefix = "tree3D_Nterm0512_step";

void getColorFromValue(float v, float &r, float &g, float &b) {
    float t = (v - 0.005f) / (0.05f - 0.005f); 
    t = max(0.0f, min(1.0f, t));
    r = t; g = 1.0f - t; b = 0.3f;
}

void drawVesselSegment(int i, bool picking = false) {
    const Segmento &s = g_segmentos[i];
    const Ponto &p1 = g_pontos[s.p1_index];
    const Ponto &p2 = g_pontos[s.p2_index];

    // PROTEÇÃO: Verifica se o vetor de raios por ponto está populado
    float r1, r2;
    if (!g_radii_points.empty() && s.p1_index < g_radii_points.size() && s.p2_index < g_radii_points.size()) {
        r1 = g_radii_points[s.p1_index] * 0.025f;
        r2 = g_radii_points[s.p2_index] * 0.025f;
    } else {
        // Fallback: Usa o raio médio do segmento se não houver dados por ponto
        r1 = r2 = s.raio * 0.025f;
    }

    float dx = p2.x - p1.x, dy = p2.y - p1.y, dz = p2.z - p1.z;
    float len = sqrt(dx*dx + dy*dy + dz*dz);
    if (len < 1e-6f) return;

    if (picking) {
        glColor3ub(i & 0xFF, (i >> 8) & 0xFF, (i >> 16) & 0xFF);
    } else if (i == g_selectedID) {
        glColor3f(1.0f, 1.0f, 0.0f); // Destaque Amarelo
    } else {
        float r, g, b;
        getColorFromValue(s.raio, r, g, b);
        glColor3f(r, g, b);
    }

    glPushMatrix();
    glTranslatef(p1.x, p1.y, p1.z);
    float angle = acos(dz / len) * 180.0f / M_PI;
    if (abs(dz / len) < 0.999f) glRotatef(angle, -dy, dx, 0.0f);
    else if (dz < 0) glRotatef(180.0f, 1.0f, 0.0f, 0.0f);

    GLUquadric* quad = gluNewQuadric();
    gluQuadricNormals(quad, g_useSmooth ? GLU_SMOOTH : GLU_FLAT);
    gluCylinder(quad, r1, r2, len, 16, 1);
    gluDeleteQuadric(quad);
    glPopMatrix();
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, 1.0, 0.1, 100.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    float x = g_camDist * cos(g_camPhi) * sin(g_camTheta);
    float y = g_camDist * sin(g_camPhi);
    float z = g_camDist * cos(g_camPhi) * cos(g_camTheta);
    gluLookAt(x, y, z, 0, 0, 0, 0, 1, 0);

    for (int i = 0; i < (int)g_segmentos.size(); i++) drawVesselSegment(i);
    glutSwapBuffers();
}

void init3D() {
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    
    float lightSpec[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpec);
    glMaterialf(GL_FRONT, GL_SHININESS, 60.0f); // Brilho Especular (Phong)
}

void timer(int v) {
    if (g_animating && g_stepIndex < (int)g_validSteps.size() - 1) {
        g_stepIndex++;
        g_current_step = g_validSteps[g_stepIndex];
        readVTKFile(g_basePath, g_ntermPrefix, g_current_step, g_pontos, g_segmentos);
        glutPostRedisplay();
        glutTimerFunc(150, timer, 0);
    } else g_animating = false;
}

void mouse(int b, int s, int x, int y) {
    if (b == GLUT_LEFT_BUTTON && s == GLUT_DOWN) {
        glDisable(GL_LIGHTING);
        for (int i = 0; i < (int)g_segmentos.size(); i++) drawVesselSegment(i, true);
        unsigned char res[3];
        GLint vp[4]; glGetIntegerv(GL_VIEWPORT, vp);
        glReadPixels(x, vp[3] - y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, res);
        int id = res[0] + (res[1] << 8) + (res[2] << 16);
        if (id < (int)g_segmentos.size()) {
            g_selectedID = id;
            cout << "Selecionado ID: " << id << " | Raio: " << g_segmentos[id].raio << endl;
        }
        glEnable(GL_LIGHTING);
        glutPostRedisplay();
    }
}

void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case 'a': g_camTheta -= 0.1f; break;
        case 'd': g_camTheta += 0.1f; break;
        case 'w': g_camPhi += 0.1f; break;
        case 's': g_camPhi -= 0.1f; break;
        case 'p': g_animating = !g_animating; if(g_animating) timer(0); break;
        case 'l': g_useSmooth = !g_useSmooth; break;
        case 27: exit(0);
    }
    glutPostRedisplay();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 800);
    glutCreateWindow("TP2 - UFOP - Visualizacao Arterial 3D");
    init3D();
    readVTKFile(g_basePath, g_ntermPrefix, g_current_step, g_pontos, g_segmentos);
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMainLoop();
    return 0;
}