#include "WinMain.h"
#include <QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	WinMain w;
	w.show();

	return a.exec();
}
