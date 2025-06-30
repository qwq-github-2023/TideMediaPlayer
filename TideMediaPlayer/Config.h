#pragma once
#include <QSettings>
#include <QFile>
#include <QtWidgets/QApplication>
#include <windows.h>

namespace Config {
	void init(bool isOverride = false, QString group = "TideMediaPlayer");
	QVariant getValue(QString key, QString group="TideMediaPlayer");
	void setValue(QString key, QString value, QString group = "TideMediaPlayer");
}
