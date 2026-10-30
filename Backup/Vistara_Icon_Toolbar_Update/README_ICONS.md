# Vistara — Icon toolbar update

This archive replaces the previous rotation update. Copy **all files from the ZIP root** into the existing Vistara source folder, replacing matching files; reload CMake, then rebuild in Qt Creator. Do not copy a nested working folder.

Changes: icon-only toolbar, 22px icons, new bundled PNG assets for Rotate Left, Rotate Right, Save Rotation, Fit, 100%. Existing Open/Previous/Next/Fullscreen icons retained. Hover tooltips and keyboard shortcuts preserved. No new Qt dependencies.

Save Rotation still creates a separate copy; JPEG is recompressed and some metadata may be lost. Test with copies first.
