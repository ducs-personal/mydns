/*
 * hash_1.c
 *
 *  Created on: Apr 17, 2015
 *      Author: root
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#define SEED (131313)

char ip[20][32];
char domain[20][32];
char list[20][32];

/**
 * www.tmall.com  111.111,111,111
 * www.tmall.co    222,222,222,222
 * www.tmall.comnn 333,333,333,333
 * www.gold2.hao61.net 444,444,444,444
 * www.google.com.cn 555,555,555,555
 * www.google.com   666.666.666.666
 * www.google.hk    777,777,777,777
 */
void init_data()
{
    memset(&ip, 0, sizeof(ip));
    printf("ip size:%ld\n", sizeof(ip));
    strcpy(ip[0], "1111111111111111111");
    strcpy(ip[1], "2222222222222222222");
    strcpy(ip[2], "3333333333333333333");
    strcpy(ip[3], "4444444444444444444");
    strcpy(ip[4], "5555555555555555555");
    strcpy(ip[5], "6666666666666666666");
    strcpy(ip[6], "7777777777777777777");

    memset(&domain, 0, sizeof(ip));
    printf("domain size:%ld\n", sizeof(domain));
    strcpy(domain[0], "11www.tmall.com.cn");
    strcpy(domain[1], "22www.tmall.com.com");
    strcpy(domain[2], "33www.tmall.com.co");
    strcpy(domain[3], "44www.tmall.com.comm");
    strcpy(domain[4], "55www.tmall.com.comn");
    strcpy(domain[5], "66www.tmall.com.cnno");
    strcpy(domain[6], "77www.tmall.com.cnnm");

}

size_t BKDRHash(const char *str)
{
    size_t hash = 0;
    size_t ch = 0;
    char *tmp = str;
    while (*tmp)
    {
        hash = hash * SEED + (size_t) *tmp++;          // 也可以乘以31、131、1313、13131、131313..

    }
    printf("%s:[%zu]-->[%d]\n", str, hash, (hash) % 19);
    return (hash % 19);
}

int python_hash(char *str, long length)
{

    char * p = str;
    long x = *p << 7;
    while (--length >= 0)
    {
        x = (1000003 * x) ^ *p++;
    }
    x ^= length;
    if (x == -1) x = -2;

    printf("---------------------------------------------------->%s:[%zu]-->[%d]\n", str, x, (x) % 19);
    return x;
}

/**
 * result: use first one
 *list size:160
 www.tmall.com.cn:[8062533816127361497]-->[12]
 ---------------------------------------------------->www.tmall.com.cn:[7188697127810624370]-->[3]
 www.tmall.com.com:[16122546339652571015]-->[8]
 ---------------------------------------------------->www.tmall.com.com:[4081633321829566262]-->[16]
 www.tmall.com.co:[8062533816127361498]-->[13]
 ---------------------------------------------------->www.tmall.com.co:[7188697127810624371]-->[4]
 www.tmall.com.comm:[10487891418970039252]-->[6]
 ---------------------------------------------------->www.tmall.com.comm:[8292516114103832841]-->[16]
 www.tmall.com.comn:[10487891418970039253]-->[7]
 ---------------------------------------------------->www.tmall.com.comn:[8292516114103832842]-->[17]
 www.tmall.com.cnno:[10487891418968316598]-->[6]
 ---------------------------------------------------->www.tmall.com.cnno:[8292515114228833355]-->[17]
 www.tmall.com.cnnm:[10487891418968316596]-->[4]
 ---------------------------------------------------->www.tmall.com.cnnm:[8292515114228833353]-->[15]
 *
 */

int hash_map_init()
{
    int i = 0;
    int pos = 0;
    memset(&list, '\0', sizeof(list));
    for (i = 0; i < 7; i++)
    {
        strcpy(list[BKDRHash(domain[i])], ip[i]);
    }
}

int search_hash(char *str)
{
    int index = BKDRHash(str);
    printf("search:[%d]---->[%s]\n", index, list[index]);
    return 0;
}

int main(int argc, char **argv)
{
    int i = 7;

    if (argc < 2)
    {
        printf("Usage...\n");
        exit(0);
    }
    init_data();
    hash_map_init();
    search_hash(argv[1]);

    return 0;
}
