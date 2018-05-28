#include "cad2d.h"

#include <cmath>
#include <cassert>

namespace cad
{
Solid* Solid::cur_solid = 0;
int Solid::solid_id = 0;
int Face::face_id = 0;
int Vertex::vertex_id = 0;

Vertex& Vertex::operator*=(double factor)
{
    x *= factor;
    y *= factor;
    return *this;
}

Vertex& Vertex::operator+=(const Vertex& v)
{
    x += v.x;
    y += v.y;
    return *this;
}

Vertex& Vertex::operator-=(const Vertex& v)
{
    x -= v.x;
    y -= v.y;
    return *this;
}

Vertex& Vertex::operator/=(double divisor)
{
    assert(divisor != 0);
    x /= divisor;
    y /= divisor;
    return *this;
}

// arithmetic operators
Vertex operator+(const Vertex& v1, const Vertex& v2)
{
    Vertex ret(v1);
    ret += v2;
    return ret;
}

Vertex operator-(const Vertex& v1, const Vertex& v2)
{
    Vertex ret(v1);
    ret -= v2;
    return ret;
}

Vertex operator*(const Vertex& v, double factor)
{
    Vertex ret(v);
    ret *= factor;
    return ret;
}

Vertex operator*(double factor, const Vertex& v)
{
    Vertex ret(v);
    ret *= factor;
    return ret;
}

Vertex operator/(const Vertex& v, double divisor)
{
    Vertex ret(v);
    ret /= divisor;
    return ret;
}

void Solid::addVertex(Vertex *newv)
{
    // if vertex list is not empty, append it or make the new vertex v
    if (v)
    {
        Vertex  *vp = v;
        while (vp->nxt)
            vp = vp->nxt;
        vp->nxt = newv;
        newv->pre = vp;
    }
    else
    {
        v = newv;
    }
}

void Solid::addFace(Face *newf)
{
    // if face list is not empty, append it or make the new face f
    if (f)
    {
        Face *fp = f;
        while (fp->nxt)
            fp = fp->nxt;
        fp->nxt = newf;
        newf->pre = fp;
    }
    else
    {
        f = newf;
    }
}

// add face f1 to face f2, make f1's loops as f2's inner loops
// finally f1 is eliminated, bringing us a ring and hole
void Solid::addFaceTo(Face *f1, Face *f2)
{
    // find the end of f2's loop list, append f1's loop list to it
    Loop* lp = f2->l;
    for (; lp->nxt; lp = lp->nxt);
    lp->nxt = f1->l;
    // modify the f1's loop's f pointer
    for (lp = f1->l; lp->nxt; lp = lp->nxt)
        lp->f = f2;

    // stupid mistake, forget to shift the face list
    // rectified at 2011-11-4 13:09:09
    if (f1 == f)    // if f1 is the first face
    {
        f = f1->nxt;
    }
    else
    {
        if(f1->pre) f1->pre->nxt = f1->nxt;
        if(f1->nxt) f1->nxt->pre = f1->pre;
    }

    delete f1;
}

void Face::addLoop(Loop *newl)
{
    // if loop list is not empty, append it or make the new loop l
    if (l)
    {
        // thus the newl is inner loop
        Loop *lp = l;
        while (lp->nxt)
            lp = lp->nxt;
        lp->nxt = newl;
        newl->pre = lp;
    }
    else
    {
        // thus the newl is out loop
        l = newl;
    }
    newl->f = this;
}

// for consistency, we assume that he1 is prior to he2
void Loop::addHalfEdgePair(Vertex *insert_v, HalfEdge *he1, HalfEdge *he2)
{

    // link the pair and make them 'pair'
    he1->nxt = he2;
    he2->pre = he1;
    he1->o = he2;
    he2->o = he1;

    HalfEdge *pre_he = he;

    // if the pair is the first pair
    if (!pre_he)
    {
        he1->pre = he2;
        he2->nxt = he1;
        he = he1;
    }
    // insert the pair
    else// look for the halfedge which ends at the the vertex insert_v
    {
        while (pre_he->nxt->v != insert_v)
        {
            pre_he = pre_he->nxt;
            if (pre_he == he) break;    // in case of infinite loop, sometimes people look for the insert_v in the wrong loop
        }
        he1->pre = pre_he;
        he2->nxt = pre_he->nxt;
        // stupid mistake, rectified at 2011-11-3 12:04:42
        pre_he->nxt->pre = he2;
        pre_he->nxt = he1;
    }

}

Vertex* Loop::delHalfEdgePair(HalfEdge *he1)
{
    HalfEdge *he2 = he1->nxt;
    assert(he1->o == he2);

    he1->pre->nxt = he2->nxt;
    he2->nxt->pre = he1->pre;

    Vertex *vp = he2->v;

    delete he1;
    delete he2;

    return vp;
}

Vertex* Loop::findVertexFromID(int id)
{
    HalfEdge *hep = he;
    do
    {
        if (hep->v->id == id)
            return hep->v;
        hep = hep->nxt;
    } while(hep != he);
    return 0;
}
///////////////////////////////////
// Euler operations
///////////////////////////////////

// generate a solid at the point p
// the solid has a vertex, a face, a loop but no edges
Solid* Solid::mvfs(Vertex* newv)
{
    Solid *s = new Solid();
    Face *newf = new Face(s);
    Loop* newl = new Loop();
    s->addVertex(newv);
    s->addFace(newf);
    newf->addLoop(newl);

    return s;
}

// newp denotes the new point, insert_v is the vertex that the pair of halfedges will be inserted to
// insert_l is the loop will be inserted to
// we will add a pair of halfedges to the solid
// for consistency, we assume that after inserting the halfedge pair
// the order is pre_he--> he1--> he2--> pre_he_nxt
HalfEdge* Solid::mev(Vertex *newv, Vertex *insert_v, Loop *insert_l)
{
    HalfEdge *he1 = new HalfEdge(insert_l);
    HalfEdge *he2 = new HalfEdge(insert_l);

    // don't forget the first vertex of loop
    if(!insert_l->he) insert_v ->he = he1;

    insert_l->addHalfEdgePair(insert_v, he1, he2);
    addVertex(newv);

    he1->v = insert_v;
    //
    he2->v = newv;
    newv->he = he2;

    return he1;
}

// for consistency, we assume that he1 start at v1, he2 start at v2
// v1 will be the insert vertex
Face* Solid::mef(Loop *oldl, Vertex *v1, Vertex *v2)
{
    HalfEdge *he1 = new HalfEdge(oldl);
    HalfEdge *he2 = new HalfEdge(oldl);

    // look for the halfedge which starts at v2 and the one which ends at v2
    HalfEdge *hep = oldl->he;
    while (hep->v != v2)
    {
        hep = hep->nxt;
        if (hep == oldl->he) break; // in case of infinite loop
    }

    // add the pair of halfedges to the old loop
    oldl->addHalfEdgePair(v1, he1, he2);
    he1->v = v1;
    he2->v = v2;

    // generate a new face and a new loop
    Face *newf = new Face(this);
    addFace(newf);
    Loop *newl = new Loop();
    newf->addLoop(newl);


    // split the old loop
    //  modify the joint of v2
    he1->nxt = hep;
    he2->pre = hep->pre;
    hep->pre->nxt = he2;
    hep->pre = he1;
    //
    newl->he = he2; he2->l = newl;
    newf->l = newl;

    // stupid mistake, rectified at 2011-11-3 20:34:06 I forget to modify the he of oldl
    oldl->he = he1->nxt;

    return newf;
}

// delete a pair of halfedges and make a inner loop
// the isolated vertex will be identified by its he which is null
// for consistency, we assume that he2 is next to he1 and they are a pair
Loop* Solid::kemr(HalfEdge *he1)
{
    Loop *lp = he1->l;
    // DelHalfEdge delete he1 and its partner halfedge
    // and return the isolated vertex
    // the isolated vertex is already in the solid's vertex list
    lp->delHalfEdgePair(he1);
    // modified at 2011-11-4 8:47:51, add a ring to the face
    Loop *newl = new Loop();
    lp->f->addLoop(newl);

    return newl;
}

// add face f1 to face f2, make f1's loops as f2's inner loops
// finally f1 is eliminated, bringing us a ring and hole
void Solid::kfmrh(Face *f1, Face *f2)
{
    addFaceTo(f1, f2);
}

} // end namespace cad
