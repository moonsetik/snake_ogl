#include "ObjLoader.h"

#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

static ObjNormal computeFaceNormal(const std::list<ObjVertex>& vs)
{
    ObjNormal n;
    n.x = 0; n.y = 0; n.z = 1;

    if (vs.size() < 3) return n;

    auto it = vs.begin();
    const ObjVertex a = *it; ++it;
    const ObjVertex b = *it; ++it;
    const ObjVertex c = *it;

    double ux = b.x - a.x, uy = b.y - a.y, uz = b.z - a.z;
    double vx = c.x - a.x, vy = c.y - a.y, vz = c.z - a.z;

    n.x = uy * vz - uz * vy;
    n.y = uz * vx - ux * vz;
    n.z = ux * vy - uy * vx;

    double len = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
    if (len > 1e-12) { n.x /= len; n.y /= len; n.z /= len; }
    else { n.x = 0;    n.y = 0;    n.z = 1; }

    return n;
}

void renderModelWithFallback(std::list<ObjFace>& Faces, int mode)
{
    for (auto it = Faces.begin(); it != Faces.end(); ++it)
    {
        const size_t vcount = it->vertex.size();
        const bool hasFullNormals = (it->normal.size() == vcount);
        const bool hasFullTex = (it->texCoord.size() == vcount);

        ObjNormal faceNormal = computeFaceNormal(it->vertex);

        glBegin(mode);
        glNormal3d(faceNormal.x, faceNormal.y, faceNormal.z);

        auto it_n = it->normal.begin();
        auto it_t = it->texCoord.begin();

        for (auto j = it->vertex.begin(); j != it->vertex.end(); ++j)
        {
            if (hasFullNormals)
                glNormal3dv((it_n++)->_ptr());

            if (hasFullTex)
                glTexCoord2dv((it_t++)->_ptr());

            glVertex4dv(j->_ptr());
        }

        glEnd();
    }

    Faces.clear();
}

int ObjModel::LoadModel(const char* filename)
{
    vector<ObjVertex>  V;
    vector<ObjTexCord> VT;
    vector<ObjNormal>  VN;

    ifstream fin(filename);
    if (!fin.is_open())
        throw std::runtime_error(std::string("File ") + filename + " can`t be opened!");

    string line;
    while (std::getline(fin, line))
    {
        istringstream isstr(line);
        string mode;
        isstr >> mode;

        if (mode == "v")
        {
            ObjVertex v;
            isstr >> v.x >> v.y >> v.z;
            V.push_back(v);
        }
        else if (mode == "vt")
        {
            ObjTexCord t;
            isstr >> t.u >> t.v;
            VT.push_back(t);
        }
        else if (mode == "vn")
        {
            ObjNormal n;
            isstr >> n.x >> n.y >> n.z;
            VN.push_back(n);
        }
        else if (mode == "f")
        {
            string face;
            ObjFace f;
            f.VertexCount = 0;

            for (isstr >> face; isstr; isstr >> face)
            {
                istringstream tokStream(face);
                string digit;
                int n[3] = { 0, 0, 0 };
                int i = 0;
                while (std::getline(tokStream, digit, '/'))
                {
                    if (!digit.empty())
                        n[i] = std::stoi(digit);
                    ++i;
                    if (i == 3) break;
                }

                if (n[0] > 0 && (size_t)n[0] <= V.size())
                    f.vertex.push_back(V[n[0] - 1]);
                if (n[1] > 0 && (size_t)n[1] <= VT.size())
                    f.texCoord.push_back(VT[n[1] - 1]);
                if (n[2] > 0 && (size_t)n[2] <= VN.size())
                    f.normal.push_back(VN[n[2] - 1]);

                f.VertexCount++;
            }

            Faces.push_back(f);
        }
    }

    glDeleteLists(listId, 1);
    listId = glGenLists(1);

    glNewList(listId, GL_COMPILE);
    renderModelWithFallback(Faces, GL_POLYGON);
    glEndList();

    return 1;
}