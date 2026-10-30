#include "mainwindow.h"
#include "imageview.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QImageReader>
#include <QImage>
#include <QToolBar>
#include <QAction>
#include <QStatusBar>
#include <QPixmap>
#include <QScreen>
#include <QGuiApplication>
#include <QTimer>
#include <QStyle>
#include <QFutureWatcher>
#include <QtConcurrent>
#include <QKeySequence>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    m_loaderPool.setMaxThreadCount(2);
    m_view = new ImageView(this);
    setCentralWidget(m_view);
    connect(m_view, &ImageView::requestNext, this, &MainWindow::nextImage);
    connect(m_view, &ImageView::requestPrev, this, &MainWindow::prevImage);
    connect(m_view, &ImageView::zoomPercentChanged, this, [this](int pct) {
        m_zoomPct = pct;
        updateStatus();
    });

    m_toolbar = addToolBar(QStringLiteral("Main"));
    m_toolbar->setMovable(false);
    m_toolbar->setObjectName(QStringLiteral("mainToolbar"));

    QAction *open = m_toolbar->addAction(style()->standardIcon(QStyle::SP_DialogOpenButton), QStringLiteral("Open"));
    connect(open, &QAction::triggered, this, &MainWindow::openDialog);
    QAction *prev = m_toolbar->addAction(style()->standardIcon(QStyle::SP_ArrowBack), QStringLiteral("Previous"));
    connect(prev, &QAction::triggered, this, &MainWindow::prevImage);
    QAction *next = m_toolbar->addAction(style()->standardIcon(QStyle::SP_ArrowForward), QStringLiteral("Next"));
    connect(next, &QAction::triggered, this, &MainWindow::nextImage);
    m_toolbar->addSeparator();
    QAction *fit = m_toolbar->addAction(QStringLiteral("Fit"));
    fit->setToolTip(QStringLiteral("Fit to window (F)"));
    fit->setShortcut(QKeySequence(Qt::Key_F));
    addAction(fit);
    connect(fit, &QAction::triggered, m_view, &ImageView::fitToWindow);
    QAction *actual = m_toolbar->addAction(QStringLiteral("100%"));
    actual->setShortcut(QKeySequence(Qt::Key_1));
    actual->setToolTip(QStringLiteral("Actual size (1)"));
    addAction(actual);
    connect(actual, &QAction::triggered, m_view, &ImageView::actualSize);
    m_fullscreenAction = m_toolbar->addAction(style()->standardIcon(QStyle::SP_TitleBarMaxButton), QStringLiteral("Fullscreen"));
    m_fullscreenAction->setObjectName(QStringLiteral("fullscreenAction"));
    m_fullscreenAction->setShortcut(QKeySequence(Qt::Key_F11));
    addAction(m_fullscreenAction);
    connect(m_fullscreenAction, &QAction::triggered, this, &MainWindow::toggleFullscreen);

    QAction *escape = new QAction(this);
    escape->setShortcut(QKeySequence(Qt::Key_Escape));
    addAction(escape);
    connect(escape, &QAction::triggered, this, &MainWindow::escapePressed);

    setWindowTitle(QStringLiteral("Vistara"));
    resize(1100, 800);
    if (auto screen = QGuiApplication::primaryScreen()) {
        const QRect bounds = screen->availableGeometry();
        move(bounds.center() - rect().center());
    }
    statusBar()->showMessage(QStringLiteral("Ready"));
    QTimer::singleShot(0, this, [this] { m_view->setFocus(); });
}

void MainWindow::openDialog()
{
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Open image"), QString(),
        QStringLiteral("Images (*.png *.jpg *.jpeg *.bmp *.gif *.webp *.tif *.tiff)"));
    if (!path.isEmpty()) openFile(path);
}

QStringList MainWindow::buildImageList(const QString &currentFilePath) const
{
    const QDir dir(QFileInfo(currentFilePath).absolutePath());
    const QStringList filters = {"*.jpg", "*.jpeg", "*.png", "*.bmp", "*.gif", "*.webp", "*.tif", "*.tiff"};
    const QFileInfoList infos = dir.entryInfoList(filters, QDir::Files | QDir::Readable,
                                                  QDir::Name | QDir::IgnoreCase);
    QStringList files;
    files.reserve(infos.size());
    for (const QFileInfo &f : infos) files.append(f.absoluteFilePath());
    return files;
}

void MainWindow::openFile(const QString &path)
{
    const QString abs = QFileInfo(path).absoluteFilePath();
    if (!QFileInfo(abs).isFile()) return;
    m_files = buildImageList(abs);
    m_index = m_files.indexOf(abs);
    if (m_index < 0) {
        // Unusual extension or a case-sensitive filter mismatch: still open requested image.
        m_files.prepend(abs);
        m_index = 0;
    }
    loadAtIndex(m_index);
}

void MainWindow::loadAtIndex(int index)
{
    if (m_files.isEmpty()) return;
    m_index = (index % m_files.size() + m_files.size()) % m_files.size();
    const QString path = m_files.at(m_index);
    const quint64 request = ++m_requestId;
    m_loading = true;
    updateStatus();

    // Decode in worker; QPixmap and all widgets are ONLY touched on the UI thread.
    auto *watcher = new QFutureWatcher<QImage>(this);
    connect(watcher, &QFutureWatcher<QImage>::finished, this, [this, watcher, request, path] {
        const QImage image = watcher->result();
        watcher->deleteLater();
        if (request != m_requestId) return; // Ignore stale responses when navigating quickly.
        m_loading = false;
        if (image.isNull()) {
            statusBar()->showMessage(QStringLiteral("Could not decode: %1").arg(QFileInfo(path).fileName()));
            return;
        }
        m_imgSize = image.size();
        m_view->setImage(QPixmap::fromImage(image));
        setWindowTitle(QStringLiteral("Vistara - %1").arg(QFileInfo(path).fileName()));
        updateStatus();
    });
    watcher->setFuture(QtConcurrent::run(&m_loaderPool, [path] {
        QImageReader reader(path);
        reader.setAutoTransform(true);
        return reader.read();
    }));
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
    m_view->setFocus();
}

void MainWindow::escapePressed()
{
    if (isFullScreen()) toggleFullscreen();
    else close();
}

void MainWindow::updateStatus()
{
    const QString name = (m_index >= 0 && m_index < m_files.size())
        ? QFileInfo(m_files.at(m_index)).fileName() : QString();
    const QString dims = m_imgSize.isValid()
        ? QStringLiteral("%1 × %2").arg(m_imgSize.width()).arg(m_imgSize.height()) : QString();
    const QString progress = m_files.isEmpty() ? QString() :
        QStringLiteral("%1 / %2").arg(m_index + 1).arg(m_files.size());
    if (m_loading) {
        statusBar()->showMessage(QStringLiteral("Opening %1 …  |  %2").arg(name, progress));
    } else {
        statusBar()->showMessage(QStringLiteral("%1  |  %2  |  %3%  |  %4  |  %5")
            .arg(name, dims).arg(m_zoomPct).arg(progress, m_view->gpuEnabled() ? QStringLiteral("OpenGL") : QStringLiteral("CPU")));
    }
}
