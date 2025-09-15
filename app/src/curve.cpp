#include "curve.h"
#include "extra.h"
#ifdef WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
using namespace std;

namespace
{
    // Approximately equal to.  We don't want to use == because of
    // precision issues with floating point.
    inline bool approx( const Vector3f& lhs, const Vector3f& rhs )
    {
        const float eps = 1e-8f;
        return ( lhs - rhs ).absSquared() < eps;
    }

    
}
    

Curve evalBezier( const vector< Vector3f >& P, unsigned steps )
{
    // Check
    if( P.size() < 4 || P.size() % 3 != 1 )
    {
        cerr << "evalBezier must be called with 3n+1 control points." << endl;
        exit( 0 );
    }

    const float EPS = 1e-8f;

    auto bernstein = [](float t, float &B0, float &B1, float &B2, float &B3){
        float it = 1.0f - t;
        B0 = it*it*it;
        B1 = 3.0f*it*it*t;
        B2 = 3.0f*it*t*t;
        B3 = t*t*t;
    };

    auto bernsteinDeriv = [](const Vector3f& p0, const Vector3f& p1,
                             const Vector3f& p2, const Vector3f& p3, float t){
        float it = 1.0f - t;
        // V'(t) para Bezier cúbica
        return 3.0f*it*it*(p1 - p0) + 6.0f*it*t*(p2 - p1) + 3.0f*t*t*(p3 - p2);
    };

    Curve R;
    R.reserve( ((P.size()-1)/3) * steps + 1 );

    const int numSeg = int(P.size()-1) / 3;

    Vector3f prevT(1,0,0); // será ajustado na primeira amostra
    Vector3f prevB(0,0,1); // idem
    bool hasPrev = false;

    for (int k = 0; k < numSeg; ++k)
    {
        const Vector3f& p0 = P[3*k + 0];
        const Vector3f& p1 = P[3*k + 1];
        const Vector3f& p2 = P[3*k + 2];
        const Vector3f& p3 = P[3*k + 3];

        // evita duplicar o ponto de junção: no primeiro segmento s=0..steps, nos demais s=1..steps
        unsigned sBegin = (k == 0) ? 0u : 1u;

        for (unsigned s = sBegin; s <= steps; ++s)
        {
            float t = (steps == 0) ? 0.0f : float(s) / float(steps);

            // Posição
            float B0, B1, B2, B3;
            bernstein(t, B0, B1, B2, B3);
            Vector3f V = B0*p0 + B1*p1 + B2*p2 + B3*p3;

            // Se ponto repetido por numérico, pule (evita frames ruins)
            if (!R.empty() && approx(R.back().V, V)) continue;

            // Tangente crua e normalizada
            Vector3f dV = bernsteinDeriv(p0,p1,p2,p3,t);
            float dVlen = dV.abs();
            Vector3f T;

            if (dVlen < EPS) {
                // Degenerado: tenta olhar adiante/atrás um pouquinho
                float dt = 1e-3f;
                float t2 = std::min(1.0f, t + dt);
                Vector3f dV2 = bernsteinDeriv(p0,p1,p2,p3,t2);
                if (dV2.abs() < EPS && t > dt) {
                    t2 = t - dt;
                    dV2 = bernsteinDeriv(p0,p1,p2,p3,t2);
                }
                if (dV2.abs() >= EPS) T = dV2.normalized();
                else {
                    // último recurso: reutiliza T anterior ou escolhe arbitrária
                    T = hasPrev ? prevT : Vector3f(1,0,0);
                }
            } else {
                T = dV / dVlen;
            }

            Vector3f N, B;

            if (!hasPrev)
            {
                // Primeira amostra: escolha B0 não paralela a T0
                Vector3f up(0,0,1);
                if (fabs(T.dot(up)) > 0.9f) up = Vector3f(1,0,0);
                N = Vector3f::cross(up, T).normalized();
                if (N.abs() < EPS) {
                    // fallback raro
                    up = Vector3f(0,1,0);
                    N = Vector3f::cross(up, T).normalized();
                }
                B = Vector3f::cross(T, N).normalized();
                hasPrev = true;
            }
            else
            {
                // Transporte "suave" (do PDF): usa B_(i-1) para projetar N atual
                N = Vector3f::cross(prevB, T).normalized();
                if (N.abs() < EPS) {
                    // T quase paralelo a prevB → escolhe um up estável e recomeça
                    Vector3f up(0,0,1);
                    if (fabs(T.dot(up)) > 0.9f) up = Vector3f(1,0,0);
                    N = Vector3f::cross(up, T).normalized();
                }
                B = Vector3f::cross(T, N).normalized();
            }

            CurvePoint cp;
            cp.V = V;
            cp.T = T;
            cp.N = N;
            cp.B = B;

            R.push_back(cp);

            prevT = T;
            prevB = B;
        }
    }

    return R;
}

Curve evalBspline( const vector< Vector3f >& P, unsigned steps )
{
    // Check
    if( P.size() < 4 )
    {
        cerr << "evalBspline must be called with 4 or more control points." << endl;
        exit( 0 );
    }

    // Conversão de cada segmento B-spline (cúbica uniforme) para Bézier cúbica.
    // Segmento i usa b0=P[i], b1=P[i+1], b2=P[i+2], b3=P[i+3].
    // Fórmulas (base-change):
    // q0 = (b0 + 4 b1 + b2)/6
    // q1 = (4 b1 + 2 b2)/6
    // q2 = (2 b1 + 4 b2)/6
    // q3 = (b1 + 4 b2 + b3)/6
    //
    // Depois concatenamos os controles Bézier de todos os segmentos numa lista Q
    // no formato 3n+1: para o primeiro seg push q0,q1,q2,q3; para os demais push q1,q2,q3.

    vector<Vector3f> Q;
    Q.reserve( 3 * (P.size() - 3) + 1 ); // 3*#segments + 1

    const int numSeg = int(P.size()) - 3;

    for (int i = 0; i < numSeg; ++i)
    {
        const Vector3f& b0 = P[i + 0];
        const Vector3f& b1 = P[i + 1];
        const Vector3f& b2 = P[i + 2];
        const Vector3f& b3 = P[i + 3];

        Vector3f q0 = (b0 + 4.0f*b1 + b2) / 6.0f;
        Vector3f q1 = (4.0f*b1 + 2.0f*b2) / 6.0f;
        Vector3f q2 = (2.0f*b1 + 4.0f*b2) / 6.0f;
        Vector3f q3 = (b1 + 4.0f*b2 + b3) / 6.0f;

        if (i == 0) {
            Q.push_back(q0);
            Q.push_back(q1);
            Q.push_back(q2);
            Q.push_back(q3);
        } else {
            // Evita duplicar o ponto de junção: adiciona só q1,q2,q3
            Q.push_back(q1);
            Q.push_back(q2);
            Q.push_back(q3);
        }
    }

    // Agora avaliamos a cadeia Bézier resultante com a sua evalBezier
    return evalBezier(Q, steps);
}


Curve evalCircle( float radius, unsigned steps )
{
    // This is a sample function on how to properly initialize a Curve
    // (which is a vector< CurvePoint >).
    
    // Preallocate a curve with steps+1 CurvePoints
    Curve R( steps+1 );

    // Fill it in counterclockwise
    for( unsigned i = 0; i <= steps; ++i )
    {
        // step from 0 to 2pi
        float t = 2.0f * M_PI * float( i ) / steps;

        // Initialize position
        // We're pivoting counterclockwise around the y-axis
        R[i].V = radius * Vector3f( cos(t), sin(t), 0 );
        
        // Tangent vector is first derivative
        R[i].T = Vector3f( -sin(t), cos(t), 0 );
        
        // Normal vector is second derivative
        R[i].N = Vector3f( -cos(t), -sin(t), 0 );

        // Finally, binormal is facing up.
        R[i].B = Vector3f( 0, 0, 1 );
    }

    return R;
}

void drawCurve( const Curve& curve, float framesize )
{
    // Save current state of OpenGL
    glPushAttrib( GL_ALL_ATTRIB_BITS );

    // Setup for line drawing
    glDisable( GL_LIGHTING ); 
    glColor4f( 1, 1, 1, 1 );
    glLineWidth( 1 );
    
    // Draw curve
    glBegin( GL_LINE_STRIP );
    for( unsigned i = 0; i < curve.size(); ++i )
    {
        glVertex( curve[ i ].V );
    }
    glEnd();

    glLineWidth( 1 );

    // Draw coordinate frames if framesize nonzero
    if( framesize != 0.0f )
    {
        Matrix4f M;

        for( unsigned i = 0; i < curve.size(); ++i )
        {
            M.setCol( 0, Vector4f( curve[i].N, 0 ) );
            M.setCol( 1, Vector4f( curve[i].B, 0 ) );
            M.setCol( 2, Vector4f( curve[i].T, 0 ) );
            M.setCol( 3, Vector4f( curve[i].V, 1 ) );

            glPushMatrix();
            glMultMatrixf( M );
            glScaled( framesize, framesize, framesize );
            glBegin( GL_LINES );
            glColor3f( 1, 0, 0 ); glVertex3d( 0, 0, 0 ); glVertex3d( 1, 0, 0 );
            glColor3f( 0, 1, 0 ); glVertex3d( 0, 0, 0 ); glVertex3d( 0, 1, 0 );
            glColor3f( 0, 0, 1 ); glVertex3d( 0, 0, 0 ); glVertex3d( 0, 0, 1 );
            glEnd();
            glPopMatrix();
        }
    }
    
    // Pop state
    glPopAttrib();
}

