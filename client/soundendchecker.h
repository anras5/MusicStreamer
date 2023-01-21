#ifndef SOUNDENDCHCKER_H
#define SOUNDENDCHCKER_H

#include <QObject>
#include <QDebug>
#include <QThread>
#include <QMutex>
#include <irrKlang.h>

class SoundEndChecker : public QObject
{
    Q_OBJECT

public:
    SoundEndChecker(irrklang::ISound* _sound, std::atomic<bool>* _isPlaying, QMutex* _mutex);
    ~SoundEndChecker();

public slots:
    void check();

signals:
    void finished();
    void endOfSong();

private:
    QMutex* mutex;
    irrklang::ISound* sound;
    std::atomic<bool>* isPlaying;
};

#endif
