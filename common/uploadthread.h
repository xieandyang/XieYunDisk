#ifndef UPLOADTHREAD_H
#define UPLOADTHREAD_H

#include <QThread>
#include "common/uploadtask.h"

class UploadThread : public QThread
{
    Q_OBJECT
public:
    UploadThread();

    // 设置上传信息
    void setUploadInfo(UploadFileInfo* info);

protected:
    void run();

signals:
    void updateDp(qint64 pos, qint64 size);
    void uploadFinish();
    void uploadError();

private:
    Common m_cm;
    QNetworkAccessManager* m_manager = NULL;
    UploadFileInfo* uploadtask = NULL;
};

#endif // UPLOADTHREAD_H
