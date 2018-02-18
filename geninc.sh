#!/bin/bash
# 
#  geninc INPUT1 INPUT2 OUTPUT
#

pushd $(dirname $1)
xxd -i $(basename $1) > $3
xxd -i $(basename $2) >> $3
popd
