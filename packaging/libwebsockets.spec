Name:       libwebsockets
Summary:    WebSocket Library
Version:    2.3.0
Release:    1
Group:      System/Libraries
License:    LGPL-2.1+ OR BSD-3-Clause
URL:        https://github.com/warmcat/libwebsockets
Source0:    %{name}-%{version}.tar.bz2
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires: zlib-devel
BuildRequires: pkgconfig(openssl1.1)
BuildRequires: openssl1.1
BuildRequires: cmake
BuildRequires: pkgconfig(libsystemd-daemon)

%define _optdeveldir /opt/usr/devel/usr/

%description
C Websockets Server Library

%package devel
Summary:    Development files for %{name}
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
Development files needed for building websocket clients and servers

%prep
%setup -q -n %{name}-%{version}

%build

%cmake -DLWS_WITH_SSL=On -DLWS_WITHOUT_TESTAPPS=ON -DLWS_WITH_SERVER_STATUS=ON

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}

%make_install

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{_libdir}/libwebsockets*.so.*
%license LICENSE

%files devel
%defattr(-,root,root,-)
%{_includedir}/libwebsockets.h
%{_includedir}/lws_config.h
%{_libdir}/libwebsockets.so
%{_libdir}/pkgconfig/*
%{_libdir}/cmake/*
