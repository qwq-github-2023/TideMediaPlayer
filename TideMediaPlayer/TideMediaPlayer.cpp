#include "TideMediaPlayer.h"  
#include "QFileDialog"  
#include <QMessageBox>  
#include <QPropertyAnimation>
#include "AboutDialog.h"

TideMediaPlayer::TideMediaPlayer(QWidget *parent)  
    : QMainWindow(parent)  
{  
    ui.setupUi(this);

    // 舞台优化
    ui.labelStage->setAttribute(Qt::WA_OpaquePaintEvent, false);
    ui.labelStage->setAttribute(Qt::WA_NoSystemBackground, true);

    hideControlsTimer.setSingleShot(true);
    connect(&hideControlsTimer, &QTimer::timeout, ui.widgetController, &QWidget::hide);

    ui.labelScale->hide();
    labelScaleTimer.setSingleShot(true);
    connect(&labelScaleTimer, &QTimer::timeout, ui.labelScale, &QLabel::hide);

    ui.widgetController->setEnabled(false);
}  

TideMediaPlayer::~TideMediaPlayer()  
{}  

void TideMediaPlayer::showAboutDialog() {
    // 显示关于窗口
    AboutDialog* about = new AboutDialog(this);
    about->exec();
    about->deleteLater();
}
void TideMediaPlayer::showScaleLabel()
{
    // 显示缩放倍率
    ui.labelScale->setText(QString("%1%").arg(QString::number(static_cast<int>(imageScale * 100))));
    ui.labelScale->show();
    labelScaleTimer.start(2000);
}
void TideMediaPlayer::adjustOffsetWithinBounds(const QSize& imageSize, const QSize& labelSize)
{
    int maxX = 0;
    int maxY = 0;
    int minX = labelSize.width() - imageSize.width();
    int minY = labelSize.height() - imageSize.height();

    // 如果图像比窗口小，允许居中（不要限制拖动）
    if (imageSize.width() <= labelSize.width())
        offset.setX((labelSize.width() - imageSize.width()) / 2);
    else
        offset.setX(qBound(minX, offset.x(), maxX));

    if (imageSize.height() <= labelSize.height())
        offset.setY((labelSize.height() - imageSize.height()) / 2);
    else
        offset.setY(qBound(minY, offset.y(), maxY));
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
    // 精度损失警告
    if (imageScale == -1) {
        imageScale = qMin(
            static_cast<double>(ui.labelStage->contentsRect().size().width()) / pixmap.width(), 
            static_cast<double>(ui.labelStage->contentsRect().size().height()) / pixmap.height()
        );
        showScaleLabel();
        offset.setX((width() - pixmap.width() * imageScale) / 2);
        offset.setY((height() - pixmap.height() * imageScale) / 2);
    }
    QPixmap scaled;
    if (pixmap.size() * imageScale == ui.labelStage->pixmap().size())
        scaled = ui.labelStage->pixmap();
    else {
        Qt::TransformationMode mode = (imageScale < 1.0) ? Qt::FastTransformation : Qt::SmoothTransformation;
        scaled = pixmap.scaled(
            pixmap.size() * imageScale,
            Qt::KeepAspectRatio,
            mode
        );
    }

    QSize labelSize = ui.labelStage->size(); 
    QPixmap canvas(labelSize);
    canvas.fill(Qt::black);

    QPainter painter(&canvas);

    // 源图像裁剪区域：从偏移位置截取视图
    QRect sourceRect = QRect(-offset, labelSize);
    sourceRect = sourceRect.intersected(QRect(QPoint(0, 0), scaled.size()));

    // 目标绘制位置：根据 offset
    QPoint drawPos = offset;

    // 裁剪后也限制目标区域不越界
    QRect targetRect(drawPos, sourceRect.size());
    targetRect = targetRect.intersected(QRect(QPoint(0, 0), labelSize));

    painter.drawPixmap(targetRect.topLeft(), scaled, sourceRect);
    ui.labelStage->setPixmap(canvas);
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
    imageScale = -1;
    offset = QPoint(0, 0);

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

    // 拖拽事件
    if (event->buttons() & Qt::LeftButton) {
        QPoint delta = event->pos() - lastMousePos;
        offset += delta;
        lastMousePos = event->pos();
        refreshStage(false);
    }

    QMainWindow::mouseMoveEvent(event);
}
void TideMediaPlayer::wheelEvent(QWheelEvent* event)
{
    // 处理缩放
    constexpr double zoomStep = 0.05;
    int numDegrees = event->angleDelta().y() / 8;
    int numSteps = numDegrees / 15;

    if (numSteps == 0) {
        event->accept();
        return;
    }

    double factor = std::pow(1.0 + zoomStep, numSteps);
    double newScale = imageScale * factor;

    // 限制缩放范围
    if (newScale < 0.01) newScale = 0.01;
    if (newScale > 10.0) newScale = 10.0;

    // 缩放中心处理（以鼠标为缩放中心）
    QPoint mousePos = event->position().toPoint();
    QPoint delta = mousePos - offset;
    offset = mousePos - delta * (newScale / imageScale);

    imageScale = newScale;
    refreshStage(false);
    showScaleLabel();
    event->accept();
}
void TideMediaPlayer::mousePressEvent(QMouseEvent* event)
{
    // 记录上次鼠标左键点击位置以应对媒体拖拽
    if (event->button() == Qt::LeftButton) {
        lastMousePos = event->pos();
    }
}
void TideMediaPlayer::resizeEvent(QResizeEvent* event) {
    // 应对调整窗口大小带来的布局问题
    ui.labelStage->setFixedSize(event->size().width(), event->size().height() - 26);
    ui.widgetController->setGeometry(0, event->size().height() - 76, event->size().width(), 50);
    ui.labelScale->move((width() - ui.labelScale->width()) / 2, (height() - ui.labelScale->height()) / 2 - 26);
    refreshStage(false);
    QMainWindow::resizeEvent(event);
}