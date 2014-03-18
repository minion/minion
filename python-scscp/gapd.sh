#!/bin/sh
###########################################################################
##
#W gapd.sh                The SCSCP package             Alexander Konovalov
#W                                                             Steve Linton
##
## gapscscp.sh [-h host] [-a] [-l] [-u] [-p port] [-t]
##
## The following options may be used to overwrite the default method to
## specify the hostname to run GAP SCSCP server, stated in scscp/config.g :
##
## 1) if '-h host' is specified, then the server will be started at 'host'.
##    'host' may be given as machine name with or without domain or even as 
##    'localhost', though we have -l option for that purpose
##
## 2) if '-a' is specified, then the output of the call to 'hostname' will 
##    be used as the SCSCP server address
##
## 3) if '-l' is specified, then the server will be started at localhost 
##    and will not accept any incoming connections from the outside
##
## 4) if '-u' is specified, the server will be started in a "universal"
##    mode and will accept all incoming connections 
## 
## The options 1-4 above are incompatible, so in case several of them will 
## be given, only the option with the biggest number will be used
##
## If none of the options 1-4 above is stated, the hostname for the server
## will be taken from the scscp/config.g file
## 
## Additionally, you may use the following options:
##
## 5) if '-p port' is specified, this will overwrite the default port for 
##    the SCSCP server given in scscp/config.g
##
## 6) if '-t' is specified than the output will be redirected to a 
##    temporary file, which name will be displayed on screen during 
##    startup. Otherwise, by default it will be redirected to /dev/null
##
##
###########################################################################
##
## PART 1. MODIFY PATHS IF NEEDED
##
###########################################################################
##
##  Define the local call of GAP and call options, if necessary, for 
##  example, memory usage, start with the workspace etc. The path may be 
##  relative (to start from this directory) or absolute. 
##  
GAP="gap.sh -b -r"
##
###########################################################################
##
##  Define the configuration file for the SCSCP server. Note that since GAP
##  reads the configuration file immediately before starting SCSCP server,
##  you may redefine in it all variables that were set to their default
##  values in scscp/config.g and scscp/configpar.g files (explicitly or
##  reading your appropriately modified private copies of that files).
##  The path may be relative (to start from this directory) or absolute. 
##
SCSCP_CONFIG="myserver.g"
##
##
###########################################################################

###########################################################################
##
## PART 2. YOU NEED NOT TO MODIFY ANYTHING BELOW
##
###########################################################################
##
##  Parse the arguments.
##
autohost="no"
localhost="no"
unimode="no"
use_temp_file="no"
host=";"
port=";"

option="yes"
while [ $option = "yes" ]; do
  option="no"
  case $1 in

    -a) shift; option="yes"; autohost="yes";;

    -h) shift; option="yes"; host=":=\""$1"\";"; shift;;

    -l) shift; option="yes"; localhost="yes";;
    
    -u) shift; option="yes"; unimode="yes";;

    -p) shift; option="yes"; port=":="$1";"; shift;;

    -t) shift; option="yes"; use_temp_file="yes";;
    
  esac
done

if [ $use_temp_file = "yes" ]; then
	OUTFILE=`mktemp /tmp/gapscscp.XXXXXX`
else
	OUTFILE="/dev/null"
fi;

if [ $autohost = "yes" ]; then
	host=":=Hostname();"
fi;

if [ $localhost = "yes" ]; then
	host=":=false;"
fi;

if [ $unimode = "yes" ]; then
	host=":=true;"
fi;

echo "Starting SCSCP server with output to $OUTFILE" 

# The next line starts GAP SCSCP server. 
# To redirect stderr to /dev/null as well,
# replace $OUTFILE 2>&1 & with $OUTFILE &

echo 'LoadPackage("scscp");SetInfoLevel(InfoSCSCP,0);SCSCPserverAddress'$host'SCSCPserverPort'$port'Read("'$SCSCP_CONFIG'"); if SCSCPserverStatus=fail then QUIT_GAP(); fi;' | exec $GAP > $OUTFILE &

###########################################################################
##
#E
##
