#include "dialognwconfig.h"
#include "ui_dialognwconfig.h"
#include "typedef.h"
#include <QDebug>

DialogNwConfig::DialogNwConfig(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogNwConfig)
{
    ui->setupUi(this);
}

DialogNwConfig::~DialogNwConfig()
{
    delete ui;
}

void DialogNwConfig::getConfig(void *handle)
{
    qDebug() << "DialogNwConfig" << ui->lineEdit->text();
    udp_hw_config_t *config = (udp_hw_config_t*)handle;
    network_config_t *nw_config = (network_config_t*)config->data;
    config->command = SET_NETWORK;
    if (ui->radioButton_2->isChecked()) {
        nw_config->ipmode = IPMODE_STATIC;
    } else {
        nw_config->ipmode = IPMODE_DHCP;
    }
    strncpy(nw_config->ip, ui->lineEdit->text().toUtf8(), IPADDR_BUF_LEN);
    strncpy(nw_config->netmask, ui->lineEdit_2->text().toUtf8(), IPADDR_BUF_LEN);
    strncpy(nw_config->gateway, ui->lineEdit_3->text().toUtf8(), IPADDR_BUF_LEN);
    strncpy(nw_config->dns1, ui->lineEdit_4->text().toUtf8(), IPADDR_BUF_LEN);
    strncpy(nw_config->dns2, ui->lineEdit_5->text().toUtf8(), IPADDR_BUF_LEN);
}

void DialogNwConfig::setConfig(void *handle)
{
    network_config_t *nw_config = (network_config_t*)handle;
    qDebug() << "ipmode:" << nw_config->ipmode;
    if (nw_config->ipmode == IPMODE_DHCP) {
        ui->radioButton->setChecked(true);
        on_radioButton_2_toggled(false);
    }
    if (nw_config->ipmode == IPMODE_STATIC) {
        ui->radioButton_2->setChecked(true);
    }
    ui->lineEdit->setText(nw_config->ip);
    ui->lineEdit_2->setText(nw_config->netmask);
    ui->lineEdit_3->setText(nw_config->gateway);
    ui->lineEdit_4->setText(nw_config->dns1);
    ui->lineEdit_5->setText(nw_config->dns2);
}

void DialogNwConfig::on_radioButton_2_toggled(bool checked)
{
    qDebug() << "on_radioButton_2_clicked" << checked;

    ui->label->setEnabled(checked);
    ui->label_2->setEnabled(checked);
    ui->label_3->setEnabled(checked);
    ui->label_4->setEnabled(checked);
    ui->label_5->setEnabled(checked);
    ui->lineEdit->setEnabled(checked);
    ui->lineEdit_2->setEnabled(checked);
    ui->lineEdit_3->setEnabled(checked);
    ui->lineEdit_4->setEnabled(checked);
    ui->lineEdit_5->setEnabled(checked);
}

