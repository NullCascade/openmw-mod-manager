#include "TreeModModel.h"

#include <QtWidgets>

TreeModModel::TreeModModel(SettingsInterface* settingsInterface, QObject *parent)
	: QAbstractItemModel(parent)
{
	settings = settingsInterface;

	QVector<QVariant> rootData;
	rootData << tr("Index") << tr("Mod") << tr("Folder") << tr("Enabled");

	rootItem = new TreeModItem(rootData);

	loadDataFromJson();
}

TreeModModel::~TreeModModel()
{
	saveDataToJson();
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
	if (parent.isValid() && parent.column() != 0)
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

	beginRemoveRows(parent, position, position + rows - 1);
	success = parentItem->removeChildren(position, rows);
	endRemoveRows();

	// Redo indexing
	recalculateIndexes(parentItem, position);

	return success;
}

int TreeModModel::rowCount(const QModelIndex &parent) const
{
	TreeModItem *parentItem = getItem(parent);

	return parentItem->childCount();
}

bool TreeModModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (role == Qt::CheckStateRole && index.column() == TreeModItem::COLUMN_ENABLED)
	{
		return getItem(index)->setData(index.column(), value.toBool() ? Qt::Checked : Qt::Unchecked);
	}

	if (role != Qt::EditRole)
		return false;

	bool result = getItem(index)->setData(index.column(), value);
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

void TreeModModel::addMods(const QJsonArray& modsArray, TreeModItem* parent)
{
	foreach( QJsonValue mod, modsArray )
	{
		QJsonObject modTable = mod.toObject();

		QVector<QVariant> data;
		data << parent->childCount() << modTable["name"].toString() << modTable["folder"].toString() << modTable["enabled"].toBool();

		parent->insertChildren(parent->childCount(), 1, rootItem->columnCount());
		TreeModItem* item = parent->child(parent->childCount()-1);
		for (int column = 0; column < data.size(); column++)
		{
			item->setData(column, data[column]);
		}

		QJsonValue subModsV = modTable["mods"];
		if (!subModsV.isUndefined())
		{
			QJsonArray subModsArray = subModsV.toArray();
			addMods(subModsArray, item);
		}
	}
}

void TreeModModel::loadDataFromJson()
{
	QJsonArray modsArray = settings->getJsonDoc().object()["mods"].toArray();
	addMods(modsArray, rootItem);
}

void TreeModModel::saveDataToJson()
{
	settings->setModJson(rootItem);
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