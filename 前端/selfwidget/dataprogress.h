#ifndef DATAPROGRESS_H
#define DATAPROGRESS_H

#include <QWidget>

// 上传，下载进度控件
namespace Ui {
class DataProgress;
}

class DataProgress : public QWidget
{
    Q_OBJECT

public:
    explicit DataProgress(QWidget *parent = 0);
    ~DataProgress();

    // 设置文件名字
    void setFileName(QString name = "测试");
    // 设置进度条的当前值value，最大值max
    void setProgress(int value, int max);

private:
    Ui::DataProgress *ui;
};

#endif // DATAPROGRESS_H
