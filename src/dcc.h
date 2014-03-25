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

#ifndef DCC_H
#define DCC_H

typedef struct
{
     int sock;
     time_t start;
     char *who;
     int out;
     char auth;
     int status;
} dcc_t;

void dcc_chat_connect (void *arg);
int init_dcc (dcc_t *dcc);
void free_dcc (dcc_t **dcc);
int read_dcc (dcc_t *dcc);
int checkpass (dcc_t *dcc);

#endif
