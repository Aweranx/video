#ifndef VLCPLAYER_H
#define VLCPLAYER_H
#include <vlc/vlc.h>
#include <QString>

class VlcPlayer
{
public:
    VlcPlayer();
    ~VlcPlayer();
    void initVlc();
    void loadMedia(const QString& path);
    // 操控
    void play(void *wid);
    void pause();
    void stop();
    // 具体信息
    float getPosition();
    void setPosition(float cur);
    int getVolume();
    void setVolume(int value);
    int64_t getTotalTime();
    int64_t getCurTime();
    void setTime(int64_t value);



public:
    libvlc_instance_t *vlc_ins_ = nullptr;
    libvlc_media_player_t *vlc_mediaPlayer_ = nullptr;
    libvlc_media_t *vlc_media_ = nullptr;
};

#endif // VLCPLAYER_H
