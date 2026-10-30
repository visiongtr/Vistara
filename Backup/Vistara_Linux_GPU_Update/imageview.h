#pragma once
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>

class ImageView : public QGraphicsView
{
    Q_OBJECT
public:
    enum class ZoomMode { Fit, Actual100, Custom };
    explicit ImageView(QWidget *parent = nullptr);
    void setImage(const QPixmap &pm);
    void fitToWindow();
    void actualSize();
    void setCustomZoom(double scale);
    double currentScale() const { return m_scale; }
    ZoomMode zoomMode() const { return m_mode; }
    bool gpuEnabled() const { return m_gpuEnabled; }
signals:
    void requestNext();
    void requestPrev();
    void zoomPercentChanged(int percent);
protected:
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
private:
    void updateZoomSignal();
    QGraphicsScene m_scene;
    QGraphicsPixmapItem *m_item = nullptr;
    ZoomMode m_mode = ZoomMode::Fit;
    double m_scale = 1.0;
    bool m_gpuEnabled = false;
};
