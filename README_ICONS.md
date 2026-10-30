# Vistara icon resource fix

Copy the project source files and the entire `icons/` folder into `/mnt/Projects/Vistara`. Overwrite existing files. Do not copy the older loose root-level PNGs: those are no longer used.

The `vistara.qrc` now explicitly maps the icons/ assets into resource paths like `:/icons/rotate-left.png`, and `Q_INIT_RESOURCE(vistara)` registers them. CMake enables AUTORCC. The fullscreen button uses its own bundled icon, and the application icon is set on both QApplication and the main window.

**After replacing files in Qt Creator:** close the running Vistara process; Build > Clear CMake Configuration, then reconfigure and build. If there is still a stale toolbar, remove ONLY the generated build folder `build/Desktop-Debug` (or whichever configured build you use), not the project source, then reconfigure and build again.

The rotation saving behavior remains unchanged: saving creates a separate rotated copy; JPEG may be recompressed and metadata may be lost.
