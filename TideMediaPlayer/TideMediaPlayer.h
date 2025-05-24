#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_TideMediaPlayer.h"

class TideMediaPlayer : public QMainWindow
{
    Q_OBJECT

public:
    TideMediaPlayer(QWidget *parent = nullptr);
    ~TideMediaPlayer();

private:
    Ui::TideMediaPlayerClass ui;
};
