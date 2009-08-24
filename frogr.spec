Name:		frogr
Version:	0.2~unreleased
Summary:	Flickr Remote Organizer for GNOME
Release:	1%{?dist}

Group:		Applications/Internet
License:	GPLv3
URL:		http://code.google.com/p/frogr
Source0:	http://frogr.googlecode.com/files/%{name}-%{version}.tar.bz2
BuildRoot:	%(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

BuildRequires:	flickcurl-devel, gtk2-devel > 2.12, glib2-devel, libxml2-devel
Requires:	gtk2 > 2.12, glib > 2.16, flickcurl > 1.0, libxml2

%description
Frogr intends to be a complete GNOME application to remotely manage a flickr
account from the desktop. It uses flickcurl, from Dave Beckett to
communicate with the server through the publicly available flickr REST API.

For more information please refer to the Frogr website.

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
%{_datadir}/icons/%{name}.xpm
%{_datadir}/icons/hicolor/*/apps/%{name}.png
%{_datadir}/icons/hicolor/*/apps/%{name}.svg
%{_datadir}/applications/%{name}.desktop
%doc README COPYING AUTHORS THANKS


%changelog
* Mon Aug 24 2009 Mario Sanchez Prada <msanchez at, igalia.com> 0.2~unreleased-1
- Updated for 0.2~unreleased.

* Sat Aug 22 2009 Adrian Perez <aperez at, igalia.com> 0.1.1-1
- First packaged release.
