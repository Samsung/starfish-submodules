Summary:	WebP image codec libraries
Name:		libwebp
Version:	1.1.0
Release:	1
Group:		Multimedia/Libraries
URL:		https://developers.google.com/speed/webp/
License:	BSD-3-clause
Source0:	%{name}-%{version}.tar.gz
Source1:	%{name}.manifest

BuildRequires:  cmake
BuildRequires:  libtool
BuildRequires:  giflib-devel

%description
WebP is an image format that does both lossy and lossless compression of
digital photographic images. WebP consists of a codec based on VP8, that Google
open-sourced in May 2010 and a container based on RIFF. Webmasters, web
developers and browser developers can use WebP to compress, archive and
distribute digital images more efficiently.

%package devel
Summary:	Libraries and header files for libwebp app development
Group:		Development/Libraries
Requires:	%{name} = %{version}-%{release}

%description devel
libwebp-devel contains the Libraries and header files you'll need to develop libwebp applications.
libwebp is an WebP image codec libraries.

%package doc
Summary:	libwebp documentation
Group:		Documentation

%description doc
Documentation for libwebp.

%package tools
Summary:	libwebp tools
Group:		Development/Tools
Requires:	%{name} = %{version}-%{release}

%description tools
libwebp tools.

%prep
%setup -q
cp %{SOURCE1} .

%build
# WEBP_USE_THREAD is added for using multi-thread(pthread)
export CFLAGS="$CFLAGS -D__TIZEN__"
%cmake .
make %{?_smp_mflags}

%install
%make_install

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%manifest %{name}.manifest
%defattr(-,root,root,-)
%license COPYING PATENTS
%{_libdir}/*.so.*

%files devel
%defattr(-,root,root,-)
%license COPYING PATENTS
%{_libdir}/*.so
%{_libdir}/pkgconfig/*.pc
%{_includedir}/webp

%files doc
%{_mandir}/man[1]/[a-z]*

%files tools
%defattr(-,root,root,-)
%{_bindir}/[a-z]*
#config
/usr/share/WebP/cmake/*
