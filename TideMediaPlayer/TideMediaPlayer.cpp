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
    
    connect(&stageSliderTimer, &QTimer::timeout, this, &TideMediaPlayer::stageClockGoing);
    stageSliderTimer.setSingleShot(false);
    stageSliderTimer.setInterval(100);
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
void TideMediaPlayer::FuckAudioCache() {
    while (audioStream && mediaHandle->getMediaType() != TMH_IMAGE) {
        if (audioStream->usedBytes() * 2 > audioStream->capacitySize()) {
            ::Sleep(200);
            continue;
        }
        audioStream->printStatus();
        qDebug() << "(bufferLow)Data size: " << audioBuffer->buffer().size();
        audioStream->writeData(audioBuffer->buffer().data(), audioBuffer->buffer().size());
        delete audioBuffer;
        if (audioBufferCache) {
            audioBuffer = audioBufferCache;
        }
        audioBufferCache = changePlaybackSpeed(
            mediaHandle->decodeAudioToQBuffer(Config::getValue("preDecodingSec").toULongLong() * 1000000),
            mediaHandle->getAudioInfo(),
            ui.doubleSpinBoxTripleSpeed->value());
    }
    
}
void TideMediaPlayer::refreshAudio(bool reload)
{
    if (mediaHandle->getMediaType() != TMH_AUDIO) return;
    if (audioSink) audioSink->stop();
    if (reload) {
        ui.horizontalSlider->setRange(0, mediaHandle->getDuration() / 1000);
        ui.horizontalSlider->setValue(0);
        QAudioFormat format = mediaHandle->getAudioInfo();
        qDebug("orgSampleRate: %d, channelCount: %d, sampleFormat: %d",
            format.sampleRate(), format.channelCount(), format.sampleFormat()
        );

        QAudioDevice outputDevice = QMediaDevices::defaultAudioOutput();
        if (audioSink) delete audioSink;
        audioSink = new QAudioSink(outputDevice, format);
        connect(audioSink, &QAudioSink::stateChanged, this, [&](QAudio::State state) {
            qDebug() << "QAudoSink state changed: " << audioSink->state();
            });
    }
    if (audioBuffer) delete audioBuffer;
    if (audioBufferCache) delete audioBufferCache;
    if (audioStream) delete audioStream;
    mediaHandle->setPlayTimestamp(ui.horizontalSlider->value() * 1000);

    audioBuffer = changePlaybackSpeed(
        mediaHandle->decodeAudioToQBuffer(Config::getValue("preDecodingSec").toULongLong() * 1000000),
        mediaHandle->getAudioInfo(),
        ui.doubleSpinBoxTripleSpeed->value());
    
    audioStream = new TideIODevice(audioBuffer->buffer().size() * 5);
    audioStream->writeData(audioBuffer->buffer().data(), audioBuffer->buffer().size());
    audioStream->printStatus();
    qDebug() << "WriteData size: " << audioBuffer->buffer().size();
    delete audioBuffer;
    for (int i=0; i < 2; ++i) {
        audioBuffer = changePlaybackSpeed(
            mediaHandle->decodeAudioToQBuffer(Config::getValue("preDecodingSec").toULongLong() * 1000000),
            mediaHandle->getAudioInfo(),
            ui.doubleSpinBoxTripleSpeed->value());
        audioStream->writeData(audioBuffer->buffer().data(), audioBuffer->buffer().size());
        audioStream->printStatus();
        qDebug() << "WriteData size: " << audioBuffer->buffer().size();
        delete audioBuffer;
    }
    
    audioBuffer = changePlaybackSpeed(
        mediaHandle->decodeAudioToQBuffer(Config::getValue("preDecodingSec").toULongLong() * 1000000),
        mediaHandle->getAudioInfo(),
        ui.doubleSpinBoxTripleSpeed->value());
    audioBufferCache = changePlaybackSpeed(
        mediaHandle->decodeAudioToQBuffer(Config::getValue("preDecodingSec").toULongLong() * 1000000),
        mediaHandle->getAudioInfo(),
        ui.doubleSpinBoxTripleSpeed->value());
    // connect(audioStream, &TideIODevice::bufferLow, this, &TideMediaPlayer::FuckAudioCache, Qt::QueuedConnection);
    // std::thread([&]() {FuckAudioCache(); }).detach();
    
    audioSink->setVolume(ui.sliderVolume->value() / 100.0);
    audioSink->start(audioStream);

    ui.widgetController->setEnabled(true);
    ui.pushButtonPlay->setStatus(1);

    return;
}

QBuffer* TideMediaPlayer::changePlaybackSpeed(QBuffer* inputBuffer, const QAudioFormat& format, float speedRatio)
{
    using namespace soundtouch;

    if (!inputBuffer || format.sampleFormat() != QAudioFormat::Int16) {
        qWarning() << "Invalid input buffer or unsupported format (only S16 supported).";
        return nullptr;
    }
    inputBuffer->open(QIODevice::ReadOnly);
    int channels = format.channelCount();
    int sampleRate = format.sampleRate();
    const int bytesPerSample = 2;
    const int frameSize = bytesPerSample * channels;

    QByteArray inputData = inputBuffer->data();
    const int16_t* inSamples = reinterpret_cast<const int16_t*>(inputData.constData());
    int totalSamples = inputData.size() / bytesPerSample;

    // 预转换 S16 -> float
    std::vector<float> floatSamples(totalSamples);
    const float invScale = 1.0f / 32768.0f;
    for (int i = 0; i < totalSamples; ++i) {
        floatSamples[i] = inSamples[i] * invScale;
    }

    // SoundTouch 配置
    soundTouch.setSampleRate(sampleRate);
    soundTouch.setChannels(channels);
    soundTouch.setTempo(speedRatio);

    QByteArray result;
    result.reserve(inputData.size());  // 最小预留原大小

    constexpr int blockSize = 4096;
    float outBuffer[blockSize];  // 栈上分配
    int processed = 0;

    // 主处理循环
    int i = 0;
    while (i < totalSamples) {
        int remain = totalSamples - i;
        int send = remain > blockSize ? blockSize : remain;
        soundTouch.putSamples(&floatSamples[i], send / channels);
        i += send;

        int received;
        do {
            received = soundTouch.receiveSamples(outBuffer, blockSize / channels);
            if (received > 0) {
                const int outCount = received * channels;
                for (int j = 0; j < outCount; ++j) {
                    float v = std::clamp(outBuffer[j], -1.0f, 1.0f);
                    int16_t sample = static_cast<int16_t>(v * 32767.0f);
                    result.append(reinterpret_cast<const char*>(&sample), 2);
                }
                processed += received;
            }
        } while (received > 0);
    }

    // flush
    soundTouch.flush();
    int received;
    do {
        received = soundTouch.receiveSamples(outBuffer, blockSize / channels);
        const int outCount = received * channels;
        for (int j = 0; j < outCount; ++j) {
            float v = std::clamp(outBuffer[j], -1.0f, 1.0f);
            int16_t sample = static_cast<int16_t>(v * 32767.0f);
            result.append(reinterpret_cast<const char*>(&sample), 2);
        }
    } while (received > 0);

    // 输出
    QBuffer* outputBuffer = new QBuffer();
    outputBuffer->setData(result);
    outputBuffer->open(QIODevice::ReadOnly);
    return outputBuffer;
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
        this->setWindowTitle("TideMediaPlayer - " + QFileInfo(fileName).fileName());
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
    if (mediaHandle->getMediaType() != TMH_IMAGE) {
        ui.widgetController->show();
        hideControlsTimer.start(5000);
    }
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
        ui.horizontalSlider->setValue(ui.horizontalSlider->value() + 100 * ui.doubleSpinBoxTripleSpeed->value());
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

void TideMediaPlayer::speedRatioChanged(double)
{
    refreshAudio(false);
}
