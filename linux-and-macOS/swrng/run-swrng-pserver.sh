#!/bin/sh

# A shell script that serves as a named pipe server for sharing random bytes produced by a cluster of one or more SwiftRNG devices.
# Last updated on 31/Jan/2017
# Configure with 'crontab -e' like the following:
# @reboot /usr/local/bin/run-swrng-pserver.sh >> /var/log/run-swrng-pserver.log 2>&1

# Named pipe used.   
PIPEFILE=/tmp/swiftrng

APPDIR=/usr/local/bin

# The application that populates the named pipe with random numbers downloaded from a cluster
# of SwiftRNG devices 
APPNAME=swrng-cl

# SwiftRNG preferred cluster size. It indicates how many devices will be running in parallel to boost performance
# and to ensure high-availability of the cluster. It will still work when using less devices.
CLUSTERSIZE=2
APPCMD="$APPDIR/$APPNAME -dd -fn $PIPEFILE -cs $CLUSTERSIZE "

if [ ! -e "$PIPEFILE" ]; then
 mkfifo $PIPEFILE
fi 

#
# You can tune up the folliwng access rights to ensure the security requirements
#
chmod a+r $PIPEFILE
retVal=$?
 if [ ! $retVal -eq 0 ]
 then
  echo "Cannot set read permission on $PIPEFILE"
  exit 1
 fi

chmod u+w $PIPEFILE
retVal=$?
 if [ ! $retVal -eq 0 ]
 then
  echo "Cannot set write permission on $PIPEFILE"
  exit 1
 fi

if [ ! -e "$APPDIR/$APPNAME" ]; then
 echo "$APPDIR/$APPNAME is not installed. Did you run 'make install' ?"
 exit 1
fi

if [ ! -x "$APPDIR/$APPNAME" ]
then
 echo "File '$APPDIR/$APPNAME' is not executable"
 exit 1
fi
CURRENTDATE=`date +"%Y-%m-%d %T"`

echo "$CURRENTDATE Start looping $APPCMD" 

while :
do
 $APPCMD
 retVal=$?
 if [ ! $retVal -eq 0 ]
 then
   if [ ! $retVal -eq 141 ]
   then
    sleep 15
   fi
 fi
done
