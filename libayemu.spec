%define name libayemu
%define version 0.9.0
%define release 1

Name:    %{name}
Summary: AY/YM emulation library 
Version: %{version}
Release: 1
License: GPL
Group: System Environment/Libraries
Source0: http://sashnov.fanstvo.com/%{name}-%{version}.tar.gz
Source1: %{name}-%{version}.tar.gz
Packager: Sashnov Alexander <sashnov@ngs.ru>
URL: http://sashnov.nm.ru/libayemu.html
BuildRoot: %{_tmppath}/%{name}-root
Requires: /sbin/ldconfig

%description
AY/YM sound chip (from ZX-Spectrum) emulation library.

Install libayemu if you want play AY/YM music and sound effect in
console player, xmms plugin or your own games/demos.

%prep
%setup

%build
%configure
%make

%install
rm -rf %{buildroot}
%makeinstall

%clean
rm -rf %{buildroot}

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root)
%doc README TODO COPYING ChangeLog
/usr/include/ayemu*
/%{_libdir}/libayemu.*

%changelog
* Thu Feb 10 2005 Alexander Sashnov <sashnov@ngs.ru>
  - Start rpm spec written
# date +"%a %b %d %Y"
