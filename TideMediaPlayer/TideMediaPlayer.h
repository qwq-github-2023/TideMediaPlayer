#pragma once

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
    TideMediaPlayer(QWidget *parent = nullptr);
    ~TideMediaPlayer();
    void refreshStage(bool reload = true);
public slots:
    void openFile();
    void showAboutDialog();
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
private:
    Ui::TideMediaPlayer ui;

    TideMediaHandle* mediaHandle;
    QTimer hideControlsTimer;
    
    QPixmap oriImagePixmap;
    double imageScale = -1;
    QTimer labelScaleTimer;
    QPoint lastMousePos;
    QPoint offset;

    void adjustOffsetWithinBounds(const QSize& imageSize, const QSize& labelSize);
    void refreshImage(bool reload = true);
    void refreshVideo(bool reload = true);
    void refreshAudio(bool reload = true);
    void showScaleLabel();
    
};
