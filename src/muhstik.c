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
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>

#include "muhstik.h"
#include "clone.h"
#include "proxy.h"
#include "control.h"
#include "link.h"
#include "dcc.h"
#include "print.h"
#include "load.h"
#include "string.h"

extern clone_t *cl[];
extern dcc_t *dcc[];
extern link_t *lk[];
extern int server_sock;
extern const char *strtype[];
extern char *channel[];
extern pthread_mutex_t mutex[];
extern pthread_attr_t attr;
extern int mass_ch;
extern int maxsock;
extern config_t conf;
extern char *hostname;

time_t t0;
pid_t pid;

void save_list (char *filename, char **list, int max)
{
     int i;
     FILE *f;

     if ((f=fopen(filename,"w")))
     {
	  for (i=0; i<max; ++i)
	       if (list[i])
		    fprintf(f, "%s\n", list[i]);
	  fclose(f);
     }
}

void main_exit ()
{
     clone_t **pcl;
     int i;

     for (i=0, pcl=cl; i<MAX_CLONES; ++i, ++pcl)
	  if (*pcl && (*pcl)->online)
	       broadcast(NET_OFFLINE, (*pcl)->nick);

     if (conf.userlist[AOP])
	  save_list(conf.userlist[AOP], conf.aop, MAX_AOPS);
     if (conf.userlist[PROT])
	  save_list(conf.userlist[PROT], conf.prot, MAX_PROTS);
     if (conf.userlist[SHIT])
	  save_list(conf.userlist[SHIT], conf.shit, MAX_SHITS);

     if (conf.link_file) unlink(conf.link_file);

     exit(EXIT_SUCCESS);
}

void ctrlc (int sig)
{
     main_exit();
}

void segfault (int sig)
{
     puts("Segmentation fault");
     puts("Please send the following things to mulder@gmx.fr :");
     puts("- the version of muhstik,");
     puts("- your operating system name and version,");
     puts("- when and how the bug released itself (with a screenshot if you can),");
     puts("- your muhstik.conf (so I can see the options you used).");
     exit(EXIT_FAILURE);
}

int main (int argc, char **argv)
{
     pthread_t tid;

     printf("muhstik %s, Copyright (C) 2001-2003, Louis Bavoil\n", VERSION);
     printf("This is free software. See the file LICENSE for more details.\n");

     if (argc < 2)
     {
	  printf("Usage: %s <config file>\n", argv[0]);
	  exit(EXIT_FAILURE);
     }

     signal(SIGTTIN, SIG_IGN);
     signal(SIGTTOU, SIG_IGN);
     signal(SIGALRM, SIG_IGN);
     signal(SIGPIPE, SIG_IGN);
     signal(SIGUSR1, SIG_IGN);
     signal(SIGSEGV, &segfault);
     signal(SIGINT, &ctrlc);

     t0 = time(NULL);
     pid = getpid();
     srandom(pid);

     init_options(argv);
     check_options();
     init_threads();
     init_hostname();

     if (!conf.bg)
     {
	  if (conf.motd) print_motd(1);
	  if (conf.help) usage(1);
     }
     else if ((pid = fork()))
     {
	  printf("Launched into the background (pid: %d)\n", (int)pid);
	  exit(EXIT_SUCCESS);
     }

     if (pthread_create(&tid, &attr, load_all, NULL))
     {
	  puts("pthread_create failed");
	  main_exit();
     }

     if (conf.link_pass)
     {
	  if (conf.link_host) init_client();
	  init_server();
     }

     if (conf.batch) read_batch();

     main_loop();

     return 0;
}

void read_batch ()
{
     FILE *f;
     char buffer[BIGBUF];

     if (!(f = fopen(conf.batch, "r")))
     {
	  print(1, 0, 0, "Cannot open the file '%s'.", conf.batch);
	  return;
     }
     while (1)
     {
	  if (!fgets(buffer, sizeof(buffer), f))
	       return;
	  broadcast_raw(buffer);
	  interpret(buffer, 0);
     }
     fclose(f);
}

clone_t *sock2clone (int sock)
{
     register int i;
     clone_t **pcl;

     for (i=0, pcl=cl; i<MAX_CLONES; ++i, ++pcl)
	  if (*pcl && (*pcl)->sock == sock)
	       return *pcl;

     return NULL;
}

dcc_t *sock2dcc (int sock)
{
     register int i;
     dcc_t **pdcc;

     for (i=0, pdcc=dcc; i<MAX_DCCS; ++i, ++pdcc)
	  if (*pdcc && (*pdcc)->sock == sock)
	       return *pdcc;

     return NULL;
}

link_t *sock2link (int sock)
{
     register int i;
     link_t **plink;

     for (i=0, plink=lk; i<MAX_LINKS; ++i, ++plink)
	  if (*plink && (*plink)->sock == sock)
	       return *plink;

     return NULL;
}

int init_gateway (clone_t *clone)
{
     int ret = EXIT;

     clone->host = clone->server;
     clone->port = clone->server_port;

     switch (clone->type)
     {
     case WINGATE:
	  ret = init_wingate(clone);
	  break;
     case SOCKS4:
	  ret = init_socks4(clone);
	  break;
     case SOCKS5:
	  ret = init_socks5(clone);
	  break;
     case PROXY:
	  ret = init_proxy(clone);
	  break;
     case CISCO:
	  ret = WAIT_CISCO;
	  break;
     case CAYMAN:
	  ret = WAIT_CAYMAN;
	  break;
     }
     return ret;
}

void write_ready (int sok)
{
     clone_t *clone;
     dcc_t *dcc;
     link_t *link;
     int len = sizeof(int);

     if ((clone = sock2clone(sok)))
     {
	  if (getsockopt(sok, SOL_SOCKET, SO_ERROR, &errno, &len))
	  {
	       print_error("getsockopt");
	       clone->status = EXIT;
	       return;
	  }
	  if (errno != 0)
	  {
	       print_error("connect");
	       clone->status = EXIT;
	       return;
	  }
	  if (clone->type != NOSPOOF)
	       print(1, 0, 0, "%s connect()ed: host=%s server=%s",
		     strtype[clone->type], clone->proxy, clone->server);
	  else
	       print(1, 0, 0, "direct connect()ed: server=%s",
		     clone->server);

	  if (clone->type == BOUNCER)
	       clone->status = init_bouncer(clone);
	  else if (clone->type == NOSPOOF || clone->type == VHOST)
	       clone->status = init_irc(clone);
	  else
	       clone->status = init_gateway(clone);

	  return;
     }

     if ((dcc = sock2dcc(sok)))
     {
	  if (getsockopt(sok, SOL_SOCKET, SO_ERROR, &errno, &len))
	  {
	       print_error("getsockopt");
	       dcc->status = EXIT;
	       return;
	  }
	  if (errno != 0)
	  {
	       print_error("connect");
	       dcc->status = EXIT;
	       return;
	  }
	  if (conf.debug) print(1, 0, 0, "DCC Chat: connected");
	  dcc->status = init_dcc(dcc);
	  return;
     }

     if ((link = sock2link(sok)))
     {
	  if (getsockopt(sok, SOL_SOCKET, SO_ERROR, &errno, &len))
	  {
	       print_error("getsockopt");
	       link->status = EXIT;
	       return;
	  }
	  if (errno != 0)
	  {
	       print_error("connect");
	       link->status = EXIT;
	       return;
	  }
	  link->status = init_link(link);
     }
}

int read_irc (clone_t *clone)
{
     char *buf, *line;
     int i, n;

     i = strlen(clone->buffer);
     n = sizeof(clone->buffer) - 1 - i;

     if (recv(clone->sock, clone->buffer + i, n, 0) <= 0)
     {
	  buf = clone->lastbuffer;
	  if (buf[strlen(buf)-1] != '\n')
	       StrCat(buf, "\n", sizeof(clone->lastbuffer));
	  if (parse_deco(clone, buf))
	       return EXIT;
	  return WAIT_CONNECT;
     }
     else
     {
	  buf = clone->buffer;
	  while (strchr(buf,'\n'))
	  {
	       line = strsep(&buf, "\r\n");
	       if (buf == NULL) buf = "";
	       snprintf(clone->lastbuffer, sizeof(clone->lastbuffer), "%s\n", line);
	       parse_irc(clone, clone->lastbuffer);
	  }
	  StrCopy(clone->lastbuffer, clone->buffer, sizeof(clone->lastbuffer));
	  StrCopy(clone->buffer, buf, sizeof(clone->buffer));
     }
     if (conf.no_restricted && clone->restricted)
	  return EXIT;
     return WAIT_IRC;
}

void read_stdin ()
{
     char buffer[BIGBUF];

     memset(buffer, 0, sizeof(buffer));
     if (!fgets(buffer, sizeof(buffer), stdin))
	  return;

     broadcast_raw(buffer);
     interpret(buffer, 1);
}

void read_ready (int sok)
{
     clone_t *clone;
     dcc_t *dcc;
     link_t *link;
     int ret = EXIT;

     if (sok == 0)
     {
	  read_stdin();
	  return;
     }

     if (sok == server_sock)
     {
	  accept_link(sok);
	  return;
     }

     if ((clone = sock2clone(sok)))
     {
	  switch (clone->status)
	  {
	  case WAIT_WINGATE:
	       ret = WAIT_IDENT;
	       break;
	  case WAIT_SOCKS4:
	       ret = read_socks4(clone);
	       break;
	  case WAIT_SOCKS5_1:
	       ret = init_read_socks5(clone);
	       break;
	  case WAIT_SOCKS5_2:
	       ret = read_socks5(clone);
	       break;
	  case WAIT_PROXY:
	       ret = read_proxy(clone);
	       break;
	  case WAIT_CISCO:
	       ret = read_cisco(clone);
	       break;
	  case WAIT_CAYMAN:
	       ret = read_cayman(clone);
	       break;
	  case WAIT_IRC:
	       ret = read_irc(clone);
	       break;
	  }

	  if (ret == WAIT_IDENT)
	       ret = init_irc(clone);

	  clone->status = ret;
	  return;
     }

     if ((dcc = sock2dcc(sok)))
     {
	  switch (dcc->status)
	  {
	  case WAIT_PASS:
	       ret = checkpass(dcc);
	       break;
	  case WAIT_CMD:
	       ret = read_dcc(dcc);
	       break;
	  }
	  dcc->status = ret;
	  return;
     }

     if ((link = sock2link(sok)))
     {
	  switch (link->status)
	  {
	  case WAIT_PASS:
	       ret = read_pass(link);
	       break;
	  case WAIT_HOST:
	       ret = read_host(link);
	       break;
	  case WAIT_CMD:
	       ret = parse_link(link);
	       break;
	  }
	  link->status = ret;
     }
}

void fill_fds (fd_set *rfds, fd_set *wfds)
{
     clone_t **pcl;
     dcc_t **pdcc;
     link_t **plink;
     register int i;
     time_t now;
  
     now = time(NULL);

     if (!conf.bg) FD_SET(0, rfds);
 
     if (server_sock != -1) FD_SET(server_sock, rfds);
 
     for (i=0, pcl=cl; i<MAX_CLONES; ++i, ++pcl)
	  if (*pcl)
	  {
	       if ((*pcl)->rejoin_time > 0)
	       {
		    if (!channel[mass_ch])
			 (*pcl)->rejoin_time = 0;
		    else if ((*pcl)->rejoin_time <= now)
		    {
			 join(*pcl, channel[mass_ch]);
			 (*pcl)->rejoin_time = 0;
		    }
	       }
	       if ((*pcl)->alarm > 0)
	       {
		    if ((*pcl)->alarm > now)
			 continue;
		    (*pcl)->alarm = 0;
	       }
	       switch ((*pcl)->status)
	       {
	       case EXIT:
		    /* Connection closed */
		    free_clone(*pcl);
		    break;
	       case WAIT_CONNECT:
		    if (now-(*pcl)->start > conf.timeout)
		    {
			 /* Connection timeout */
			 free_clone(*pcl);
			 break;
		    }
		    FD_SET((*pcl)->sock, wfds);
		    break;
	       default:
		    FD_SET((*pcl)->sock, rfds);
		    break;
	       }
	  }

     for (i=0, pdcc=dcc; i<MAX_DCCS; ++i, ++pdcc)
	  if (*pdcc)
	       switch ((*pdcc)->status)
	       {
	       case EXIT:
		    free_dcc(pdcc);
		    break;
	       case WAIT_CONNECT:
		    if (now-(*pdcc)->start > conf.timeout)
			 free_dcc(pdcc);
		    else
			 FD_SET((*pdcc)->sock, wfds);
		    break;
	       default:
		    FD_SET((*pdcc)->sock, rfds);
		    break;
	       }

     for (i=0, plink=lk; i<MAX_LINKS; ++i, ++plink)
	  if (*plink)
	       switch ((*plink)->status)
	       {
	       case EXIT:
		    free_link(plink);
		    break;
	       case WAIT_CONNECT:
		    if (now-(*plink)->start > conf.timeout)
			 free_link(plink);
		    else
			 FD_SET((*plink)->sock, wfds);
		    break;
	       default:
		    FD_SET((*plink)->sock, rfds);
		    break;
	       }
}

void main_loop ()
{
     fd_set rfds, wfds;
     register int sok;
     struct timeval tv;

     while (1)
     {
	  FD_ZERO (&rfds);
	  FD_ZERO (&wfds);

	  pthread_mutex_lock(&mutex[0]);
	  fill_fds(&rfds, &wfds);
	  pthread_mutex_unlock(&mutex[0]);

	  tv.tv_sec = 1;
	  tv.tv_usec = 0;

	  if (select(maxsock+1, &rfds, &wfds, NULL, &tv) == -1)
	  {
#ifdef __CYGWIN__
	       puts("WARNING: muhstik is using too many sockets.");
	       puts("Load slower or use a 3.x version.");
	       if (FD_ISSET(maxsock, &wfds) || FD_ISSET(maxsock, &rfds))
		    close(maxsock);
	       maxsock--;
	       continue;
#else
	       print_error("select");
	       main_exit();
#endif
	  }

	  for (sok = 0; sok <= maxsock; ++sok)
	       if (FD_ISSET(sok, &wfds))
	       {
		    pthread_mutex_lock(&mutex[0]);
		    write_ready(sok);
		    pthread_mutex_unlock(&mutex[0]);
	       }

	  for (sok = maxsock; sok >= 0; --sok)
	       if (FD_ISSET(sok, &rfds))
	       {
		    pthread_mutex_lock(&mutex[0]);
		    read_ready(sok);
		    pthread_mutex_unlock(&mutex[0]);
	       }
     }
}
