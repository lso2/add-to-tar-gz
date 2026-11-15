// coffee_icon.h: the coffee cup glyph (U+2615 from Segoe UI Symbol) drawn into 32-bit images in one color.
// Globals: CreateCoffeeBitmap, CreateCoffeeIcon.
// Doc: docs/notes/src/tray/coffee_icon.md
#pragma once
#include <windows.h>

// A size x size 32-bit DIB with premultiplied alpha, the format of a menu item bitmap. The caller deletes it
// with DeleteObject. nullptr when GDI fails.
HBITMAP CreateCoffeeBitmap(int size, COLORREF color);

// The same image as an icon, for BM_SETIMAGE on a button. The caller deletes it with DestroyIcon. nullptr when
// GDI fails.
HICON CreateCoffeeIcon(int size, COLORREF color);
