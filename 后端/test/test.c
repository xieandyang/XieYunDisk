// gcc -o main test.c ../common/base64.c -I ../include/

#include "base64.h"
#include <string.h>
#include <stdio.h>

int main()
{
    char test[4] = "tes"; // 011101 000110 010101 110011
    char res[1024];
    memset(res, 0x00, sizeof(res));
    base64_encode(test, 3, res);
    printf("%s\n", res);
    return 0;
}