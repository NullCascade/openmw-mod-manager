#include "TreeModModel.h"

#include <QtWidgets>

TreeModModel::TreeModModel(SettingsInterface* settingsInterface, OpenMWConfigInterface* configInterface, QObject *parent)
	: QAbstractItemModel(parent)
{
	settings = settingsInterface;
	config = configInterface;

	QVector<QVariant> rootData;
	rootData << tr("Index") << tr("Mod") << tr("Folder") << tr("Enabled");

	rootItem = new TreeModItem(rootData);

	loadDataFromJson();
}

TreeModModel::~TreeModModel()
{
	saveDataToJson();
	saveDataToConfig();
	delete rootItem;
}

int TreeModModel::columnCount(const QModelIndex & /* parent */) const
{
	return rootItem->columnCount();
}

QVariant TreeModModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	if (role == Qt::TextColorRole)
	{
		foreach (const QModelIndex& conflict, currentConflicts)
		{
			if (conflict == index.sibling(index.row(),0))
				return QVariant(QColor(255, 0, 0));
		}

	}

	if (role == Qt::DisplayRole || role == Qt::EditRole)
	{
		if (index.column() != TreeModItem::COLUMN_ENABLED)
		{
			return getItem(index)->data(index.column());
		}
	}
	else if (role == Qt::CheckStateRole && index.column() == TreeModItem::COLUMN_ENABLED)
	{
		return getItem(index)->data(index.column()).toBool() ? Qt::Checked : Qt::Unchecked;
	}

	return QVariant();
}

Qt::ItemFlags TreeModModel::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return Qt::ItemIsDropEnabled;

	QFlags<Qt::ItemFlag> flag = QAbstractItemModel::flags(index);
	flag.setFlag(Qt::ItemIsSelectable, true);
	flag.setFlag(Qt::ItemIsEditable, true);
	if (index.column() == TreeModItem::COLUMN_INDEX)
	{
		flag.setFlag(Qt::ItemIsEditable, false);
		flag.setFlag(Qt::ItemIsDragEnabled, true);
	}
	else if (index.column() == TreeModItem::COLUMN_ENABLED)
	{
		flag.setFlag(Qt::ItemIsEditable, false);
		flag.setFlag(Qt::ItemIsUserCheckable, true);
	}

	return flag;
}

TreeModItem *TreeModModel::getItem(const QModelIndex &index) const
{
	if (index.isValid()) {
		TreeModItem *item = static_cast<TreeModItem*>(index.internalPointer());
		if (item)
			return item;
	}
	return rootItem;
}

QVariant TreeModModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
		return rootItem->data(section);

	return QVariant();
}

QModelIndex TreeModModel::index(int row, int column, const QModelIndex &parent) const
{
	if (!hasIndex(row, column, parent))
		return QModelIndex();

	TreeModItem *parentItem = getItem(parent);
	TreeModItem *childItem = parentItem->child(row);
	if (childItem)
		return createIndex(row, column, childItem);
	else
		return QModelIndex();
}

bool TreeModModel::insertColumns(int position, int columns, const QModelIndex &parent)
{
	bool success;

	beginInsertColumns(parent, position, position + columns - 1);
	success = rootItem->insertColumns(position, columns);
	endInsertColumns();

	return success;
}

bool TreeModModel::insertRows(int position, int rows, const QModelIndex &parent)
{
	TreeModItem *parentItem = getItem(parent);
	bool success;

	beginInsertRows(parent, position, position + rows - 1);
	success = parentItem->insertChildren(position, rows, rootItem->columnCount());
	endInsertRows();

	// Redo indexing
	recalculateIndexes(parentItem, position);

	return success;
}

QModelIndex TreeModModel::parent(const QModelIndex &index) const
{
	if (!index.isValid())
		return QModelIndex();

	TreeModItem *childItem = getItem(index);
	TreeModItem *parentItem = childItem->parent();

	if (parentItem == rootItem)
		return QModelIndex();

	return createIndex(parentItem->childNumber(), 0, parentItem);
}

QModelIndex TreeModModel::getIndexForFolder(const QString& folder)
{
	QModelIndexList matches = this->match(this->index(0, TreeModItem::COLUMN_FOLDER), Qt::DisplayRole, QVariant::fromValue(folder), 1, Qt::MatchCaseSensitive | Qt::MatchRecursive);
	if (matches.count() > 0)
		return matches.first();
	return QModelIndex();
}

bool TreeModModel::removeColumns(int position, int columns, const QModelIndex &parent)
{
	bool success;

	beginRemoveColumns(parent, position, position + columns - 1);
	success = rootItem->removeColumns(position, columns);
	endRemoveColumns();

	if (rootItem->columnCount() == 0)
		removeRows(0, rowCount());

	return success;
}

bool TreeModModel::removeRows(int position, int rows, const QModelIndex &parent)
{
	TreeModItem* parentItem = getItem(parent);
	bool success = true;

	for (int r = 0; r < rows; r++)
	{
		QString folder = parent.child(position+r, TreeModItem::COLUMN_FOLDER).data().toString();
		foreach(const QString& key, folderConflicts.keys())
		{
			if (folderConflicts[key].contains(folder))
				folderConflicts.remove(folder);
			if (folderConflicts[key].isEmpty())
				folderConflicts.remove(key);
		}
	}
//	if (index.column() == TreeModItem::COLUMN_FOLDER)
//	{
//		QString oldFolder = parent.child(row)
//		QString newFolder = value.toString();

//		// Remove all references to the old folder.
//		foreach(const QString& key, folderConflicts)
//		{
//			if (folderConflicts[key].contains(oldFolder))
//				folderConflicts.remove(oldFolder);
//			if (folderConflicts[key].isEmpty())
//				folderConflicts.remove(key);
//		}
//	}

	beginRemoveRows(parent, position, position + rows - 1);
	success = parentItem->removeChildren(position, rows);
	endRemoveRows();

	// Redo indexing
	recalculateIndexes(parentItem, position);

	// Clear conflicts; another selection is going to come right after.
	currentConflicts.clear();

	return success;
}

int TreeModModel::rowCount(const QModelIndex &parent) const
{
	return getItem(parent)->childCount();
}

bool TreeModModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (role == Qt::CheckStateRole && index.column() == TreeModItem::COLUMN_ENABLED)
	{
		return getItem(index)->setData(index.column(), value.toBool() ? Qt::Checked : Qt::Unchecked);
	}

	if (role != Qt::EditRole)
		return false;

	if (index.column() == TreeModItem::COLUMN_FOLDER)
	{
		QString oldFolder = index.data().toString();
		QString newFolder = value.toString();

		// Remove all references to the old folder.
		foreach(const QString& key, folderConflicts.keys())
		{
			if (folderConflicts[key].contains(oldFolder))
				folderConflicts.remove(oldFolder);
			if (folderConflicts[key].isEmpty())
				folderConflicts.remove(key);
		}

		// Go through all files and add them to the conflict map.
		if (QDir(newFolder).exists())
		{
			QDirIterator it(newFolder, QStringList(), QDir::Files, QDirIterator::Subdirectories);
			while (it.hasNext())
			{
				QString foundPath = it.next();
				QString relativePath = foundPath.right(foundPath.length() - newFolder.length() - 1);
				folderConflicts[relativePath].push_back(newFolder);
			}
		}
	}

	bool result = false;
	if (index.column() == TreeModItem::COLUMN_INDEX)
		result = getItem(index)->setData(index.column(), index.row());
	else
		result = getItem(index)->setData(index.column(), value);

	if (result)
		emit dataChanged(index, index);

	return result;
}

bool TreeModModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
{
	if (role != Qt::EditRole || orientation != Qt::Horizontal)
		return false;

	bool result = rootItem->setData(section, value);
	if (result)
		emit headerDataChanged(orientation, section, section);

	return result;
}

void TreeModModel::addMods(const QJsonArray& modsArray, QModelIndex& parent)
{
	foreach( QJsonValue mod, modsArray )
	{
		QJsonObject modTable = mod.toObject();

		QVector<QVariant> data;
		data << 0 << modTable["name"].toString() << modTable["folder"].toString() << modTable["enabled"].toBool();

		int newRow = this->rowCount(parent);
		this->insertRow(newRow, parent);
		for (int column = 0; column < data.size(); column++)
		{
			this->setData(this->index(newRow, column, parent), data[column]);
		}

		QJsonValue subModsV = modTable["mods"];
		if (!subModsV.isUndefined())
		{
			QJsonArray subModsArray = subModsV.toArray();
			addMods(subModsArray, this->index(newRow, 0, parent));
		}
	}
}

void TreeModModel::loadDataFromJson()
{
	QJsonArray modsArray = settings->getJsonDoc().object()["mods"].toArray();
	addMods(modsArray, QModelIndex());
}

void TreeModModel::saveDataToJson()
{
	settings->setModJson(rootItem);
}

void TreeModModel::saveDataToConfig()
{
	QVector<QVariant>& dataVect = config->getByKey("data");
	dataVect.clear();
	rootItem->serialize(dataVect);
}

void TreeModModel::recalculateIndexes(TreeModItem* parent, int startAt)
{
	for (int i = startAt; i < parent->childCount(); i++)
	{
		parent->child(i)->setData(TreeModItem::COLUMN_INDEX, i);
	}
}

QStringList TreeModModel::mimeTypes() const
{
	QStringList types;
	types << "application/openmwmm.text.data";
	return types;
}

QMimeData* TreeModModel::mimeData(const QModelIndexList &indexes) const
{
	QMimeData *mimeData = new QMimeData();
	QByteArray encodedData;

	QDataStream stream(&encodedData, QIODevice::WriteOnly);
	stream.setVersion(QDataStream::Qt_5_7);

	// Serialize item.
	stream << indexes.length();
	foreach (const QModelIndex &index, indexes) {
		if (index.isValid()) {
			this->getItem(index)->serialize(stream);
		}
	}

	mimeData->setData("application/openmwmm.text.data", encodedData);
	return mimeData;
}

void TreeModModel::importFromDataStream(QDataStream& stream, const QModelIndex& parent, int row)
{
	if (!this->insertRow(row, parent))
		return;

	for (int column = 0; column < this->columnCount(); column++)
	{
		QVariant columnData;
		stream >> columnData;
		QModelIndex child = this->index(row, column, parent);
		if (column == TreeModItem::COLUMN_INDEX)
			this->setData(child, row);
		else if (column == TreeModItem::COLUMN_ENABLED)
			this->setData(child, columnData, Qt::CheckStateRole);
		else
			this->setData(child, columnData);
	}

	// Handle children.
	QVariant childrenCount;
	stream >> childrenCount;
	for (int child = 0; child < childrenCount.toInt(); child++)
		importFromDataStream(stream, this->index(row, 0, parent), child);
}

bool TreeModModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
	if (!canDropMimeData(data, action, row, column, parent))
		return false;

	if (action == Qt::IgnoreAction)
		return true;

	if (data->hasFormat("application/openmwmm.text.data"))
	{
		QByteArray encodedData = data->data("application/openmwmm.text.data");
		QDataStream stream(&encodedData, QIODevice::ReadOnly);

		int sourceIndexCount;
		stream >> sourceIndexCount;
		for (int i = 0; i < sourceIndexCount; i++)
			importFromDataStream(stream, parent, row+i);

		return true;
	}

	return false;
}

bool TreeModModel::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const
{
	Q_UNUSED(action);
	Q_UNUSED(row);
	Q_UNUSED(column);
	Q_UNUSED(parent);

	if (data->hasFormat("application/openmwmm.text.data"))
		return true;

	return false;
}

Qt::DropActions TreeModModel::supportedDragActions() const
{
	return supportedDropActions();
}

Qt::DropActions TreeModModel::supportedDropActions() const
{
	return Qt::MoveAction;
}

//! TODO: Optimize this.
void TreeModModel::updateConflictSelection(const QItemSelection& selected, const QItemSelection& deselected)
{
	Q_UNUSED(deselected);

	if (selected == currentSelection)
		return;
	currentSelection = selected;

	// Clear conflicts.
	QModelIndexList selectionCopy = currentConflicts;
	currentConflicts.clear();
	foreach(const QModelIndex& index, selectionCopy)
		this->dataChanged(index.sibling(index.row(), 0), index.sibling(index.row(), TreeModItem::COLUMN_COUNT), QVector<int>() << Qt::TextColorRole);

	// We don't care what happens if the selection is empty.
	if (selected.empty())
		return;

	QString baseFolder;
	QModelIndex thisIndex;
	foreach(const QModelIndex& index, selected.indexes())
	{
		if (index.column() == TreeModItem::COLUMN_FOLDER)
		{
			thisIndex = index;
			baseFolder = index.data().toString();
			break;
		}
	}

	if (baseFolder.isEmpty())
	{
		qDebug("Warning: Could not locate folder for selection.");
		return;
	}

	QDirIterator it(baseFolder, QStringList(), QDir::Files, QDirIterator::Subdirectories);
	while (it.hasNext())
	{
		QString foundPath = it.next();
		QString relativePath = foundPath.right(foundPath.length() - baseFolder.length() - 1);
		if (folderConflicts.contains(relativePath) && folderConflicts[relativePath].size() > 1)
		{
			foreach (const QString& conflictingFolder, folderConflicts[relativePath])
			{
				if (conflictingFolder == baseFolder)
					continue;

				QModelIndex conflictingIndex = getIndexForFolder(conflictingFolder);
				if (!conflictingIndex.isValid())
				{
					qDebug() << "Warning: Could not find index for conflict for '" + conflictingFolder + "'";
					continue;
				}

				while (conflictingIndex.isValid())
				{
					currentConflicts.push_back(conflictingIndex.sibling(conflictingIndex.row(), 0));
					this->dataChanged(conflictingIndex.sibling(conflictingIndex.row(), 0), conflictingIndex.sibling(conflictingIndex.row(), TreeModItem::COLUMN_COUNT), QVector<int>() << Qt::TextColorRole);
					QModelIndex p = conflictingIndex.parent();
					conflictingIndex = p.sibling(p.row(), conflictingIndex.column());
				}
			}
		}
	}
}
