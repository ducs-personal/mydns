/*
 * zdns.c
 *
 *  Created on: Apr 15, 2015
 *      Author: root
 */

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<unistd.h>

#include "zdns.h"

#define T_A         1          /*Ipv4 address*/
#define T_NS        2          /*Nameserver*/
#define T_CNAME     5          /* canonical name*/
#define T_SOA       6          /* start of authority zone */
#define T_PTR       12         /* domain name pointer */
#define T_MX        15         /*Mail server*/

typedef struct _t_dns_header
{
    unsigned short id; /* identification number */

    unsigned char rd :1; /* recursion desired */
    unsigned char tc :1; /* truncated message */
    unsigned char aa :1; /* authoritive answer */
    unsigned char opcode :4; /* purpose of message */
    unsigned char qr :1; /* query/response flag */

    unsigned char rcode :4; /* response code */
    unsigned char cd :1; /* checking disabled */
    unsigned char ad :1; /* authenticated data */
    unsigned char z :1; /* its z! reserved */
    unsigned char ra :1; /* recursion available */

    unsigned short q_count; /* number of question entries */
    unsigned short ans_count; /* number of answer entries */
    unsigned short auth_count; /* number of authority entries */
    unsigned short add_count; /* number of resource entries */
} dns_header_t;

/*head +qname+question*/
typedef struct _t_question
{
    unsigned short qtype;
    unsigned short qclass;
} question_t;

/*Constant sized fields of the resource record structure*/
#pragma pack(push, 1)
typedef struct _t_res_data
{
    unsigned short type;
    unsigned short _class;
    unsigned int ttl;
    unsigned short data_len;
} res_data_t;
#pragma pack(pop)

/*Pointers to resource record contents*/
typedef struct _t_res
{
    unsigned char *name;
    res_data_t *resource;
    unsigned char *rdata;
} res_t;

static char dns_servers[2][32];
static res_t answers[20];

int isready_read(int fd, int timeout)
{
    int rc;
    fd_set rfds;
    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    rc = select(fd + 1, &rfds, NULL, NULL, &tv);
    if (rc < 0) /*error */
    return -1;
    return FD_ISSET(fd, &rfds) ? 1 : 0;
}

static int is_ipaddr(char *host)
{
    int a, b, c, d;
    char t = '\0';
    int ret = -1;
    if (4 <= sscanf(host, "%d.%d.%d.%d%c", &a, &b, &c, &d, &t))
    {
        if ((0 <= a && a <= 255) && (0 <= b && b <= 255) && (0 <= c && c <= 255) && (0 <= d && d <= 255) && t == '\0')
        {
            printf("[%s] is valid IPv4\n", host);
            ret = 1;
        }
        else
        {
            printf("[%s] is invalid IPv4 < include other msg >\n", host);
            ret = 0;
        }
    }
    else
    {
        //printf("[%s] is invalid IPv4 < it's a url >\n", host);
        ret = -1;
    }
    return ret;
}

void domain_format(unsigned char* dns, unsigned char* tmp)
{
    int lock = 0, i;
    char host[32];

    strcpy(host, tmp);
    strcat((char*) host, ".");
    for (i = 0; i < strlen((char*) host); i++)
    {
        if (host[i] == '.')
        {
            *dns++ = i - lock;
            for (; lock < i; lock++)
            {
                *dns++ = host[lock];
            }
            lock++;
        }
    }
    *dns++ = '\0';
}

void _show_dnshdr(dns_header_t *dnshdr)
{
    printf("\nThe response contains : ");
    printf("\n %d Questions.", ntohs(dnshdr->q_count));
    printf("\n %d Answers.", ntohs(dnshdr->ans_count));
    printf("\n %d Authoritative Servers.", ntohs(dnshdr->auth_count));
    printf("\n %d Additional records.\n\n", ntohs(dnshdr->add_count));
}

void _show_answers(int ans_count, char *ipstr)
{
    int i = 0;
    long *p = NULL;
    struct sockaddr_in tmp;

    for (i = 0; i < ans_count; i++)
    {
        printf("Name : %s ", answers[i].name);

        /*it's a ipv4 addr */
        if (ntohs(answers[i].resource->type) == T_A)
        {
            p = (long*) answers[i].rdata;
            /*do not use ntohl */
            tmp.sin_addr.s_addr = (*p);
            printf("has IPv4 address : [%s]\n", inet_ntoa(tmp.sin_addr));
            strcpy(ipstr, inet_ntoa(tmp.sin_addr));
            break;
        }

        if (ntohs(answers[i].resource->type) == 5)
        {
            /*it's a CNAME */
            printf("has alias name : [%s]\n", answers[i].rdata);
        }
    }
}

void _pack_sockaddr(struct sockaddr_in *dest)
{
    dest->sin_family = AF_INET;
    dest->sin_port = htons(53);
    dest->sin_addr.s_addr = inet_addr(dns_servers[0]); /*dns servers */
}

void _pack_dnshdr(dns_header_t *dnshdr)
{
    dnshdr->id = (unsigned short) htons(getpid());
    dnshdr->qr = 0; /*This is a query */
    dnshdr->opcode = 0; /*This is a standard query */
    dnshdr->aa = 0; /*Not Authoritative */
    dnshdr->tc = 0; /*This message is not truncated */
    dnshdr->rd = 1; /*Recursion Desired */
    dnshdr->ra = 0; /*Recursion not available! hey we dont have it (lol) */
    dnshdr->z = 0;
    dnshdr->ad = 0;
    dnshdr->cd = 0;
    dnshdr->rcode = 0;
    dnshdr->q_count = htons(1); /*we have only 1 question */
    dnshdr->ans_count = 0;
    dnshdr->auth_count = 0;
    dnshdr->add_count = 0;
}

unsigned char * read_name(unsigned char* reader, unsigned char* buffer, int* count)
{
    unsigned char *name;
    unsigned int p = 0, jumped = 0, offset;
    int i, j;

    *count = 1;
    name = (unsigned char*) malloc(256);
    name[0] = '\0';

    /*read the names in 3www6google3com format */
    while (*reader != 0)
    {
        if (*reader >= 192)
        {
            /*  49152 = 11000000 00000000    192=11000000 */
            offset = ((*reader) << 8) + *(reader + 1) - 49152;
            reader = buffer + offset - 1;
            jumped = 1;
        }
        else
        {
            name[p++] = *reader;
        }

        reader = reader + 1;

        if (jumped == 0)
        {
            *count = *count + 1;
        }
    }

    name[p] = '\0';
    if (jumped == 1)
    {
        *count = *count + 1;
    }

    /*now convert 3www6google3com0 to www.google.com */
    for (i = 0; i < (int) strlen((const char*) name); i++)
    {
        p = name[i];
        for (j = 0; j < (int) p; j++)
        {
            name[i] = name[i + 1];
            i = i + 1;
        }
        name[i] = '.';
    }
    name[i - 1] = '\0';
    return name;
}

void _deal_recv(char *buf, int qname_len)
{
    dns_header_t * dnshdr = NULL;
    int i = 0, j = 0, stop = 0;
    char *reader = NULL, qname = NULL;

    dnshdr = (dns_header_t*) buf;
    _show_dnshdr(dnshdr);

    reader = (buf + sizeof(dns_header_t) + qname_len + sizeof(question_t));

    for (i = 0; i < ntohs(dnshdr->ans_count); i++)
    {
        answers[i].name = read_name(reader, buf, &stop);
        reader = reader + stop;

        answers[i].resource = (res_data_t*) (reader);
        reader = reader + sizeof(res_data_t);

        /*if its an ipv4 address */
        if (ntohs(answers[i].resource->type) == 1)
        {
            answers[i].rdata = (unsigned char*) malloc(ntohs(answers[i].resource->data_len));

            for (j = 0; j < ntohs(answers[i].resource->data_len); j++)
            {
                answers[i].rdata[j] = reader[j];
            }

            answers[i].rdata[ntohs(answers[i].resource->data_len)] = '\0';
            reader = reader + ntohs(answers[i].resource->data_len);
        }
        else
        {
            answers[i].rdata = read_name(reader, buf, &stop);
            reader = reader + stop;
        }
    }
}

int zgethostbyname(unsigned char *host, char *ipstr, int query_type, int timeout)
{
    int sock = -1;
    int len = -1;
    int ret = 0;
    int qname_len = 0;
    unsigned char *qname = NULL;
    unsigned char *buf = NULL;
    question_t *qinfo = NULL;

    printf("HOST:%s\n", host);
    if (is_ipaddr(host) == 1)
    {
        strcpy(ipstr, host);
        return 0;
    }

    struct sockaddr_in dest;
    buf = (unsigned char *) malloc(1024);

    do
    {
        sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        _pack_sockaddr(&dest);

        /*question body*/
        {
            /*step 1 pack dns header*/
            _pack_dnshdr((dns_header_t *) buf);

            /*step 2 pack question name not constent lenth*/
            qname = (unsigned char*) (buf + sizeof(dns_header_t));
            domain_format(qname, host);
            qname_len = (strlen((const char*) qname) + 1);

            /*step 3 pack question info*/
            qinfo = (question_t*) (buf + sizeof(dns_header_t) + qname_len);

            /*type of the query , A , MX , CNAME , NS etc */
            qinfo->qtype = htons(query_type);
            qinfo->qclass = htons(1);
        }

        /*Sent the question*/
        if (sendto(sock, (char*) buf, sizeof(dns_header_t) + qname_len + sizeof(question_t), 0, (struct sockaddr*) &dest, sizeof(dest)) < 0)
        {
            printf("\nsendto failed\n");
        }

        if ((isready_read(sock, timeout)) == 0)
        {
            printf("timeout\n");
            ret = -1;
            break;
        }

        /*Receive the answer*/
        len = sizeof(dest);
        if (recvfrom(sock, (char*) buf, 1024, 0, (struct sockaddr*) &dest, (socklen_t*) &(len)) < 0)
        {
            perror("recvfrom failed");
        }

        _deal_recv(buf, qname_len);
        _show_answers(ntohs(((dns_header_t *) buf)->ans_count), ipstr);
    } while (0);

    free(buf);
    return ret;
}

void get_dns_servers(char *zdns_server)
{
    if (zdns_server)
    {
        strcpy(dns_servers[0], zdns_server);
    }
}

/* ipstr domain alloced by user*/
int zdns_srv(char *domain, char *ipstr, char *dns_srv, int timeout)
{
    get_dns_servers(dns_srv);

    return zgethostbyname(domain, ipstr, T_A, timeout);
}

/* test for zdns.c*/
#if 0
int main()
{
    char ipstr[32];
    int ret = -1;
    ret = zdns_srv("wwww.tmall.com", ipstr, "8.8.8.8", 1);
    if (ret < 0)
    {
        printf("timeout!\n");
    }
    else
    {
        printf("\n%s\n", ipstr);
    }

    return 0;
}
#endif
