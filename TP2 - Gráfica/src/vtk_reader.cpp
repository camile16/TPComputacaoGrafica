#include "vtk_reader.h"
#include "data_structures.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>

using namespace std;

// Definição da variável global declarada no header
vector<float> g_radii_points;

static string lower(const string &s) {
    string out;
    for (char c : s) out.push_back((char)tolower((unsigned char)c));
    return out;
}

void readVTKFile(const string &base_path, const string &nterm_prefix, int step_number, vector<Ponto> &pontos, vector<Segmento> &segmentos) {
    pontos.clear();
    segmentos.clear();
    g_radii_points.clear();

    stringstream filename_ss;
    filename_ss << base_path << nterm_prefix << setw(4) << setfill('0') << step_number << ".vtk";
    string filename = filename_ss.str();

    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "ERRO: Arquivo nao encontrado: " << filename << endl;
        return;
    }

    string line;
    vector<float> radii_points_tmp;

    while (getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        stringstream ss(line);
        string keyword;
        ss >> keyword;
        string keyl = lower(keyword);

        if (keyl == "points") {
            int num; ss >> num;
            for (int i = 0; i < num; i++) {
                float x, y, z;
                file >> x >> y >> z;
                pontos.push_back({x, y, z});
            }
        } else if (keyl == "lines") {
            int numLines, total; ss >> numLines >> total;
            for (int i = 0; i < numLines; i++) {
                int count, p1, p2;
                file >> count >> p1 >> p2;
                segmentos.push_back({p1, p2, 0.0f});
            }
        } else if (keyl == "point_data") {
            int count; ss >> count;
            string next;
            while (file >> next) { // Busca palavra por palavra em vez de linha por linha
                if (lower(next) == "scalars") {
                    string arrayName, type;
                    file >> arrayName >> type; // Lendo nome e tipo
                    file >> next; // Lendo "LOOKUP_TABLE"
                    file >> next; // Lendo o nome da tabela (ex: "default")
                    
                    for (int i = 0; i < count; i++) {
                        float r;
                        if (file >> r) radii_points_tmp.push_back(r);
                    }
                    break;
                }
            }
        }
    }
    file.close();

    g_radii_points = radii_points_tmp;

    // Normalização 3D (X, Y e Z)
    if (!pontos.empty()) {
        float minX = pontos[0].x, maxX = pontos[0].x;
        float minY = pontos[0].y, maxY = pontos[0].y;
        float minZ = pontos[0].z, maxZ = pontos[0].z;

        for (auto &p : pontos) {
            minX = min(minX, p.x); maxX = max(maxX, p.x);
            minY = min(minY, p.y); maxY = max(maxY, p.y);
            minZ = min(minZ, p.z); maxZ = max(maxZ, p.z);
        }

        float midX = (maxX + minX) * 0.5f;
        float midY = (maxY + minY) * 0.5f;
        float midZ = (maxZ + minZ) * 0.5f;
        float scale = max({maxX - minX, maxY - minY, maxZ - minZ}) * 0.5f;

        for (auto &p : pontos) {
            p.x = (p.x - midX) / scale;
            p.y = (p.y - midY) / scale;
            p.z = (p.z - midZ) / scale;
        }
    }

    // Calcula raio médio para os segmentos (compatibilidade com cores)
    if (!g_radii_points.empty()) {
        for (auto &s : segmentos) {
            s.raio = (g_radii_points[s.p1_index] + g_radii_points[s.p2_index]) * 0.5f;
        }
    }
}