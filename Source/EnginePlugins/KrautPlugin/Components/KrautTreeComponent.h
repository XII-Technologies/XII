#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <KrautPlugin/KrautDeclarations.h>
#include <KrautPlugin/Renderer/KrautRenderData.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

struct xiiMsgExtractGeometry;
struct xiiMsgBuildStaticMesh;
struct xiiResourceEvent;
class xiiKrautRenderData;
class xiiAbstractObjectNode;

using xiiKrautTreeResourceHandle      = xiiTypedResourceHandle<class xiiKrautTreeResource>;
using xiiKrautGeneratorResourceHandle = xiiTypedResourceHandle<class xiiKrautGeneratorResource>;

class XII_KRAUTPLUGIN_DLL xiiKrautTreeComponentManager : public xiiComponentManager<class xiiKrautTreeComponent, xiiBlockStorageType::Compact>
{
public:
  typedef xiiComponentManager<xiiKrautTreeComponent, xiiBlockStorageType::Compact> SUPER;

  xiiKrautTreeComponentManager(xiiWorld* pWorld) :
    SUPER(pWorld)
  {
  }

  void Update(const xiiWorldModule::UpdateContext& context);
  void EnqueueUpdate(xiiComponentHandle hComponent);

private:
  void ResourceEventHandler(const xiiResourceEvent& e);

  mutable xiiMutex             m_Mutex;
  xiiDeque<xiiComponentHandle> m_RequireUpdate;

protected:
  virtual void Initialize() override;
  virtual void Deinitialize() override;
};

class XII_KRAUTPLUGIN_DLL xiiKrautTreeComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiKrautTreeComponent, xiiRenderComponent, xiiKrautTreeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& stream) override;

protected:
  virtual void OnActivated() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiRenderComponent

protected:
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& bounds, bool& bAlwaysVisible, xiiMsgUpdateLocalBounds& msg) override;
  void              OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const;

  //////////////////////////////////////////////////////////////////////////
  // xiiKrautTreeComponent

public:
  xiiKrautTreeComponent();
  ~xiiKrautTreeComponent();

  // see xiiKrautTreeComponent::GetLocalBounds for details
  static const int s_iLocalBoundsScale = 3;

  void OnMsgExtractGeometry(xiiMsgExtractGeometry& msg) const;
  void OnBuildStaticMesh(xiiMsgBuildStaticMesh& msg) const;

  void        SetKrautFile(const char* szFile); // [ property ]
  const char* GetKrautFile() const;             // [ property ]

  void      SetVariationIndex(xiiUInt16 uiIndex); // [ property ]
  xiiUInt16 GetVariationIndex() const;            // [ property ]

  void      SetCustomRandomSeed(xiiUInt16 uiSeed); // [ property ]
  xiiUInt16 GetCustomRandomSeed() const;           // [ property ]

  void                                   SetKrautGeneratorResource(const xiiKrautGeneratorResourceHandle& hTree);
  const xiiKrautGeneratorResourceHandle& GetKrautGeneratorResource() const { return m_hKrautGenerator; }

private:
  xiiResult CreateGeometry(xiiGeometry& geo, xiiWorldGeoExtractionUtil::ExtractionMode mode) const;
  void      EnsureTreeIsGenerated();

  xiiUInt16                       m_uiVariationIndex   = 0xFFFF;
  xiiUInt16                       m_uiCustomRandomSeed = 0xFFFF;
  xiiKrautTreeResourceHandle      m_hKrautTree;
  xiiKrautGeneratorResourceHandle m_hKrautGenerator;

  void ComputeWind() const;

  mutable xiiUInt64 m_uiLastWindUpdate = (xiiUInt64)-1;
  mutable xiiVec3   m_vWindSpringPos;
  mutable xiiVec3   m_vWindSpringVel;
};
