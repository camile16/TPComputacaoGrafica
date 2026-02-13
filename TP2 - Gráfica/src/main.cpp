#include <GL/freeglut.h>
#include <cmath>
#include <iostream>
#include <vector>
#include <algorithm>
#include "vtk_reader.h"
#include "data_structures.h"

using namespace std;

// dados da arvore
vector<Ponto> g_pontos;
vector<Segmento> g_segmentos;

// controle da camera
// distancia e angulos para girar em volta da arvore
float g_camDist = 3.0f, g_camTheta = 0.0f, g_camPhi = 1.0f; 

// variaveis de controle
bool g_animating = false; // se a animacao esta ligada
bool g_useSmooth = true;  // controla o tipo de iluminacao (liso ou facetado)
int g_selectedID = -1;    // qual galho o usuario clicou

// controle de qual passo do crescimento estamos mostrando
int g_current_step = 64;
int g_stepIndex = 0;
vector<int> g_validSteps; // lista de passos que existem na pasta

// caminhos dos arquivos
string g_basePath = "";     
string g_ntermPrefix = "";  

// funcao auxiliar para carregar a arvore
// pega o passo atual e chama o leitor de arquivos
void loadTree() {
    if (g_stepIndex >= g_validSteps.size()) g_stepIndex = 0;
    g_current_step = g_validSteps[g_stepIndex];

    cout << "carregando passo: " << g_current_step << endl;
    readVTKFile(g_basePath, g_ntermPrefix, g_current_step, g_pontos, g_segmentos);
    
    glutPostRedisplay(); // pede para redesenhar a tela
}

// configura o programa para ler uma arvore especifica
// chamada quando apertamos os numeros do teclado
void setResolutionPreset(int nterm) {
    g_animating = false; // pausa animacao para nao dar erro
    g_stepIndex = 0;     // volta para o inicio
    g_selectedID = -1;   // tira a selecao

    // define onde estao os arquivos e quais passos existem
    if (nterm == 64) {
        g_basePath = "../data/Nterm_064/"; 
        g_ntermPrefix = "tree3D_Nterm0064_step";
        g_validSteps = {8, 16, 24, 32, 40, 48, 56, 64};
    } 
    else if (nterm == 128) {
        g_basePath = "../data/Nterm_128/";
        g_ntermPrefix = "tree3D_Nterm0128_step";
        g_validSteps = {16, 32, 48, 64, 80, 96, 112, 128};
    }
    else if (nterm == 256) {
        g_basePath = "../data/Nterm_256/";
        g_ntermPrefix = "tree3D_Nterm0256_step";
        g_validSteps = {32, 64, 96, 128, 160, 192, 224, 256};
    }
    else if (nterm == 512) {
        g_basePath = "../data/Nterm_512/";
        g_ntermPrefix = "tree3D_Nterm0512_step";
        g_validSteps = {64, 128, 192, 256, 320, 384, 448, 512};
    }

    loadTree(); // carrega o primeiro estado da nova arvore
}

// cria um gradiente de cor: azul (fino) ate vermelho (grosso)
void getColorFromValue(float v, float &r, float &g, float &b) {
    float max_expected_radius = 0.05f; 
    float t = v / max_expected_radius; 
    t = max(0.0f, min(1.0f, t));
    r = t; g = 0.2f; b = 1.0f - t;
}

// funcao que desenha um unico pedaco de galho (cilindro)
void drawVesselSegment(int i, bool picking = false) {
    if (i >= g_segmentos.size()) return;

    const Segmento &s = g_segmentos[i];
    const Ponto &p1 = g_pontos[s.p1_index]; // ponto inicial
    const Ponto &p2 = g_pontos[s.p2_index]; // ponto final

    float r1, r2;
    float visual_scale = 1.0f; 

    // usa espessura variavel (cone) se tiver dados
    if (!g_radii_points.empty()) {
        r1 = g_radii_points[s.p1_index] * visual_scale;
        r2 = g_radii_points[s.p2_index] * visual_scale;
    } else {
        r1 = r2 = s.raio * visual_scale; // senao usa fixa
    }

    // matematica para calcular tamanho e direcao
    float dx = p2.x - p1.x;
    float dy = p2.y - p1.y;
    float dz = p2.z - p1.z;
    float len = sqrt(dx*dx + dy*dy + dz*dz);
    
    if (len < 1e-6f) return;

    // logica de cores
    if (picking) {
        // modo de selecao: pinta com uma cor codigo unica
        glColor3ub(i & 0xFF, (i >> 8) & 0xFF, (i >> 16) & 0xFF);
    } else if (i == g_selectedID) {
        glColor3f(1.0f, 1.0f, 0.0f); // amarelo se selecionado
    } else {
        // cor normal baseada na espessura
        float r, g, b;
        getColorFromValue(s.raio, r, g, b);
        glColor4f(r, g, b, 0.65f); // transparencia
    }

    // move o pincel 3d para o ponto inicial
    glPushMatrix();
    glTranslatef(p1.x, p1.y, p1.z);

    // rotacao do cilindro para alinhar com o galho
    float rx = -dy; float ry = dx; float rz = 0.0f;
    float angle = acos(dz / len) * 180.0f / M_PI;

    // tratamento para evitar erros se o galho for vertical
    if (sqrt(rx*rx + ry*ry) < 0.001f) {
        if (dz < 0) glRotatef(180.0f, 1.0f, 0.0f, 0.0f);
    } else {
        glRotatef(angle, rx, ry, rz);
    }

    // desenha o cilindro
    GLUquadric* quad = gluNewQuadric();
    // escolhe iluminacao suave ou facetada
    gluQuadricNormals(quad, g_useSmooth ? GLU_SMOOTH : GLU_FLAT);
    gluCylinder(quad, r1, r2, len, 12, 1);
    gluDeleteQuadric(quad);
    glPopMatrix();
}

// funcao principal de desenho
void display() {
    // limpa a tela e o buffer de profundidade
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // configura a camera (perspectiva)
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, 1.0, 0.1, 100.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // calcula posicao da camera baseada nos angulos
    float x = g_camDist * cos(g_camPhi) * sin(g_camTheta);
    float y = g_camDist * sin(g_camPhi);
    float z = g_camDist * cos(g_camPhi) * cos(g_camTheta);
    
    // trava a camera para nao virar de ponta cabeca
    if (g_camPhi > 1.5f) g_camPhi = 1.5f;
    if (g_camPhi < -1.5f) g_camPhi = -1.5f;

    // aponta para o centro
    gluLookAt(x, y, z, 0, 0, 0, 0, 1, 0);

    // desenha todos os galhos
    for (int i = 0; i < (int)g_segmentos.size(); i++) {
        drawVesselSegment(i);
    }
    glutSwapBuffers();
}

// configuracoes iniciais de luz e cor
void init3D() {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // fundo cinza escuro
    glEnable(GL_DEPTH_TEST); // ativa o z-buffer
    
    // configura iluminacao
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    
    // permite cor nos materiais
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    
    // ativa transparencia
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glEnable(GL_NORMALIZE); 
    
    // posicao da luz
    GLfloat light_pos[] = { 5.0f, 5.0f, 10.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
    
    // brilho especular
    float lightSpec[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpec);
    glMaterialf(GL_FRONT, GL_SHININESS, 60.0f); 
}

// funcao de tempo para animacao
void timer(int v) {
    if (g_animating) {
        g_stepIndex++;
        // faz o loop da animacao
        if (g_stepIndex >= g_validSteps.size()) g_stepIndex = 0;
        
        loadTree(); // carrega o proximo passo
        
        glutTimerFunc(400, timer, 0); // espera e chama de novo
    }
}

// lida com cliques do mouse
void mouse(int b, int s, int x, int y) {
    if (b == GLUT_LEFT_BUTTON && s == GLUT_DOWN) {
        // tecnica de selecao por cor (picking)
        // desliga luzes para desenhar cores puras
        glDisable(GL_LIGHTING);
        glDisable(GL_DITHER); 
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // reposiciona a camera igual ao modo de visualizacao
        glMatrixMode(GL_PROJECTION); glLoadIdentity(); gluPerspective(45.0, 1.0, 0.1, 100.0);
        glMatrixMode(GL_MODELVIEW); glLoadIdentity();
        float cx = g_camDist * cos(g_camPhi) * sin(g_camTheta);
        float cy = g_camDist * sin(g_camPhi);
        float cz = g_camDist * cos(g_camPhi) * cos(g_camTheta);
        gluLookAt(cx, cy, cz, 0, 0, 0, 0, 1, 0);

        // desenha objetos com cores codigo
        for (int i = 0; i < (int)g_segmentos.size(); i++) drawVesselSegment(i, true);
        
        // le a cor do pixel clicado
        unsigned char res[3];
        GLint vp[4]; glGetIntegerv(GL_VIEWPORT, vp);
        glReadPixels(x, vp[3] - y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, res);
        
        // converte cor para id
        int id = res[0] + (res[1] << 8) + (res[2] << 16);
        
        if (id >= 0 && id < (int)g_segmentos.size()) {
            g_selectedID = id;
            const Segmento &seg = g_segmentos[id];
            cout << "selecionado id: " << id << ", raio: " << seg.raio << endl;
        } else {
            g_selectedID = -1;
        }
        
        // religa luzes e redesenha
        glEnable(GL_LIGHTING);
        glEnable(GL_DITHER);
        glutPostRedisplay(); 
    }
}

// lida com o teclado
void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        // troca de arvores
        case '1': setResolutionPreset(64); break;
        case '2': setResolutionPreset(128); break;
        case '3': setResolutionPreset(256); break;
        case '4': setResolutionPreset(512); break;

        // movimentacao da camera
        case 'a': g_camTheta -= 0.1f; break; // esquerda
        case 'd': g_camTheta += 0.1f; break; // direita
        case 'w': g_camPhi += 0.1f; break;   // sobe
        case 's': g_camPhi -= 0.1f; break;   // desce
        case '+': g_camDist -= 0.2f; break;  // zoom in
        case '-': g_camDist += 0.2f; break;  // zoom out
        
        // opcoes
        case 'p': // pausa ou toca animacao
            g_animating = !g_animating; 
            if(g_animating) timer(0); 
            break;
        case 'l': // alterna iluminacao (suave ou facetada)
            g_useSmooth = !g_useSmooth; 
            break;
        case 27: exit(0); // sai com esc
    }
    glutPostRedisplay();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 800);
    glutCreateWindow("tp2 - arvore arterial 3d - multiplas resolucoes");
    
    init3D(); 
    
    setResolutionPreset(128); // comeca com a arvore de 128
    
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMainLoop();
    return 0;
}