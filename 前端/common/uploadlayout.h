#ifndef UPLOADLAYOUT_H
#define UPLOADLAYOUT_H
#include "common.h"
#include <QVBoxLayout>

// 上传进度布局类，单例模式
class UploadLayout
{
public:
    static UploadLayout* getInstance();     // 保证唯一一个实例
    void setUploadLayout(QWidget* p);       // 设置布局
    QLayout* getUploadLayout();             // 获取布局 - 添加控件

private:
    UploadLayout()
    {

    }
    ~UploadLayout()
    {

    }

    // 静态数据成员，类中声明，类外必须定义
    static UploadLayout* instance;
    QLayout* m_layout;
    QWidget* m_wg;

    //它的唯一工作就是在析构函数中删除Singleton的实例
    class Garbo{
    public:
        ~Garbo()
        {
            if(NULL != UploadLayout::instance)
            {
                delete UploadLayout::instance;
                UploadLayout::instance = NULL;
                cout << "instance is delete";
            }
        }
    };

    // 定义一个静态成员变量，程序结束时，系统会自动调用它的析构函数
    // static类的析构函数在main()退出后调用
    static Garbo temp; // 静态数据成员，类中声明，类外定义
};

#endif // UPLOADLAYOUT_H
