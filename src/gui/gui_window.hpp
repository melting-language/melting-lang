#ifndef GUI_WINDOW_H
#define GUI_WINDOW_H

#include <cstdint>

// Shows the image buffer in a window; blocks until the user closes the window.
// When USE_GUI is defined and SDL2 is linked, this displays the image.
// Called from the imagePreview() built-in when Melt is built with make with-gui.
void runImagePreviewWindow(int width, int height, const uint8_t* rgbData);

#endif
