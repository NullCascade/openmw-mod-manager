#include "WinMain.h"
#include "ui_WinMain.h"

#include <QStandardItem>
#include <QStandardItemModel>
#include <QCheckBox>
#include <QFileDialog>

WinMain::WinMain(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::WinMain)
{
	ui->setupUi(this);

	settings = new SettingsInterface("C:\\Users\\Michael\\Desktop\\mods.json");
	openMWConfig = new OpenMWConfigInterface("C:\\Users\\Michael\\Documents\\My Games\\OpenMW\\openmw.cfg");

	ui->tvMain->setModel(new TreeModModel(settings));
	ui->tvMain->header()->setContextMenuPolicy(Qt::CustomContextMenu);

	// Complicated signals/slots
	connect(ui->tvMain, SIGNAL(customContextMenuRequested(QPoint)),
			this, SLOT(actContextMenuDataTree(QPoint)));
	connect(ui->tvMain->header(), SIGNAL(customContextMenuRequested(QPoint)),
			this, SLOT(actContextMenuDataTreeHeader(QPoint)));
}

WinMain::~WinMain()
{
	QAbstractItemModel* model = ui->tvMain->model();
	delete ui;
	delete model;
	delete settings;
	delete openMWConfig;
}

void WinMain::actAddData()
{
	QFileInfo result = QFileDialog::getExistingDirectory(this,tr("Open Directory"), settings->getSetting("defaultModFolder").toString(), QFileDialog::ShowDirsOnly);
	if (!result.exists())
		return;

	QModelIndex index = ui->tvMain->selectionModel()->currentIndex();
	addNewData(ui->tvMain->model(), index.parent(), index.row()+1, result);
}

void WinMain::actAddChildData()
{
	QFileInfo result = QFileDialog::getExistingDirectory(this,tr("Open Directory"), settings->getSetting("defaultModFolder").toString(), QFileDialog::ShowDirsOnly);
	if (!result.exists())
		return;

	QModelIndex index = ui->tvMain->selectionModel()->currentIndex();
	addNewData(ui->tvMain->model(), index, 0, result);
}

void WinMain::actDeleteData()
{
	QModelIndex index = ui->tvMain->selectionModel()->currentIndex();
	QAbstractItemModel* model = ui->tvMain->model();
	model->removeRow(index.row(), index.parent());
}

void WinMain::actContextMenuDataTree(const QPoint& pos)
{
	QMenu menu(this);
	menu.addAction(ui->actionAddData);
	menu.addAction(ui->actionDeleteData);
//	menu.addAction(ui->actionAddChildData);
	menu.exec(ui->tvMain->viewport()->mapToGlobal(pos));
}

void WinMain::actContextMenuDataTreeHeader(const QPoint& pos)
{
	QHeaderView* header = ui->tvMain->header();
	QMenu menu(this);

	QMenu* menuHeaders = menu.addMenu("Headers");
	for (int column = 0; column < header->count(); column++)
	{
		QAction* action = menuHeaders->addAction(ui->tvMain->model()->headerData(column, Qt::Horizontal).toString());
		action->setData(column);
		action->setCheckable(true);
		action->setChecked(!ui->tvMain->header()->isSectionHidden(column));
	}
	connect(menuHeaders, SIGNAL(triggered(QAction*)), this, SLOT(actContextMenuDataTreeHeaderTriggered(QAction*)));

	menu.addSeparator();
	QAction* actContextMenuDataTreeHeaderSortBy = menu.addAction("Sort by");
	connect(actContextMenuDataTreeHeaderSortBy, SIGNAL(triggered(bool)), this, SLOT(actContextMenuDataTreeHeaderSortBy(bool)));

	menu.exec(ui->tvMain->header()->viewport()->mapToGlobal(pos));
}

void WinMain::actContextMenuDataTreeHeaderTriggered(QAction* action)
{
	int index = action->data().toInt();
	ui->tvMain->header()->setSectionHidden(index, !ui->tvMain->header()->isSectionHidden(index));
}

void WinMain::actContextMenuDataTreeHeaderSortBy(bool checked)
{
	qDebug("IT'S SORTING TIME");
}

void WinMain::dragEnterEvent(QDragEnterEvent* event)
{
	auto data = event->mimeData()->data("text/uri-list");
	QTextStream stream(&data);
	QString uri = stream.readLine();
	while (!uri.isEmpty())
	{
		QFileInfo file = QUrl(uri).toLocalFile();
		if (file.isDir() && file.exists())
			event->acceptProposedAction();

		uri = stream.readLine();
	}
}

void WinMain::dragMoveEvent(QDragMoveEvent* event)
{
	if (event->mimeData()->hasFormat("text/uri-list")
		&& event->answerRect().intersects(ui->tvMain->geometry()))
	{
		event->acceptProposedAction();
	}
}

void WinMain::dropEvent(QDropEvent* event)
{
	auto data = event->mimeData()->data("text/uri-list");
	QTextStream stream(&data);
	QString uri = stream.readLine();
	while (!uri.isEmpty())
	{
		QFileInfo file = QUrl(uri).toLocalFile();
		if (file.isDir() && file.exists())
		{
			addNewData(ui->tvMain->model(), ui->tvMain->rootIndex(), ui->tvMain->model()->rowCount(), file);
			event->acceptProposedAction();
		}

		uri = stream.readLine();
	}
}


void WinMain::addNewData(QAbstractItemModel* model, const QModelIndex& parent, int position, const QFileInfo& target)
{
	// Get path as string.
	QString path = target.absoluteFilePath();

	// Make sure we aren't adding a duplicate.
	//! TODO

	// Add to model.
	if (!model->insertRow(position, parent))
		return;
	for (int column = 0; column < model->columnCount(parent); ++column) {
		QModelIndex child = model->index(position, column, parent);
		if ( column == TreeModItem::COLUMN_INDEX )
		{
			model->setData(child, position, Qt::EditRole);
		}
		else if ( column == TreeModItem::COLUMN_NAME )
		{
			model->setData(child, target.baseName(), Qt::EditRole);
		}
		else if ( column == TreeModItem::COLUMN_FOLDER )
		{
			model->setData(child, target.absoluteFilePath(), Qt::EditRole);
		}
		else if ( column == TreeModItem::COLUMN_ENABLED )
		{
			model->setData(child, true, Qt::EditRole);
		}
		else
		{
			model->setData(child, QVariant("[No data]"), Qt::EditRole);
		}
	}
}
