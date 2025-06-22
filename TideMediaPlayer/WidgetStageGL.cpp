#include "WidgetStageGL.h"
#include "TideMediaPlayer.h"  
#include <QPainter>
#include <QWheelEvent>
#include <QMouseEvent>

WidgetStageGL::WidgetStageGL(QWidget* parent)
    : QOpenGLWidget(parent)
{
    setMouseTracking(true);
}

bool WidgetStageGL::isScaled()
{
    return m_scaled;
}

void WidgetStageGL::loadPixmap(const QPixmap& pixmap)
{
    m_scaled = false;
    m_pixmap = pixmap;
    m_scaleFactor = qMin(
        static_cast<qreal>(this->size().height()) / m_pixmap.size().height(),
        static_cast<qreal>(this->size().width()) / m_pixmap.size().width()
    );
    
    QSizeF widgetSize = size();
    QSizeF pixmapSize = m_pixmap.size() * m_scaleFactor;

    m_offset = QPointF(
        (widgetSize.width() - pixmapSize.width()) / 2,
        (widgetSize.height() - pixmapSize.height()) / 2
    );

    m_tideMediaPlayer->showScaleLabel(m_scaleFactor);
    update();
}

void WidgetStageGL::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.translate(m_offset);
    painter.scale(m_scaleFactor, m_scaleFactor);
    painter.drawPixmap(0, 0, m_pixmap);
}

void WidgetStageGL::wheelEvent(QWheelEvent* event)
{
    const QPointF pos = event->position();
    const qreal oldScale = m_scaleFactor;
    m_scaleFactor *= (event->angleDelta().y() > 0) ? 1.05 : 0.95;
    if (m_scaleFactor < 0.01)
        m_scaleFactor = 0.01;
    if (m_scaleFactor > 100.0)
        m_scaleFactor = 100.0;
    // 保持以鼠标为中心缩放
    QPointF delta = (pos - m_offset) / oldScale;
    m_offset = pos - delta * m_scaleFactor;

    m_tideMediaPlayer->showScaleLabel(m_scaleFactor);

    m_tideMediaPlayer->ui.pushButtonResetScale->show();
    m_scaled = true;
    update();
}

void WidgetStageGL::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
        m_lastMousePos = event->pos();
}

void WidgetStageGL::mouseMoveEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton) {
        QPointF delta = event->pos() - m_lastMousePos;
        m_offset += delta;
        m_lastMousePos = event->pos();

        m_tideMediaPlayer->ui.pushButtonResetScale->show();
        m_scaled = true;
        update();
    }
    m_tideMediaPlayer->mouseMoveEvent(event);
}
