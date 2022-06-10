#!/usr/bin/env bash

set -Eeuo pipefail

if [[ -z ${1:-} ]]; then
    echo "usage: $0 [dir.tproj]"
    exit 1
fi

FILES=($(find $1 -type f -name '*.c' -or -name '*.h' -maxdepth 1))

fix_include()
{
    sed -i .bak -re 's?^#include <sys/socket.h>?#include "../bsd/sys/socket.h"?'    \
                 -e 's?^#include <sys/sockio.h>?#include "../bsd/sys/sockio.h"?'    \
                 -e 's?^#include <sys/ioctl.h>?#include "../bsd/sys/ioctl.h"?'      \
                 -e 's?^#include <net/([^>]+)>?#include "../bsd/net/\1"?'           \
                 -e 's?^#include <netinet/([^>]+)>?#include "../bsd/netinet/\1"?'   \
                 -e 's?^#include <netinet6/([^>]+)>?#include "../bsd/netinet6/\1"?' $1

    if [[ -e $1.bak ]]; then
        rm $1.bak
    fi

    if fgrep '#define PRIVATE 1' $1 &>/dev/null; then
        echo "$(basename $1): (PRIVATE=1)"
    else
        echo "$(basename $1)"
    fi
}

for FILE in ${FILES[@]}; do
    fix_include ${FILE}
done

# // Define for XNU/BSD headers
# #define PRIVATE 1
