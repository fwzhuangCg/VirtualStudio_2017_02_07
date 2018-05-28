/*
 * Halfedge data structure, Euler operators
 */
#ifndef CAD2D_H
#define CAD2D_H

namespace cad
{
// forward declaration
class Face;
class Loop;
class HalfEdge;
class Edge;
//////////////////////////////////////////////
class Vertex
{
public:
    // data members
    double x;
    double y;

    int id;
    HalfEdge *he;
    Vertex *pre, *nxt;

    static int vertex_id;

public:
    // ctors
    Vertex(double tx, double ty)
        : x(tx), y(ty), id(vertex_id++), he(nullptr), pre(nullptr), nxt(nullptr)
    {}

    Vertex(const Vertex& v)
        : x(v.x), y(v.y) 
    {
    }

    ~Vertex()
    {
        if (id == vertex_id) vertex_id--;
        he = nullptr; pre = nxt = nullptr;
    }

    // assignment operators
    Vertex& operator*=(double factor);
    Vertex& operator+=(const Vertex& v);
    Vertex& operator-=(const Vertex& v);
    Vertex& operator/=(double divisor);
};

// arithmetic operators
Vertex operator+(const Vertex& v1, const Vertex& v2);
Vertex operator-(const Vertex& v1, const Vertex& v2);
Vertex operator*(const Vertex& v, double factor);
Vertex operator*(double factor, const Vertex& v);
Vertex operator/(const Vertex& v, double divisor);
//////////////////////////////////////////////
class Solid
{
public:
    static Solid* cur_solid;    // always point to the current solid

    int     id;
    Face*   f;
    Vertex* v;
    Solid*  pre;
    Solid*  nxt;

    static int  solid_id;

public:
    Solid() : id(solid_id++), f(nullptr), v(nullptr), nxt(nullptr)
    {
        pre = cur_solid;
        if (cur_solid)
            cur_solid->nxt = this;
        cur_solid = this;
    }

    ~Solid();

    void addVertex(Vertex* newv);
    void addFace(Face* newf);
    void addFaceTo(Face* f1, Face *f2);

    // Euler operators
    static Solid* mvfs(Vertex *newv);
    HalfEdge* mev(Vertex *newv, Vertex* insert_v, Loop* insert_l);
    Face* mef(Loop *oldl, Vertex *v1, Vertex *v2);
    Loop* kemr(HalfEdge *he1);
    void kfmrh(Face *f1, Face *f2);
};
//////////////////////////////////////////////
class Face
{
public:
    int     id;
    //Vertex eq;  // eq ==> Ax + By + Cz + D = 0, here we store (A, B, C), i.e. the normal
    Solid*  s;
    Loop*   l;    // the first loop is the out loop, after which are inner loops
    Face*   pre;
    Face*   nxt;

    static int face_id;

public:
    Face(Solid *s) : id(face_id++), s(s), l(nullptr), pre(nullptr), nxt(nullptr) {}
    ~Face()
    {
        if(id == face_id) face_id--;
        s = nullptr; l = nullptr; pre = nxt = nullptr;
    }

    void addLoop(Loop *newl);
};
//////////////////////////////////////////////
class Loop
{
public:
    Face*       f;
    HalfEdge*   he;
    Loop*       pre;
    Loop*       nxt;

public:
    // loop can be generated without an already existed face
    // you can see this in mef, we generate a new loop and a new face
    // then attach the new loop to the new face
    Loop() : f(nullptr), he(nullptr), pre(nullptr), nxt(nullptr) {}
    ~Loop();

    void addHalfEdgePair(Vertex* insert_v, HalfEdge *he1, HalfEdge *he2);
    Vertex* delHalfEdgePair(HalfEdge *he1);
    Vertex* findVertexFromID(int id);// find out the vertex by a given id
};
//////////////////////////////////////////////
class HalfEdge
{
public:
    Loop*       l;
    Vertex*     v;
    HalfEdge*   o;  //the other halfedge in the same edge
    HalfEdge*   pre;
    HalfEdge*   nxt;

    HalfEdge(Loop* l) : l(l), v(nullptr), pre(nullptr), nxt(nullptr) {}
    ~HalfEdge()
    {
        l = nullptr; v = nullptr; pre = nxt = nullptr;
    }
};
//////////////////////////////////////////////
class Edge
{

};

} // end namespace cad
#endif // CAD2D_H