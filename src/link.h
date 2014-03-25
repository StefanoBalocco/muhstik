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

#ifndef LINK_H
#define LINK_H

#define MAX_LINKS 128

typedef struct
{
     int sock;
     char auth;
     char *host;
     int status;
     int start;
} link_t;

/* Linking protocol */

#define NET_OFFLINE '0'
#define NET_ONLINE  '1'
#define NET_PROTECT '2'
#define NET_AUTOOP  '3'

void broadcast_raw (char *buffer);
void broadcast (char type, char *msg);

int init_link (link_t *link);
void free_link (link_t **plink);

void init_server ();
void init_client ();
void accept_link (int sock);
int read_pass (link_t *link);
int read_host (link_t *link);
int parse_link (link_t *link);

#endif
