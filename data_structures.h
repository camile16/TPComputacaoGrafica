#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include <vector>

struct Ponto
{
    float x, y, z;
};

struct Segmento
{
    int p1_index;
    int p2_index;
    float raio; // valor vindo do arquivo (não normalizado junto com pontos)
};

#endif // DATA_STRUCTURES_H