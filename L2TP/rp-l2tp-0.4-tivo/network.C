// Code for handling the UDP socket we send/receive on.  The
// tunnel uses a single UDP socket which stays open for the life of
// the application.
//
// This code is based in large part on rp-l2tpd, and
// carries forth its copyrights and licensing.
//
// Copyright (C) 2002 by Roaring Penguin Software Inc.
//
// This software may be distributed under the terms of the GNU General
// Public License, Version 2, or (at your option) any later version.
//
// LIC: GPL

#include "includes.h"

// Our socket
int Sock;
char Hostname[MAX_HOSTNAME];

static EventHandler *networkReadHandler;

/**********************************************************************
* %FUNCTION: network_readable
* %ARGUMENTS:
*  es -- event selector
*  fd -- socket
*  flags -- event-handling flags telling what happened
*  data -- not used
* %RETURNS:
*  Nothing
* %DESCRIPTION:
*  Called when a packet arrives on the UDP socket.
***********************************************************************/
static void network_readable(EventSelector *es, int fd, unsigned int flags, void *data)
{
	l2tp_dgram *dgram;

	struct sockaddr_in from;
	dgram = l2tp_dgram_take_from_wire(&from);
	if (!dgram)
	{
		return;
	}

	// It's a control packet if we get here
	l2tp_tunnel_handle_received_control_datagram(dgram, es, &from);
	l2tp_dgram_free(dgram);
}

void l2tp_network_uninit(EventSelector *es)
// Clean up after l2tp_network_init
{
	Event_DelHandler(es,networkReadHandler);
	close(Sock);
}

static int bind_to_device(int sock,const char *interfaceName,struct in_addr *address)
// Bind the passed socket to the passed device, and set the address
// of the interface into *address.
// On success, return 0, else return -1.
{
	struct ifreq interface;

	strncpy(interface.ifr_ifrn.ifrn_name,interfaceName,IFNAMSIZ);
	interface.ifr_ifrn.ifrn_name[IFNAMSIZ-1]='\0';

	if(setsockopt(sock,SOL_SOCKET,SO_BINDTODEVICE,(char *)&interface,sizeof(interface))>=0)
	{
		if(ioctl(sock,SIOCGIFADDR,&interface)>=0)		// get the address of the interface
		{
			*address=((struct sockaddr_in *)&interface.ifr_addr)->sin_addr;
			return(0);
		}
	}
	return(-1);
}

/**********************************************************************
* %FUNCTION: network_init
* %ARGUMENTS:
*  es -- an event selector
* %RETURNS:
*  >= 0 if all is OK, <0 if not
* %DESCRIPTION:
*  Initializes network; opens socket on UDP port 1701; sets up
*  event handler for incoming packets.
***********************************************************************/
int l2tp_network_init(EventSelector *es,const char *interfaceName)
{
	struct sockaddr_in me;
	int flags;

	gethostname(Hostname, sizeof(Hostname));
	Hostname[sizeof(Hostname)-1] = 0;

	if((Sock = socket(PF_INET, SOCK_DGRAM, 0))>=0)
	{
		me.sin_family = AF_INET;
		me.sin_addr.s_addr = htonl(INADDR_ANY);	// default to using any source address
		me.sin_port = htons(1701);

		if((!interfaceName)||(bind_to_device(Sock,interfaceName,&me.sin_addr)>=0))
		{
			if (bind(Sock, (struct sockaddr *) &me, sizeof(me)) >= 0)
			{
				// Set socket non-blocking
				flags = fcntl(Sock, F_GETFL);
				flags |= O_NONBLOCK;
				fcntl(Sock, F_SETFL, flags);

				// Set up the network read handler
				if((networkReadHandler=Event_AddHandler(es, Sock, EVENT_FLAG_READABLE, network_readable, NULL)))
				{
					return Sock;
				}
				else
				{
					fprintf(stderr,"%s: Event_AddHandler failed\n",__FUNCTION__);
				}
			}
			else
			{
				fprintf(stderr,"%s: bind failed: %s\n",__FUNCTION__,strerror(errno));
			}
		}
		else
		{
			fprintf(stderr,"%s: bind_to_device failed: %s\n",__FUNCTION__,strerror(errno));
		}
		close(Sock);
	}
	else
	{
		fprintf(stderr,"%s: socket failed: %s\n",__FUNCTION__,strerror(errno));
	}
	return(-1);
}

