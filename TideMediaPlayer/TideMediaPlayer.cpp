#include "TideMediaPlayer.h"  
#include "AboutDialog.h"
#include "DialogSetting.h"
#include "Config.h"


TideMediaPlayer::TideMediaPlayer(QWidget *parent)  
    : QMainWindow(parent)  
{  
    ui.setupUi(this);  
    ui.pushButtonResetScale->hide();

    hideControlsTimer.setSingleShot(true);  
    connect(&hideControlsTimer, &QTimer::timeout, ui.widgetController, &QWidget::hide);  
    connect(&hideControlsTimer, &QTimer::timeout, ui.pushButtonResetScale, &QPushButton::hide);
    
    ui.labelScale->hide();  
    labelScaleTimer.setSingleShot(true);  
    connect(&labelScaleTimer, &QTimer::timeout, ui.labelScale, &QLabel::hide);  

    ui.widgetController->setEnabled(false);
    
    ui.openGLWidgetStage->setTideMediaPlayer(this);
    Config::init();

    connect(audioSink, &QAudioSink::stateChanged, this, [&](QAudio::State state) {
        if (ui.pushButtonPlay->getStatus() && audioSink->state() == QtAudio::StoppedState && mediaHandle->getMediaType() == TMH_AUDIO) {
            // 音频播放结束，重置音频缓冲区
            if (audioBuffer)  delete audioBuffer;
            if (audioBufferCache) {
                audioBuffer = audioBufferCache;
            }
            audioBuffer->open(QIODevice::ReadOnly);
            qDebug() << "Data size: " << audioBuffer->buffer().size();
            audioSink->start(audioBuffer);
            audioBufferCache = mediaHandle->decodeAudioToQBuffer(Config::getValue("preDecodingSec").toULongLong() * 1000000);
        }
        });
    
    connect(&stageSliderTimer, &QTimer::timeout, this, &TideMediaPlayer::stageClockGoing);
    stageSliderTimer.setSingleShot(false);
    stageSliderTimer.setInterval(10);
    stageSliderTimer.start();
}  

TideMediaPlayer::~TideMediaPlayer()  
{}  

void TideMediaPlayer::showAboutDialog() {
    // 显示关于窗口
    AboutDialog* aboutdlg = new AboutDialog(this);
    aboutdlg->exec();
    aboutdlg->deleteLater();
}
void TideMediaPlayer::showSetting()
{
    // 显示设置窗口
    DialogSetting* settingdlg = new DialogSetting(this);
    settingdlg->exec();
    settingdlg->deleteLater();
}
void TideMediaPlayer::showScaleLabel(qreal scaleFactor)
{
    // 显示缩放倍率
    ui.labelScale->setText(QString("%1%").arg(QString::number(static_cast<int>(scaleFactor * 100))));
    ui.labelScale->show();
    labelScaleTimer.start(2000);
}
void TideMediaPlayer::refreshImage(bool reload)
{
    if (mediaHandle->getMediaType() != TMH_IMAGE) return;
    // 是否重载？
    QPixmap pixmap;
    if (reload) {
        pixmap = mediaHandle->getImagePixmap();
        if (pixmap.isNull()) {
            QMessageBox::critical(nullptr, "错误", "加载图片失败！");
            return;
        }
        oriImagePixmap = pixmap;
    }
    else
        pixmap = oriImagePixmap;
    ui.openGLWidgetStage->loadPixmap(pixmap);
    ui.widgetController->setEnabled(false);
    
    return;
}
void TideMediaPlayer::refreshVideo(bool reload)
{
    if (mediaHandle->getMediaType() != TMH_VIDEO) return;
    ui.widgetController->setEnabled(true);
    return;
}
void TideMediaPlayer::refreshAudio(bool reload)
{
    if (mediaHandle->getMediaType() != TMH_AUDIO) return;
    if (audioSink) audioSink->stop();
    if (reload) {
        
        QAudioFormat format = mediaHandle->getAudioInfo();
        qDebug("orgSampleRate: %d, channelCount: %d, sampleFormat: %d",
            format.sampleRate(), format.channelCount(), format.sampleFormat()
        );

        QAudioDevice outputDevice = QMediaDevices::defaultAudioOutput();
        if (audioSink) delete audioSink;
        audioSink = new QAudioSink(outputDevice, format);
        ui.horizontalSlider->setRange(0, mediaHandle->getDuration() / 1000);
        ui.horizontalSlider->setValue(0);
    }
    if (audioBuffer) delete audioBuffer;
    if (audioBufferCache) delete audioBufferCache;
    mediaHandle->setPlayTimestamp(ui.horizontalSlider->value() * 1000);
    audioBuffer = mediaHandle->decodeAudioToQBuffer(Config::getValue("preDecodingSec").toULongLong() * 1000000);
    audioBufferCache = mediaHandle->decodeAudioToQBuffer(Config::getValue("preDecodingSec").toULongLong() * 1000000);
    audioBuffer->open(QIODevice::ReadOnly);

    ui.widgetController->setEnabled(true);
    ui.pushButtonPlay->setStatus(1);

    audioSink->setVolume(ui.sliderVolume->value() / 100.0);
    qDebug() << "Data size: " << audioBuffer->buffer().size();
    audioSink->start(audioBuffer);
    if (audioSink->state() == QtAudio::IdleState) {
        QMessageBox::critical(nullptr, "错误", "这似乎不是一个正确的音频文件！");
        ui.widgetController->setEnabled(false);
        ui.pushButtonPlay->setStatus(0);
        return;
    }
    return;
}

void TideMediaPlayer::openFile()  
{  
    // 重置信息以应对多次打开

    // 打开媒体
    if (mediaHandle)
        delete mediaHandle;
    mediaHandle = new TideMediaHandle();
    QString fileName = QFileDialog::getOpenFileName(this, tr("打开媒体文件"), "", tr("媒体文件 (*.*)"));  
    if (!fileName.isEmpty()) {  
        mediaHandle->setFileName(fileName);  
        if (!mediaHandle->loadMedia()) {
            QMessageBox::critical(nullptr, "错误", "未知的文件类型！");  
            return;
        }  
        refreshImage(true);
        refreshAudio(true);
        refreshVideo(true);
    }  
    else {  
        QMessageBox::critical(nullptr, "错误", "文件不存在或无法打开！");  
    } 
}
void TideMediaPlayer::resetScale()
{
    refreshImage(false);
    ui.pushButtonResetScale->hide();
    ui.openGLWidgetStage->m_scaled = false;
}
void TideMediaPlayer::mouseMoveEvent(QMouseEvent* event) {
    // 控制栏显示
    if (ui.openGLWidgetStage->isScaled()) {
        ui.pushButtonResetScale->show();
    }
    ui.widgetController->show();
    hideControlsTimer.start(5000);
    QMainWindow::mouseMoveEvent(event);
}
void TideMediaPlayer::resizeEvent(QResizeEvent* event) {
    // 应对调整窗口大小带来的布局问题 widgetStageGL
    ui.openGLWidgetStage->setFixedSize(event->size().width(), event->size().height() - 26);
    ui.widgetController->setGeometry(0, event->size().height() - 76, event->size().width(), 50);
    ui.labelScale->move((width() - ui.labelScale->width()) / 2, (height() - ui.labelScale->height()) / 2 - 26);
    refreshImage(false);
    QMainWindow::resizeEvent(event);
}
void TideMediaPlayer::sliderUserValueChanged()
{
    if (mediaHandle->getMediaType() == TMH_AUDIO) {
        refreshAudio(false);
    }
}
QString formatMillisecondsToTime(qint64 milliseconds) {
    qint64 totalSeconds = milliseconds / 1000;
    qint64 hours = totalSeconds / 3600;
    qint64 minutes = (totalSeconds % 3600) / 60;
    qint64 seconds = totalSeconds % 60;

    return QString("%1:%2:%3")
        .arg(hours)
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(seconds, 2, 10, QLatin1Char('0'));
}
void TideMediaPlayer::stageClockGoing()
{
    if (ui.pushButtonPlay->getStatus()) {
        if (audioSink && audioSink->state() == QtAudio::SuspendedState) audioSink->resume();
        ui.horizontalSlider->setValue(ui.horizontalSlider->value() + 10);
        ui.labelMediaStatus->setText(formatMillisecondsToTime(ui.horizontalSlider->value()) + " / " + formatMillisecondsToTime(ui.horizontalSlider->maximum()));
    }
    else {
        if (audioSink && audioSink->state() == QtAudio::ActiveState) audioSink->suspend();
    }
}
void TideMediaPlayer::volumeValueChanged(int value)
{
    qreal linearVolume = QtAudio::convertVolume(value / qreal(100.0),
        QtAudio::LogarithmicVolumeScale,
        QtAudio::LinearVolumeScale);
    audioSink->setVolume(linearVolume);
    ui.volumeLabel->setText(QString("音量: %1%2").arg(value).arg("%"));
}