#include "soundendchecker.h"

SoundEndChecker::SoundEndChecker(irrklang::ISound* _sound, std::atomic<bool>* _isPlaying, QMutex* _mutex)
{
    sound = _sound;
    isPlaying = _isPlaying;
    mutex = _mutex;
}

SoundEndChecker::~SoundEndChecker () {}

void SoundEndChecker::check() {
    while(true) {
        mutex->lock();
        irrklang::ik_u32 x = sound->getPlayPosition();
        irrklang::ik_u32 y = sound->getPlayLength();
        if(x >= y){
            qDebug() << x << " >= " << y ;
            mutex->unlock();
            break;
        }
        mutex->unlock();
        QThread::sleep(0.3);
    }
    if(*isPlaying){
       qDebug() << "end of song (thread)";
       emit endOfSong();
    }
    qDebug() << "ending thread";
    emit finished();
}
