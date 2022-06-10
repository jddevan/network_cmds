#!/usr/bin/env bash

set -Eeuo pipefail

if [[ -z ${1:-} ]]; then
    echo "usage: $0 [dir.tproj]"
    exit 1
fi

FILES=($(find $1 -type f -name '*.c' -or -name '*.h'))

fix_include()
{
    sed -i .bak -re 's?^#include <sys/socket.h>$?#include "../bsd/sys/socket.h"?'    \
                 -e 's?^#include <net/([^>]+)>$?#include "../bsd/net/\1"?'           \
                 -e 's?^#include <netinet/([^>]+)>$?#include "../bsd/netinet/\1"?'   \
                 -e 's?^#include <netinet6/([^>]+)>$?#include "../bsd/netinet6/\1"?' $1

    if [[ -e $1.bak ]]; then
        rm $1.bak
    fi
}

for FILE in ${FILES[@]}; do
    echo $(basename ${FILE})
    fix_include ${FILE}
done
