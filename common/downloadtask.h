#ifndef DOWNLOADTASK_H
#define DOWNLOADTASK_H


#include "common.h"
#include <QVBoxLayout>
#include "selfwidget/dataprogress.h"
#include <QFile>
#include <QList>


// 下载文件信息
struct DownloadInfo{
    QString md5;            // 文件md5码
    QFile* file;            // 文件指针
    QString user;           // 下载用户
    QString fileName;       // 文件名
    QString url;            // 下载网址
    bool isDownload;          // 是否已经在下载
    DataProgress* dp;       // 下载进度控件
    bool isShare;           // 是否为共享文件下载
};


//下载任务列表类，单例模式，一个程序只能有一个下载任务列表
class DownloadTask
{
public:
    static DownloadTask* getInstance(); // 保证唯一一个实例

    //追加任务到下载队列
    //参数：info：下载文件信息， filePathName：文件保存路径, isShare: 是否为共享文件下载, 默认为false
    //返回值：成功为0
    //失败：
    //  -1: 下载的文件是否已经在下载队列中
    //  -2: 打开文件失败
    int appendDownloadList(FileInfo *info, QString filePathName, bool isShare = false);

    bool isEmpty();         // 判断下载队列是否为空
    bool isDownload();      // 判断是否有文件正在下载
    bool isShareTask();     //第一个任务是否为共享文件的任务

    // 取出第1个下载任务，如果任务队列没有任务在下载，设置第一个任务下载
    DownloadInfo* takeTask();
    // 删除下载完成的任务
    void delDownloadTask();
    // 清空下载列表
    void clearList();

private:
    DownloadTask(){}
    ~DownloadTask(){}

    // 静态数据成员，类中声明，类外必须定义
    static DownloadTask* instance;

    //它的唯一工作就是在析构函数中删除Singleton的实例
    class Garbo
    {
    public:
        ~Garbo()
        {
          if(NULL != DownloadTask::instance)
          {
            DownloadTask::instance->clearList();

            delete DownloadTask::instance;
            DownloadTask::instance = NULL;
            cout << "instance is detele";
          }
        }
    };

    //定义一个静态成员变量，程序结束时，系统会自动调用它的析构函数
    //static类的析构函数在main()退出后调用
    static Garbo temp; //静态数据成员，类中声明，类外定义

    QList<DownloadInfo*> list; //下载任务列表(任务队列)
};

#endif // DOWNLOADTASK_H
