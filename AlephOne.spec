%define name AlephOne
%define version 20051119
%define release 1

Summary: 3D first-person shooter game
Name: %{name}
Version: %{version}
Release: %{release}
Copyright: GPL
Group: Amusements/Games
Source: %{name}-%{version}.tar.gz
URL: http://www.uni-mainz.de/~bauec002/A1Main.html
BuildRoot: %{_tmppath}/%{name}-root
# not relocatable because the data file packages depend upon the location
# of the data files in this package

Requires: AlephOne-core-data
Requires: SDL >= 1.2.0
Requires: SDL_image >= 1.2.0

%description
Aleph One is an Open Source 3D first-person shooter game, based on the game
Marathon 2 by Bungie Software. It is set in a Sci-Fi universe dominated by
deviant computer AIs and features a well thought-out plot. Aleph One
supports, but doesn't require, OpenGL for rendering.

This package requires additional data -- shape, sound, and map information
-- in order to be installed. One source of this core data is the
AlephOne-minf-demo package.

%prep
%setup -q

%build
CFLAGS=${RPM_OPT_FLAGS} CXXFLAGS=${RPM_OPT_FLAGS} ./configure --prefix=%{_prefix} --mandir=%{_mandir}
if [ -x /usr/bin/getconf ] ; then
  NCPU=$(/usr/bin/getconf _NPROCESSORS_ONLN)
  if [ $NCPU -eq 0 ] ; then
    NCPU=1
  fi
else
  NCPU=1
fi
PARL=$[ $NCPU + 1 ]
make -j $PARL

%install
rm -rf ${RPM_BUILD_ROOT}
make DESTDIR=${RPM_BUILD_ROOT} install-strip

%clean
rm -rf ${RPM_BUILD_ROOT}

%files
%defattr(-,root,root)
%doc AUTHORS COPYING INSTALL.Unix README docs/MML.html docs/Cheat_Codes
%{_bindir}/alephone
%{_datadir}/AlephOne/Fonts
%{_datadir}/AlephOne/MML
%{_datadir}/AlephOne/Themes/Default

%changelog
* Thu Oct  5 2000 Christian Bauer <Christian.Bauer@uni-mainz.de>
- Added docs and theme data files
- Package name and version are set by configure script

* Fri Sep 30 2000 Tom Moertel <tom-rpms-alephone@moertel.com>
- Added a requirement to the base package for AlephOne-core-data
- Split out the Marathon Infinity Demo data into its own package

* Thu Sep 29 2000 Tom Moertel <tom-rpms-alephone@moertel.com>
- Added patch for SDL 1.1.5 SDL_SetClipping incompatability.

* Sat Sep 23 2000 Tom Moertel <tom-rpms-alephone@moertel.com>
- Added Marathon Infinity Demo data to package.
