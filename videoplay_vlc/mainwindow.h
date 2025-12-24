#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "vlcplayer.h"
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void loadMedia();
    void playMedia();
    void pauseMedia();
    void stopMedia();
    void browserFile();
    void updateSlider();
    void resetUI();
    void onProgessReleased();
    void onProgessMoved(int value);
    void onVolumeReleased();
    void onVolumeMoved(int value);

private:
    Ui::MainWindow *ui;
    VlcPlayer vlc_player;
    QTimer *timer_;
    int64_t totalTime_ = 0, curTime_;
    int volume_;
    bool isLoad = false;
    QString formatTime(int64_t ms);
};
#endif // MAINWINDOW_H
