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

// lista global de espessuras
vector<float> g_radii_points;

// funcao simples para transformar texto em minusculo
// ajuda a ler o arquivo mesmo se estiver escrito diferente
static string lower(const string &s) {
    string out;
    for (char c : s) out.push_back((char)tolower((unsigned char)c));
    return out;
}

// funcao principal que le os arquivos vtk
void readVTKFile(const string &base_path, const string &nterm_prefix, int step_number, vector<Ponto> &pontos, vector<Segmento> &segmentos) {
    // limpa os dados antigos para nao misturar com a nova arvore
    pontos.clear();
    segmentos.clear();
    g_radii_points.clear();

    // constroi o nome do arquivo juntando as partes do texto e o numero
    stringstream filename_ss;
    filename_ss << base_path << nterm_prefix << setw(4) << setfill('0') << step_number << ".vtk";
    string filename = filename_ss.str();

    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "erro: nao achei o arquivo: " << filename << endl;
        return;
    } else {
        cout << "lendo arquivo: " << filename << endl;
    }

    string line;
    vector<float> radii_points_tmp;

    while (getline(file, line)) {
        if (line.empty() || line[0] == '#') continue; 
        stringstream ss(line);
        string keyword;
        ss >> keyword;
        string keyl = lower(keyword);

        // se encontrar points le as coordenadas x, y, z
        if (keyl == "points") {
            int num; ss >> num;
            for (int i = 0; i < num; i++) {
                float x, y, z;
                file >> x >> y >> z;
                pontos.push_back({x, y, z});
            }
        } 
        // se encontrar lines le como os pontos se conectam
        else if (keyl == "lines") {
            int numLines, total; ss >> numLines >> total;
            for (int i = 0; i < numLines; i++) {
                int count, p1, p2;
                file >> count >> p1 >> p2; // le quais pontos formam a linha
                segmentos.push_back({p1, p2, 0.0f});
            }
        } 
        // se encontrar scalars le a espessura de cada ponto
        else if (keyl == "point_data") {
            int count; ss >> count;
            string next;
            while (file >> next) { 
                if (lower(next) == "scalars") {
                    // pula cabecalhos inuteis
                    string ignore; 
                    file >> ignore >> ignore >> ignore >> ignore; 
                    
                    for (int i = 0; i < count; i++) {
                        float r;
                        file >> r;
                        radii_points_tmp.push_back(r);
                    }
                    break;
                }
            }
        }
    }
    file.close();

    g_radii_points = radii_points_tmp;

    // parte de ajuste (normalizacao)
    // encolhe a arvore para caber num cubo pequeno entre -1 e 1
    // isso facilita posicionar a camera
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
        
        float maxDim = max({maxX - minX, maxY - minY, maxZ - minZ});
        float scale = (maxDim > 0) ? (2.0f / maxDim) : 1.0f;

        // aplica o encolhimento e centraliza todos os pontos
        for (auto &p : pontos) {
            p.x = (p.x - midX) * scale;
            p.y = (p.y - midY) * scale;
            p.z = (p.z - midZ) * scale;
        }

        // importante: tambem encolhe a espessura dos galhos
        for (float &r : g_radii_points) {
            r *= scale; 
        }
        
        // calcula uma media de raio para usar de reserva
        for (auto &s : segmentos) {
            float r1 = (s.p1_index < g_radii_points.size()) ? g_radii_points[s.p1_index] : 0.01f;
            float r2 = (s.p2_index < g_radii_points.size()) ? g_radii_points[s.p2_index] : 0.01f;
            s.raio = (r1 + r2) * 0.5f;
        }
    }
}