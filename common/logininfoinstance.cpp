#include "logininfoinstance.h"

// 静态数据成员，类中声明，类外必须定义
LoginInfoInstance::Garbo LoginInfoInstance::tmp;
LoginInfoInstance* LoginInfoInstance::instance = new LoginInfoInstance;

LoginInfoInstance::LoginInfoInstance()
{

}

LoginInfoInstance::~LoginInfoInstance()
{

}

//把复制构造函数和=操作符也设为私有,防止被复制
LoginInfoInstance::LoginInfoInstance(const LoginInfoInstance& )
{
}

LoginInfoInstance& LoginInfoInstance::operator=(const LoginInfoInstance&)
{
    return *this;
}

LoginInfoInstance* LoginInfoInstance::getInstance() // 保证唯一实例
{
    return instance;
}

void LoginInfoInstance::destroy() // 释放堆区空间
{
    if(NULL != instance)
    {
        delete instance;
        instance = NULL;
        cout << "instance is delete";
    }
}

void LoginInfoInstance::setLoginInfo(QString tmpUser, QString tmpIP, QString tmpPort, QString token) // 设置登陆信息
{
    user = tmpUser;
    ip = tmpIP;
    port = tmpPort;
    this->token = token;
}

QString LoginInfoInstance::getUser() const
{
    return user;
}

QString LoginInfoInstance::getIp() const
{
    return ip;
}

QString LoginInfoInstance::getPort() const
{
    return port;
}

QString LoginInfoInstance::getToken() const
{
    return token;
}
