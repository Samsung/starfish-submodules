%global OPENSSL_ASM_ENABLED %{?openssl_asm:%openssl_asm}%{!?openssl_asm:ON}
%define openssldir %{_sysconfdir}/ssl

Summary:    Secure Sockets Layer and cryptography libraries and tools
Name:       openssl3
Version:    3.0.8
Release:    0
License:    Apache-2.0
Group:      Security/Crypto Libraries
Url:        https://www.openssl.org/
source0:    https://www.openssl.org/source/${name}-%{version}.tar.gz
Source1:    %{name}.manifest
Requires:   lib%{name} = %{version}-%{release}

%description
The OpenSSL Project is a collaborative effort to develop a robust,
commercial-grade, full-featured, and open source toolkit implementing
the Secure Sockets Layer (SSL v2/v3) and Transport Layer Security (TLS
v1) protocols with full-strength cryptography. The project is managed
by a worldwide community of volunteers that use the Internet to
communicate, plan, and develop the OpenSSL toolkit and its related
documentation.

%package -n lib%{name}
Summary:    Secure Sockets Layer and cryptography libraries
Group:      Security/Crypto Libraries

%description -n lib%{name}
The OpenSSL Project is a collaborative effort to develop a robust,
commercial-grade, full-featured, and open source toolkit implementing
the Secure Sockets Layer (SSL v2/v3) and Transport Layer Security (TLS
v1) protocols with full-strength cryptography. The project is managed
by a worldwide community of volunteers that use the Internet to
communicate, plan, and develop the OpenSSL toolkit and its related
documentation.

%package -n lib%{name}-devel
Summary:    Secure Sockets Layer and cryptography libraries mandatory for Development
Group:      Development/Libraries
Requires:   lib%{name} = %{version}-%{release}
Requires:   zlib-devel
Conflicts:  libopenssl-devel
Conflicts:  libopenssl1.1-devel

%description -n lib%{name}-devel
The OpenSSL Project is a collaborative effort to develop a robust,
commercial-grade, full-featured, and open source toolkit implementing
the Secure Sockets Layer (SSL v2/v3) and Transport Layer Security (TLS
v1) protocols with full-strength cryptography. The project is managed
by a worldwide community of volunteers that use the Internet to
communicate, plan, and develop the OpenSSL toolkit and its related
documentation.

%prep
%setup -q
cp %{SOURCE1} .

%build
%ifarch %{arm}
OPENSSL_ARCH=linux-armv4
%endif
%ifarch aarch64
OPENSSL_ARCH=linux-aarch64
%endif
%ifarch %{ix86}
OPENSSL_ARCH=linux-elf
%endif
%ifarch x86_64
OPENSSL_ARCH=linux-x86_64
%endif
%ifarch riscv64
OPENSSL_ARCH=linux64-riscv64
%endif

RPM_OPT_FLAGS="${RPM_OPT_FLAGS} -std=gnu99 -fPIC -pie"

OPENSSL_CONFIG_ARGS+=" --prefix=%{_prefix} --openssldir=%{openssldir} --libdir=%{_lib} "
OPENSSL_CONFIG_ARGS+=" threads shared no-idea no-rc5 no-camellia enable-md2 enable-weak-ssl-ciphers no-afalgeng "
%if %{OPENSSL_ASM_ENABLED} == OFF
OPENSSL_CONFIG_ARGS+=" no-asm "
%endif

./Configure ${OPENSSL_CONFIG_ARGS} ${OPENSSL_ARCH} ${RPM_OPT_FLAGS}
make %{?_smp_mflags} build_sw

%check
make test

%install
rm -rf ${RPM_BUILD_ROOT}
make DESTDIR=${RPM_BUILD_ROOT} install_sw install_ssldirs
mv ${RPM_BUILD_ROOT}%{openssldir}/openssl.cnf ${RPM_BUILD_ROOT}%{openssldir}/openssl3.cnf

%files
%manifest %{name}.manifest
%license LICENSE.txt
%{_bindir}/openssl
%{_bindir}/c_rehash
%{openssldir}/*.cnf*
%{openssldir}/misc

%files -n lib%{name}
%manifest %{name}.manifest
%license LICENSE.txt
%{_libdir}/*.so.*
%{_libdir}/engines-3/*.so
%{_libdir}/ossl-modules/legacy.so
%post -n lib%{name} -p /sbin/ldconfig
%postun -n lib%{name} -p /sbin/ldconfig

%files -n lib%{name}-devel
%manifest %{name}.manifest
%license LICENSE.txt
%{_includedir}/openssl
%{_libdir}/*.so
%exclude %{_libdir}/*.a
%{_libdir}/pkgconfig/*.pc

