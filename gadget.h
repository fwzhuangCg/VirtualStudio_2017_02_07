//----------------------------------------------------------------------------
// File: gadget.h
//
// Functions to draw lights, 3D manipulator and UI from arrays in memory 
//
// Copyleft ()) 505 Lab. No rights reserved.
//-----------------------------------------------------------------------------
#ifndef GADGET_H
#define GADGET_H

#include <QVector>
#include <QVector2D>
#include <QVector3D>
#include <Qvector4D>

class QOpenGLBuffer;
class QOpenGLVertexArrayObject;

// forward declaration
struct DecorativeObject
{
    QVector<QVector3D> positions;
    QVector<QVector3D> normals;
    QVector<QVector2D> texcoords;
    QVector<uint>      indices; 

    QOpenGLBuffer* pb;
    QOpenGLBuffer* nb;
    QOpenGLBuffer* tb;
    QOpenGLBuffer* ib;

    QOpenGLVertexArrayObject* vao;

    DecorativeObject()
        : pb(nullptr),
        nb(nullptr),
        tb(nullptr),
        ib(nullptr)
    {
    }

    ~DecorativeObject();

    bool createVBO();

    bool hasMaterial() const;
};


#endif // GADGET_H