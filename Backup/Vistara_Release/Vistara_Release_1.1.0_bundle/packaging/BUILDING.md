# Packaging Vistara 1.1.0

## Debian / LMDE 7 (on your tower)
Use your tested source folder (or extract `Vistara-1.1.0.tar.gz`), and run:

```bash
bash packaging/build-deb.sh /mnt/Projects/Vistara 1.1.0
```

The result is `/mnt/Projects/Vistara/dist/vistara_1.1.0_amd64.deb` on an amd64 system.
Dependencies: CMake, a C++ compiler, Qt 6 development packages (Widgets, Network, Concurrent, OpenGLWidgets), dpkg-dev and dpkg-deb.

## Fedora / RPM
Copy the entire `Vistara_Release_1.1.0` release bundle to a supported RPM-based build machine, then run:

```bash
sudo dnf install rpm-build cmake gcc-c++ qt6-qtbase-devel desktop-file-utils
bash packaging/build-rpm.sh
```

Creates binary and source RPM files in `dist/` if the build succeeds. Distros may have different Qt dependency/package names; validate on each target distribution.

## GitHub
Use `Vistara-1.1.0.tar.gz` / `Vistara-1.1.0-source.zip` as source artifacts, and attach tested binary packages once built.
Verify redistribution licenses for all bundled assets; select and add your desired license to the Git repository.

Do not advertise an RPM as tested on Fedora until it builds and runs there.
