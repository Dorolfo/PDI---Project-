#include "curve.h"
#include "extra.h"
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

    // Number of Bezier pieces (segments)
    unsigned numPieces = (P.size() - 1) / 3;
    
    // Total number of points to generate
    unsigned totalPoints = numPieces * steps + 1;
    
    // Preallocate curve
    Curve R(totalPoints);
    
    // Keep track of previous binormal for continuity
    Vector3f prevB;
    bool firstSegment = true;
    
    // Generate points for each Bezier piece
    for (unsigned piece = 0; piece < numPieces; ++piece)
    {
        // Get control points for this piece
        Vector3f P0 = P[3 * piece];
        Vector3f P1 = P[3 * piece + 1];
        Vector3f P2 = P[3 * piece + 2];
        Vector3f P3 = P[3 * piece + 3];
        
        // Generate points for this piece
        for (unsigned i = 0; i <= steps; ++i)
        {
            // Skip the last point of non-final pieces to avoid duplication
            if (piece < numPieces - 1 && i == steps) continue;
            
            float t = float(i) / float(steps);
            
            // Compute position using De Casteljau's algorithm / Bernstein polynomials
            float u = 1.0f - t;
            float tt = t * t;
            float uu = u * u;
            float uuu = uu * u;
            float ttt = tt * t;
            
            Vector3f V = uuu * P0 + 3 * uu * t * P1 + 3 * u * tt * P2 + ttt * P3;
            
            // Compute first derivative (tangent direction)
            Vector3f dV = 3 * uu * (P1 - P0) + 6 * u * t * (P2 - P1) + 3 * tt * (P3 - P2);
            
            // Compute second derivative (for normal calculation)
            Vector3f ddV = 6 * u * (P2 - 2 * P1 + P0) + 6 * t * (P3 - 2 * P2 + P1);
            
            // Normalize tangent
            Vector3f T = dV.normalized();
            
            // Calculate normal using cross product of T and second derivative
            Vector3f N;
            if (ddV.abs() > 1e-8f)
            {
                // Use Frenet frame: N = (T × (T × ddV)).normalized()
                Vector3f temp = Vector3f::cross(T, ddV);
                if (temp.abs() > 1e-8f)
                {
                    N = Vector3f::cross(temp, T).normalized();
                }
                else
                {
                    // Fallback: create arbitrary perpendicular vector
                    if (abs(T.x()) < 0.9f)
                        N = Vector3f::cross(T, Vector3f(1, 0, 0)).normalized();
                    else
                        N = Vector3f::cross(T, Vector3f(0, 1, 0)).normalized();
                }
            }
            else
            {
                // Straight line case: create arbitrary perpendicular vector
                if (abs(T.x()) < 0.9f)
                    N = Vector3f::cross(T, Vector3f(1, 0, 0)).normalized();
                else
                    N = Vector3f::cross(T, Vector3f(0, 1, 0)).normalized();
            }
            
            // Calculate binormal
            Vector3f B = Vector3f::cross(T, N).normalized();
            
            // Ensure continuity of binormal across segments
            if (!firstSegment && i == 0)
            {
                // Check if binormal flipped
                if (Vector3f::dot(B, prevB) < 0)
                {
                    B = -B;
                    N = -N;
                }
            }
            
            // Store the curve point
            unsigned idx = piece * steps + i;
            R[idx].V = V;
            R[idx].T = T;
            R[idx].N = N;
            R[idx].B = B;
            
            // Update previous binormal
            prevB = B;
            firstSegment = false;
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

    // TODO:
    // It is suggested that you implement this function by changing
    // basis from B-spline to Bezier.  That way, you can just call
    // your evalBezier function.

    cerr << "\t>>> evalBSpline has been called with the following input:" << endl;

    cerr << "\t>>> Control points (type vector< Vector3f >): "<< endl;
    for( unsigned i = 0; i < P.size(); ++i )
    {
        cerr << "\t>>> " << P[i] << endl;
    }

    cerr << "\t>>> Steps (type steps): " << steps << endl;
    cerr << "\t>>> Returning empty curve." << endl;

    // Return an empty curve right now.
    return Curve();
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

