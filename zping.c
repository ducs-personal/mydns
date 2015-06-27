/*
 * zping.c
 *
 *  Created on: Jun 4, 2015
 *      Author: root
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/time.h>

#define DEFAULT_DATALEN    (56)

typedef struct _ping_conf
{
    int sockfd;
    char ipaddr[128];
    int sendnum;
    int recvnum;
    int datalen;
    struct sockaddr_in addr;
} pconf_t;

static pconf_t self;

int init_conf()
{
    memset(&self, 0, sizeof(pconf_t));

    self.datalen = DEFAULT_DATALEN;
    self.sockfd = -1;
    return 0;
}

unsigned short _check_sum(unsigned short *data, int len)
{
    int result = 0;
    int i = 0;

    for (i = 0; i < len / 2; i++)
    {
        result += *data;
        data++;
    }

    while (result >> 16)
    {
        result = (result & 0xffff) + (result >> 16);
    }

    return ~result;
}

static void tv_sub(struct timeval* recvtime, const struct timeval* sendtime)
{
    int sec = (recvtime->tv_sec - sendtime->tv_sec);
    int usec = recvtime->tv_usec - sendtime->tv_usec;

    if (usec >= 0)
    {
        recvtime->tv_sec = sec;
        recvtime->tv_usec = usec;
    }
    else
    {
        recvtime->tv_sec = sec - 1;
        recvtime->tv_usec = -usec;
    }
}

void send_icmp()
{
    int len = -1, ret = -1;
    char sendbuf[64];
    struct icmp* icmp = NULL;

    icmp = (struct icmp*) sendbuf;
    icmp->icmp_type = ICMP_ECHO;
    icmp->icmp_code = 0;
    icmp->icmp_cksum = 0;
    icmp->icmp_id = getpid();                 //needn't use htons() call,networking kernel didn't handle this data
    icmp->icmp_seq = ++self.sendnum;          //needn't use hotns() call too.
    gettimeofday((struct timeval*) icmp->icmp_data, NULL);
    icmp->icmp_cksum = _check_sum((unsigned short*) icmp, self.datalen + 8);

    ret = sendto(self.sockfd, sendbuf, self.datalen + 8, 0, (struct sockaddr*) &self.addr, sizeof(self.addr));
    if (ret == -1)
    {
        exit(-1);
    }
    else
    {
        printf("send icmp request to %s(%d) bytes\n", self.ipaddr, ret, len);
    }
}

static void _print_buf(char *buf, int len)
{
    int i = 0;
    for (i = 0; i < len; i++)
    {
        printf(" [%X] ", htons(*(buf + i)));
        if (i % 10 == 0) printf("\n");
    }
}

int recv_icmp(int limit)
{
    int ret = -1;
    char recvbuf[256];

    struct ip *ip = NULL;
    struct timeval recvtime;
    struct icmp * icmp = NULL;
    struct timeval *sendtime = NULL;

    while (1)
    {

        memset(recvbuf, 0, sizeof(recvbuf));
        ret = recvfrom(self.sockfd, recvbuf, sizeof(recvbuf), 0, 0, 0);
        if (ret == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }
            else
            {
                perror("recvfrom()");
                exit(-1);
            }
        }
        else
        {

            ip = (struct ip*) recvbuf;
            if (ip->ip_src.s_addr != self.addr.sin_addr.s_addr)
            {
                continue;
            }

            self.recvnum++;
            gettimeofday(&recvtime, NULL);

            icmp = ((struct icmp *) (recvbuf + ((ip->ip_hl) * 4)));
            sendtime = (struct timeval*) icmp->icmp_data;
            tv_sub(&recvtime, sendtime);

            printf("%s(%d bytes)\tttl=%d\tseq=%d\ttime=%d.%06d\n", self.ipaddr, ret, ip->ip_ttl, icmp->icmp_seq, recvtime.tv_sec, recvtime.tv_usec);

            if ((limit--) == 0)
            {
                printf("\nPing statics:send %d packets, recv %d packets, %d%% lost...\n", self.sendnum, self.recvnum,
                        (int) ((float) (self.sendnum - self.recvnum) / self.sendnum) * 100);
                break;
            }
        }
    }
}

void send_icmp_ping_handler(int signum)
{
    send_icmp();
    alarm(1);
}

void catch_sigint_handler(int signum)
{
    printf("\nPing statics:send %d packets, recv %d packets, %d%% lost...\n", self.sendnum, self.recvnum,
            (int) ((float) (self.sendnum - self.recvnum) / self.sendnum) * 100);
    exit(0);
}

int init_icmp_signal()
{
    struct sigaction sa1;
    memset(&sa1, 0, sizeof(sa1));
    sa1.sa_handler = send_icmp_ping_handler;
    sigemptyset(&sa1.sa_mask);
    sa1.sa_flags = 0;
    if (sigaction(SIGALRM, &sa1, NULL) == -1)
    {
        perror("sigaction()");
        return -1;
    }

    struct sigaction sa2;
    memset(&sa2, 0, sizeof(sa2));
    sa2.sa_handler = catch_sigint_handler;
    sigemptyset(&sa2.sa_mask);
    sa2.sa_flags = 0;
    if (sigaction(SIGINT, &sa2, NULL) == -1)
    {
        perror("sigaction()");
        return -1;
    }

    return 0;
}

int init_icmp_ip(char *input)
{
    struct hostent* host = NULL;
    int ret = -1;

    memset(&(self.addr), 0, sizeof(self.addr));
    self.addr.sin_family = AF_INET;
    ret = inet_pton(AF_INET, input, &(self.addr.sin_addr));

    if (ret == -1 || ret == 0)
    {          //usr input is domain
        host = gethostbyname(input);
        if (host == NULL)
        {
            fprintf(stderr, "gethostbyname(%s):%s\n", input, strerror(errno));
            return -1;
        }

        if (host->h_addr_list != NULL && *(host->h_addr_list) != NULL)
        {
            strncpy((char*) &self.addr.sin_addr, *(host->h_addr_list), 4);
            inet_ntop(AF_INET, *(host->h_addr_list), self.ipaddr, sizeof(self.ipaddr));
        }
        printf("Ping address:%s (%s)\n\n", host->h_name, self.ipaddr);
    }
    else
    {          //usr input is ip
        strcpy(self.ipaddr, input);
        printf("Ping address:%s(%s)\n\n", self.ipaddr, self.ipaddr);
    }
    return 0;
}

int init_icmp_sock()
{
    self.sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

    if (self.sockfd == -1)
    {
        return -1;
    }
    return self.sockfd;
}

int start_icmp_ping(int limit)
{
    alarm(1);
    return recv_icmp(limit);
}
/**
 * @eg.    :ping www.tmall.com  try_times
 * @return :the nums of success time;
 */
int main(int argc, char **argv)
{
    if (argc < 3)
    {
        printf("Usage...< ping > <hostname> <try times>\n");
        exit(0);
    }

    if (init_icmp_sock() < 0)
    {
        printf("init socket error\n");
        exit(0);
    }

    if (init_icmp_ip(argv[1]) < 0)
    {
        printf("init ip error\n");
        exit(0);
    }

    if (init_icmp_signal() < 0)
    {
        printf("init signal error\n");
        exit(0);
    }

    int limit_time = atoi(argv[2]);
    if (start_icmp_ping(limit_time) < 0)
    {
        printf("start icmp ping  error\n");
        exit(0);
    }

    return 0;
}

