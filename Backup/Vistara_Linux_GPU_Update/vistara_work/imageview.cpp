#include "imageview.h"

#include <QWheelEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QSurfaceFormat>
#include <QCoreApplication>
#ifdef VISTARA_HAS_OPENGL
#include <QOpenGLWidget>
#endif
#include <algorithm>
#include <cmath>

ImageView::ImageView(QWidget* parent) : QGraphicsView(parent)
{
    setScene(&m_scene);
    m_item = m_scene.addPixmap(QPixmap());

#ifdef VISTARA_HAS_OPENGL
    // --no-gpu permits a CPU-only fallback without changing settings.
    if (!QCoreApplication::arguments().contains(QStringLiteral("--no-gpu"))) {
        auto *glViewport = new QOpenGLWidget(this);
        setViewport(glViewport);
        setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
        m_usingOpenGL = true;
    }
#endif

    setRenderHint(QPainter::SmoothPixmapTransform, true);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setFocusPolicy(Qt::StrongFocus);
    setBackgroundBrush(QColor(35, 35, 35));
}

void ImageView::setImage(const QPixmap& pm)
{
    m_item->setPixmap(pm);
    m_scene.setSceneRect(QRectF(pm.rect()));
    fitToWindow();
}

void ImageView::resizeEvent(QResizeEvent* e)
{
    QGraphicsView::resizeEvent(e);
    if (m_mode == ZoomMode::Fit)
        fitToWindow();
}

void ImageView::fitToWindow()
{
    m_mode = ZoomMode::Fit;
    if (m_item->pixmap().isNull()) return;
    resetTransform();
    fitInView(m_item, Qt::KeepAspectRatio);
    m_scale = transform().m11();
    updateZoomSignal();
}

void ImageView::actualSize()
{
    m_mode = ZoomMode::Actual100;
    if (m_item->pixmap().isNull()) return;
    resetTransform();
    m_scale = 1.0;
    centerOn(m_item);
    updateZoomSignal();
}

void ImageView::setCustomZoom(double factor)
{
    m_mode = ZoomMode::Custom;
    if (m_item->pixmap().isNull()) return;
    m_scale = std::clamp(factor, 0.02, 32.0);
    resetTransform();
    scale(m_scale, m_scale);
    updateZoomSignal();
}

void ImageView::wheelEvent(QWheelEvent* e)
{
    if (m_item->pixmap().isNull()) {
        QGraphicsView::wheelEvent(e);
        return;
    }
    if (e->modifiers() & Qt::ShiftModifier) {
        QGraphicsView::wheelEvent(e);
        return;
    }

    const QPoint angle = e->angleDelta();
    const QPoint pixel = e->pixelDelta();
    const double delta = angle.y() != 0 ? double(angle.y()) / 120.0 : double(pixel.y()) / 120.0;
    if (delta == 0.0) { e->accept(); return; }
    const double base = (e->modifiers() & Qt::ControlModifier) ? 1.05 : 1.12;
    const double desired = std::clamp(m_scale * std::pow(base, delta), 0.02, 32.0);
    const double ratio = desired / m_scale;
    m_mode = ZoomMode::Custom;
    scale(ratio, ratio);
    m_scale = desired;
    updateZoomSignal();
    e->accept();
}

void ImageView::mouseDoubleClickEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton) {
        emit requestToggleFullscreen();
        e->accept();
        return;
    }
    QGraphicsView::mouseDoubleClickEvent(e);
}

void ImageView::mousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::BackButton) {
        emit requestPrev(); e->accept(); return;
    }
    if (e->button() == Qt::ForwardButton) {
        emit requestNext(); e->accept(); return;
    }
    QGraphicsView::mousePressEvent(e);
}

void ImageView::keyPressEvent(QKeyEvent* e)
{
    switch (e->key()) {
        case Qt::Key_Right: case Qt::Key_Down:
        case Qt::Key_PageDown: case Qt::Key_Space:
            emit requestNext(); e->accept(); return;
        case Qt::Key_Left: case Qt::Key_Up:
        case Qt::Key_PageUp: case Qt::Key_Backspace:
            emit requestPrev(); e->accept(); return;
        case Qt::Key_F:
            fitToWindow(); e->accept(); return;
        case Qt::Key_1:
            actualSize(); e->accept(); return;
        case Qt::Key_F11:
            emit requestToggleFullscreen(); e->accept(); return;
        case Qt::Key_Escape:
            emit requestEscape(); e->accept(); return;
    }
    QGraphicsView::keyPressEvent(e);
}

void ImageView::updateZoomSignal()
{
    emit zoomPercentChanged(int(std::lround(m_scale * 100.0)));
}
