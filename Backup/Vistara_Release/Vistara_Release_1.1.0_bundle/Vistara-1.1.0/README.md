# Vistara 1.1.0 (release candidate)

A lightweight Qt 6/C++17 image viewer for Windows and Linux. No gallery indexing or photo database.

## Features
- Browse images with arrow keys and mouse back/forward buttons
- Mouse-wheel zoom, fit to window (`F`), actual size (`1`), fullscreen (`F11`)
- `Esc` exits fullscreen or quits when windowed
- Rotate left/right and save a separate rotated copy (original is preserved)
- Qt OpenGL viewport when available, software fallback
- Asynchronous image decoding

## Building from source

Requires CMake 3.20+, a C++17 compiler, and Qt 6 components Widgets, Network, Concurrent and OpenGLWidgets.

```sh
cmake -S . -B build/Release -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build/Release --parallel
```

Install on Linux via `cmake --install build/Release` (with the needed permissions), or use the packaging scripts in the release bundle.

## Notes
- `.deb` builds should be made on the target Debian/LMDE baseline; `.rpm` builds on the target RPM distribution baseline.
- JPEG copies written by rotation saving may be recompressed and may not retain all metadata.
- Hardware OpenGL availability depends on drivers and Qt platform integration.
- This source archive is a release candidate and still needs building/testing on the intended target distributions.
