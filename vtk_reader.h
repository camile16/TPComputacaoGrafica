#ifndef VTK_READER_H
#define VTK_READER_H

#include "data_structures.h"
#include <vector>
#include <string> // Adicionado para usar std::string na assinatura

// Nova assinatura: aceita o caminho base e o prefixo da série (ex: "tree2D_Nterm0064_step")
void readVTKFile(const std::string &base_path, const std::string &nterm_prefix, int step, std::vector<Ponto> &pontos, std::vector<Segmento> &segmentos);

#endif // VTK_READER_H