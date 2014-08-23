#ifndef BOUNDING_VOLUME_H
#define BOUNDING_VOLUME_H

#include <QVector3D>
#include <QVector>

// Axis-aligned bounding box
// An AABB is defined by to extreme point 
// on its diagonal pt_min and pt_max where pt_min <= pt_max
struct AABB
{
    QVector3D pt_min;
    QVector3D pt_max;
};

// Oriented bounding box
// An OBB is described by c, the center point of the box,  and
// three normalized vectors, u, v, and w, that describe the 
// side directions of the box. Their repective positive half-
// lengths are denoted hu, hv, hw, which is the distance from c 
// to the center of the respective face.
class QMatrix4x4;
struct OBB
{
    OBB() {}

    OBB(float min_x, float min_y, float min_z, 
        float max_x, float max_y, float max_z)
        : c( (min_x+max_x)/2, (min_y+max_y)/2, (min_z+max_z)/2 ),
        u(1, 0, 0), v(0, 1, 0), w(0, 0, 1),
        hu( (max_x-min_x)/2 ),
        hv( (max_y-min_y)/2 ),
        hw( (max_z-min_z)/2 )
    {
    }

    OBB(const QVector3D& pt_min, const QVector3D& pt_max)
        : c((pt_min + pt_max) / 2),
        u(1, 0, 0), v(0, 1, 0), w(0, 0, 1),
        hu((pt_max.x() - pt_min.x()) / 2),
        hv((pt_max.y() - pt_min.y()) / 2),
        hw((pt_max.z() - pt_min.z()) / 2)
    {

    }

    OBB(const AABB& box)
        : c((box.pt_min + box.pt_max) / 2),
        u(1, 0, 0), v(0, 1, 0), w(0, 0, 1),
        hu((box.pt_max.x() - box.pt_min.x()) / 2),
        hv((box.pt_max.y() - box.pt_min.y()) / 2),
        hw((box.pt_max.z() - box.pt_min.z()) / 2)
    {
    }

    void setTransform(const QMatrix4x4& xform);

    QVector3D c; // center of the box
    QVector3D u, v, w; // local frame of refernce
    double hu, hv, hw; // half-lengths of faces
};

struct OBBTreeNode
{
    OBB b;

    QVector<OBBTreeNode*> children;
};

struct Sphere
{
    QVector3D c; // center
    double r;    // radius

    Sphere(const QVector3D& center, double radius)
        : c(center), r(radius)
    {}

    Sphere(const AABB& box)
        : c((box.pt_min + box.pt_max) / 2),
        r((box.pt_max - box.pt_min).length()/2)
    {}
};

struct Ray
{
    QVector3D o; // origin
    QVector3D d; // direction
};

bool IntersectSphere( const QVector3D& orig, const QVector3D& dir, const Sphere& s );
bool IntersectSphere( const Ray& ray, const Sphere& s );

bool IntersectSphere( const QVector3D& orig, const QVector3D& dir, const Sphere& s, float& t);
bool IntersectSphere( const Ray& ray, const Sphere& s, float& t );

bool IntersectTriangle( const QVector3D& orig, const QVector3D& dir, 
                       const QVector3D& v0, const QVector3D& v1, const QVector3D& v2, 
                       double& t, double& u, double& v );
bool IntersectTriangle( const Ray& ray,  
                       const QVector3D& v0, const QVector3D& v1, const QVector3D& v2,
                       double& t, double& u, double& v );

bool IntersectAABB( const QVector3D& orig, const QVector3D& dir, const AABB& box );
bool IntersectAABB( const Ray& ray, const AABB& box );

bool IntersectOBB( const QVector3D& orig, const QVector3D& dir, const OBB& box );
bool IntersectOBB( const Ray& ray, const OBB& box );
#endif // BOUNDING_VOLUME_H