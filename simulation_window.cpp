#include "simulation_window.h"

#include "scene.h"

#include <QtGui>
#include <QtWidgets/QtWidgets>
#include <QOpenGLShaderProgram>

SimulationWindow::SimulationWindow( Scene* scene, QWindow* screen )
	: QWindow( screen ), 
	  scene_( scene ),
	  m_leftButtonPressed( false )
{	
	// Tell Qt we will use OpenGL for this window
	setSurfaceType(OpenGLSurface);

	// Specify the format we wish to use
	QSurfaceFormat format;
	format.setDepthBufferSize(24);
	format.setMajorVersion(4);
	format.setMinorVersion(0);
	format.setSamples(4);
	format.setProfile(QSurfaceFormat::CoreProfile);
	
	setFormat(format);
	create();
 
	// Create an OpenGL context
	context_ = new QOpenGLContext;
	context_->setFormat(format);
	context_->create();
 
	// Setup our scene
	context_->makeCurrent(this);
	scene_->setContext(context_);
	initializeGL();
 
	// Make sure we tell OpenGL about new window sizes
	connect( this, SIGNAL( widthChanged( int ) ), this, SLOT( resizeGL() ) );
	connect( this, SIGNAL( heightChanged( int ) ), this, SLOT( resizeGL() ) );
}

void SimulationWindow::initializeGL()
{
	context_->makeCurrent(this);
	scene_->initialize();
}

void SimulationWindow::resizeGL()
{
	context_->makeCurrent(this);
	scene_->resize(width(), height());
	paintGL();
}

void SimulationWindow::paintGL()
{
	//if (isExposed()) 
	{
		// Make the context current
		context_->makeCurrent(this);

		// Do the rendering (to the back buffer)
		scene_->render();

		// Swap front/back buffers
		context_->swapBuffers(this);
	}
}

void SimulationWindow::mousePressEvent( QMouseEvent *event )
{
	if (event->button() == Qt::LeftButton) 
	{
		m_leftButtonPressed = true;
		cur_pos_ = prev_pos_ = event->pos();

		if (scene_->interactionMode() == Scene::SELECT)
			scene_->pickCloth(pickColor(event->pos()), false);
	}
	
	QWindow::mousePressEvent(event);
}

void SimulationWindow::mouseReleaseEvent(QMouseEvent* e)
{
	if ( e->button() == Qt::LeftButton )
		m_leftButtonPressed = false;
	QWindow::mouseReleaseEvent( e );
}

void SimulationWindow::mouseMoveEvent( QMouseEvent *event )
{
	cur_pos_ = event->pos();
	float dx = +12.0f * float(cur_pos_.x() - prev_pos_.x()) / width();
	float dy = -12.0f * float(cur_pos_.y() - prev_pos_.y()) / height();

	if (scene_->interactionMode() != Scene::SELECT)
	{
		if (event->buttons() & Qt::LeftButton) 
		{
			scene_->rotate(prev_pos_, cur_pos_);
		}
		else if (event->buttons() & Qt::RightButton)
		{
			scene_->pan(dx, dy);
		}
	}
	else
	{
		if (event->buttons() & Qt::LeftButton) 
		{
			scene_->rotateCloth(prev_pos_, cur_pos_);
		}
		else if (event->buttons() & Qt::RightButton)
		{
			scene_->moveCloth(dx, dy);
		}
		else
		{
			scene_->pickCloth(pickColor(event->pos()), true);
		}
	}

	prev_pos_ = cur_pos_;
	paintGL();

	QWindow::mouseMoveEvent(event);
}

void SimulationWindow::wheelEvent( QWheelEvent *event )
{
	if (event->isAccepted()) 
	 {
		if (scene_->interactionMode() != Scene::SELECT)
			scene_->zoom(event->delta());
		else
			scene_->zoomCloth(event->delta());
		event->accept();
		paintGL();
	}
	QWindow::wheelEvent(event);
}
 
// void SimulationWindow::keyPressEvent(QKeyEvent* e)
// {
//     const float speed = 440.7f;
//     switch (e->key())
//     {
//     case Qt::Key_W:
//         scene_->setForwardSpeed( speed );
//         break;
//     case Qt::Key_S:
//         scene_->setForwardSpeed( -speed );
//         break;
//     case Qt::Key_A:
//         scene_->setSideSpeed( -speed );
//         break;
//     case Qt::Key_D:
//         scene_->setSideSpeed( speed );
//         break;
//     case Qt::Key_Q:
//         scene_->setVerticalSpeed( speed );
//         break;
//     case Qt::Key_E:
//         scene_->setVerticalSpeed( -speed );
//         break;
//     default:
//         QWindow::keyPressEvent(e);
//     }
// }
// 
// void SimulationWindow::keyReleaseEvent(QKeyEvent* e)
// {
//     switch ( e->key() )
//     {
//     case Qt::Key_D:
//     case Qt::Key_A:
//         scene_->setSideSpeed( 0.0f );
//         break;
// 
//     case Qt::Key_W:
//     case Qt::Key_S:
//         scene_->setForwardSpeed( 0.0f );
//         break;
// 
//     case Qt::Key_Q:
//     case Qt::Key_E:
//         scene_->setVerticalSpeed(0.0f);
//         break;
// 
//     default:
//         QWindow::keyReleaseEvent( e );
//     }
// }

void SimulationWindow::updateAnimation(const Animation* anim, int frame)
{
	// 更新Avatar和Cloth动画
	scene_->updateAvatarAnimation(anim, frame);
	paintGL();
}

void SimulationWindow::restoreToBindpose()
{
	scene_->restoreToBindpose();
	paintGL();
}

void SimulationWindow::paintForPick()
{
	if (isExposed()) {
		// Make the context current
		context_->makeCurrent(this);

		// Do the rendering (to the back buffer)
		scene_->renderForPick();

		//context_->swapBuffers(this);
	}
}

unsigned char SimulationWindow::pickColor(QPoint pos)
{
	BYTE data[3];
	paintForPick();
	glReadBuffer(GL_BACK);
	glReadPixels(pos.x(), height() - pos.y(), 1, 1, GL_RGB,GL_UNSIGNED_BYTE, data);
	glReadBuffer(GL_FRONT);
	return data[0];
}