/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <Foundation/Math/Size.h>

class xiiGALTexture;
class xiiImage;

/// Reads a single-sample RGBA8 2D render target, waits for GPU completion, and scales to the requested size.
/// Must run after rendering has completed on the CPU. Output is empty on failure.
XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiResult xiiCaptureThumbnail(xiiGALTexture* pTexture, xiiSizeU32 outputSize, xiiImage& out_image);
