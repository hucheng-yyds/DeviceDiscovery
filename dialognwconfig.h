#ifndef DIALOGNWCONFIG_H
#define DIALOGNWCONFIG_H

#include <QDialog>

namespace Ui {
class DialogNwConfig;
}

class DialogNwConfig : public QDialog
{
    Q_OBJECT

public:
    explicit DialogNwConfig(QWidget *parent = nullptr);
    ~DialogNwConfig();
    void getConfig(void *handle);
    void setConfig(void *handle);

private slots:
    void on_radioButton_2_toggled(bool checked);

private:
    Ui::DialogNwConfig *ui;
};

#endif // DIALOGNWCONFIG_H
