#include "gadget.h"

#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QDebug>

DecorativeObject::~DecorativeObject()
{
    if (pb)
    {
        pb->release();
        delete pb;
    }

    if (nb)
    {
        nb->release();
        delete nb;
    }

    if (tb)
    {
        tb->release();
        delete tb;
    }

    if (ib)
    {
        ib->release();
        delete ib;
    }
}

bool DecorativeObject::hasMaterial() const
{
    return !texcoords.isEmpty();
}

bool DecorativeObject::createVBO()
{
    // ´´½¨µØ°åVBO
    pb = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    pb->create();
    if ( !pb->bind() ) 
    {
        qWarning() << "Could not bind position buffer to the context";
        return false;
    }
    pb->setUsagePattern( QOpenGLBuffer::StaticDraw );
    pb->allocate( positions.data(), positions.size() * sizeof(QVector3D) );
    pb->release();

    nb = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    nb->create();
    if ( !nb->bind() ) 
    {
        qWarning() << "Could not bind normal buffer to the context";
        return false;
    }
    nb->setUsagePattern( QOpenGLBuffer::StaticDraw );
    nb->allocate( normals.data(), normals.size() * sizeof(QVector3D) );
    nb->release();

    tb = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    tb->create();
    if ( !tb->bind() ) 
    {
        qWarning() << "Could not bind texcoord buffer to the context";
        return false;
    }
    tb->setUsagePattern( QOpenGLBuffer::StaticDraw );
    tb->allocate( texcoords.data(), texcoords.size() * sizeof(QVector2D) );
    tb->release();

    ib = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    ib->create();
    if ( !ib->bind() ) 
    {
        qWarning() << "Could not bind index buffer to the context";
        return false;
    }
    ib->setUsagePattern( QOpenGLBuffer::StaticDraw );
    ib->allocate( indices.data(), indices.size() * sizeof(uint) );
    ib->release();

    return true;
}
