#!/bin/bash

for filename in configs/.config.*; do
    [ -e "$filename" ] || continue
    cp "$filename" .config
    make savedefconfig
    cp defconfig "$filename"  
done
