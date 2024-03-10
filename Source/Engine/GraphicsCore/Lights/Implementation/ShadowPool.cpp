#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Profiling/Profiling.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Lights/DirectionalLightComponent.h>
#include <GraphicsCore/Lights/Implementation/ShadowPool.h>
#include <GraphicsCore/Lights/PointLightComponent.h>
#include <GraphicsCore/Lights/SpotLightComponent.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>
#include <GraphicsFoundation/CommandEncoder/GraphicsCommandEncoder.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Device/Pass.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Texture.h>

#include <GraphicsCore/../../../Data/Base/Shaders/Common/LightData.h>

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsCore, ShadowPool)
BEGIN_SUBSYSTEM_DEPENDENCIES
"Foundation",
"Core",
"RenderWorld"
END_SUBSYSTEM_DEPENDENCIES

ON_HIGHLEVELSYSTEMS_STARTUP
{
  xiiShadowPool::OnEngineStartup();
}

ON_HIGHLEVELSYSTEMS_SHUTDOWN
{
  xiiShadowPool::OnEngineShutdown();
}

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
xiiCVarBool cvar_RenderingShadowsShowPoolStats("Rendering.Shadows.ShowPoolStats", false, xiiCVarFlags::Default, "Display same stats of the shadow pool");
#endif

static xiiUInt32 s_uiShadowAtlasTextureWidth  = 4096; ///\todo make this configurable
static xiiUInt32 s_uiShadowAtlasTextureHeight = 4096;
static xiiUInt32 s_uiShadowMapSize            = 1024;
static xiiUInt32 s_uiMinShadowMapSize         = 64;
static float     s_fFadeOutScaleStart         = (s_uiMinShadowMapSize + 1.0f) / s_uiShadowMapSize;
static float     s_fFadeOutScaleEnd           = s_fFadeOutScaleStart * 0.5f;

struct ShadowView
{
  xiiViewHandle m_hView;
  xiiCamera     m_Camera;
};

struct ShadowData
{
  xiiHybridArray<xiiViewHandle, 6> m_Views;
  xiiUInt32                        m_uiType;
  float                            m_fShadowMapScale;
  float                            m_fPenumbraSize;
  float                            m_fSlopeBias;
  float                            m_fConstantBias;
  float                            m_fFadeOutStart;
  float                            m_fMinRange;
  xiiUInt32                        m_uiPackedDataOffset; // in 16 bytes steps
};

struct LightAndRefView
{
  XII_DECLARE_POD_TYPE();

  const xiiLightComponent* m_pLight;
  const xiiView*           m_pReferenceView;
};

struct SortedShadowData
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiIndex;
  float     m_fShadowMapScale;

  XII_ALWAYS_INLINE bool operator<(const SortedShadowData& other) const
  {
    if (m_fShadowMapScale > other.m_fShadowMapScale) // we want to sort descending (higher scale first)
      return true;

    return m_uiIndex < other.m_uiIndex;
  }
};

static xiiDynamicArray<SortedShadowData> s_SortedShadowData;

struct AtlasCell
{
  XII_DECLARE_POD_TYPE();

  XII_ALWAYS_INLINE AtlasCell() :
    m_Rect(0, 0, 0, 0)
  {
    m_uiChildIndices[0] = m_uiChildIndices[1] = m_uiChildIndices[2] = m_uiChildIndices[3] = 0xFFFF;
    m_uiDataIndex                                                                         = xiiInvalidIndex;
  }

  XII_ALWAYS_INLINE bool IsLeaf() const
  {
    return m_uiChildIndices[0] == 0xFFFF && m_uiChildIndices[1] == 0xFFFF && m_uiChildIndices[2] == 0xFFFF && m_uiChildIndices[3] == 0xFFFF;
  }

  xiiRectU32 m_Rect;
  xiiUInt16  m_uiChildIndices[4];
  xiiUInt32  m_uiDataIndex;
};

static xiiDeque<AtlasCell> s_AtlasCells;

static AtlasCell* Insert(AtlasCell* pCell, xiiUInt32 uiShadowMapSize, xiiUInt32 uiDataIndex)
{
  if (!pCell->IsLeaf())
  {
    for (xiiUInt32 i = 0; i < 4; ++i)
    {
      AtlasCell* pChildCell = &s_AtlasCells[pCell->m_uiChildIndices[i]];
      if (AtlasCell* pNewCell = Insert(pChildCell, uiShadowMapSize, uiDataIndex))
      {
        return pNewCell;
      }
    }

    return nullptr;
  }
  else
  {
    if (pCell->m_uiDataIndex != xiiInvalidIndex)
      return nullptr;

    if (pCell->m_Rect.width < uiShadowMapSize || pCell->m_Rect.height < uiShadowMapSize)
      return nullptr;

    if (pCell->m_Rect.width == uiShadowMapSize && pCell->m_Rect.height == uiShadowMapSize)
    {
      pCell->m_uiDataIndex = uiDataIndex;
      return pCell;
    }

    // Split
    xiiUInt32 x = pCell->m_Rect.x;
    xiiUInt32 y = pCell->m_Rect.y;
    xiiUInt32 w = pCell->m_Rect.width / 2;
    xiiUInt32 h = pCell->m_Rect.height / 2;

    xiiUInt32 uiCellIndex                 = s_AtlasCells.GetCount();
    s_AtlasCells.ExpandAndGetRef().m_Rect = xiiRectU32(x, y, w, h);
    s_AtlasCells.ExpandAndGetRef().m_Rect = xiiRectU32(x + w, y, w, h);
    s_AtlasCells.ExpandAndGetRef().m_Rect = xiiRectU32(x, y + h, w, h);
    s_AtlasCells.ExpandAndGetRef().m_Rect = xiiRectU32(x + w, y + h, w, h);

    for (xiiUInt32 i = 0; i < 4; ++i)
    {
      pCell->m_uiChildIndices[i] = static_cast<xiiUInt16>(uiCellIndex + i);
    }

    AtlasCell* pChildCell = &s_AtlasCells[pCell->m_uiChildIndices[0]];
    return Insert(pChildCell, uiShadowMapSize, uiDataIndex);
  }
}

static xiiRectU32 FindAtlasRect(xiiUInt32 uiShadowMapSize, xiiUInt32 uiDataIndex)
{
  XII_ASSERT_DEBUG(xiiMath::IsPowerOf2(uiShadowMapSize), "Size must be power of 2");

  AtlasCell* pCell = Insert(&s_AtlasCells[0], uiShadowMapSize, uiDataIndex);
  if (pCell != nullptr)
  {
    XII_ASSERT_DEBUG(pCell->IsLeaf() && pCell->m_uiDataIndex == uiDataIndex, "Implementation error");
    return pCell->m_Rect;
  }

  xiiLog::Warning("Shadow Pool is full. Not enough space for a {0}x{0} shadow map. The light will have no shadow.", uiShadowMapSize);
  return xiiRectU32(0, 0, 0, 0);
}

static float AddSafeBorder(xiiAngle fov, float fPenumbraSize)
{
  float fHalfHeight = xiiMath::Tan(fov * 0.5f);
  float fNewFov     = xiiMath::ATan(fHalfHeight + fPenumbraSize).GetDegree() * 2.0f;
  return fNewFov;
}

xiiTagSet s_ExcludeTagsWhiteList;

static void CopyExcludeTagsOnWhiteList(const xiiTagSet& referenceTags, xiiTagSet& out_targetTags)
{
  out_targetTags.Clear();
  out_targetTags.SetByName("EditorHidden");

  for (auto& tag : referenceTags)
  {
    if (s_ExcludeTagsWhiteList.IsSet(tag))
    {
      out_targetTags.Set(tag);
    }
  }
}

// must not be in anonymous namespace
template <>
struct xiiHashHelper<LightAndRefView>
{
  XII_ALWAYS_INLINE static xiiUInt32 Hash(LightAndRefView value) { return xiiHashingUtils::xxHash32(&value.m_pLight, sizeof(LightAndRefView)); }

  XII_ALWAYS_INLINE static bool Equal(const LightAndRefView& a, const LightAndRefView& b)
  {
    return a.m_pLight == b.m_pLight && a.m_pReferenceView == b.m_pReferenceView;
  }
};

//////////////////////////////////////////////////////////////////////////

struct xiiShadowPool::Data
{
  Data() { Clear(); }

  ~Data()
  {
    for (auto& shadowView : m_ShadowViews)
    {
      xiiRenderWorld::DeleteView(shadowView.m_hView);
    }

    xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();
    if (!m_hShadowAtlasTexture.IsInvalidated())
    {
      pDevice->DestroyTexture(m_hShadowAtlasTexture);
      m_hShadowAtlasTexture.Invalidate();
    }

    if (!m_hShadowDataBuffer.IsInvalidated())
    {
      pDevice->DestroyBuffer(m_hShadowDataBuffer);
      m_hShadowDataBuffer.Invalidate();
    }
  }

  enum
  {
    MAX_SHADOW_DATA = 1024
  };

  void CreateShadowAtlasTexture()
  {
    if (m_hShadowAtlasTexture.IsInvalidated())
    {
      xiiGALTextureCreationDescription desc;
      desc.m_sName              = "Shadow Atlas Texture";
      desc.m_Type               = xiiGALResourceDimension::Texture2D;
      desc.m_Format             = xiiGALTextureFormat::D16UNormalized;
      desc.m_Size.width         = s_uiShadowAtlasTextureWidth;
      desc.m_Size.height        = s_uiShadowAtlasTextureHeight;
      desc.m_uiArraySizeOrDepth = 1;
      desc.m_uiMipLevels        = 1;
      desc.m_uiSampleCount      = xiiGALSampleCount::OneSample;
      desc.m_BindFlags          = xiiGALBindFlags::DepthStencil | xiiGALBindFlags::ShaderResource;

      m_hShadowAtlasTexture = xiiGALDevice::GetDefaultDevice()->CreateTexture(desc);
    }
  }

  void CreateShadowDataBuffer()
  {
    if (m_hShadowDataBuffer.IsInvalidated())
    {
      xiiGALBufferCreationDescription desc;
      desc.m_uiElementByteStride = sizeof(xiiVec4);
      desc.m_uiSize              = desc.m_uiElementByteStride * MAX_SHADOW_DATA;
      desc.m_Mode                = xiiGALBufferMode::Structured;
      desc.m_BindFlags           = xiiGALBindFlags::ShaderResource;
      desc.m_ResourceUsage       = xiiGALResourceUsage::Dynamic;
      desc.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;

      m_hShadowDataBuffer = xiiGALDevice::GetDefaultDevice()->CreateBuffer(desc);
    }
  }

  xiiViewHandle CreateShadowView()
  {
    CreateShadowAtlasTexture();
    CreateShadowDataBuffer();

    xiiView*      pView = nullptr;
    xiiViewHandle hView = xiiRenderWorld::CreateView("Unknown", pView);

    pView->SetCameraUsageHint(xiiCameraUsageHint::Shadow);

    xiiGALRenderTargets renderTargets;
    renderTargets.m_hDSTarget = m_hShadowAtlasTexture;
    pView->SetRenderTargets(renderTargets);

    XII_ASSERT_DEV(m_ShadowViewsMutex.IsLocked(), "m_ShadowViewsMutex must be locked at this point.");
    m_ShadowViewsMutex.Unlock(); // if the resource gets loaded in the call below, his could lead to a deadlock

    // ShadowMapRenderPipeline.xiiRenderPipelineAsset
    pView->SetRenderPipelineResource(xiiResourceManager::LoadResource<xiiRenderPipelineResource>("{ 4f4d9f16-3d47-4c67-b821-a778f11dcaf5 }"));

    m_ShadowViewsMutex.Lock();

    // Set viewport size to something valid, this will be changed to the proper location in the atlas texture in OnEndExtraction before
    // rendering.
    pView->SetViewport(xiiRectFloat(0.0f, 0.0f, 1024.0f, 1024.0f));

    const xiiTag& tagCastShadows = xiiTagRegistry::GetGlobalRegistry().RegisterTag("CastShadow");
    pView->m_IncludeTags.Set(tagCastShadows);

    pView->m_ExcludeTags.SetByName("EditorHidden");

    return hView;
  }

  ShadowView& GetShadowView(xiiView*& out_pView)
  {
    XII_LOCK(m_ShadowViewsMutex);

    if (m_uiUsedViews == m_ShadowViews.GetCount())
    {
      m_ShadowViews.ExpandAndGetRef().m_hView = CreateShadowView();
    }

    auto& shadowView = m_ShadowViews[m_uiUsedViews];
    if (xiiRenderWorld::TryGetView(shadowView.m_hView, out_pView))
    {
      out_pView->SetCamera(&shadowView.m_Camera);
      out_pView->SetLodCamera(nullptr);
    }

    m_uiUsedViews++;
    return shadowView;
  }

  bool GetDataForExtraction(const xiiLightComponent* pLight, const xiiView* pReferenceView, float fShadowMapScale, xiiUInt32 uiPackedDataSizeInBytes, ShadowData*& out_pData)
  {
    XII_LOCK(m_ShadowDataMutex);

    LightAndRefView key = {pLight, pReferenceView};

    xiiUInt32 uiDataIndex = xiiInvalidIndex;
    if (m_LightToShadowDataTable.TryGetValue(key, uiDataIndex))
    {
      out_pData                    = &m_ShadowData[uiDataIndex];
      out_pData->m_fShadowMapScale = xiiMath::Max(out_pData->m_fShadowMapScale, fShadowMapScale);
      return true;
    }

    m_ShadowData.EnsureCount(m_uiUsedShadowData + 1);

    out_pData                       = &m_ShadowData[m_uiUsedShadowData];
    out_pData->m_fShadowMapScale    = fShadowMapScale;
    out_pData->m_fPenumbraSize      = pLight->GetPenumbraSize();
    out_pData->m_fSlopeBias         = pLight->GetSlopeBias() * 100.0f;    // map from user friendly range to real range
    out_pData->m_fConstantBias      = pLight->GetConstantBias() / 100.0f; // map from user friendly range to real range
    out_pData->m_fFadeOutStart      = 1.0f;
    out_pData->m_fMinRange          = 1.0f;
    out_pData->m_uiPackedDataOffset = m_uiUsedPackedShadowData;

    m_LightToShadowDataTable.Insert(key, m_uiUsedShadowData);

    ++m_uiUsedShadowData;
    m_uiUsedPackedShadowData += uiPackedDataSizeInBytes / sizeof(xiiVec4);

    return false;
  }

  void Clear()
  {
    m_uiUsedViews      = 0;
    m_uiUsedShadowData = 0;

    m_LightToShadowDataTable.Clear();

    m_uiUsedPackedShadowData = 0;
  }

  xiiMutex             m_ShadowViewsMutex;
  xiiDeque<ShadowView> m_ShadowViews;
  xiiUInt32            m_uiUsedViews = 0;

  xiiMutex                                 m_ShadowDataMutex;
  xiiDeque<ShadowData>                     m_ShadowData;
  xiiUInt32                                m_uiUsedShadowData = 0;
  xiiHashTable<LightAndRefView, xiiUInt32> m_LightToShadowDataTable;

  xiiDynamicArray<xiiVec4, xiiAlignedAllocatorWrapper> m_PackedShadowData[2];
  xiiUInt32                                            m_uiUsedPackedShadowData = 0; // in 16 bytes steps (sizeof(xiiVec4))

  xiiGALTextureHandle m_hShadowAtlasTexture;
  xiiGALBufferHandle  m_hShadowDataBuffer;
};

//////////////////////////////////////////////////////////////////////////

xiiShadowPool::Data* xiiShadowPool::s_pData = nullptr;

// static
xiiUInt32 xiiShadowPool::AddDirectionalLight(const xiiDirectionalLightComponent* pDirLight, const xiiView* pReferenceView)
{
  XII_ASSERT_DEBUG(pDirLight->GetCastShadows(), "Implementation error");

  // No shadows in orthographic views
  if (pReferenceView->GetCullingCamera()->IsOrthographic())
  {
    return xiiInvalidIndex;
  }

  float fMaxReferenceSize = xiiMath::Max(pReferenceView->GetViewport().width, pReferenceView->GetViewport().height);
  float fShadowMapScale   = fMaxReferenceSize / s_uiShadowMapSize;

  ShadowData* pData = nullptr;
  if (s_pData->GetDataForExtraction(pDirLight, pReferenceView, fShadowMapScale, sizeof(xiiDirShadowData), pData))
  {
    return pData->m_uiPackedDataOffset;
  }

  xiiUInt32        uiNumCascades    = xiiMath::Min(pDirLight->GetNumCascades(), 4u);
  const xiiCamera* pReferenceCamera = pReferenceView->GetCullingCamera();

  pData->m_uiType        = LIGHT_TYPE_DIR;
  pData->m_fFadeOutStart = pDirLight->GetFadeOutStart();
  pData->m_fMinRange     = pDirLight->GetMinShadowRange();
  pData->m_Views.SetCount(uiNumCascades);

  // determine cascade ranges
  float fNearPlane       = pReferenceCamera->GetNearPlane();
  float fShadowRange     = pDirLight->GetMinShadowRange();
  float fSplitModeWeight = pDirLight->GetSplitModeWeight();

  float fCascadeRanges[4];
  for (xiiUInt32 i = 0; i < uiNumCascades; ++i)
  {
    float f              = float(i + 1) / uiNumCascades;
    float logDistance    = fNearPlane * xiiMath::Pow(fShadowRange / fNearPlane, f);
    float linearDistance = fNearPlane + (fShadowRange - fNearPlane) * f;
    fCascadeRanges[i]    = xiiMath::Lerp(linearDistance, logDistance, fSplitModeWeight);
  }

  const char* viewNames[4] = {"DirLightViewC0", "DirLightViewC1", "DirLightViewC2", "DirLightViewC3"};

  const xiiGameObject* pOwner            = pDirLight->GetOwner();
  const xiiVec3        vLightDirForwards = pOwner->GetGlobalDirForwards();
  const xiiVec3        vLightDirUp       = pOwner->GetGlobalDirUp();

  float fAspectRatio = pReferenceView->GetViewport().width / pReferenceView->GetViewport().height;

  float   fCascadeStart = 0.0f;
  float   fCascadeEnd   = 0.0f;
  float   fTanFovX      = xiiMath::Tan(pReferenceCamera->GetFovX(fAspectRatio) * 0.5f);
  float   fTanFovY      = xiiMath::Tan(pReferenceCamera->GetFovY(fAspectRatio) * 0.5f);
  xiiVec3 corner        = xiiVec3(fTanFovX, fTanFovY, 1.0f);

  float fNearPlaneOffset = pDirLight->GetNearPlaneOffset();

  for (xiiUInt32 i = 0; i < uiNumCascades; ++i)
  {
    xiiView*    pView      = nullptr;
    ShadowView& shadowView = s_pData->GetShadowView(pView);
    pData->m_Views[i]      = shadowView.m_hView;

    // Setup view
    {
      pView->SetName(viewNames[i]);
      pView->SetWorld(const_cast<xiiWorld*>(pDirLight->GetWorld()));
      pView->SetLodCamera(pReferenceCamera);
      CopyExcludeTagsOnWhiteList(pReferenceView->m_ExcludeTags, pView->m_ExcludeTags);
    }

    // Setup camera
    {
      fCascadeStart = fCascadeEnd;
      fCascadeEnd   = fCascadeRanges[i];

      xiiVec3 startCorner = corner * fCascadeStart;
      xiiVec3 endCorner   = corner * fCascadeEnd;

      // Find the enclosing sphere for the frustum:
      // The sphere center must be on the view's center ray and should be equally far away from the corner points.
      // x = distance from camera origin to sphere center
      // d1^2 = sc.x^2 + sc.y^2 + (x - sc.z)^2
      // d2^2 = ec.x^2 + ec.y^2 + (x - ec.z)^2
      // d1 == d2 and solve for x:
      float x = (endCorner.Dot(endCorner) - startCorner.Dot(startCorner)) / (2.0f * (endCorner.z - startCorner.z));
      x       = xiiMath::Min(x, fCascadeEnd);

      xiiVec3 center = pReferenceCamera->GetPosition() + pReferenceCamera->GetDirForwards() * x;

      // prevent too large values
      // sometimes this can happen when imported data is badly scaled and thus way too large
      // then adding dirForwards result in no change and we run into other asserts later
      center.x = xiiMath::Clamp(center.x, -1000000.0f, +1000000.0f);
      center.y = xiiMath::Clamp(center.y, -1000000.0f, +1000000.0f);
      center.z = xiiMath::Clamp(center.z, -1000000.0f, +1000000.0f);

      endCorner.z -= x;
      float radius = endCorner.GetLength();

      if (false)
      {
        xiiDebugRenderer::DrawLineSphere(pReferenceView->GetHandle(), xiiBoundingSphere(center, radius), xiiColor::OrangeRed);
      }

      float   fCameraToCenterDistance = radius + fNearPlaneOffset;
      xiiVec3 shadowCameraPos         = center - vLightDirForwards * fCameraToCenterDistance;
      float   fFarPlane               = radius + fCameraToCenterDistance;

      xiiCamera& camera = shadowView.m_Camera;
      camera.LookAt(shadowCameraPos, center, vLightDirUp);
      camera.SetCameraMode(xiiCameraMode::OrthoFixedWidth, radius * 2.0f, 0.0f, fFarPlane);

      // stabilize
      xiiMat4 worldToLightMatrix = pView->GetViewMatrix(xiiCameraEye::Left);
      xiiVec3 offset             = worldToLightMatrix.TransformPosition(xiiVec3::ZeroVector());
      float   texelInWorld       = (2.0f * radius) / s_uiShadowMapSize;
      offset.x -= xiiMath::Floor(offset.x / texelInWorld) * texelInWorld;
      offset.y -= xiiMath::Floor(offset.y / texelInWorld) * texelInWorld;

      camera.MoveLocally(0.0f, offset.x, offset.y);
    }

    xiiRenderWorld::AddViewToRender(shadowView.m_hView);
  }

  return pData->m_uiPackedDataOffset;
}

// static
xiiUInt32 xiiShadowPool::AddPointLight(const xiiPointLightComponent* pPointLight, float fScreenSpaceSize, const xiiView* pReferenceView)
{
  XII_ASSERT_DEBUG(pPointLight->GetCastShadows(), "Implementation error");

  if (fScreenSpaceSize < s_fFadeOutScaleEnd * 2.0f)
  {
    return xiiInvalidIndex;
  }

  ShadowData* pData = nullptr;
  if (s_pData->GetDataForExtraction(pPointLight, nullptr, fScreenSpaceSize, sizeof(xiiPointShadowData), pData))
  {
    return pData->m_uiPackedDataOffset;
  }

  pData->m_uiType = LIGHT_TYPE_POINT;
  pData->m_Views.SetCount(6);

  xiiVec3 faceDirs[6] = {
    xiiVec3(1.0f, 0.0f, 0.0f),
    xiiVec3(-1.0f, 0.0f, 0.0f),
    xiiVec3(0.0f, 1.0f, 0.0f),
    xiiVec3(0.0f, -1.0f, 0.0f),
    xiiVec3(0.0f, 0.0f, 1.0f),
    xiiVec3(0.0f, 0.0f, -1.0f),
  };

  const char* viewNames[6] = {
    "PointLightView+X",
    "PointLightView-X",
    "PointLightView+Y",
    "PointLightView-Y",
    "PointLightView+Z",
    "PointLightView-Z",
  };

  const xiiGameObject* pOwner    = pPointLight->GetOwner();
  xiiVec3              vPosition = pOwner->GetGlobalPosition();
  xiiVec3              vUp       = xiiVec3(0.0f, 0.0f, 1.0f);

  float fPenumbraSize = xiiMath::Max(pPointLight->GetPenumbraSize(), (0.5f / s_uiMinShadowMapSize)); // at least one texel for hardware pcf
  float fFov          = AddSafeBorder(xiiAngle::Degree(90.0f), fPenumbraSize);

  float fNearPlane = 0.1f; ///\todo expose somewhere
  float fFarPlane  = pPointLight->GetEffectiveRange();

  for (xiiUInt32 i = 0; i < 6; ++i)
  {
    xiiView*    pView      = nullptr;
    ShadowView& shadowView = s_pData->GetShadowView(pView);
    pData->m_Views[i]      = shadowView.m_hView;

    // Setup view
    {
      pView->SetName(viewNames[i]);
      pView->SetWorld(const_cast<xiiWorld*>(pPointLight->GetWorld()));
      CopyExcludeTagsOnWhiteList(pReferenceView->m_ExcludeTags, pView->m_ExcludeTags);
    }

    // Setup camera
    {
      xiiVec3 vForward = faceDirs[i];

      xiiCamera& camera = shadowView.m_Camera;
      camera.LookAt(vPosition, vPosition + vForward, vUp);
      camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovX, fFov, fNearPlane, fFarPlane);
    }

    xiiRenderWorld::AddViewToRender(shadowView.m_hView);
  }

  return pData->m_uiPackedDataOffset;
}

// static
xiiUInt32 xiiShadowPool::AddSpotLight(const xiiSpotLightComponent* pSpotLight, float fScreenSpaceSize, const xiiView* pReferenceView)
{
  XII_ASSERT_DEBUG(pSpotLight->GetCastShadows(), "Implementation error");

  if (fScreenSpaceSize < s_fFadeOutScaleEnd)
  {
    return xiiInvalidIndex;
  }

  ShadowData* pData = nullptr;
  if (s_pData->GetDataForExtraction(pSpotLight, nullptr, fScreenSpaceSize, sizeof(xiiSpotShadowData), pData))
  {
    return pData->m_uiPackedDataOffset;
  }

  pData->m_uiType = LIGHT_TYPE_SPOT;
  pData->m_Views.SetCount(1);

  xiiView*    pView      = nullptr;
  ShadowView& shadowView = s_pData->GetShadowView(pView);
  pData->m_Views[0]      = shadowView.m_hView;

  // Setup view
  {
    pView->SetName("SpotLightView");
    pView->SetWorld(const_cast<xiiWorld*>(pSpotLight->GetWorld()));
    CopyExcludeTagsOnWhiteList(pReferenceView->m_ExcludeTags, pView->m_ExcludeTags);
  }

  // Setup camera
  {
    const xiiGameObject* pOwner    = pSpotLight->GetOwner();
    xiiVec3              vPosition = pOwner->GetGlobalPosition();
    xiiVec3              vForward  = pOwner->GetGlobalDirForwards();
    xiiVec3              vUp       = pOwner->GetGlobalDirUp();

    float fFov       = AddSafeBorder(pSpotLight->GetOuterSpotAngle(), pSpotLight->GetPenumbraSize());
    float fNearPlane = 0.1f; ///\todo expose somewhere
    float fFarPlane  = pSpotLight->GetEffectiveRange();

    xiiCamera& camera = shadowView.m_Camera;
    camera.LookAt(vPosition, vPosition + vForward, vUp);
    camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovX, fFov, fNearPlane, fFarPlane);
  }

  xiiRenderWorld::AddViewToRender(shadowView.m_hView);

  return pData->m_uiPackedDataOffset;
}

// static
xiiGALTextureHandle xiiShadowPool::GetShadowAtlasTexture()
{
  return s_pData->m_hShadowAtlasTexture;
}

// static
xiiGALBufferHandle xiiShadowPool::GetShadowDataBuffer()
{
  return s_pData->m_hShadowDataBuffer;
}

// static
void xiiShadowPool::AddExcludeTagToWhiteList(const xiiTag& tag)
{
  s_ExcludeTagsWhiteList.Set(tag);
}

// static
void xiiShadowPool::OnEngineStartup()
{
  s_pData = XII_DEFAULT_NEW(xiiShadowPool::Data);

  xiiRenderWorld::GetExtractionEvent().AddEventHandler(OnExtractionEvent);
  xiiRenderWorld::GetRenderEvent().AddEventHandler(OnRenderEvent);
}

// static
void xiiShadowPool::OnEngineShutdown()
{
  xiiRenderWorld::GetExtractionEvent().RemoveEventHandler(OnExtractionEvent);
  xiiRenderWorld::GetRenderEvent().RemoveEventHandler(OnRenderEvent);

  XII_DEFAULT_DELETE(s_pData);
}

// static
void xiiShadowPool::OnExtractionEvent(const xiiRenderWorldExtractionEvent& e)
{
  if (e.m_Type != xiiRenderWorldExtractionEvent::Type::EndExtraction)
    return;

  XII_PROFILE_SCOPE("Shadow Pool Update");

  xiiUInt32 uiDataIndex      = xiiRenderWorld::GetDataIndexForExtraction();
  auto&     packedShadowData = s_pData->m_PackedShadowData[uiDataIndex];
  packedShadowData.SetCountUninitialized(s_pData->m_uiUsedPackedShadowData);

  if (s_pData->m_uiUsedShadowData == 0)
    return;

  // Sort by shadow map scale
  s_SortedShadowData.Clear();

  for (xiiUInt32 uiShadowDataIndex = 0; uiShadowDataIndex < s_pData->m_uiUsedShadowData; ++uiShadowDataIndex)
  {
    auto& shadowData = s_pData->m_ShadowData[uiShadowDataIndex];

    auto& sorted             = s_SortedShadowData.ExpandAndGetRef();
    sorted.m_uiIndex         = uiShadowDataIndex;
    sorted.m_fShadowMapScale = shadowData.m_uiType == LIGHT_TYPE_DIR ? 100.0f : xiiMath::Min(shadowData.m_fShadowMapScale, 10.0f);
  }

  s_SortedShadowData.Sort();

  // Prepare atlas
  s_AtlasCells.Clear();
  s_AtlasCells.ExpandAndGetRef().m_Rect = xiiRectU32(0, 0, s_uiShadowAtlasTextureWidth, s_uiShadowAtlasTextureHeight);

  float fAtlasInvWidth  = 1.0f / s_uiShadowAtlasTextureWidth;
  float fAtlasInvHeight = 1.0f / s_uiShadowAtlasTextureWidth;

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  xiiUInt32 uiTotalAtlasSize = s_uiShadowAtlasTextureWidth * s_uiShadowAtlasTextureHeight;
  xiiUInt32 uiUsedAtlasSize  = 0;

  xiiDebugRendererContext debugContext(xiiWorld::GetWorld(0));
  if (const xiiView* pView = xiiRenderWorld::GetViewByUsageHint(xiiCameraUsageHint::MainView, xiiCameraUsageHint::EditorView))
  {
    debugContext = xiiDebugRendererContext(pView->GetHandle());
  }

  if (cvar_RenderingShadowsShowPoolStats)
  {
    xiiDebugRenderer::DrawInfoText(debugContext, xiiDebugTextPlacement::TopLeft, "ShadowPoolStats", "Shadow Pool Stats:", xiiColor::LightSteelBlue);
    xiiDebugRenderer::DrawInfoText(debugContext, xiiDebugTextPlacement::TopLeft, "ShadowPoolStats", "Details (Name: Size - Atlas Offset)", xiiColor::LightSteelBlue);
  }

#endif

  for (auto& sorted : s_SortedShadowData)
  {
    xiiUInt32 uiShadowDataIndex = sorted.m_uiIndex;
    auto&     shadowData        = s_pData->m_ShadowData[uiShadowDataIndex];

    xiiUInt32 uiShadowMapSize = s_uiShadowMapSize;
    float     fadeOutStart    = s_fFadeOutScaleStart;
    float     fadeOutEnd      = s_fFadeOutScaleEnd;

    // point lights use a lot of atlas space thus we cut the shadow map size in half
    if (shadowData.m_uiType == LIGHT_TYPE_POINT)
    {
      uiShadowMapSize /= 2;
      fadeOutStart *= 2.0f;
      fadeOutEnd *= 2.0f;
    }

    uiShadowMapSize = xiiMath::PowerOfTwo_Ceil((xiiUInt32)(uiShadowMapSize * xiiMath::Clamp(shadowData.m_fShadowMapScale, fadeOutStart, 1.0f)));

    xiiHybridArray<xiiView*, 8>   shadowViews;
    xiiHybridArray<xiiRectU32, 8> atlasRects;

    // Fill atlas
    for (xiiUInt32 uiViewIndex = 0; uiViewIndex < shadowData.m_Views.GetCount(); ++uiViewIndex)
    {
      xiiView* pShadowView = nullptr;
      xiiRenderWorld::TryGetView(shadowData.m_Views[uiViewIndex], pShadowView);
      shadowViews.PushBack(pShadowView);

      XII_ASSERT_DEV(pShadowView != nullptr, "Implementation error");

      xiiRectU32 atlasRect = FindAtlasRect(uiShadowMapSize, uiShadowDataIndex);
      atlasRects.PushBack(atlasRect);

      pShadowView->SetViewport(xiiRectFloat((float)atlasRect.x, (float)atlasRect.y, (float)atlasRect.width, (float)atlasRect.height));

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
      if (cvar_RenderingShadowsShowPoolStats)
      {
        xiiDebugRenderer::DrawInfoText(debugContext, xiiDebugTextPlacement::TopLeft, "ShadowPoolStats", xiiFmt("{0}: {1} - {2}x{3}", pShadowView->GetName(), atlasRect.width, atlasRect.x, atlasRect.y), xiiColor::LightSteelBlue);

        uiUsedAtlasSize += atlasRect.width * atlasRect.height;
      }
#endif
    }

    // Fill shadow data
    if (shadowData.m_uiType == LIGHT_TYPE_DIR)
    {
      xiiUInt32 uiNumCascades = shadowData.m_Views.GetCount();

      xiiUInt32 uiMatrixIndex      = GET_WORLD_TO_LIGHT_MATRIX_INDEX(shadowData.m_uiPackedDataOffset, 0);
      xiiMat4&  worldToLightMatrix = *reinterpret_cast<xiiMat4*>(&packedShadowData[uiMatrixIndex]);

      worldToLightMatrix = shadowViews[0]->GetViewProjectionMatrix(xiiCameraEye::Left);

      for (xiiUInt32 uiViewIndex = 0; uiViewIndex < uiNumCascades; ++uiViewIndex)
      {
        if (uiViewIndex >= 1)
        {
          xiiMat4 cascadeToWorldMatrix = shadowViews[uiViewIndex]->GetInverseViewProjectionMatrix(xiiCameraEye::Left);
          xiiVec3 cascadeCorner        = cascadeToWorldMatrix.TransformPosition(xiiVec3(0.0f));
          cascadeCorner                = worldToLightMatrix.TransformPosition(cascadeCorner);

          xiiVec3 otherCorner = cascadeToWorldMatrix.TransformPosition(xiiVec3(1.0f));
          otherCorner         = worldToLightMatrix.TransformPosition(otherCorner);

          xiiUInt32 uiCascadeScaleIndex  = GET_CASCADE_SCALE_INDEX(shadowData.m_uiPackedDataOffset, uiViewIndex - 1);
          xiiUInt32 uiCascadeOffsetIndex = GET_CASCADE_OFFSET_INDEX(shadowData.m_uiPackedDataOffset, uiViewIndex - 1);

          xiiVec4& cascadeScale  = packedShadowData[uiCascadeScaleIndex];
          xiiVec4& cascadeOffset = packedShadowData[uiCascadeOffsetIndex];

          cascadeScale  = xiiVec3(1.0f).CompDiv(otherCorner - cascadeCorner).GetAsVec4(1.0f);
          cascadeOffset = cascadeCorner.GetAsVec4(0.0f).CompMul(-cascadeScale);
        }

        xiiUInt32 uiAtlasScaleOffsetIndex = GET_ATLAS_SCALE_OFFSET_INDEX(shadowData.m_uiPackedDataOffset, uiViewIndex);
        xiiVec4&  atlasScaleOffset        = packedShadowData[uiAtlasScaleOffsetIndex];

        xiiRectU32 atlasRect = atlasRects[uiViewIndex];
        if (atlasRect.HasNonZeroArea())
        {
          xiiVec2 scale  = xiiVec2(atlasRect.width * fAtlasInvWidth, atlasRect.height * fAtlasInvHeight);
          xiiVec2 offset = xiiVec2(atlasRect.x * fAtlasInvWidth, atlasRect.y * fAtlasInvHeight);

          // combine with tex scale offset
          atlasScaleOffset.x = scale.x * 0.5f;
          atlasScaleOffset.y = scale.y * -0.5f;
          atlasScaleOffset.z = offset.x + scale.x * 0.5f;
          atlasScaleOffset.w = offset.y + scale.y * 0.5f;
        }
        else
        {
          atlasScaleOffset.Set(1.0f, 1.0f, 0.0f, 0.0f);
        }
      }

      const xiiCamera* pFirstCascadeCamera = shadowViews[0]->GetCamera();
      const xiiCamera* pLastCascadeCamera  = shadowViews[uiNumCascades - 1]->GetCamera();

      float cascadeSize        = pFirstCascadeCamera->GetFovOrDim();
      float texelSize          = 1.0f / uiShadowMapSize;
      float penumbraSize       = xiiMath::Max(shadowData.m_fPenumbraSize / cascadeSize, texelSize);
      float goodPenumbraSize   = 8.0f / uiShadowMapSize;
      float relativeShadowSize = uiShadowMapSize * fAtlasInvHeight;

      // params
      {
        // tweak values to keep the default values consistent with spot and point lights
        float     slopeBias          = shadowData.m_fSlopeBias * xiiMath::Max(penumbraSize, goodPenumbraSize);
        float     constantBias       = shadowData.m_fConstantBias * 0.2f;
        xiiUInt32 uilastCascadeIndex = uiNumCascades - 1;

        xiiUInt32 uiParamsIndex = GET_SHADOW_PARAMS_INDEX(shadowData.m_uiPackedDataOffset);
        xiiVec4&  shadowParams  = packedShadowData[uiParamsIndex];
        shadowParams.x          = slopeBias;
        shadowParams.y          = constantBias;
        shadowParams.z          = penumbraSize * relativeShadowSize;
        shadowParams.w          = *reinterpret_cast<float*>(&uilastCascadeIndex);
      }

      // params2
      {
        float ditherMultiplier = 0.2f / cascadeSize;
        float zRange           = cascadeSize / pFirstCascadeCamera->GetFarPlane();

        float actualPenumbraSize    = shadowData.m_fPenumbraSize / pLastCascadeCamera->GetFovOrDim();
        float penumbraSizeIncrement = xiiMath::Max(goodPenumbraSize - actualPenumbraSize, 0.0f) / shadowData.m_fMinRange;

        xiiUInt32 uiParams2Index = GET_SHADOW_PARAMS2_INDEX(shadowData.m_uiPackedDataOffset);
        xiiVec4&  shadowParams2  = packedShadowData[uiParams2Index];
        shadowParams2.x          = 1.0f - (xiiMath::Max(penumbraSize, goodPenumbraSize) + texelSize) * 2.0f;
        shadowParams2.y          = ditherMultiplier;
        shadowParams2.z          = ditherMultiplier * zRange;
        shadowParams2.w          = penumbraSizeIncrement * relativeShadowSize;
      }

      // fadeout
      {
        float fadeOutRange = 1.0f - shadowData.m_fFadeOutStart;
        float xyScale      = -1.0f / fadeOutRange;
        float xyOffset     = -xyScale;

        float zFadeOutRange = fadeOutRange * pLastCascadeCamera->GetFovOrDim() / pLastCascadeCamera->GetFarPlane();
        float zScale        = -1.0f / zFadeOutRange;
        float zOffset       = -zScale;

        xiiUInt32 uiFadeOutIndex = GET_FADE_OUT_PARAMS_INDEX(shadowData.m_uiPackedDataOffset);
        xiiVec4&  fadeOutParams  = packedShadowData[uiFadeOutIndex];
        fadeOutParams.x          = xyScale;
        fadeOutParams.y          = xyOffset;
        fadeOutParams.z          = zScale;
        fadeOutParams.w          = zOffset;
      }
    }
    else // spot or point light
    {
      xiiMat4 texMatrix;
      texMatrix.SetIdentity();
      texMatrix.SetDiagonal(xiiVec4(0.5f, -0.5f, 1.0f, 1.0f));
      texMatrix.SetTranslationVector(xiiVec3(0.5f, 0.5f, 0.0f));

      xiiAngle fov;

      for (xiiUInt32 uiViewIndex = 0; uiViewIndex < shadowData.m_Views.GetCount(); ++uiViewIndex)
      {
        xiiView* pShadowView = shadowViews[uiViewIndex];
        XII_ASSERT_DEV(pShadowView != nullptr, "Implementation error");

        xiiUInt32 uiMatrixIndex      = GET_WORLD_TO_LIGHT_MATRIX_INDEX(shadowData.m_uiPackedDataOffset, uiViewIndex);
        xiiMat4&  worldToLightMatrix = *reinterpret_cast<xiiMat4*>(&packedShadowData[uiMatrixIndex]);

        xiiRectU32 atlasRect = atlasRects[uiViewIndex];
        if (atlasRect.HasNonZeroArea())
        {
          xiiVec2 scale  = xiiVec2(atlasRect.width * fAtlasInvWidth, atlasRect.height * fAtlasInvHeight);
          xiiVec2 offset = xiiVec2(atlasRect.x * fAtlasInvWidth, atlasRect.y * fAtlasInvHeight);

          xiiMat4 atlasMatrix;
          atlasMatrix.SetIdentity();
          atlasMatrix.SetDiagonal(xiiVec4(scale.x, scale.y, 1.0f, 1.0f));
          atlasMatrix.SetTranslationVector(offset.GetAsVec3(0.0f));

          fov                           = pShadowView->GetCamera()->GetFovY(1.0f);
          const xiiMat4& viewProjection = pShadowView->GetViewProjectionMatrix(xiiCameraEye::Left);

          worldToLightMatrix = atlasMatrix * texMatrix * viewProjection;
        }
        else
        {
          worldToLightMatrix.SetIdentity();
        }
      }

      float screenHeight       = xiiMath::Tan(fov * 0.5f) * 20.0f; // screen height in worldspace at 10m distance
      float texelSize          = 1.0f / uiShadowMapSize;
      float penumbraSize       = xiiMath::Max(shadowData.m_fPenumbraSize / screenHeight, texelSize);
      float relativeShadowSize = uiShadowMapSize * fAtlasInvHeight;

      float slopeBias    = shadowData.m_fSlopeBias * penumbraSize * xiiMath::Tan(fov * 0.5f);
      float constantBias = shadowData.m_fConstantBias * s_uiShadowMapSize / uiShadowMapSize;
      float fadeOut      = xiiMath::Clamp((shadowData.m_fShadowMapScale - fadeOutEnd) / (fadeOutStart - fadeOutEnd), 0.0f, 1.0f);

      xiiUInt32 uiParamsIndex = GET_SHADOW_PARAMS_INDEX(shadowData.m_uiPackedDataOffset);
      xiiVec4&  shadowParams  = packedShadowData[uiParamsIndex];
      shadowParams.x          = slopeBias;
      shadowParams.y          = constantBias;
      shadowParams.z          = penumbraSize * relativeShadowSize;
      shadowParams.w          = xiiMath::Sqrt(fadeOut);
    }
  }

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  if (cvar_RenderingShadowsShowPoolStats)
  {
    xiiDebugRenderer::DrawInfoText(debugContext, xiiDebugTextPlacement::TopLeft, "ShadowPoolStats", xiiFmt("Atlas Utilization: {0}%%", xiiArgF(100.0 * (double)uiUsedAtlasSize / uiTotalAtlasSize, 2)), xiiColor::LightSteelBlue);
  }
#endif

  s_pData->Clear();
}

// static
void xiiShadowPool::OnRenderEvent(const xiiRenderWorldRenderEvent& e)
{
  if (e.m_Type != xiiRenderWorldRenderEvent::Type::BeginRender)
    return;

  if (s_pData->m_hShadowAtlasTexture.IsInvalidated() || s_pData->m_hShadowDataBuffer.IsInvalidated())
    return;

  xiiGALDevice* pDevice  = xiiGALDevice::GetDefaultDevice();
  xiiGALPass*   pGALPass = pDevice->BeginPass("Shadow Atlas");

  xiiGALRenderingSetup renderingSetup;
  renderingSetup.m_RenderTargetSetup.SetDepthStencilTarget(pDevice->GetTexture(s_pData->m_hShadowAtlasTexture)->GetDefaultView(xiiGALTextureViewType::DepthStencil));
  renderingSetup.m_bClearDepth = true;

  auto pCommandEncoder = pGALPass->BeginRendering(renderingSetup);

  xiiUInt32 uiDataIndex      = xiiRenderWorld::GetDataIndexForRendering();
  auto&     packedShadowData = s_pData->m_PackedShadowData[uiDataIndex];
  if (!packedShadowData.IsEmpty())
  {
    XII_PROFILE_SCOPE("Shadow Data Buffer Update");

    pCommandEncoder->UpdateBuffer(s_pData->m_hShadowDataBuffer, 0, packedShadowData.GetByteArrayPtr());
  }

  pGALPass->EndRendering(pCommandEncoder);
  pDevice->EndPass(pGALPass);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lights_Implementation_ShadowPool);
