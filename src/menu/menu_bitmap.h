// menu_bitmap.h: the tgz icon as a 32-bit premultiplied-alpha bitmap, the form a menu item shows with transparency.
// Globals: CreateMenuBitmap.
// Doc: docs/notes/src/menu/menu_bitmap.md
#pragma once
#include <windows.h>

// Bitmap at small-icon size from the IDI_TGZ resource of the module, or nullptr. The caller deletes it.
HBITMAP CreateMenuBitmap(HMODULE module);
