Name:		frogr
Version:	0.3
Summary:	Flickr Remote Organizer for GNOME
Release:	1%{?dist}

Group:		Applications/Internet
License:	GPLv3
URL:		http://code.google.com/p/frogr
Source0:	http://frogr.googlecode.com/files/%{name}-%{version}.tar.bz2
BuildRoot:	%(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

BuildRequires:	gtk2-devel > 2.12, glib2-devel, libsoup-devel, libxml2-devel
Requires:	gtk2 > 2.12, glib > 2.16, libsoup > 2.24, libxml2

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
%{_datadir}/locale/es/LC_MESSAGES/frogr.mo
%doc README NEWS COPYING AUTHORS THANKS TODO MAINTAINERS TRANSLATORS


%changelog
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
