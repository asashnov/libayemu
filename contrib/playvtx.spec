%define name playvtx
%define version 0.9.0
%define release 1

Summary: VTX file format player based on AY/YM emulation library.
Name: %{name}
Version: %{version}
Release: %{release}
Group: Multimedia/Sound
License: GPL
Packager: Sashnov Alexander <sashnov@ngs.ru>
Url: http://sashnov.nm.ru/libayemu.html
Source0: http://sashnov.fanstvo.com/%{name}-%{version}.tar.gz
Source1: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-root
Requires: libayemu >= 0.9.0

%description
This is player for AY/YM sound chip music packed to VTX format.

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

%postun

%files
%defattr(-,root,root)
%doc README TODO COPYING ChangeLog BUGS
%{_bindir}/playvtx

%changelog
* Thu Feb 10 2005 Alexander Sashnov <sashnov@ngs.ru>
  - Start this spec file.
