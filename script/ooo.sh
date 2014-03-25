#!/bin/bash

# A name for the script
NAME="[ooo]"

if [ "${BASH_VERSION%%.*}" -lt 2 ]
    then
    echo "This function is only available in bash 2.x"
    read
    exit
fi

source common.sh

ooo () {
    local period dest
    
    echo "Period (in seconds) ?" 1>&2
    read period

    echo "Destination ?" 1>&2
    read dest
    
    declare -a char
    char=(o o O)
    declare -a ctrl
    ctrl=($B $R $U ' #')
    size=512

    init
    while true
      do
      buffer=""
      i=0
      while [ $i -lt $size ]
	do
	s1=""
	n=$((RANDOM%5+1))
	j=0
	while [ $j -lt $n ]
	  do
	  s1=${s1}${char[((RANDOM%3))]}
	  let j=j+1
	done
	s2=${ctrl[$((RANDOM%4))]}
	buffer=${buffer}${s1}${s2}
	let i=i+n+1
      done
      echo -e "privmsg $dest :$buffer"
      xsleep $period
    done
}

ooo | telnet $HOST $PORT
