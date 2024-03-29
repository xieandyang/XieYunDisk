#ifndef TITLEWIDGET_H
#define TITLEWIDGET_H

#include <QWidget>

namespace Ui {
class TitleWidget;
}

class TitleWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TitleWidget(QWidget *parent = 0);
    ~TitleWidget();

    void setParent(QWidget *parent);

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

signals:
    void showSetWidget();
    void closeWindow();

private:
    Ui::TitleWidget *ui;

    QPoint m_pos; // 保存鼠标按下时的坐标
    QWidget *m_parent; // 父窗口指针
};

#endif // TITLEWIDGET_H
