/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

/// Provides hash functions for GAL descriptor objects.
class XII_GRAPHICSFOUNDATION_DLL xiiGALDescriptorHash
{
public:
  static xiiUInt32 Hash(const xiiGALBlendStateCreationDescription& blendStateDescription);
  static bool      Equal(const xiiGALBlendStateCreationDescription& a, const xiiGALBlendStateCreationDescription& b);

  static xiiUInt32 Hash(const xiiGALRenderPassCreationDescription& renderPassDescription);
  static bool      Equal(const xiiGALRenderPassCreationDescription& a, const xiiGALRenderPassCreationDescription& b);

  static xiiUInt32 Hash(const xiiGALFramebufferCreationDescription& framebufferDescription);
  static bool      Equal(const xiiGALFramebufferCreationDescription& a, const xiiGALFramebufferCreationDescription& b);

  static xiiUInt32 Hash(const xiiGALPipelineStateCreationDescription& description);
  static bool      Equal(const xiiGALPipelineStateCreationDescription& a, const xiiGALPipelineStateCreationDescription& b);

  static xiiUInt32 Hash(const xiiGALGraphicsPipelineStateCreationDescription& description);
  static bool      Equal(const xiiGALGraphicsPipelineStateCreationDescription& a, const xiiGALGraphicsPipelineStateCreationDescription& b);

  static xiiUInt32 Hash(const xiiGALComputePipelineStateCreationDescription& description);
  static bool      Equal(const xiiGALComputePipelineStateCreationDescription& a, const xiiGALComputePipelineStateCreationDescription& b);

  static xiiUInt32 Hash(const xiiGALRayTracingPipelineStateCreationDescription& description);
  static bool      Equal(const xiiGALRayTracingPipelineStateCreationDescription& a, const xiiGALRayTracingPipelineStateCreationDescription& b);

  static xiiUInt32 Hash(const xiiGALTilePipelineStateCreationDescription& description);
  static bool      Equal(const xiiGALTilePipelineStateCreationDescription& a, const xiiGALTilePipelineStateCreationDescription& b);

  static xiiUInt32 Hash(const xiiGALPipelineResourceSignatureCreationDescription& description);
  static bool      Equal(const xiiGALPipelineResourceSignatureCreationDescription& a, const xiiGALPipelineResourceSignatureCreationDescription& b);
};
