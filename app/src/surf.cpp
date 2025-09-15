#include "surf.h"
#include "extra.h"
using namespace std;

namespace
{
    
    // We're only implenting swept surfaces where the profile curve is
    // flat on the xy-plane.  This is a check function.
    static bool checkFlat(const Curve &profile)
    {
        for (unsigned i=0; i<profile.size(); i++)
            if (profile[i].V[2] != 0.0 ||
                profile[i].T[2] != 0.0 ||
                profile[i].N[2] != 0.0)
                return false;
    
        return true;
    }
}

Surface makeSurfRev(const Curve &profile, unsigned steps)
{
    Surface surface;

    if (!checkFlat(profile))
    {
        cerr << "surfRev profile curve must be flat on xy plane." << endl;
        exit(0);
    }

    // Quantidade de amostras ao longo do perfil
    const unsigned m = (unsigned)profile.size();
    if (m < 2 || steps < 3) {
        // precisa de pelo menos 2 pontos no perfil e 3 fatias de revolução
        return surface;
    }

    // Vamos criar (steps + 1) anéis para fechar a costura (s=0 e s=steps coincidem)
    const unsigned rings = steps + 1;

    surface.VV.reserve(rings * m);
    surface.VN.reserve(rings * m);
    surface.VF.reserve((steps) * (m - 1) * 2); // 2 triângulos por quad

    auto rotY = [](const Vector3f& v, float c, float s) -> Vector3f {
        // Rotação em torno do eixo Y:
        // (x, y, z) -> ( x*c + z*s,  y,  -x*s + z*c )
        return Vector3f(v.x()*c + v.z()*s, v.y(), -v.x()*s + v.z()*c);
    };

    // --- 1) Gerar vértices e normais por rotação rígida do perfil ---
    for (unsigned s = 0; s < rings; ++s)
    {
        float theta = 2.0f * M_PI * float(s) / float(steps);
        float c = cosf(theta);
        float si = sinf(theta);

        for (unsigned i = 0; i < m; ++i)
        {
            // Posição do perfil (no plano XY; z≈0)
            Vector3f P = profile[i].V;

            // Normal do perfil: para perfil 2D, o PDF define N apontando para a esquerda do movimento.
            // A normal de superfície para revolução é a rotação rígida da normal do perfil.
            Vector3f Np = profile[i].N;

            // Gera ponto e normal rotacionados
            Vector3f Vr = rotY(P, c, si);
            Vector3f Nr = rotY(Np, c, si).normalized();

            surface.VV.push_back(Vr);
            surface.VN.push_back(Nr);
        }
    }

    // Função para indexar (anel s, ponto do perfil i)
    auto vid = [m](unsigned s, unsigned i) -> unsigned {
        return s * m + i;
    };

    // --- 2) Gerar faces (triângulos), conectando anéis s e s+1 ---
    // Malha “reticulada”:
    // a = (s,   i)
    // b = (s+1, i)
    // c = (s+1, i+1)
    // d = (s,   i+1)
    // Triângulos: (a, b, c) e (a, c, d) em ordem CCW (vistos de fora)
    for (unsigned s = 0; s < steps; ++s)
    {
        for (unsigned i = 0; i < m - 1; ++i)
        {
            unsigned a = vid(s,   i);
            unsigned b = vid(s+1, i);
            unsigned c = vid(s+1, i+1);
            unsigned d = vid(s,   i+1);

            surface.VF.push_back(Tup3u(a, b, c));
            surface.VF.push_back(Tup3u(a, c, d));
        }
    }

    return surface;
}


Surface makeGenCyl(const Curve &profile, const Curve &sweep )
{
    Surface surface;

    if (!checkFlat(profile))
    {
        cerr << "genCyl profile curve must be flat on xy plane." << endl;
        exit(0);
    }

    const unsigned m = (unsigned)profile.size(); // pontos no perfil
    const unsigned n = (unsigned)sweep.size();   // amostras ao longo da varredura

    if (m < 2 || n < 2) {
        // nada a fazer
        return surface;
    }

    surface.VV.reserve(m * n);
    surface.VN.reserve(m * n);
    surface.VF.reserve((n - 1) * (m - 1) * 2);

    // --- 1) Geração de vértices e normais ---
    // Mapeamos perfil (x,y,0) no frame do sweep: x->N, y->B; centro na posição V do sweep.
    for (unsigned s = 0; s < n; ++s)
    {
        const Vector3f& Vs = sweep[s].V; // origem do frame no espaço
        const Vector3f& Ts = sweep[s].T; // não usado para posicionar perfil, mas define orientação do frame
        const Vector3f& Ns = sweep[s].N; // eixo "x" local
        const Vector3f& Bs = sweep[s].B; // eixo "y" local

        for (unsigned i = 0; i < m; ++i)
        {
            const Vector3f& P = profile[i].V; // (x, y, 0) no plano XY
            const Vector3f& Np = profile[i].N; // (nx, ny, 0) normal 2D do perfil

            // posição: leva x ao longo de N, y ao longo de B
            Vector3f Vr = Vs + P.x() * Ns + P.y() * Bs;

            // normal: combina nx*N + ny*B (nada de componente em T)
            Vector3f Nr = (Np.x() * Ns + Np.y() * Bs).normalized();

            surface.VV.push_back(Vr);
            surface.VN.push_back(Nr);
        }
    }

    // Indexador (anel s, ponto i)
    auto vid = [m](unsigned s, unsigned i) -> unsigned {
        return s * m + i;
    };

    // --- 2) Geração das faces (triângulos) ligando anéis consecutivos ---
    // Para cada quad:
    // a = (s,   i)
    // b = (s+1, i)
    // c = (s+1, i+1)
    // d = (s,   i+1)
    // Triângulos CCW: (a,b,c) e (a,c,d)
    for (unsigned s = 0; s < n - 1; ++s)
    {
        for (unsigned i = 0; i < m - 1; ++i)
        {
            unsigned a = vid(s,   i);
            unsigned b = vid(s+1, i);
            unsigned c = vid(s+1, i+1);
            unsigned d = vid(s,   i+1);

            surface.VF.push_back(Tup3u(a, b, c));
            surface.VF.push_back(Tup3u(a, c, d));
        }
    }

    return surface;
}


void drawSurface(const Surface &surface, bool shaded)
{
    // Save current state of OpenGL
    glPushAttrib(GL_ALL_ATTRIB_BITS);

    if (shaded)
    {
        // This will use the current material color and light
        // positions.  Just set these in drawScene();
        glEnable(GL_LIGHTING);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        // This tells openGL to *not* draw backwards-facing triangles.
        // This is more efficient, and in addition it will help you
        // make sure that your triangles are drawn in the right order.
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    }
    else
    {        
        glDisable(GL_LIGHTING);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        
        glColor4f(0.4f,0.4f,0.4f,1.f);
        glLineWidth(1);
    }

    glBegin(GL_TRIANGLES);
    for (unsigned i=0; i<surface.VF.size(); i++)
    {
        glNormal(surface.VN[surface.VF[i][0]]);
        glVertex(surface.VV[surface.VF[i][0]]);
        glNormal(surface.VN[surface.VF[i][1]]);
        glVertex(surface.VV[surface.VF[i][1]]);
        glNormal(surface.VN[surface.VF[i][2]]);
        glVertex(surface.VV[surface.VF[i][2]]);
    }
    glEnd();

    glPopAttrib();
}

void drawNormals(const Surface &surface, float len)
{
    // Save current state of OpenGL
    glPushAttrib(GL_ALL_ATTRIB_BITS);

    glDisable(GL_LIGHTING);
    glColor4f(0,1,1,1);
    glLineWidth(1);

    glBegin(GL_LINES);
    for (unsigned i=0; i<surface.VV.size(); i++)
    {
        glVertex(surface.VV[i]);
        glVertex(surface.VV[i] + surface.VN[i] * len);
    }
    glEnd();

    glPopAttrib();
}

void outputObjFile(ostream &out, const Surface &surface)
{
    
    for (unsigned i=0; i<surface.VV.size(); i++)
        out << "v  "
            << surface.VV[i][0] << " "
            << surface.VV[i][1] << " "
            << surface.VV[i][2] << endl;

    for (unsigned i=0; i<surface.VN.size(); i++)
        out << "vn "
            << surface.VN[i][0] << " "
            << surface.VN[i][1] << " "
            << surface.VN[i][2] << endl;

    out << "vt  0 0 0" << endl;
    
    for (unsigned i=0; i<surface.VF.size(); i++)
    {
        out << "f  ";
        for (unsigned j=0; j<3; j++)
        {
            unsigned a = surface.VF[i][j]+1;
            out << a << "/" << "1" << "/" << a << " ";
        }
        out << endl;
    }
}
