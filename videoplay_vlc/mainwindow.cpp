#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <vlc/libvlc.h>
#include <QDebug>
#include <QPushButton>
#include <QFileDialog>
#include <QStandardPaths>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow) ,volume_(50)
{
    ui->setupUi(this);
    timer_ = new QTimer(this);
    connect(ui->playBtn, &QPushButton::clicked, this, &MainWindow::playMedia);
    connect(ui->pauseBtn, &QPushButton::clicked, this, &MainWindow::pauseMedia);
    connect(ui->stopBtn, &QPushButton::clicked, this, &MainWindow::stopMedia);
    connect(ui->fileBtn, &QPushButton::clicked, this, &MainWindow::browserFile);
    // 进度条
    connect(timer_, &QTimer::timeout, this, &MainWindow::updateSlider);
    connect(ui->posSlider, &QSlider::sliderReleased, this, &MainWindow::onProgessReleased);
    connect(ui->posSlider, &QSlider::sliderMoved, this, &MainWindow::onProgessMoved);
    // 音量条
    ui->volumeSlider->setMaximumHeight(100);
    connect(ui->volumeSlider, &QSlider::sliderReleased, this, &MainWindow::onVolumeReleased);
    connect(ui->volumeSlider, &QSlider::sliderMoved, this, &MainWindow::onVolumeMoved);
}

void MainWindow::loadMedia() {
    // http://vjs.zencdn.net/v/oceans.mp4
    // 可用来测试网络链接
    QString url = ui->locationEdit->text().trimmed();
    if (url.isEmpty()) {
        return;
    }
    // 如果当前正在播放(或者已经加载了)上一个视频，先执行停止
    if (isLoad) {
        stopMedia();
    }
    vlc_player.loadMedia(url);
    isLoad = true;
    resetUI();
    // playMedia();
}
void MainWindow::playMedia() {
    loadMedia();
    if (!isLoad) return;
    vlc_player.play((void*)ui->playWidget->winId());
    timer_->start(500);
    ui->posSlider->setEnabled(true);
}
void MainWindow::pauseMedia() {
    vlc_player.pause();
}
void MainWindow::stopMedia() {
    // 可通过销毁视频播放窗口来避免vlcstop时的异常卡死
    if (timer_->isActive()) {
        timer_->stop();
    }
    if (isLoad) {
        vlc_player.stop();
        // 刷新窗口背景，去除残留画面
        ui->playWidget->update();
        // 标志位复位
        isLoad = false;
    }
    resetUI();
}
void MainWindow::browserFile() {
    QString defaultPath = "D:\\video\\test";
    if (defaultPath.isEmpty()) {
        defaultPath = QDir::homePath();
    }
    // 只显示视频文件
    QString filter = tr("Video Files (*.mp4 *.mkv *.avi *.mov *.flv *.wmv);;All Files (*.*)");

    // 打开对话框并获取文件路径
    QString filePath = QFileDialog::getOpenFileName(this, tr("选择视频文件"), defaultPath, filter);
    if (filePath.isEmpty()) {
        return;
    }
    // 转换为系统原生路径分隔符
    filePath = QDir::toNativeSeparators(filePath);
    ui->locationEdit->setText(filePath);
}
void MainWindow::updateSlider() {
    if (ui->posSlider->isSliderDown()) {
        return;
    }

    float position = vlc_player.getPosition();

    int sliderMax = ui->posSlider->maximum();
    int sliderValue = static_cast<int>(position * sliderMax);
    ui->posSlider->setValue(sliderValue);

    curTime_ = vlc_player.getCurTime();
    totalTime_ = vlc_player.getTotalTime();
    ui->timeLb->setText(QString("%1 / %2").arg(formatTime(curTime_),formatTime(totalTime_)));
}

// 重置 UI 状态
void MainWindow::resetUI() {
    // 重置时间变量
    totalTime_ = 0;
    curTime_ = 0;

    // 重置进度条
    ui->posSlider->setValue(0);
    ui->posSlider->setEnabled(false); // 加载完成前禁用进度条，防止乱拖
    ui->timeLb->setText("--:-- / --:--");
    ui->volumeSlider->setValue(volume_);
}

void MainWindow::onProgessReleased() {
    int value = ui->posSlider->value();
    int max = ui->posSlider->maximum();

    float pos = static_cast<float>(value) / static_cast<float>(max);
    vlc_player.setPosition(pos);
}

void MainWindow::onProgessMoved(int value) {
    int max = ui->posSlider->maximum();

    float pos = static_cast<float>(value) / static_cast<float>(max);
    vlc_player.setPosition(pos);
}

void MainWindow::onVolumeReleased() {
    int value = ui->volumeSlider->value();
    vlc_player.setVolume(value);
    volume_ = value;
    ui->volumeLb->setText(QString("%1%").arg(volume_));
}

void MainWindow::onVolumeMoved(int value) {
    vlc_player.setVolume(value);
    volume_ = value;
    ui->volumeLb->setText(QString("%1%").arg(volume_));
}

QString MainWindow::formatTime(int64_t ms) {
    int seconds = (ms / 1000) % 60;
    int minutes = (ms / 60000) % 60;
    int hours   = (ms / 3600000);

    QTime time(hours, minutes, seconds);
    if (hours > 0) {
        return time.toString("HH:mm:ss");
    }
    return time.toString("mm:ss");
}
MainWindow::~MainWindow()
{
    stopMedia();
    delete ui;
}
