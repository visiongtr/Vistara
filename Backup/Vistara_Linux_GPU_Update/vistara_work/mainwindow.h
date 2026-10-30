#pragma once

#include <QMainWindow>
#include <QStringList>
#include <QSize>
#include <QThreadPool>

class ImageView;
class QToolBar;
class QAction;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;
    void openFile(const QString &path);

private slots:
    void openDialog();
    void nextImage();
    void prevImage();
    void toggleFullscreen();
    void handleEscape();

private:
    QStringList buildImageList(const QString &currentFilePath) const;
    void loadAtIndex(int index);
    void updateStatus();
    void updateFullscreenAction();

    ImageView *m_view = nullptr;
    QToolBar *m_toolbar = nullptr;
    QAction *m_fullscreenAction = nullptr;
    QStringList m_files;
    int m_index = -1;
    int m_zoomPct = 100;
    int m_requestId = 0;
    QSize m_imgSize;
    QThreadPool m_loaderPool;
};
