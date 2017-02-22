#include "OpenMWConfigInterface.h"

OpenMWConfigInterface::OpenMWConfigInterface(const QString& configFilePath)
{
	setConfigPath(configFilePath);
	load();
}

OpenMWConfigInterface::~OpenMWConfigInterface()
{
	save();
}

void OpenMWConfigInterface::insert(const QString& key, const QVariant& value)
{
	getByKey(key).push_back(value);
}

QVector<QVariant>& OpenMWConfigInterface::getByKey(const QString &key)
{
	if (!data.contains(key))
	{
		data[key] = QVector<QVariant>();
	}

	return data[key];
}

void OpenMWConfigInterface::save()
{
	QFile cfgFile(cfgPath);
	if (!cfgFile.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		qWarning("Couldn't open OpenMW config file for writing.");
		return;
	}

	foreach(const QString& key, data.keys())
	{
		foreach(const QVariant& value, data[key])
		{
			QString line = key + "=" + value.toString() + '\n';
			cfgFile.write(line.toUtf8());
		}
	}
}

void OpenMWConfigInterface::load()
{
	// Clear current map.
	data.clear();

	// Load json from settings file.
	QFile cfgFile(cfgPath);
	if (!cfgFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning("Couldn't open OpenMW config file for reading.");
		return;
	}

	QString line;
	line = cfgFile.readLine();
	while (!line.isEmpty())
	{
		int splitIndex = line.indexOf('=');
		insert(line.left(splitIndex).trimmed(), line.right(line.length()-splitIndex-1).trimmed());
		line = cfgFile.readLine();
	}
}

void OpenMWConfigInterface::setConfigPath(const QString& path)
{
	cfgPath = path;
}
