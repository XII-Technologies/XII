/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Resources/Sampler.h>
#include <GraphicsFoundation/Shader/ShaderByteCode.h>
#include <GraphicsFoundation/States/PipelineResourceSignature.h>

class XII_GRAPHICSFOUNDATION_DLL xiiGALGraphicsUtilities
{
public:
  /// This converts swap chain usage flags to bind flags.
  [[nodiscard]] static xiiBitflags<xiiGALBindFlags> SwapChainUsageFlagsToBindFlags(xiiBitflags<xiiGALSwapChainUsageFlags> swapChainUsageFlags);

  /// This returns the valid pipeline resource flags for a given shader resource type.
  [[nodiscard]] static xiiBitflags<xiiGALPipelineResourceFlags> GetValidPipelineResourceFlags(xiiEnum<xiiGALShaderResourceType> type);

  /// Returns the default sampler creation description.
  [[nodiscard]] static xiiGALSamplerCreationDescription GetDefaultSamplerDescription() noexcept;
};
