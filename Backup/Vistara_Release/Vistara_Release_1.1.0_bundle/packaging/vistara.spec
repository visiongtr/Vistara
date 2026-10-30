Name:           vistara
Version:        1.1.0
Release:        1%{?dist}
Summary:        Lightweight Qt image viewer
License:        LicenseRef-Proprietary
URL:            https://github.com/visiongtr
Source0:        Vistara-%{version}.tar.gz

BuildRequires:  gcc-c++
BuildRequires:  cmake
BuildRequires:  qt6-qtbase-devel
BuildRequires:  desktop-file-utils

%description
Lightweight Qt image viewer with OpenGL-assisted display, navigation,
and save-to-copy rotation.

%prep
%autosetup -n Vistara-%{version}

%build
%cmake -DCMAKE_BUILD_TYPE=Release
%cmake_build

%install
%cmake_install

%check
desktop-file-validate %{buildroot}%{_datadir}/applications/vistara.desktop

%files
%{_bindir}/Vistara
%{_datadir}/applications/vistara.desktop
%{_datadir}/icons/hicolor/256x256/apps/vistara.png

%changelog
* Wed Oct 28 2026 Vistara Project <noreply@example.invalid> - 1.1.0-1
- Initial Linux packaging candidate
