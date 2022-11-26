#pragma once

#include <BakingPlugin/Declarations.h>
#include <Core/Graphics/AmbientCubeBasis.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/SimdMath/SimdTransform.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/BakedProbes/BakingInterface.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

class xiiWorld;
class xiiProgress;
class xiiTracerInterface;

class XII_BAKINGPLUGIN_DLL xiiBakingScene
{
public:
  xiiResult Extract();

  xiiResult Bake(const xiiStringView& sOutputPath, xiiProgress& progress);

  xiiResult RenderDebugView(const xiiMat4& InverseViewProjection, xiiUInt32 uiWidth, xiiUInt32 uiHeight, xiiDynamicArray<xiiColorGammaUB>& out_Pixels, xiiProgress& progress) const;

public:
  const xiiWorldGeoExtractionUtil::MeshObjectList& GetMeshObjects() const { return m_MeshObjects; }
  const xiiBoundingBox&                            GetBoundingBox() const { return m_BoundingBox; }

  bool IsBaked() const { return m_bIsBaked; }

private:
  friend class xiiBaking;
  friend class xiiMemoryUtils;

  xiiBakingScene();
  ~xiiBakingScene();

  xiiBakingSettings                                                      m_Settings;
  xiiDynamicArray<xiiBakingInternal::Volume, xiiAlignedAllocatorWrapper> m_Volumes;
  xiiWorldGeoExtractionUtil::MeshObjectList                              m_MeshObjects;
  xiiBoundingBox                                                         m_BoundingBox;

  xiiUInt32                        m_uiWorldIndex = xiiInvalidIndex;
  xiiUniquePtr<xiiTracerInterface> m_pTracer;

  bool m_bIsBaked = false;
};

class XII_BAKINGPLUGIN_DLL xiiBaking : public xiiBakingInterface
{
  XII_DECLARE_SINGLETON_OF_INTERFACE(xiiBaking, xiiBakingInterface);

public:
  xiiBaking();

  void Startup();
  void Shutdown();

  xiiBakingScene*       GetOrCreateScene(const xiiWorld& world);
  xiiBakingScene*       GetScene(const xiiWorld& world);
  const xiiBakingScene* GetScene(const xiiWorld& world) const;

  // xiiBakingInterface
  virtual xiiResult RenderDebugView(const xiiWorld& world, const xiiMat4& InverseViewProjection, xiiUInt32 uiWidth, xiiUInt32 uiHeight, xiiDynamicArray<xiiColorGammaUB>& out_Pixels, xiiProgress& progress) const override;
};
