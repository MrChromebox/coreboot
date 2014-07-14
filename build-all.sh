#!/bin/bash
#

build_targets=("auron_paine" "auron_yuna" "falco" "gandof" \
    "guado" "leon" "lulu" "mccloud" "monroe" "ninja" \
    "panther" "parrot" "peppy" "rikku" "samus" "stumpy" \
    "tidus" "tricky" "wolf" "zako");

for device in ${build_targets[@]}
do
    ./build-${device}.sh $1
    if [ $? -ne 0 ]; then
        echo "Error building for device ${device}; aborting"
        exit
    fi
done
