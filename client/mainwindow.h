#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <iostream>
#include <QStandardItem>
#include <QFileDialog>
#include <QVBoxLayout>
#include <QString>
#include <QRadioButton>
#include <QRadioButton>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QMutex>
#include <fstream>
#include <vector>
#include <mutex>
#include <atomic>
#include <string>
#include <sstream>
#include <thread>
#include <irrKlang.h>
#include <cstdlib>
#include "constants.h"
#include "structures.h"
#include "soundendchecker.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    std::vector<QString> songsAvail;
    std::vector<QString> songsAvailMem;
    std::vector<QString> songsQueue;

    QMutex mutex;
    std::atomic<bool> isPlaying{false};
    bool shouldPlay;
    int mode;

    irrklang::ISoundEngine* engine;
    irrklang::ISound* sound;
    irrklang::ik_u32 musicPosition = 0.0;

    void receivePacket();
    void put_packet_on_queue(Packet &packet);

protected:
    QTcpSocket * sock {nullptr};
    void joinBtnHit();
    void socketConnected();
    void refreshAvailSongs();
    void addNewSong();
    void addSongToQueue();
    void startQueue();
    void playMusic();
    void stopMusic();
    void skipMusic();
    void swapSongs();

private slots:
    void endMusicDetected();
    void inputFirstSwapChanged(int value);
    void inputSecondSwapChanged(int value);

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
