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

#define SEED (131333)

char domain[19][32];
char ip[19][32];
char list[19][32];

void init_data()
{
    memset(&ip, 0, sizeof(ip));
    strcpy(ip[0], "183.81.183.236");
    strcpy(ip[1], "216.58.221.100");
    strcpy(ip[2], "203.208.50.145");
    strcpy(ip[3], "4444444444444444444");
    strcpy(ip[4], "5555555555555555555");
    strcpy(ip[5], "6666666666666666666");
    strcpy(ip[6], "7777777777777777777");

    memset(&domain, 0, sizeof(domain));
    strcpy(domain[0], "gold2.hao61.net");
    strcpy(domain[1], "www.google.com");
    strcpy(domain[2], "www.google.com.cn");
    strcpy(domain[3], "44www.tmall.com.comm");
    strcpy(domain[4], "55www.tmall.com.comn");
    strcpy(domain[5], "66www.tmall.com.cnno");
    strcpy(domain[6], "77www.tmall.com.cnnm");

    memset(&list, '\0', sizeof(list));
}

size_t BKDRHash(const char *str)
{
    size_t hash = 0;

    char *tmp = str;
    while (*tmp)
    {
        /*31、131、1313、13131、131313..*/
        hash = hash * SEED + (size_t) *tmp++;
    }
    printf("%s:[%zu]-->[%d]\n", str, hash, (hash) % 19);
    return (hash % 19);
}

void init_hash_map()
{
    int i = 0;

    for (i = 0; domain[i][0] != '\0'; i++)
    {
        strcpy(list[BKDRHash(domain[i])], ip[i]);
    }
}

int search_hash(char *str)
{
    puts("\n>>>>>>>>>>>>>>>search<<<<<<<<<<<<<");
    int index = 0;
    int i = 0;
    for (i = 0; domain[i][0] != '\0'; i++)
    {
        index = BKDRHash(domain[i]);
        printf("--->search:\n\t[%d]-->[%s]---->[%s]\n", index, domain[i], list[index]);
    }

    return 0;
}

int main(int argc, char **argv)
{
    int i = 7;

    init_data();
    init_hash_map();

    search_hash(NULL);

    return 0;
}
