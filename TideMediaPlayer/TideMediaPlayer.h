#pragma once

#include "WidgetStageGL.h"
#include <QtWidgets/QMainWindow>
#include "ui_TideMediaPlayer.h"
#include "TideMediaHandle.h"
#include "TideIODevice.h"
#include <QTimer>
#include <QMouseEvent>
#include <QPainter>
#include <QAudioSink>
#include <QFileDialog>
#include <QMessageBox>  
#include <QPropertyAnimation>
#include <QAudioOutput>
#include <QAudioFormat>
#include <QMediaDevices>
#include <QSettings>
#include <QMutex>
#include <SoundTouch.h>
#ifdef _DEBUG
#pragma comment(lib, "SoundTouchDebug.lib")
#else
#pragma comment(lib, "SoundTouch.lib")
#endif

class TideMediaPlayer : public QMainWindow
{
    Q_OBJECT
public:
    Ui::TideMediaPlayer ui;

    TideMediaPlayer(QWidget *parent = nullptr);
    ~TideMediaPlayer();
    void refreshImage(bool reload);
    void refreshVideo(bool reload);
    void refreshAudio(bool reload);
    
    void showScaleLabel(qreal scaleFactor);
public slots:
    void openFile();
    void showSetting();
    void showAboutDialog();
    void resetScale();
    void mouseMoveEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void sliderUserValueChanged();
    void volumeValueChanged(int value);
    void speedRatioChanged(double);
    void FuckAudioCache();
private:
    TideMediaHandle* mediaHandle;
    QTimer hideControlsTimer;
    
    QPixmap oriImagePixmap;
    QTimer labelScaleTimer;
    QTimer stageSliderTimer;

    soundtouch::SoundTouch soundTouch;
    QAudioSink* audioSink;
    QBuffer* audioBuffer;
    QBuffer* audioBufferCache;
    TideIODevice* audioStream;
    bool isAudioPlaying;

    void stageClockGoing();
    QBuffer* changePlaybackSpeed(QBuffer* inputBuffer, const QAudioFormat& format, float speedRatio);
};
