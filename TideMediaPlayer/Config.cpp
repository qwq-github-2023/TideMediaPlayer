#include "Config.h"

void Config::init(bool isOverride, QString group)
{
    if (!QFile::exists("./TideMediaPlayer.ini") || isOverride) {
        DEVMODE dm;
        dm.dmSize = sizeof(DEVMODE);
        ::EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm);
        
        QSettings settings("./TideMediaPlayer.ini", QSettings::IniFormat);
        settings.beginGroup(group);
        settings.setValue("FPS", static_cast<UINT>(dm.dmDisplayFrequency));
        settings.setValue("preDecodingSec", 3);
        settings.endGroup();
    }
}

QVariant Config::getValue(QString key, QString group)
{
    QSettings settings("./TideMediaPlayer.ini", QSettings::IniFormat);
    settings.beginGroup(group);
    QVariant ret = settings.value(key);
    settings.endGroup();
    return ret;
}

void Config::setValue(QString key, QString value, QString group)
{
    QSettings settings("./TideMediaPlayer.ini", QSettings::IniFormat);
    settings.beginGroup(group);
    settings.setValue(key, value);
    settings.endGroup();
    return;
}
