#include "vtk_reader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

// ===============================
// REMOVIDO: O caminho base agora é passado como parâmetro
// ===============================

static std::string lower(const std::string &s)
{
    std::string out;
    out.reserve(s.size());
    for (char c : s)
        out.push_back((char)std::tolower((unsigned char)c));
    return out;
}

// A função agora recebe base_path e nterm_prefix
void readVTKFile(const std::string &base_path, const std::string &nterm_prefix, int step_number, std::vector<Ponto> &pontos, std::vector<Segmento> &segmentos)
{
    pontos.clear();
    segmentos.clear();

    // monta o nome completo do arquivo usando os parâmetros
    std::stringstream filename_ss;
    filename_ss
        << base_path
        << nterm_prefix // Agora é dinâmico
        << std::setw(4) << std::setfill('0') << step_number
        << ".vtk";

    std::string filename = filename_ss.str();

    std::cout << "Carregando arquivo: " << filename << std::endl;

    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "ERRO: Não foi possível abrir: " << filename << std::endl;
        return;
    }

    std::string line;
    int num_points = 0;

    std::vector<float> radii_cells;
    std::vector<float> radii_points;

    while (std::getline(file, line))
    {
        if (line.empty())
            continue;
        if (line[0] == '#')
            continue;

        std::stringstream ss(line);
        std::string keyword;
        ss >> keyword;
        std::string keyl = lower(keyword);

        // ================
        // LENDO POINTS
        // ================
        if (keyl == "points")
        {
            ss >> num_points;
            std::string type;
            ss >> type;

            pontos.reserve(num_points);

            for (int i = 0; i < num_points; i++)
            {
                float x, y, z;
                if (!(file >> x >> y >> z))
                {
                    std::cerr << "ERRO lendo coordenadas dos pontos!\n";
                    return;
                }
                pontos.push_back({x, y, z});
            }

            std::getline(file, line); // limpeza
        }

        // ================
        // LENDO LINES
        // ================
        else if (keyl == "lines")
        {
            int numLines, totalValues;
            ss >> numLines >> totalValues;

            segmentos.reserve(numLines);

            for (int i = 0; i < numLines; i++)
            {
                int count;
                file >> count;

                std::vector<int> idx(count);
                for (int k = 0; k < count; k++)
                    file >> idx[k];

                // decompor
                for (int k = 0; k < count - 1; k++)
                    segmentos.push_back({idx[k], idx[k + 1], 0.0f});
            }

            std::getline(file, line);
        }

        // ======================
        // CELL_DATA ou POINT_DATA
        // ======================
        else if (keyl == "cell_data" || keyl == "point_data")
        {
            int count;
            ss >> count;

            std::streampos savedPos = file.tellg();
            std::string next;

            while (std::getline(file, next))
            {
                if (next.empty())
                    continue;

                std::stringstream ss2(next);
                std::string kw;
                ss2 >> kw;

                if (lower(kw) == "scalars")
                {
                    std::string arrayName, type;
                    ss2 >> arrayName >> type;

                    std::getline(file, next); // LOOKUP_TABLE

                    if (lower(keyword) == "cell_data")
                    {
                        radii_cells.reserve(count);
                        for (int i = 0; i < count; i++)
                        {
                            float r;
                            file >> r;
                            radii_cells.push_back(r);
                        }
                        std::getline(file, next);
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
                        std::getline(file, next);
                        break;
                    }
                }

                savedPos = file.tellg();
            }
        }
    }

    file.close();

    // Atribui os raios
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

    // Normalização dos pontos para [-1,1]
    if (!pontos.empty())
    {
        float minX = pontos[0].x;
        float maxX = pontos[0].x;
        float minY = pontos[0].y;
        float maxY = pontos[0].y;

        for (auto &p : pontos)
        {
            minX = std::min(minX, p.x);
            maxX = std::max(maxX, p.x);
            minY = std::min(minY, p.y);
            maxY = std::max(maxY, p.y);
        }

        float rangeX = maxX - minX;
        float rangeY = maxY - minY;
        float scale = std::max(rangeX, rangeY);
        float halfRange = scale * 0.5f;

        float midX = (maxX + minX) * 0.5f;
        float midY = (maxY + minY) * 0.5f;

        for (auto &p : pontos)
        {
            p.x = (p.x - midX) / halfRange;
            p.y = (p.y - midY) / halfRange;
        }
    }

    std::cout << "Leitura OK. Pontos: " << pontos.size()
              << " Segmentos: " << segmentos.size() << std::endl;
}