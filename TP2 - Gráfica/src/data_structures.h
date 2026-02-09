#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include <vector>

struct Ponto {
    float x, y, z;
};

struct Segmento {
    int p1_index;
    int p2_index;
    float raio;
};

//Armazena o raio específico de cada vértice para espessura variável
extern std::vector<float> g_radii_points; 

#endif