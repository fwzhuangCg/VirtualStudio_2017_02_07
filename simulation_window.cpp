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
			scene_->moveCloth(-dx, dy);
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
	// ����Avatar��Cloth����
	scene_->updateAvatarAnimation(anim, frame * AnimationClip::SAMPLE_SLICE);
	if(scene_->isReplay())
		scene_->updateClothAnimation(frame);
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

void SimulationWindow::startSimulate(const Animation* anim)
{
	double length;
	if (anim->ticks_per_second) {
		length = (anim->ticks / anim->ticks_per_second) * 1000; // �����������ʱ��
	}
	else {
		length = anim->ticks * 1000;
	}

	int total_frame = static_cast<int>(length / AnimationClip::SIM_SLICE);
	bool inited = false;

	int factor = AnimationClip::SAMPLE_SLICE / AnimationClip::SIM_SLICE;

	QProgressDialog process(NULL);  
	process.setLabelText(tr("simulating..."));  
	process.setRange(0, total_frame + 1);  
	process.setModal(true);  
	process.setCancelButtonText(tr("cancel"));

	for(int i = 0; i <= total_frame; ++i)
	{
		process.setValue(i + 1);
		if(process.wasCanceled())  
			break;  
		if(!inited)
		{
			scene_->updateAvatarAnimation(anim, 0);
			scene_->initAvatar2Simulation();
			if(!scene_->startSimulate()) {
				process.cancel();
				break;
			}
			inited = true;
		}
		else
		{
			scene_->updateAvatarAnimation(anim, i * AnimationClip::SIM_SLICE);
			scene_->updateAvatar2Simulation();
			if(!scene_->simulateStep()) {
				process.cancel();
				break;
			}
		}

		if(i % factor == 0)
			scene_->writeAFrame(i / factor);
		paintGL();
	}
	scene_->finishedSimulate();

	QMessageBox::information(NULL, "Simulation finished", "Simulation finished.", QMessageBox::Ok);
}

void SimulationWindow::record(const Animation* anim)
{
	QString file_name = QFileDialog::getSaveFileName(NULL, tr("Save As AVI"),  ".", tr("AVI files (*.avi)"));

	AviGen = new AVIGenerator;

	// set 15fps
	AviGen->SetRate(15);
	
	// give info about bitmap
	AviGen->SetBitmapHeader(width(), height());		

	// set filename, extension ".avi" is appended if necessary
	AviGen->SetFileName(file_name.toLocal8Bit().constData());

	// retreiving size of image
	lpbih=AviGen->GetBitmapHeader();

	// allocating memory
	bmBits=new BYTE[lpbih->biSizeImage];

	HRESULT hr=AviGen->InitEngine();
	if (FAILED(hr))
	{
		QMessageBox::critical(0, "error", "InitEngine error!");
		AviGen->ReleaseEngine();
		delete[] bmBits;
		delete AviGen;
		return;
	}

	double length;
	if (anim->ticks_per_second) {
		length = (anim->ticks / anim->ticks_per_second) * 1000; // �����������ʱ��
	}
	else {
		length = anim->ticks * 1000;
	}

	int total_frame = static_cast<int>(length / AnimationClip::SAMPLE_SLICE);

	QProgressDialog process(NULL);  
	process.setLabelText(tr("Recording..."));  
	process.setRange(0, total_frame);  
	process.setModal(true);  
	process.setCancelButtonText(tr("cancel"));

	for(int i = 0; i < total_frame; ++i)
	{
		updateAnimation(anim, i);
		process.setValue(i + 1);
		if(process.wasCanceled())  
			break;

		glReadBuffer(GL_BACK);
		glReadPixels(0,0,lpbih->biWidth,lpbih->biHeight,GL_BGR_EXT,GL_UNSIGNED_BYTE,bmBits); 
		// send to avi engine
		HRESULT hr=AviGen->AddFrame(bmBits);
		if (FAILED(hr))
		{
			QMessageBox::critical(0, "error", "AddFrame error!");
			AviGen->ReleaseEngine();
			delete[] bmBits;
			delete AviGen;
			return;
		}
		glReadBuffer(GL_FRONT);
	}
	
	AviGen->ReleaseEngine();
	delete[] bmBits;
	delete AviGen;
	QMessageBox::information(NULL, "Record finished", "Record finished.", QMessageBox::Ok);
}