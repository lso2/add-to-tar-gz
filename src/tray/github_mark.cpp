// github_mark.cpp: WIC decoding of the GitHub mark PNG into an icon at the small icon size.
// Globals: none beyond github_mark.h.
// Doc: docs/notes/src/tray/github_mark.md
#include "github_mark.h"
#include "tray_ids.h"
#include <objbase.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <algorithm>
#include <cstring>
#include <vector>

using Microsoft::WRL::ComPtr;

namespace {

// A top-down 32-bit DIB section and its pixels.
HBITMAP CreateDib(int side, BYTE** pixels)
{
    BITMAPINFO info = {};
    info.bmiHeader.biSize = sizeof(info.bmiHeader);
    info.bmiHeader.biWidth = side;
    info.bmiHeader.biHeight = -side;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    void* bits = nullptr;
    HBITMAP bitmap = CreateDIBSection(nullptr, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
    *pixels = static_cast<BYTE*>(bits);
    return bitmap;
}

// An icon from straight-alpha BGRA pixels, with an AND mask of 1 for each fully transparent pixel.
// Monochrome bitmap rows are padded to 16 bits.
HICON IconFromPixels(HBITMAP color, const BYTE* pixels, int side)
{
    const int stride = (side + 15) / 16 * 2;
    std::vector<BYTE> maskBits(static_cast<size_t>(stride) * side, 0);
    for (int row = 0; row < side; ++row) {
        for (int col = 0; col < side; ++col) {
            if (pixels[(static_cast<size_t>(row) * side + col) * 4 + 3] == 0) {
                maskBits[static_cast<size_t>(row) * stride + col / 8] |= static_cast<BYTE>(0x80 >> (col % 8));
            }
        }
    }
    HBITMAP mask = CreateBitmap(side, side, 1, 1, maskBits.data());
    if (mask == nullptr) {
        return nullptr;
    }
    ICONINFO info = {};
    info.fIcon = TRUE;
    info.hbmMask = mask;
    info.hbmColor = color;
    HICON icon = CreateIconIndirect(&info);
    DeleteObject(mask);
    return icon;
}

}  // namespace

HICON CreateIconFromPng(const void* data, DWORD size, int pixels, COLORREF color)
{
    ComPtr<IWICImagingFactory> factory;
    ComPtr<IWICStream> stream;
    ComPtr<IWICBitmapDecoder> decoder;
    ComPtr<IWICBitmapFrameDecode> frame;
    UINT sourceWidth = 0;
    UINT sourceHeight = 0;
    if (data == nullptr || size == 0 || pixels <= 0 ||
        FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory))) ||
        FAILED(factory->CreateStream(&stream)) ||
        FAILED(stream->InitializeFromMemory(static_cast<BYTE*>(const_cast<void*>(data)), size)) ||
        FAILED(factory->CreateDecoderFromStream(stream.Get(), nullptr, WICDecodeMetadataCacheOnDemand, &decoder)) ||
        FAILED(decoder->GetFrame(0, &frame)) || FAILED(frame->GetSize(&sourceWidth, &sourceHeight)) ||
        sourceWidth == 0 || sourceHeight == 0) {
        return nullptr;
    }
    // The longer side fills the icon. The shorter side keeps the image's proportions.
    const UINT side = static_cast<UINT>(pixels);
    const UINT width = sourceWidth >= sourceHeight ? side : (std::max)(1u, sourceWidth * side / sourceHeight);
    const UINT height = sourceHeight >= sourceWidth ? side : (std::max)(1u, sourceHeight * side / sourceWidth);
    ComPtr<IWICBitmapScaler> scaler;
    ComPtr<IWICFormatConverter> converter;
    std::vector<BYTE> scaled(static_cast<size_t>(width) * height * 4);
    if (FAILED(factory->CreateBitmapScaler(&scaler)) ||
        FAILED(scaler->Initialize(frame.Get(), width, height, WICBitmapInterpolationModeFant)) ||
        FAILED(factory->CreateFormatConverter(&converter)) ||
        FAILED(converter->Initialize(scaler.Get(), GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, nullptr, 0.0,
                                     WICBitmapPaletteTypeCustom)) ||
        FAILED(converter->CopyPixels(nullptr, width * 4, static_cast<UINT>(scaled.size()), scaled.data()))) {
        return nullptr;
    }
    BYTE* out = nullptr;
    HBITMAP bitmap = CreateDib(pixels, &out);
    if (bitmap == nullptr) {
        return nullptr;
    }
    std::fill(out, out + static_cast<size_t>(side) * side * 4, BYTE{0});
    const UINT left = (side - width) / 2;
    const UINT top = (side - height) / 2;
    for (UINT row = 0; row < height; ++row) {
        std::memcpy(out + (static_cast<size_t>(top + row) * side + left) * 4, scaled.data() + static_cast<size_t>(row) * width * 4,
                    static_cast<size_t>(width) * 4);
    }
    for (size_t i = 0; i < static_cast<size_t>(side) * side; ++i) {
        out[i * 4] = GetBValue(color);
        out[i * 4 + 1] = GetGValue(color);
        out[i * 4 + 2] = GetRValue(color);
    }
    HICON icon = IconFromPixels(bitmap, out, pixels);
    DeleteObject(bitmap);
    return icon;
}

HICON LoadGithubMark(HINSTANCE instance, int pixels, COLORREF color)
{
    HRSRC resource = FindResourceW(instance, MAKEINTRESOURCEW(IDR_GITHUB_MARK), RT_RCDATA);
    if (resource == nullptr) {
        return nullptr;
    }
    HGLOBAL loaded = LoadResource(instance, resource);
    const void* data = loaded != nullptr ? LockResource(loaded) : nullptr;
    return CreateIconFromPng(data, SizeofResource(instance, resource), pixels, color);
}
