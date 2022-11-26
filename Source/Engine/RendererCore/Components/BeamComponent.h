#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Core/Messages/EventMessage.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>

typedef xiiComponentManagerSimple<class xiiBeamComponent, xiiComponentUpdateType::Always> xiiBeamComponentManager;

struct xiiMsgExtractRenderData;
class xiiGeometry;
class xiiMeshResourceDescriptor;

/// \brief A beam component
class XII_RENDERERCORE_DLL xiiBeamComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiBeamComponent, xiiRenderComponent, xiiBeamComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiRenderComponent

public:
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& bounds, bool& bAlwaysVisible, xiiMsgUpdateLocalBounds& msg) override;


  //////////////////////////////////////////////////////////////////////////
  // xiiBeamComponent

public:
  xiiBeamComponent();
  ~xiiBeamComponent();

  void SetTargetObject(const char* szReference); // [ property ]

  void  SetWidth(float fWidth); // [ property ]
  float GetWidth() const;       // [ property ]

  void  SetUVUnitsPerWorldUnit(float fUVUnitsPerWorldUnit); // [ property ]
  float GetUVUnitsPerWorldUnit() const;                     // [ property ]

  void        SetMaterialFile(const char* szFile); // [ property ]
  const char* GetMaterialFile() const;             // [ property ]

  xiiMaterialResourceHandle GetMaterial() const;

  xiiGameObjectHandle m_hTargetObject; // [ property ]

  xiiColor m_Color; // [ property ]

protected:
  void Update();

  void OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const;

  float m_fWidth               = 0.1f; // [ property ]
  float m_fUVUnitsPerWorldUnit = 1.0f; // [ property ]

  xiiMaterialResourceHandle m_hMaterial; // [ property ]

  const float m_fDistanceUpdateEpsilon = 0.02f;

  // State
  xiiMeshResourceHandle m_hMesh;

  xiiVec3 m_vLastOwnerPosition  = xiiVec3::ZeroVector();
  xiiVec3 m_vLastTargetPosition = xiiVec3::ZeroVector();

  void CreateMeshes();
  void BuildMeshResourceFromGeometry(xiiGeometry& Geometry, xiiMeshResourceDescriptor& MeshDesc) const;
  void ReinitMeshes();
  void Cleanup();

  const char* DummyGetter() const { return nullptr; }
};
