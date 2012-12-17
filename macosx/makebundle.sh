#!/bin/bash

# Copied and modified from GEdit

if [ -d frogr.app ] && [ "$1x" = "-fx" ]; then
    rm -rf frogr.app
fi

if [ ! -d frogr.app ]; then
    echo "Creating new frogr.app bundle..."
    gtk-mac-bundler frogr.bundle
else
    echo "Note frogr.app bundle already exists, only stripping it..."
fi

function do_strip {
    tp=$(file -b --mime-type "$1")

    if [ "$tp" != "application/octet-stream" ]; then
        return
    fi

    name=$(mktemp -t bundle)
    st=$(stat -f %p "$1")

    strip -o "$name" -S "$1"
    mv -f "$name" "$1"

    chmod "$st" "$1"
    chmod u+w "$1"
}

echo "Strip debug symbols from bundle binaries"

# Strip debug symbols
for i in $(find -E frogr.app/Contents/Resources -type f -regex '.*\.(so|dylib)$'); do
    do_strip "$i"
done

if [ -d frogr.app/Contents/Resources/bin ]; then
    for i in $(find frogr.app/Contents/Resources/bin -type f); do
        if [ -x "$i" ]; then
            do_strip "$i"
        fi
    done
fi

do_strip frogr.app/Contents/MacOS/frogr-bin
