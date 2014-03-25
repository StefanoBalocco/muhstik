######################################
# hostname of the server
HOST=localhost

# port of the server
PORT=7777

# The botnet password
PASS="goodpass"

######################################
CTCP='\001'
B='\002'
C='\003'
R='\026'
U='\037'

# sleep n seconds if n > 0, else exit
xsleep () {
    if [ $1 -le 0 ] || [ $? = 2 ]
	then
	exit
    fi
    sleep $1
}

init () {
    echo "$PASS"
    echo "$NAME"
}

PATH=.:/bin:$PATH
