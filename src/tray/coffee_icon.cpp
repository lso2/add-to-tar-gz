// coffee_icon.cpp: draws the cup glyph at 4 times the size and averages it down to a coverage mask, then colors
// the mask.
// Globals: none beyond coffee_icon.h.
// Doc: docs/notes/src/tray/coffee_icon.md
#include "coffee_icon.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace {

constexpr wchar_t kCup = 0x2615;
constexpr wchar_t kFontName[] = L"Segoe UI Symbol";
// GDI's grayscale antialiasing is coarse at 16 pixels. Averaging 4 x 4 samples gives 17 coverage levels.
constexpr int kScale = 4;

// A top-down 32-bit DIB section and its pixels.
HBITMAP CreateDib(int width, int height, unsigned** pixels)
{
    BITMAPINFO info = {};
    info.bmiHeader.biSize = sizeof(info.bmiHeader);
    info.bmiHeader.biWidth = width;
    info.bmiHeader.biHeight = -height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    void* bits = nullptr;
    HBITMAP bitmap = CreateDIBSection(nullptr, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
    *pixels = static_cast<unsigned*>(bits);
    return bitmap;
}

HFONT CreateCupFont(int pixelHeight)
{
    LOGFONTW font = {};
    font.lfHeight = -pixelHeight;
    font.lfWeight = FW_NORMAL;
    font.lfCharSet = DEFAULT_CHARSET;
    font.lfQuality = ANTIALIASED_QUALITY;
    wcscpy_s(font.lfFaceName, kFontName);
    return CreateFontIndirectW(&font);
}

// The cup's coverage of each pixel of a size x size box, 0 to 255. The glyph's ink box is scaled to 15/16 of the
// box and centered in it. Empty when GDI fails.
std::vector<unsigned char> CupCoverage(int size)
{
    std::vector<unsigned char> coverage;
    const int big = size * kScale;
    unsigned* pixels = nullptr;
    HBITMAP canvas = CreateDib(big, big, &pixels);
    HDC dc = CreateCompatibleDC(nullptr);
    HFONT measureFont = CreateCupFont(big);
    if (canvas == nullptr || dc == nullptr || measureFont == nullptr) {
        if (measureFont != nullptr) {
            DeleteObject(measureFont);
        }
        if (dc != nullptr) {
            DeleteDC(dc);
        }
        if (canvas != nullptr) {
            DeleteObject(canvas);
        }
        return coverage;
    }
    std::fill(pixels, pixels + static_cast<size_t>(big) * big, 0u);
    HGDIOBJ originalBitmap = SelectObject(dc, canvas);
    HGDIOBJ originalFont = SelectObject(dc, measureFont);
    const MAT2 identity = {{0, 1}, {0, 0}, {0, 0}, {0, 1}};
    GLYPHMETRICS metrics = {};
    HFONT drawFont = nullptr;
    if (GetGlyphOutlineW(dc, kCup, GGO_METRICS, &metrics, 0, nullptr, &identity) != GDI_ERROR &&
        metrics.gmBlackBoxX > 0 && metrics.gmBlackBoxY > 0) {
        const int ink = (std::max)(static_cast<int>(metrics.gmBlackBoxX), static_cast<int>(metrics.gmBlackBoxY));
        drawFont = CreateCupFont(MulDiv(big, big * 15 / 16, ink));
    }
    bool drawn = false;
    TEXTMETRICW text = {};
    if (drawFont != nullptr) {
        SelectObject(dc, drawFont);
        if (GetGlyphOutlineW(dc, kCup, GGO_METRICS, &metrics, 0, nullptr, &identity) != GDI_ERROR &&
            GetTextMetricsW(dc, &text)) {
            // TextOutW places the top of the character cell at y, and the baseline is tmAscent below it.
            const int x = (big - static_cast<int>(metrics.gmBlackBoxX)) / 2 - metrics.gmptGlyphOrigin.x;
            const int y = (big - static_cast<int>(metrics.gmBlackBoxY)) / 2 - text.tmAscent + metrics.gmptGlyphOrigin.y;
            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, RGB(255, 255, 255));
            drawn = TextOutW(dc, x, y, &kCup, 1) != FALSE;
            GdiFlush();
        }
    }
    if (drawn) {
        // White on black: the green channel of each sample is its coverage.
        coverage.resize(static_cast<size_t>(size) * size);
        for (int row = 0; row < size; ++row) {
            for (int col = 0; col < size; ++col) {
                unsigned sum = 0;
                for (int dy = 0; dy < kScale; ++dy) {
                    const unsigned* line = pixels + static_cast<size_t>(row * kScale + dy) * big + col * kScale;
                    for (int dx = 0; dx < kScale; ++dx) {
                        sum += (line[dx] >> 8) & 0xFF;
                    }
                }
                coverage[static_cast<size_t>(row) * size + col] =
                    static_cast<unsigned char>((sum + kScale * kScale / 2) / (kScale * kScale));
            }
        }
    }
    SelectObject(dc, originalFont);
    SelectObject(dc, originalBitmap);
    DeleteObject(measureFont);
    if (drawFont != nullptr) {
        DeleteObject(drawFont);
    }
    DeleteDC(dc);
    DeleteObject(canvas);
    return coverage;
}

}  // namespace

HBITMAP CreateCoffeeBitmap(int size, COLORREF color)
{
    const std::vector<unsigned char> coverage = CupCoverage(size);
    unsigned* pixels = nullptr;
    HBITMAP bitmap = coverage.empty() ? nullptr : CreateDib(size, size, &pixels);
    if (bitmap == nullptr) {
        return nullptr;
    }
    const unsigned red = GetRValue(color);
    const unsigned green = GetGValue(color);
    const unsigned blue = GetBValue(color);
    for (size_t i = 0; i < coverage.size(); ++i) {
        // Premultiplied alpha: each color channel is scaled by the pixel's alpha.
        const unsigned alpha = coverage[i];
        pixels[i] = (alpha << 24) | ((red * alpha / 255) << 16) | ((green * alpha / 255) << 8) | (blue * alpha / 255);
    }
    return bitmap;
}

HICON CreateCoffeeIcon(int size, COLORREF color)
{
    const std::vector<unsigned char> coverage = CupCoverage(size);
    unsigned* pixels = nullptr;
    HBITMAP colorBitmap = coverage.empty() ? nullptr : CreateDib(size, size, &pixels);
    if (colorBitmap == nullptr) {
        return nullptr;
    }
    // Icons take straight alpha: every pixel has the whole color, and the alpha carries the coverage.
    const unsigned rgb = (static_cast<unsigned>(GetRValue(color)) << 16) |
                         (static_cast<unsigned>(GetGValue(color)) << 8) | GetBValue(color);
    // The AND mask has 1 for each fully transparent pixel, for drawing paths that ignore the alpha channel.
    // Monochrome bitmap rows are padded to 16 bits.
    const int stride = (size + 15) / 16 * 2;
    std::vector<unsigned char> maskBits(static_cast<size_t>(stride) * size, 0);
    for (int row = 0; row < size; ++row) {
        for (int col = 0; col < size; ++col) {
            const unsigned alpha = coverage[static_cast<size_t>(row) * size + col];
            pixels[static_cast<size_t>(row) * size + col] = (alpha << 24) | rgb;
            if (alpha == 0) {
                maskBits[static_cast<size_t>(row) * stride + col / 8] |= static_cast<unsigned char>(0x80 >> (col % 8));
            }
        }
    }
    HBITMAP mask = CreateBitmap(size, size, 1, 1, maskBits.data());
    HICON icon = nullptr;
    if (mask != nullptr) {
        ICONINFO info = {};
        info.fIcon = TRUE;
        info.hbmMask = mask;
        info.hbmColor = colorBitmap;
        icon = CreateIconIndirect(&info);
        DeleteObject(mask);
    }
    DeleteObject(colorBitmap);
    return icon;
}
