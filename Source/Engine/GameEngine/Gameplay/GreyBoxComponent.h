#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Rasterizer/RasterizerObject.h>

class xiiMeshRenderData;
class xiiGeometry;
struct xiiMsgExtractRenderData;
struct xiiMsgBuildStaticMesh;
struct xiiMsgExtractGeometry;
struct xiiMsgExtractOccluderData;
struct xiiMsgTransformChanged;
class xiiMeshResourceDescriptor;
using xiiMeshResourceHandle     = xiiTypedResourceHandle<class xiiMeshResource>;
using xiiMaterialResourceHandle = xiiTypedResourceHandle<class xiiMaterialResource>;

using xiiGreyBoxComponentManager = class xiiGreyBoxComponent;

struct XII_GAMEENGINE_DLL xiiGreyBoxShape
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    Box,
    RampX,
    RampY,
    Column,
    StairsX,
    StairsY,
    ArchX,
    ArchY,
    SpiralStairs,

    Default = Box
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GAMEENGINE_DLL, xiiGreyBoxShape)

class XII_GAMEENGINE_DLL xiiGreyBoxComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiGreyBoxComponent, xiiRenderComponent, xiiGreyBoxComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

  virtual void SerializeComponent(xiiWorldWriter& stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& stream) override;

  virtual void OnActivated() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiRenderComponent
protected:
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& bounds, bool& bAlwaysVisible, xiiMsgUpdateLocalBounds& msg) override;
  void              OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const;

  //////////////////////////////////////////////////////////////////////////
  // xiiGreyBoxComponent

public:
  xiiGreyBoxComponent();
  ~xiiGreyBoxComponent();

  void                     SetShape(xiiEnum<xiiGreyBoxShape> shape);           // [ property ]
  xiiEnum<xiiGreyBoxShape> GetShape() const { return m_Shape; }                // [ property ]
  void                     SetMaterialFile(const char* szFile);                // [ property ]
  const char*              GetMaterialFile() const;                            // [ property ]
  void                     SetSizeNegX(float f);                               // [ property ]
  float                    GetSizeNegX() const { return m_fSizeNegX; }         // [ property ]
  void                     SetSizePosX(float f);                               // [ property ]
  float                    GetSizePosX() const { return m_fSizePosX; }         // [ property ]
  void                     SetSizeNegY(float f);                               // [ property ]
  float                    GetSizeNegY() const { return m_fSizeNegY; }         // [ property ]
  void                     SetSizePosY(float f);                               // [ property ]
  float                    GetSizePosY() const { return m_fSizePosY; }         // [ property ]
  void                     SetSizeNegZ(float f);                               // [ property ]
  float                    GetSizeNegZ() const { return m_fSizeNegZ; }         // [ property ]
  void                     SetSizePosZ(float f);                               // [ property ]
  float                    GetSizePosZ() const { return m_fSizePosZ; }         // [ property ]
  void                     SetDetail(xiiUInt32 uiDetail);                      // [ property ]
  xiiUInt32                GetDetail() const { return m_uiDetail; }            // [ property ]
  void                     SetCurvature(xiiAngle curvature);                   // [ property ]
  xiiAngle                 GetCurvature() const { return m_Curvature; }        // [ property ]
  void                     SetSlopedTop(bool b);                               // [ property ]
  bool                     GetSlopedTop() const { return m_bSlopedTop; }       // [ property ]
  void                     SetSlopedBottom(bool b);                            // [ property ]
  bool                     GetSlopedBottom() const { return m_bSlopedBottom; } // [ property ]
  void                     SetThickness(float f);                              // [ property ]
  float                    GetThickness() const { return m_fThickness; }       // [ property ]

  void SetGenerateCollision(bool b);                                 // [ property ]
  bool GetGenerateCollision() const { return m_bGenerateCollision; } // [ property ]

  void SetIncludeInNavmesh(bool b);                                // [ property ]
  bool GetIncludeInNavmesh() const { return m_bIncludeInNavmesh; } // [ property ]

  void                      SetMaterial(const xiiMaterialResourceHandle& hMaterial) { m_hMaterial = hMaterial; }
  xiiMaterialResourceHandle GetMaterial() const { return m_hMaterial; }

protected:
  void OnBuildStaticMesh(xiiMsgBuildStaticMesh& msg) const;
  void OnMsgExtractGeometry(xiiMsgExtractGeometry& msg) const;
  void OnMsgExtractOccluderData(xiiMsgExtractOccluderData& msg) const;

  xiiEnum<xiiGreyBoxShape>  m_Shape;
  xiiMaterialResourceHandle m_hMaterial;
  xiiColor                  m_Color     = xiiColor::White;
  float                     m_fSizeNegX = 0;
  float                     m_fSizePosX = 0;
  float                     m_fSizeNegY = 0;
  float                     m_fSizePosY = 0;
  float                     m_fSizeNegZ = 0;
  float                     m_fSizePosZ = 0;
  xiiUInt32                 m_uiDetail  = 16;
  xiiAngle                  m_Curvature;
  float                     m_fThickness         = 0.5f;
  bool                      m_bSlopedTop         = false;
  bool                      m_bSlopedBottom      = false;
  bool                      m_bGenerateCollision = true;
  bool                      m_bIncludeInNavmesh  = true;
  bool                      m_bUseAsOccluder     = true;

  void InvalidateMesh();
  void BuildGeometry(xiiGeometry& geom, xiiEnum<xiiGreyBoxShape> shape, bool bOnlyRoughDetails) const;

  template <typename ResourceType>
  xiiTypedResourceHandle<ResourceType> GenerateMesh() const;

  void GenerateMeshName(xiiStringBuilder& out_sName) const;
  void GenerateMeshResourceDescriptor(xiiMeshResourceDescriptor& desc) const;

  xiiMeshResourceHandle m_hMesh;

  mutable xiiSharedPtr<const xiiRasterizerObject> m_pOccluderObject;
};
