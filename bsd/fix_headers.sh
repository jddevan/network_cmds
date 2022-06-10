#!/usr/bin/env bash

set -Eeuo pipefail

# restore to original from apple-oss-distributions/xnu
git checkout 79f32e3 .
git restore --staged .

# sys/socket.h
# net/*
# netinet/*
# netinet6/*

DIRS=(net netinet netinet6)
FILES=(sys/socket.h `find ${DIRS[@]} -type f -maxdepth 1`)

fix_header()
{
    sed -i .bak -re 's?^#include <net/([^>]+)>$?#include "../net/\1"?'           \
                 -e 's?^#include <netinet/([^>]+)>$?#include "../netinet/\1"?'   \
                 -e 's?^#include <netinet6/([^>]+)>$?#include "../netinet6/\1"?' $1

    if [[ -e $1.bak ]]; then
        rm $1.bak
    fi
}

for FILE in ${FILES[@]}; do
    echo ${FILE}
    fix_header ${FILE}
done
