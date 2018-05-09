#ifndef WINMAIN_H
#define WINMAIN_H

#include <QDebug>
#include <QDesktopServices>
#include <QMainWindow>
#include <QContextMenuEvent>
#include <QDirIterator>
#include <QFileInfo>
#include <QMimeData>
#include <QStandardPaths>
#include <QTextCodec>
#include <QTextStream>

#include "OpenMWConfigInterface.h"
#include "SettingsInterface.h"
#include "TreeModModel.h"
#include "TreeModItem.h"

namespace Ui {
	class WinMain;
}

class WinMain : public QMainWindow
{
	Q_OBJECT

public:
	explicit WinMain(QWidget *parent = 0);
	~WinMain();

public slots:
	void actAddData();
	void actAddChildData();
	void actDeleteData();
	void actContextMenuDataTree(const QPoint& pos);
	void actContextMenuDataTreeOpenFolder();
	void actContextMenuDataTreeHeader(const QPoint& pos);

	void actContextMenuDataTreeHeaderTriggered(QAction* action);

protected:
	void dragEnterEvent(QDragEnterEvent* event) Q_DECL_OVERRIDE;
	void dragMoveEvent(QDragMoveEvent* event) Q_DECL_OVERRIDE;
	void dropEvent(QDropEvent* event) Q_DECL_OVERRIDE;

private:
	/** Open a file-chooser to locate config folder manually. */
	QString locateConfigFolder();
	void addNewData(QAbstractItemModel* model, const QModelIndex& parent, int position, const QFileInfo& target);

	Ui::WinMain *ui;

	SettingsInterface* settings;
	OpenMWConfigInterface* openMWConfig;
};

#endif // WINMAIN_H
