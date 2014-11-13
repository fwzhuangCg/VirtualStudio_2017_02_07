#include "mainwindow.h"

#include <QtGui>
#include <QApplication>
#include <io.h>
#include <fcntl.h>

// error code
#define OPENGL_NOT_SUPPORTED -1

int main(int argc, char *argv[])
{
	if ( AllocConsole() )
	{
		int hCrt = _open_osfhandle((long)
			GetStdHandle(STD_OUTPUT_HANDLE), _O_TEXT);
		*stdout = *(::_fdopen(hCrt, "w"));
		::setvbuf(stdout, NULL, _IONBF, 0);
		*stderr = *(::_fdopen(hCrt, "w"));
		::setvbuf(stderr, NULL, _IONBF, 0);
	}

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
