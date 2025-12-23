#include "vlcplayer.h"
#include <array>
#include <QMessageBox>
#include <QUrl>
#include <QDir>

VlcPlayer::VlcPlayer() {
    initVlc();
}

void VlcPlayer::initVlc() {
    std::array vlc_args = {
        "--ignore-config",       // 忽略默认配置
        "--no-osd",              // 禁用屏幕显示
        "--no-snapshot-preview"  // 禁用快照预览
        // 核心修改：禁用硬件解码
        // "--avcodec-hw=none",
    };
    vlc_ins_ = libvlc_new(static_cast<int>(vlc_args.size()), vlc_args.data());
    if (!vlc_ins_)
    {
        QString message = QString("Failed to initialize libVLC: %1").arg(QString(libvlc_errmsg()));
        QMessageBox::critical(NULL, "Error", message);
        qDebug() << message;
    }
}

void VlcPlayer::loadMedia(const QString &path)
{
    // 如果播放器存在，先停止，再释放
    if (vlc_mediaPlayer_) {
        libvlc_media_player_stop(vlc_mediaPlayer_);    // 停止播放
        libvlc_media_player_release(vlc_mediaPlayer_); // 释放内存
        vlc_mediaPlayer_ = nullptr;                    // 置空指针防止野指针
    }

    // 如果媒体对象存在，释放
    if (vlc_media_) {
        libvlc_media_release(vlc_media_);              // 释放内存
        vlc_media_ = nullptr;
    }
    // 判断流类型
    QUrl url(path);
    bool isNetwork = !url.scheme().isEmpty() && path.contains("://");

    // 网络流
    if (isNetwork) {
        vlc_media_ = libvlc_media_new_location(vlc_ins_, path.toUtf8().constData());
    }
    // 本地文件
    else {
        QString localPath = QDir::toNativeSeparators(path);
        vlc_media_ = libvlc_media_new_path(vlc_ins_, localPath.toUtf8().constData());
    }
    if(!vlc_media_) {
        QString message = QString("Failed to load media: %1").arg(QString(libvlc_errmsg()));
        QMessageBox::critical(NULL, "Error", message);
        qDebug() << message;
        return;
    }
    vlc_mediaPlayer_ = libvlc_media_player_new_from_media(vlc_media_);
    if(!vlc_mediaPlayer_) {
        QString message = QString("Failed to initialize media player: %1").arg(QString(libvlc_errmsg()));
        QMessageBox::critical(NULL, "Error", message);
        qDebug() << message;
        return;
    }
}
void VlcPlayer::play(void *wid) {
    if(vlc_mediaPlayer_) {
        libvlc_media_player_play(vlc_mediaPlayer_);
        libvlc_media_player_set_hwnd(vlc_mediaPlayer_, (void*)wid);
    }
}
void VlcPlayer::pause() {
    if(vlc_mediaPlayer_ && libvlc_media_player_can_pause(vlc_mediaPlayer_)) {
        libvlc_media_player_set_pause(vlc_mediaPlayer_, 1);
    }
}
void VlcPlayer::stop() {
    // 停止之前如果处于暂停状态，会导致渲染线程（Vout）锁着当前帧
    // 解码器想要回收当前帧，陷入死锁，所以要恢复播放再关闭
    if (libvlc_media_player_get_state(vlc_mediaPlayer_) == libvlc_Paused) {
        libvlc_media_player_set_pause(vlc_mediaPlayer_, 0);
    }
    if (vlc_mediaPlayer_)
    {
        libvlc_media_player_set_hwnd(vlc_mediaPlayer_, nullptr);
        libvlc_media_player_stop(vlc_mediaPlayer_);
        libvlc_media_player_release(vlc_mediaPlayer_);
        vlc_mediaPlayer_ = nullptr;
        setPosition(0.0f);
    }
    if (vlc_media_)
    {
        libvlc_media_release(vlc_media_);
        vlc_media_ = nullptr;
    }
}
float VlcPlayer::getPosition() {
    if(vlc_mediaPlayer_) {
        return libvlc_media_player_get_position(vlc_mediaPlayer_);
    }
    return 0;
}
void VlcPlayer::setPosition(float cur) {
    if(vlc_mediaPlayer_) {
        libvlc_media_player_set_position(vlc_mediaPlayer_, cur);
    }
}
int VlcPlayer::getVolume() {
    if(vlc_mediaPlayer_) {
        return libvlc_audio_get_volume(vlc_mediaPlayer_);
    }
    return 0;
}
void VlcPlayer::setVolume(int value) {
    if(vlc_mediaPlayer_) {
        libvlc_audio_set_volume(vlc_mediaPlayer_, value);
    }
}
int64_t VlcPlayer::getTotalTime() {
    if(vlc_mediaPlayer_) {
        return libvlc_media_player_get_length(vlc_mediaPlayer_);
    }
    return 0;
}
int64_t VlcPlayer::getCurTime() {
    if(vlc_mediaPlayer_) {
        return libvlc_media_player_get_time(vlc_mediaPlayer_);
    }
    return 0;
}
void VlcPlayer::setTime(int64_t value) {
    if(vlc_mediaPlayer_) {
        libvlc_media_player_set_time(vlc_mediaPlayer_, value);
    }
}

VlcPlayer::~VlcPlayer() {
    libvlc_media_player_release(vlc_mediaPlayer_);
    libvlc_media_release(vlc_media_);
    libvlc_release(vlc_ins_);
}


