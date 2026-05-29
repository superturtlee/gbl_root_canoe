Summary: The ABL Source for native building
Name: abl
Version: 1.0
Release: 1%{?dist}
Source0: %{name}-%{version}.tar.gz
License: BSD-2-Clause-Patent
Group: Development/Tools

Requires(post): info
Requires(preun): info

%description
The ABL Source, capable of getting built natively on the Qdrive3.0 target

%prep
%setup -q

%build
make BUILD_NATIVE_AARCH64=true SIGN_ABL_IMAGE=true
cp -f signed_abl/sdm855/abl.elf /
