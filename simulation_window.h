#ifndef SIMULATION_WINDOW_H
#define SIMULATION_WINDOW_H

#include <QWindow>
#include <QTime>

class Scene;
class Animation;
class SimulationWindow : public QWindow
{
	Q_OBJECT

public:
    SimulationWindow( Scene* scene, QWindow* parent = 0 );

	void initializeGL();
	void paintGL();

public slots:
	void updateAnimation(const Animation* anim, int frame);
	void restoreToBindpose();

    void resizeGL();

protected:
    void mousePressEvent( QMouseEvent *event );
    void mouseReleaseEvent( QMouseEvent* e );
    void mouseMoveEvent( QMouseEvent *event );
    void wheelEvent( QWheelEvent *event );

//     void keyPressEvent( QKeyEvent* e );
//     void keyReleaseEvent( QKeyEvent* e );

private:
	QOpenGLContext* context_;
	Scene*	scene_;

    bool m_leftButtonPressed;
	QPoint cur_pos_;
	QPoint prev_pos_;
};

#endif // SIMULATION_WINDOW_H
