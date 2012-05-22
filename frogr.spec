Name:		frogr
Version:	0.8
Summary:	Flickr Remote Organizer for GNOME
Release:	0

Group:		Applications/Internet
License:	GPLv3
URL:		http://live.gnome.org/Frogr
Source0:	http://ftp.gnome.org/pub/GNOME/sources/%{name}/0.7/%{name}-%{version}.tar.xz

BuildRequires:	intltool
BuildRequires:	gettext
BuildRequires:	gnome-doc-utils
BuildRequires:	gtk3-devel
BuildRequires:	glib2-devel > 2.22
BuildRequires:	libsoup-devel > 2.26
BuildRequires:	libxml2-devel > 2.6.8
BuildRequires:	libexif-devel > 0.6.14
BuildRequires:	libgcrypt-devel
BuildRequires:	desktop-file-utils
#Explicitly Requires: gvfs since we need to be able to open a web
#browser when associating frogr with a flickr account (this is how
#application/flickr pairing works, through flickr.com)
Requires:	gvfs

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
make install DESTDIR=$RPM_BUILD_ROOT
desktop-file-validate $RPM_BUILD_ROOT/%{_datadir}/applications/%{name}.desktop
%find_lang %{name}


%post
update-desktop-database &> /dev/null || :
touch --no-create %{_datadir}/icons/hicolor &>/dev/null || :


%postun
update-desktop-database &> /dev/null || :
if [ $1 -eq 0 ] ; then
    touch --no-create %{_datadir}/icons/hicolor &>/dev/null
    gtk-update-icon-cache %{_datadir}/icons/hicolor &>/dev/null || :
fi


%posttrans
gtk-update-icon-cache %{_datadir}/icons/hicolor &>/dev/null || :


%files -f %{name}.lang
%defattr(-,root,root,-)
%{_bindir}/%{name}
%{_datadir}/%{name}/*
%{_datadir}/pixmaps/%{name}.xpm
%{_datadir}/icons/hicolor/*/apps/%{name}.png
%{_datadir}/icons/hicolor/*/apps/%{name}.svg
%{_datadir}/applications/%{name}.desktop
%{_datadir}/gnome/help/frogr/*
%{_mandir}/man1/frogr.1.*
%doc README NEWS COPYING AUTHORS THANKS TODO MAINTAINERS TRANSLATORS


%changelog
* Fri May 22 2012 Mario Sanchez Prada <msanchez at, igalia.com> 0.7-0
- New upstream release (see NEWS file for details).
- Package manually generated out of any distro.

* Fri Aug 19 2011 Mario Sanchez Prada <msanchez at, igalia.com> 0.6.1-0
- New upstream release (see NEWS file for details).
- Package manually generated out of any distro.

* Fri Aug 13 2011 Mario Sanchez Prada <msanchez at, igalia.com> 0.6-0
- New upstream release (see NEWS file for details).
- Package manually generated out of any distro.

* Fri May 27 2011 Mario Sanchez Prada <msanchez at, igalia.com> 0.5-1
- New upstream release (see NEWS file for details).
- Package manually generated out of any distro.

* Sat Feb 05 2011 Mario Sanchez Prada <msanchez at, igalia.com> 0.4-1
- New upstream release (see NEWS file for details).
- Package manually generated out of any distro.

* Wed Dec 22 2010 Mario Sanchez Prada <msanchez at, igalia.com> 0.3-1
- New upstream release (see NEWS file for details).
- Package manually generated out of any distro.

* Mon Oct 12 2009 Mario Sanchez Prada <msanchez at, igalia.com> 0.2-1
- New upstream release (see NEWS file for details).
- Package manually generated out of any distro.

* Sat Aug 22 2009 Adrian Perez <aperez at, igalia.com> 0.1.1-1
- First packaged release
- Bugfixing release
- Fixed problems with 'make dist' (missing files in EXTRA_DIST)
- Package manually generated out of any distro.
