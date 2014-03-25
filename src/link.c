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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "globals.h"
#include "link.h"
#include "control.h"
#include "proxy.h"
#include "print.h"
#include "muhstik.h"
#include "lists.h"
#include "net.h"
#include "string.h"

extern int maxsock;
extern char *hostname;
extern config_t conf;
extern char *broth[];
extern clone_t *cl[];
extern queue public_queue;

link_t *lk[MAX_LINKS];
char link_master = 1;
int server_sock = -1;

void broadcast_raw (char *buffer)
{
     register int i;
     link_t **plink;

     if (!StrCmpPrefix(buffer, ADDSCAN))
	  return;

     for (i=0, plink=lk; i<MAX_LINKS; ++i, ++plink)
	  if ((*plink) && (*plink)->auth)
	       send((*plink)->sock, buffer, strlen(buffer), 0);
}

void broadcast (char type, char *msg)
{
     char buffer[MEDBUF];
     snprintf(buffer, sizeof(buffer), "%c %s\n", type, msg);
     broadcast_raw(buffer);
}

void free_link (link_t **plink)
{
     if ((*plink)->host)
     {
	  print(1, 1, 0, "Lost link with %s.", (*plink)->host);
	  free((*plink)->host);
     }
     close((*plink)->sock);
     free(*plink);
     *plink = NULL;
}

link_t *new_link (int sock)
{
     register int i;
     link_t **plink;

     for (i=0, plink=lk; *plink; ++i, ++plink)
	  if (i == MAX_LINKS-1)
	       return NULL;

     (*plink) = (link_t *)xmalloc(sizeof(link_t));
     (*plink)->sock = sock;
     (*plink)->host = NULL;
     (*plink)->auth = 0;

     return *plink;
}

int read_pass (link_t *link)
{
     char buffer[BIGBUF];

     memset(buffer, 0, sizeof(buffer));
     if (!readline(link->sock, buffer, sizeof(buffer)))
	  return EXIT;

     StrFirstToken(buffer);
     if (!strcmp(buffer, conf.link_pass))
     {
	  send_sock(link->sock, "%s\n", hostname);
	  return WAIT_HOST;
     }
     return EXIT;
}

int parse_link (link_t *link)
{
     register int i;
     link_t **plink;
     char buffer[BIGBUF];
  
     memset(buffer, 0, sizeof(buffer));
     if (!readline(link->sock, buffer, sizeof(buffer)))
	  return EXIT;

     for (i=0, plink=lk; i<MAX_LINKS; ++i, ++plink)
     {
	  if ((*plink) && (*plink) != link && (*plink)->auth)
	       send((*plink)->sock, buffer, strlen(buffer), 0);
     }

     switch (buffer[0])
     {
     case NET_AUTOOP:
	  if ((sscanf(buffer, "%*c %s", buffer) == 1) &&
	      (match_table(buffer, conf.aop, MAX_AOPS) == -1))
	       add_table(buffer, conf.aop, MAX_AOPS);
	  break;
     case NET_PROTECT:
	  if ((sscanf(buffer, "%*c %s", buffer) == 1) &&
	      (!occur_table(buffer, conf.prot, MAX_PROTS)))
	       add_table(buffer, conf.prot, MAX_PROTS);
	  break;
     case NET_ONLINE:
	  if ((sscanf(buffer, "%*c %s", buffer) == 1) &&
	      (!occur_table(buffer, broth, MAX_BROTHERS)))
	       add_table(buffer, broth, MAX_BROTHERS);
	  break;
     case NET_OFFLINE:
	  if (sscanf(buffer, "%*c %s", buffer) == 1)
	       remove_table(buffer, broth, MAX_BROTHERS);
	  break;
     default:
	  interpret(buffer, -1);
     }
     return WAIT_CMD;
}

void sync_link (int sock)
{
     register int i;

     for (i=0; i<MAX_CLONES; ++i)
	  if (cl[i] && cl[i]->online)
	       send_sock(sock, "%c %s\n", NET_ONLINE, cl[i]->nick);

     for (i=0; i<MAX_BROTHERS; ++i)
	  if (broth[i])
	       send_sock(sock, "%c %s\n", NET_ONLINE, broth[i]);

     for (i=0; i<MAX_PROTS; ++i)
	  if (conf.prot[i])
	       send_sock(sock, "%c %s\n", NET_PROTECT, conf.prot[i]);

     for (i=0; i<MAX_AOPS; ++i)
	  if (conf.aop[i])
	       send_sock(sock, "%c %s\n", NET_AUTOOP, conf.aop[i]);
}

int read_host (link_t *link)
{
     char buffer[BIGBUF];

     memset(buffer, 0, sizeof(buffer));
     if (!readline(link->sock, buffer, sizeof(buffer)))
	  return EXIT;

     StrFirstToken(buffer);
     if (StrCompare(buffer, hostname))
     {
	  print(1, 1, 0, "Link established: %s <-> %s", hostname, buffer);
	  link->host = StrDuplicate(buffer);
	  sync_link(link->sock);
	  link->auth = 1;
	  return WAIT_CMD;
     }
     return EXIT;
}

void accept_link (int sock)
{
     int sock2;
     struct sockaddr_in addr;
     int len = sizeof(addr);
     link_t *link;

     memset(&addr, 0, sizeof(addr));
     if (!(sock2 = accept(sock, (struct sockaddr *)&addr, &len)))
     {
	  print_error("link: accept");
	  close(sock);
	  return;
     }
     if (!(link = new_link(sock2)))
     {
	  print(1, 0, 0, "link: no slot available");
	  close(sock2);
	  return;
     }
     maxsock = max(maxsock, sock2);
     link->status = WAIT_PASS;
}

void init_server ()
{
     int sock;
     struct sockaddr_in addr;

     if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
     {
	  print_error("link: socket");
	  return;
     }

     set_nonblocking(sock);
     net_set_socket_options(sock);

     memset(&addr, 0, sizeof(addr));
     addr.sin_family = AF_INET;
     addr.sin_port = htons(conf.link_port);
     addr.sin_addr.s_addr = htonl(INADDR_ANY);

     if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
     {
	  print_error("link: bind");
	  close(sock);
	  return;
     }
     if (listen(sock, 5) == -1)
     {
	  print_error("link: listen");
	  close(sock);
	  return;
     }
     server_sock = sock;
     maxsock = max(maxsock, server_sock);
}

void init_client ()
{
     struct sockaddr_in addr;
     int sock, port;
     link_t *link;

     if (strchr(conf.link_host,':'))
     {
	  sscanf(conf.link_host, "%*[^:]:%d", &port);
	  sscanf(conf.link_host, "%[^:]", conf.link_host);
     }
     else
     {
	  print(1, 4, 0, "link: %s: error (host:port)", conf.link_host);
	  return;
     }

     if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
     {
	  print_error("link: socket");
	  return;
     }
     set_nonblocking(sock);
     net_set_socket_options(sock);

     memset(&addr, 0, sizeof(addr));
     addr.sin_family = AF_INET;
     addr.sin_port = htons(port);
     if (resolve(conf.link_host, &addr.sin_addr))
     {
	  print(1, 4, 0, "link: %s: nslookup failed", conf.link_host);
	  close(sock);
	  return;
     }
     if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1)
	  if (errno != EINPROGRESS)
	  {
	       print_error("link: connect");
	       close(sock);
	       return;
	  }
     if (!(link = new_link(sock)))
     {
	  print(1, 0, 0, "link: No slot available");
	  close(sock);
	  return;
     }
     maxsock = max(maxsock, sock);
     link->status = WAIT_CONNECT;
     link->start = time(NULL);
}

int init_link (link_t *link)
{
     link_master = 0;
     send_sock(link->sock, "%s\n", conf.link_pass);
     send_sock(link->sock, "%s\n", hostname);
     return WAIT_HOST;
}
