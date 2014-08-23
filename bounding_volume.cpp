#include "bounding_volume.h"

#include <QMatrix4x4>
#include <QVector4D>

// realtime rendering 3rd E p741

bool IntersectSphere(const QVector3D& orig, const QVector3D& dir, const Sphere& sphere)
{
    QVector3D l = sphere.c - orig;
    float s = QVector3D::dotProduct(l, dir);
    float l_squared = l.lengthSquared();
    float r_squared = sphere.r * sphere.r;

    if (s < 0 && l_squared > r_squared)
        return false;
    float m_squared = l_squared - s * s;
    if (m_squared > r_squared)
        return false;
    return true;
}

bool IntersectSphere(const Ray& ray, const Sphere& sphere)
{
    return IntersectSphere(ray.o, ray.d, sphere);
}

bool IntersectSphere(const QVector3D& orig, const QVector3D& dir, const Sphere& sphere, float& t)
{
    QVector3D l = sphere.c - orig;
    float s = QVector3D::dotProduct(l, dir);
    float l_squared = l.lengthSquared();
    float r_squared = sphere.r * sphere.r;

    if (s < 0 && l_squared > r_squared)
        return false;
    float m_squared = l_squared - s * s;
    if (m_squared > r_squared)
        return false;
    // calculate t
    float q = sqrtf(r_squared - m_squared);
    if (l_squared > r_squared)
        t = s - q;
    else
        t = s + q;
    return true;
}

bool IntersectSphere(const Ray& ray, const Sphere& sphere, float& t)
{
    return IntersectSphere(ray.o, ray.d, sphere, t);
}


// Given a ray (defined by its origin and direction), and three vertices of a triangle,
// this function returns true and the interpolated texture coordinate if the ray intersects
// the triangle

bool IntersectTriangle(const QVector3D& orig, const QVector3D& dir, const QVector3D& v0, const QVector3D& v1, const QVector3D& v2, double& t, double& u, double& v)
{
    // Find the vectors for two edges sharing vert0
    QVector3D edge1 = v1 - v0;
    QVector3D edge2 = v2 - v0;

    // Begin calculating determinant - also used to calculate U parameter
    QVector3D pvec = QVector3D::crossProduct(dir, edge2);

    // If determinant is near zero, ray lies in plane of triangle
    double det = QVector3D::dotProduct(edge1, pvec);

    QVector3D tvec;
    if ( det > 0 )
    {
        tvec = dir - v0;
    }
    else
    {
        tvec = v0 - orig;
        det = -det;
    }

    if ( det < 0.0001f )
        return false;

    // Calculate U parameter and test bounds
    u = QVector3D::dotProduct( tvec, pvec );
    if ( u < 0.0f || u > det )
        return false;

    // Prepare to test V parameter
    QVector3D qvec = QVector3D::crossProduct( tvec, edge1 );

    // Calculate V parameter and test bounds
    v = QVector3D::dotProduct( dir, qvec );
    if ( v < 0.0f || u + v > det )
        return false;

    // Calculate t, scale parameters, ray intersects triangle
    t = QVector3D::dotProduct( edge2, qvec );
    double fInvDet = 1.0f / det;
    t *= fInvDet;
    u *= fInvDet;
    v *= fInvDet;

    return true;
}

bool IntersectTriangle(const Ray& ray, 
                       const QVector3D& v0, const QVector3D& v1, const QVector3D& v2, 
                       double& t, double& u, double& v)
{
    return IntersectTriangle(ray.o, ray.d, v0, v1, v2, t, u, v);
}

bool IntersectAABB(const QVector3D& orig, const QVector3D& dir, const AABB& box)
{
    return true;
}

bool IntersectAABB(const Ray& ray, const AABB& box)
{
    return IntersectAABB(ray.o, ray.d, box);
}

bool IntersectOBB(const QVector3D& orig, const QVector3D& dir, const OBB& box)
{
    float t_min = 0;
    return true;
}

bool IntersectOBB(const Ray& ray, const OBB& box)
{
    return IntersectOBB(ray.o, ray.d, box);
}

void OBB::setTransform(const QMatrix4x4& xform)
{
    c = (xform * QVector4D(c, 1.0f)).toVector3D();
    QMatrix3x3 vecXform = xform.normalMatrix();
    //u = u * vecXform;
}
