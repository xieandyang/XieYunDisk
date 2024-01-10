/*
 * @file reg_cgi.c
 * @brief 用户注册CGI程序
 * @author Hongxing Xie
 * @version 1.0
 * @date 2023.11.24
*/

#include "fcgi_config.h"
#include "fcgi_stdio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include <mysql/mysql.h>
#include "cfg.h"
#include "deal_mysql.h"
#include <sys/time.h>
#include "util_cgi.h"
#include "make_log.h"

#define REG_LOG_MODULE       "cgi"
#define REG_LOG_PROC         "reg"

// 解析用户注册信息的json包
int get_reg_info(char* reg_buf, char* userName, char* passwd, char* nickName, char* phone, char* email)
{
    int ret = 0;
    /*json数据如下
        {
            userName:xxxx,
            passwd:xxx,
            nickName:xxx,
            phone:xxx,
            email:xxx
        }
    */

   // 解析json包
   // 解析一个json字符串为cJSON对象
   cJSON* root = cJSON_Parse(reg_buf);
   if(root == NULL)
   {
        LOG(REG_LOG_MODULE, REG_LOG_PROC, "cJSON_Parse err\n");
        ret = -1;
        goto END;
   }
   // 返回指定字符串对应的json对象 
   // 用户名
   cJSON* child1 = cJSON_GetObjectItem(root, "userName");
   if(NULL == child1)
   {
        LOG(REG_LOG_MODULE, REG_LOG_PROC, "cJSON_GetObjectItem err\n");
        ret = -1;
        goto END;
   }
   strcpy(userName, child1->valuestring); // 拷贝内容
   // 密码
   cJSON* child2 = cJSON_GetObjectItem(root, "passwd");
   if(NULL == child2)
   {
        LOG(REG_LOG_MODULE, REG_LOG_PROC, "cJSON_GetObjectItem err\n");
        ret = -1;
        goto END;
   }
   strcpy(passwd, child2->valuestring); // 拷贝内容
   // 昵称
   cJSON* child3 = cJSON_GetObjectItem(root, "nickName");
   if(NULL == child3)
   {
        LOG(REG_LOG_MODULE, REG_LOG_PROC, "cJSON_GetObjectItem err\n");
        ret = -1;
        goto END;
   }
   strcpy(nickName, child3->valuestring); // 拷贝内容
   // 电话
   cJSON* child4 = cJSON_GetObjectItem(root, "phone");
   if(NULL == child4)
   {
        LOG(REG_LOG_MODULE, REG_LOG_PROC, "cJSON_GetObjectItem err\n");
        ret = -1;
        goto END;
   }
   strcpy(phone, child4->valuestring); // 拷贝内容
   // 邮箱
   cJSON* child5 = cJSON_GetObjectItem(root, "email");
   if(NULL == child5)
   {
        LOG(REG_LOG_MODULE, REG_LOG_PROC, "cJSON_GetObjectItem err\n");
        ret = -1;
        goto END;
   }
   strcpy(email, child5->valuestring); // 拷贝内容
   
END:
    if(root != NULL)
    {
        cJSON_Delete(root);//删除json对象
        root = NULL;
    }

    return ret;
}

//注册用户，成功返回0，失败返回-1, 该用户已存在返回-2
int user_register(char* reg_buf)
{
    int ret = 0;
    MYSQL* conn = NULL;

    //获取数据库用户名、用户密码、数据库标示等信息
    char mysql_user[256] = {0};
    char mysql_passwd[256] = {0};
    char mysql_db[256] = {0};

    ret = get_mysql_info(mysql_user, mysql_passwd, mysql_db);
    if(ret != 0)
    {
        goto END;
    }
    LOG(REG_LOG_MODULE, REG_LOG_PROC, "mysql_user = %s, mysql_passwd = %s, mysql_db = %s\n", mysql_user, mysql_passwd, mysql_db);

    // 获取注册用户的信息
    char userName[128] = {0};
    char passwd[128] = {0};
    char nickName[128] = {0};
    char phone[128] = {0};
    char email[128] = {0};
    ret = get_reg_info(reg_buf, userName, passwd, nickName, phone, email);
    if(ret != 0)
    {
        goto END;
    }
    LOG(REG_LOG_MODULE, REG_LOG_PROC, "userName = %s, nickName = %s, passwd = %s, phone = %s, email = %s\n", userName, nickName, passwd, phone, email);

    // 连接数据库
    conn = mysql_conn(mysql_user, mysql_passwd, mysql_db);
    if(conn == NULL)
    {
        LOG(REG_LOG_MODULE, REG_LOG_PROC, "msql_conn err\n");
        ret = -1;
        goto END;
    }

    // 设置数据库编码，主要处理中文编码问题
    mysql_query(conn, "set names utf8;");

    char sql_cmd[SQL_MAX_LEN] = {0};
    sprintf(sql_cmd, "select * from user where name = '%s'", userName);

    // 查看该用户是否存在
    // 返回值： 0成功并保存记录集，1没有记录集，2有记录集但是没有保存，-1失败
    int ret2 = process_result_one(conn, sql_cmd, NULL); // 指向sql查询语句
    if(2 == ret2)
    {
        LOG(REG_LOG_MODULE, REG_LOG_PROC, "【%s】该用户已存在\n");
        ret = -2;
        goto END;
    }

    // 当前时间戳
    struct timeval tv;
    struct tm* ptm;
    char time_str[128];

    // 使用函数gettimeofday()函数来得到时间。它的精度可以达到微妙
    gettimeofday(&tv, NULL);
    ptm = localtime(&tv.tv_sec); // 把从1970-1-1零点零分到当前时间系统所偏移的秒数时间转换为本地时间
    // strftime() 函数根据区域设置格式化本地时间/日期，函数的功能将时间格式化，或者说格式化一个时间字符串
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", ptm);

    // sql语句，插入注册信息
    sprintf(sql_cmd, "insert into user (name, nickname, password, phone, createtime, email) values ('%s', '%s', '%s', '%s', '%s', '%s')", userName, nickName, passwd, phone, time_str, email);
    if(mysql_query(conn, sql_cmd) != 0)
    {
        LOG(REG_LOG_MODULE, REG_LOG_PROC, "%s 插入失败：%s\n", sql_cmd, mysql_error(conn));
        ret = -1;
        goto END;
    }

END:
    if(conn != NULL)
    {
        mysql_close(conn); //断开数据库连接
    }

    return ret;
}

int main()
{
     // 阻塞等待用户连接
    while(FCGI_Accept() >= 0)
    {
        // 1、根据content-length得到post数据块的长度
        char *contentLength = getenv("CONTENT_LENGTH");
        int len;
        printf("Content-type: text/html\r\n\r\n");
        // printf("content-type: application/json\r\n");
        if(contentLength == NULL) len = 0;
        else len = atoi(contentLength); // 字符串转int
        // 2、根据长度将post数据块读到内存
        if(len <= 0) // 没有用户登录信息
        {
            printf("No data from standard input.<p>\n");
            LOG(REG_LOG_MODULE, REG_LOG_PROC, "len = 0, No data from standard input\n");
            continue;
        }
        // 获取用户登录信息
        char buf[4 * 1024] = {0};
        int ret = 0;
        char* out = NULL;
        ret = fread(buf, 1, len, stdin); // 从标准输入(web服务器)读取内容
        if(ret == 0)
        {
            LOG(REG_LOG_MODULE, REG_LOG_PROC, "fread(buf, 1, len, stdin) err\n");
            continue;
        }
        LOG(REG_LOG_MODULE, REG_LOG_PROC, "buf = %s\n", buf);
        // 3、解析json对象，得到用户名、密码、昵称、邮箱、手机号
        // 4、连接数据库 - mysql,oracle
        // 5、查询，看看有没有用户名冲突
        // 6、有冲突 -> 注册失败，通知客户端 -> {"code":"003"}
        // 7、没有冲突 -> 用户数据插入到数据库中
        // 8、成功 -> 通知客户端 -> {"code":"002"}
        /*
            注册：
                成功：{"code":"002"}
                该用户已存在：{"code":"003"}
                失败：{"code":"004"}
            */
        ret = user_register(buf);
        if(ret == 0)
        {
             // 登陆成功
             // 返回前端注册情况， 002代表成功
             out = return_status("002");
        }
        else if(ret == -1)
        {
             // 返回前端注册情况, 004代表失败
             out = return_status("004");
        }
        else if(ret == -2)
        {
             // 返回前端注册情况， 003代表该用户已存在
             out = return_status("003");
        }
        // 9、通知客户端回传的字符串的格式
        // printf("content-type: application/json\r\n");
        if(out != NULL)
        {
             printf(out); // 给前端反馈信息
             free(out); // 释放
        }
    }
    return 0;
}