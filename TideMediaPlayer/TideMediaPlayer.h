#pragma once

#include "WidgetStageGL.h"
#include <QtWidgets/QMainWindow>
#include "ui_TideMediaPlayer.h"
#include "TideMediaHandle.h"
#include <QTimer>
#include <QMouseEvent>
#include <QPainter>

class TideMediaPlayer : public QMainWindow
{
    Q_OBJECT
public:
    Ui::TideMediaPlayer ui;

    TideMediaPlayer(QWidget *parent = nullptr);
    ~TideMediaPlayer();
    void refreshStage(bool reload = true);
    void showScaleLabel(qreal scaleFactor);
public slots:
    void openFile();
    void showSetting();
    void showAboutDialog();
    void resetScale();
    void mouseMoveEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    
private:
    TideMediaHandle* mediaHandle;
    QTimer hideControlsTimer;
    
    QPixmap oriImagePixmap;
    QTimer labelScaleTimer;

    void refreshImage(bool reload = true);
    void refreshVideo(bool reload = true);
    void refreshAudio(bool reload = true);
};
