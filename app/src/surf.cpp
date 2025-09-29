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

    const unsigned m = (unsigned)profile.size();
    if (m < 2 || steps < 3) {
        return surface;
    }

    const unsigned rings = steps + 1;

    surface.VV.reserve(rings * m);
    surface.VN.reserve(rings * m);
    surface.VF.reserve((steps) * (m - 1) * 2); 

    auto rotY = [](const Vector3f& v, float c, float s) -> Vector3f {
        return Vector3f(v.x()*c + v.z()*s, v.y(), -v.x()*s + v.z()*c);
    };

    for (unsigned s = 0; s < rings; ++s)
    {
        float theta = 2.0f * M_PI * float(s) / float(steps);
        float c = cosf(theta);
        float si = sinf(theta);

        for (unsigned i = 0; i < m; ++i)
        {
            Vector3f P = profile[i].V;

            Vector3f Np = profile[i].N;

            Vector3f Vr = rotY(P, c, si);
            Vector3f Nr = rotY(Np, c, si).normalized();

            surface.VV.push_back(Vr);
            surface.VN.push_back(Nr);
        }
    }

    auto vid = [m](unsigned s, unsigned i) -> unsigned {
        return s * m + i;
    };

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

    const unsigned m = (unsigned)profile.size(); 
    const unsigned n = (unsigned)sweep.size();   

    if (m < 2 || n < 2) {
        return surface;
    }

    surface.VV.reserve(m * n);
    surface.VN.reserve(m * n);
    surface.VF.reserve((n - 1) * (m - 1) * 2);

    for (unsigned s = 0; s < n; ++s)
    {
        const Vector3f& Vs = sweep[s].V; 
        const Vector3f& Ts = sweep[s].T; 
        const Vector3f& Ns = sweep[s].N; 
        const Vector3f& Bs = sweep[s].B; 

        for (unsigned i = 0; i < m; ++i)
        {
            const Vector3f& P = profile[i].V; 
            const Vector3f& Np = profile[i].N;

            Vector3f Vr = Vs + P.x() * Ns + P.y() * Bs;

            Vector3f Nr = (Np.x() * Ns + Np.y() * Bs).normalized();

            surface.VV.push_back(Vr);
            surface.VN.push_back(Nr);
        }
    }

    auto vid = [m](unsigned s, unsigned i) -> unsigned {
        return s * m + i;
    };

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
