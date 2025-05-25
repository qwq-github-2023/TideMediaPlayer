#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_TideMediaPlayer.h"
#include "TideMediaHandle.h"
#include <QTimer>
#include <QMouseEvent>

class TideMediaPlayer : public QMainWindow
{
    Q_OBJECT

public:
    TideMediaPlayer(QWidget *parent = nullptr);
    ~TideMediaPlayer();
    void refreshStage(bool reload = true);
public slots:
    void openFile();
    void showAboutDialog();
    void mouseMoveEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
private:
    Ui::TideMediaPlayer ui;

    TideMediaHandle* mediaHandle;
    QTimer hideControlsTimer;
    
    QPixmap orgImagePixmap;
    void refreshImage(bool reload = true);

    void refreshVideo(bool reload = true);

    void refreshAudio(bool reload = true);
};
