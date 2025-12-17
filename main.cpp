#include <GL/freeglut.h>
#include <cmath>
#include <iostream>
#include <vector>
#include <algorithm>
#include "vtk_reader.h"
#include "data_structures.h"

std::vector<Ponto> g_pontos;
std::vector<Segmento> g_segmentos;

float g_transX = 0.0f;
float g_transY = 0.0f;
float g_rotAngulo = 0.0f;
float g_escala = 1.0f;

int g_current_step = 64;
std::vector<int> g_validSteps;
int g_stepIndex = 0;

float g_globalRadiusScale = 0.05f;
float g_minRaio = 0.0f;
float g_maxRaio = 1.0f;

static std::string g_basePath =
    "./TP_CCO_Pacote_Dados/TP1_2D/Nterm_064/";
static std::string g_ntermPrefix = "tree2D_Nterm0064_step";

// define cor fixa
bool g_useFixedColor = false;                           
float g_fixedR = 0.0f, g_fixedG = 0.7f, g_fixedB = 0.0f; 


void init()
{

    // (Aula 07: Teoria das Cores) - define a cor de fundo usando o modelo RGB (1,1,1 = Branco)
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    // (Aula 05: Atributos de Primitivas) - define a largura da linha como um atributo de estado
    glLineWidth(2.0f);
}

void getColorFromValue(float v, float &r, float &g, float &b)
{
    // (Aula 07: Teoria das Cores) - implementação de um Colormap (Pseudo-coloração)
    // mapeia valores escalares para componentes R, G, B para facilitar a visualização
    float mn = g_minRaio;
    float mx = g_maxRaio;
    float t = 0.0f;
    if (mx > mn)
        t = (v - mn) / (mx - mn);
    t = std::max(0.0f, std::min(1.0f, t));

    if (t <= 0.125f)
    {
        r = 0.0f;
        g = 0.0f;
        b = 0.5f + (t / 0.125f) * 0.5f;
    }
    else if (t <= 0.375f)
    {
        float tt = (t - 0.125f) / 0.25f;
        r = 0.0f;
        g = tt;
        b = 1.0f;
    }
    else if (t <= 0.625f)
    {
        float tt = (t - 0.375f) / 0.25f;
        r = tt;
        g = 1.0f;
        b = 1.0f - tt;
    }
    else if (t <= 0.875f)
    {
        float tt = (t - 0.625f) / 0.25f;
        r = 1.0f;
        g = 1.0f - 0.5f * tt;
        b = 0.0f;
    }
    else
    {
        float tt = (t - 0.875f) / 0.125f;
        r = 1.0f;
        g = 0.5f - 0.5f * tt;
        b = 0.0f;
    }
}

void drawSegmentAsQuad(const Ponto &a, const Ponto &b, float halfWidth)
{
    // (Aula 05: Primitivas Gráficas) - o codigo gera manualmente um quadrilátero para dar espessura ao vaso
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 1e-6f)
        return;

    // (Aula 08: Operações Geométricas)- calculo de Vetor Normal para determinar a espessura
    float nx = -dy / len;
    float ny = dx / len;

    float ox = nx * halfWidth;
    float oy = ny * halfWidth;

    float v1x = a.x + ox, v1y = a.y + oy;
    float v2x = b.x + ox, v2y = b.y + oy;
    float v3x = b.x - ox, v3y = b.y - oy;
    float v4x = a.x - ox, v4y = a.y - oy;

    // (Aula 05: Primitivas) - GL_TRIANGLES é a primitiva básica de preenchimento
    glBegin(GL_TRIANGLES);
    glVertex2f(v1x, v1y);
    glVertex2f(v2x, v2y);
    glVertex2f(v3x, v3y);

    glVertex2f(v3x, v3y);
    glVertex2f(v4x, v4y);
    glVertex2f(v1x, v1y);
    glEnd();
}

void updateRadiusRange()
{
    if (g_segmentos.empty())
    {
        g_minRaio = 0.0f;
        g_maxRaio = 1.0f;
        return;
    }
    g_minRaio = g_segmentos[0].raio;
    g_maxRaio = g_segmentos[0].raio;
    for (const auto &s : g_segmentos)
    {
        g_minRaio = std::min(g_minRaio, s.raio);
        g_maxRaio = std::max(g_maxRaio, s.raio);
    }
    if (std::abs(g_maxRaio - g_minRaio) < 1e-6f)
    {
        float eps = std::max(1e-3f, 0.1f * std::abs(g_minRaio));
        g_minRaio -= eps;
        g_maxRaio += eps;
    }
    std::cout << "Raio range: [" << g_minRaio << ", " << g_maxRaio << "]\n";
}

void loadTree()
{
    readVTKFile(g_basePath, g_ntermPrefix, g_current_step, g_pontos, g_segmentos);
    updateRadiusRange();
}

void setResolutionPreset(int nterm)
{
    g_validSteps.clear();
    g_stepIndex = 0;

    if (nterm == 64)
    {
        g_basePath = "./TP_CCO_Pacote_Dados/TP1_2D/Nterm_064/";
        g_ntermPrefix = "tree2D_Nterm0064_step";
        g_validSteps = {8, 16, 24, 32, 40, 48, 56, 64};
    }
    else if (nterm == 128)
    {
        g_basePath = "./TP_CCO_Pacote_Dados/TP1_2D/Nterm_128/";
        g_ntermPrefix = "tree2D_Nterm0128_step";
        g_validSteps = {16, 32, 48, 64, 80, 96, 112, 128};
    }
    else if (nterm == 256)
    {
        g_basePath = "./TP_CCO_Pacote_Dados/TP1_2D/Nterm_256/";
        g_ntermPrefix = "tree2D_Nterm0256_step";
        g_validSteps = {32, 64, 96, 128, 160, 192, 224, 256};
    }

    if (!g_validSteps.empty())
    {
        auto it = std::find(g_validSteps.begin(), g_validSteps.end(), g_current_step);
        if (it != g_validSteps.end())
            g_stepIndex = std::distance(g_validSteps.begin(), it);
        else
            g_stepIndex = g_validSteps.size() - 1;

        g_current_step = g_validSteps[g_stepIndex];
    }
}

void display()
{
    // (Aula 09: Projeções) - cria uma Projeção Ortográfica, define o volume de visualização onde os objetos serão mapeados para a tela
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.5, 1.5, -1.5, 1.5, -1.0, 1.0);

    // (Aula 07: Transformações Geométricas) - uso da Pilha de Matrizes e Transformações 2D
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glTranslatef(g_transX, g_transY, 0.0f); // translação
    glRotatef(g_rotAngulo, 0.0f, 0.0f, 1.0f); // rotação em torno do eixo Z
    glScalef(g_escala, g_escala, 1.0f); // escala Uniforme

    const float shadowOffsetX = 0.005f;
    const float shadowOffsetY = -0.005f;
    const float shadowColorR = 0.2f;
    const float shadowColorG = 0.2f;
    const float shadowColorB = 0.2f;

    for (const auto &s : g_segmentos)
    {
        const Ponto &pa = g_pontos[s.p1_index];
        const Ponto &pb = g_pontos[s.p2_index];

        float halfWidth = s.raio * g_globalRadiusScale;
        if (halfWidth < 0.0008f)
            halfWidth = 0.0008f;

        // sombra
        // (Aula 07: Transformações) - glPushMatrix/popMatrix para isolar a transformação da sombra
        glPushMatrix();
        glTranslatef(shadowOffsetX, shadowOffsetY, 0.0f);
        glColor3f(shadowColorR, shadowColorG, shadowColorB); // atributo de Cor
        drawSegmentAsQuad(pa, pb, halfWidth);
        glPopMatrix();

        // cor fixa
        // (Aula 07: Teoria das Cores) - aplicação da cor RGB
        if (g_useFixedColor)
        {
            glColor3f(g_fixedR, g_fixedG, g_fixedB);
        }
        else
        {
            float rr, gg, bb;
            getColorFromValue(s.raio, rr, gg, bb);
            glColor3f(rr, gg, bb);
        }

        drawSegmentAsQuad(pa, pb, halfWidth);
    }

    glutSwapBuffers(); // (Aula 03: Fundamentos) - double buffering para evitar flickering
}

void reshape(int w, int h)
{
    // (Aula 03: Fundamentos) - ajuste do Viewport 
    glViewport(0, 0, w, h);
}

void keyboard(unsigned char key, int x, int y)
{
    switch (key)
    {
    case '1':
        setResolutionPreset(64);
        loadTree();
        break;

    case '2':
        setResolutionPreset(128);
        loadTree();
        break;

    case '3':
        setResolutionPreset(256);
        loadTree();
        break;

    case '+': // shift +
        if (g_stepIndex + 1 < g_validSteps.size())
        {
            g_stepIndex++;
            g_current_step = g_validSteps[g_stepIndex];
            loadTree();
        }
        break;

    case '-': // -
        if (g_stepIndex > 0)
        {
            g_stepIndex--;
            g_current_step = g_validSteps[g_stepIndex];
            loadTree();
        }
        break;

    // movimento, rotação, zoom
    case 'w':
        g_transY += 0.05f;
        break;
    case 's':
        g_transY -= 0.05f;
        break;
    case 'a':
        g_transX -= 0.05f;
        break;
    case 'd':
        g_transX += 0.05f;
        break;
    case 'q':
        g_rotAngulo += 5.0f;
        break;
    case 'e':
        g_rotAngulo -= 5.0f;
        break;
    case 'z':
        g_escala += 0.1f;
        break;
    case 'x':
        g_escala = std::max(0.1f, g_escala - 0.1f);
        break;

    // escala da largura
    case 'k':
    case 'K':
        g_globalRadiusScale *= 1.2f;
        std::cout << "globalRadiusScale = " << g_globalRadiusScale << std::endl;
        break;

    case 'j':
    case 'J':
        g_globalRadiusScale /= 1.2f;
        std::cout << "globalRadiusScale = " << g_globalRadiusScale << std::endl;
        break;

    // alterna entre cor fixa e cor default
    case 'c':
    case 'C':
        g_useFixedColor = true;
        std::cout << "Modo: COR FIXA\n";
        break;

    case 'v':
    case 'V':
        g_useFixedColor = false;
        std::cout << "Modo: COLORMAP\n";
        break;

    case 'r':
        g_transX = g_transY = 0.0f;
        g_rotAngulo = 0.0f;
        g_escala = 1.0f;
        break;

    case 27:
        exit(0);
        break;
    }

    glutPostRedisplay();
}

int main(int argc, char **argv)
{
    setResolutionPreset(64);

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(800, 800);
    glutCreateWindow("TP1 - Visualizacao 2D Arterial - Colorido");

    init();
    loadTree();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);

    glutMainLoop();
    return 0;
}
