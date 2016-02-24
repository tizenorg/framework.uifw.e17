Name:       e17
Summary:    The Enlightenment window manager
Version:    1.0.6.001+svn.76808slp2+build25
Release:    1
Group:      System/GUI/Other
License:    BSD 2-clause and Flora-1.1
URL:        http://www.enlightenment.org/
Source0:    %{name}-%{version}.tar.gz
BuildRequires:  pkgconfig(alsa)
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(ecore-con)
BuildRequires:  pkgconfig(ecore-evas)
BuildRequires:  pkgconfig(ecore-file)
BuildRequires:  pkgconfig(ecore-imf)
BuildRequires:  pkgconfig(ecore-imf-evas)
BuildRequires:  pkgconfig(ecore-input)
BuildRequires:  pkgconfig(ecore-input-evas)
BuildRequires:  pkgconfig(ecore-ipc)
BuildRequires:  pkgconfig(ecore-x)
BuildRequires:  pkgconfig(edbus)
BuildRequires:  pkgconfig(edje)
BuildRequires:  pkgconfig(eet)
BuildRequires:  pkgconfig(ehal)
BuildRequires:  pkgconfig(eina)
BuildRequires:  pkgconfig(evas)
BuildRequires:  pkgconfig(eio)
BuildRequires:  pkgconfig(elementary)
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(xext)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(ttrace)
BuildRequires:  pkgconfig(xrender)
BuildRequires:  pkgconfig(xcomposite)
BuildRequires:  pkgconfig(pixman-1)
%if "%{?tizen_profile_name}" != "tv"
BuildRequires:  tzsh-devel
%endif
#BuildRequires:  pkgconfig(display-capture-api)
BuildRequires:  edje-bin
BuildRequires:  embryo-bin
BuildRequires:  eet-bin
BuildRequires:  gettext-devel
Requires(post): e17-data
Requires(post): sys-assert
#Requires: display-capture-api

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
export CFLAGS+=" -fvisibility=hidden -fPIC -Werror-implicit-function-declaration "
export LDFLAGS+=" -fvisibility=hidden -Wl,--hash-style=both -Wl,--as-needed"
export CFLAGS="$CFLAGS -DTIZEN_DEBUG_ENABLE"

%autogen --disable-static
LIBS='-ledbus' %configure --prefix=/usr --disable-static \
%if "%{?tizen_profile_name}" == "tv"
           --enable-virt-res \
%else
           --enable-tzsh \
%endif
           --enable-extra-features

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

# for license notification
mkdir -p %{buildroot}/usr/share/license
cp -a %{_builddir}/%{buildsubdir}/COPYING %{buildroot}/usr/share/license/%{name}
cp -a %{_builddir}/%{buildsubdir}/COPYING %{buildroot}/usr/share/license/%{name}-data
cat %{_builddir}/%{buildsubdir}/COPYING.Flora >> %{buildroot}/usr/share/license/%{name}
cat %{_builddir}/%{buildsubdir}/COPYING.Flora >> %{buildroot}/usr/share/license/%{name}-data


%files
%manifest e17.manifest
%defattr(-,root,root,-)
/usr/bin/enlightenment
/usr/bin/enlightenment_imc
/usr/bin/enlightenment_remote
/usr/bin/enlightenment_start
#%config /usr/etc/enlightenment/sysactions.conf
/usr/share/license/%{name}

%files devel
%defattr(-,root,root,-)
/usr/lib/pkgconfig/enlightenment.pc
/usr/include/enlightenment/*.h

%files data
%manifest e17-data.manifest
%defattr(-,root,root,-)
/etc/smack/accesses.d/e17.efl
/usr/share/license/%{name}-data
#%exclude /usr/etc/xdg/*
%exclude /usr/lib/enlightenment/utils/*
%exclude /usr/share/enlightenment/AUTHORS
%exclude /usr/share/enlightenment/COPYING
%exclude /usr/share/enlightenment/data/themes/*
%exclude /usr/share/enlightenment/data/backgrounds/*
%exclude /usr/share/enlightenment/data/config/*
%exclude /usr/share/enlightenment/data/icons/*
%exclude /usr/share/enlightenment/data/images/*
%exclude /usr/share/enlightenment/data/input_methods/*
%exclude /usr/share/locale/*
%exclude /usr/share/xsessions/*
%exclude /usr/share/applications/enlightenment_filemanager.desktop
%exclude /usr/share/enlightenment/data/flags/*
%exclude /usr/share/enlightenment/data/favorites/*
%exclude /usr/share/enlightenment/data/favorites/.order
%exclude /usr/bin/enlightenment_filemanager

%define _unpackaged_files_terminate_build 0
