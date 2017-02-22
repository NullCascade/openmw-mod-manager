#ifndef SETTINGSINTERFACE_H
#define SETTINGSINTERFACE_H

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

#include "TreeModItem.h"

class SettingsInterface
{
public:
	SettingsInterface(const QString& jsonFilePath);
	~SettingsInterface();

	void save();

	QVariant getSetting(const QString& key);
	void setSetting(const QString& key, const QString& value);

	const QJsonDocument& getJsonDoc();
	void setModJson(TreeModItem* rootItem);

private:
	QString jsonPath;
	QJsonDocument json;
};

#endif // SETTINGSINTERFACE_H
