Name:       libwebsockets
Summary:    WebSocket Library
Version:    4.3.2
Release:    0
Group:      System/Libraries
License:    MIT with exceptions
URL:        https://github.com/warmcat/libwebsockets
Source0:    %{name}-%{version}.tar.gz
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires: zlib-devel
BuildRequires: pkgconfig(openssl1.1)
BuildRequires: openssl1.1
BuildRequires: cmake

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

%cmake -DLWS_WITH_SSL=On \
	-DLWS_WITHOUT_TESTAPPS=ON \
	-DLWS_WITH_SERVER_STATUS=ON \
	-DLWS_IPV6=ON \
	-DLWS_WITH_SO_BINDTODEVICE=ON \
	-DLWS_WITH_HTTP2=OFF\
	-DLWS_WITHOUT_EXTENSIONS=OFF \
	-DLWS_HTTP_HEADERS_ALL=ON \
	.

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
%{_includedir}/libwebsockets/*
%{_libdir}/libwebsockets.so
%{_libdir}/pkgconfig/*
%{_libdir}/cmake/*
