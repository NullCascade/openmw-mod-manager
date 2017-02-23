#include "TreeModItem.h"

#include <QStringList>

TreeModItem::TreeModItem(const QVector<QVariant> &data, TreeModItem *parent)
{
	parentItem = parent;
	itemData = data;
}

TreeModItem::~TreeModItem()
{
	qDeleteAll(childItems);
}

TreeModItem *TreeModItem::child(int number)
{
	return childItems.value(number);
}

int TreeModItem::childCount() const
{
	return childItems.count();
}

int TreeModItem::childNumber() const
{
	if (parentItem)
		return parentItem->childItems.indexOf(const_cast<TreeModItem*>(this));

	return 0;
}

int TreeModItem::columnCount() const
{
	return itemData.count();
}

QVariant TreeModItem::data(int column) const
{
	return itemData.value(column);
}

bool TreeModItem::insertChildren(int position, int count, int columns)
{
	if (position < 0 || position > childItems.size())
		return false;

	for (int row = 0; row < count; ++row) {
		QVector<QVariant> data(columns);
		TreeModItem *item = new TreeModItem(data, this);
		childItems.insert(position, item);
	}

	return true;
}

bool TreeModItem::insertColumns(int position, int columns)
{
	if (position < 0 || position > itemData.size())
		return false;

	for (int column = 0; column < columns; ++column)
		itemData.insert(position, QVariant());

	foreach (TreeModItem *child, childItems)
		child->insertColumns(position, columns);

	return true;
}

TreeModItem *TreeModItem::parent()
{
	return parentItem;
}

bool TreeModItem::removeChildren(int position, int count)
{
	if (position < 0 || position + count > childItems.size())
		return false;

	for (int row = 0; row < count; ++row)
		delete childItems.takeAt(position);

	return true;
}

bool TreeModItem::removeColumns(int position, int columns)
{
	if (position < 0 || position + columns > itemData.size())
		return false;

	for (int column = 0; column < columns; ++column)
		itemData.remove(position);

	foreach (TreeModItem *child, childItems)
		child->removeColumns(position, columns);

	return true;
}

bool TreeModItem::setData(int column, const QVariant &value)
{
	if (column < 0 || column >= itemData.size())
		return false;

//	if (column == TreeModItem::COLUMN_ENABLED)
//	{
//		itemData[column] = value.toBool() ? Qt::Checked : Qt::Unchecked;
//		return true;
//	}

	itemData[column] = value;
	return true;
}

QJsonArray TreeModItem::getChildrenAsJsonArray()
{
	QJsonArray array;

	foreach (TreeModItem *child, childItems)
	{
		array.insert(child->childNumber(), child->toJsonObject());
	}

	return array;
}

QJsonValue TreeModItem::toJsonObject()
{
	if (parentItem == 0)
	{
		return getChildrenAsJsonArray();
	}

	QJsonObject obj;
	obj["name"] = data(1).toString();
	obj["folder"] = data(2).toString();
	obj["enabled"] = data(3).toBool();
	if (childCount() > 0)
		obj["mods"] = getChildrenAsJsonArray();
	return obj;
}

void TreeModItem::serialize(QDataStream& stream)
{
	// Store base data.
	for (int column = 0; column < TreeModItem::COLUMN_COUNT; column++)
	{
		Qt::ItemDataRole role = Qt::DisplayRole;
		if (column == TreeModItem::COLUMN_ENABLED)
			role = Qt::CheckStateRole;

		stream << this->data(column);
	}

	// Store children.
	stream << QVariant(this->childCount());
	for (int child = 0; child < this->childCount(); child++)
	{
		this->child(child)->serialize(stream);
	}
}

void TreeModItem::serialize(QVector<QVariant>& dataVect)
{
	if (data(COLUMN_ENABLED).toBool())
	{
		if (parentItem)
			dataVect.push_back(data(COLUMN_FOLDER));
		foreach (TreeModItem* child, childItems)
			child->serialize(dataVect);
	}
}
