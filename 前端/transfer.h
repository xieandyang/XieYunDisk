#ifndef TRANSFER_H
#define TRANSFER_H

#include <QWidget>
#include "common/common.h"

namespace Ui {
class Transfer;
}

class Transfer : public QWidget
{
    Q_OBJECT

public:
    explicit Transfer(QWidget *parent = 0);
    ~Transfer();

    // 显示数据传输记录
    void displayDataRecord(QString path = RECORDDIR);
    // 显示上传窗口
    void showUpload();
    // 显示下载窗口
    void showDownload();

signals:
    // 告诉主界面，当前是哪个tab
    void currentTabSignal(QString);

private:
    Ui::Transfer *ui;
};

#endif // TRANSFER_H
