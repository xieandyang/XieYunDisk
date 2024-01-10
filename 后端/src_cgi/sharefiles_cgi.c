/**
 * @file sharefiles_cgi.c
 * @brief  共享文件列表展示CGI程序
 * @author Hongxing Xie
 * @version 2.0
 * @date 2023年12月24日
*/

#include "fcgi_config.h"
#include "fcgi_stdio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "make_log.h" //日志头文件
#include "util_cgi.h"
#include "deal_mysql.h"
#include "redis_op.h"
#include "redis_keys.h"
#include "cfg.h"
#include "cJSON.h"
#include <sys/time.h>

#define SHAREFILES_LOG_MODULE       "cgi"
#define SHAREFILES_LOG_PROC         "sharefiles"

// mysql数据库配置信息，用户名、密码、数据库名称
static char mysql_user[128] = {0};
static char mysql_pwd[128] = {0};
static char mysql_db[128] = {0};

// redis 服务器ip，端口
static char redis_ip[30] = {0};
static char redis_port[10] = {0};
static char redis_pwd[128] = {0};

// 读取数据库配置信息
void read_cfg()
{
    // 读取mysql数据库配置信息
    get_cfg_value(CFG_PATH, "mysql", "user", mysql_user);
    get_cfg_value(CFG_PATH, "mysql", "password", mysql_pwd);
    get_cfg_value(CFG_PATH, "mysql", "database", mysql_db);
    LOG(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, "mysql:[user=%s, pwd=%s, database=%s]\n", mysql_user, mysql_pwd, mysql_db);

    // 读取redis配置信息
    get_cfg_value(CFG_PATH, "redis", "ip", redis_ip);
    get_cfg_value(CFG_PATH, "redis", "port", redis_port);
    get_cfg_value(CFG_PATH, "redis", "passwd", redis_pwd);
    LOG(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, "redis:[ip=%s,port=%s]\n", redis_ip, redis_port);
}

void get_share_files_count() // 获取共享文件个数
{
    char sql_cmd[SQL_MAX_LEN] = {0};
    MYSQL* conn = NULL;
    long line = 0;
    // connect the database
    conn = mysql_conn(mysql_user, mysql_pwd, mysql_db);
    if(conn == NULL)
    {
        LOG(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, "msql_conn err\n");
        goto END;
    }

    sprintf(sql_cmd, "select count from user_file_count where user=\"%s\"", "xxx_share_xxx_file_xxx_list_xxx_count_xxx");
    char tmp[512] = {0};
    int ret2 = process_result_one(conn, sql_cmd, tmp);
    if(ret2 != 0)
    {
        LOG(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, "%s 操作失败\n", sql_cmd);
        goto END;
    }
    line = atol(tmp);

END:
    if(conn != NULL)
    {
        mysql_close(conn);
        conn = NULL;
    }
    LOG(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, "line = %ld\n", line);

    // 给前端反馈的消息
    printf("%ld", line);
}

//通过json包获取信息
int get_fileslist_json_info(char* buf, int* start, int* count)
{
    int ret = 0;
    /*json数据如下
    {
        "start": 0
        "count": 10
    }
    */
    //解析json包
    //解析一个json字符串为cJSON对象
    cJSON* root = cJSON_Parse(buf);
    if(root == NULL)
    {
        LOG(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, "cJSON_Parse err\n");
        ret = -1;
        goto END;
    }
    // 返回指定字符串对应的json对象
    // start
    cJSON* child1 = cJSON_GetObjectItem(root, "start");
    if(NULL == child1)
    {
        LOG(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, "cJSON_GetObjectItem err\n");
        ret = -1;
        goto END;
    }
    *start = child1->valueint;

    // count
    cJSON* child2 = cJSON_GetObjectItem(root, "count");
    if(NULL == child2)
    {
        LOG(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, "cJSON_GetObjectItem err\n");
        ret = -1;
        goto END;
    }
    *count = child2->valueint;

END:
    if(root != NULL)
    {
        cJSON_Delete(root);
        root = NULL;
    }
    return ret;
}

// 获取共享文件列表
// 获取用户文件信息 127.0.0.1:80/sharefiles&cmd=normal
int get_share_filelist(int start, int count)
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
        LOG(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, "msql_conn err\n");
        ret = -1;
        goto END;
    }

    //设置数据库编码，主要处理中文编码问题
    mysql_query(conn, "set names utf8");

    // sql语句
    sprintf(sql_cmd, "select share_file_list.*, file_info.url, file_info.size, file_info.type from file_info, share_file_list where file_info.md5 = share_file_list.md5 limit %d, %d", start, count);

    LOG(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, "%s 在操作\n", sql_cmd);

    if(mysql_query(conn, sql_cmd) != 0)
    {
        LOG(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, "%s 操作失败：%s\n", sql_cmd, mysql_error(conn));
        ret = -1;
        goto END;
    }

    /*生成结果集*/
    res_set = mysql_store_result(conn);
    if(res_set == NULL)
    {
        LOG(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, "smysql_store_result error: %s!\n", mysql_error(conn));
        ret = -1;
        goto END;
    }

    // mysql_num_rows接受由mysql_store_result返回的结果结构集，并返回结构集中的行数
    long line = mysql_num_rows(res_set);
    if(line == 0) // 没有结果
    {
        LOG(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, "mysql_num_rows(res_set) failed：%s\n", mysql_error(conn));
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

        //mysql_num_fields获取结果中列的个数
        /*for(i = 0; i < mysql_num_fields(res_set); i++)
        {
            if(row[i] != NULL)
            {

            }
        }*/

        /*
        {
        "user": "yoyo",
        "md5": "e8ea6031b779ac26c319ddf949ad9d8d",
        "time": "2017-02-26 21:35:25",
        "filename": "test.mp4",
        "share_status": 1,
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
    LOG(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, "%s\n", out);

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

// 获取共享文件排行版
// 按下载量降序127.0.0.1:80/sharefiles?cmd=pvdesc
int get_ranking_filelist(int start, int count)
{
    /*
    a) mysql共享文件数量和redis共享文件数量对比，判断是否相等
    b) 如果不相等，清空redis数据，从mysql中导入数据到redis (mysql和redis交互)
    c) 从redis读取数据，给前端反馈相应信息
    */

    int ret = 0;
    char sql_cmd[SQL_MAX_LEN] = {0};
    cJSON* root = NULL;
    cJSON* array = NULL;
    RVALUES value = NULL;
    char* out = NULL;
    char* out2 = NULL;
    MYSQL_RES* res_set = NULL;
    int ret2 = 0;
    char tmp[512] = {0};
    redisContext* redis_conn = NULL;
    
    //连接redis数据库
    redis_conn = rop_connectdb(redis_ip, redis_port, redis_pwd);
    if (redis_conn == NULL)
    {
        LOG(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, "redis connected error");
        ret = -1;
        goto END;
    }

    // connect mysql database
    MYSQL* conn = mysql_conn(mysql_user, mysql_pwd, mysql_db);
    if(conn == NULL)
    {
        LOG(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, "msql_conn err\n");
        ret = -1;
        goto END;
    }

    //设置数据库编码，主要处理中文编码问题
    mysql_query(conn, "set names utf8");

    sprintf(sql_cmd, "select count from user_file_count where user = 'xxx_share_xxx_file_xxx_list_xxx_count_xxx'");
    //返回值： 0成功并保存记录集，1没有记录集，2有记录集但是没有保存，-1失败
    ret2 = process_result_one(conn, sql_cmd, tmp);
    if(ret2 != 0)
    {
        LOG(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, "%s 操作失败\n", sql_cmd);
        ret = -1;
        goto END;
    }

    int sql_num = atoi(tmp);

    //===2、redis共享文件数量
    int redis_num = rop_zset_zcard(redis_conn, FILE_PUBLIC_ZSET);
    if(redis_num == -1)
    {
        LOG(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, "rop_zset_zcard 操作失败\n");
        ret = -1;
        goto END;
    }

    LOG(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, "sql_num = %d, redis_num = %d\n", sql_num, redis_num);

    //===3、mysql共享文件数量和redis共享文件数量对比，判断是否相等
    if(sql_num != redis_num)
    {
        //===4、如果不相等，清空redis数据，重新从mysql中导入数据到redis (mysql和redis交互)
        //a) 清空redis有序数据
        rop_del_key(redis_conn, FILE_PUBLIC_ZSET);
        rop_del_key(redis_conn, FILE_NAME_HASH);

        //b) 从mysql中导入数据到redis
        //sql语句
        sprintf(sql_cmd, "select md5, filename, pv, from share_file_list order by pv desc");
        LOG(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, "%s 在操作\n", sql_cmd);
        if (mysql_query(conn, sql_cmd) != 0)
        {
            LOG(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, "%s 操作失败: %s\n", sql_cmd, mysql_error(conn));
            ret = -1;
            goto END;
        }

        /*生成结果集*/
        res_set = mysql_store_result(conn);
        if(res_set == NULL)
        {
            LOG(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, "smysql_store_result error: %s!\n", mysql_error(conn));
            ret = -1;
            goto END;
        }

        // mysql_num_rows接受由mysql_store_result返回的结果结构集，并返回结构集中的行数
        long line = mysql_num_rows(res_set);
        if(line == 0) // 没有结果
        {
            LOG(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, "mysql_num_rows(res_set) failed：%s\n", mysql_error(conn));
            ret = -1;
            goto END;
        }

        MYSQL_ROW row;
        // mysql_fetch_row从使用mysql_store_result得到的结果结构中提取一行，并把它放到一个行结构中。
        // 当数据用完或发生错误时返回NULL.
        while((row = mysql_fetch_row(res_set)) != NULL)
        {
            // md5, filename, pv
            if(row[0] == NULL || row[1] == NULL || row[2] == NULL)
            {
                LOG(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, "mysql_fetch_row(res_set)) failed\n");
                ret = -1;
                goto END;
            }

            char fileid[1024] = {0};
            sprintf(fileid, "%s%s", row[0], row[1]); // 文件标示，md5+文件名

            // 增加有序集合成员
            rop_zset_add(redis_conn, FILE_PUBLIC_ZSET, atoi(row[2]), fileid);

            // 增加hash记录
            rop_hash_set(redis_conn, FILE_NAME_HASH, fileid, row[1]);
        }
    }

    //===5、从redis读取数据，给前端反馈相应信息
    value = (RVALUES)calloc(count, VALUES_ID_SIZE); // 堆区请求空间
    if(value == NULL)
    {
        ret = -1;
        goto END;
    }

    int n = 0;
    int end = start + count - 1; // 加载资源的结束位置
    // 降序获取有序集合的元素
    ret = rop_zset_zrevrange(redis_conn, FILE_PUBLIC_ZSET, start, end, value, &n);
    if(ret != 0)
    {
        LOG(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, "rop_zset_zrevrange 操作失败\n");
        goto END;
    }

    root = cJSON_CreateObject();
    array = cJSON_CreateArray();
    // 遍历元素个数
    for(int i = 0;i < n;i++)
    {
        cJSON* item = cJSON_CreateObject();
        /*
        {
            "filename": "test.mp4",
            "pv": 0
        }
        */
        //-- filename 文件名字
        char filename[1024] = {0};
        ret = rop_hash_get(redis_conn, FILE_NAME_HASH, value[i], filename);
        if(ret != 0)
        {
            LOG(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, "rop_hash_get 操作失败\n");
            ret = -1;
            goto END;
        }
        cJSON_AddStringToObject(item, "filename", filename);

        //-- pv 文件下载量
        int score = rop_zset_get_score(redis_conn, FILE_PUBLIC_ZSET, value[i]);
        if(score == -1)
        {
            LOG(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, "rop_zset_get_score 操作失败\n");
            ret = -1;
            goto END;
        }
        cJSON_AddNumberToObject(item, "pv", score);

        cJSON_AddItemToArray(array, item);
    }

    cJSON_AddItemToObject(root, "files", array);

    out = cJSON_Print(root);

    LOG(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, "%s\n", out);

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
    if(redis_conn != NULL)
    {
        rop_disconnect(redis_conn);
        redis_conn = NULL;
    }
    if(conn != NULL)
    {
        mysql_close(conn);
        conn = NULL;
    }
    if(value != NULL)
    {
        free(value);
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

int main()
{
    char cmd[20] = {0};

    //读取数据库配置信息
    read_cfg();

    while(FCGI_Accept() >= 0)
    {
        // 获取URL地址 "?" 后面的内容
        char* query = getenv("QUERY_STRING");
        // 解析命令
        query_parse_key_value(query, "cmd", cmd, NULL);
        LOG(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, "cmd = %s\n", cmd);

        printf("content-type: text/html\r\n\r\n");

        if(strcmp(cmd, "count") == 0) // count 获取用户文件个数
        {
            get_share_files_count(); // 获取共享文件个数
        }
        else
        {
            char* contentLength = getenv("CONTENT_LENGTH");
            int len;

            if(contentLength == NULL)
            {
                len = 0;
            }
            else len = atoi(contentLength);
            // 2、根据长度将post数据块读到内存
            if(len <= 0)
            {
                printf("No data from standard input.<p>\n");
                LOG(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, "len = 0, No data from standard input\n");
            }
            else
            {
                char buf[4*1024] = {0};
                int ret = 0;
                ret = fread(buf, 1, len, stdin); // 从标准输入(web服务器)读取内容
                if(ret == 0)
                {
                    LOG(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, "fread(buf, 1, len, stdin) err\n");
                    continue;
                }
                LOG(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, "buf = %s\n", buf);
                
                //获取共享文件信息 127.0.0.1:80/sharefiles&cmd=normal
                //按下载量升序 127.0.0.1:80/sharefiles?cmd=pvasc
                //按下载量降序127.0.0.1:80/sharefiles?cmd=pvdesc
                int start; // 文件起点
                int count; // 文件个数
                //通过json包获取信息
                get_fileslist_json_info(buf, &start, &count);
                LOG(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, "start = %d, count = %d\n", start, count);

                if(strcmp(cmd, "normal") == 0)
                {
                    get_share_filelist(start, count); // 获取共享文件列表
                }
                else if(strcmp(cmd, "pvdesc") == 0)
                {
                    get_ranking_filelist(start, count); // 获取共享文件排行版
                }
            }
        }
    }
    return 0;
}