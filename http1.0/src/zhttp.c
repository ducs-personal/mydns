/*
 * zhttp.c
 *
 *  Created on: Apr 16, 2015
 *      Author: root
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <time.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <syslog.h>
#include <string.h>

#include "http_1_0.h"
#include "zdns.h"

#define GET                 (1)
#define POST                (2)
#define DOWNLOAD            (3)

/*ERRNO DEFINETION*/
#define SOCKET_ERROR        (1)
#define TIMEOUT             (2)

#define BUFF_SIZE           (1024)

#define POST_HDR_FRAME ("POST %s HTTP/1.0\r\nHost: %s\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: %ld\r\nConnection:   Close\r\n\r\n")
#define GET_HDR_FRAME  ("GET %s HTTP/1.0\r\nHost: %s\r\nContent-Type: application/x-www-form-urlencoded\r\nConnection:   Close\r\n\r\n")

typedef struct _t_package_hdr
{

    int type;
    int hdr_len;
    char hdr[1024];
    char dst_ip[32];

} package_hdr_t;

char *safe_malloc(long size)
{
    char *mem = NULL;
    mem = malloc(size);
    if (mem == NULL)
    {
        printf("malloc fail!\n");
//        exit(1);          //TODO:fix
    }
    return mem;
}

int http_1_0_hdrlen(char *resp)
{
    int hdr_len = -1;
    char *p = NULL;

    if (resp != NULL && (p = strstr(resp, "\r\n\r\n")) != NULL)
    {
        hdr_len = p - (resp) + 4;
    }

    return hdr_len;
}

static int isready_read(int fd, int timeout)
{
    int rc;
    fd_set rfds;
    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    rc = select(fd + 1, &rfds, NULL, NULL, &tv);
    if (rc < 0)          //error
        return -1;
    return FD_ISSET(fd, &rfds) ? 1 : 0;
}

static int sock_stream_new()
{
    int sockfd = -1;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        fprintf(stderr, "create socket failed!\n");
        return -1;
    }
    return sockfd;
}

static int send_request(int sock, char *request, long request_len)
{
    int len = 0, writen = 0;
    while (writen < request_len)
    {
        len = write(sock, request + writen, request_len - writen);
        if (len == -1)
        {
            printf("send error!%s\n ", strerror(errno));
            return -1;
        }
        writen += len;
    }
    return 0;
}

static int connect_to_server(char *dst_ip, int port)
{
    int sock = -1;
    struct sockaddr_in srvaddr;

    sock = sock_stream_new();

    bzero(&srvaddr, sizeof(srvaddr));
    srvaddr.sin_family = PF_INET;
    srvaddr.sin_port = htons(port);
    if (inet_pton(PF_INET, dst_ip, &srvaddr.sin_addr) <= 0)
    {
        fprintf(stderr, "inet_pton error!\n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *) &srvaddr, sizeof(srvaddr)) < 0)
    {
        fprintf(stderr, "connect error!\n");
        return -1;
    }

    return sock;
}

static int _process_post_get(int sock, char **buf, int timeout, int *err_no, long *response_len)
{
    int len = 0;
    int tic = 0;
    int ret = 0;
    tic = isready_read(sock, timeout);
    if (tic <= 0)
    {
        printf("timeout \n");
        ret = -1;
    }
    else
    {
        while ((len = read(sock, *buf, BUFF_SIZE)) > 0)
        {
            printf("read_len:[%d]\n", len);
            ret = 0;
            break;
        }
        shutdown(sock, 2);
        close(sock);
    }

    *err_no = ret;
    *response_len = len;
    return ret;
}

static int _process_download(int sock, char **buf, char *path)
{
    FILE *f;
    int len = 0;
    int resp_hdr_len = 0;
    char *p = NULL;

    f = fopen(path, "w+");
    if (f == NULL)
    {
        return -1;
    }

    while ((len = read(sock, *buf, BUFF_SIZE)) > 0)
    {
        if ((p = strstr(*buf, "\r\n\r\n")) != NULL)
        {
            resp_hdr_len = p - (*buf) + 4;
            fwrite(p + 4, len - resp_hdr_len, 1, f);
        }
        else
        {
            fwrite(*buf, len, 1, f);
        }
    }
    shutdown(sock, 2);
    close(sock);

    if (f) fclose(f);

    return 0;
}

static char *_process(int type, package_hdr_t *pkg_hdr, char *data, long data_len, int timeout, int *err_no, long *response_len, char *path)
{
    int sock = -1;
    int len = 0;
    int ret = 0;
    FILE *f;
    char * buf = NULL;

    buf = safe_malloc(BUFF_SIZE);
    memset(buf, 0, BUFF_SIZE);
    memcpy(buf, pkg_hdr->hdr, pkg_hdr->hdr_len);
    memcpy(buf + pkg_hdr->hdr_len, data, data_len);

    sock = connect_to_server(pkg_hdr->dst_ip, 80);

    len = send_request(sock, buf, pkg_hdr->hdr_len + data_len);

    memset(buf, 0, BUFF_SIZE);

    if (type == POST || type == GET)
    {
        _process_post_get(sock, &buf, timeout, err_no, response_len);
    }

    if (type == DOWNLOAD)
    {
        *err_no = _process_download(sock, &buf, path);
        if (buf) free(buf);
        return NULL;
    }

    return buf;
}
/**
 * @host: www.tmall.com or 192.168.2.1
 * @uri:   api/ping/?....
 * @data:  msg_body
 * @data_len:msg_body lenth
 * @timeout: timeout to get reply
 */
static char *socket_adapter_request(int type, char *host, char *uri, char *data, long data_len, int timeout, int *err_no, long *response_len, char *path)
{
    package_hdr_t pkg_hdr;

    zdns_srv(host, pkg_hdr.dst_ip, "223.5.5.5", 3);

    pkg_hdr.type = type;
    if (type == POST)
    {
        sprintf((pkg_hdr.hdr), POST_HDR_FRAME, uri, host, data_len);
    }
    else if (type == GET)
    {
        sprintf((pkg_hdr.hdr), GET_HDR_FRAME, uri, host);
    }
    else if (type == DOWNLOAD)
    {
        sprintf((pkg_hdr.hdr), GET_HDR_FRAME, uri, host);
    }

    pkg_hdr.hdr_len = strlen(pkg_hdr.hdr);

    return _process(type, &pkg_hdr, data, data_len, timeout, err_no, response_len, path);
}

int http_1_0_post(char *host, char *uri, char *data, long data_len, int timeout, char **resp, long *resp_len)
{
    int err_no = 0;
    char *response = NULL;
    long response_len;

    do
    {
        response = socket_adapter_request(POST, host, uri, data, data_len, timeout, &err_no, &response_len, NULL);

        if (0 == (response_len))
        {
            break;
        }
        *resp = (char *) response;
        *resp_len = response_len;

    } while (0);

    return err_no;
}

int http_1_0_get(char *host, char *uri, int timeout, char **resp, long *resp_len)
{

    int err_no = 0;
    char *response;
    long response_len;

    do
    {
        response = socket_adapter_request(GET, host, uri, NULL, 0, timeout, &err_no, &response_len, NULL);
        if (0 == (response_len))
        {
            break;
        }
        *resp = (char *) response;
        *resp_len = response_len;

    } while (0);

    return err_no;
}

int http_1_0_download(char *host, char *uri, int timeout, char *dst)
{
    int err_no = 0;
    char *response = NULL;
    long response_len = 0;

    do
    {
        response = socket_adapter_request(DOWNLOAD, host, uri, NULL, 0, timeout, &err_no, &response_len, dst);

        if (0 == (response_len))
        {
            break;
        }

    } while (0);

    return err_no;
}

#if 0

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
    if (0)
    {
        ret = http_1_0_get("portal.zmeng123.cn", "/auth_server/ping", timeout, &resp, &resp_len);
        printf("===========(%d)=======\n%s\n", ret, resp);
        if (resp) free(resp);
    }

    /*test zhttp download*/
    if (1)
    {
        ret = http_1_0_download("gold2.hao61.net", "/o-peng/a.bin", timeout, "/tmp/b.bin");
        if (ret == 0)
        {
            printf("file has been saved  errno:%d\n", ret);
        }
    }
    return 0;
}
#endif

