Frogr as an xdg-app
===================

This directory contains the relevant files to build frogr as an xdg-app.

Requirements:
------------

  * xdg-app >= 0.5
  * xdg-app-builder >= 0.5
  * appstream-composer (automatically run by xdg-app-builder)
  * org.gnome and org.freedesktop Platform and Sdk runtimes

Instructions:
-------------

(1) Install the xdg-app repository for GNOME nightly:
```
  wget -O - http://sdk.gnome.org/apt/debian/conf/alexl.gpg.key|sudo apt-key add -
  xdg-app --user remote-add --gpg-key=nightly.gpg gnome-nightly http://sdk.gnome.org/nightly/repo
```
(2) Install the required runtimes
```
  xdg-app --user install gnome-nightly org.gnome.Platform
  xdg-app --user install gnome-nightly org.gnome.Sdk
  xdg-app --user install gnome-nightly org.freedesktop.Platform
  xdg-app --user install gnome-nightly org.freedesktop.Sdk
```
(3) Build frogr From this directory:
```
  xdg-app-builder --repo=repo frogr.appdir org.gnome.Frogr.json
```
(4) Add a remote to your local repo and install it:
```
  xdg-app --user remote-add --no-gpg-verify frogr-repo repo
  xdg-app --user install frogr-repo org.gnome.Frogr
```
(5) Run frogr as an xdg-app:
```
  xdg-app run org.gnome.Frogr
```

Note that if you do further changes in the `appdir` (e.g. to the metadata), you'll need to re-publish it in your local repo and update before running it again:
```
  xdg-app build-export repo frogr.appdir
  xdg-app --user update org.gnome.Frogr
```

Last, you can bundle frogr to a file with the `build-bundle` subcommand:
```
  xdg-app build-bundle repo frogr.bundle org.gnome.Frogr

Known Issues
------------

For frogr to be useful, it needs to be able to launch your default browser to authorize your flickr account, which won't work at the moment since gtk_show_uri() from inside an xdg-app won't be able to do that until there's a Portal for it.

As a workaround, you can use your distribution's version of frogr to authenticate it from a normal session and then copy the accounts.xml file over to xdg-app's realms:

```
  $ frogr # launch frogr from commandline outside xdg-app, and authorize it
  $ cp ~/.config/frogr/accounts.xml ~/.var/app/org.gnome.Frogr/config/frogr
  $ xdg-app run org.gnome.Frogr  # Now frogr should connect to your account
```
