#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "structures.h"
#include "helpers.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow) {
    ui->setupUi(this);
    ui->labelMode->setVisible(false);

    connect(ui->joinButton, &QPushButton::clicked, this, &MainWindow::joinBtnHit);
    connect(ui->btnRefreshAvailSongs, &QPushButton::clicked, this, &MainWindow::refreshAvailSongs);
    connect(ui->btnAddNewSong, &QPushButton::clicked, this, &MainWindow::addNewSong);
    connect(ui->btnAddSongToQueue, &QPushButton::clicked, this, &MainWindow::addSongToQueue);
    connect(ui->btnStartQueue, &QPushButton::clicked, this, &MainWindow::startQueue);
    connect(ui->btnStart, &QPushButton::clicked, this, &MainWindow::playMusic);
    connect(ui->btnStop, &QPushButton::clicked, this, &MainWindow::stopMusic);
    connect(ui->btnSkip, &QPushButton::clicked, this, &MainWindow::skipMusic);
    connect(ui->btnSwap, &QPushButton::clicked, this, &MainWindow::swapSongs);
    connect(ui->inputFirstSwap, SIGNAL(valueChanged(int)), this, SLOT(inputFirstSwapChanged(int)));
    connect(ui->inputSecondSwap, SIGNAL(valueChanged(int)), this, SLOT(inputSecondSwapChanged(int)));

}

MainWindow::~MainWindow() {
    isPlaying = false;
    engine->drop();
    if(sock)
        sock->close();
    delete ui;
}

void MainWindow::receivePacket() {

    QByteArray response;
    Packet packet;
    while(sock->waitForReadyRead(50) || sock->bytesAvailable() > 0){

        response.append(sock->readAll());

        while(read_byte_array(response, packet)) {
            if(packet.type == P_AVAILABLE_SONG) {
                std::cout<< "P_AVAILABLE_SONG" << std::endl;
                ui->listAvailSongs->clear();
                songsAvailMem.clear();
                songsAvail.push_back(QString(packet.data));
                std::cout << packet.data << std::endl;
            }
            if(packet.type == P_AVAILABLE_SONGS_END) {
                std::cout << "P_AVAILABLE_SONGS_END" << std::endl;
                for(int i=0;i<songsAvail.size();i++) {
                    ui->listAvailSongs->append(songsAvail[i].toUtf8());
                    songsAvailMem.push_back(songsAvail[i]);
                }
                songsAvail.clear();
                ui->btnAddSongToQueue->setEnabled(true);
            }
            if(packet.type == P_SONG_IN_QUEUE) {
                std::cout << "P_SONG_IN_QUEUE " << packet.data << std::endl;
                ui->listSongsQueue->clear();
                songsQueue.push_back(QString(packet.data));
            }
            if(packet.type == P_SONGS_IN_QUEUE_END) {
                std::cout << "P_SONGS_IN_QUEUE_END" << std::endl;
                for(int i=0;i<songsQueue.size();i++)
                    ui->listSongsQueue->append(songsQueue[i].toUtf8());
                if(songsQueue.size() > 1) {
                    ui->inputFirstSwap->setEnabled(true);
                    ui->inputSecondSwap->setEnabled(true);
                    ui->inputSecondSwap->setMaximum(songsQueue.size());
                    ui->btnSwap->setEnabled(true);
                } else {
                    ui->inputFirstSwap->setEnabled(false);
                    ui->inputSecondSwap->setEnabled(false);
                    ui->btnSwap->setEnabled(false);
                }
                songsQueue.clear();
            }
            if(packet.type == P_EMPTY_QUEUE) {
                std::cout << "P_EMPTY_QUEUE" << std::endl;
                ui->listSongsQueue->clear();
                ui->btnSkip->setEnabled(false);
                ui->btnStart->setEnabled(false);
                ui->btnStop->setEnabled(false);
                ui->inputFirstSwap->setEnabled(false);
                ui->inputSecondSwap->setEnabled(false);
                ui->btnSwap->setEnabled(false);
                ui->btnStartQueue->setEnabled(true);
                songsQueue.clear();
            }
            if(packet.type == P_SONG_BEGIN) {
                mode = 1;
                ui->labelMode->setVisible(false);
                ui->btnStart->setEnabled(false);
                ui->btnStop->setEnabled(false);
                std::cout << "P_SONG_BEGIN" << std::endl;
                std::ofstream file("temp.wav", std::ios::binary);
                file.close();
                if(isPlaying) {
                    mutex.lock();
                    isPlaying = false;
                    sound->stop();
                    mutex.unlock();
                }
            }
            if(packet.type == P_SONG) {
                std::ofstream file("temp.wav", std::ios::out | std::ios::binary | std::ios::app);
                file.write(packet.data, packet.size);
                file.close();
            }
            if(packet.type == P_SONG_END) {
                std::cout << "P_SONG_END" << std::endl;
                mutex.lock();
                sound = engine->play2D("temp.wav", false, false, true, irrklang::ESM_AUTO_DETECT, false);
                sound->setPlayPosition(atoi(packet.data));
                mutex.unlock();
                musicPosition = 0.0;
                isPlaying = true;
                ui->btnStop->setEnabled(true);
                ui->btnStartQueue->setEnabled(false);
                ui->btnSkip->setEnabled(true);

                // start a thread to constantly check playPosition
                QThread* thread = new QThread;
                SoundEndChecker* worker = new SoundEndChecker(sound, &isPlaying, &mutex);
                worker->moveToThread(thread);
                connect(thread, SIGNAL(started()), worker, SLOT(check()));
                connect(worker, SIGNAL(endOfSong()), this, SLOT(endMusicDetected()));
                connect(worker, SIGNAL(finished()), thread, SLOT(quit()));
                connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
                connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
                thread->start();
            }
            if(packet.type == P_PAUSE && mode) {
                std::cout << "P_PAUSE" << std::endl;
                mutex.lock();
                musicPosition = sound->getPlayPosition();
                isPlaying = false;
                sound->stop();
                mutex.unlock();
                ui->btnStart->setEnabled(true);
                ui->btnStop->setEnabled(false);
            }
            if(packet.type == P_START && mode) {
                std::cout << "P_START" << std::endl;
                mutex.lock();
                sound = engine->play2D("temp.wav", false, false, true, irrklang::ESM_AUTO_DETECT, false);
                sound->setPlayPosition(musicPosition);
                isPlaying = true;
                mutex.unlock();
                ui->btnStart->setEnabled(false);
                ui->btnStop->setEnabled(true);

                QThread* thread = new QThread;
                SoundEndChecker* worker = new SoundEndChecker(sound, &isPlaying, &mutex);
                worker->moveToThread(thread);
                connect(thread, SIGNAL(started()), worker, SLOT(check()));
                connect(worker, SIGNAL(endOfSong()), this, SLOT(endMusicDetected()));
                connect(worker, SIGNAL(finished()), thread, SLOT(quit()));
                connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
                connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
                thread->start();
            }
            if(packet.type == P_UNLOCK_SQ_BTN) {
                ui->btnStartQueue->setEnabled(true);
            }
            if(packet.type == P_CLIENT_MODE) {
                std::string str(packet.data);
                if(str == "WAIT") {
                    mode = 0;
                    ui->btnStart->setEnabled(false);
                    ui->btnStop->setEnabled(false);
                    ui->btnStartQueue->setEnabled(false);
                    ui->labelMode->setVisible(true);
                } else {
                    mode = 1;
                    ui->labelMode->setVisible(false);
                }
            }
            response.remove(0, 1024);
        }
    }
}

void MainWindow::joinBtnHit(){
    if(sock)
        delete sock;
    sock = new QTcpSocket(this);

    connect(sock, &QTcpSocket::connected, this, &MainWindow::socketConnected);
    connect(sock, &QTcpSocket::readyRead, this, &MainWindow::receivePacket);
    sock->connectToHost(ui->hostInput->text(), ui->portInput->value());

    engine = irrklang::createIrrKlangDevice();

}

void MainWindow::socketConnected() {
    ui->joinButton->setEnabled(false);
    ui->hostInput->setEnabled(false);
    ui->portInput->setEnabled(false);
    ui->btnRefreshAvailSongs->setEnabled(true);
    ui->btnAddNewSong->setEnabled(true);
    ui->btnStartQueue->setEnabled(true);
}

void MainWindow::refreshAvailSongs() {

    ui->listAvailSongs->clear();

    Packet packet;
    packet.type = P_AVAILABLE_SONGS_ASK;
    packet.size = 0;
    send_packet_using_qt_socket(*sock, packet);
}

void MainWindow::addNewSong() {
    QString filter = "wav file (*.wav)";
    QString fileName = QFileDialog::getOpenFileName(this, "Choose file to upload", QDir::homePath(), filter);
    std::ifstream file(fileName.toStdString(), std::ios::binary);
    if(!file.is_open()) {
        std::cerr << "Error opening file" << std::endl;
        return;
    }

    QFileInfo fileInfo(fileName);
    std::string songToUpload = fileInfo.fileName().toStdString();

    std::cout << songToUpload << std::endl;

    // read .wav file and send it to the server
    Packet packet;
    packet.type = P_UPLOAD_SONG_BEGIN;
    packet.data = const_cast<char*>(songToUpload.c_str());
    packet.size = sizeof(songToUpload);
    bool sent = send_packet_using_qt_socket(*sock, packet);
    if(!sent){
        file.close();
        return;
    }

    while(true) {
        packet.type = P_UPLOAD_SONG;
        packet.size = PACKET_DATA_MAX_SIZE;
        packet.data = new char[PACKET_DATA_MAX_SIZE];
        file.read(packet.data, PACKET_DATA_MAX_SIZE);
        int bytesRead = file.gcount();
        if(bytesRead == 0) {
            std::cout << "Whole file read" << std::endl;
            delete_packet(packet);
            break;
        }
        if(!send_packet_using_qt_socket(*sock, packet)) {
            std::cout << "File could not be sent" << std::endl;
            delete_packet(packet);
            break;
        }
        delete_packet(packet);
    }

    file.close();

    packet.type = P_UPLOAD_SONG_END;
    packet.size = 0;
    send_packet_using_qt_socket(*sock, packet);
}

void MainWindow::addSongToQueue() {
    QDialog* dialog = new QDialog();
    QListWidget* listWidget = new QListWidget();
    QPushButton* okButton = new QPushButton("OK");

    for(auto& qstr : songsAvailMem) {
        listWidget->addItem(qstr);
    }



    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(listWidget);
    layout->addWidget(okButton);
    dialog->setLayout(layout);

    QObject::connect(okButton, &QPushButton::clicked, this,
        [listWidget,dialog, this](){
            QListWidgetItem* selectedItem = listWidget->currentItem();
            if (selectedItem) {
                // send
                std::string songToUpload = selectedItem->text().toStdString();
                Packet packet;
                packet.type = P_ADD_SONG_TO_QUEUE;
                packet.size = songToUpload.length();
                packet.data = const_cast<char*>(songToUpload.c_str());
                packet.size = sizeof(songToUpload);
                std::cout << packet.data << std::endl;
                bool sent = send_packet_using_qt_socket(*sock, packet);
                dialog->accept();
            }
        });

    dialog->exec();
}

void MainWindow::startQueue() {

    Packet packet;
    packet.type = P_START_QUEUE;
    packet.size = 0;
    send_packet_using_qt_socket(*sock, packet);

}

void MainWindow::stopMusic() {
    std::cout << "wants to stop music" << std::endl;
    Packet packet;
    packet.type = P_WANTS_TO_PAUSE;
    packet.size = 0;
    send_packet_using_qt_socket(*sock, packet);
}

void MainWindow::playMusic() {
    std::cout << "wants to play music" << std::endl;
    Packet packet;
    packet.type = P_WANTS_TO_START;
    packet.size = 0;
    send_packet_using_qt_socket(*sock, packet);
}

void MainWindow::endMusicDetected() {
    std::cout << "end of song, send request for the next one" << std::endl;
    Packet packet;
    packet.type = P_NEXT_SONG;
    packet.size = 0;
    send_packet_using_qt_socket(*sock, packet);
}

void MainWindow::skipMusic() {
    std::cout << "wants to skip music" << std::endl;
    mutex.lock();
    isPlaying = false;
    sound->stop();
    mutex.unlock();
    Packet packet;
    packet.type = P_SKIP;
    packet.size = 0;
    send_packet_using_qt_socket(*sock, packet);
}

void MainWindow::inputFirstSwapChanged(int value) {
    if(value >= ui->inputSecondSwap->value()) {
        if(ui->inputSecondSwap->value() < ui->inputSecondSwap->maximum())
            ui->inputSecondSwap->setValue(value + 1);
        else
            ui->inputFirstSwap->setValue(value - 1);
    }
}

void MainWindow::inputSecondSwapChanged(int value) {
    if(value <= ui->inputFirstSwap->value()){
        ui->inputFirstSwap->setValue(value - 1);
    }
}

void MainWindow::swapSongs() {
    int f = ui->inputFirstSwap->value();
    int s = ui->inputSecondSwap->value();
    std::ostringstream str;
    str << f << "#" << s;
    Packet packet;
    packet.type = P_SWAP;
    packet.data = const_cast<char*>(str.str().c_str());
    packet.size = sizeof(packet.data);
    std::cout << "swap songs" << std::endl;
    send_packet_using_qt_socket(*sock, packet);

}
