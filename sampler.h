#ifndef SAMPLER_H
#define SAMPLER_H

#include <QtOpenGL/QtOpenGL>
#include <QSharedPointer>

class QOpenGLFunctions_4_0_Core;

class Sampler
{
public:
	Sampler();
	~Sampler();

	void create();
	void destroy();
	GLuint samplerId() const { return m_samplerId; }
	void bind( GLuint unit );
	void release( GLuint unit );

	enum CoordinateDirection {
		DirectionS = GL_TEXTURE_WRAP_S,
		DirectionT = GL_TEXTURE_WRAP_T,
		DirectionR = GL_TEXTURE_WRAP_R
	};

	void setWrapMode( CoordinateDirection direction, GLenum wrapMode );

	void setMinificationFilter( GLenum filter );
	void setMagnificationFilter( GLenum filter );
    void generateMipmap( GLenum type);

private:
	GLuint m_samplerId;
	QOpenGLFunctions_4_0_Core* m_funcs;
};

typedef QSharedPointer<Sampler> SamplerPtr;
#endif // SAMPLER_H