#pragma once

#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Meshes/SkinnedMeshComponent.h>
#include <memory>

struct xiiMsgExtractRenderData;
struct xiiMsgSetColor;
struct xiiMsgSetMeshMaterial;
struct xiiMsgRopePoseUpdated;
class xiiShaderTransform;

using xiiRopeRenderComponentManager = xiiComponentManager<class xiiRopeRenderComponent, xiiBlockStorageType::Compact>;

class XII_RENDERERCORE_DLL xiiRopeRenderComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiRopeRenderComponent, xiiRenderComponent, xiiRopeRenderComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& ref_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& ref_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiRenderComponent

protected:
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& bounds, bool& bAlwaysVisible, xiiMsgUpdateLocalBounds& msg) override;
  void              OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const; // [ msg handler ]

  //////////////////////////////////////////////////////////////////////////
  // xiiRopeRenderComponent

public:
  xiiRopeRenderComponent();
  ~xiiRopeRenderComponent();

  xiiColor m_Color = xiiColor::White; // [ property ]

  void        SetMaterialFile(const char* szFile); // [ property ]
  const char* GetMaterialFile() const;             // [ property ]

  void                      SetMaterial(const xiiMaterialResourceHandle& hMaterial) { m_hMaterial = hMaterial; }
  xiiMaterialResourceHandle GetMaterial() const { return m_hMaterial; }

  void  SetThickness(float fThickness);               // [ property ]
  float GetThickness() const { return m_fThickness; } // [ property ]

  void      SetDetail(xiiUInt32 uiDetail);           // [ property ]
  xiiUInt32 GetDetail() const { return m_uiDetail; } // [ property ]

  void SetSubdivide(bool bSubdivide);                // [ property ]
  bool GetSubdivide() const { return m_bSubdivide; } // [ property ]

  void  SetUScale(float fUScale);               // [ property ]
  float GetUScale() const { return m_fUScale; } // [ property ]

  void OnMsgSetColor(xiiMsgSetColor& ref_msg);               // [ msg handler ]
  void OnMsgSetMeshMaterial(xiiMsgSetMeshMaterial& ref_msg); // [ msg handler ]

private:
  void OnRopePoseUpdated(xiiMsgRopePoseUpdated& msg); // [ msg handler ]

  void GenerateRenderMesh(xiiUInt32 uiNumRopePieces);

  void UpdateSkinningTransformBuffer(xiiArrayPtr<const xiiTransform> skinningTransforms);

  xiiBoundingBoxSphere m_LocalBounds;

  xiiSkinningState m_SkinningState;

  xiiMeshResourceHandle     m_hMesh;
  xiiMaterialResourceHandle m_hMaterial;

  float     m_fThickness = 0.05f;
  xiiUInt32 m_uiDetail   = 6;
  bool      m_bSubdivide = false;

  float m_fUScale = 1.0f;
};
