# Vistara rotation update

Copy the contents of this folder into your Vistara source directory, overwriting matching source files. Reload CMake and rebuild in Qt Creator. No new dependencies beyond the previous GPU update are required.

- Rotate Left / Right toolbar buttons, also Ctrl+L / Ctrl+R.
- Rotation is rendered through QGraphicsPixmapItem rather than decoding the picture again.
- Save Rotation… (Ctrl+Shift+S) writes a new file, suggesting a `_rotated` suffix. The original is never overwritten.
- Save is performed in the background. The output format is selected by the extension you enter; unsupported formats produce an error.
- JPEG output is re-encoded at quality 95, **not** lossless. Saving through Qt may strip EXIF and other metadata. Preserve originals for archival use.
- This is a source update, not a verified executable. Build and test using copies of photographs first.
