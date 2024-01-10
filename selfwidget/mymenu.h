#ifndef MYMENU_H
#define MYMENU_H

#include <QMenu>

class MyMenu : public QMenu
{
    Q_OBJECT
public:
    explicit MyMenu(QWidget *parent = 0);
};

#endif // MYMENU_H
