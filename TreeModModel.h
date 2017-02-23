#ifndef TREEMODMODEL_H
#define TREEMODMODEL_H

#include <QObject>
#include <QAbstractItemModel>
#include <QJsonDocument>

#include "OpenMWConfigInterface.h"
#include "SettingsInterface.h"
#include "TreeModItem.h"

class TreeModModel : public QAbstractItemModel
{
public:
	TreeModModel(SettingsInterface* settingsInterface, OpenMWConfigInterface* configInterface, QObject *parent = 0);
	~TreeModModel();

	// Basic data
	QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

	// Row/column/index
	QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
	QModelIndex parent(const QModelIndex &index) const Q_DECL_OVERRIDE;

	int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
	int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

	// Data manipulation
	Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;
	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) Q_DECL_OVERRIDE;
	bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role = Qt::EditRole) Q_DECL_OVERRIDE;

	bool insertColumns(int position, int columns, const QModelIndex &parent = QModelIndex()) Q_DECL_OVERRIDE;
	bool removeColumns(int position, int columns, const QModelIndex &parent = QModelIndex()) Q_DECL_OVERRIDE;
	bool insertRows(int position, int rows, const QModelIndex &parent = QModelIndex()) Q_DECL_OVERRIDE;
	bool removeRows(int position, int rows, const QModelIndex &parent = QModelIndex()) Q_DECL_OVERRIDE;
	void importFromDataStream(QDataStream& stream, const QModelIndex& parent, int row);

	// Drag/drop
	QStringList mimeTypes() const Q_DECL_OVERRIDE;
	QMimeData* mimeData(const QModelIndexList &indexes) const Q_DECL_OVERRIDE;
	bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) Q_DECL_OVERRIDE;
	bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const Q_DECL_OVERRIDE;
	Qt::DropActions supportedDragActions() const Q_DECL_OVERRIDE;
	Qt::DropActions supportedDropActions() const Q_DECL_OVERRIDE;

private:
	void addMods(const QJsonArray& modsArray, TreeModItem* parent);
	void loadDataFromJson();
	void saveDataToJson();
	void saveDataToConfig();

	void recalculateIndexes(TreeModItem* parent, int startAt = 0);

	TreeModItem *getItem(const QModelIndex &index) const;
	TreeModItem *rootItem;

	SettingsInterface* settings;
	OpenMWConfigInterface* config;
};

#endif // TREEMODMODEL_H
