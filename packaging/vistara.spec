Name:           vistara
Version:        1.2.0
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
A lightweight Qt viewer with OpenGL-assisted rendering and non-destructive rotation saves.

%prep
%autosetup -n Vistara-%{version}

%build
%cmake -DCMAKE_BUILD_TYPE=Release
%cmake_build

%install
%cmake_install
install -Dm644 vistara.desktop %{buildroot}%{_datadir}/applications/vistara.desktop
install -Dm644 icons/vistara_icon_256.png %{buildroot}%{_datadir}/icons/hicolor/256x256/apps/vistara.png

%check
desktop-file-validate %{buildroot}%{_datadir}/applications/vistara.desktop

%files
%{_bindir}/Vistara
%{_datadir}/applications/vistara.desktop
%{_datadir}/icons/hicolor/256x256/apps/vistara.png
