%define libver  8.2.2

Name:           libjpeg-turbo
Version:        2.1.2
Release:        1
Summary:        A MMX/SSE2 accelerated library for manipulating JPEG image files
License:        BSD License (BSD 3-clause, Historic Permission Notice and Disclaimer, libjpeg License)
Group:          Graphics & UI Framework/Libraries
Url:            http://sourceforge.net/projects/libjpeg-turbo
Source0:        %{name}-%{version}.tar.gz
Source1001: 	libjpeg-turbo.manifest
BuildRequires:  cmake
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
Obsoletes:      libjpeg-devel < %{version}

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
%cmake . -DCMAKE_BUILD_TYPE=Release -DENABLE_SHARED=TRUE -DENABLE_STATIC=FALSE \
%if "%{tizen_profile_name}" == "tv"
	 -DENABLE_COLOR_PICKER=TRUE \
%endif
	-DWITH_JPEG8=TRUE
%__make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}

%make_install

# Remove unwanted files
rm %{buildroot}%{_bindir}/*

# Remove docs, we'll select docs manually
rm -rf %{buildroot}%{_datadir}/doc/

%post -n libjpeg -p /sbin/ldconfig

%postun -n libjpeg -p /sbin/ldconfig

%docs_package

%files
%manifest %{name}.manifest
%defattr(-,root,root)
%license README.ijg
%license COPYING

%files -n libjpeg
%manifest %{name}.manifest
%defattr(-,root,root)
%{_libdir}/libturbojpeg.so.*
%{_libdir}/libjpeg.so.*
%license README.ijg
%license COPYING

%files -n libjpeg-devel
%defattr(-,root,root)
%{_includedir}/*.h
%{_libdir}/pkgconfig/libturbojpeg.pc
%{_libdir}/pkgconfig/libjpeg.pc
%{_libdir}/libturbojpeg.so
%{_libdir}/libjpeg.so
%{_libdir}/cmake/%{name}/*.cmake
%doc coderules.txt jconfig.txt libjpeg.txt structure.txt example.txt

%changelog
