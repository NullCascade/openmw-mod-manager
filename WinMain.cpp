#include "WinMain.h"
#include "ui_WinMain.h"

#include <QStandardItem>
#include <QStandardItemModel>
#include <QCheckBox>
#include <QFileDialog>
#include <QMessageBox>

WinMain::WinMain(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::WinMain)
{
	ui->setupUi(this);
	
	// Get OpenMW config folder.
	QString configFolder;
	#if defined(Q_OS_MACOS)
		configFolder = QStandardPaths::locate(QStandardPaths::ConfigLocation, "openmw", QStandardPaths::LocateDirectory);
	#elif defined(Q_OS_WIN)
		configFolder = QStandardPaths::locate(QStandardPaths::DocumentsLocation, "My Games/OpenMW", QStandardPaths::LocateDirectory);
	#elif defined(Q_OS_LINUX)
		// Default flatpak location
		configFolder = QStandardPaths::locate(QStandardPaths::HomeLocation, ".var/app/org.openmw.OpenMW/config/openmw/", QStandardPaths::LocateDirectory);
	#endif

	// If not found automatically, ask where it is...
	if(configFolder.isEmpty())
	{
		QMessageBox mbox;
		mbox.setText("Couldn't locate your OpenMW config folder. Please locate the directory containing the files:\n\n  - openmw.cfg\n  - mods.json");
		mbox.setModal(true);
		mbox.setStandardButtons(QMessageBox::Open);
		mbox.setButtonText(0, "Locate...");
		mbox.setIcon(QMessageBox::Icon::Question);
		mbox.exec();

		configFolder = locateConfigFolder();
	}

	// Load configs.
	settings = new SettingsInterface(configFolder + "/mods.json");
	openMWConfig = new OpenMWConfigInterface(configFolder + "/openmw.cfg");

	// Set up mod view.
	TreeModModel* model = new TreeModModel(settings, openMWConfig);
	ui->tvMain->setModel(model);
	ui->tvMain->header()->setContextMenuPolicy(Qt::CustomContextMenu);
	ui->tvMain->header()->setSectionsMovable(false);
	connect(ui->tvMain->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
			model, SLOT(updateConflictSelection(const QItemSelection&, const QItemSelection&)));

	// Resize columns to fit.
	for (int column = 0; column < ui->tvMain->header()->count(); column++)
		ui->tvMain->resizeColumnToContents(column);

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
	QModelIndex index = ui->tvMain->selectionModel()->currentIndex();

	QMenu menu(this);

	menu.addAction(ui->actionAddData);
	menu.addAction(ui->actionDeleteData);

	menu.addSeparator();
	QAction* actOpenFolder = menu.addAction("Open folder", this, SLOT(actContextMenuDataTreeOpenFolder()));
	QDir folder = index.sibling(index.row(), TreeModItem::COLUMN_FOLDER).data().toString();
	if (!folder.exists())
		actOpenFolder->setDisabled(true);

	menu.exec(ui->tvMain->viewport()->mapToGlobal(pos));
}

void WinMain::actContextMenuDataTreeOpenFolder()
{
	QModelIndex index = ui->tvMain->selectionModel()->currentIndex();
	QDir folder = index.sibling(index.row(), TreeModItem::COLUMN_FOLDER).data().toString();
	if (!folder.exists())
	{
		qDebug() << "Warning: Folder '" << folder.absolutePath() << "' does not exist.";
		return;
	}

	QDesktopServices::openUrl(QUrl::fromLocalFile(folder.absolutePath()));
}

void WinMain::actContextMenuDataTreeHeader(const QPoint& pos)
{
	QHeaderView* header = ui->tvMain->header();
	QMenu menu(this);

	//QMenu* menuHeaders = menu.addMenu("Headers");
	for (int column = 0; column < header->count(); column++)
	{
		QAction* action = menu.addAction(ui->tvMain->model()->headerData(column, Qt::Horizontal).toString());
		action->setData(column);
		action->setCheckable(true);
		action->setChecked(!ui->tvMain->header()->isSectionHidden(column));
	}
	connect(&menu, SIGNAL(triggered(QAction*)),
			this, SLOT(actContextMenuDataTreeHeaderTriggered(QAction*)));

	menu.exec(ui->tvMain->header()->viewport()->mapToGlobal(pos));
}

void WinMain::actContextMenuDataTreeHeaderTriggered(QAction* action)
{
	int column = action->data().toInt();
	ui->tvMain->header()->setSectionHidden(column, !ui->tvMain->header()->isSectionHidden(column));
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

QString WinMain::locateConfigFolder()
{
	QString configFolder = QFileDialog::getExistingDirectory(this, "Locate OpenMW config folder...", QString(), 0);

	// Cancelled...
	if(configFolder.isEmpty())
	{
		exit(0);
	}

	QDir configDir(configFolder);

	bool foundConfig = QFile(configDir.filePath("openmw.cfg")).exists();
	bool foundJson = QFile(configDir.filePath("mods.json")).exists();

	if(foundConfig && foundJson)
	{
		return configFolder;
	}

	QMessageBox mbox;
	mbox.setText("Directory must contain the following files:\n\n - openmw.cfg\n - mods.json");
	mbox.setModal(true);
	mbox.setStandardButtons(QMessageBox::Cancel | QMessageBox::Open);
	mbox.setDefaultButton(QMessageBox::Open);
	mbox.setIcon(QMessageBox::Icon::Warning);
	int result = mbox.exec();

	if(result == QMessageBox::Cancel)
	{
		exit(0);
	}

	return locateConfigFolder();
}


void WinMain::addNewData(QAbstractItemModel* model, const QModelIndex& parent, int position, const QFileInfo& target)
{
	// Get path as string.
	QString path = target.absoluteFilePath();

	//! Make sure we aren't adding a duplicate.

	// Add to model.
	if (!model->insertRow(position, parent))
		return;
	for (int column = 0; column < model->columnCount(parent); ++column) {
		QModelIndex child = model->index(position, column, parent);
		if ( column == TreeModItem::COLUMN_INDEX )
			model->setData(child, position, Qt::EditRole);
		else if ( column == TreeModItem::COLUMN_NAME )
			model->setData(child, target.baseName(), Qt::EditRole);
		else if ( column == TreeModItem::COLUMN_FOLDER )
			model->setData(child, target.absoluteFilePath(), Qt::EditRole);
		else if ( column == TreeModItem::COLUMN_ENABLED )
			model->setData(child, true, Qt::EditRole);
		else
			model->setData(child, QVariant("[No data]"), Qt::EditRole);
	}

	// Look for valid sub-elements.
	QDirIterator it(target.absoluteFilePath(), QDir::AllDirs);
	while (it.hasNext())
	{
		QFileInfo subFolder = it.next();
		bool converted = false;
		QVariant firstToken = subFolder.baseName().split(" ").at(0);
		int firstTokenAsInt = firstToken.toString().toInt(&converted);
		if (converted)
		{
			QModelIndex newParent = model->index(position, 0, parent);
			if (!model->insertRow(model->rowCount(newParent), newParent))
				return;
			for (int column = 0; column < model->columnCount(parent); ++column) {
				QModelIndex child = model->index(model->rowCount(newParent)-1, column, newParent);
				if ( column == TreeModItem::COLUMN_INDEX )
					model->setData(child, firstTokenAsInt, Qt::EditRole);
				else if ( column == TreeModItem::COLUMN_NAME )
					model->setData(child, subFolder.baseName().right(subFolder.baseName().length() - firstToken.toString().length() - 1), Qt::EditRole);
				else if ( column == TreeModItem::COLUMN_FOLDER )
					model->setData(child, subFolder.absoluteFilePath(), Qt::EditRole);
				else if ( column == TreeModItem::COLUMN_ENABLED )
					model->setData(child, true, Qt::EditRole);
				else
					model->setData(child, QVariant("[No data]"), Qt::EditRole);
			}
		}
	}

	ui->tvMain->expand(ui->tvMain->model()->index(position, 0, parent));
}
