#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include <vector>

// representa um vertice no espaço 3D
struct Ponto
{
    float x, y, z;
};

// representa uma aresta (um vaso sanguineo)
struct Segmento
{
    int p1_index; // indice do ponto inicial no vetor g_pontos
    int p2_index; // indice do ponto final no vetor g_pontos
    float raio; // espessura do vaso
};

#endif // DATA_STRUCTURES_H
