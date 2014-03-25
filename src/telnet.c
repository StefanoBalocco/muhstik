#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>

int lookup_host (char *host, struct in_addr *addr)
{
     struct hostent *he;

     he = gethostbyname(host);
     if (he == NULL)
	  return 0;

     memcpy(addr, he->h_addr, he->h_length);
     return 1;
}

int lookup_ip (char *host, struct in_addr *addr)
{
     /* Use inet_addr for portability to Solaris */
     return (((*addr).s_addr = inet_addr(host)) != INADDR_NONE);
}

int resolve (char *host, struct in_addr *addr)
{
     return (lookup_host(host,addr) || lookup_ip(host,addr));
}

int sock;

int main (int argc, char **argv)
{
     struct sockaddr_in addr;
     char *host;
     unsigned short port;
     char buffer[1024];
     FILE *fd;
 
     if (argc < 2)
     {
	  printf("Usage: %s <host> <port>\n", argv[0]);
	  exit(1);
     }

     host = argv[1];
     port = atoi(argv[2]);

     if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
     {
	  perror("socket");
	  exit(1);
     }

     memset(&addr, 0, sizeof(addr));
     addr.sin_family = AF_INET;
     addr.sin_port = htons(port);
     if (!resolve(host, &addr.sin_addr))
     {
	  printf("%s: nslookup failed\n", host);
	  exit(1);
     }
     if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1)
     {
	  perror("connect");
	  exit(1);
     }

     if (!(fd = fdopen(sock, "r")))
     {
	  perror("fdopen");
	  exit(1);
     }
     puts("connected");
     while (fgets(buffer, sizeof(buffer), stdin))
     {
	  send(sock, buffer, strlen(buffer), 0);
	  printf("%s",buffer);
     }

     exit(0);
}
