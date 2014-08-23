#include "mainwindow.h"

#include <QtGui>
#include <QApplication>
//#include <QtOpenGL>

// error code
#define OPENGL_NOT_SUPPORTED -1

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	// Test if the system has OpenGL Support
// 	if (!QGLFormat::hasOpenGL()) {
// 		QMessageBox::critical(NULL, QCoreApplication::applicationName(), QObject::tr("This System has no OpenGL support"));
// 		return OPENGL_NOT_SUPPORTED;
// 	}

	MainWindow main_window;
	main_window.showMaximized();

	return app.exec();
}
