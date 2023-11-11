#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/SimdMath/SimdBBox.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <GraphicsCore/Rasterizer/RasterizerObject.h>
#include <GraphicsCore/Rasterizer/RasterizerView.h>
#include <GraphicsCore/Rasterizer/Thirdparty/Occluder.h>
#include <GraphicsCore/Rasterizer/Thirdparty/Rasterizer.h>

xiiCVarInt cvar_SpatialCullingOcclusionMaxResolution("Spatial.Occlusion.MaxResolution", 512, xiiCVarFlags::Default, "Max resolution for occlusion buffers.");
xiiCVarInt cvar_SpatialCullingOcclusionMaxOccluders("Spatial.Occlusion.MaxOccluders", 64, xiiCVarFlags::Default, "Max number of occluders to rasterize per frame.");

xiiRasterizerView::xiiRasterizerView()  = default;
xiiRasterizerView::~xiiRasterizerView() = default;

void xiiRasterizerView::SetResolution(xiiUInt32 uiWidth, xiiUInt32 uiHeight, float fAspectRatio)
{
  if (m_uiResolutionX != uiWidth || m_uiResolutionY != uiHeight)
  {
    m_uiResolutionX = uiWidth;
    m_uiResolutionY = uiHeight;

    m_pRasterizer = XII_DEFAULT_NEW(Rasterizer, uiWidth, uiHeight);
  }

  if (fAspectRatio == 0.0f)
    m_fAspectRation = float(m_uiResolutionX) / float(m_uiResolutionY);
  else
    m_fAspectRation = fAspectRatio;
}

void xiiRasterizerView::BeginScene()
{
  XII_ASSERT_DEV(m_pRasterizer != nullptr, "Call SetResolution() first.");

  XII_PROFILE_SCOPE("Occlusion::Clear");

  m_pRasterizer->clear();
  m_bAnyOccludersRasterized = false;
}

void xiiRasterizerView::ReadBackFrame(xiiArrayPtr<xiiColorLinearUB> targetBuffer) const
{
  XII_PROFILE_SCOPE("Occlusion::ReadFrame");

  XII_ASSERT_DEV(m_pRasterizer != nullptr, "Call SetResolution() first.");
  XII_ASSERT_DEV(targetBuffer.GetCount() >= m_uiResolutionX * m_uiResolutionY, "Target buffer is too small.");

  m_pRasterizer->readBackDepth(targetBuffer.GetPtr());
}

void xiiRasterizerView::EndScene()
{
  if (m_Instances.IsEmpty())
    return;

  XII_PROFILE_SCOPE("Occlusion::RasterizeScene");

  SortObjectsFrontToBack();

  UpdateViewProjectionMatrix();

  // only rasterize a limited number of the closest objects
  RasterizeObjects(cvar_SpatialCullingOcclusionMaxOccluders);

  m_Instances.Clear();

  m_pRasterizer->setModelViewProjection(m_mViewProjection.m_fElementsCM);
}

void xiiRasterizerView::RasterizeObjects(xiiUInt32 uiMaxObjects)
{
#if XII_ENABLED(XII_RASTERIZER_SUPPORTED)

  XII_PROFILE_SCOPE("Occlusion::RasterizeObjects");

  for (const Instance& inst : m_Instances)
  {
    ApplyModelViewProjectionMatrix(inst.m_Transform);

    bool            bNeedsClipping;
    const Occluder& occluder = inst.m_pObject->m_Occluder;

    if (m_pRasterizer->queryVisibility(occluder.m_boundsMin, occluder.m_boundsMax, bNeedsClipping))
    {
      m_bAnyOccludersRasterized = true;

      if (bNeedsClipping)
      {
        m_pRasterizer->rasterize<true>(occluder);
      }
      else
      {
        m_pRasterizer->rasterize<false>(occluder);
      }

      if (--uiMaxObjects == 0)
        return;
    }
  }
#endif
}

void xiiRasterizerView::UpdateViewProjectionMatrix()
{
  xiiMat4 mProjection;
  m_pCamera->GetProjectionMatrix(m_fAspectRation, mProjection, xiiCameraEye::Left, xiiClipSpaceDepthRange::ZeroToOne);

  m_mViewProjection = mProjection * m_pCamera->GetViewMatrix();
}

void xiiRasterizerView::ApplyModelViewProjectionMatrix(const xiiTransform& modelTransform)
{
  const xiiMat4 mModel = modelTransform.GetAsMat4();
  const xiiMat4 mMVP   = m_mViewProjection * mModel;

  m_pRasterizer->setModelViewProjection(mMVP.m_fElementsCM);
}

void xiiRasterizerView::SortObjectsFrontToBack()
{
#if XII_ENABLED(XII_RASTERIZER_SUPPORTED)
  XII_PROFILE_SCOPE("Occlusion::SortObjects");

  const xiiVec3 camPos = m_pCamera->GetCenterPosition();

  m_Instances.Sort([&](const Instance& i1, const Instance& i2) {
      const float d1 = (i1.m_Transform.m_vPosition - camPos).GetLengthSquared();
      const float d2 = (i2.m_Transform.m_vPosition - camPos).GetLengthSquared();

      return d1 < d2; });
#endif
}

bool xiiRasterizerView::IsVisible(const xiiSimdBBox& aabb) const
{
#if XII_ENABLED(XII_RASTERIZER_SUPPORTED)
  if (!m_bAnyOccludersRasterized)
    return true; // assume that people already do frustum culling anyway

  XII_PROFILE_SCOPE("Occlusion::IsVisible");

  xiiSimdVec4f vmin = aabb.m_Min;
  xiiSimdVec4f vmax = aabb.m_Max;

  // xiiSimdBBox makes no guarantees what's in the W component
  // but the SW rasterizer requires them to be 1
  vmin.SetW(1);
  vmax.SetW(1);

  bool needsClipping = false;
  return m_pRasterizer->queryVisibility(vmin.m_v, vmax.m_v, needsClipping);
#else
  return true;
#endif
}

xiiRasterizerView* xiiRasterizerViewPool::GetRasterizerView(xiiUInt32 uiWidth, xiiUInt32 uiHeight, float fAspectRatio)
{
  XII_PROFILE_SCOPE("Occlusion::GetViewFromPool");

  XII_LOCK(m_Mutex);

  const float divX = (float)uiWidth / (float)cvar_SpatialCullingOcclusionMaxResolution;
  const float divY = (float)uiHeight / (float)cvar_SpatialCullingOcclusionMaxResolution;
  const float div  = xiiMath::Max(divX, divY);

  if (div > 1.0)
  {
    uiWidth  = (xiiUInt32)(uiWidth / div);
    uiHeight = (xiiUInt32)(uiHeight / div);
  }

  uiWidth  = xiiMath::RoundDown(uiWidth, 8);
  uiHeight = xiiMath::RoundDown(uiHeight, 8);

  uiWidth  = xiiMath::Clamp<xiiUInt32>(uiWidth, 32u, cvar_SpatialCullingOcclusionMaxResolution);
  uiHeight = xiiMath::Clamp<xiiUInt32>(uiHeight, 32u, cvar_SpatialCullingOcclusionMaxResolution);

  for (PoolEntry& entry : m_Entries)
  {
    if (entry.m_bInUse)
      continue;

    if (entry.m_RasterizerView.GetResolutionX() == uiWidth && entry.m_RasterizerView.GetResolutionY() == uiHeight)
    {
      entry.m_bInUse = true;
      entry.m_RasterizerView.SetResolution(uiWidth, uiHeight, fAspectRatio);
      return &entry.m_RasterizerView;
    }
  }

  auto& ne = m_Entries.ExpandAndGetRef();
  ne.m_RasterizerView.SetResolution(uiWidth, uiHeight, fAspectRatio);
  ne.m_bInUse = true;

  return &ne.m_RasterizerView;
}

void xiiRasterizerViewPool::ReturnRasterizerView(xiiRasterizerView* pView)
{
  if (pView == nullptr)
    return;

  XII_PROFILE_SCOPE("Occlusion::ReturnViewToPool");

  pView->SetCamera(nullptr);

  XII_LOCK(m_Mutex);

  for (PoolEntry& entry : m_Entries)
  {
    if (&entry.m_RasterizerView == pView)
    {
      entry.m_bInUse = false;
      return;
    }
  }

  XII_ASSERT_NOT_IMPLEMENTED;
}


XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Rasterizer_Implementation_RasterizerView);
