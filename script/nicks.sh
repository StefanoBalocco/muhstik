#!/bin/sh

# A name for the script
NAME="[nicks]"

. ./common.sh

nicks () {
    local period
    
    echo "Period (in seconds) ?" 1>&2
    read period

    init
    while true
      do
      echo nicks
      xsleep $period
    done
}

nicks | telnet $HOST $PORT
