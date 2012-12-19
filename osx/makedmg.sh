#!/bin/bash

# Copied and modified from GEdit

pushd $(dirname $0) &>/dev/null

echo "Generating bundle..."
./makebundle.sh

volume_name=frogr

# Build the name of the DMG file
app_version=$(cat ../configure.ac | grep AC_INIT | cut -d "[" -f 3 | cut -d "]" -f 1)
if [ "x$app_version" = "x" ]; then
    echo "No package version for $volume_name was found"
    exit 1;
fi
dmg_filename="$volume_name-$app_version.macosx.intel.dmg"

dmg_app=frogr.app
mount_point=$volume_name.mounted

rm -f $dmg_filename

# Compute an approximated image size in MB, and bloat by 15 MB
image_size=$(du -ck $dmg_app | tail -n1 | cut -f1)
image_size=$((($image_size + 15000) / 1000))

echo "Creating disk image (${image_size}MB)..."
cp template.dmg tmp.dmg
hdiutil resize -size ${image_size}m tmp.dmg

echo "Attaching to disk image..."
hdiutil attach tmp.dmg -readwrite -noautoopen -mountpoint $mount_point -quiet

echo "Populating image..."
cp -r $dmg_app $mount_point

echo "Detaching from disk image..."
hdiutil detach $mount_point -quiet

echo "Converting to final image..."
hdiutil convert -quiet -format UDBZ -o $dmg_filename tmp.dmg
rm tmp.dmg

echo "Done."

popd &>/dev/null
