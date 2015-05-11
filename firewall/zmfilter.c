/*
 * zmfilter.c
 *
 *  Created on: 2015年3月6日
 *      Author: dev
 */
#include <linux/types.h>
#include <linux/ip.h>
#include <linux/netfilter.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <net/ip.h>

#include <linux/netfilter_ipv4.h>
#include <net/tcp.h>

#include <linux/netfilter_bridge.h>
#include <net/netfilter/nf_conntrack.h>
#include <linux/inetdevice.h>

#include <linux/rcupdate.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/jhash.h>
#include <br_private.h>
#include <br_fdb.h>
#include <linux/rcupdate.h>
#include <linux/rculist.h>
#include <linux/random.h>
#include <linux/list.h>

#include <linux/version.h>   /*  version info */
#include <net/netfilter/nf_conntrack_ecache.h>   /* for nf mark */

#include "skh.h"
#include "zm_util.h"
#include "zmfilter_protocol.h"
#include "db_manager.h"
#include "lgate_interface.h"
#include "http_inject.h"

MODULE_LICENSE("GPL");

#ifdef CONFIG_X86_32
#define NET_DEV_BR "br-lan"
#else
#define NET_DEV_BR "br0"
#endif

#define __jhash_final_zm(a, b, c)			\
{						\
	c ^= b; c -= rol32(b, 14);		\
	a ^= c; a -= rol32(c, 11);		\
	b ^= a; b -= rol32(a, 25);		\
	c ^= b; c -= rol32(b, 16);		\
	a ^= c; a -= rol32(c, 4);		\
	b ^= a; b -= rol32(a, 14);		\
	c ^= b; c -= rol32(b, 24);		\
}

static __be32 B_LocalNet_IP;
static __be32 B_LocalNet_Mask;
static __be32 C_LocalNet_IP;
static __be32 C_LocalNet_Mask;

static __be32 LocalGate_IP;
static __be32 LocalGate_Mask;

static void pre_init(void)
{
    B_LocalNet_Mask = htonl(0xFFF00000);
    C_LocalNet_Mask = htonl(0xFFFF0000);
    B_LocalNet_IP = htonl(0xAC100000);
    C_LocalNet_IP = htonl(0xC0A80000);

    LocalGate_Mask = htonl(0xFFFFFF00);
    LocalGate_IP = htonl(0x0A010000);
}

static int tcp_special_case(struct iphdr *iph, struct tcphdr *tcph, __be32 destip, __be16 destport)
{
    if (tcph->fin || tcph->syn || tcph->rst)
    {
        return 1;
    }

    if (0 == ntohs(iph->tot_len) - ((char *) tcph + (tcph->doff * 4) - (char *) iph))
    {
        return 1;
    }
    return 0;
}

static int udp_special_case(struct udphdr *h, __be32 destip, __be16 destport)
{
    if ((destport == htons(53)) || (destport == htons(5246)) || (destport == htons(5247)))
    {
        return 1;
    }
    return 0;
}

#if LINUX_VERSION_CODE == KERNEL_VERSION(3, 13, 11)
static unsigned int zmeng_filter(const struct nf_hook_ops *ops, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff *))
{
    unsigned int hooknum = ops->hooknum;
#else
    static unsigned int zmeng_filter(unsigned int hooknum, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff *))
    {
#endif

        struct iphdr *iph = ip_hdr(skb);
        enum ip_conntrack_info ctinfo;
        __be32 _destip;
        __be16 _destport;
        unsigned int nf_return_rule = NF_DROP;
        unsigned char _destip_str[64] =
        {   0};

        if (DBM_STATUS_STOP == dbm_filter_status()) return NF_ACCEPT;

        if (skb_is_nonlinear(skb))
        {
            skb_linearize(skb);
        }

        if (memcmp(in->name, NET_DEV_BR, 3) != 0)
        {
            return NF_ACCEPT;
        }

        nf_ct_get(skb, &ctinfo);
        if (CTINFO2DIR(ctinfo) != IP_CT_DIR_ORIGINAL)
        {
            return NF_ACCEPT;
        }

        _destip = iph->daddr;
        ip2string(_destip, _destip_str, 64);

        if (LocalGate_IP == (_destip & LocalGate_Mask))
        {
            /**
             * request to GateWay, response agreements;
             */
            do
            {
                nf_return_rule = NF_ACCEPT;
                print2file("Got Local Gate request to %s\n", _destip_str);
                if (IPPROTO_TCP != iph->protocol)
                break;
                if (htons(80) != skh_tcphdr(skb)->dest)
                break;

                nf_return_rule = lgi_http_handle(skb);
            }while(0);
        }
        else if ( (B_LocalNet_IP == (_destip&B_LocalNet_Mask)) || (C_LocalNet_IP == (_destip&C_LocalNet_Mask)))
        {
            /**
             * forward to local network, Just Accept
             */

            print2file("Got Local Net request to %s\n", _destip_str);
            nf_return_rule = NF_ACCEPT;
        }
        else
        {
            switch(iph->protocol)
            {
                case IPPROTO_UDP:
                _destport = skh_udphdr(skb)->dest;
                if (udp_special_case(skh_udphdr(skb), _destip, _destport))
                {
                    return NF_ACCEPT;
                }
                break;
                case IPPROTO_TCP:
                _destport = skh_tcphdr(skb)->dest;
                if (tcp_special_case(iph, skh_tcphdr(skb), _destip, _destport))
                {
                    return NF_ACCEPT;
                }
                break;
                default:
                print2file("Accept other protocol\n");
                return NF_ACCEPT;
            }
            nf_return_rule = zm_net_filter(skb, iph->protocol, _destip, _destport);
        }

        return nf_return_rule;
    }

#if LINUX_VERSION_CODE == KERNEL_VERSION(3, 13, 11)
static unsigned int http_inject_hook(const struct nf_hook_ops *ops, struct sk_buff *skb, const struct net_device *in, const struct net_device *out,
        int (*okfn)(struct sk_buff *))
{
    unsigned int hooknum = ops->hooknum;
#else
    static unsigned int http_inject_hook(unsigned int hooknum, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff *))
    {
#endif

    const struct iphdr *iph = ip_hdr(skb);

    if (iph->protocol != IPPROTO_TCP)
    {
        return NF_ACCEPT;
    }

    inject_process(skb, skh_tcphdr(skb));

    return NF_ACCEPT;
}

static inline u32 jhash_3words_zm(u32 a, u32 b, u32 c, u32 initval)
{
    a += JHASH_INITVAL;
    b += JHASH_INITVAL;
    c += initval;

    __jhash_final_zm(a, b, c);

    return c;
}

static void _print_eth_mac(char *addr)
{
    print2file("eth_mac:%02x:%02x:%02x:%02x:%02x:%02x\n", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
}

static u32 fdb_salt_zm __read_mostly;

static struct kmem_cache *br_fdb_cache __read_mostly;
int __init br_fdb_init_zm (void)
{
    br_fdb_cache = kmem_cache_create("bridge_fdb_cache",sizeof(struct net_bridge_fdb_entry),0,SLAB_HWCACHE_ALIGN, NULL);
    if (!br_fdb_cache)
    return -ENOMEM;

    get_random_bytes(&fdb_salt_zm, sizeof(fdb_salt_zm));
    return 0;
}

static inline u32 jhash_1word_zm(u32 a, u32 initval)
{
    return jhash_3words_zm(a, 0, 0, initval);
}
static inline int br_mac_hash_zm(const unsigned char *mac)
{
    /* use 1 byte of OUI cnd 3 bytes of NIC */
    u32 key = get_unaligned((u32 *) (mac + 2));
    return jhash_1word_zm(key, fdb_salt_zm) & (BR_HASH_SIZE - 1);
}

struct net_bridge_fdb_entry *__br_fdb_get_zm(struct net_bridge *br, const unsigned char *addr)
{
    struct hlist_node *h;
    struct net_bridge_fdb_entry *fdb;

    hlist_for_each_entry_rcu(fdb, h, &br->hash[br_mac_hash_zm(addr)], hlist)
    {
        if (!compare_ether_addr(fdb->addr.addr, addr))
        {
            break;
        }
    }
    return fdb;
}

#define hlist_for_each_entry_zm(tpos, pos, head, member)            \
    for (pos = (head)->first;                    \
         pos &&                          \
        ({ tpos = hlist_entry(pos, typeof(*tpos), member); 1;}); \
         pos = pos->next)

struct net_bridge_fdb_entry *fdb_find_zm(struct hlist_head *head, const unsigned char *addr)
{
    struct hlist_node *h;
    struct net_bridge_fdb_entry *fdb;

    hlist_for_each_entry(fdb, h, head, hlist)
    {
        if (!compare_ether_addr(fdb->addr.addr, addr)) return fdb;
    }
    return NULL;
}

static struct net_bridge_fdb_entry *fdb_find_rcu_zm(struct hlist_head *head, const unsigned char *addr)
{
    struct hlist_node *h;
    struct net_bridge_fdb_entry *fdb;

    hlist_for_each_entry_rcu_zm(fdb, h, head, hlist)
    {
        if (!compare_ether_addr(fdb->addr.addr, addr)) return fdb;
    }
    return NULL;
}

#if LINUX_VERSION_CODE == KERNEL_VERSION(3, 13, 11)
static unsigned int wfrom(const struct nf_hook_ops *ops, struct sk_buff *skb, struct net_device *in, struct net_device *out,
        int (*okfn)(struct sk_buff *))
{
    unsigned int hooknum = ops->hooknum;
#else
    static unsigned int wfrom(unsigned int hooknum, struct sk_buff *skb, struct net_device *in, struct net_device *out, int (*okfn)(struct sk_buff *))
    {
#endif

        struct net_bridge_fdb_entry *fdb = NULL;
        struct net_bridge_port *port = NULL;
        struct net_bridge *br = NULL;
        struct net_device *origin = NULL;
        struct hlist_node *h = NULL;
        unsigned char *addr = eth_hdr(skb)->h_source;
        struct iphdr *iph = ip_hdr(skb);
        struct udphdr *udph = NULL;
        int i = 0;

        if (iph->protocol != IPPROTO_UDP)
        {
            return NF_ACCEPT;
        }

//        udph = (void *) iph + (iph->ihl * 4);
//        if (udph->dest != htons(53))
//        {
//            return NF_ACCEPT;
//        }
        print2file("in:[%s]\n",in->name);
        _print_eth_mac(addr);
        rcu_read_lock();
        print2file("11111111\n");

//
//        port = br_port_get_rcu(dev);
//            if (!port)
//                ret = 0;
//            else {
//                fdb = __br_fdb_get(port->br, addr);
//                ret = fdb && fdb->dst && fdb->dst->dev != dev &&
//                    fdb->dst->state == BR_STATE_FORWARDING;
//            }

//struct net_device  *brdev = BR_INPUT_SKB_CB(skb)->brdev;
        br = netdev_priv(in);
//        if(br)
//        {
//            port = br_port_get_rcu(in);          //got br list
//        }
//        else
//        {
//            print2file("in is null\n");
//        }
//        if (!port)
//        {
//            print2file("333333\n");
//            print2file("no port!\n");
//        }
//        else
        if(br)
        {
            print2file("333333\n");

            print2file("hash key:[%d]\n",br_mac_hash_zm(addr));

            // fdb = __br_fdb_get_zm(port->br, addr);
            //fdb = __br_fdb_get_zm(br, addr);
            int find = 0;
            if(1)
            {
                for(i =0; i< BR_HASH_SIZE; i++)
                {
                    // hlist_for_each_entry_rcu(fdb, h, &br->hash[br_mac_hash_zm(addr)], hlist)
                    hlist_for_each_entry_rcu(fdb, h, &br->hash[i], hlist)
                    {
                        print2file("in br0:\n");
                        _print_eth_mac(fdb->addr.addr);
                        if (!compare_ether_addr(fdb->addr.addr, addr))
                        {
                            print2file("commpare\n");
                            break;
                        }
                    }
                    if(flag)
                    {
                        break;
                    }
                }

            }

            if(0)
            {
                struct hlist_head *head = &br->hash[br_mac_hash_zm(addr)];
                fdb = fdb_find_zm(head,addr);
                //fdb = fdb_find_rcu_zm(head,addr);
            }

            if(fdb)
            {
                origin = fdb->dst->dev;          //got it
                print2file("got dev :[%s][%X][%d]\n",origin->name);
            }
            else
            {
                print2file("5555555\n");
            }

        }
        rcu_read_unlock();

        return NF_ACCEPT;
    }

static struct nf_hook_ops hooks[] __read_mostly =
{
    {
        .hook = wfrom,
        .owner = THIS_MODULE,
        .pf = NFPROTO_IPV4,
        .hooknum = NF_INET_FORWARD,
        //.hooknum        = NF_INET_LOCAL_IN,
        .priority = NF_IP_PRI_FIRST,
    },
//    {
//        .hook = zmeng_filter,
//        .owner = THIS_MODULE,
//        .pf = NFPROTO_IPV4,
//        .hooknum = NF_INET_FORWARD,
//        //.hooknum        = NF_INET_LOCAL_IN,
//        .priority = NF_IP_PRI_FIRST,
//    },
//    {
//        .hook = http_inject_hook,
//        .owner = THIS_MODULE,
//        .pf = NFPROTO_IPV4,
//        .hooknum = NF_INET_FORWARD,
//        .priority = NF_IP_PRI_FIRST,
//    },
};

static int zmeng_filter_init(void)
{
    pri_init();
    pre_init();

    dbm_init();

    print2file("zmeng_filter_init ----------\n");
    return nf_register_hooks(hooks, ARRAY_SIZE(hooks));
}

static void zmeng_filter_fini(void)
{
    print2file("zmeng_filter_exit ----------\n");
    dbm_release();
    nf_unregister_hooks(hooks, ARRAY_SIZE(hooks));
}

module_init( zmeng_filter_init);
module_exit( zmeng_filter_fini);
