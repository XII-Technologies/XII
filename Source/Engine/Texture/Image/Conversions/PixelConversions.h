/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Texture/Image/Image.h>

XII_TEXTURE_DLL xiiColorBaseUB xiiDecompressA4B4G4R4(xiiUInt16 uiColor);
XII_TEXTURE_DLL xiiColorBaseUB xiiDecompressB4G4R4A4(xiiUInt16 uiColor);
XII_TEXTURE_DLL xiiColorBaseUB xiiDecompressB5G6R5(xiiUInt16 uiColor);
XII_TEXTURE_DLL xiiColorBaseUB xiiDecompressB5G5R5X1(xiiUInt16 uiColor);
XII_TEXTURE_DLL xiiColorBaseUB xiiDecompressB5G5R5A1(xiiUInt16 uiColor);
XII_TEXTURE_DLL xiiColorBaseUB xiiDecompressX1B5G5R5(xiiUInt16 uiColor);
XII_TEXTURE_DLL xiiColorBaseUB xiiDecompressA1B5G5R5(xiiUInt16 uiColor);
XII_TEXTURE_DLL xiiUInt16      xiiCompressA4B4G4R4(xiiColorBaseUB color);
XII_TEXTURE_DLL xiiUInt16      xiiCompressB4G4R4A4(xiiColorBaseUB color);
XII_TEXTURE_DLL xiiUInt16      xiiCompressB5G6R5(xiiColorBaseUB color);
XII_TEXTURE_DLL xiiUInt16      xiiCompressB5G5R5X1(xiiColorBaseUB color);
XII_TEXTURE_DLL xiiUInt16      xiiCompressB5G5R5A1(xiiColorBaseUB color);
XII_TEXTURE_DLL xiiUInt16      xiiCompressX1B5G5R5(xiiColorBaseUB color);
XII_TEXTURE_DLL xiiUInt16      xiiCompressA1B5G5R5(xiiColorBaseUB color);
