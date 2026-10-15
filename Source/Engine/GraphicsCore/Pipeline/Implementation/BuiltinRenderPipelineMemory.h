/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsFoundation/Resources/Texture.h>

namespace xiiBuiltinPipelineUtils
{
  // Replace a history pair only after both allocations succeed. A factory parameter keeps
  // failure handling independently verifiable without exhausting physical GPU memory.
  template <typename CreateTexture>
  xiiResult EnsureTemporalHistoryTextures(xiiSharedPtr<xiiGALTexture> (&textures)[2], const xiiGALTextureCreationDescription& description, bool& inout_bHistoryValid, CreateTexture&& createTexture)
  {
    if (textures[0] != nullptr && textures[1] != nullptr && textures[0]->GetDescription() == description && textures[1]->GetDescription() == description)
      return XII_SUCCESS;

    xiiSharedPtr<xiiGALTexture> replacements[2];
    for (auto& texture : replacements)
    {
      texture = createTexture(description);
      if (texture == nullptr)
        return XII_FAILURE;
    }
    textures[0]         = std::move(replacements[0]);
    textures[1]         = std::move(replacements[1]);
    inout_bHistoryValid = false;
    return XII_SUCCESS;
  }
} // namespace xiiBuiltinPipelineUtils
