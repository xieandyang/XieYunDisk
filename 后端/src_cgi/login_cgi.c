/*
 * @file login_cgi.c
 * @brief 登陆后台CGI程序
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
#include "redis_op.h"
#include "base64.h"
#include "md5.h"
#include "des.h"
#include "make_log.h"
#include "util_cgi.h"
#include <time.h>

#define LOGIN_LOG_MODULE "cgi"
#define LOGIN_LOG_PROC   "login"

// 解析用户登陆信息的json包
int get_login_info(char* login_buf, char* userName, char* passwd)
{
    int ret = 0;
    /*json数据如下
        {
            userName:xxxx,
            passwd:xxx,
        }
    */

   // 解析json包
   // 解析一个json字符串为cJSON对象
   cJSON* root = cJSON_Parse(login_buf);
   if(root == NULL)
   {
        LOG(LOGIN_LOG_MODULE, LOGIN_LOG_PROC, "cJSON_Parse err\n");
        ret = -1;
        goto END;
   }
   // 返回指定字符串对应的json对象 
   // 用户名
   cJSON* child1 = cJSON_GetObjectItem(root, "userName");
   if(NULL == child1)
   {
        LOG(LOGIN_LOG_MODULE, LOGIN_LOG_PROC, "cJSON_GetObjectItem err\n");
        ret = -1;
        goto END;
   }
   strcpy(userName, child1->valuestring); // 拷贝内容
   // 密码
   cJSON* child2 = cJSON_GetObjectItem(root, "passwd");
   if(NULL == child2)
   {
        LOG(LOGIN_LOG_MODULE, LOGIN_LOG_PROC, "cJSON_GetObjectItem err\n");
        ret = -1;
        goto END;
   }
   strcpy(passwd, child2->valuestring); // 拷贝内容
   
END:
    if(root != NULL)
    {
        cJSON_Delete(root);//删除json对象
        root = NULL;
    }

    return ret;
}

/* -------------------------------------------*/
/**
 * @brief  判断用户登陆情况
 *
 * @param user 		用户名
 * @param pwd 		密码
 *
 * @returns
 *      成功: 0
 *      失败：-1
 */
 /* -------------------------------------------*/
int check_user_pwd(char* login_buf)
{
    int ret = 0;
    MYSQL* conn = NULL;

    //获取数据库用户名、用户密码、数据库标示等信息
    char mysql_user[256] = {0};
    char mysql_passwd[256] = {0};
    char mysql_db[256] = {0};

    ret = get_mysql_info(mysql_user, mysql_passwd, mysql_db);
    LOG(LOGIN_LOG_MODULE, LOGIN_LOG_PROC, "mysql_user = %s, mysql_pwd = %s, mysql_db = %s\n", mysql_user, mysql_passwd, mysql_db);
    if(ret != 0)
    {
        goto END;
    }

    // 获取登陆用户的信息
    char userName[128] = {0};
    char passwd[128] = {0};
    ret = get_login_info(login_buf, userName, passwd);
    if(ret != 0)
    {
        LOG(LOGIN_LOG_MODULE, LOGIN_LOG_PROC, "get_login_info error!!!\n");
        goto END;
    }

    // 连接数据库
    conn = mysql_conn(mysql_user, mysql_passwd, mysql_db);
    if(conn == NULL)
    {
        LOG(LOGIN_LOG_MODULE, LOGIN_LOG_PROC, "mysql_conn error!!!\n");
        ret = -1;
        goto END;
    }

    // 设置数据库编码，主要处理中文编码问题
    mysql_query(conn, "set names utf8;");

    char sql_cmd[SQL_MAX_LEN] = {0};
    sprintf(sql_cmd, "select * from user where name = '%s' and password = '%s'", userName, passwd);

    // 查看该用户是否存在
    // 返回值： 0成功并保存记录集，1没有记录集，2有记录集但是没有保存，-1失败
    int ret2 = process_result_one(conn, sql_cmd, NULL); // 指向sql查询语句
    if(2 == ret2)
    {
        LOG(LOGIN_LOG_MODULE, LOGIN_LOG_PROC, "%s 查询成功\n", sql_cmd);
        ret = 0;
        goto END;
    }
    else ret = -1;

END:
    if(conn != NULL)
    {
        mysql_close(conn); //断开数据库连接
    }

    return ret;
}

/* -------------------------------------------*/
/**
 * @brief  生成token字符串, 保存redis数据库
 *
 * @param user 		用户名
 * @param token     生成的token字符串
 *
 * @returns
 *      成功: 0
 *      失败：-1
 */
 /* -------------------------------------------*/
 int set_token(char *user, char *token)
 {
    int ret = 0;
    //redis 服务器ip、端口
    char redis_ip[30] = {0};
    char redis_port[10] = {0};
    char redis_pwd[30] = {0};

    // 读取redis配置信息
    get_cfg_value(CFG_PATH, "redis", "ip", redis_ip);
    get_cfg_value(CFG_PATH, "redis", "port", redis_port);
    get_cfg_value(CFG_PATH, "redis", "passwd", redis_pwd);
    LOG(LOGIN_LOG_MODULE, LOGIN_LOG_PROC, "redis:[ip=%s,port=%s]\n", redis_ip, redis_port);

    //连接redis数据库
    redisContext* conn = rop_connectdb(redis_ip, redis_port, redis_pwd);
    if(conn == NULL)
    {
        LOG(LOGIN_LOG_MODULE, LOGIN_LOG_PROC, "redis connected error\n");
        ret = -1;
        goto END;
    }

    //产生4个1000以内的随机数
    int rand_num[4] = {0};
    int i = 0;
    //设置随机种子
    srand((unsigned int)time(NULL));
    for(i = 0; i < 4; ++i)
    {
        rand_num[i] = rand()%1000;//随机数
    }

    char tmp[1024] = {0};
    sprintf(tmp, "%s%d%d%d%d", user, rand_num[0], rand_num[1], rand_num[2], rand_num[3]);
    LOG(LOGIN_LOG_MODULE, LOGIN_LOG_PROC, "tmp = %s\n", tmp);

    // 加密
    char enc_tmp[1024*2] = {0};
    int enc_len = 0;
    ret = DesEnc((unsigned char*)tmp, strlen(tmp), (unsigned char*)enc_tmp, &enc_len);
    if(ret != 0)
    {
        LOG(LOGIN_LOG_MODULE, LOGIN_LOG_PROC, "DesEnc error\n");
        ret = -1;
        goto END;
    }

    // to base64
    char base64[1024*3] = {0};
    base64_encode((const unsigned char*)enc_tmp, enc_len, base64);
    LOG(LOGIN_LOG_MODULE, LOGIN_LOG_PROC, "base64 = %s\n", base64);

    // to md5
    MD5_CTX md5;
    MD5Init(&md5);
    unsigned char decrypt[16];
    MD5Update(&md5, (unsigned char*)base64, strlen(base64));
    MD5Final(&md5, decrypt);

    char str[100] = {0};
    for(i = 0;i < 16;i++)
    {
        sprintf(str, "%02x", decrypt[i]);
        strcat(token, str);
    }

    LOG(LOGIN_LOG_MODULE, LOGIN_LOG_PROC, "token OK \n");

    // redis保存此字符串，用户名：token, 有效时间为24小时
    ret = rop_setex_string(conn, user, 86400, token);

    LOG(LOGIN_LOG_MODULE, LOGIN_LOG_PROC, "ret == %d \n", ret);

END:
    if(conn != NULL)
    {
        rop_disconnect(conn);
        conn = NULL;
    }
    return ret;
 }

 // 返回前端情况
 void return_login_status(char* status_num, char* token)
 {
    char* out = NULL;
    cJSON* root= cJSON_CreateObject(); // 创建json项目
    cJSON_AddStringToObject(root, "code", status_num);
    cJSON_AddStringToObject(root, "token", token);
    out = cJSON_Print(root);

    cJSON_Delete(root);

    if(out != NULL)
    {
        printf(out); // 发送给前端
        free(out);
    }
 }

int main()
{
    while(FCGI_Accept() >= 0)
    {
        // 1、根据content-length得到post数据块的长度
        char* contentlength = getenv("CONTENT_LENGTH");
        int len;
        char token[128] = {0};
        printf("content-type: text/html\r\n\r\n");
        if(contentlength == NULL) len = 0;
        else len = atoi(contentlength);
        // 2、根据长度将post数据块读到内存
        if(len <= 0) // 没有登录信息
        {
            printf("No data from standard input.<p>\n");
            LOG(LOGIN_LOG_MODULE, LOGIN_LOG_PROC, "len = 0, No data from standard input\n");
        }
        else // 获取登录用户
        {
            char buf[4 * 1024] = {0};
            int ret = 0;
            ret = fread(buf, 1, len, stdin); // 从标准输入(web服务器)读取内容
            if(ret == 0)
            {
                LOG(LOGIN_LOG_MODULE, LOGIN_LOG_PROC, "fread(buf, 1, len, stdin) err\n");
                continue;
            }
            LOG(LOGIN_LOG_MODULE, LOGIN_LOG_PROC, "buf = %s\n", buf);

            // 3、解析json对象，得到用户名、密码
            char user[512] = {0};
            char pwd[512] = {0};
            get_login_info(buf, user, pwd);
            LOG(LOGIN_LOG_MODULE, LOGIN_LOG_PROC, "user = %s, pwd = %s\n", user, pwd);
            // 4、连接数据库 - mysql,oracle
            // 5、查询，查看用户名密码是否正确
            ret = check_user_pwd(buf);
            /*
                    登录：
                        成功：{"code":"000", "token":token}
                        失败：{"code":"001", "token":"fail"}
            */
            // 6、错误 -> 登陆失败，通知客户端 -> {"code":"001", "token":"fail"}
            if(ret == -1)
            {
                return_login_status("001", "fail");
            }
            // 7、正确 -> 登录成功，通知客户端 -> {"code":"000", "token":token}
            else if(ret == 0)
            {
                // 7.1 生成token
                memset(token, 0x00, sizeof(token));
                ret = set_token(user, token);
                LOG(LOGIN_LOG_MODULE, LOGIN_LOG_PROC, "token = %s\n", token);
                return_login_status("000", token);
            }
        }
    }
    return 0;
}