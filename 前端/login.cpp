#include "login.h"
#include "ui_login.h"
#include <QPainter>
#include <QRegExp>
#include <QMessageBox>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include "common/logininfoinstance.h"
#include "common/des.h"

Login::Login(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Login)
{
    ui->setupUi(this);

    // 初始化
    // 网络请求（http）类
    m_manager = Common::getNetManager();

    m_mainWin = new MainWindow;
    // 设置title_widget的父窗口
    ui->title_widget->setParent(this);

    // 窗口图标
    this->setWindowIcon(QIcon(":/images/logo.ico"));
    m_mainWin->setWindowIcon(QIcon(":/images/logo.ico"));

    // 去掉边框
    this->setWindowFlags(this->windowFlags() | Qt::FramelessWindowHint);

    // 设置当前窗口的字体信息
    this->setFont(QFont("新宋体", 12, QFont::Bold, false));

    // 当前显示的窗口
    ui->stackedWidget->setCurrentIndex(0);
    ui->log_usr->setFocus();
    // 密码
    ui->log_pwd->setEchoMode(QLineEdit::Password);
    ui->reg_pwd->setEchoMode(QLineEdit::Password);
    ui->reg_surepwd->setEchoMode(QLineEdit::Password);
    // 数据的格式提示
    ui->log_usr->setToolTip("合法字符:[a-z|A-Z|#|@|0-9|-|_|*],字符个数: 3~16");
    ui->reg_usr->setToolTip("合法字符:[a-z|A-Z|#|@|0-9|-|_|*],字符个数: 3~16");
    ui->reg_nickname->setToolTip("合法字符:[a-z|A-Z|#|@|0-9|-|_|*],字符个数: 3~16");
    ui->log_pwd->setToolTip("合法字符:[a-z|A-Z|#|@|0-9|-|_|*],字符个数: 6~18");
    ui->reg_pwd->setToolTip("合法字符:[a-z|A-Z|#|@|0-9|-|_|*],字符个数: 6~18");
    ui->reg_surepwd->setToolTip("合法字符:[a-z|A-Z|#|@|0-9|-|_|*],字符个数: 6~18");

    // 读取配置文件信息，并初始化
    readCfg();
    ui->address_server->setText(QString("47.113.193.186"));
    ui->port_server->setText(QString("80"));
    ui->log_usr->setText(QString("xie2018"));
    ui->log_pwd->setText(QString("240905"));

    // 加载图片信息 - 显示文件列表的时候用，在此初始化
    m_cm.getFileTypeList();

    // 处理接收的title_widget信号
    // 设置按钮
    connect(ui->title_widget, &TitleWidget::showSetWidget, this, [=](){
        ui->stackedWidget->setCurrentWidget(ui->set_page);
        ui->address_server->setFocus();
    });
    // 关闭按钮
    connect(ui->title_widget, &TitleWidget::closeWindow, this, [=](){
        // 如果是登录窗口
        if(ui->stackedWidget->currentWidget() == ui->login_page)
        {
            close();
        }
        // 如果是注册窗口
        else if(ui->stackedWidget->currentWidget() == ui->register_page)
        {
            // 清空数据
            ui->reg_usr->clear();
            ui->reg_nickname->clear();
            ui->reg_pwd->clear();
            ui->reg_surepwd->clear();
            ui->reg_phone->clear();
            ui->reg_mail->clear();
            // 窗口切换
            ui->stackedWidget->setCurrentWidget(ui->login_page);
            ui->log_usr->setFocus();
        }
        // 如果是设置窗口
        else if(ui->stackedWidget->currentWidget() == ui->set_page)
        {
            // 清空数据
            ui->address_server->clear();
            ui->port_server->clear();
            // 窗口切换
            ui->stackedWidget->setCurrentWidget(ui->login_page);
            ui->log_usr->setFocus();
        }
    });
    // 注册按钮
    connect(ui->log_register_btn, &QToolButton::clicked, this, [=](){
        // 切换到注册页面
        ui->stackedWidget->setCurrentWidget(ui->register_page);
        ui->reg_usr->setFocus();
    });
    // 切换用户-重新登陆
    connect(m_mainWin, &MainWindow::changeUser, this, [=](){
        m_mainWin->hide();
        this->show();
    });
}

Login::~Login()
{
    delete ui;
}

void Login::paintEvent(QPaintEvent *event)
{
    QPainter p(this);
    p.drawPixmap(0, 0, this->width(), this->height(), QPixmap(":/images/login_bk.jpg"));
}

// 将注册信息打包为json格式
QByteArray Login::setRegisterJson(QString userName, QString passwd, QString nickName, QString phone, QString email)
{
    QMap<QString, QVariant> reg;
    reg.insert("userName", userName);
    reg.insert("passwd", passwd);
    reg.insert("nickName", nickName);
    reg.insert("phone", phone);
    reg.insert("email", email);

    /*json数据如下
        {
            userName:xxxx,
            passwd:xxx,
            nickName:xxx,
            phone:xxx,
            email:xxx
        }
    */

    QJsonDocument jsonDocument = QJsonDocument::fromVariant(reg);
    if(jsonDocument.isNull())
    {
        cout << "jsonDocument.isNULL()";
        return "";
    }
    return jsonDocument.toJson();
}

// 将登录用户信息打包为json包
QByteArray Login::setLoginJson(QString userName, QString passwd)
{
    QMap<QString, QVariant> login;
    login.insert("userName", userName);
    login.insert("passwd", passwd);
    /*json数据如下
        {
            userName:xxxx,
            passwd:xxx
        }
    */
    QJsonDocument jsonDocument = QJsonDocument::fromVariant(login);
    if(jsonDocument.isNull())
    {
        cout << "jsonDocument.isNULL()";
        return "";
    }
    return jsonDocument.toJson();
}

// 得到服务器回复的登陆状态， 状态码返回值为 "000", 或 "001"，还有登陆section
QStringList Login::getLoginStatus(QByteArray json)
{
    QJsonParseError error;
    QStringList list;
    // 将来源数据json转化为JsonDocument
    // 由QByteArray对象构造一个QJsonDocument对象，用于我们的读写操作ss
    QJsonDocument doc = QJsonDocument::fromJson(json, &error);
    if(error.error == QJsonParseError::NoError)
    {
        if(doc.isNull() || doc.isEmpty())
        {
            cout << "doc.isNull() || doc.isEmpty()";
            return list;
        }

        if(doc.isObject())
        {
            // 取得最外层这个大对象
            QJsonObject obj = doc.object();
            // 状态码
            list.append(obj.value("code").toString());
            // 登录token
            list.append(obj.value("token").toString());
            return list;
        }
    }
    else
    {
        cout << "error = " << error.errorString();
    }
    return list;
}

void Login::on_register_btn_clicked()
{
    // 1、从控件中取出用户输入的数据
    QString userName = ui->reg_usr->text();
    QString nickName = ui->reg_nickname->text();
    QString passwd = ui->reg_pwd->text();
    QString surePwd = ui->reg_surepwd->text();
    QString phone = ui->reg_phone->text();
    QString email = ui->reg_mail->text();

    // 2、数据校验
    // 校验用户名、密码
    QRegExp regexp(USER_REG);
    // bool bl = regexp.exactMatch(userName);
    if(!regexp.exactMatch(userName))
    {
        QMessageBox::warning(this, "警告", "用户名格式不正确！");
        ui->reg_usr->clear();
        ui->reg_usr->setFocus();
        return;
    }
    regexp.setPattern(PASSWD_REG);
    if(!regexp.exactMatch(passwd))
    {
        QMessageBox::warning(this, "警告", "密码格式不正确！");
        ui->reg_pwd->clear();
        ui->reg_pwd->setFocus();
        return;
    }
    if(surePwd != passwd)
    {
        QMessageBox::warning(this, "警告", "两次输入的密码不相同，请重新输入");
        ui->reg_pwd->clear();
        ui->reg_surepwd->clear();
        ui->reg_pwd->setFocus();
        return;
    }
    regexp.setPattern(PHONE_REG);
    if(!regexp.exactMatch(phone))
    {
        QMessageBox::warning(this, "警告", "手机号码格式不正确！");
        ui->reg_phone->clear();
        ui->reg_phone->setFocus();
        return;
    }
    regexp.setPattern(EMAIL_REG);
    if(!regexp.exactMatch(email))
    {
        QMessageBox::warning(this, "警告", "邮箱格式不正确！");
        ui->reg_mail->clear();
        ui->reg_mail->setFocus();
        return;
    }

    // 3、用户信息发送给服务器
    // - 如何发送：使用http协议发送，使用post方式
    // - 数据格式：json对象
    // 将注册信息打包为json格式
    QByteArray array = setRegisterJson(userName, m_cm.getStrMd5(passwd), nickName, phone, email);
    // 设置连接服务器要发送的url
    QNetworkRequest request;
    QString url = QString("http://%1:%2/reg").arg(ui->address_server->text()).arg(ui->port_server->text());
    request.setUrl(QUrl(url));
    // 请求头信息
    request.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/json"));
    request.setHeader(QNetworkRequest::ContentLengthHeader, QVariant(array.size()));
    // 发送数据
    QNetworkReply* reply = m_manager->post(request, array);
    cout << "post url:" << url << "post data: " << array;

    // 4、接收服务器发送的响应数据
    connect(reply, &QNetworkReply::readyRead, this, [=](){
        // 5、对服务器响应进行分析处理，成功or失败
        // 5.1、接收数据
        QByteArray jsonData = reply->readAll();
        // 5.2、需要知道服务器往回发送的字符串的格式 -> 解析
        /*
        注册 - server端返回的json格式数据：
            成功:         {"code":"002"}
            该用户已存在：  {"code":"003"}
            失败:         {"code":"004"}
        */
        QString status = m_cm.getCode(jsonData);
        // 5.3、判断成功失败，给用户提示
        if("002" == status)
        {
            // 注册成功
            QMessageBox::information(this, "注册成功", "注册成功， 请登录");
            // 清空行编辑内容
            ui->reg_usr->clear();
            ui->reg_pwd->clear();
            ui->reg_surepwd->clear();
            ui->reg_nickname->clear();
            ui->reg_phone->clear();
            ui->reg_mail->clear();
            // 设置登录窗口的登录信息
            ui->log_usr->setText(userName);
            ui->log_pwd->setText(passwd);
            ui->rember_pwd->setChecked(true);
            // 切换到登陆页面
            ui->stackedWidget->setCurrentWidget(ui->login_page);
        }
        else if("003" == status)
        {
            // 该用户已存在
            QMessageBox::warning(this, "注册失败", QString("%1 该用户已存在！！！").arg(userName));
        }
        else if("004" == status)
        {
            QMessageBox::warning(this, "注册失败", "注册失败！！！");
        }
        // 释放资源
        delete reply;
    });
}

// 用户设置操作
void Login::on_set_ok_btn_clicked()
{
    QString ip = ui->address_server->text();
    QString port = ui->port_server->text();

    // 数据判断
    // 服务器IP
    // \\d 和 \\. 中第一个\是转义字符, 这里使用的是标准正则
    QRegExp regexp(IP_REG);
    if(!regexp.exactMatch(ip))
    {
        QMessageBox::warning(this, "设置失败", "输入的IP地址格式不正确，请重新输入！");
        return;
    }
    // 端口
    regexp.setPattern(PORT_REG);
    if(!regexp.exactMatch(port))
    {
        QMessageBox::warning(this, "设置失败", "输入的端口号格式不正确，请重新输入！");
        return;
    }
    // 跳转到登录界面
    ui->stackedWidget->setCurrentWidget(ui->login_page);
    // 将配置信息写入配置文件中
    m_cm.writeWebInfo(ip, port);
}

void Login::on_login_btn_clicked()
{
    // 获取用户登录信息
    QString userName = ui->log_usr->text();
    QString passwd = ui->log_pwd->text();
    QString address = ui->address_server->text();
    QString port = ui->port_server->text();

    // 数据校验
    QRegExp regexp(USER_REG);
    if(!regexp.exactMatch(userName))
    {
        QMessageBox::warning(this, "警告", "用户名格式不正确！");
        ui->log_usr->clear();
        ui->log_usr->setFocus();
        return;
    }
    regexp.setPattern(PASSWD_REG);
    if(!regexp.exactMatch(passwd))
    {
        QMessageBox::warning(this, "警告", "密码格式不正确！");
        ui->log_pwd->clear();
        ui->log_pwd->setFocus();
        return;
    }

    // 登录信息写入配置文件cfg.json
    m_cm.writeLoginInfo(userName, passwd, ui->rember_pwd->isChecked());
    // 设置登陆信息json包, 密码经过md5加密， getStrMd5()
    QByteArray array = setLoginJson(userName, m_cm.getStrMd5(passwd));
    // 设置登陆的url
    QNetworkRequest request;
    QString url = QString("http://%1:%2/login").arg(address).arg(port);
    request.setUrl(QUrl(url));
    cout << "post url:" << url << "post data: " << array;
    // 请求头信息
    request.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/json"));
    request.setHeader(QNetworkRequest::ContentLengthHeader, QVariant(array.size()));
    // 发送数据
    QNetworkReply* reply = m_manager->post(request, array);
    if(reply == NULL)
    {
        cout << "reply == NULL";
        return;
    }
    // 4、接收服务器发送的响应数据
    connect(reply, &QNetworkReply::finished, this, [=](){
        // 出错了
        if(reply->error() != QNetworkReply::NoError)
        {
            cout << reply->errorString();
            // 释放资源
            reply->deleteLater();
            return;
        }
        // 5.1、接收数据
        QByteArray jsonData = reply->readAll();
        // 5.2、需要知道服务器往回发送的字符串的格式 -> 解析
        /*
            登陆 - 服务器回写的json数据包格式：
                成功：{"code":"000"}
                失败：{"code":"001"}
        */
        cout << "server return value: " << jsonData;
        QStringList list = getLoginStatus(jsonData);
        if("000" == list.at(0))
        {
            cout << "登陆成功";
            // 设置登录信息，显示文件列表需要这些信息
            LoginInfoInstance *p = LoginInfoInstance::getInstance();
            p->setLoginInfo(userName, address, port, list.at(1));
            // 当前窗口隐藏
            this->hide();
            // 主窗口显示
            m_mainWin->showMainWindow();
        }
        else
        {
            QMessageBox::warning(this, "登陆失败", "用户名或密码不正确！");
        }
        reply->deleteLater(); // 释放资源
    });
}


// 读取配置信息，设置默认登录状态，默认设置信息
void Login::readCfg()
{
    QString user = m_cm.getCfgValue("login", "user");
    QString pwd = m_cm.getCfgValue("login", "pwd");
    QString remeber = m_cm.getCfgValue("login", "remember");

    int ret = 0;
    if(remeber == "yes") // 记住密码
    {
        // 密码解密
        unsigned char encPwd[512] = {0};
        int encPwdLen = 0;
        // toLocal8Bit(), 转换为本地字符集，默认windows则为gbk编码，linux为utf-8编码
        QByteArray tmp = QByteArray::fromBase64(pwd.toLocal8Bit());
        ret = DesDec((unsigned char *)tmp.data(), tmp.size(), encPwd, &encPwdLen);
        if(ret != 0)
        {
            cout << "DesDec";
            return;
        }
        #ifdef _WIN32 // 如果是windows平台
        // fromLocal8Bit()，本地字符集转换为utf8
            ui->log_pwd->setText(QString::fromLocal8Bit((const char *)encPwd, encPwdLen));
        # else // 其他平台
            ui->log_pwd->setText((const char *)encPwd);
        # endif

        ui->rember_pwd->setCheckable(true);
    }
    else // 没有记住密码
    {
        ui->log_pwd->setText("");
        ui->rember_pwd->setChecked(false);
    }

    // 用户解密
    unsigned char encUser[512] = {0};
    int encUserLen = 0;
    // toLocal8Bit(), 转换为本地字符集，默认windows则为gbk编码，linux为utf-8编码
    QByteArray tmp = QByteArray::fromBase64(user.toLocal8Bit());
    ret = DesDec((unsigned char *)tmp.data(), tmp.size(), encUser, &encUserLen);
    if(ret != 0)
    {
        cout << "DesDec";
        return;
    }
    #ifdef _WIN32 // 如果是windows平台
    // fromLocal8Bit()，本地字符集转换为utf8
        ui->log_usr->setText(QString::fromLocal8Bit((const char *)encUser, encUserLen));
    # else // 其他平台
        ui->log_usr->setText((const char *)encUser);
    # endif

    QString ip = m_cm.getCfgValue("web_server", "ip");
    QString port = m_cm.getCfgValue("web_server", "port");
    ui->address_server->setText(ip);
    ui->port_server->setText(port);
}
