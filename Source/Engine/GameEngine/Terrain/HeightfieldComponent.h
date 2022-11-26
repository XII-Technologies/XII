#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <Foundation/Math/Vec2.h>
#include <GameEngine/GameEngineDLL.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>

class xiiGeometry;
struct xiiMsgExtractRenderData;
struct xiiMsgBuildStaticMesh;
struct xiiMsgExtractGeometry;
class xiiHeightfieldComponent;
class xiiMeshResourceDescriptor;

using xiiMeshResourceHandle      = xiiTypedResourceHandle<class xiiMeshResource>;
using xiiMaterialResourceHandle  = xiiTypedResourceHandle<class xiiMaterialResource>;
using xiiImageDataResourceHandle = xiiTypedResourceHandle<class xiiImageDataResource>;

class XII_GAMEENGINE_DLL xiiHeightfieldComponentManager : public xiiComponentManager<xiiHeightfieldComponent, xiiBlockStorageType::Compact>
{
public:
  xiiHeightfieldComponentManager(xiiWorld* pWorld);
  ~xiiHeightfieldComponentManager();

  virtual void Initialize() override;

  void Update(const xiiWorldModule::UpdateContext& context);
  void AddToUpdateList(xiiHeightfieldComponent* pComponent);

private:
  void ResourceEventHandler(const xiiResourceEvent& e);

  xiiDeque<xiiComponentHandle> m_ComponentsToUpdate;
};

/// \brief This component utilizes a greyscale image to generate an elevation mesh, which is typically used for simple terrain
///
/// The component always creates a mesh for rendering, which uses a single material.
/// For different layers of grass, dirt, etc. the material can combine multiple textures and a mask.
///
/// If the "GenerateCollision" property is set, the component also generates a static collision mesh during scene export.
class XII_GAMEENGINE_DLL xiiHeightfieldComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiHeightfieldComponent, xiiRenderComponent, xiiHeightfieldComponentManager);

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
  // xiiHeightfieldComponent

public:
  xiiHeightfieldComponent();
  ~xiiHeightfieldComponent();

  xiiVec2 GetHalfExtents() const { return m_vHalfExtents; } // [ property ]
  void    SetHalfExtents(xiiVec2 value);                    // [ property ]

  float GetHeight() const { return m_fHeight; } // [ property ]
  void  SetHeight(float value);                 // [ property ]

  xiiVec2 GetTexCoordOffset() const { return m_vTexCoordOffset; } // [ property ]
  void    SetTexCoordOffset(xiiVec2 value);                       // [ property ]

  xiiVec2 GetTexCoordScale() const { return m_vTexCoordScale; } // [ property ]
  void    SetTexCoordScale(xiiVec2 value);                      // [ property ]

  void        SetMaterialFile(const char* szFile); // [ property ]
  const char* GetMaterialFile() const;             // [ property ]

  void                      SetMaterial(const xiiMaterialResourceHandle& hMaterial) { m_hMaterial = hMaterial; }
  xiiMaterialResourceHandle GetMaterial() const { return m_hMaterial; }

  void        SetHeightfieldFile(const char* szFile); // [ property ]
  const char* GetHeightfieldFile() const;             // [ property ]

  void                       SetHeightfield(const xiiImageDataResourceHandle& hResource);
  xiiImageDataResourceHandle GetHeightfield() const { return m_hHeightfield; }

  xiiVec2U32 GetTesselation() const { return m_vTesselation; } // [ property ]
  void       SetTesselation(xiiVec2U32 value);                 // [ property ]

  void SetGenerateCollision(bool b);                                 // [ property ]
  bool GetGenerateCollision() const { return m_bGenerateCollision; } // [ property ]

  xiiVec2U32 GetColMeshTesselation() const { return m_vColMeshTesselation; } // [ property ]
  void       SetColMeshTesselation(xiiVec2U32 value);                        // [ property ]

  void SetIncludeInNavmesh(bool b);                                // [ property ]
  bool GetIncludeInNavmesh() const { return m_bIncludeInNavmesh; } // [ property ]

protected:
  void OnBuildStaticMesh(xiiMsgBuildStaticMesh& msg) const;    // [ msg handler ]
  void OnMsgExtractGeometry(xiiMsgExtractGeometry& msg) const; // [ msg handler ]

  void      InvalidateMesh();
  void      BuildGeometry(xiiGeometry& geom) const;
  xiiResult BuildMeshDescriptor(xiiMeshResourceDescriptor& desc) const;

  template <typename ResourceType>
  xiiTypedResourceHandle<ResourceType> GenerateMesh() const;

  xiiUInt32                  m_uiHeightfieldChangeCounter = 0;
  xiiImageDataResourceHandle m_hHeightfield;
  xiiMaterialResourceHandle  m_hMaterial;

  xiiVec2 m_vHalfExtents = xiiVec2(100.0f);
  float   m_fHeight      = 50.0f;

  xiiVec2 m_vTexCoordOffset = xiiVec2::ZeroVector();
  xiiVec2 m_vTexCoordScale  = xiiVec2(1);

  xiiVec2U32 m_vTesselation        = xiiVec2U32(128);
  xiiVec2U32 m_vColMeshTesselation = xiiVec2U32(64);

  bool m_bGenerateCollision = true;
  bool m_bIncludeInNavmesh  = true;

  xiiMeshResourceHandle m_hMesh;
};
