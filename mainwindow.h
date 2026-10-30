#pragma once
#include <QMainWindow>
#include <QStringList>
#include <QSize>
#include <QThreadPool>
#include <QtGlobal>

class ImageView;
class QToolBar;
class QAction;
class QTimer;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    void openFile(const QString &path);
private slots:
    void openDialog();
    void nextImage();
    void prevImage();
    void toggleFullscreen();
    void escapePressed();
    void saveRotation();
private:
    QStringList buildImageList(const QString &currentFilePath) const;
    void loadAtIndex(int index);
    void updateStatus();
    ImageView *m_view = nullptr;
    QToolBar *m_toolbar = nullptr;
    QTimer *m_fullscreenToolbarTimer = nullptr;
    QAction *m_fullscreenAction = nullptr;
    QStringList m_files;
    int m_index = -1;
    int m_zoomPct = 100;
    QSize m_imgSize;
    QThreadPool m_loaderPool;
    quint64 m_requestId = 0;
    bool m_loading = false;
    bool m_saving = false;
    QAction *m_saveRotationAction = nullptr;
};
