#pragma once
#include "ui_DialogSetting.h"
#include <QDialog>

class DialogSetting  : public QDialog
{
	Q_OBJECT

public:
	Ui::DialogSetting ui;

	DialogSetting(QWidget *parent = nullptr);
	~DialogSetting();
	void loadConfig();
public slots:
	void save();
	void dlgsetReset();
};

