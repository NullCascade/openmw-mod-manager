#ifndef OPENMWCONFIGINTERFACE_H
#define OPENMWCONFIGINTERFACE_H

#include <QFile>
#include <QMap>
#include <QString>
#include <QSettings>
#include <QVariant>
#include <QVector>

class OpenMWConfigInterface
{
public:
	OpenMWConfigInterface(const QString& configFilePath);
	~OpenMWConfigInterface();

	void insert(const QString& key, const QVariant& value);
	QVector<QVariant>& getByKey(const QString& key);

	void save();
	void load();

	void setConfigPath(const QString& path);

private:
	QString cfgPath;
	QMap<QString, QVector<QVariant>> data;
};

#endif // OPENMWCONFIGINTERFACE_H
