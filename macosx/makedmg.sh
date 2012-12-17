#!/bin/bash

# Copied and modified from GEdit

pushd $(dirname $0) &>/dev/null

echo "Generating bundle..."
./makebundle.sh

export VOLUME_NAME=frogr

if [ "$1x" = "x" ]; then
    export DMG_FILE=$VOLUME_NAME.dmg
else
    export DMG_FILE=$1.dmg
fi

export DMG_APP=frogr.app
export MOUNT_POINT=$VOLUME_NAME.mounted

rm -f $DMG_FILE

# Compute an approximated image size in MB, and bloat by 15 MB
export image_size=$(du -ck $DMG_APP | tail -n1 | cut -f1)
export image_size=$((($image_size + 15000) / 1000))

echo "Creating disk image (${image_size}MB)..."
cp template.dmg tmp.dmg
hdiutil resize -size ${image_size}m tmp.dmg

echo "Attaching to disk image..."
hdiutil attach tmp.dmg -readwrite -noautoopen -mountpoint $MOUNT_POINT -quiet

echo "Populating image..."
cp -r $DMG_APP $MOUNT_POINT

echo "Detaching from disk image..."
hdiutil detach $MOUNT_POINT -quiet

rm -rf $DMG_APP

echo "Converting to final image..."
hdiutil convert -quiet -format UDBZ -o $DMG_FILE tmp.dmg
rm tmp.dmg

echo "Done."

popd &>/dev/null
