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

    ui.labelScale->hide();  
    labelScaleTimer.setSingleShot(true);  
    connect(&labelScaleTimer, &QTimer::timeout, ui.labelScale, &QLabel::hide);  

    ui.widgetController->setEnabled(false);  

    ui.openGLWidgetStage->setTideMediaPlayer(this);
}  

TideMediaPlayer::~TideMediaPlayer()  
{}  

void TideMediaPlayer::showAboutDialog() {
    // 显示关于窗口
    AboutDialog* about = new AboutDialog(this);
    about->exec();
    about->deleteLater();
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
    // 是否重载？
    QPixmap pixmap;
    if (reload) {
        QByteArray imageData = mediaHandle->readAll();
        pixmap.loadFromData(imageData);
        if (!pixmap.loadFromData(imageData)) {
            qDebug() << "Unable load image.";
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
    // 刷新舞台
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
        refreshStage();
    }  
    else {  
        QMessageBox::critical(nullptr, "错误", "文件不存在或无法打开！");  
    } 
}
void TideMediaPlayer::mouseMoveEvent(QMouseEvent* event) {
    // 控制栏显示
    ui.widgetController->show();
    hideControlsTimer.start(5000);
    QMainWindow::mouseMoveEvent(event);
}
void TideMediaPlayer::resizeEvent(QResizeEvent* event) {
    // 应对调整窗口大小带来的布局问题 widgetStageGL
    ui.openGLWidgetStage->setFixedSize(event->size().width(), event->size().height() - 26);
    ui.widgetController->setGeometry(0, event->size().height() - 76, event->size().width(), 50);
    ui.labelScale->move((width() - ui.labelScale->width()) / 2, (height() - ui.labelScale->height()) / 2 - 26);
    refreshStage(false);
    QMainWindow::resizeEvent(event);
}