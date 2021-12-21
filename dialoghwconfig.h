#ifndef DialogHwConfig_H
#define DialogHwConfig_H

#include <QDialog>

namespace Ui {
class DialogHwConfig;
}

class DialogHwConfig : public QDialog
{
    Q_OBJECT

public:
    explicit DialogHwConfig(QWidget *parent = nullptr);
    ~DialogHwConfig();
    void getConfig(void *handle);
    void setConfig(int cmd, void *handle);
    int getCurrentIndex();
    void displayHwInfo(const QString &sn, const QString &lan);

Q_SIGNALS:
    void currentChanged(int index);

private:
    Ui::DialogHwConfig *ui;
};

#endif // DialogHwConfig_H
