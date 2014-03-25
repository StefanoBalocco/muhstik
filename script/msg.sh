#!/bin/sh

# A name for the script
NAME="[msg]"

. ./common.sh

msg () {
    local period dest message

    echo "Period (in seconds) ?" 1>&2
    read period
    echo "Destination (nick or channel) ?" 1>&2
    read dest
    echo "Enter your message (one line) :" 1>&2
    read message

    init
    while true
      do
      echo "privmsg $dest :$message"
      xsleep $period
    done
}

msg | telnet $HOST $PORT
