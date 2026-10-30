#include "mainwindow.h"
#include "imageview.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QImageReader>
#include <QImage>
#include <QToolBar>
#include <QIcon>
#include <QAction>
#include <QStatusBar>
#include <QPixmap>
#include <QScreen>
#include <QGuiApplication>
#include <QTimer>
#include <QCursor>
#include <QStyle>
#include <QFutureWatcher>
#include <QtConcurrent>
#include <QKeySequence>
#include <QImageWriter>
#include <QSaveFile>
#include <QMessageBox>
#include <QDir>
#include <QTransform>
#include <QFile>

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
    m_toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_toolbar->setIconSize(QSize(22, 22));

    // Poll the cursor only during fullscreen. This also works over QOpenGLWidget,
    // whose viewport can replace widgets and swallow normal mouse-move events.
    m_fullscreenToolbarTimer = new QTimer(this);
    m_fullscreenToolbarTimer->setInterval(120);
    connect(m_fullscreenToolbarTimer, &QTimer::timeout, this, [this] {
        if (!isFullScreen()) return;
        const QPoint cursor = mapFromGlobal(QCursor::pos());
        const bool inside = rect().contains(cursor);
        if (!inside) {
            if (m_toolbar->isVisible()) m_toolbar->hide();
            return;
        }
        // Enter the top 8px to reveal the toolbar, including over the GPU view.
        if (!m_toolbar->isVisible() && cursor.y() <= 8) {
            m_toolbar->show();
        } else if (m_toolbar->isVisible() && cursor.y() > m_toolbar->height() + 12) {
            m_toolbar->hide();
        }
    });

    QAction *open = m_toolbar->addAction(style()->standardIcon(QStyle::SP_DialogOpenButton), QStringLiteral("Open"));
    open->setToolTip(QStringLiteral("Open image"));
    connect(open, &QAction::triggered, this, &MainWindow::openDialog);
    QAction *prev = m_toolbar->addAction(style()->standardIcon(QStyle::SP_ArrowBack), QStringLiteral("Previous"));
    prev->setToolTip(QStringLiteral("Previous image"));
    connect(prev, &QAction::triggered, this, &MainWindow::prevImage);
    QAction *next = m_toolbar->addAction(style()->standardIcon(QStyle::SP_ArrowForward), QStringLiteral("Next"));
    next->setToolTip(QStringLiteral("Next image"));
    connect(next, &QAction::triggered, this, &MainWindow::nextImage);
    m_toolbar->addSeparator();
    QAction *rotateLeft = m_toolbar->addAction(QIcon(QStringLiteral(":/icons/rotate-left.png")), QStringLiteral("Rotate Left"));
    rotateLeft->setToolTip(QStringLiteral("Rotate 90° counterclockwise (Ctrl+L)"));
    rotateLeft->setShortcut(QKeySequence(QStringLiteral("Ctrl+L")));
    addAction(rotateLeft);
    connect(rotateLeft, &QAction::triggered, m_view, &ImageView::rotateLeft);
    QAction *rotateRight = m_toolbar->addAction(QIcon(QStringLiteral(":/icons/rotate-right.png")), QStringLiteral("Rotate Right"));
    rotateRight->setToolTip(QStringLiteral("Rotate 90° clockwise (Ctrl+R)"));
    rotateRight->setShortcut(QKeySequence(QStringLiteral("Ctrl+R")));
    addAction(rotateRight);
    connect(rotateRight, &QAction::triggered, m_view, &ImageView::rotateRight);
    m_saveRotationAction = m_toolbar->addAction(QIcon(QStringLiteral(":/icons/save.png")), QStringLiteral("Save Rotation…"));
    m_saveRotationAction->setToolTip(QStringLiteral("Save a rotated copy of this image (Ctrl+Shift+S)"));
    m_saveRotationAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+S")));
    addAction(m_saveRotationAction);
    connect(m_saveRotationAction, &QAction::triggered, this, &MainWindow::saveRotation);
    m_saveRotationAction->setEnabled(false);
    m_toolbar->addSeparator();
    QAction *fit = m_toolbar->addAction(QIcon(QStringLiteral(":/icons/fit.png")), QStringLiteral("Fit"));
    fit->setToolTip(QStringLiteral("Fit to window (F)"));
    fit->setShortcut(QKeySequence(Qt::Key_F));
    addAction(fit);
    connect(fit, &QAction::triggered, m_view, &ImageView::fitToWindow);
    QAction *actual = m_toolbar->addAction(QIcon(QStringLiteral(":/icons/actual-size.png")), QStringLiteral("100%"));
    actual->setShortcut(QKeySequence(Qt::Key_1));
    actual->setToolTip(QStringLiteral("Actual size (1)"));
    addAction(actual);
    connect(actual, &QAction::triggered, m_view, &ImageView::actualSize);
    m_fullscreenAction = m_toolbar->addAction(QIcon(QStringLiteral(":/icons/fullscreen.png")), QStringLiteral("Fullscreen"));
    m_fullscreenAction->setObjectName(QStringLiteral("fullscreenAction"));
    m_fullscreenAction->setToolTip(QStringLiteral("Toggle fullscreen (F11)"));
    m_fullscreenAction->setShortcut(QKeySequence(Qt::Key_F11));
    addAction(m_fullscreenAction);
    connect(m_fullscreenAction, &QAction::triggered, this, &MainWindow::toggleFullscreen);

    QAction *escape = new QAction(this);
    escape->setShortcut(QKeySequence(Qt::Key_Escape));
    addAction(escape);
    connect(escape, &QAction::triggered, this, &MainWindow::escapePressed);

    setWindowIcon(QIcon(QStringLiteral(":/icons/vistara_icon_256.png")));
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
    m_saveRotationAction->setEnabled(false);
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
        m_saveRotationAction->setEnabled(true);
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
        m_fullscreenToolbarTimer->stop();
        showNormal();
        m_toolbar->show();
        statusBar()->show();
    } else {
        m_toolbar->hide();
        statusBar()->hide();
        showFullScreen();
        m_fullscreenToolbarTimer->start();
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

void MainWindow::saveRotation()
{
    if (m_loading || m_saving || m_index < 0 || m_index >= m_files.size()) return;
    const int degrees = m_view->rotationDegrees();
    if (degrees == 0) {
        QMessageBox::information(this, QStringLiteral("No rotation"),
            QStringLiteral("Use Rotate Left or Rotate Right first."));
        return;
    }
    const QString sourcePath = m_files.at(m_index);
    const QFileInfo info(sourcePath);
    const QString suggested = info.dir().filePath(info.completeBaseName()
        + QStringLiteral("_rotated.") + info.suffix());
    const QString dest = QFileDialog::getSaveFileName(this,
        QStringLiteral("Save rotated copy"), suggested,
        QStringLiteral("Image files (*.%1);;All files (*)").arg(info.suffix()));
    if (dest.isEmpty()) return;
    if (QFileInfo(dest).absoluteFilePath() == info.absoluteFilePath()) {
        QMessageBox::warning(this, QStringLiteral("Original protected"),
            QStringLiteral("Choose a different filename. Vistara preserves the original image."));
        return;
    }
    if (QFile::exists(dest)) {
        const auto result = QMessageBox::question(this, QStringLiteral("Replace file?"),
            QStringLiteral("Replace the existing file?\n%1").arg(dest),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (result != QMessageBox::Yes) return;
    }
    // Load and encode in a worker so the viewer remains responsive.
    // Auto-transform matches the EXIF orientation displayed by the viewer.
    m_saving = true;
    m_saveRotationAction->setEnabled(false);
    statusBar()->showMessage(QStringLiteral("Saving rotated copy…"));
    auto *watcher = new QFutureWatcher<QString>(this);
    connect(watcher, &QFutureWatcher<QString>::finished, this, [this, watcher, dest] {
        const QString error = watcher->result();
        watcher->deleteLater();
        m_saving = false;
        m_saveRotationAction->setEnabled(!m_loading && m_index >= 0);
        if (!error.isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("Could not save image"), error);
        } else {
            statusBar()->showMessage(QStringLiteral("Saved: %1").arg(dest), 6000);
        }
    });
    watcher->setFuture(QtConcurrent::run(&m_loaderPool, [sourcePath, dest, degrees]() -> QString {
        QImageReader reader(sourcePath);
        reader.setAutoTransform(true);
        QImage img = reader.read();
        if (img.isNull()) return QStringLiteral("Image read failed: %1").arg(reader.errorString());
        QTransform transform;
        transform.rotate(degrees);
        img = img.transformed(transform, Qt::SmoothTransformation);
        QSaveFile output(dest);
        if (!output.open(QIODevice::WriteOnly)) return output.errorString();
        const QByteArray fmt = QFileInfo(dest).suffix().toLatin1().toLower();
        QImageWriter writer(&output, fmt);
        if (fmt == "jpg" || fmt == "jpeg") writer.setQuality(95);
        if (!writer.write(img)) {
            output.cancelWriting();
            return QStringLiteral("Image write failed: %1").arg(writer.errorString());
        }
        if (!output.commit()) return output.errorString();
        return {};
    }));
}
