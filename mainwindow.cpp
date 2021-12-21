#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "typedef.h"
#include "dialoghwconfig.h"
#include "dialognwconfig.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <QTimer>
#include <QUuid>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressDialog>
#include <QMouseEvent>
#include <QCryptographicHash>

#define HOST_ADDRESS    "224.0.0.2"

void get_checksum(char *content, unsigned int content_len, short *checksum)
{
    uint i;
    if( (content == NULL) || (checksum == NULL))
        return;
    for( i=0, *checksum=0; i<content_len; i++)
    {
        *checksum += *(content+i);
    }
}

int verify_checksum(char *content, unsigned int content_len, short checksum)
{
    uint i;
    short sum;
    if( content == NULL)
        return -1;
    qDebug("verify_checksum(), content_len=0x%04x, checksum=0x%04x\n", content_len, checksum);
    for( i=0, sum=0; i<content_len; i++)
        sum += *(content+i);
    if( sum == checksum)
    {
        qDebug("verify_checksum() ok\n");
        return 1;
    }
    qDebug("checksum error, checksum=0x%04x, sum=0x%04x\n", checksum, sum);
    return -2;
}

int send_content(QTcpSocket *tcpSocket, head_type_e head, char* content, int len)
{
    Cmd_pkt_t send_pkt;
    qDebug("send_content(), len=%d\n", len);
    if((len > MAX_CONTENT_LEN))
        return -1;
    send_pkt.head = head;
    send_pkt.len = len;
    memcpy( send_pkt.content, content, len);
    get_checksum(send_pkt.content, send_pkt.len, &(send_pkt.checksum));
    return tcpSocket->write((char *)&send_pkt, 6 + send_pkt.len);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , tcpSocket (new QTcpSocket(this))
    , udpSocket (new QUdpSocket(this))
    , libvlc_inst (libvlc_new (0, NULL))
    , libvlc_mp (nullptr)
    , playerstate (STATE_STOP)
    , connectRow (-1)
    , currentRow (-1)
    , dialognwconfig (new DialogNwConfig(this))
    , dialoghwconfig (new DialogHwConfig(this))
{
    ui->setupUi(this);
    setWindowTitle("设备发现工具" + buildDateTime());
    qsrand(QTime::currentTime().msecsSinceStartOfDay());
    uuid = "neostra" + QByteArray::number(100 + qrand()%100);
    qDebug() << "uuid:" << uuid;
    ui->lineEditUrl->setReadOnly(true);
    ui->lineEditUrl->installEventFilter(this);

    udpSocket->bind(QHostAddress::AnyIPv4, 6868);
    udpSocket->joinMulticastGroup(QHostAddress(HOST_ADDRESS));
    udpSocket->setSocketOption(QAbstractSocket::MulticastTtlOption, 64);
    udpSocket->socketOption(QAbstractSocket::MulticastTtlOption);
    udpSocket->setSocketOption(QAbstractSocket::MulticastLoopbackOption, 0);
    connect(udpSocket, &QUdpSocket::readyRead, this, &MainWindow::onReadUdpDatagrams);

    connect(tcpSocket, &QTcpSocket::readyRead, this, &MainWindow::onReadTcpDatagrams);
    connect(tcpSocket, &QTcpSocket::connected, this, &MainWindow::onTcpConnected);
    connect(tcpSocket, &QTcpSocket::disconnected, this, &MainWindow::onTcpDisconnected);

    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->setShowGrid(false);
    ui->tableWidget->verticalHeader()->setVisible(false);
    ui->tableWidget->horizontalHeader()->setHighlightSections(false);
    ui->tableWidget->horizontalHeader()->setMinimumSectionSize(0);
    ui->tableWidget->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    ui->tableWidget->setFocusPolicy(Qt::NoFocus);
    ui->tableWidget->setColumnWidth(0, 120);
    ui->tableWidget->setColumnWidth(1, 100);
    ui->tableWidget->setColumnWidth(2, 120);
    ui->tableWidget->setColumnWidth(3, 120);
    ui->tableWidget->setColumnWidth(4, 0);
    ui->tableWidget->setColumnWidth(5, 60);
    ui->tableWidget->setColumnWidth(6, 30);

    ui->tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    QMenu *table_widget_menu = new QMenu(ui->tableWidget);
    table_widget_menu->addAction("设置网络信息", this, SLOT(setNetworkConfig()));
    table_widget_menu->addAction("设置硬件信息", this, SLOT(setHardwareConfig()));
    connect(ui->tableWidget, &QTableWidget::customContextMenuRequested, this, [=](const QPoint &pos) {
        currentRow = ui->tableWidget->currentRow();
        if (connectRow == -1) {
            ui->tableWidget->setCurrentItem(nullptr);
        }
        ui->tableWidget->selectRow(connectRow);
        if (currentRow >= 0) {
            table_widget_menu->exec(QCursor::pos());
        }
        Q_UNUSED(pos);
    });
    connect(ui->tableWidget, &QTableWidget::itemSelectionChanged, this, [=]() {
        if (ui->widget_2->isEnabled()) {
            ui->tableWidget->selectRow(connectRow);
        } else {
            connectRow = -1;
        }
    });
    connect(dialoghwconfig, &DialogHwConfig::currentChanged, this, &MainWindow::onCurrentChanged);
}

MainWindow::~MainWindow()
{
    tcpSocket->abort();
    delete ui;
    delete tcpSocket;
    delete udpSocket;
    libvlc_release (libvlc_inst);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj == ui->lineEditUrl && ui->widget_2->isEnabled()) {
        if (ev->type() == QEvent::MouseButtonDblClick) {
            QString fileName = QFileDialog::getOpenFileName(this,
                                                            tr("选择文件"),
                                                            ".",
                                                            tr("升级包 (*.bin)"));
            qDebug() << "filename : " << fileName;
            if (fileName.size() > 0) {
                ui->lineEditUrl->setText(fileName);
                QFile file(fileName);
//                QFile file(ui->lineEditUrl->text());
                if (!file.exists()) {
                    qDebug() << "file not exists";
                }
                if (file.open(QFile::ReadOnly)) {
                    if (QMessageBox::Yes == QMessageBox::question(this, "提示",
                                                  "上传文件过程中请勿做任何操作！您确定要上传吗？",
                                                  QMessageBox::Yes | QMessageBox::No)) {
                        int size = 0;
                        int val = 0;
                        QWidget bg(this);
                        QProgressDialog dialog("正在上传文件...", nullptr, 0, 8, this);
                        showDialog(&bg, &dialog);
                        dialog.installEventFilter(this);
                        while (1) {
                            dialog.setValue(val++);
                            QByteArray byte = file.read(2 * 1024 * 1024);
                            qDebug() << size;
                            postUpgrade(byte, QString::number(size));
                            size += byte.size();
                            if (byte.size() <= 0) {
                                tcpSocket->disconnectFromHost();
                                break ;
                            }
                        }
                        dialog.removeEventFilter(this);
                        QMessageBox::information(this, "提示",
                                                 "文件上传完成，升级大约需要2分钟，请稍候！",
                                                 QMessageBox::NoButton);
                    }
                }
            }
            return true;
        } else {
            return false;
        }
    } else if (obj->objectName() == "dialog_upgrade") {
        if (ev->type() == QEvent::Close) {
            ev->ignore();
            qDebug() << "dialog_upgrade Close";
            return true;
        } else {
            return false;
        }
    }/* else if (obj == ui->tableWidget) {
        if (ev->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mouseEvent = (QMouseEvent *)ev;
            if(mouseEvent->buttons() & Qt::RightButton) {
                ui->tableWidget->selectRow(connectRow);
                qDebug() << "RightButton";
            }
            return true;
        } else {
            return false;
        }
    }*/ else {
        // pass the event on to the parent class
        return QMainWindow::eventFilter(obj, ev);
    }
}

void MainWindow::onReadUdpDatagrams()
{
    while (udpSocket->hasPendingDatagrams()) {
#if QT_VERSION > 0x051500
        QNetworkDatagram datagram = udpSocket->receiveDatagram();
        if (0 == datagram.data().indexOf("neostra2021")) {
            continue ;
        }
        QByteArray byte = datagram.data();
#else
        qint64 recvSize = udpSocket->pendingDatagramSize();
//        if (recvSize != sizeof(udp_pkt_t) && recvSize != sizeof(udp_ack_t)) {
//            udpSocket->readDatagram(nullptr, 0);
//            continue ;
//        }
        QByteArray recvData(recvSize, 0);
        udpSocket->readDatagram(recvData.data(), recvSize);
#endif
        if (recvSize == sizeof(udp_ack_t)) {
            udp_ack_t *ack = (udp_ack_t*)recvData.data();
            qDebug() << "recv ack:" << ack->cid;
            if (uuid != ack->cid) {
                qDebug() << "ack not match";
                continue ;
            }

            switch (ack->command) {
            case GET_NETWORK:
                dialognwconfig->setConfig(ack->data);
                break;
            case GET_TUYA:
            case GET_CMCC:
                dialoghwconfig->setConfig(ack->command, ack->data);
                break;
            default:
                if (ack->state == 0)  {
                    QMessageBox::information(this, "提示",
                                             "设置成功！",
                                             QMessageBox::NoButton);
                }
                if (ack->state == 1) {
                    QMessageBox::information(this, "提示",
                                             "用户名或密码错误！",
                                             QMessageBox::NoButton);
                }
                break;
            }
        } else if (recvSize == sizeof(udp_pkt_t)) {
            udp_pkt_t *pkt = (udp_pkt_t*)recvData.data();
            if (uuid != pkt->cid) {
                qDebug() << "uuid not match";
                continue ;
            }
//            qDebug() << pkt->mac << pkt->sn << pkt->ip << pkt->ver;
            int count = ui->tableWidget->rowCount();
            ui->tableWidget->setRowCount(count+1);
            ui->tableWidget->setItem(count, 0, new QTableWidgetItem(pkt->mac));
            ui->tableWidget->setItem(count, 1, new QTableWidgetItem(pkt->sn));
            ui->tableWidget->setItem(count, 2, new QTableWidgetItem(pkt->ip));
            ui->tableWidget->setItem(count, 3, new QTableWidgetItem(pkt->ver));
            ui->tableWidget->setItem(count, 4, new QTableWidgetItem(pkt->commit));
            ui->tableWidget->setItem(count, 5, new QTableWidgetItem(QString::number(pkt->type)));
            ui->tableWidget->setItem(count, 6, new QTableWidgetItem(QString::number(pkt->uid)));
//        ui->tableWidget->setColumnCount(8);
//        ui->tableWidget->setCellWidget(count, 7, new QPushButton("设置IP"));
//        ui->tableWidget->setHorizontalHeaderItem(7, new QTableWidgetItem);
        }
    }
}

void MainWindow::onReadTcpDatagrams()
{
    QByteArray byte = tcpSocket->readAll();
    Cmd_pkt_t *recv_pkt = (Cmd_pkt_t*)byte.data();

    char *ptr = (char *)recv_pkt;
    short *ptrs = (short *)recv_pkt;

    qDebug("recv_data head: 0x%02x %02x %02x %02x %02x %02x, 0x%04x 0x%04x 0x%04x",
        *ptr, *(ptr+1), *(ptr+2), *(ptr+3), *(ptr+4), *(ptr+5), *(ptrs), *(ptrs+1), *(ptrs+2));
    if( recv_pkt->len > MAX_CONTENT_LEN)
    {
        qDebug("recv_pkt->len=0x%04x > MAX_CONTENT_LEN", recv_pkt->len);
        return ;
    }

    if( recv_pkt->len > 0)
    {
        if( verify_checksum(recv_pkt->content, recv_pkt->len, recv_pkt->checksum) < 0 )
        {
            return ;
        }
    }

    switch( recv_pkt->head)
    {
    case HEAD_CONTENT_ACK:
        qDebug() << "recv_pkt->content:" << (int)recv_pkt->content[0];
        if (1 == recv_pkt->content[0]) {
//            qDebug() << "login success";
            ui->btnConnect->setText("断开连接");
            ui->widget_2->setEnabled(true);
            connectRow = ui->tableWidget->currentRow();
        } else {
            QMessageBox::information(this, "提示",
                                     "用户名或密码错误！",
                                     QMessageBox::NoButton);
        }
        break;
    }
}

void MainWindow::onTcpConnected()
{
    qDebug() << "tcpConnected";

    QByteArray hashValue = QCryptographicHash::hash(ui->lineEdit_passwd->text().toUtf8(),
                                                    QCryptographicHash::Md5);
    QString user = ui->lineEdit_user->text();
    QString passwd = QString::fromUtf8(hashValue.toHex());
    QJsonObject jsonObj;
    jsonObj.insert(user, passwd);
    QJsonDocument document(jsonObj);
    QByteArray jsonByte = document.toJson();
//    qDebug() << "jsonByte:" << jsonByte;

    send_content(tcpSocket, HEAD_LOGIN, jsonByte.data(), jsonByte.size());
}

void MainWindow::onTcpDisconnected()
{
    ui->btnConnect->setText("连接设备");
    ui->widget_2->setEnabled(false);

    if (playerstate == STATE_PLAY) {
        vlcStop();
        ui->btnPlayVideo->setText("播放视频");
    }
    qDebug() << "tcpDisconnected";
}

void MainWindow::setNetworkConfig()
{
    udp_hw_config_t config;
    memset(&config, 0, sizeof(config));
    getConfig(&config);
    config.command = GET_NETWORK;
    udpSocket->writeDatagram((const char*)&config, sizeof(udp_hw_config_t),
                             QHostAddress(HOST_ADDRESS), 6868);
    dialognwconfig->open();
    dialognwconfig->exec();
    if (dialognwconfig->result()) {
        dialognwconfig->getConfig(&config);
        udpSocket->writeDatagram((const char*)&config, sizeof(udp_hw_config_t),
                                 QHostAddress(HOST_ADDRESS), 6868);
    }
}

void MainWindow::setHardwareConfig()
{
    onCurrentChanged(dialoghwconfig->getCurrentIndex());
    dialoghwconfig->displayHwInfo(ui->tableWidget->item(currentRow, 1)->text(),
                                  ui->tableWidget->item(currentRow, 0)->text());
    dialoghwconfig->open();
    dialoghwconfig->exec();
    if (dialoghwconfig->result()) {
        udp_hw_config_t config;
        getConfig(&config);
        dialoghwconfig->getConfig(&config);
        udpSocket->writeDatagram((const char*)&config, sizeof(udp_hw_config_t),
                                 QHostAddress(HOST_ADDRESS), 6868);
    }
}

void MainWindow::onCurrentChanged(int index)
{
    udp_hw_config_t config;
    getConfig(&config);
    if (0 == index) {
        config.command = GET_TUYA;
    }
    if (1 == index) {
        config.command = GET_CMCC;
    }
    udpSocket->writeDatagram((const char*)&config, sizeof(udp_hw_config_t),
                             QHostAddress(HOST_ADDRESS), 6868);
}

void MainWindow::sendBroadcast()
{
    udpSocket->writeDatagram(uuid, QHostAddress(HOST_ADDRESS), 6868);
#if 0
    QList<QNetworkInterface> interfaceList = QNetworkInterface::allInterfaces();
    foreach(QNetworkInterface interfaceItem, interfaceList) {
        if(interfaceItem.flags().testFlag(QNetworkInterface::IsUp)
                &&interfaceItem.flags().testFlag(QNetworkInterface::IsRunning)
                &&interfaceItem.flags().testFlag(QNetworkInterface::CanBroadcast)
                &&interfaceItem.flags().testFlag(QNetworkInterface::CanMulticast)
                &&!interfaceItem.flags().testFlag(QNetworkInterface::IsLoopBack)
                &&!interfaceItem.humanReadableName().contains("VMware")) {
            QList<QNetworkAddressEntry> addressEntryList=interfaceItem.addressEntries();
            foreach(QNetworkAddressEntry addressEntryItem, addressEntryList) {
                if(addressEntryItem.ip().protocol()==QAbstractSocket::IPv4Protocol) {
                    qDebug()<<"------------------------------------------------------------";
                    qDebug()<<"Adapter Name:"<<interfaceItem.name();
                    qDebug()<<"Adapter Address:"<<interfaceItem.hardwareAddress();
                    qDebug()<<"IP Address:"<<addressEntryItem.ip().toString();
                    qDebug()<<"IP Mask:"<<addressEntryItem.netmask().toString();
                    qDebug()<<"broadcast:"<<addressEntryItem.broadcast().toString();
                    udpSocket->writeDatagram("neostra2021", addressEntryItem.broadcast(), 6868);
                }
            }
        }
    }
#endif
}

void MainWindow::vlcPlay(const char *url)
{
    qDebug() << "vlc play";
    playerstate = STATE_PLAY;
    libvlc_media_t *libvlc_m;

    //Create a new item
    libvlc_m = libvlc_media_new_location (libvlc_inst, url);
    libvlc_media_add_option(libvlc_m, ":network-caching=300");
    libvlc_media_add_option(libvlc_m, ":rtsp-frame-buffer-size=300000");

    /* Create a media player playing environement */
    libvlc_mp = libvlc_media_player_new_from_media (libvlc_m);

    libvlc_event_manager_t *_vlcEvents = libvlc_media_player_event_manager(libvlc_mp);
    QList<libvlc_event_e> list;
    list << libvlc_MediaPlayerMediaChanged
         << libvlc_MediaPlayerNothingSpecial
         << libvlc_MediaPlayerOpening
         << libvlc_MediaPlayerBuffering
         << libvlc_MediaPlayerPlaying
         << libvlc_MediaPlayerPaused
         << libvlc_MediaPlayerStopped;

    foreach (const libvlc_event_e &event, list) {
        libvlc_event_attach(_vlcEvents, event, libvlc_callback, nullptr);
    }

    /* No need to keep the media now */
    libvlc_media_release (libvlc_m);

    WId winid = ui->widget->winId();
    // play the media_player
    libvlc_media_player_set_hwnd(libvlc_mp, (void *)winid);
    libvlc_media_player_play (libvlc_mp);
}

void MainWindow::vlcStop()
{
    qDebug() << "vlc stop";
    playerstate = STATE_STOP;
    if (libvlc_mp) {
        // Stop playing
        libvlc_media_player_stop (libvlc_mp);
        // Free the media_player
        libvlc_media_player_release (libvlc_mp);
        libvlc_mp = nullptr;
    }
}

void MainWindow::postUpgrade(const QByteArray &binary, const QString &offset)
{
    int row = ui->tableWidget->currentRow();
    if (row < 0) {
        return ;
    }
    QString ip = ui->tableWidget->item(row, 2)->text();
    const QString &url = "http://"+ ip +"/upload/798123498/upgrade?offset="+ offset +"&name=firmware.bin";
    QNetworkRequest request;
    request.setUrl(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QNetworkAccessManager *manager = new QNetworkAccessManager;
    QNetworkReply *reply = manager->post(request, binary);

    connect(reply, &QNetworkReply::finished, this, [=]() {
        QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
        qDebug() << "postUpgrade on_finished:" << reply->error();
        reply->deleteLater();
        reply->manager()->deleteLater();
    });
    QEventLoop eventloop;
    connect(reply, &QNetworkReply::finished, &eventloop, &QEventLoop::quit);
    eventloop.exec();
}

void MainWindow::showDialog(QWidget *bg, QProgressDialog *dialog)
{
    bg->resize(size());
    bg->setStyleSheet("background-color: rgb(0,0,0,128);");
    bg->show();
    dialog->resize(300, 100);
    dialog->setObjectName("dialog_upgrade");
    dialog->setStyleSheet("QLabel{font-family: 微软雅黑;"
                                 "font-weight: bold;"
                                 "font-size: 15px;"
                                 "color: #303030;}"
                         "QProgressBar{border: 1px solid grey;"         //外边框
                                 "border-color: rgb(128, 128, 128);"     //外边框颜色
                                 "text-align: center;"                  //字体对齐方式
                                 "background: rgb(255, 255, 255);}"
                         "QProgressBar::chunk {border: none;"
                                 "background: rgb(123, 199, 187);}"     //进度条颜色
                         );
    dialog->setWindowTitle("请等待");
    dialog->setModal(true);
    dialog->show();
}

void MainWindow::getConfig(void *handle)
{
    udp_hw_config_t *config = (udp_hw_config_t*)handle;
    strncpy(config->target_ip, ui->tableWidget->item(currentRow, 2)->text().toUtf8(),
            sizeof(config->target_ip));
    strncpy(config->cid, uuid, sizeof(config->cid));
    strncpy(config->user, ui->lineEdit_user->text().toUtf8(), sizeof(config->user));

    QByteArray hashValue = QCryptographicHash::hash(ui->lineEdit_passwd->text().toUtf8(),
                                                    QCryptographicHash::Md5);
    strncpy(config->passwd, hashValue.toHex(), sizeof(config->passwd));
    qDebug() << hashValue.toHex();
}

QString MainWindow::buildDateTime() const
{
    QString dateTime;
    dateTime += __DATE__;
    dateTime += __TIME__;
    dateTime.replace("  "," 0");//注意是两个空格，用于日期为单数时需要转成“空格+0”
    return QLocale(QLocale::English).toDateTime(dateTime, "MMM dd yyyyhh:mm:ss").toString(" yyyy.MM.dd");
}

void MainWindow::libvlc_callback(const libvlc_event_t *event,
                                     void *data)
{
    Q_UNUSED(data);
    qDebug() << "event->type:" << event->type;
    switch (event->type) {
    case libvlc_MediaPlayerMediaChanged:
        qDebug() << "emit core->mediaChanged" << event->u.media_player_media_changed.new_media;
        break;
    case libvlc_MediaPlayerNothingSpecial:
        qDebug() << "emit core->nothingSpecial();";
        break;
    case libvlc_MediaPlayerOpening:
        qDebug() << "emit core->opening();";
        break;
    case libvlc_MediaPlayerBuffering:
        qDebug() << "emit core->buffering"
                 << qRound(event->u.media_player_buffering.new_cache)
                 << event->u.media_player_buffering.new_cache;
        break;
    case libvlc_MediaPlayerPlaying:
        qDebug() << "emit core->playing();";
        break;
    case libvlc_MediaPlayerPaused:
        qDebug() << "emit core->paused();";
        break;
    case libvlc_MediaPlayerStopped:
        qDebug() << "emit core->stopped();";
        break;
    default:
        break;
    }

    if (event->type >= libvlc_MediaPlayerNothingSpecial
        && event->type <= libvlc_MediaPlayerEncounteredError) {
        qDebug() << "emit core->stateChanged();";
    }
}

void MainWindow::on_btnSearch_clicked()
{
    qDebug() << "Start Scan !";
    ui->tableWidget->clearContents();
    ui->tableWidget->setRowCount(0);
    sendBroadcast();
}


void MainWindow::on_btnPlayVideo_clicked()
{
    int row = ui->tableWidget->currentRow();
    qDebug() << "row:" << row << "playerstate:" << playerstate;
    if (playerstate == STATE_PLAY) {
        vlcStop();
        ui->btnPlayVideo->setText("播放视频");
        return ;
    }
    if (row < 0) {
        return ;
    }
    QString urlText = "rtsp://" + ui->tableWidget->item(row, 2)->text();
    if (playerstate == STATE_STOP) {
        vlcPlay(urlText.toUtf8());
        ui->btnPlayVideo->setText("停止播放");
    }
    qDebug() << "url: " << urlText;
}


void MainWindow::on_tableWidget_cellDoubleClicked(int row, int column)
{
    Q_UNUSED(column);
    QString ipText = ui->tableWidget->item(row, 2)->text();
    QString user = ui->lineEdit_user->text();
    QString passwd = ui->lineEdit_passwd->text();
    if (user.isEmpty() || passwd.isEmpty()) {
        qDebug() << "user or passwd is empty";
        return ;
    }
    if (QAbstractSocket::UnconnectedState == tcpSocket->state()) {
        tcpSocket->connectToHost(ipText, 8180);
    }
    if (tcpSocket->waitForConnected(1000)) {
        QString urlText = "rtsp://" + ui->tableWidget->item(row, 2)->text();
        if (playerstate == STATE_STOP) {
            vlcPlay(urlText.toUtf8());
            ui->btnPlayVideo->setText("停止播放");
        }
    }
}


void MainWindow::on_btnConnect_clicked()
{
    if (QAbstractSocket::ConnectedState == tcpSocket->state()) {
        tcpSocket->disconnectFromHost();
        return ;
    }
    QString user = ui->lineEdit_user->text();
    QString passwd = ui->lineEdit_passwd->text();
    int row = ui->tableWidget->currentRow();
    if (user.isEmpty() || passwd.isEmpty() || row < 0) {
        qDebug() << "user or passwd is empty";
        return ;
    }
    QString ipText = ui->tableWidget->item(row, 2)->text();
    tcpSocket->connectToHost(ipText, 8180);
}


void MainWindow::on_btnIrOpen_clicked()
{
    send_content(tcpSocket, HEAD_IR_LED_OPEN, NULL, 0);
}


void MainWindow::on_btnIrClose_clicked()
{
    send_content(tcpSocket, HEAD_IR_LED_CLOSE, NULL, 0);
}


void MainWindow::on_btnTestSpeaker_clicked()
{
    static bool enable = false;
    if(enable) {
        send_content(tcpSocket, HEAD_SPEAKER_STOP, NULL, 0);
        ui->btnTestSpeaker->setText("测试喇叭");
    } else {
        send_content(tcpSocket, HEAD_SPEAKER_START, NULL, 0);
        ui->btnTestSpeaker->setText("停止测试");
    }
    enable = !enable;
}


void MainWindow::on_btnLedOpen_clicked()
{
    send_content(tcpSocket, HEAD_WHITE_LED_OPEN, NULL, 0);
}


void MainWindow::on_btnLedClose_clicked()
{
    send_content(tcpSocket, HEAD_WHITE_LED_CLOSE, NULL, 0);
}


void MainWindow::on_btnReboot_clicked()
{
    send_content(tcpSocket, HEAD_REBOOT, NULL, 0);
    tcpSocket->disconnectFromHost();
}


void MainWindow::on_btnClearConfig_clicked()
{
    send_content(tcpSocket, HEAD_CLEAR_CONFIG, NULL, 0);
    tcpSocket->disconnectFromHost();
}


void MainWindow::on_btnAlarmOpen_clicked()
{
    send_content(tcpSocket, HEAD_ALARM_LED_OPEN, NULL, 0);
}


void MainWindow::on_btnAlarmClose_clicked()
{
    send_content(tcpSocket, HEAD_ALARM_LED_COLSE, NULL, 0);
}


void MainWindow::on_btnAlarmBOpen_clicked()
{
    send_content(tcpSocket, HEAD_ALARM_BLED_OPEN, NULL, 0);
}


void MainWindow::on_btnAlarmBClose_clicked()
{
    send_content(tcpSocket, HEAD_ALARM_BLED_COLSE, NULL, 0);
}


void MainWindow::on_btnIrcutDay_clicked()
{
    send_content(tcpSocket, HEAD_IRCUT_DAY, NULL, 0);
}


void MainWindow::on_btnIrcutNight_clicked()
{
    send_content(tcpSocket, HEAD_IRCUT_NIGHT, NULL, 0);
}



void MainWindow::on_btnFormatSd_clicked()
{
    int row = ui->tableWidget->currentRow();
    if (row < 0) {
        return ;
    }
    QString ip = ui->tableWidget->item(row, 2)->text();
    const QString &url = "http://"+ ip +"/api";
    QByteArray json("{\"action\":\"set\","
                    "\"data\":{\"mem_card\":{\"format\":1}},"
                    "\"token\":\"486c7c6f4e32f39840bd022616feab35\"}");
    QNetworkRequest request;
    request.setUrl(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json;charset=UTF-8");
    QNetworkAccessManager *manager = new QNetworkAccessManager;
    QNetworkReply *reply = manager->post(request, json);

    connect(reply, &QNetworkReply::finished, this, [=]() {
        QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
        qDebug() << "postJson on_finished:" << reply->error();
        qDebug() << "readAll:" << reply->readAll();
        reply->deleteLater();
        reply->manager()->deleteLater();
    });

    QWidget bg(this);
    QProgressDialog dialog("正在格式化...", nullptr, 0, 0, this);
    showDialog(&bg, &dialog);
    dialog.installEventFilter(this);

    QEventLoop eventloop;
    connect(reply, &QNetworkReply::finished, &eventloop, &QEventLoop::quit);
    eventloop.exec();
    dialog.removeEventFilter(this);
}

