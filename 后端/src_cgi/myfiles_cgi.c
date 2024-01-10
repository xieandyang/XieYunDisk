/**
 * @file myfiles_cgi.c
 * @brief  用户列表展示CGI程序
 * @author Hongxing Xie
 * @version 2.0
 * @date 2023年12月18日
 */

#include "fcgi_config.h"
#include "fcgi_stdio.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "make_log.h"
#include "util_cgi.h"
#include "deal_mysql.h"
#include "cfg.h"
#include "cJSON.h"
#include <sys/time.h>

#define MYFILES_LOG_MODULE      "cgi"
#define MYFILES_LOG_PROC        "myfiles"

// mysql数据库配置信息，用户名、密码、数据库名称
static char mysql_user[128] = {0};
static char mysql_pwd[128] = {0};
static char mysql_db[128] = {0};

// 读取数据库配置信息
void read_cfg()
{
    // 读取mysql数据库配置信息
    get_cfg_value(CFG_PATH, "mysql", "user", mysql_user);
    get_cfg_value(CFG_PATH, "mysql", "password", mysql_pwd);
    get_cfg_value(CFG_PATH, "mysql", "database", mysql_db);
    LOG(MYFILES_LOG_MODULE, MYFILES_LOG_PROC, "mysql:[user=%s, pwd=%s, database=%s]\n", mysql_user, mysql_pwd, mysql_db);

    // 读取redis配置信息
}

// 通过json包获取用户名, token
int get_count_json_info(char* buf, char* user, char* token)
{
    int ret = 0;
    /*json数据如下
    {
        "token": "9e894efc0b2a898a82765d0a7f2c94cb",
        user:xxxx
    }
    */
    // 解析json包
    // 解析一个json字符串为cJSON对象
    cJSON* root = cJSON_Parse(buf);
    if(NULL == root)
    {
         LOG(MYFILES_LOG_MODULE, MYFILES_LOG_PROC, "cJSON_Parse err\n");
         ret = -1;
         goto END;
    }
    // 返回指定字符串对应的json对象
    cJSON* child1 = cJSON_GetObjectItem(root, "user");
    if(NULL == child1)
    {
        LOG(MYFILES_LOG_MODULE, MYFILES_LOG_PROC, "cJSON_GetObjectItem err\n");
        ret = -1;
        goto END;
    }
    strcpy(user, child1->valuestring); // 拷贝内容
    cJSON* child2 = cJSON_GetObjectItem(root, "token");
    if(NULL == child2)
    {
        LOG(MYFILES_LOG_MODULE, MYFILES_LOG_PROC, "cJSON_GetObjectItem err\n");
        ret = -1;
        goto END;
    }
    strcpy(token, child2->valuestring); // 拷贝内容

END:
    if(root != NULL)
    {
        cJSON_Delete(root);
        root = NULL;
    }
    return ret;
}

// 给前端反馈的消息
void return_login_status(long line, int ret)
{
    char* out = NULL;
    char* token;
    char num_buf[128] = {0};

    if(ret == 0)
    {
        token = "110"; // 成功
    }
    else
    {
        token = "111"; // 失败
    }

    // 数字
    sprintf(num_buf, "%ld", line);

    cJSON* root = cJSON_CreateObject(); //创建json项目
    cJSON_AddStringToObject(root, "num", num_buf); // {"num":"1111"}
    cJSON_AddStringToObject(root, "code", token); // {"code":"110"}
    out = cJSON_Print(root);

    cJSON_Delete(root);

    if(out != NULL)
    {
        printf(out); // 给前端反馈信息
        free(out);
    }
}

// 获取用户文件个数
void get_user_files_count(char* user, int ret)
{
    char sql_cmd[SQL_MAX_LEN] = {0};
    MYSQL* conn = NULL;
    long line = 0;
    // connect the database
    conn = mysql_conn(mysql_user, mysql_pwd, mysql_db);
    if(conn == NULL)
    {
        LOG(MYFILES_LOG_MODULE, MYFILES_LOG_PROC, "msql_conn err\n");
        goto END;
    }

    sprintf(sql_cmd, "select count from user_file_count where user=\"%s\"", user);
    char tmp[512] = {0};
    int ret2 = process_result_one(conn, sql_cmd, tmp);
    if(ret2 != 0)
    {
        LOG(MYFILES_LOG_MODULE, MYFILES_LOG_PROC, "%s 操作失败\n", sql_cmd);
        goto END;
    }
    line = atol(tmp);

END:
    if(conn != NULL)
    {
        mysql_close(conn);
        conn = NULL;
    }
    LOG(MYFILES_LOG_MODULE, MYFILES_LOG_PROC, "line = %ld\n", line);

    // 给前端反馈的消息
    return_login_status(line, ret);
}

//通过json包获取信息
int get_fileslist_json_info(char* buf, char* user, char* token, int* start, int* count)
{
    int ret = 0;
    /*json数据如下
    {
        "user": "yoyo"
        "token": xxxx
        "start": 0
        "count": 10
    }
    */
   //解析json包
    //解析一个json字符串为cJSON对象
   cJSON* root = cJSON_Parse(buf);
   if(root == NULL)
    {
        LOG(MYFILES_LOG_MODULE, MYFILES_LOG_PROC, "cJSON_Parse err\n");
        ret = -1;
        goto END;
    }
    // 返回指定字符串对应的json对象
    // user
    cJSON* child1 = cJSON_GetObjectItem(root, "user");
    if(NULL == child1)
    {
        LOG(MYFILES_LOG_MODULE, MYFILES_LOG_PROC, "cJSON_GetObjectItem err\n");
        ret = -1;
        goto END;
    }
    strcpy(user, child1->valuestring);
    // token
    cJSON* child2 = cJSON_GetObjectItem(root, "token");
    if(NULL == child2)
    {
        LOG(MYFILES_LOG_MODULE, MYFILES_LOG_PROC, "cJSON_GetObjectItem err\n");
        ret = -1;
        goto END;
    }
    strcpy(token, child2->valuestring);
    // start
    cJSON* child3 = cJSON_GetObjectItem(root, "start");
    if(NULL == child3)
    {
        LOG(MYFILES_LOG_MODULE, MYFILES_LOG_PROC, "cJSON_GetObjectItem err\n");
        ret = -1;
        goto END;
    }
    *start = child3->valueint;
    // count
    cJSON* child4 = cJSON_GetObjectItem(root, "count");
    if(NULL == child4)
    {
        
        
        ret = -1;
        goto END;
    }
    *count = child4->valueint;

END:
    if(root != NULL)
    {
        cJSON_Delete(root);
        root = NULL;
    }
    return ret;
}

//获取用户文件列表
//获取用户文件信息 localhost:8088/myfiles&cmd=normal
//按下载量升序 localhost:8088/myfiles?cmd=pvasc
//按下载量降序 localhost:8088/myfiles?cmd=pvdesc
int get_user_filelist(char* cmd, char* user, int start, int count)
{
    int ret = 0;
    char sql_cmd[SQL_MAX_LEN] = {0};
    cJSON* root = NULL;
    cJSON* array = NULL;
    char* out = NULL;
    char* out2 = NULL;
    MYSQL_RES* res_set = NULL;
    
    // connect mysql database
    MYSQL* conn = mysql_conn(mysql_user, mysql_pwd, mysql_db);
    if(conn == NULL)
    {
        LOG(MYFILES_LOG_MODULE, MYFILES_LOG_PROC, "msql_conn err\n");
        ret = -1;
        goto END;
    }

    //设置数据库编码，主要处理中文编码问题
    mysql_query(conn, "set names utf8");

    // 多表指定行范围查询
    if(strcmp(cmd, "normal") == 0) // 获取用户文件信息
    {
        // sql语句
        sprintf(sql_cmd, "select user_file_list.*, file_info.url, file_info.size, file_info.type from file_info, user_file_list where user = '%s' and file_info.md5 = user_file_list.md5 limit %d, %d", user, start, count);
    }
    else if(strcmp(cmd, "pvasc") == 0) // 按下载量升序
    {
        // sql语句
        sprintf(sql_cmd, "select user_file_list.*, file_info.url, file_info.size, file_info.type from file_info, user_file_list where user = '%s' and file_info.md5 = user_file_list.md5 order by pv asc limit %d, %d", user, start, count);
    }
    else if(strcmp(cmd, "pvdesc") == 0) // 按下载量降序
    {
        // sql语句
        sprintf(sql_cmd, "select user_file_list.*, file_info.url, file_info.size, file_info.type from file_info, user_file_list where user = '%s' and file_info.md5 = user_file_list.md5 order by pv desc limit %d, %d", user, start, count);
    }

    LOG(MYFILES_LOG_MODULE, MYFILES_LOG_PROC, "%s 在操作\n", sql_cmd);

    if(mysql_query(conn, sql_cmd) != 0)
    {
        LOG(MYFILES_LOG_MODULE, MYFILES_LOG_PROC, "%s 操作失败：%s\n", sql_cmd, mysql_error(conn));
        ret = -1;
        goto END;
    }

    /*生成结果集*/
    res_set = mysql_store_result(conn);
    if(res_set == NULL)
    {
        LOG(MYFILES_LOG_MODULE, MYFILES_LOG_PROC, "smysql_store_result error: %s!\n", mysql_error(conn));
        ret = -1;
        goto END;
    }

    // mysql_num_rows接受由mysql_store_result返回的结果结构集，并返回结构集中的行数
    long line = mysql_num_rows(res_set);
    if(line == 0) // 没有结果
    {
        LOG(MYFILES_LOG_MODULE, MYFILES_LOG_PROC, "mysql_num_rows(res_set) failed：%s\n", mysql_error(conn));
        ret = -1;
        goto END;
    }

    MYSQL_ROW row;
    root = cJSON_CreateObject();
    array = cJSON_CreateArray();
    // mysql_fetch_row从使用mysql_store_result得到的结果结构中提取一行，并把它放到一个行结构中。
    // 当数据用完或发生错误时返回NULL.
    while((row = mysql_fetch_row(res_set)) != NULL)
    {
        cJSON* item = cJSON_CreateObject();

        // mysql_num_fields获取结果中列的个数
        /*
        {
        "user": "yoyo",
        "md5": "e8ea6031b779ac26c319ddf949ad9d8d",
        "time": "2017-02-26 21:35:25",
        "filename": "test.mp4",
        "share_status": 0,
        "pv": 0,
        "url": "http://192.168.31.109:80/group1/M00/00/00/wKgfbViy2Z2AJ-FTAaM3As-g3Z0782.mp4",
        "size": 27473666,
         "type": "mp4"
        }
        */

       // user 文件所属用户
       if(row[0] != NULL)
       {
            cJSON_AddStringToObject(item, "user", row[0]);
       }
       // md5 文件md5
       if(row[1] != NULL)
       {
            cJSON_AddStringToObject(item, "md5", row[1]);
       }
       // time 文件创建时间
       if(row[2] != NULL)
       {
            cJSON_AddStringToObject(item, "time", row[2]);
       }
       // filename 文件名
       if(row[3] != NULL)
       {
            cJSON_AddStringToObject(item, "filename", row[3]);
       }
       // share_status 共享状态, 0为没有共享， 1为共享
       if(row[4] != NULL)
       {
            cJSON_AddStringToObject(item, "share_status", row[4]);
       }
       // pv 文件下载量，默认值为0，下载一次加1
       if(row[5] != NULL)
       {
            cJSON_AddStringToObject(item, "pv", row[5]);
       }
       // url 文件url
       if(row[6] != NULL)
       {
            cJSON_AddStringToObject(item, "url", row[6]);
       }
       // size 文件大小, 以字节为单位
       if(row[7] != NULL)
       {
            cJSON_AddStringToObject(item, "size", row[7]);
       }
       // type 文件类型： png, zip, mp4……
       if(row[8] != NULL)
       {
            cJSON_AddStringToObject(item, "type", row[8]);
       }

       cJSON_AddItemToArray(array, item);
    }

    cJSON_AddItemToObject(root, "files", array);

    out = cJSON_Print(root);
    LOG(MYFILES_LOG_MODULE, MYFILES_LOG_PROC, "%s\n", out);

END:
    if(ret == 0)
    {
        printf("%s", out); // 给前端反馈消息
    }
    else
    {
        // 失败
        /*
        获取用户文件列表：
            成功：文件列表json
            失败：{"code": "015"}
        */
       out = NULL;
       out2 = return_status("015");
    }
    if(out2 != NULL)
    {
        // 反馈错误码
        printf(out2);
        free(out2);
    }
    if(res_set != NULL)
    {
        //完成所有对数据的操作后，调用mysql_free_result来善后处理
        mysql_free_result(res_set);
    }
    if(conn != NULL)
    {
        mysql_close(conn);
        conn = NULL;
    }
    if(root != NULL)
    {
        cJSON_Delete(root);
    }

    if(out != NULL)
    {
        free(out);
    }
    return ret;
}

//获取用户文件数量 localhost:8088/myfiles?cmd=count
//获取用户文件信息 localhost:8088/myfiles&cmd=normal
//按下载量升序 localhost:8088/myfiles?cmd=pvasc
//按下载量降序 localhost:8088/myfiles?cmd=pvdesc
int main()
{
    // count 获取用户文件个数
    // display 获取用户文件信息，展示到前端
    char cmd[20];
    char user[USER_NAME_LEN];
    char token[TOKEN_LEN];

    // 读取数据库配置信息
    read_cfg();

    // 阻塞等待用户连接
    while(FCGI_Accept() >= 0)
    {
        LOG(MYFILES_LOG_MODULE, MYFILES_LOG_PROC, "=============\n");
        // 获取URL地址 "?" 后面的内容
        char* query = getenv("QUERY_STRING");
        LOG(MYFILES_LOG_MODULE, MYFILES_LOG_PROC, "query = %s\n", query);

        // 解析命令
        query_parse_key_value(query, "cmd", cmd, NULL);
        LOG(MYFILES_LOG_MODULE, MYFILES_LOG_PROC, "cmd = %s\n", cmd);

        char* contentLength = getenv("CONTENT_LENGTH");
        int len;

        printf("content-type: text/html\r\n\r\n");

        if(contentLength == NULL)
        {
            len = 0;
        }
        else len = atoi(contentLength);
        // 2、根据长度将post数据块读到内存
        if(len <= 0)
        {
            printf("No data from standard input.<p>\n");
            LOG(MYFILES_LOG_MODULE, MYFILES_LOG_PROC, "len = 0, No data from standard input\n");
        }
        else
        {
            char buf[4*1024] = {0};
            int ret = 0;
            ret = fread(buf, 1, len, stdin); // 从标准输入(web服务器)读取内容
            if(ret == 0)
            {
                LOG(MYFILES_LOG_MODULE, MYFILES_LOG_PROC, "fread(buf, 1, len, stdin) err\n");
                continue;
            }
            LOG(MYFILES_LOG_MODULE, MYFILES_LOG_PROC, "buf = %s\n", buf);
            if(strcmp(cmd, "count") == 0) // count 获取用户文件个数
            {
                get_count_json_info(buf, user, token); // 通过json包获取用户名, token
                // 验证登录token，成功返回0，失败返回-1
                ret = verify_token(user, token);
                // 获取用户文件个数
                get_user_files_count(user, ret);
            }
            //获取用户文件信息 localhost:8088/myfiles&cmd=normal
            //按下载量升序 localhost:8088/myfiles?cmd=pvasc
            //按下载量降序 localhost:8088/myfiles?cmd=pvdesc
            else
            {
                int start; // 文件起点
                int count; // 文件个数
                //通过json包获取信息
                get_fileslist_json_info(buf, user, token, &start, &count);
                LOG(MYFILES_LOG_MODULE, MYFILES_LOG_PROC, "user = %s, token = %s, start = %d, count = %d\n", user, token, start, count);

                // 验证登陆token，成功返回0，失败返回-1
                ret = verify_token(user, token);
                if(ret == 0)
                {
                    // 获取用户文件列表
                    get_user_filelist(cmd, user, start, count);
                }
                else
                {
                    char* out = return_status("111"); // token验证失败错误码
                    if(out != NULL)
                    {
                        printf(out); // 给前端反馈错误码
                        free(out);
                    }
                }
            }
        }
    }
    return 0;
}