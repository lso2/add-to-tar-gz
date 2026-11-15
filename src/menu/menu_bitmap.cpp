// menu_bitmap.cpp: draws the tgz icon into a premultiplied ARGB bitmap through buffered paint.
// Globals: none beyond menu_bitmap.h.
// Doc: docs/notes/src/menu/menu_bitmap.md
#include "menu_bitmap.h"
#include "resource_ids.h"
#include <uxtheme.h>

HBITMAP CreateMenuBitmap(HMODULE module)
{
    const int width = GetSystemMetrics(SM_CXSMICON);
    const int height = GetSystemMetrics(SM_CYSMICON);
    HICON icon = static_cast<HICON>(LoadImageW(module, MAKEINTRESOURCEW(IDI_TGZ), IMAGE_ICON, width, height, 0));
    if (icon == nullptr) {
        return nullptr;
    }
    HBITMAP bitmap = nullptr;
    if (SUCCEEDED(BufferedPaintInit())) {
        HDC target = CreateCompatibleDC(nullptr);
        BITMAPINFO info = {};
        info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth = width;
        info.bmiHeader.biHeight = height;
        info.bmiHeader.biPlanes = 1;
        info.bmiHeader.biBitCount = 32;
        info.bmiHeader.biCompression = BI_RGB;
        void* bits = nullptr;
        bitmap = target != nullptr ? CreateDIBSection(target, &info, DIB_RGB_COLORS, &bits, nullptr, 0) : nullptr;
        if (bitmap != nullptr) {
            HGDIOBJ previous = SelectObject(target, bitmap);
            RECT area = {0, 0, width, height};
            BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
            BP_PAINTPARAMS params = {};
            params.cbSize = sizeof(params);
            params.dwFlags = BPPF_ERASE;
            params.pBlendFunction = &blend;
            HDC buffer = nullptr;
            // Every image in tgz.ico has an alpha channel. DrawIconEx writes premultiplied pixels for such images.
            HPAINTBUFFER paint = BeginBufferedPaint(target, &area, BPBF_DIB, &params, &buffer);
            if (paint != nullptr) {
                DrawIconEx(buffer, 0, 0, icon, width, height, 0, nullptr, DI_NORMAL);
                EndBufferedPaint(paint, TRUE);
            }
            SelectObject(target, previous);
        }
        if (target != nullptr) {
            DeleteDC(target);
        }
        BufferedPaintUnInit();
    }
    DestroyIcon(icon);
    return bitmap;
}
