/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/Components/Render/RenderComponent.h>
#include <GraphicsCore/Decals/DecalResource.h>
#include <GraphicsCore/Pipeline/RenderData.h>

class xiiMeshResource;
struct xiiMsgExtractRenderData;

using xiiDecalComponentManager = xiiComponentManager<class xiiDecalComponent, xiiBlockStorageType::Compact>;

/// \brief Renderer-facing packet for both projected deferred decals and mesh decals.
class XII_GRAPHICSCORE_DLL xiiDecalRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDecalRenderData, xiiRenderData);

public:
  xiiEnum<xiiDecalProjectionMode> m_Mode = xiiDecalProjectionMode::Projected;

  xiiDecalResourceHandle      m_hDecal;
  xiiDecalAtlasResourceHandle m_hAtlas;
  xiiHashedString             m_sAtlasId;

  xiiTexture2DResourceHandle m_hAlbedo;
  xiiTexture2DResourceHandle m_hNormal;
  xiiTexture2DResourceHandle m_hMaterial;
  xiiTexture2DResourceHandle m_hEmissive;

  xiiMeshResourceHandle       m_hMesh;
  xiiMeshBufferResourceHandle m_hMeshBuffer;

  xiiVec3 m_vExtents  = xiiVec3(1.0f, 1.0f, 0.25f);
  xiiVec2 m_vUVOffset = xiiVec2::MakeZero();
  xiiVec2 m_vUVScale  = xiiVec2(1.0f);
  xiiVec4 m_vAtlasUVRect = xiiVec4(0.0f, 0.0f, 1.0f, 1.0f);

  xiiColor m_Tint = xiiColor::White;
  xiiBitflags<xiiDecalChannelMask> m_ChannelMask = xiiDecalChannelMask::Default;

  float    m_fOpacity     = 1.0f;
  float    m_fNormalBlend = 1.0f;
  float    m_fRoughness   = 0.5f;
  float    m_fMetallic    = 0.0f;
  float    m_fEmissive    = 0.0f;
  float    m_fFadeAngle   = 1.0f;
  xiiUInt8 m_uiPriority   = 128U;

  xiiUInt32 m_uiUniqueID       = 0U;
  xiiUInt32 m_uiFirstPrimitive = 0U;
  xiiUInt32 m_uiPrimitiveCount = 0U;
  xiiUInt32 m_uiFirstMeshlet   = 0U;
  xiiUInt32 m_uiMeshletCount   = 0U;
};

/// \brief Places a decal projector or mesh decal into the render world.
class XII_GRAPHICSCORE_DLL xiiDecalComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiDecalComponent, xiiRenderComponent, xiiDecalComponentManager);

public:
  xiiDecalComponent();
  ~xiiDecalComponent();

  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg) override;

  void                         SetMode(xiiEnum<xiiDecalProjectionMode> mode);
  xiiEnum<xiiDecalProjectionMode> GetMode() const;

  void                            SetDecal(const xiiDecalResourceHandle& hDecal);
  const xiiDecalResourceHandle&   GetDecal() const;

  void                               SetAtlas(const xiiDecalAtlasResourceHandle& hAtlas);
  const xiiDecalAtlasResourceHandle& GetAtlas() const;

  void          SetAtlasId(xiiStringView sAtlasId);
  xiiStringView GetAtlasId() const;

  void                         SetMesh(const xiiMeshResourceHandle& hMesh);
  const xiiMeshResourceHandle& GetMesh() const;

  void          SetExtents(xiiVec3 vExtents);
  const xiiVec3& GetExtents() const;

  void           SetUVOffset(xiiVec2 vOffset);
  const xiiVec2& GetUVOffset() const;

  void           SetUVScale(xiiVec2 vScale);
  const xiiVec2& GetUVScale() const;

  void            SetTint(const xiiColor& tint);
  const xiiColor& GetTint() const;

  void                                SetChannelMask(xiiBitflags<xiiDecalChannelMask> mask);
  xiiBitflags<xiiDecalChannelMask>    GetChannelMask() const;

  void  SetOpacity(float fOpacity);
  float GetOpacity() const;

  void  SetNormalBlend(float fNormalBlend);
  float GetNormalBlend() const;

  void  SetRoughness(float fRoughness);
  float GetRoughness() const;

  void  SetMetallic(float fMetallic);
  float GetMetallic() const;

  void  SetEmissive(float fEmissive);
  float GetEmissive() const;

  void     SetPriority(xiiUInt8 uiPriority);
  xiiUInt8 GetPriority() const;

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;

private:
  void FillMeshRange(xiiDecalRenderData& ref_renderData, const xiiMeshResource& mesh) const;

private:
  xiiEnum<xiiDecalProjectionMode> m_Mode = xiiDecalProjectionMode::Projected;

  xiiDecalResourceHandle      m_hDecal;
  xiiDecalAtlasResourceHandle m_hAtlas;
  xiiHashedString             m_sAtlasId;
  xiiMeshResourceHandle       m_hMesh;

  xiiVec3 m_vExtents  = xiiVec3(1.0f, 1.0f, 0.25f);
  xiiVec2 m_vUVOffset = xiiVec2::MakeZero();
  xiiVec2 m_vUVScale  = xiiVec2(1.0f);
  xiiColor m_Tint     = xiiColor::White;

  xiiBitflags<xiiDecalChannelMask> m_ChannelMask = xiiDecalChannelMask::Default;

  float    m_fOpacity     = 1.0f;
  float    m_fNormalBlend = 1.0f;
  float    m_fRoughness   = 0.5f;
  float    m_fMetallic    = 0.0f;
  float    m_fEmissive    = 0.0f;
  xiiUInt8 m_uiPriority   = 128U;
};
