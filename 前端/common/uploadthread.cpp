#include "uploadthread.h"
#include "common/common.h"
#include "common/logininfoinstance.h"
#include <QHttpPart>
#include <QHttpMultiPart>
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

UploadThread::UploadThread()
{

}

// 设置上传信息
void UploadThread::setUploadInfo(UploadFileInfo* info)
{
    this->uploadtask = info;
    this->m_manager = Common::getNetManager();
}

void UploadThread::run()
{
    // 取出上传任务
    QFile *file = uploadtask->file;           //文件指针
    qint64 pos = 0;
    QString fileName = uploadtask->fileName;  //文件名字
    QString md5 = uploadtask->md5;            //文件md5码
    qint64 size = uploadtask->size;           //文件大小
    QString boundary = m_cm.getBoundary();   //产生分隔线

    // 获取登录实例信息
    LoginInfoInstance* login = LoginInfoInstance::getInstance();

    while(pos < size)
    {
        QHttpPart part;
        QString disp = QString("from-data; user=\"%1\"; filename=\"%2\"; md5=\"%3\"; pos=%4; size=%5")
                .arg(login->getUser()).arg(fileName).arg(md5).arg(pos).arg(size);
        part.setHeader(QNetworkRequest::ContentDispositionHeader, disp);
        part.setHeader(QNetworkRequest::ContentTypeHeader, "image/png"); // 传输的文件对应的content-type
//        part.setBodyDevice(file);
        file->seek(pos);
        part.setBody(file->read(1024 * 100));
        QHttpMultiPart* multipart = new QHttpMultiPart(QHttpMultiPart::FormDataType, this);
        multipart->append(part);


        QNetworkRequest request;
        QString url = QString("http://%1:%2/upload").arg(login->getIp()).arg(login->getPort());
        request.setUrl(QUrl(url));

        cout << url;

        // qt默认的请求头
        request.setHeader(QNetworkRequest::ContentTypeHeader, "multipart/form-data");

        // 发送post请求
        QNetworkReply* reply = m_manager->post(request, multipart);
        if(reply == NULL)
        {
            cout << "reply == NULL";
            return;
        }

        cout << "已经发送" << url;
        // 有可用数据更新时
        emit updateDp(pos, size); // 设置进度条

        QEventLoop eventLoop;
        connect(reply, &QNetworkReply::readyRead, &eventLoop, &QEventLoop::quit);
        eventLoop.exec(QEventLoop::ExcludeUserInputEvents);       //block until finish

        if (reply->error() != QNetworkReply::NoError) //有错误
        {
            cout << reply->errorString();
            reply->deleteLater();
            m_cm.writeRecord(login->getUser(), uploadtask->fileName, "009");
            emit uploadError();
            return;
        }

        QByteArray array = reply->readAll();

        reply->deleteLater();
        // 析构对象
        multipart->deleteLater();
        /*
            上传文件：
                成功：{"code":"008"}
                失败：{"code":"009"}
            */
        cout << "返回数据：code=" << m_cm.getCode(array) << ", data=" << QString(array);
        if("008" == m_cm.getCode(array))
        {
            pos += 1024 * 100;
        }
        else if("009" == m_cm.getCode(array))
        {
            cout << reply->errorString();
            reply->deleteLater();
            m_cm.writeRecord(login->getUser(), uploadtask->fileName, "009");
            emit uploadError();
            return;
        }

        if(pos >= size)
        {
            m_cm.writeRecord(login->getUser(), uploadtask->fileName, "008");
            emit uploadFinish();
            return;
        }
    }
}
