#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Resources/Sampler.h>
#include <GraphicsFoundation/States/PipelineResourceSignature.h>

class XII_GRAPHICSFOUNDATION_DLL xiiGALGraphicsUtilities
{
public:
  /// \brief This converts swap chain usage flags to bind flags.
  static xiiBitflags<xiiGALBindFlags> SwapChainUsageFlagsToBindFlags(xiiBitflags<xiiGALSwapChainUsageFlags> swapChainUsageFlags);

  /// \brief This returns the valid pipeline resource flags for a given shader resource type.
  static [[nodiscard]] xiiBitflags<xiiGALPipelineResourceFlags> GetValidPipelineResourceFlags(xiiEnum<xiiGALShaderResourceType> type);

  /// \brief Returns the default sampler creation description.
  static [[nodiscard]] xiiGALSamplerCreationDescription GetDefaultSamplerDescription() noexcept;
};
