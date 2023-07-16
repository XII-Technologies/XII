#pragma once

#include <Texture/Image/Image.h>

class xiiColorLinear16f;

XII_TEXTURE_DLL void xiiDecompressBlockBC1(const xiiUInt8* pSource, xiiColorBaseUB* pTarget, bool bForceFourColorMode);
XII_TEXTURE_DLL void xiiDecompressBlockBC4(const xiiUInt8* pSource, xiiUInt8* pTarget, xiiUInt32 uiStride, xiiUInt8 uiBias);
XII_TEXTURE_DLL void xiiDecompressBlockBC6(const xiiUInt8* pSource, xiiColorLinear16f* pTarget, bool bIsSigned);
XII_TEXTURE_DLL void xiiDecompressBlockBC7(const xiiUInt8* pSource, xiiColorBaseUB* pTarget);

XII_TEXTURE_DLL void xiiUnpackPaletteBC4(xiiUInt32 ui0, xiiUInt32 ui1, xiiUInt32* pAlphas);
