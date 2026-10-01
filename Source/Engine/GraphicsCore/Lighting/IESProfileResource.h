/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Reflection/Reflection.h>
#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiStringBuilder;

using xiiIESProfileResourceHandle = xiiTypedResourceHandle<class xiiIESProfileResource>;

/// Runtime representation of an LM-63 Type-C photometric web.
///
/// Profiles are resampled to a fixed full-sphere grid at import time. A fixed
/// layout makes profiles inexpensive to deduplicate and upload as one GPU
/// structured buffer while preserving asymmetric luminaires and their roll.
struct XII_GRAPHICSCORE_DLL xiiIESProfileResourceDescriptor
{
  static constexpr xiiUInt32 s_uiVerticalSampleCount   = 64U;
  static constexpr xiiUInt32 s_uiHorizontalSampleCount = 32U;
  static constexpr xiiUInt32 s_uiSampleCount           = s_uiVerticalSampleCount * s_uiHorizontalSampleCount;

  float                  m_fMaximumCandela = 0.0f;
  float                  m_fReportedLumens = 0.0f;
  xiiDynamicArray<float> m_NormalizedCandela;

  [[nodiscard]] bool IsValid() const;

  /// Parses an IESNA LM-63 photometric file. Type-C data and TILT=NONE are
  /// supported; invalid or unsupported inputs fail without modifying this descriptor.
  [[nodiscard]] xiiResult ParseLM63(xiiStringView sSource, xiiStringBuilder* pError = nullptr);

  /// Bilinear full-sphere lookup used by validation tools and CPU sensor paths.
  [[nodiscard]] float Sample(xiiAngle verticalAngle, xiiAngle horizontalAngle) const;

  void                    Serialize(xiiStreamWriter& inout_stream) const;
  [[nodiscard]] xiiResult Deserialize(xiiStreamReader& inout_stream);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiIESProfileResourceDescriptor);

/// Loadable and programmatically createable IES light profile resource.
/// Raw .ies files are parsed directly by the default file resource loader.
class XII_GRAPHICSCORE_DLL xiiIESProfileResource : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiIESProfileResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiIESProfileResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiIESProfileResource, xiiIESProfileResourceDescriptor);

public:
  xiiIESProfileResource();

  [[nodiscard]] const xiiIESProfileResourceDescriptor& GetDescriptor() const { return m_Descriptor; }

protected:
  virtual xiiResourceLoadDescription UnloadData(Unload whatToUnload) override;
  virtual xiiResourceLoadDescription UpdateContent(xiiStreamReader* pStream) override;
  virtual void                       UpdateMemoryUsage(MemoryUsage& out_memoryUsage) override;

private:
  xiiIESProfileResourceDescriptor m_Descriptor;
};
