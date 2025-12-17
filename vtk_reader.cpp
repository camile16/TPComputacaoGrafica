#include "vtk_reader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

using namespace std;

static string lower(const string &s)
{
    string out;
    out.reserve(s.size());
    for (char c : s)
        out.push_back((char)tolower((unsigned char)c));
    return out;
}

// (Aula 06: Modelagem de Objetos)- processo de leitura de estrutura de dados (Pontos e Conectividade)
void readVTKFile(const string &base_path, const string &nterm_prefix, int step_number, vector<Ponto> &pontos, vector<Segmento> &segmentos)
{
    pontos.clear();
    segmentos.clear();

    stringstream filename_ss;
    filename_ss
        << base_path
        << nterm_prefix 
        << setw(4) << setfill('0') << step_number
        << ".vtk";

    string filename = filename_ss.str();

    ifstream file(filename);
    if (!file.is_open())
    {
        cerr << "ERRO: Não foi possível abrir: " << filename << endl;
        return;
    }

    string line;
    int num_points = 0;

    vector<float> radii_cells;
    vector<float> radii_points;

    while (getline(file, line))
    {
        if (line.empty())
            continue;
        if (line[0] == '#')
            continue;

        stringstream ss(line);
        string keyword;
        ss >> keyword;
        string keyl = lower(keyword);

        if (keyl == "points")
        {
            ss >> num_points;
            string type;
            ss >> type;

            pontos.reserve(num_points);

            for (int i = 0; i < num_points; i++)
            {
                float x, y, z;
                if (!(file >> x >> y >> z))
                {
                    cerr << "ERRO lendo coordenadas dos pontos!\n";
                    return;
                }
                pontos.push_back({x, y, z});
            }

            getline(file, line);
        }

        else if (keyl == "lines")
        {
            int numLines, totalValues;
            ss >> numLines >> totalValues;

            segmentos.reserve(numLines);

            for (int i = 0; i < numLines; i++)
            {
                int count;
                file >> count;

                vector<int> idx(count);
                for (int k = 0; k < count; k++)
                    file >> idx[k];

                for (int k = 0; k < count - 1; k++)
                    segmentos.push_back({idx[k], idx[k + 1], 0.0f});
            }

            getline(file, line);
        }

        else if (keyl == "cell_data" || keyl == "point_data")
        {
            int count;
            ss >> count;

            streampos savedPos = file.tellg();
            string next;

            while (getline(file, next))
            {
                if (next.empty())
                    continue;

                stringstream ss2(next);
                string kw;
                ss2 >> kw;

                if (lower(kw) == "scalars")
                {
                    string arrayName, type;
                    ss2 >> arrayName >> type;

                    getline(file, next); 

                    if (lower(keyword) == "cell_data")
                    {
                        radii_cells.reserve(count);
                        for (int i = 0; i < count; i++)
                        {
                            float r;
                            file >> r;
                            radii_cells.push_back(r);
                        }
                        getline(file, next);
                        break;
                    }
                    else
                    {
                        radii_points.reserve(count);
                        for (int i = 0; i < count; i++)
                        {
                            float r;
                            file >> r;
                            radii_points.push_back(r);
                        }
                        getline(file, next);
                        break;
                    }
                }

                savedPos = file.tellg();
            }
        }
    }

    file.close();

    // atribui os raios
    if (!radii_cells.empty())
    {
        for (size_t i = 0; i < radii_cells.size() && i < segmentos.size(); i++)
            segmentos[i].raio = radii_cells[i];
    }
    else if (!radii_points.empty())
    {
        for (auto &s : segmentos)
        {
            float ra = radii_points[s.p1_index];
            float rb = radii_points[s.p2_index];
            s.raio = (ra + rb) * 0.5f;
        }
    }

    // normalizacao dos pontos para [-1,1]
    if (!pontos.empty())
    {
        float minX = pontos[0].x;
        float maxX = pontos[0].x;
        float minY = pontos[0].y;
        float maxY = pontos[0].y;

        for (auto &p : pontos)
        {
            minX = min(minX, p.x);
            maxX = max(maxX, p.x);
            minY = min(minY, p.y);
            maxY = max(maxY, p.y);
        }

        float rangeX = maxX - minX;
        float rangeY = maxY - minY;
        float scale = max(rangeX, rangeY);
        float halfRange = scale * 0.5f;

        float midX = (maxX + minX) * 0.5f;
        float midY = (maxY + minY) * 0.5f;

        // (Aula 08: Operações e Posições) - normalização de coordenadas
        // transforma as coordenadas originais do arquivo para o intervalo [-1, 1]
        for (auto &p : pontos)
        {
            p.x = (p.x - midX) / halfRange;
            p.y = (p.y - midY) / halfRange;
        }
    }
}