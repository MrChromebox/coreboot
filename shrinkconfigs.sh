#!/bin/bash

for filename in configs/*/config.*.uefi; do
    [ -e "$filename" ] || continue
    cp "$filename" .config
    make savedefconfig
    cp defconfig "$filename"
done
