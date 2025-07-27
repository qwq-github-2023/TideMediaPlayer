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
    void refreshStage(bool reload);
    void showScaleLabel(qreal scaleFactor);
public slots:
    void openFile();
    void showSetting();
    void showAboutDialog();
    void resetScale();
    void mouseMoveEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void sliderValueChanged(int value);
private:
    TideMediaHandle* mediaHandle;
    QTimer hideControlsTimer;
    
    QPixmap oriImagePixmap;
    QTimer labelScaleTimer;
    
    QAudioSink* audioSink;
    QBuffer* audioBuffer;
    QBuffer* audioBufferCache;
    QTimer stageSliderTimer;
    bool isAudioPlaying;
    void refreshImage(bool reload);
    void refreshVideo(bool reload);
    void refreshAudio(bool reload);
    void stageClockGoing();
};
