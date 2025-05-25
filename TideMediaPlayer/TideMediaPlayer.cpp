#include "TideMediaPlayer.h"  
#include "QFileDialog"  
#include <QMessageBox>  
#include <QPropertyAnimation>
#include "AboutDialog.h"

TideMediaPlayer::TideMediaPlayer(QWidget *parent)  
    : QMainWindow(parent)  
{  
    ui.setupUi(this);

    hideControlsTimer.setSingleShot(true);
    connect(&hideControlsTimer, &QTimer::timeout, ui.widgetController, &QWidget::hide);

    ui.widgetController->setEnabled(false);
}  

TideMediaPlayer::~TideMediaPlayer()  
{}  

void TideMediaPlayer::showAboutDialog() {
    AboutDialog* about = new AboutDialog(this);
    about->exec();
    about->deleteLater();
}

void TideMediaPlayer::refreshImage(bool reload)
{
    QPixmap pixmap;
    if (reload) {
        QByteArray imageData = mediaHandle->readAll();
        pixmap.loadFromData(imageData);
        if (!pixmap.loadFromData(imageData)) {
            qDebug() << "Unable load image.";
            QMessageBox::critical(nullptr, "错误", "加载图片失败！");
            return;
        }
        orgImagePixmap = pixmap;
    }
    else
        pixmap = orgImagePixmap;
    ui.labelStage->setPixmap(pixmap.scaled(
        ui.labelStage->contentsRect().size(),
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    ));
    ui.widgetController->setEnabled(false);
    return;
}
void TideMediaPlayer::refreshVideo(bool reload)
{
    ui.widgetController->setEnabled(true);
    return;
}
void TideMediaPlayer::refreshAudio(bool reload)
{
    ui.widgetController->setEnabled(true);
    return;
}
void TideMediaPlayer::refreshStage(bool reload)  
{  
    switch (mediaHandle->getMediaType()) {
    case TMH_AUDIO: 
        refreshAudio(reload);
        break;  
    case TMH_IMAGE:  
        refreshImage(reload);
        break;  
    case TMH_VIDEO:  
        refreshVideo(reload);
        break;  
    default:  
        break;  
    }  
}  
void TideMediaPlayer::openFile()  
{  
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
        refreshStage();
    }  
    else {  
        QMessageBox::critical(nullptr, "错误", "文件不存在或无法打开！");  
    } 
}

void TideMediaPlayer::mouseMoveEvent(QMouseEvent* event) {
    ui.widgetController->show();
    hideControlsTimer.start(5000);
    QMainWindow::mouseMoveEvent(event);
}

void TideMediaPlayer::resizeEvent(QResizeEvent* event) {
    ui.labelStage->setFixedSize(event->size().width(), event->size().height() - 26);
    ui.widgetController->setGeometry(0, event->size().height() - 76, event->size().width(), 50);
    refreshStage(false);
    QMainWindow::resizeEvent(event);
}