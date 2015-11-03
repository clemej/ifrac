#
#  ifrac RPM spec file
#  Created 19-Sep-00 08:29
#  Modified 17-Nov-04 18:10
#  by Michael Glickman
#

%define version 1.4.0
%define prefix /usr/local
%define man_prefix /usr/local/man/man6

Summary: Intelligent FRAC - an easy to play 3D packing game  
Name: ifrac
Version: %{version}
Release: 1
Copyright: Michael Glickman <xifrac@yahoo.com.au>
Group:  Applications/Games
Source: %{name}-%{version}.tar.gz 
URL: http://ifrac.tripod.com

%description
The game was originally implemented for MS DOS by
Simsalabim Software (Max Shapiro & Per Bergland).
The Linux release is available in svgalib and X11
implementations, distributed as a single package.
Features: supports all colour modes from 4 to 32bpp;
full screen mode (using Xf86-DGA 2.0);
demo mode (plays itself).
Optional features: joystick, background music
(choose your favourite tune), volume and balance control,
world wide score submission.

%prep

%setup
./configure --with-batch --with-cfontdir=/usr/lib/kbd/consolefonts --with-mangz=yes

%build
make

%install
make install

%files
%doc README
%doc INSTALL
%doc LICENSE
%doc LICENSE.OLD
%doc VERSION
%doc ifrac.bgl
%doc ifrac.lsm
%{prefix}/bin/%{name}
%{prefix}/bin/x%{name}
%{prefix}/share/pixmaps/ifrac_ico.xpm
%{prefix}/share/pixmaps/ifrac_ico_big.xpm
%{man_prefix}/%{name}.6.gz
%{man_prefix}/x%{name}.6.gz
/var/games/ifrac.scores


%clean

%changelog
* Mon Nov 17 2003 Michael Glickman
- v 1.4.0 
- added (limited) mouse support
- fixed bugs

