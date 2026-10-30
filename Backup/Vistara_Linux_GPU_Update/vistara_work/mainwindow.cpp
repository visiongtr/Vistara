#include "mainwindow.h"
#include "imageview.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QImageReader>
#include <QToolBar>
#include <QAction>
#include <QStatusBar>
#include <QPixmap>
#include <QScreen>
#include <QGuiApplication>
#include <QApplication>
#include <QTimer>
#include <QPointer>
#include <QCoreApplication>
#include <QMetaObject>
#include <QKeySequence>
#include <QStyle>
#include <QRunnable>
#include <QImage>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    m_loaderPool.setMaxThreadCount(1); // Avoid decoding multiple large images simultaneously.
    m_loaderPool.setExpiryTimeout(-1);
    m_view = new ImageView(this);
    setCentralWidget(m_view);

    connect(m_view, &ImageView::requestNext, this, &MainWindow::nextImage);
    connect(m_view, &ImageView::requestPrev, this, &MainWindow::prevImage);
    connect(m_view, &ImageView::zoomPercentChanged, this, [this](int pct) {
        m_zoomPct = pct;
        updateStatus();
    });
    connect(m_view, &ImageView::requestToggleFullscreen, this, &MainWindow::toggleFullscreen);
    connect(m_view, &ImageView::requestEscape, this, &MainWindow::handleEscape);

    m_toolbar = addToolBar(QStringLiteral("Main"));
    m_toolbar->setMovable(false);

    auto *open = m_toolbar->addAction(style()->standardIcon(QStyle::SP_DialogOpenButton), QStringLiteral("Open"));
    connect(open, &QAction::triggered, this, &MainWindow::openDialog);

    m_toolbar->addSeparator();
    auto *prev = m_toolbar->addAction(style()->standardIcon(QStyle::SP_ArrowBack), QStringLiteral("Previous"));
    connect(prev, &QAction::triggered, this, &MainWindow::prevImage);
    auto *next = m_toolbar->addAction(style()->standardIcon(QStyle::SP_ArrowForward), QStringLiteral("Next"));
    connect(next, &QAction::triggered, this, &MainWindow::nextImage);

    m_toolbar->addSeparator();
    auto *fit = m_toolbar->addAction(QStringLiteral("Fit"));
    fit->setToolTip(QStringLiteral("Fit to window (F)"));
    connect(fit, &QAction::triggered, m_view, &ImageView::fitToWindow);
    auto *actual = m_toolbar->addAction(QStringLiteral("100%"));
    actual->setToolTip(QStringLiteral("Actual size (1)"));
    connect(actual, &QAction::triggered, m_view, &ImageView::actualSize);
    m_fullscreenAction = m_toolbar->addAction(style()->standardIcon(QStyle::SP_TitleBarMaxButton), QStringLiteral("Fullscreen"));
    connect(m_fullscreenAction, &QAction::triggered, this, &MainWindow::toggleFullscreen);

    statusBar()->showMessage(QStringLiteral("Ready"));
    setWindowTitle(QStringLiteral("Vistara"));
    resize(1100, 800);
    if (const auto *screen = QGuiApplication::primaryScreen()) {
        const QRect r = screen->availableGeometry();
        move(r.center().x() - width() / 2, r.center().y() - height() / 2);
    }
    QTimer::singleShot(0, this, [this] { m_view->setFocus(); });
}

MainWindow::~MainWindow()
{
    m_loaderPool.clear();
    m_loaderPool.waitForDone();
}

void MainWindow::openDialog()
{
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Open image"), QString(),
        QStringLiteral("Images (*.png *.jpg *.jpeg *.bmp *.gif *.webp *.tif *.tiff)"));
    if (!path.isEmpty()) openFile(path);
}

QStringList MainWindow::buildImageList(const QString& currentFilePath) const
{
    const QDir dir(QFileInfo(currentFilePath).absolutePath());
    const QStringList filters = {"*.jpg", "*.jpeg", "*.png", "*.bmp", "*.gif", "*.webp", "*.tif", "*.tiff"};
    const QFileInfoList infos = dir.entryInfoList(filters,
        QDir::Files | QDir::Readable, QDir::Name | QDir::IgnoreCase);
    QStringList results;
    results.reserve(infos.size());
    for (const auto& f : infos) results.push_back(f.absoluteFilePath());
    return results;
}

void MainWindow::openFile(const QString& path)
{
    const QFileInfo fi(path);
    if (!fi.isFile()) return;
    m_files = buildImageList(fi.absoluteFilePath());
    m_index = m_files.indexOf(fi.absoluteFilePath());
    if (m_index < 0) { m_files.prepend(fi.absoluteFilePath()); m_index = 0; }
    loadAtIndex(m_index);
}

void MainWindow::loadAtIndex(int index)
{
    if (m_files.isEmpty()) return;
    index = (index % m_files.size() + m_files.size()) % m_files.size();
    m_index = index;
    const QString path = m_files.at(index);
    const int request = ++m_requestId;

    m_loaderPool.clear(); // Drop obsolete queued loads when navigating quickly.
    statusBar()->showMessage(QStringLiteral("Opening %1...").arg(QFileInfo(path).fileName()));
    setWindowTitle(QStringLiteral("Vistara - %1").arg(QFileInfo(path).fileName()));
    QPointer<MainWindow> guard(this);
    // QImageReader is worker-thread local; QPixmap and widgets remain GUI-thread only.
    m_loaderPool.start([guard, path, request] {
        QImageReader reader(path);
        reader.setAutoTransform(true);
        QImage image = reader.read();
        const QString error = reader.errorString();
        auto *app = QCoreApplication::instance();
        if (!app) return;
        QMetaObject::invokeMethod(app, [guard, request, path, image = std::move(image), error] () mutable {
            if (!guard || request != guard->m_requestId) return;
            if (image.isNull()) {
                guard->statusBar()->showMessage(QStringLiteral("Could not open %1: %2")
                    .arg(QFileInfo(path).fileName(), error));
                return;
            }
            guard->m_imgSize = image.size();
            guard->m_view->setImage(QPixmap::fromImage(std::move(image)));
            guard->updateStatus();
        }, Qt::QueuedConnection);
    });
}

void MainWindow::nextImage() { if (!m_files.isEmpty()) loadAtIndex(m_index + 1); }
void MainWindow::prevImage() { if (!m_files.isEmpty()) loadAtIndex(m_index - 1); }

void MainWindow::toggleFullscreen()
{
    if (isFullScreen()) {
        showNormal();
        m_toolbar->show();
        statusBar()->show();
    } else {
        m_toolbar->hide();
        statusBar()->hide();
        showFullScreen();
    }
    updateFullscreenAction();
    m_view->setFocus();
}

void MainWindow::handleEscape()
{
    if (isFullScreen()) toggleFullscreen();
    else close();
}

void MainWindow::updateFullscreenAction()
{
    m_fullscreenAction->setText(isFullScreen() ? QStringLiteral("Windowed") : QStringLiteral("Fullscreen"));
}

void MainWindow::updateStatus()
{
    const QString name = (m_index >= 0 && m_index < m_files.size())
        ? QFileInfo(m_files.at(m_index)).fileName() : QString();
    const QString dims = m_imgSize.isValid() ?
        QStringLiteral("%1 × %2").arg(m_imgSize.width()).arg(m_imgSize.height()) : QString();
    statusBar()->showMessage(QStringLiteral("%1  |  %2  |  %3%  |  %4/%5  |  %6")
        .arg(name, dims)
        .arg(m_zoomPct)
        .arg(m_index + 1)
        .arg(m_files.size())
        .arg(m_view->usingOpenGL() ? QStringLiteral("OpenGL") : QStringLiteral("CPU")));
}
