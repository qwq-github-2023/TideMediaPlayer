#include "DialogSetting.h"
#include "Config.h"
#include <QMessageBox>

DialogSetting::DialogSetting(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	loadConfig();
}

DialogSetting::~DialogSetting()
{}

void DialogSetting::loadConfig()
{
	ui.cBoxPreDecodingSec->setCurrentText(Config::getValue("preDecodingSec").toString());
	ui.cBoxFPS->setCurrentText(Config::getValue("FPS").toString());
}

void DialogSetting::dlgsetReset()
{
	if (QMessageBox::warning(this, "警告", "这将删除您已经调整的所有设置！", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
		Config::init(true);
		loadConfig();
	}
}

void DialogSetting::save()
{
	Config::setValue("preDecodingSec", ui.cBoxPreDecodingSec->currentText());
	Config::setValue("FPS", ui.cBoxFPS->currentText());
}

