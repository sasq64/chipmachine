#!/bin/sh
## uade-1.5 and above Meta input plugin script ##
##  to be used with bmp-meta-input for Beep Mediaplayer Classic, Audacious (?)
##  and Xmms. (http://mitglied.lycos.de/mldoering/bmp-meta-input/)
##
## based on the midi script from xmms meta input plugin (c) by Mikael Boulliot 
## (c) mld/uade team
##
UADEPREFIX="$HOME/.uade2"

# strip "file:" URL tag from path
fname=`echo $2| sed -e s/["file:"]*//`


case $1 in
        play)
		if [ "$3" == "0" ]; then
		  subsong=""
		else
		  subsong="--subsong $3"
		fi
		$UADEPREFIX/uade123 $subsong -e raw  -f /dev/stdout --ignore -k --stderr 2>/dev/null "$fname"
                exit 0
                ;;
        isOurFile)
		if test -x "$UADEPREFIX/uade123"; then
		 if ! [ -z "`$UADEPREFIX/uade123 2>/dev/null --ignore -g "$fname"|grep 'subsong_info:'`" ] ; then
		 exit 0
		 fi
		fi
                exit -1
                ;;
esac
exit -1
