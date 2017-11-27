%define major   8
%define minor   0
%define micro   2
%define srcver  1.4.2
%define libver  %{major}.%{minor}.%{micro}
# major number of library from jpeg8
%define cmajor  8

Name:           libjpeg-turbo
Version:        %{srcver}
Release:        1
Summary:        A MMX/SSE2 accelerated library for manipulating JPEG image files
License:        BSD License (BSD 3-clause, Historic Permission Notice and Disclaimer, libjpeg License)
Group:          Graphics & UI Framework/Libraries
Url:            http://sourceforge.net/projects/libjpeg-turbo
Source0:        http://downloads.sourceforge.net/project/%{name}/%{version}/%{name}-%{version}.tar.gz
Source1:        baselibs.conf
Source1001: 	libjpeg-turbo.manifest
BuildRequires:  gcc-c++
BuildRequires:  libtool
BuildRequires:  yasm

%description
The libjpeg-turbo package contains a library of functions for manipulating
JPEG images.

%package -n libjpeg

Version:        %{libver}
Release:        0
Summary:        The MMX/SSE accelerated JPEG compression/decompression library
Group:          Graphics & UI Framework/Libraries

Provides:       libjpeg = %{version}
Provides:       libjpeg8
Obsoletes:      libjpeg < %{version}

%description -n libjpeg
This library contains MMX/SSE accelerated functions for manipulating
JPEG images.

%package -n libjpeg-devel
Version:        %{libver}
Release:        0
Summary:        Development Tools for applications which will use the Libjpeg Library
Group:          Graphics & UI Framework/Development

Provides:       libjpeg-turbo-devel
Requires:       libjpeg = %{version}
Provides:       libjpeg-devel = %{version}
Provides:       libjpeg8-devel
Obsoletes:      libjpeg-devel < %{version}
%if "%{major}" != "%{cmajor}"
Conflicts:      libjpeg-devel
%endif

%description -n libjpeg-devel
The libjpeg-devel package includes the header files and libraries
necessary for compiling and linking programs which will manipulate JPEG
files using the libjpeg library.

%prep
%setup -q
cp %{SOURCE1001} .

%build
%if "%{tizen_profile_name}" == "tv"
echo "tizen_product_tv"
export CFLAGS="$CFLAGS -D_TIZEN_PRODUCT_TV -D_USE_PRODUCT_TV"
%endif
autoreconf -fiv
%configure --enable-shared --disable-static --with-jpeg8
make %{?_smp_mflags}

#%check
#make test libdir=%{_libdir}

%install
%makeinstall
# Fix perms
chmod -x README-turbo.txt

# Remove unwanted files
rm -f %{buildroot}%{_libdir}/lib{,turbo}jpeg.la

rm %{buildroot}%{_bindir}/tjbench

# Remove docs, we'll select docs manually
rm -rf %{buildroot}%{_datadir}/doc/

%clean
rm -rf $RPM_BUILD_ROOT

%post -n libjpeg -p /sbin/ldconfig

%postun -n libjpeg -p /sbin/ldconfig

%docs_package

%files
%manifest %{name}.manifest
%defattr(-,root,root)
%license COPYING
%license README

%files -n libjpeg
%manifest %{name}.manifest
%defattr(-,root,root)
%{_libdir}/libturbojpeg.so.*
%{_libdir}/libjpeg.so.%{libver}
%{_libdir}/libjpeg.so.%{major}
%license COPYING
%license README

%files -n libjpeg-devel
%defattr(-,root,root)
%{_includedir}/*.h
%{_libdir}/pkgconfig/turbojpeg.pc
%{_libdir}/libturbojpeg.so
%{_libdir}/libjpeg.so
%doc coderules.txt jconfig.txt libjpeg.txt structure.txt example.c

%changelog
