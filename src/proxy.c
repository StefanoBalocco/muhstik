/* Muhstik, Copyright (C) 2001-2002, Louis Bavoil <mulder@gmx.fr>       */
/*                                                                      */
/* This program is free software; you can redistribute it and/or        */
/* modify it under the terms of the GNU Library General Public License  */
/* as published by the Free Software Foundation; either version 2       */
/* of the License, or (at your option) any later version.               */
/*                                                                      */
/* This program is distributed in the hope that it will be useful,      */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "proxy.h"
#include "dcc.h"
#include "link.h"
#include "print.h"
#include "net.h"
#include "string.h"
#include "print.h"

extern config_t conf;
extern clone_t *cl[];

const char *strtype[] =
{
     "wingate",
     "SOCKS4",
     "SOCKS5",
     "proxy",
     "cisco",
     "cayman",
     "bouncer",
     "vhost",
     "direct"
};

int connect_clone (clone_t *clone, char *host, unsigned short port)
{
     char *vhost = NULL;
     netstore *ns = net_store_new();

     if (clone->type == VHOST)
	  vhost = clone->proxy;

     if (net_resolve(ns, host, port))
     {
	  print(1, 4, 0, "%s: %s: nslookup failed",
		strtype[clone->type], host);
	  net_store_destroy(ns);
	  return 1;
     }
     if (net_connect(ns, &clone->sock, vhost))
	  if (errno != EINPROGRESS)
	  {
	       net_store_destroy(ns);
	       return 1;
	  }
     clone->start = time(NULL);
     net_store_destroy(ns);
     return 0;
}

int init_irc (clone_t *clone)
{
     send_irc_nick(clone, clone->nick);
     register_clone(clone);
     return WAIT_IRC;
}

int init_bouncer (clone_t *clone)
{
     init_irc(clone);
     send_sock(clone->sock, "CONN %s:%d\n",
	       clone->server, clone->server_port);
     return WAIT_IRC;
}

int init_vhost (int sock, char *vhost)
{
     netstore *ns = net_store_new();

     if (net_resolve(ns, vhost, 0))
     {
	  print(1, 4, 0, "vhost: %s: nslookup failed", vhost);
	  net_store_destroy(ns);
	  return 1;
     }
     if (net_bind(ns, sock))
     {
	  print(1, 0, 0, "vhost: bind: %s", strerror(errno));
	  net_store_destroy(ns);
	  return 1;
     }

     net_store_destroy(ns);
     return 0;
}

int init_wingate (clone_t *clone)
{
     send_sock(clone->sock, "%s %d\r\n", clone->host, clone->port);
     return WAIT_WINGATE;
}

int init_socks4 (clone_t *clone)
{
     char buffer[9];
     struct in_addr addr;
     unsigned short port;

     port = htons(clone->port);
     if (resolve(clone->host, &addr))
     {
	  print(1, 4, 0, "%s: %s: nslookup failed",
		strtype[clone->type], clone->host);
	  clone->status = EXIT;
     }
     memcpy(&buffer[2], &port, 2);
     memcpy(&buffer[4], &addr.s_addr, 4);
     buffer[0] = 4;
     buffer[1] = 1;
     buffer[8] = 0;
     send(clone->sock, buffer, 9, 0);
     return WAIT_SOCKS4;
}

struct sock5_connect1
{
     char version;
     char nmethods;
     char method;
};

int init_socks5 (clone_t *clone)
{
     struct sock5_connect1 sc1;

     sc1.version = 5;
     sc1.nmethods = 1;
     sc1.method = 0;
     send(clone->sock, (char *) &sc1, 3, 0);

     return WAIT_SOCKS5_1;
}

int init_read_socks5 (clone_t *clone)
{
     unsigned short port;
     unsigned char *sc2;
     unsigned int addrlen;
     unsigned int packetlen;
     char buf[10];

     if (recv(clone->sock, buf, 2, 0) < 2)
	  return EXIT;

     if (buf[0] != 5 && buf[1] != 0)
	  return EXIT;

     port = htons(clone->port);
     addrlen = strlen(clone->host);
     packetlen = 4 + 1 + addrlen + 2;
     sc2 = xmalloc(packetlen);
     sc2[0] = 5;				                  /* version */
     sc2[1] = 1;						  /* command */
     sc2[2] = 0;						  /* reserved */
     sc2[3] = 3;						  /* address type */
     sc2[4] = (unsigned char) addrlen;	                  /* hostname length */
     memcpy(sc2 + 5, clone->host, addrlen);
     memcpy(sc2 + 5 + addrlen, &port, sizeof(unsigned short));

     send(clone->sock, sc2, packetlen, 0);
     free(sc2);

     return WAIT_SOCKS5_2;
}

int init_proxy (clone_t *clone)
{
     send_sock(clone->sock, "CONNECT %s:%d HTTP/1.0\r\n\r\n",
	       clone->host, clone->port);
     return WAIT_PROXY;
}

int readline (int s, char *buffer, size_t buffer_size)
{
     char c;
     int i = 0;

     do {
	  if (1 > read(s, &c, 1))
	       return 0;
	  if (i < (buffer_size - 1))
	       buffer[i++] = c;
     } while (c != '\n');
     buffer[i] = 0;

     return i;
}

#define CAYMAN_GREET "Cayman"

int read_cayman (clone_t *clone)
{
     char buf[MEDBUF];
     char ip[MINIBUF];

     memset(buf, 0, sizeof(buf));
     if (!readline(clone->sock, buf, MEDBUF))
	  return EXIT;

     if (!strstr(buf, CAYMAN_GREET))
	  return WAIT_CAYMAN;

     host2ip(ip, clone->host, sizeof(ip));
     send_sock(clone->sock, "telnet %s %d\r\n", ip, clone->port);
     return WAIT_IDENT;
}

#define CISCO_GREET "User Access Verification"
#define CISCO_PWD "cisco"

int read_cisco (clone_t *clone)
{
     char buf[MEDBUF];

     memset(buf, 0, sizeof(buf));
     if (!readline(clone->sock, buf, MEDBUF))
	  return EXIT;

     if (StrCmpPrefix(buf, CISCO_GREET))
	  return WAIT_CISCO;

     send_sock(clone->sock, "%s\n", CISCO_PWD);
     send_sock(clone->sock, "telnet %s %d\n",
	       clone->host, clone->port);
     return WAIT_IDENT;
}

int read_proxy (clone_t *clone)
{
     char buf[MEDBUF];
  
     memset(buf, 0, sizeof(buf));
     if (!readline(clone->sock, buf, MEDBUF))
	  return EXIT;

     if (memcmp(buf, "HTTP/", 5) || memcmp(buf + 9, "200", 3))
     {
	  if (conf.debug) print(0, 2, 0, "[%s;%s] PROXY: %s",
				clone->nick, clone->proxy, buf);
	  return EXIT;
     }

     return WAIT_IDENT;
}

int read_socks4 (clone_t *clone)
{
     char buffer[9];
  
     if (recv(clone->sock, buffer, 8, 0) < 8)
	  return EXIT;

     if (buffer[1] != 0x5A)
     {
	  if (conf.debug) print(1, 2, 0, "[%s;%s] SOCKS4: Connection refused",
				clone->nick, clone->proxy);
	  return EXIT;
     }

     if (conf.debug) print(1, 2, 0, "[%s;%s] SOCKS4: Success",
			   clone->nick, clone->proxy);
     return WAIT_IDENT;
}

int read_socks5 (clone_t *clone)
{
     unsigned char buf[MEDBUF];
     unsigned int packetlen;

     /* consume all of the reply */
     if (recv(clone->sock, buf, 4, 0) < 4)
     {
	  if (conf.debug) print(1, 2, 0, "[%s;%s] SOCKS5: Permission denied",
				clone->nick, clone->proxy);
	  return EXIT;
     }

     if (buf[0] != 5 && buf[1] != 0)
	  return EXIT;

     if (buf[3] == 1)
     {
	  if (recv(clone->sock, buf, 6, 0) != 6)
	       return EXIT;
     }
     else if (buf[3] == 4)
     {
	  if (recv(clone->sock, buf, 18, 0) != 18)
	       return EXIT;
     }
     else if (buf[3] == 3)
     {
	  if (recv(clone->sock, buf, 1, 0) != 1)
	       return EXIT;
	  packetlen = buf[0] + 2;
	  if (recv(clone->sock, buf, packetlen, 0) != packetlen)
	       return EXIT;
     }

     if (conf.debug) print(1, 2, 0, "[%s;%s] SOCKS5: Success",
			   clone->nick, clone->proxy);
     return WAIT_IDENT;
}
