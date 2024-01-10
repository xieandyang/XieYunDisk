/**
 * @file util_cgi.c
 * @brief  cgi后台通用接口
 * @author HongXing Xie
 * @version 1.0
 * @date 2023年11月10日23:38:31
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include <ctype.h>
#include "hiredis/hiredis.h"
#include "make_log.h"
#include "util_cgi.h"
#include "cfg.h"
#include "redis_op.h"
//返回前端情况，NULL代表失败, 返回的指针不为空，则需要free
char * return_status(char *status_num)
{
    char* out = NULL;
    cJSON* root = cJSON_CreateObject(); // 创建json项目
    cJSON_AddStringToObject(root, "code", status_num); // {"code":"000"}
    out = cJSON_Print(root); // cJSON to string(char *)

    cJSON_Delete(root);
    return out;
}

/**
 * @brief  去掉一个字符串两边的空白字符
 *
 * @param inbuf确保inbuf可修改
 *
 * @returns   
 *      0 成功
 *      -1 失败
 */
int trim_space(char* inbuf)
{
    int i = 0;
    int j = strlen(inbuf) - 1;

    char* str = inbuf;

    int count = 0;

    if(str == NULL)
    {
        return -1;
    }

    while(isspace(str[i]) && str[i] != '\0') i++;
    while(isspace(str[j]) && j > i) j--;

    count = j - i + 1;

    strncpy(inbuf, str + i, count);
    inbuf[count] = '\0';
    return 0;
}

/**
 * @brief  在字符串full_data中查找字符串substr第一次出现的位置
 *
 * @param full_data 	源字符串首地址
 * @param full_data_len 源字符串长度
 * @param substr        匹配字符串首地址
 *
 * @returns   
 *      成功: 匹配字符串首地址
 *      失败：NULL
 */
char* memstr(char* full_data, int full_data_len, char* substr)
{
    // 异常处理
    if(full_data == NULL || full_data_len <= 0 || substr == NULL) return NULL;
    if(*substr == '\0') return NULL;

    // 字符串匹配
    int sublen = strlen(substr);
    int i;
    char* cur = full_data;
    int last_possible = full_data_len - sublen + 1;
    for(i = 0;i < last_possible;i++)
    {
        if(*cur == *substr)
        {
            if(memcmp(cur, substr, sublen) == 0) return cur;
        }
        cur++;
    }
    return NULL;
}

/**
 * @brief  解析url query 类似 abc=123&bbb=456 字符串
 *          传入一个key,得到相应的value
 * @returns
 *          0 成功, -1 失败
 */
int query_parse_key_value(const char* query, const char* key, char* value, int* value_len_p)
{
    char* temp = NULL;
    char* end = NULL;
    int value_len = 0;

    // 查询是否存在key
    temp = strstr(query, key);
    if(temp == NULL) return -1;
    temp += strlen(key);
    temp++;

    end = temp;
    while('\0' != *end && '#' != *end && '&' != *end) end++;

    value_len = end - temp;

    strncpy(value, temp, value_len);
    value[value_len] = '\0';
    if(value_len_p != NULL) *value_len_p = value_len;
    return 0;
}

//通过文件名file_name， 得到文件后缀字符串, 保存在suffix 如果非法文件后缀,返回"null"
int get_file_suffix(const char* file_name, char* suffix)
{
    const char*p = file_name;
    int len = 0;
    const char* q = NULL;
    const char* k = NULL;

    if(p == NULL)
    {
        return -1;
    }

    q = p;
    while(*q != '\0') q++;
    k = q;
    while(*k != '.' && k != p) k--;
    if(*k == '.')
    {
        k++;
        len = q - k;

        if(len != 0)
        {
            strncpy(suffix, k, len);
            suffix[len] = '\0';
        }
        else
        {
            strncpy(suffix, "null", 5);
        }
    }
    else strncpy(suffix, "null", 5);
    return 0;
}

//字符串strSrc中的字串strFind，替换为strReplace
void str_replace(char* strSrc, char* strFind, char* strReplace)
{
    while(*strSrc != '\0')
    {
        if(*strSrc == *strFind)
        {
            if(strncmp(strSrc, strFind, strlen(strFind)) == 0)
            {
                int i = 0;
                char *q = NULL;
                char *p = NULL;
                char *repl = NULL;
                int lastLen = 0;

                i = strlen(strFind);
                q = strSrc+i;
                p = q;//p、q均指向剩余字符串的首地址
                repl = strReplace;

                while (*q++ != '\0')
                    lastLen++;
                char* temp = (char *)malloc(lastLen+1); //临时开辟一段内存保存剩下的字符串,防止内存覆盖
                int k = 0;
                for (k = 0; k < lastLen; k++)
                {
                    *(temp+k) = *(p+k);
                }
                *(temp+lastLen) = '\0';
                while (*repl != '\0')
                {
                    *strSrc++ = *repl++;
                }
                p = strSrc;
                char* pTemp = temp;//回收动态开辟内存
                while (*pTemp != '\0')
                {
                    *p++ = *pTemp++;
                }
                free(temp);
                *p = '\0';
            }
            else strSrc++;
        }
        else strSrc++;
    }
}

//验证登陆token，成功返回0，失败-1
int verify_token(char* user, char* token)
{
    int ret = 0;
    redisContext* conn = NULL;
    char temp_token[128] = {0};

    // redis服务器ip，port
    char redis_ip[30] = {0};
    char redis_port[10] = {0};
    char redis_pwd[50] = {0};

    // 读取redis配置信息
    get_cfg_value(CFG_PATH, "redis", "ip", redis_ip);
    get_cfg_value(CFG_PATH, "redis", "port", redis_port);
    get_cfg_value(CFG_PATH, "redis", "passwd", redis_pwd);

    // 连接数据库
    conn = rop_connectdb(redis_ip, redis_port, redis_pwd);
    if(conn == NULL)
    {
        LOG(UTIL_LOG_MODULE, UTIL_LOG_PROC, "redis connect error\n");
        ret = -1;
        goto END;
    }

    // 获取user对应的token
    ret = rop_get_string(conn, user, temp_token);
    if(ret == 0)
    {
        if(strcmp(token, temp_token) != 0) ret = -1;
    }

END:
    if(conn != NULL)
    {
        rop_disconnect(conn);
    }
    return ret;
}