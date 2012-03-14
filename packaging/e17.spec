Name:       e17
Summary:    The Enlightenment window manager
Version:    1.0.0.001+svn.65871slp2
Release:    1.1
Group:      System/GUI/Other
License:    BSD
URL:        http://www.enlightenment.org/
Source0:    %{name}-%{version}.tar.gz
BuildRequires:  pkgconfig(alsa)
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(ecore-con)
BuildRequires:  pkgconfig(ecore-evas)
BuildRequires:  pkgconfig(ecore-fb)
BuildRequires:  pkgconfig(ecore-file)
BuildRequires:  pkgconfig(ecore-imf)
BuildRequires:  pkgconfig(ecore-imf-evas)
BuildRequires:  pkgconfig(ecore-input)
BuildRequires:  pkgconfig(ecore-input-evas)
BuildRequires:  pkgconfig(ecore-ipc)
BuildRequires:  pkgconfig(ecore-x)
BuildRequires:  pkgconfig(edbus)
BuildRequires:  pkgconfig(elementary)
BuildRequires:  pkgconfig(edje)
BuildRequires:  pkgconfig(eet)
BuildRequires:  pkgconfig(efreet)
BuildRequires:  pkgconfig(efreet-mime)
BuildRequires:  pkgconfig(efreet-trash)
BuildRequires:  pkgconfig(ehal)
BuildRequires:  pkgconfig(eina)
BuildRequires:  pkgconfig(evas)
BuildRequires:  pkgconfig(utilX)
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(xext)
BuildRequires:  edje-bin
BuildRequires:  embryo-bin
BuildRequires:  eet-bin
BuildRequires:  gettext-devel


%description
The Enlightenment DR17 Window Manager Enlightenment is an advanced window manager for X11. Unique
 features include: a fully animated background, nice drop shadows
 around windows, backed by an extremely clean and optimized
 foundation of APIs.
 .
 This package contains the core files for Enlightenment DR17.



%package devel
Summary:    The Enlightenment window mgr (devel)
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
The Enlightenment window manager (devel)

%package data
Summary:    The Enlightenment window mgr (data)
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description data
The Enlightenment window manager (data)


%prep
%setup -q


%build

export CFLAGS+=" -fvisibility=hidden -fPIC -DDO_NOT_USE_SIG_HANDLER"
export LDFLAGS+=" -fvisibility=hidden -Wl,--hash-style=both -Wl,--as-needed"

%autogen --disable-static
LIBS='-ledbus' ./configure --prefix=/usr --disable-static \
    --enable-extra-features \
    --disable-clock \
    --disable-temperature \
    --disable-mixer \
    --disable-everything \
    --disable-ibar \
    --disable-dropshadow \
    --disable-pager \
    --disable-battery \
    --disable-cpufreq \
    --disable-ibox \
    --disable-start \
    --disable-exebuf \
    --disable-winlist \
    --disable-fileman \
    --disable-fileman-opinfo \
    --disable-wizard \
    --disable-conf \
    --disable-conf-wallpaper \
    --disable-conf-wallpaper2 \
    --disable-conf-theme \
    --disable-conf-colors \
    --disable-conf-fonts \
    --disable-conf-borders \
    --disable-conf-icon-theme \
    --disable-conf-mouse-cursor \
    --disable-conf-transitions \
    --disable-conf-startup \
    --disable-conf-intl \
    --disable-conf-imc \
    --disable-conf-profiles \
    --disable-msgbus-lang \
    --disable-conf-engine \
    --disable-conf-applications \
    --disable-conf-desks \
    --disable-conf-desk \
    --disable-conf-display \
    --disable-conf-desklock \
    --disable-conf-screensaver \
    --disable-conf-dpms \
    --disable-conf-shelves \
    --disable-conf-shelves \
    --disable-conf-keybindings \
    --disable-conf-mousebindings \
    --disable-conf-edgebindings \
    --disable-conf-mouse \
    --disable-conf-window-display \
    --disable-conf-window-focus \
    --disable-conf-window-remembers \
    --disable-conf-window-manipulation \
    --disable-conf-menus \
    --disable-conf-clientlist \
    --disable-conf-dialogs \
    --disable-conf-performance \
    --disable-conf-winlist \
    --disable-conf-exebuf \
    --disable-conf-paths \
    --disable-conf-mime \
    --disable-conf-interaction \
    --disable-conf-scale \
    --disable-gadman \
    --disable-mixel \
    --disable-connman \
    --disable-illume \
    --disable-syscon \
    --disable-bluez \
    --disable-ofono \
    --disable-msgbus \
    --disable-systray \
    --disable-conf_acpibindings \
    --disable-everything-apps \
    --disable-everything-aspell \
    --disable-everything-calc \
    --disable-everything-files \
    --disable-everything-settings \
    --disable-everything-windows \
    --disable-illume-bluetooth \
    --disable-illume-home \
    --disable-illume-toggle \
    --disable-illume-indicator \
    --disable-illume-kbd-toggle \
    --disable-illume-keyboard \
    --disable-illume-mode-toggle \
    --disable-illume-softkey \
    --disable-comp \
    --disable-illume2 \
    --disable-notification \
    --disable-conf_randr \
    --disable-tasks \
    --disable-backlight
#    --disable-backlight \ 
#    --disable-shot

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install
#make install

%files 
%defattr(-,root,root,-)
/usr/etc/*
/usr/bin/*
/usr/lib/enlightenment/*

%files devel
%defattr(-,root,root,-)
/usr/lib/pkgconfig/enlightenment.pc
/usr/include/*

%files data 
%defattr(-,root,root,-)
/usr/share/*
