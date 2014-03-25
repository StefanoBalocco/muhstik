#!/bin/sh

# The name of this script
NAME="[ctcp]"

. ./common.sh

ctcp () {
    local period dest ctcp

    echo "Period (in seconds) ?" 1>&2
    read period
    echo "Destination (nick or channel) ?" 1>&2
    read dest
    echo "CTCP (PING, VERSION, etc) ?" 1>&2
    read ctcp
    
    init
    while true
      do
      echo -e "privmsg $dest :${CTCP}${ctcp}${CTCP}"
      xsleep $period
    done
}

ctcp | telnet $HOST $PORT
