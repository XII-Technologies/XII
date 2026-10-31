/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiStreamReader;
class xiiStreamWriter;
struct xiiMaterialResourceDescriptor;

namespace xiiMaterialAssetFile
{
  /// Writes version 9 material content after the standard asset header. Native typed
  /// PBR state and custom shader parameters/bindings are both preserved. The output
  /// is staged so failed destination writes propagate through xiiResult.
  XII_GRAPHICSCORE_DLL xiiResult Write(xiiStreamWriter& stream, const xiiMaterialResourceDescriptor& description);

  /// Reads the typed state block in version 9 content. Other descriptor fields remain
  /// unchanged. Rejects truncated, non-finite and unknown enum data transactionally.
  XII_GRAPHICSCORE_DLL xiiResult ReadPbrBlock(xiiStreamReader& stream, xiiMaterialResourceDescriptor& inout_description);
} // namespace xiiMaterialAssetFile
