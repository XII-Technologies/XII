#include <BakingPlugin/BakingPluginPCH.h>

#include <BakingPlugin/BakingScene.h>
#include <BakingPlugin/Tasks/PlaceProbesTask.h>
#include <BakingPlugin/Tasks/SkyVisibilityTask.h>
#include <BakingPlugin/Tracer/TracerEmbree.h>
#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <Foundation/Utilities/Progress.h>
#include <RendererCore/BakedProbes/BakedProbesComponent.h>
#include <RendererCore/BakedProbes/BakedProbesVolumeComponent.h>
#include <RendererCore/BakedProbes/ProbeTreeSectorResource.h>
#include <RendererCore/Meshes/MeshComponentBase.h>

xiiResult xiiBakingScene::Extract()
{
  m_Volumes.Clear();
  m_MeshObjects.Clear();
  m_BoundingBox.SetInvalid();
  m_bIsBaked = false;

  const xiiWorld* pWorld = xiiWorld::GetWorld(m_uiWorldIndex);
  if (pWorld == nullptr)
  {
    return XII_FAILURE;
  }

  const xiiWorld& world = *pWorld;
  XII_LOCK(world.GetReadMarker());

  // settings
  {
    auto pManager   = world.GetComponentManager<xiiBakedProbesComponentManager>();
    auto pComponent = pManager->GetSingletonComponent();

    m_Settings = pComponent->m_Settings;
  }

  // volumes
  {
    if (auto pManager = world.GetComponentManager<xiiBakedProbesVolumeComponentManager>())
    {
      for (auto it = pManager->GetComponents(); it.IsValid(); ++it)
      {
        if (it->IsActiveAndInitialized())
        {
          xiiSimdTransform scaledTransform = it->GetOwner()->GetGlobalTransformSimd();
          scaledTransform.m_Scale          = scaledTransform.m_Scale.CompMul(xiiSimdConversion::ToVec3(it->GetExtents())) * 0.5f;

          auto& volume                    = m_Volumes.ExpandAndGetRef();
          volume.m_GlobalToLocalTransform = scaledTransform.GetAsMat4().GetInverse();

          xiiBoundingBoxSphere globalBounds = it->GetOwner()->GetGlobalBounds();
          if (globalBounds.IsValid())
          {
            m_BoundingBox.ExpandToInclude(globalBounds.GetBox());
          }
        }
      }
    }

    if (m_Volumes.IsEmpty())
    {
      xiiLog::Error("No Baked Probes Volume found");
      return XII_FAILURE;
    }
  }

  xiiBoundingBox queryBox = m_BoundingBox;
  queryBox.Grow(xiiVec3(m_Settings.m_fMaxRayDistance));

  xiiSpatialSystem::QueryParams queryParams;
  queryParams.m_uiCategoryBitmask = xiiDefaultSpatialDataCategories::RenderStatic.GetBitmask();
  queryParams.m_ExcludeTags.SetByName("Editor");

  xiiMsgExtractGeometry msg;
  msg.m_Mode         = xiiWorldGeoExtractionUtil::ExtractionMode::RenderMesh;
  msg.m_pMeshObjects = &m_MeshObjects;

  world.GetSpatialSystem()->FindObjectsInBox(queryBox, queryParams, [&](xiiGameObject* pObject) {
    pObject->SendMessage(msg);

    return xiiVisitorExecution::Continue;
  });

  return XII_SUCCESS;
}

xiiResult xiiBakingScene::Bake(const xiiStringView& sOutputPath, xiiProgress& progress)
{
  XII_ASSERT_DEV(!xiiThreadUtils::IsMainThread(), "BakeScene must be executed on a worker thread");

  if (m_pTracer == nullptr)
  {
    m_pTracer = XII_DEFAULT_NEW(xiiTracerEmbree);
  }

  xiiProgressRange pgRange("Baking Scene", 2, true, &progress);
  pgRange.SetStepWeighting(0, 0.95f);
  pgRange.SetStepWeighting(1, 0.05f);

  if (!pgRange.BeginNextStep("Building Scene"))
    return XII_FAILURE;

  XII_SUCCEED_OR_RETURN(m_pTracer->BuildScene(*this));

  xiiBakingInternal::PlaceProbesTask placeProbesTask(m_Settings, m_BoundingBox, m_Volumes);
  placeProbesTask.Execute();

  xiiBakingInternal::SkyVisibilityTask skyVisibilityTask(m_Settings, *m_pTracer, placeProbesTask.GetProbePositions());
  skyVisibilityTask.Execute();

  if (!pgRange.BeginNextStep("Writing Result"))
    return XII_FAILURE;

  xiiStringBuilder sFullOutputPath = sOutputPath;
  sFullOutputPath.Append("_Global.xiiProbeTreeSector");

  xiiFileWriter file;
  XII_SUCCEED_OR_RETURN(file.Open(sFullOutputPath));

  xiiAssetFileHeader header;
  header.SetFileHashAndVersion(1, 1);
  XII_SUCCEED_OR_RETURN(header.Write(file));

  xiiProbeTreeSectorResourceDescriptor desc;
  desc.m_vGridOrigin    = placeProbesTask.GetGridOrigin();
  desc.m_vProbeSpacing  = m_Settings.m_vProbeSpacing;
  desc.m_vProbeCount    = placeProbesTask.GetProbeCount();
  desc.m_ProbePositions = placeProbesTask.GetProbePositions();
  desc.m_SkyVisibility  = skyVisibilityTask.GetSkyVisibility();

  XII_SUCCEED_OR_RETURN(desc.Serialize(file));

  m_bIsBaked = true;

  return XII_SUCCESS;
}

xiiResult xiiBakingScene::RenderDebugView(const xiiMat4& InverseViewProjection, xiiUInt32 uiWidth, xiiUInt32 uiHeight, xiiDynamicArray<xiiColorGammaUB>& out_Pixels, xiiProgress& progress) const
{
  if (!m_bIsBaked)
    return XII_FAILURE;

  const xiiUInt32 uiNumPixel = uiWidth * uiHeight;
  out_Pixels.SetCountUninitialized(uiNumPixel);

  xiiHybridArray<xiiTracerInterface::Ray, 128> rays;
  rays.SetCountUninitialized(128);

  xiiHybridArray<xiiTracerInterface::Hit, 128> hits;
  hits.SetCountUninitialized(128);

  xiiUInt32 uiStartPixel    = 0;
  xiiUInt32 uiPixelPerBatch = rays.GetCount();
  while (uiStartPixel < uiNumPixel)
  {
    uiPixelPerBatch = xiiMath::Min(uiPixelPerBatch, uiNumPixel - uiStartPixel);

    for (xiiUInt32 i = 0; i < uiPixelPerBatch; ++i)
    {
      xiiUInt32 uiPixelIndex = uiStartPixel + i;
      xiiUInt32 x            = uiPixelIndex % uiWidth;
      xiiUInt32 y            = uiHeight - (uiPixelIndex / uiWidth) - 1;

      auto& ray = rays[i];
      xiiGraphicsUtils::ConvertScreenPosToWorldPos(InverseViewProjection, 0, 0, uiWidth, uiHeight, xiiVec3(float(x), float(y), 0), ray.m_vStartPos, &ray.m_vDir).IgnoreResult();
      ray.m_fDistance = 1000.0f;
    }

    m_pTracer->TraceRays(rays, hits);

    for (xiiUInt32 i = 0; i < uiPixelPerBatch; ++i)
    {
      xiiUInt32 uiPixelIndex = uiStartPixel + i;

      auto& hit = hits[i];
      if (hit.m_fDistance >= 0.0f)
      {
        xiiVec3 normal           = hit.m_vNormal * 0.5f + xiiVec3(0.5f);
        out_Pixels[uiPixelIndex] = xiiColorGammaUB(xiiMath::ColorFloatToByte(normal.x), xiiMath::ColorFloatToByte(normal.y), xiiMath::ColorFloatToByte(normal.z));
      }
      else
      {
        out_Pixels[uiPixelIndex] = xiiColorGammaUB(0, 0, 0);
      }
    }

    uiStartPixel += uiPixelPerBatch;

    progress.SetCompletion((float)uiStartPixel / uiNumPixel);
    if (progress.WasCanceled())
      break;
  }

  return XII_SUCCESS;
}

xiiBakingScene::xiiBakingScene()  = default;
xiiBakingScene::~xiiBakingScene() = default;

//////////////////////////////////////////////////////////////////////////

namespace
{
  static xiiDynamicArray<xiiUniquePtr<xiiBakingScene>, xiiStaticAllocatorWrapper> s_BakingScenes;
}

XII_IMPLEMENT_SINGLETON(xiiBaking);

xiiBaking::xiiBaking() :
  m_SingletonRegistrar(this)
{
}

void xiiBaking::Startup()
{
}

void xiiBaking::Shutdown()
{
  s_BakingScenes.Clear();
}

xiiBakingScene* xiiBaking::GetOrCreateScene(const xiiWorld& world)
{
  const xiiUInt32 uiWorldIndex = world.GetIndex();

  s_BakingScenes.EnsureCount(uiWorldIndex + 1);
  if (s_BakingScenes[uiWorldIndex] == nullptr)
  {
    auto pScene            = XII_DEFAULT_NEW(xiiBakingScene);
    pScene->m_uiWorldIndex = uiWorldIndex;

    s_BakingScenes[uiWorldIndex] = pScene;
  }

  return s_BakingScenes[uiWorldIndex].Borrow();
}

xiiBakingScene* xiiBaking::GetScene(const xiiWorld& world)
{
  const xiiUInt32 uiWorldIndex = world.GetIndex();

  if (uiWorldIndex < s_BakingScenes.GetCount())
  {
    return s_BakingScenes[uiWorldIndex].Borrow();
  }

  return nullptr;
}

const xiiBakingScene* xiiBaking::GetScene(const xiiWorld& world) const
{
  const xiiUInt32 uiWorldIndex = world.GetIndex();

  if (uiWorldIndex < s_BakingScenes.GetCount())
  {
    return s_BakingScenes[uiWorldIndex].Borrow();
  }

  return nullptr;
}

xiiResult xiiBaking::RenderDebugView(const xiiWorld& world, const xiiMat4& InverseViewProjection, xiiUInt32 uiWidth, xiiUInt32 uiHeight, xiiDynamicArray<xiiColorGammaUB>& out_Pixels, xiiProgress& progress) const
{
  if (const xiiBakingScene* pScene = GetScene(world))
  {
    return pScene->RenderDebugView(InverseViewProjection, uiWidth, uiHeight, out_Pixels, progress);
  }

  return XII_FAILURE;
}
