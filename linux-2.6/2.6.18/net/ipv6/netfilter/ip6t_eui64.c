/* Kernel module to match EUI64 address parameters. */

/* (C) 2001-2002 Andras Kis-Szabo <kisza@sch.bme.hu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/ipv6.h>
#include <linux/if_ether.h>

#include <linux/netfilter_ipv6/ip6_tables.h>

MODULE_DESCRIPTION("IPv6 EUI64 address checking match");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andras Kis-Szabo <kisza@sch.bme.hu>");

static int
match(const struct sk_buff *skb,
      const struct net_device *in,
      const struct net_device *out,
      const struct xt_match *match,
      const void *matchinfo,
      int offset,
      unsigned int protoff,
      int *hotdrop)
{
	unsigned char eui64[8];
	int i = 0;

	if (!(skb->mac.raw >= skb->head &&
	      (skb->mac.raw + ETH_HLEN) <= skb->data) &&
	    offset != 0) {
		*hotdrop = 1;
		return 0;
	}

	memset(eui64, 0, sizeof(eui64));

	if (eth_hdr(skb)->h_proto == htons(ETH_P_IPV6)) {
		if (skb->nh.ipv6h->version == 0x6) {
			memcpy(eui64, eth_hdr(skb)->h_source, 3);
			memcpy(eui64 + 5, eth_hdr(skb)->h_source + 3, 3);
			eui64[3] = 0xff;
			eui64[4] = 0xfe;
			eui64[0] |= 0x02;

			i = 0;
			while ((skb->nh.ipv6h->saddr.s6_addr[8+i] == eui64[i])
			       && (i < 8))
				i++;

			if (i == 8)
				return 1;
		}
	}

	return 0;
}

static struct ip6t_match eui64_match = {
	.name		= "eui64",
	.match		= match,
	.matchsize	= sizeof(int),
	.hooks		= (1 << NF_IP6_PRE_ROUTING) | (1 << NF_IP6_LOCAL_IN) |
			  (1 << NF_IP6_FORWARD),
	.me		= THIS_MODULE,
};

static int __init ip6t_eui64_init(void)
{
	return ip6t_register_match(&eui64_match);
}

static void __exit ip6t_eui64_fini(void)
{
	ip6t_unregister_match(&eui64_match);
}

module_init(ip6t_eui64_init);
module_exit(ip6t_eui64_fini);
