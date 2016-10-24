Frogr as an flatpak application
===============================

This directory contains the relevant files to build frogr as an flatpak application.

Requirements:
------------

  * flatpak >= 0.6.7
  * flatpak-builder >= 0.6.7
  * xdg-desktop-portal >= 0.1
  * xdg-desktop-portal-gtk >= 0.1
  * appstream-composer (automatically run by flatpak-builder)
  * org.gnome Platform and Sdk runtimes >= 3.22

Instructions:
-------------

(1) Install the flatpak repository for GNOME nightly:
```
  wget https://sdk.gnome.org/keys/gnome-sdk.gpg
  flatpak --user remote-add --gpg-import=gnome-sdk.gpg gnome http://sdk.gnome.org/repo
```
(2) Install the required runtimes
```
  flatpak --user install gnome org.gnome.Platform 3.22
  flatpak --user install gnome org.gnome.Sdk 3.22
```
(3) Build frogr From this directory:
```
  flatpak-builder --force-clean --ccache --require-changes --repo=repo frogr.appdir org.gnome.frogr.json
```
(4) Add a remote to your local repo and install it:
```
  flatpak --user remote-add --no-gpg-verify frogr-repo repo
  flatpak --user install frogr-repo org.gnome.frogr
```
(5) Run frogr as an flatpak:
```
  flatpak run org.gnome.frogr
```

Note that if you do further changes in the `appdir` (e.g. to the metadata), you'll need to re-publish it in your local repo and update before running it again:
```
  flatpak build-export repo frogr.appdir
  flatpak --user update org.gnome.frogr
```

Last, you can bundle frogr to a file with the `build-bundle` subcommand:
```
  flatpak build-bundle repo org.gnome.frogr.flatpak org.gnome.frogr
```
