#include "dialoghwconfig.h"
#include "ui_dialoghwconfig.h"
#include "typedef.h"
#include <QDebug>
#include <QTableWidget>

DialogHwConfig::DialogHwConfig(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogHwConfig)
{
    ui->setupUi(this);
    ui->lineEdit->setMaxLength(MAX_SN_LEN-1);
    ui->lineEdit_2->setMaxLength(UDP_MAC_LEN-6);
    ui->lineEdit_3->setMaxLength(UDP_MAC_LEN-6);

    ui->lineEdit_4->setMaxLength(MAX_SN_LEN-1);
    ui->lineEdit_5->setMaxLength(MAX_SN_LEN-1);
    ui->lineEdit_6->setMaxLength(MAX_SN_LEN);

    ui->lineEdit_8->setMaxLength(MAX_SN_LEN-1);
    ui->lineEdit_9->setMaxLength(KEY_LEN-1);
    ui->lineEdit_10->setMaxLength(KEY_LEN-1);

    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &DialogHwConfig::currentChanged);
}

DialogHwConfig::~DialogHwConfig()
{
    delete ui;
}

void DialogHwConfig::getConfig(void *handle)
{
    udp_hw_config_t *config = (udp_hw_config_t*)handle;
    strncpy(config->hw_sn, ui->lineEdit->text().toUtf8(), MAX_SN_LEN);
    strncpy(config->hw_mac, ui->lineEdit_2->text().toUtf8(), UDP_MAC_LEN);
    strncpy(config->hw_lan, ui->lineEdit_3->text().toUtf8(), UDP_MAC_LEN);

    if (ui->tabWidget->currentIndex() == 0) {
        config->command = SET_TUYA;
        tuya_config_t *tuya = (tuya_config_t*)config->data;
        strncpy(tuya->pid, ui->lineEdit_4->text().toUtf8(), MAX_SN_LEN);
        strncpy(tuya->uuid, ui->lineEdit_5->text().toUtf8(), MAX_SN_LEN);
        strncpy(tuya->authkey, ui->lineEdit_6->text().toUtf8(), MAX_SN_LEN);
    }
    if (ui->tabWidget->currentIndex() == 1) {
        config->command = SET_CMCC;
        cmcc_config_t *cmcc = (cmcc_config_t*)config->data;
        strncpy(cmcc->sn, ui->lineEdit->text().toUtf8(), MAX_SN_LEN);
        strncpy(cmcc->cmei, ui->lineEdit_8->text().toUtf8(), MAX_SN_LEN);
        strncpy(cmcc->videokey, ui->lineEdit_9->text().toUtf8(), KEY_LEN);
        strncpy(cmcc->loginkey, ui->lineEdit_10->text().toUtf8(), KEY_LEN);
    }
}

void DialogHwConfig::setConfig(int cmd, void *handle)
{
//    udp_hw_config_t *config = (udp_hw_config_t*)handle;
    if (GET_TUYA == cmd) {
        tuya_config_t *tuya = (tuya_config_t*)handle;
        ui->lineEdit_4->setText(tuya->pid);
        ui->lineEdit_5->setText(tuya->uuid);
        ui->lineEdit_6->setText(tuya->authkey);
    }
    if (GET_CMCC == cmd) {
        cmcc_config_t *cmcc = (cmcc_config_t*)handle;
        ui->lineEdit->setText(cmcc->sn);
        ui->lineEdit_8->setText(cmcc->cmei);
        ui->lineEdit_9->setText(cmcc->videokey);
        ui->lineEdit_10->setText(cmcc->loginkey);
    }
}

int DialogHwConfig::getCurrentIndex()
{
    return ui->tabWidget->currentIndex();
}

void DialogHwConfig::displayHwInfo(const QString &sn, const QString &lan)
{
    ui->lineEdit->setText(sn);
    ui->lineEdit_3->setText(lan);
}
