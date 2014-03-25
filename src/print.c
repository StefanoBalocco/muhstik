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
#include <stdarg.h>
#include <time.h>

#include "print.h"
#include "dcc.h"
#include "string.h"
#include "lists.h"
#include "link.h"
#include "clone.h"

extern link_t *lk[];
extern config_t conf;
extern int mute;
extern dcc_t *dcc[];
extern char *dcc_who[];
extern char *channel[];
extern char *broth[];
extern clone_t *cl[];
extern const char *strtype[];
extern char *op_nick;
extern int t0;
extern pthread_mutex_t mutex[];

inline void print_error(char *prefix)
{
     print(1, 0, 0, "%s: %s", prefix, strerror(errno));
}

void print_irc (char *buffer, int color, int sock)
{
     static char *color_dcc[] =
	  { "", "\026", "\0032", "\0037", "\0035", "\00314" };

     if (!conf.nodcccolor)
	  send(sock, color_dcc[color], strlen(color_dcc[color]), 0);

     send(sock, buffer, strlen(buffer), 0);
}

void print_dcc (char *buffer, int color, int dest)
{
     dest -= 2;
     if (dcc[dest] && dcc[dest]->auth)
	  print_irc(buffer, color, dcc[dest]->sock);
}

void print_all_dcc (char *buffer, int color)
{
     register int i;
     for (i=0; i<MAX_DCCS; ++i)
	  if (dcc[i] && dcc[i]->auth)
	       print_irc(buffer, color, dcc[i]->sock);
}

void print_privmsg (char *buffer, int color, int dest)
{
     char tosend[BIGBUF];
     dest = -dest-2;
     snprintf(tosend, sizeof(tosend), "PRIVMSG %s :%s", op_nick, buffer);
     print_irc(tosend, color, cl[dest]->sock);
}

void print_console (char *buffer, int ret, int color, int dest)
{
     int len;
     static char *color_term[] =
	  { "\033[0;0m", "\033[0;35m", "\033[0;34m", "\033[0;1;1m", "\033[0;31m", "" };

     if (!conf.nocolor)
     {
	  len = strlen(buffer) - 1;
	  if (buffer[len] == '\n') ret = 1;
	  if (ret) buffer[len] = 0;
	  printf(color_term[color]);
     }

     printf("%s", buffer);

     if (!conf.nocolor)
     {
	  printf(color_term[0]);
	  if (ret) printf("\n");
     }
}

void print (int ret, int color, int dest, const char *fmt, ...)
{
     char buffer[BIGBUF];
     va_list ap;

     if (dest == -1)
	  return;

     va_start(ap, fmt);
     vsnprintf(buffer, sizeof(buffer), fmt, ap);
     va_end(ap);

     if (ret) StrCat(buffer, "\n", sizeof(buffer));

     if (dest >= 2)
     {
	  print_dcc(buffer, color, dest);
	  return;
     }

     if (dest <= -2)
     {
	  print_privmsg(buffer, color, dest);
	  return;
     }

     if (dest == 0)
	  print_all_dcc(buffer, color);

     if (!conf.bg && !mute)
     {
	  pthread_mutex_lock(&mutex[1]);
	  print_console(buffer, ret, color, dest);
	  pthread_mutex_unlock(&mutex[1]);
     }
}

void print_prefix (clone_t *clone, int color, int dest)
{
     print(0, color, dest,
	   "[%s;%s;%s] ",
	   clone->nick,
	   clone->proxy ? clone->proxy : "",
	   clone->server);
}

void print_desc (int out, char *com, char *desc)
{
     register int i;

     print(0, 0, out, "%s", com);
     for (i=strlen(com); i<LEN_TAB; ++i)
	  print(0, 0, out, " ");
     print(1, 0, out, "%s", desc);
}

void print_line (int out)
{
     register int i;

     if (out >= 0)
	  for (i=0; i<LEN_LINE; ++i)
	       print(0, 0, out, "-");
     print(0, 0, out, "\n");
}

void print_motd (int out)
{
     FILE *f;
     char buffer[BIGBUF];

     if ((f=fopen(conf.motd,"r")))
     {
	  while (fgets(buffer, sizeof(buffer), f))
	       print(0, 0, out, "%s", buffer);
	  fclose(f);
     }
}

void usage (int out)
{
     char *(*p)[2];
     static char *desc_general[][2] =
	  {
	       { "help or ?", "print this help" 				     },
	       { STATUS,      "print status (uptime, nicks, channels, users, links)" },
	       { NULL,        NULL                                                   }
	  };
     static char *desc_actions[][2] =
	  {
	       { "join",      "join or rejoin a channel"			     },
	       { "part",      "part a channel"				             },
	       { "quit",      "quit IRC and shut down the bot"			     },
	       { "privmsg",   "send a message or a CTCP to a nick or a channel"      },
	       { NICKS,       "change all clone nicks"                               },
	       { KICKBAN,     "kickban a nick from a channel" 			     },
	       { ECHO,        "make all the clones repeat what one man says" 	     },
	       { TAKEOVER,    "collide nicks on different servers"                   },
	       { SELECT,      "make one clone send something to IRC"                 },
	       { NULL,        NULL                                                   }
	  };
     static char *desc_modes[][2] =
	  {
	       { AGGRESS,     "deop enemys actively and kick them on privmsg" 	     },
	       { PEACE,       "don't automatically deop enemies" 		     },
	       { RANDOM,      "use the wordlist to set the nicks" 		     },
	       { BROADCAST,   "switch the broadcast mode in the command echo"        },
	       { MUTE,        "stop writing to stdout" 		       	             },
	       { NULL,        NULL                                                   }
	  };  
     static char *desc_env[][2] =
	  {
	       { NICKLIST,    "change the active wordlist"                           },
	       { CHANKEY,     "set a key to be used when rejoining a +k channel"     },
	       { PASSWD,      "change the DCC password" 		             },
	       { LOAD,        "load a clone dynamically"                             },
	       { ADDPROT "/" RMPROT, "set/unset a protected nick"                    },
	       { ADDOP   "/" RMOP,   "add/remove a pattern to/from the aop list"     },
	       { ADDSHIT "/" RMSHIT, "add/remove a pattern to/from the shit list"    },
	       { ADDSCAN "/" RMSCAN, "set/unset a scan on join"                      },
	       { NULL,        NULL                                                   }
	  };

     print (1, 0, out, "Available commands:");
     print_line (out);
     for (p=desc_general; **p; ++p)
	  print_desc (out, (*p)[0], (*p)[1]);

     print (1, 0, out, "\n- Actions:");
     for (p=desc_actions; **p; ++p)
	  print_desc (out, (*p)[0], (*p)[1]);

     print (1, 0, out, "%s, %s, %s, %s, %s: "
	    "massop, masskick, masskickban, massdeop, massunban",
	    MASSOP, MASSKICK, MASSKICKBAN, MASSDEOP, MASSUNBAN);
     print (1, 0, out, "kick, mode, topic: IRC protocol");

     print (1, 0, out, "\n- Modes:");
     for (p=desc_modes; **p; ++p)
	  print_desc (out, (*p)[0], (*p)[1]);

     print (1, 0, out, "\n- Environment:");
     for (p=desc_env; **p; ++p)
	  print_desc (out, (*p)[0], (*p)[1]);

     print_line (out);
}

void print_uptime (int out)
{
     int uptime;
     uptime = time(NULL) - t0;
     print(1, 0, out, "muhstik version %s, up for %d days, %d h and %d min.",
	   VERSION, uptime/86400, (uptime/3600)%24, (uptime/60)%60);
}

int nofclones ()
{
     register int i;
     int k=0;
     clone_t **pcl;

     for (i=0, pcl=cl; i<MAX_CLONES; ++i, ++pcl)
	  if (*pcl && (*pcl)->online)
	       ++k;
     for (i=0; i<MAX_BROTHERS; ++i)
	  if (broth[i])
	       ++k;
     return k;
}

int is_empty_str (char **list, int max)
{
     register int i;
     for (i=0; i<max; ++i, ++list)
	  if (*list) return 0;
     return 1;
}

int is_empty_link (link_t **list, int max)
{
     register int i;
     for (i=0; i<max; ++i, ++list)
	  if (*list && (*list)->auth) return 0;
     return 1;
}

int is_empty_scan (clone_t **list, int max)
{
     register int i;
     for (i=0; i<max; ++i, ++list)
	  if (*list && (*list)->scan) return 0;
     return 1;
}

void print_nicks (int out)
{
     char nicks[BIGBUF];
     char **p;
     clone_t **pcl;
     register int i;

     if (!(i = nofclones()))
     {
	  print(1, 0, out, "- no clone online");
	  return;
     }
  
     print(1, 0, out, "- %d clones online:", i);
  
     memset(nicks, 0, sizeof(nicks));

     for (i=0, pcl=cl; i<MAX_CLONES; ++i, ++pcl)
	  if (*pcl && (*pcl)->online)
	  {
	       StrCat(nicks, (*pcl)->nick, sizeof(nicks));
	       StrCat(nicks, " ", sizeof(nicks));
	  }

     for (i=0, p=broth; i<MAX_BROTHERS; ++i, ++p)
	  if (*p)
	  {
	       StrCat(nicks, *p, sizeof(nicks));
	       StrCat(nicks, " ", sizeof(nicks));
	  }

     print(1, 0, out, "%s", nicks);
}

int nops (int chid)
{
     register int i;
     clone_t **pcl;
     int k=0;

     for (i=0, pcl=cl; i<MAX_CLONES; ++i, ++pcl)
	  if (is_op(*pcl, chid))
	       ++k;

     return k;
}

void print_channels (int out)
{
     char **p;
     register int i;
 
     print(1, 0, out, "- channels:");
     for (i=0, p=channel; i<MAX_CHANS; ++i, ++p)
	  if (*p)
	       print(1, 0, out, "%s (%d ops) ", *p, nops(i));
}

void print_active_dcc (int out)
{
     char **p;
     register int i;
  
     print(1, 0, out, "- active DCC:");
     for (i=0, p=dcc_who; i<MAX_DCCS; ++i, ++p)
	  if (*p)
	       print(1, 0, out, "%s", *p);
}

void print_scans (int out)
{
     clone_t **p;
     register int i;

     print(1, 0, out, "- scans:");
     for (i=0, p=cl; i<MAX_CLONES; ++i, ++p)
	  if ((*p) && (*p)->online && (*p)->scan)
	       print(1, 0, out,
		     "[%d] nick=%s, type=%s, "
		     "port=%d, server=%s, saveto=%s",
		     i, (*p)->nick, strtype[(*p)->scan->type],
		     (*p)->scan->proxy_port,
		     (*p)->scan->server, (*p)->scan->save);
}

void print_aops (int out)
{
     char **p;
     register int i;

     print(1, 0, out, "- auto op list:");
     for (i=0, p=conf.aop; i<MAX_AOPS; ++i, ++p)
	  if (*p)
	       print(1, 0, out, "[%d] %s", i, *p);
}

void print_prot (int out)
{
     char **p;
     register int i;

     print(1, 0, out, "- protected nicks:");
     for (i=0, p=conf.prot; i<MAX_PROTS; ++i, ++p)
	  if (*p)
	       print(1, 0, out, "[%d] %s", i, *p);
}

void print_shit (int out)
{
     char **p;
     register int i;

     print(1, 0, out, "- shit list:");
     for (i=0, p=conf.shit; i<MAX_SHITS; ++i, ++p)
	  if (*p)
	  {
	       if (strchr(*p, ':'))
		    print(1, 0, out, "[%d] %s", i, *p);
	       else
		    print(1, 0, out, "[%d] %s :no reason", i, *p);
	  }
}

void print_links (int out)
{
     link_t **p;
     register int i;

     print(1, 0, out, "- links:");
     for (i=0, p=lk; i<MAX_LINKS; ++i, ++p)
	  if (*p && (*p)->auth && (*p)->host)
	       print(1, 0, out, "%s", (*p)->host);
}

void status (int out)
{
     print_line(out);
     print_uptime(out);
     print_nicks(out);

     if (!is_empty_str(channel, MAX_CHANS))
	  print_channels(out);

     if (!is_empty_str(dcc_who, MAX_DCCS))
	  print_active_dcc(out);

     if (!is_empty_scan(cl, MAX_CLONES))
	  print_scans(out);

     if (!is_empty_str(conf.aop, MAX_AOPS))
	  print_aops(out);

     if (!is_empty_str(conf.prot, MAX_PROTS))
	  print_prot(out);

     if (!is_empty_str(conf.shit, MAX_SHITS))
	  print_shit(out);

     if (!is_empty_link(lk, MAX_LINKS))
	  print_links(out);

     print_line(out);
}
