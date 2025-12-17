#ifndef VTK_READER_H
#define VTK_READER_H

#include "data_structures.h"
#include <vector>
#include <string> 

using namespace std;

void readVTKFile(const string &base_path, const string &nterm_prefix, int step, vector<Ponto> &pontos, vector<Segmento> &segmentos);

#endif // VTK_READER_H