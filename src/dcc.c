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
/* GNU Library General Public License for more details.                 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "globals.h"
#include "control.h"
#include "dcc.h"
#include "string.h"
#include "lists.h"
#include "proxy.h"
#include "print.h"
#include "muhstik.h"
#include "link.h"

extern config_t conf;
extern int help;
extern int maxsock;

dcc_t *dcc[MAX_DCCS];
char *dcc_who[MAX_DCCS];

dcc_t *new_dcc (int sock, char *who)
{
     register int i;
     dcc_t **pdcc;

     for (i=0, pdcc=dcc; *pdcc; ++i, ++pdcc)
	  if (i == MAX_DCCS-1)
	       return NULL;

     (*pdcc) = (dcc_t *)xmalloc(sizeof(dcc_t));
     (*pdcc)->sock = sock;
     (*pdcc)->who = who;
     (*pdcc)->auth = 0;
     (*pdcc)->out = i+2;
     (*pdcc)->start = time(NULL);
     (*pdcc)->status = WAIT_CONNECT;

     return *pdcc;
}

void free_dcc (dcc_t **dcc)
{
     remove_table((*dcc)->who, dcc_who, MAX_DCCS);
     free((*dcc)->who);
     close((*dcc)->sock);
     free(*dcc);
     *dcc = NULL;
}

void adduser (char *who)
{
     char nick[MINIBUF+1];

     if (match_table(who, conf.aop, MAX_AOPS) == -1)
	  add_table(who, conf.aop, MAX_AOPS);

     sscanf(who, "%"MINIBUF_TXT"[^!]", nick);
     if (!occur_table(nick, conf.prot, MAX_PROTS))
	  add_table(nick, conf.prot, MAX_PROTS);

     broadcast(NET_PROTECT, nick);
     broadcast(NET_AUTOOP, who);

     add_table(who, dcc_who, MAX_DCCS);
}

int init_dcc (dcc_t *dcc)
{
     /* Ask for the password */
     send_sock(dcc->sock, DCC1, VERSION);
     return WAIT_PASS;
}

int checkpass (dcc_t *dcc)
{
     char buffer[MEDBUF];

     /* Get the password */
     memset(buffer, 0, sizeof(buffer));
     if (!readline(dcc->sock, buffer, sizeof(buffer)))
     {
	  if (conf.debug) print(1, 0, 0, "DCC Chat: disconnected");
	  return EXIT;
     }

     /* Exit if the password is wrong */
     StrFirstToken(buffer);
     if (strcmp(buffer, conf.dcc_pass))
     {
	  send_sock(dcc->sock, DCC2);
	  return EXIT;      
     }
  
     dcc->auth = 1;
     adduser(dcc->who);

     if (conf.motd) print_motd(dcc->out);
     if (conf.help) usage(dcc->out);

     return WAIT_CMD;
}

void dcc_chat_connect (void *arg)
{
     int sock;
     char *who;
     char buffer[BIGBUF];
     char parm[MINIBUF];
     struct sockaddr_in addr;

     StrCopy(buffer, (char *)arg, sizeof(buffer));
     free(arg);

     /* Initialize the sockaddr structure */
     memset(&addr, 0, sizeof(addr));
     addr.sin_family = AF_INET;

     if (StrParam(parm, sizeof(parm), buffer, 6))
	  return;

     if (!strchr(parm, '.'))
	  addr.sin_addr.s_addr = htonl(strtoul(parm, NULL, 10));
     else if (resolve(parm, &addr.sin_addr))
	  return;

     if (StrParam(parm, sizeof(parm), buffer, 7))
	  return;

     addr.sin_port = htons((unsigned short)strtoul(parm, NULL, 10));

     /* Initialize the socket in IPv4 */
     if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
	  return;
     maxsock = max(maxsock, sock);
     set_nonblocking(sock);
     net_set_socket_options(sock);

     /* Connect to the IRC client */
     if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1)
	  if (errno != EINPROGRESS)
	  {
	       print_error("dcc: connect");
	       close(sock);
	       return;
	  }
  
     /* Save the full address of the client */
     StrFirstToken(buffer);
     who = StrDuplicate(buffer);

     if (!new_dcc(sock, who))
     {
	  print(1, 0, 0, "dcc: no slot available");
	  close(sock);
	  free(who);
     }
}

int read_dcc (dcc_t *dcc)
{
     char buffer[BIGBUF];

     memset(buffer, 0, sizeof(buffer));
     if (!readline(dcc->sock, buffer, sizeof(buffer)))
     {
	  if (conf.debug) print(1, 0, 0, "DCC Chat: disconnected");
	  return EXIT;
     }
     broadcast_raw(buffer);
     interpret(buffer, dcc->out);
     return WAIT_CMD;
}
