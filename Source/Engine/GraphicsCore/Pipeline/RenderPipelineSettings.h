/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsFoundation/Declarations/GraphicsTypes.h>

struct xiiRenderPipelineDeviceFeature
{
  using StorageType = xiiUInt8;
  enum Enum : StorageType
  {
    None              = 0,
    Compute           = XII_BIT(0),
    MeshShaders       = XII_BIT(1),
    RayTracing        = XII_BIT(2),
    BindlessResources = XII_BIT(3),
    Default           = None
  };
  struct Bits
  {
    StorageType Compute : 1;
    StorageType MeshShaders : 1;
    StorageType RayTracing : 1;
    StorageType BindlessResources : 1;
  };
};
XII_DECLARE_FLAGS_OPERATORS(xiiRenderPipelineDeviceFeature);
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineDeviceFeature);

struct xiiGALDeviceFeatures;
struct xiiGALTextureCreationDescription;

/// Authored pipeline-wide contracts. These constrain execution, never select or
/// construct topology. Subgraph interfaces use pin contracts for their outputs.
class XII_GRAPHICSCORE_DLL xiiRenderPipelineSettings : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderPipelineSettings, xiiReflectedClass);

public:
  xiiBitflags<xiiRenderPipelineDeviceFeature> m_RequiredDeviceFeatures;
  xiiEnum<xiiGALResourceFormat>               m_OutputFormat    = xiiGALResourceFormat::Unknown;
  xiiUInt32                                   m_uiOutputSamples = 0; ///< Zero accepts the selected output's sample count.

  xiiResult Validate(xiiStringBuilder* out_error = nullptr) const;
  /// Runtime checks on previously validated settings; no reflection or allocation on success.
  xiiResult ValidateDevice(const xiiGALDeviceFeatures* features, xiiStringBuilder* out_error = nullptr) const;
  xiiResult ValidateOutput(const xiiGALTextureCreationDescription* output, xiiStringBuilder* out_error = nullptr) const;
};
