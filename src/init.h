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

#ifndef INIT_H
#define INIT_H

#include "globals.h"

#define USERLISTS 3
typedef enum {
     AOP,
     PROT,
     SHIT
} list_t;

#define PROXYS 8
typedef enum {
     WINGATE,
     SOCKS4,
     SOCKS5,
     PROXY,
     CISCO,
     CAYMAN,
     BOUNCER,
     VHOST,
     NOSPOOF
} proxy_t;

typedef struct {
     int bg;
     int load;
     int group;
     int max_clones;
     int wait_socks;
     int rewind_socks;
     int timeout;
     int max_reco;
     int wait_reco;
     int no_restricted;
     int scan;
     int help;
     int verbose;
     int debug;
     int nocolor;
     int nodcccolor;
     int dalnet;
     int use_wordlist;
     int nick_length;
     int ident_length;
     int real_length;
     int rejoin;
     int multi_op;
     int multi_kick;
     int multi_deop;
     int aggressive;
     int peace;
     int repeat;
     int notice;
     int dcc_filter;
     int link_port;

     char *link_host;
     char *link_pass;
     char *link_file;
     char *dcc_pass;
     char *motd;
     char *batch;

     char *g[PROXYS][MAX_GROUPS];
     FILE *h[PROXYS][MAX_GROUPS];
     FILE *s[PROXYS][MAX_GROUPS];
     FILE *direct[2];

     char *aop[MAX_AOPS];
     char *prot[MAX_PROTS];
     char *shit[MAX_SHITS];
     char *userlist[USERLISTS];

     char *nicks[MAX_NICKS];
     char *idents[MAX_IDENTS];
     char *names[MAX_NAMES];
} config_t;

void init_options (char **argv);
void check_options();
void init_threads();
void init_hostname();

#endif
