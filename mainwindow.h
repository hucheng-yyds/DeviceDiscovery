#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtNetwork>
//#include <QTableWidgetItem>
#include "vlc/vlc.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QProgressDialog;
class DialogNwConfig;
class DialogHwConfig;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    enum PlayerState {
        STATE_PLAY,
        STATE_STOP
    };
    Q_ENUM(PlayerState)

    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;

private:
    Ui::MainWindow *ui;
    QTcpSocket *tcpSocket;
    QUdpSocket *udpSocket;
    libvlc_instance_t *libvlc_inst;
    libvlc_media_player_t *libvlc_mp;
    PlayerState playerstate;
    QByteArray uuid;
    int connectRow;
    int currentRow;
    DialogNwConfig *dialognwconfig;
    DialogHwConfig *dialoghwconfig;

    void sendBroadcast();
    void vlcPlay(const char *url);
    void vlcStop();
    void postUpgrade(const QByteArray &binary, const QString &offset);
    void showDialog(QWidget *bg, QProgressDialog *dialog);
    void getConfig(void *handle);
    QString buildDateTime() const;
    static void libvlc_callback(const libvlc_event_t *event, void *data);

public slots:
    void onReadUdpDatagrams();
    void onReadTcpDatagrams();
    void onTcpConnected();
    void onTcpDisconnected();
    void setNetworkConfig();
    void setHardwareConfig();
    void onCurrentChanged(int index);

private slots:
    void on_btnSearch_clicked();
    void on_btnPlayVideo_clicked();
    void on_tableWidget_cellDoubleClicked(int row, int column);
    void on_btnConnect_clicked();
    void on_btnIrOpen_clicked();
    void on_btnIrClose_clicked();
    void on_btnTestSpeaker_clicked();
    void on_btnLedOpen_clicked();
    void on_btnLedClose_clicked();
    void on_btnReboot_clicked();
    void on_btnClearConfig_clicked();
    void on_btnAlarmOpen_clicked();
    void on_btnAlarmClose_clicked();
    void on_btnAlarmBOpen_clicked();
    void on_btnAlarmBClose_clicked();
    void on_btnIrcutDay_clicked();
    void on_btnIrcutNight_clicked();
    void on_btnFormatSd_clicked();
};
#endif // MAINWINDOW_H
