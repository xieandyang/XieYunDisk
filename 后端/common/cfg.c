/**
 * @file cfg.c
 * @brief  读取配置文件信息
 * @author Hongxing Xie
 * @version 1.0
 * @date 2023.11.25
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include "cfg.h"
#include "make_log.h"

/* -------------------------------------------*/
/**
 * @brief  从配置文件中得到相对应的参数
 *
 * @param profile   配置文件路径
 * @param title      配置文件title名称[title]
 * @param key       key
 * @param value    (out)  得到的value
 *
 * @returns
 *      0 succ, -1 fail
 */
/* -------------------------------------------*/
int get_cfg_value(const char *profile, char *title, char *key, char *value)
{
    int ret = 0;
    char* buf = NULL;
    FILE* fp = NULL;

    // 异常处理
    if(profile == NULL || title == NULL || key == NULL || value == NULL)
    {
        return -1;
    }

    // 只读方式打开文件
    fp = fopen(profile, "rb");
    if(fp == NULL) // 打开文件失败
    {
        perror("fopen error: ");
        LOG(CFG_LOG_MODULE, CFG_LOG_PROC, "fopen err\n");
        ret = -1;
        goto END;
    }

    fseek(fp, 0, SEEK_END); // 光标移动到文件末尾
    long size = ftell(fp); // 获取文件大小
    fseek(fp, 0, SEEK_SET); // 光标移动到文件开头

    buf = (char *)calloc(1, size + 1); // 动态分配空间
    if(buf == NULL) // 分配空间失败
    {
        perror("calloc error: ");
        LOG(CFG_LOG_MODULE, CFG_LOG_PROC, "calloc err\n");
        ret = -1;
        goto END;
    }

    // 读取文件内容
    fread(buf, 1, size, fp);

    // 解析一个json字符串为cJSON对象
    cJSON* root = cJSON_Parse(buf);
    if(NULL == root) // 解析失败
    {
        LOG(CFG_LOG_MODULE, CFG_LOG_PROC, "root err\n");
        ret = -1;
        goto END;
    }

    // 返回指定字符串对应的json对象
    cJSON* father = cJSON_GetObjectItem(root, title);
    if(NULL == father)
    {
        LOG(CFG_LOG_MODULE, CFG_LOG_PROC, "father err\n");
        ret = -1;
        goto END;
    }
    cJSON* son = cJSON_GetObjectItem(father, key);
    if(NULL == son)
    {
        LOG(CFG_LOG_MODULE, CFG_LOG_PROC, "son err\n");
        ret = -1;
        goto END;
    }
    strcpy(value, son->valuestring);

    cJSON_Delete(root); // 删除json对象

END:
    if(fp != NULL)
    {
        fclose(fp);
    }

    if(buf != NULL)
    {
        free(buf);
    }

    return ret;
}

//获取数据库用户名、用户密码、数据库标示等信息
int get_mysql_info(char *mysql_user, char *mysql_pwd, char *mysql_db)
{
    if(-1 == get_cfg_value(CFG_PATH, "mysql", "user", mysql_user))
    {
        LOG(CFG_LOG_MODULE, CFG_LOG_PROC, "mysql_user err\n");
        return -1;
    }
    if(-1 == get_cfg_value(CFG_PATH, "mysql", "password", mysql_pwd))
    {
        LOG(CFG_LOG_MODULE, CFG_LOG_PROC, "mysql_pwd err\n");
        return -1;
    }
    if(-1 == get_cfg_value(CFG_PATH, "mysql", "database", mysql_db))
    {
        LOG(CFG_LOG_MODULE, CFG_LOG_PROC, "mysql_db err\n");
        return -1;
    }
    return 0;
}