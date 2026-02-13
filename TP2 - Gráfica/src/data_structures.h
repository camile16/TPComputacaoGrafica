#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include <vector>

// representa um local no espaco 3d (coordenadas x, y, z)
struct Ponto {
    float x, y, z;
};

// representa um pedaco do vaso sanguineo (um tubo)
// conecta dois pontos (p1 e p2) e tem uma espessura (raio)
struct Segmento {
    int p1_index; // indice do ponto de inicio
    int p2_index; // indice do ponto final
    float raio;   // espessura media desse pedaco
};

// lista global que guarda a espessura exata em cada ponto da arvore
// usada para fazer o vaso ficar grosso no começo e fino no final
extern std::vector<float> g_radii_points; 

#endif