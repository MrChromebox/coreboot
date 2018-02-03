#!/bin/bash
#

set -e

filename="coreboot_tiano-fizz-vsn_`date +"%Y%m%d"`.rom"
rm -f ~/dev/firmware/${filename}*
rm -rf ./build
cp configs/.config.fizz.uefi .config
make
cp ./build/coreboot.rom ./${filename}
cbfstool ${filename} print
sha1sum ${filename} > ${filename}.sha1
mv ${filename}* ~/dev/firmware/

