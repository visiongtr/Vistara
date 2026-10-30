# Vistara fullscreen toolbar + icons update

- In fullscreen (F11 or double-click), move the cursor to the top 8 pixels of the window to show the toolbar. Move below the toolbar to hide it.
- In windowed mode, toolbar remains visible as before.
- Esc exits fullscreen, or quits when already windowed.
- Rotate Left / Rotate Right icon artwork is now off-white on transparent backgrounds.
- No changes to image decoding, GPU renderer, or rotation saving.

## Install into your existing Qt Creator source project
Back up your working source first, then copy the source and `icons/` files from this archive over the matching paths in the project. Reconfigure CMake if asked and rebuild. The PNGs are embedded by `vistara.qrc`; an installed .deb/.rpm must be rebuilt to include changes.

Based on the latest Vistara Icon Resource Fix source shared in this conversation. This source was inspected and patched but has not been compiled against Qt in this environment.
