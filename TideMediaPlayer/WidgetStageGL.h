#ifndef WIDGETSTAGEGL_H
#define WIDGETSTAGEGL_H
#include <QOpenGLWidget>
#include <QPixmap>
#include <QPointF>

class TideMediaPlayer;
class WidgetStageGL : public QOpenGLWidget
{
    Q_OBJECT

public:
    explicit WidgetStageGL(QWidget* parent = nullptr);
    void loadPixmap(const QPixmap& pixmap);
    void setTideMediaPlayer(TideMediaPlayer* _) { m_tideMediaPlayer = _; };
protected:
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    QPixmap m_pixmap;
    QPointF m_offset;
    QPointF m_lastMousePos;
    qreal m_scaleFactor = 1.0;
    TideMediaPlayer* m_tideMediaPlayer;
};

#endif // WIDGETSTAGEGL_H
