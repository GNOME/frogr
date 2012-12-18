#!/bin/bash

# Copied and modified from GEdit

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

BUNDLE_APP=frogr.app
FORCE_REMOVAL=false
STRIP_DEBUG=true
while [ $# -gt 0 ]; do
    [ "$1x" = "-fx" ] && FORCE_REMOVAL=true
    [ "$1x" = "-nostripx" ] && STRIP_DEBUG=false
    shift 1
done

if [ -d $BUNDLE_APP ] && $FORCE_REMOVAL; then
    echo "Removing old $BUNDLE_APP bundle..."
    rm -rf $BUNDLE_APP
fi

if [ ! -d $BUNDLE_APP ]; then
    echo "Creating new $BUNDLE_APP bundle..."
    gtk-mac-bundler frogr.bundle
elif $STRIP_DEBUG; then
    echo "Note $BUNDLE_APP bundle already exists, only stripping it..."
else
    echo "Note $BUNDLE_APP bundle already exists. Nothing to do"
fi

if $STRIP_DEBUG; then
    echo "Strip debug symbols from bundle binaries"

    # Strip debug symbols in lib/
    for i in $(find -E $BUNDLE_APP/Contents/Resources -type f -regex '.*\.(so|dylib)$'); do
        do_strip "$i"
    done

    # Strip debug symbols bin/
    if [ -d $BUNDLE_APP/Contents/Resources/bin ]; then
        for i in $(find $BUNDLE_APP/Contents/Resources/bin -type f); do
            if [ -x "$i" ]; then
                do_strip "$i"
            fi
        done
    fi

    # Strip debug symbols in the main binary
    do_strip $BUNDLE_APP/Contents/MacOS/frogr-bin
fi

echo "Bundled created in $BUNDLE_APP!"
