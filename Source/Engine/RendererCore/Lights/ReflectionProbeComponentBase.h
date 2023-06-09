#pragma once

#include <Core/World/Component.h>
#include <RendererCore/Lights/Implementation/ReflectionProbeData.h>

struct xiiMsgUpdateLocalBounds;
struct xiiMsgExtractRenderData;
struct xiiMsgTransformChanged;
class xiiAbstractObjectNode;

/// \brief Base class for all reflection probes.
class XII_RENDERERCORE_DLL xiiReflectionProbeComponentBase : public xiiComponent
{
  XII_ADD_DYNAMIC_REFLECTION(xiiReflectionProbeComponentBase, xiiComponent);

public:
  xiiReflectionProbeComponentBase();
  ~xiiReflectionProbeComponentBase();

  void                            SetReflectionProbeMode(xiiEnum<xiiReflectionProbeMode> mode); // [ property ]
  xiiEnum<xiiReflectionProbeMode> GetReflectionProbeMode() const;                               // [ property ]

  const xiiTagSet& GetIncludeTags() const;              // [ property ]
  void             InsertIncludeTag(const char* szTag); // [ property ]
  void             RemoveIncludeTag(const char* szTag); // [ property ]

  const xiiTagSet& GetExcludeTags() const;              // [ property ]
  void             InsertExcludeTag(const char* szTag); // [ property ]
  void             RemoveExcludeTag(const char* szTag); // [ property ]

  float GetNearPlane() const { return m_Desc.m_fNearPlane; } // [ property ]
  void  SetNearPlane(float fNearPlane);                      // [ property ]

  float GetFarPlane() const { return m_Desc.m_fFarPlane; } // [ property ]
  void  SetFarPlane(float fFarPlane);                      // [ property ]

  const xiiVec3& GetCaptureOffset() const { return m_Desc.m_vCaptureOffset; } // [ property ]
  void           SetCaptureOffset(const xiiVec3& vOffset);                    // [ property ]

  void SetShowDebugInfo(bool bShowDebugInfo); // [ property ]
  bool GetShowDebugInfo() const;              // [ property ]

  void SetShowMipMaps(bool bShowMipMaps); // [ property ]
  bool GetShowMipMaps() const;            // [ property ]

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& ref_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& ref_stream) override;

protected:
  float ComputePriority(xiiMsgExtractRenderData& msg, xiiReflectionProbeRenderData* pRenderData, float fVolume, const xiiVec3& vScale) const;

protected:
  xiiReflectionProbeDesc m_Desc;

  xiiReflectionProbeId m_Id;
  // Set to true if a change was made that requires recomputing the cube map.
  mutable bool m_bStatesDirty = true;
};
