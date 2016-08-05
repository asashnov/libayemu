%define name libayemu
%define version 0.9.5
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
AY/YM sound chip emulation library.

Install libayemu if you want play AY/YM music and sound effect in
console player, xmms plugin or your own games/demos.

%package devel
Summary: AY/YM emulation library headers and static libs
Group:          Development/Libraries

%description devel
This package contains all of the
headers and the static libraries for
libayemu.

You'll only need this package if you
are doing development.

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

%post
ldconfig

%postun
ldconfig

%files
%defattr(-,root,root)
%doc README TODO COPYING ChangeLog
/%{_libdir}/libayemu.so*

%files devel
/usr/include/ayemu*
/%{_libdir}/libayemu.a*

%changelog
* Wed Sep 21 2005 Alexander Sashnov <sashnov@ngs.ru>
  - split package to two: libayemu and libayemu-devel
* Thu Feb 10 2005 Alexander Sashnov <sashnov@ngs.ru>
  - Start rpm spec written

# LANG=C date +"%a %b %d %Y"
