#pragma once

#include <Core/World/World.h>
#include <GraphicsCore/Meshes/MeshComponent.h>

using xiiSkyBoxComponentManager    = xiiComponentManager<class xiiSkyBoxComponent, xiiBlockStorageType::Compact>;
using xiiTextureCubeResourceHandle = xiiTypedResourceHandle<class xiiTextureCubeResource>;

class XII_GRAPHICSCORE_DLL xiiSkyBoxComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiSkyBoxComponent, xiiRenderComponent, xiiSkyBoxComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

protected:
  virtual void Initialize() override;
  virtual void OnActivated() override;


  //////////////////////////////////////////////////////////////////////////
  // xiiRenderComponent

public:
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg) override;


  //////////////////////////////////////////////////////////////////////////
  // xiiSkyBoxComponent

public:
  xiiSkyBoxComponent();
  ~xiiSkyBoxComponent();

  void  SetExposureBias(float fExposureBias);               // [ property ]
  float GetExposureBias() const { return m_fExposureBias; } // [ property ]

  void SetInverseTonemap(bool bInverseTonemap);                // [ property ]
  bool GetInverseTonemap() const { return m_bInverseTonemap; } // [ property ]

  void SetUseFog(bool bUseFog);                // [ property ]
  bool GetUseFog() const { return m_bUseFog; } // [ property ]

  void  SetVirtualDistance(float fVirtualDistance);               // [ property ]
  float GetVirtualDistance() const { return m_fVirtualDistance; } // [ property ]

  void        SetCubeMapFile(const char* szFile); // [ property ]
  const char* GetCubeMapFile() const;             // [ property ]

  void                                SetCubeMap(const xiiTextureCubeResourceHandle& hCubeMap);
  const xiiTextureCubeResourceHandle& GetCubeMap() const;

private:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const;
  void UpdateMaterials();

  float m_fExposureBias    = 0.0f;
  float m_fVirtualDistance = 1000.0f;
  bool  m_bInverseTonemap  = false;
  bool  m_bUseFog          = true;

  xiiTextureCubeResourceHandle m_hCubeMap;

  xiiMeshResourceHandle     m_hMesh;
  xiiMaterialResourceHandle m_hCubeMapMaterial;
};
