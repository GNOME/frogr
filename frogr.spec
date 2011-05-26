Name:		frogr
Version:	0.6
Summary:	Flickr Remote Organizer for GNOME
Release:	1%{?dist}

Group:		Applications/Internet
License:	GPLv3
URL:		http://live.gnome.org/Frogr
Source0:	http://download.gnome.org/sources/%{name}/%{version}/%{name}-%{version}.tar.bz2
BuildRoot:	%(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

BuildRequires:	gtk3-devel > 3.0, libsoup-devel > 2.24, libxml2-devel > 2.6.8, libexif-devel > 0.6.14
Requires:	gtk3 > 3.0, libsoup, libxml2, libexif, gvfs, desktop-file-utils

%description
Frogr is a small application for the GNOME desktop that allows users
to manage their accounts in the Flickr image hosting website. It
supports all the basic tasks, including uploading pictures, adding
descriptions, setting tags and managing sets.

%prep
%setup -q


%build
%configure
make %{?_smp_mflags}


%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
desktop-file-validate $RPM_BUILD_ROOT/%{_datadir}/applications/%{name}.desktop

%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root,-)
%{_bindir}/%{name}
%{_datadir}/%{name}/*
%{_datadir}/pixmaps/%{name}.xpm
%{_datadir}/icons/hicolor/*/apps/%{name}.png
%{_datadir}/icons/hicolor/*/apps/%{name}.svg
%{_datadir}/applications/%{name}.desktop
%{_datadir}/locale/*/LC_MESSAGES/frogr.mo
%{_mandir}/man1/frogr.1.*
%doc README NEWS COPYING AUTHORS THANKS TODO MAINTAINERS TRANSLATORS


%changelog
* Fri May 27 2011 Mario Sanchez Prada <msanchez at, igalia.com> 0.5-1
- New upstream release
- Allow loading files from remote servers (use URIs instead of paths).
- Show username instead of full name in the UI, which is more useful.
- Allow disabling tags autocompletion through preferences dialog.
- Show tooltips in the icon view with basic info (title, size, date).
- Allow enabling/disabling tooltips in the main window.
- Use check menu items for the account items.
- Better handling of files with UTF-8 characters in their names.
- Show the total amount of data to be uploaded in the status bar.
- Allow sorting pictures by title and date taken (reversed or not).
- Allow copying new photoset's details into the selected pictures.
- Correct photo orientation when needed.
- Added support for generating MacOSX packages.
- Updated marital status in THANKS file.
- Fix compilation errors in some architectures (Alberto Garcia).
- Make GTK3 the default version (opposite to just being supported).
- Introduced new dependency: libexif (> 0.6.14).
- Several other minor improvements and cleanups.
- Fixed bugs: #643288, #643296, #644717, #644797, #644799, #644800,
  #644801, #648597 and #648706.

- New and updated translations: cs, da, de, el, es, fr, gl, pl, pt_BR,
   ru, sl, sv, tr, uk and zh_CN.

* Sat Feb 05 2011 Mario Sanchez Prada <msanchez at, igalia.com> 0.4-1
- Fixed (more) problems building debian packages.
- Modified package description for fedora and debian.
- Converted remaining files in latin1 to utf-8.
- Fixed capitalization problems in translatable strings (Philip Withnall).
- Use Unicode ellipsis instead of three dots (Philip Withnall).
- Added 'Translated by' tab to about dialog.
- Fixed slow startup (no longer wait for albums to be loaded).
- Automatically re-show the authorization dialog if needed.
- More descriptive strings in picture details dialog.
- Implemented new settings dialog, allowing to pre-set default values for the visibility of the pictures.
- Added support for specifying an HTTP proxy.
- Added basic man page (Alberto Garcia).
- Report in the progress dialog when pictures are being added to sets, and make that operation itself cancellable too.
- Better and safer implementation for cancelling operations.
- Allow adding pictures to group pools.
- Allow changing sorting order in 'add to album/group' dialogs.
- Be aware of changes in basic account info on startup.
- Added support for handling multiple accounts.
- Added support for creating new albums right from frogr.
- Retrieve list of tags and use it to auto-complete in entries.
- Do not accept tabs in text views, to allow keyboard navigation.
- Added support for setting 'content type' and 'safety level'.
- Added support to declare pictures to 'show up on global search results'.
- Added mnemonics to the 'Edit details' and the 'Settings' dialog.
- Renamed 'Albums' as 'Sets'.
- Remove pictures from UI as soon as they get uploaded.
- Removed dependency from libgnome when GTK+ < 2.14, and raised minimum version required fo GTK+ from 2.12 up to 2.14.
- Allow compiling with gtk 3.0 by passing --with-gtk=3.0 to configure.
- Better reporting progress to the users for time consuming operations.
- Allow specifying a list of pictures to be loaded from command line.
- Register frogr as image mime types handler, so it's possible to load pictures on it from other applications (e.g. nautilus or eog)
- Added new translations: British English (en_GB), French (fr), Swedish (sv), Galician (gl), German (de), Slovenian (sl), Italian (it), Czech (cs), and Brazilian Portuguese (pt_BR). See TRANSLATORS file for more details.
- As usual, lots of bugfixes and several minor improvements.

* Wed Dec 22 2010 Mario Sanchez Prada <msanchez at, igalia.com> 0.3-1
- Updated for 0.3
- New upstream release
- Replaced flickcurl with flicksoup (asynchronous library)
- Improved authentication process. Made it more seamless
- Better error handling and reporting to the user
- Added support to allow cancelling uploads
- Added support for adding pictures to albums
- Menubar redesigned
- Updated translations
- Lots of cleanups. Simplified code both in frogr and flicksoup
- Lots of bugfixes and minor improvements

* Mon Oct 12 2009 Mario Sanchez Prada <msanchez at, igalia.com> 0.2-1
- Updated for 0.2
- New upstream release
- Added .spec file for fedora
- Added drag'n'drop support for dragging files into the UI
- Added support to 'add tags' to pictures (instead of replacing)
- Added i18n support and es_ES translation
- Support silent build rules
- Lots of bugfixes and minor improvements

* Sat Aug 22 2009 Adrian Perez <aperez at, igalia.com> 0.1.1-1
- First packaged release
- Bugfixing release
- Fixed problems with 'make dist' (missing files in EXTRA_DIST)
