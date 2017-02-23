#ifndef TREEMODITEM_H
#define TREEMODITEM_H

#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QVariant>
#include <QVector>

class TreeModItem
{
public:
	explicit TreeModItem(const QVector<QVariant> &data, TreeModItem *parent = 0);
	~TreeModItem();

	TreeModItem *child(int number);
	int childCount() const;
	int columnCount() const;
	QVariant data(int column) const;
	bool insertChildren(int position, int count, int columns);
	bool insertColumns(int position, int columns);
	TreeModItem *parent();
	bool removeChildren(int position, int count);
	bool removeColumns(int position, int columns);
	int childNumber() const;
	bool setData(int column, const QVariant &value);

	QJsonArray getChildrenAsJsonArray();
	QJsonValue toJsonObject();
	void serialize(QDataStream& stream);
	void serialize(QVector<QVariant>& dataVect);

	enum Columns {
		COLUMN_INDEX,
		COLUMN_NAME,
		COLUMN_FOLDER,
		COLUMN_ENABLED,
		COLUMN_COUNT
	};

private:
	// Parents & Children
	QList<TreeModItem*> childItems;
	TreeModItem *parentItem;

	// Data
	QVector<QVariant> itemData;
};

#endif // TREEMODITEM_H
