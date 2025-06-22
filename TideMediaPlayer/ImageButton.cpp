#include "ImageButton.h"
#include <QtWidgets>
#include <QPixmap>
#include <QSvgRenderer>
ImageButton::ImageButton(QWidget* parent)
    : QPushButton(parent)
    //设置按钮初始状态
    , curStatus_(ST_INIT)
{
    //设置按钮状态为正常
    curStatus_ = ST_NORMAL;
    //存储按钮三态图片
    isPlaying = false;
    imageName_[ST_NORMAL] = ":/PlayButton/Icons/PlayBtn-Disable.svg";
    imageName_[ST_HOVER] = ":/PlayButton/Icons/PlayBtn-Disable.svg";
    imageName_[ST_PRESS] = ":/PlayButton/Icons/PlayBtn-Disable.svg";

    //获得图片的大小以便用来调整按钮的大小
    QPixmap icon;
    icon.load(imageName_[ST_NORMAL]);
    this->icon_width = icon.width();
    this->icon_height = icon.height();
    this->setGeometry(0, 0, icon_width, icon_height);
}

//按钮进入事件
void ImageButton::enterEvent(QEvent*)
{
    if (curStatus_ == ST_INIT)
    {
        return;
    }

    curStatus_ = ST_NORMAL;
    update();
}

//按钮离开事件
void ImageButton::leaveEvent(QEvent*)
{
    if (curStatus_ == ST_INIT)
    {
        return;
    }

    curStatus_ = ST_NORMAL;
    update();
}

//按钮按下事件
void ImageButton::mousePressEvent(QMouseEvent* event)
{
    if (curStatus_ == ST_INIT)
    {
        return;
    }

    //如果鼠标左键点击
    if (event->button() == Qt::LeftButton)
    {
        //将按钮状态设置为按下，并绘图
        curStatus_ = ST_PRESS;
        update();

    }

}

//按钮释放事件
void ImageButton::mouseReleaseEvent(QMouseEvent* event)
{
    //如果鼠标左键释放
    if (event->button() == Qt::LeftButton)
    {
        if (isPlaying) {
            imageName_[ST_NORMAL] = ":/PlayButton/Icons/PlayBtn-Pause.svg";
            imageName_[ST_HOVER] = ":/PlayButton/Icons/PlayBtn-PauseHover.svg";
            imageName_[ST_PRESS] = ":/PlayButton/Icons/PlayBtn-PauseHover.svg";
            isPlaying = false;
        }
        else {
            imageName_[ST_NORMAL] = ":/PlayButton/Icons/PlayBtn-Play.svg";
            imageName_[ST_HOVER] = ":/PlayButton/Icons/PlayBtn-PlayHover.svg";
            imageName_[ST_PRESS] = ":/PlayButton/Icons/PlayBtn-PlayHover.svg";
            isPlaying = true;
        }
        if (!parentWidget()->isEnabled()) {
            imageName_[ST_NORMAL] = ":/PlayButton/Icons/PlayBtn-Disable.svg";
            imageName_[ST_HOVER] = ":/PlayButton/Icons/PlayBtn-Disable.svg";
            imageName_[ST_PRESS] = ":/PlayButton/Icons/PlayBtn-Disable.svg";
        }
        if (curStatus_ != ST_INIT)
        {
            //将按钮状态设置为悬停，并绘图
            curStatus_ = ST_HOVER;
            update();
        }
    }
    // 鼠标在弹起的时候光标在按钮上才激发clicked信号
    if (rect().contains(event->pos()))
    {
        emit clicked();
    }
}

//按钮样式绘图
void ImageButton::paintEvent(QPaintEvent* event)
{
    if (curStatus_ == ST_INIT)
    {
        QPushButton::paintEvent(event);
        return;
    }

    
    QPainter painter(this);
    // QPixmap pixmap(imageName_[curStatus_]);
    // painter.setRenderHint(QPainter::Antialiasing);
    // painter.drawPixmap(rect(), pixmap);
    QSvgRenderer svgRender;
    svgRender.load(imageName_[curStatus_]);
    svgRender.render(&painter, this->rect());
}