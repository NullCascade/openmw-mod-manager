#include "SettingsInterface.h"

SettingsInterface::SettingsInterface(const QString& jsonFilePath)
{
	jsonPath = jsonFilePath;

	// Load json from settings file.
	QFile jsonFile(jsonPath);
	if (!jsonFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning("Couldn't open settings file for reading.");
		return;
	}

	json = QJsonDocument::fromJson(jsonFile.readAll());
}

SettingsInterface::~SettingsInterface()
{
	save();
}

void SettingsInterface::save()
{
	QFile jsonFile(jsonPath);
	if (!jsonFile.open(QIODevice::WriteOnly))
	{
		qWarning("Couldn't open settings file for writing.");
		return;
	}

	jsonFile.write(json.toJson());
}


QVariant SettingsInterface::getSetting(const QString& key)
{
	return (QVariant) json.object()["settings"].toObject()[key];
}

void SettingsInterface::setSetting(const QString& key, const QString& value)
{
	QJsonObject rootObject = json.object();

	QJsonObject settingsObject = rootObject["settings"].toObject();
	settingsObject[key] = value;
	rootObject["settings"] = settingsObject;

	json = QJsonDocument(rootObject);
}

const QJsonDocument& SettingsInterface::getJsonDoc()
{
	return json;
}

void SettingsInterface::setModJson(TreeModItem* rootItem)
{
	QJsonObject rootObject = json.object();
	rootObject["mods"] = rootItem->toJsonObject();
	json = QJsonDocument(rootObject);
}
