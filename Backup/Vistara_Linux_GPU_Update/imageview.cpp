#include "imageview.h"
#include <QWheelEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QAction>
#include <QColor>
#include <QTransform>
#include <QOpenGLWidget>
#include <QOpenGLContext>
#include <QSurfaceFormat>
#include <QByteArray>
#include <algorithm>
#include <cmath>

ImageView::ImageView(QWidget *parent) : QGraphicsView(parent)
{
    setScene(&m_scene);
    m_item = m_scene.addPixmap(QPixmap());
    // An OpenGL viewport accelerates drawing/scaling; decoding remains CPU-based.
    // Set VISTARA_SOFTWARE=1 to use Qt's regular raster viewport.
    if (qEnvironmentVariableIntValue("VISTARA_SOFTWARE") != 1) {
        QOpenGLContext probe;
        if (probe.create()) {
            setViewport(new QOpenGLWidget(this));
            setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
            m_gpuEnabled = true;
        }
    }
    setRenderHint(QPainter::SmoothPixmapTransform, true);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setFocusPolicy(Qt::StrongFocus);
    setBackgroundBrush(QColor(33, 33, 33));
}

void ImageView::setImage(const QPixmap &pm)
{
    m_item->setPixmap(pm);
    m_scene.setSceneRect(QRectF(pm.rect()));
    fitToWindow();
}

void ImageView::resizeEvent(QResizeEvent *e)
{
    QGraphicsView::resizeEvent(e);
    if (m_mode == ZoomMode::Fit) fitToWindow();
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
    updateZoomSignal();
}

void ImageView::setCustomZoom(double factor)
{
    m_mode = ZoomMode::Custom;
    if (m_item->pixmap().isNull()) return;
    m_scale = std::clamp(factor, 0.02, 32.0);
    QTransform t;
    t.scale(m_scale, m_scale);
    setTransform(t);
    updateZoomSignal();
}

void ImageView::wheelEvent(QWheelEvent *e)
{
    if (m_item->pixmap().isNull() || (e->modifiers() & Qt::ShiftModifier)) {
        QGraphicsView::wheelEvent(e);
        return;
    }
    const QPoint delta = e->angleDelta();
    if (delta.y() == 0) { e->ignore(); return; }
    const double base = (e->modifiers() & Qt::ControlModifier) ? 1.05 : 1.12;
    const double factor = std::pow(base, delta.y() / 120.0);
    const double target = std::clamp(m_scale * factor, 0.02, 32.0);
    if (std::abs(target - m_scale) > 1e-9) {
        scale(target / m_scale, target / m_scale);
        m_scale = target;
        m_mode = ZoomMode::Custom;
        updateZoomSignal();
    }
    e->accept();
}

void ImageView::mouseDoubleClickEvent(QMouseEvent *e)
{
    // Fullscreen is handled by a shortcut/action in the main window.
    if (e->button() == Qt::LeftButton) {
        if (auto *w = window()) {
            if (auto *act = w->findChild<QAction *>(QStringLiteral("fullscreenAction")))
                act->trigger();
        }
        e->accept();
        return;
    }
    QGraphicsView::mouseDoubleClickEvent(e);
}

void ImageView::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::BackButton) { emit requestPrev(); e->accept(); return; }
    if (e->button() == Qt::ForwardButton) { emit requestNext(); e->accept(); return; }
    QGraphicsView::mousePressEvent(e);
}

void ImageView::keyPressEvent(QKeyEvent *e)
{
    switch (e->key()) {
    case Qt::Key_Right: case Qt::Key_Down: case Qt::Key_PageDown: case Qt::Key_Space:
        emit requestNext(); e->accept(); return;
    case Qt::Key_Left: case Qt::Key_Up: case Qt::Key_PageUp: case Qt::Key_Backspace:
        emit requestPrev(); e->accept(); return;
    default: break;
    }
    QGraphicsView::keyPressEvent(e);
}

void ImageView::updateZoomSignal()
{
    emit zoomPercentChanged(static_cast<int>(std::lround(m_scale * 100.0)));
}
