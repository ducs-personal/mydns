/*
 * zhttp.c
 *
 *  Created on: Apr 16, 2015
 *      Author: root
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "http_1_0.h"

int main()
{

    int ret = -1;
    int timeout = 2;
    long resp_len = 0;
    char *resp = NULL;

    /*test zhttp post*/
    if (0)
    {
        // ret = http_1_0_post("portal.zmeng123.cn", "auth_server/ping", NULL, 0, timeout, &resp, &resp_len);
        ret = http_1_0_post("115.28.115.214", "/auth_server/ping", NULL, 0, timeout, &resp, &resp_len);
        printf("===========(%d)=======\n%s\n", ret, resp);
        if (resp) free(resp);
    }

    /*test zhttp get*/
    if (1)
    {
        ret = http_1_0_get("portal.zmeng123.cn", "/auth_server/ping", timeout, &resp, &resp_len);
        printf("===========(%d)=======\n%s\n", ret, resp);
        if (resp) free(resp);
    }

    /*test zhttp download*/
    if (0)
    {
        ret = http_1_0_download("gold2.hao61.net", "/o-peng/a.bin", timeout, "/tmp/b.bin");
        if (ret == 0)
        {
            printf("file has been saved  errno:%d\n", ret);
        }
    }
    return 0;
}

