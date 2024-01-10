#include "titlewidget.h"
#include "ui_titlewidget.h"
#include <QMouseEvent>
#include <QPushButton>

TitleWidget::TitleWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TitleWidget)
{
    ui->setupUi(this);
    // setParent(parent);
    // logo
    ui->logo->setPixmap(QPixmap(":/images/logo.ico").scaled(40, 40));

    // 按钮
    // 最小化
    connect(ui->min_btn, &QToolButton::clicked, this, [=](){
        m_parent->showMinimized();
    });
    // 设置
    connect(ui->set_btn, &QToolButton::clicked, this, [=](){
        emit showSetWidget();
    });
    // 关闭
    connect(ui->close_btn, &QToolButton::clicked, this, [=](){
        emit closeWindow();
    });
}

TitleWidget::~TitleWidget()
{
    delete ui;
}

void TitleWidget::setParent(QWidget *parent)
{
    this->m_parent = parent;
}

void TitleWidget::mousePressEvent(QMouseEvent *event)
{
    // 如果是左键, 计算窗口左上角, 和当前按钮位置的距离
    if(event->button() & Qt::LeftButton)
    m_pos = event->globalPos() - m_parent->geometry().topLeft();
}

void TitleWidget::mouseMoveEvent(QMouseEvent *event)
{
    // 移动是持续的状态, 需要使用buttons
    if(event->buttons() & Qt::LeftButton)
    {
        QPoint pos = event->globalPos() - m_pos;
        m_parent->move(pos);
    }
}
