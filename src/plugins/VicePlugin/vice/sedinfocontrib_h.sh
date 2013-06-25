#!/bin/sh
#
# sedinfocontrib_h.sh - infocontrib.h filter script
#
# written by Christian Vogelgsang <chris@vogelgsang.org>

# on Macs we need to set the correct locale otherwise the 8 Bit chars in sed generate errors
test -f /usr/bin/uname && if [ `/usr/bin/uname` = "Darwin" ]; then
  export LC_CTYPE=en_US.ISO8859-15
fi

# filter stdin to stdout
# first arg is filter script
exec sed -f "$1"
