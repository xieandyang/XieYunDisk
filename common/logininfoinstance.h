#ifndef LOGININFOINSTANCE_H
#define LOGININFOINSTANCE_H

#include <QString>
#include "common.h"

// 单例模式，主要保存当前登录用户、服务器信息
class LoginInfoInstance
{
public:
    static LoginInfoInstance *getInstance(); // 保证唯一实例
    static void destroy(); // 释放堆区空间
    void setLoginInfo(QString tmpUser, QString tmpIP, QString tmpPort, QString token=""); // 设置登陆信息
    QString getUser() const;
    QString getIp() const;
    QString getPort() const;
    QString getToken() const;
private:
    // 构造函数与析构函数设为私有
    LoginInfoInstance();
    ~LoginInfoInstance();
    // 把复制构造函数和=操作符也设为私有，防止被复制
    LoginInfoInstance(const LoginInfoInstance&);
    LoginInfoInstance& operator =(const LoginInfoInstance&);

    // 他的唯一工作就是在析构函数中删除instance实例
    class Garbo{
    public:
        ~Garbo()
        {
            // 释放堆区空间；
            LoginInfoInstance::destroy();
        }
    };

    // 定义一个静态成员变量，程序结束时，系统会自动调用它的析构函数
    // static类的析构函数在main()退出后调用
    static Garbo tmp; // 静态数据成员，类中声明，类外必须定义

    // 静态数据成员，类中声明，类外必须定义
    static LoginInfoInstance *instance;

    QString user; // 当前登录用户名
    QString token; // 登录token
    QString ip; // web服务器ip
    QString port; // web服务器端口
};

#endif // LOGININFOINSTANCE_H
