#!/bin/bash
#

build_targets=("auron_paine" "auron_yuna" "candy" "enguarde" "falco" "gandof" \
    "glimmer" "gnawty" "guado" "kip" "leon" "lulu" "monroe" "ninja" "panther" \
    "parrot" "peppy" "quawks" "rikku" "samus" "stumpy" "swanky" "tidus" "wolf");

for device in ${build_targets[@]}
do
    ./build-${device}.sh
    if [ $? -ne 0 ]; then
        echo "Error building for device ${device}; aborting"
        exit
    fi
done
