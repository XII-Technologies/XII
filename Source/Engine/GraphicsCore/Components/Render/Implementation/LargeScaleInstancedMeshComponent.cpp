#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Render/LargeScaleInstancedMeshComponent.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GAL/Device/GALDevice.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLargeScaleInstancedMeshRenderData, 1, xiiRTTIDefaultAllocator<xiiLargeScaleInstancedMeshRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiLargeScaleInstancedMeshComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Mesh",         GetMeshFile,     SetMeshFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Mesh_Static")),
    XII_ACCESSOR_PROPERTY("MeshletAsset", GetMeshletFile,  SetMeshletFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Meshlet")),
    XII_ACCESSOR_PROPERTY("Material",     GetMaterialFile, SetMaterialFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Material")),
    XII_ACCESSOR_PROPERTY("MaxInstances", GetMaxInstances, SetMaxInstances)->AddAttributes(new xiiDefaultValueAttribute(1000u), new xiiClampValueAttribute(1u, 10000000u)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering/Instancing"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiLargeScaleInstancedMeshComponent::xiiLargeScaleInstancedMeshComponent()  = default;
xiiLargeScaleInstancedMeshComponent::~xiiLargeScaleInstancedMeshComponent()
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();
  if (pDevice && m_hInstanceBuffer.IsValid())
    pDevice->DestroyBuffer(m_hInstanceBuffer);
}

// ---- Serialization ----

void xiiLargeScaleInstancedMeshComponent::SerializeComponent(xiiWorldWriter& s) const
{
  SUPER::SerializeComponent(s);
  xiiStreamWriter& stream = s.GetStream();
  stream << m_hMesh << m_hMeshlets << m_hMaterial;
  stream << m_uiMaxInstances;
  stream << m_bHasCustomBounds;
  if (m_bHasCustomBounds)
    stream << m_CustomBounds;
}

void xiiLargeScaleInstancedMeshComponent::DeserializeComponent(xiiWorldReader& s)
{
  SUPER::DeserializeComponent(s);
  xiiStreamReader& stream = s.GetStream();
  stream >> m_hMesh >> m_hMeshlets >> m_hMaterial;
  stream >> m_uiMaxInstances;
  stream >> m_bHasCustomBounds;
  if (m_bHasCustomBounds)
    stream >> m_CustomBounds;
}

// ---- Bounds ----

xiiResult xiiLargeScaleInstancedMeshComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  XII_IGNORE_UNUSED(ref_msg);
  ref_bAlwaysVisible = false;

  if (m_bHasCustomBounds)
  {
    ref_bounds = m_CustomBounds;
    return XII_SUCCESS;
  }

  if (!m_hMesh.IsValid())
    return XII_FAILURE;

  // For large-scale instancing the local bounds should encompass all instances.
  // Without knowing all positions, report a very large sphere so the object is
  // never frustum-culled at component level (per-instance GPU cull handles it).
  ref_bounds = xiiBoundingBoxSphere::MakeFromSphere(xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3::MakeZero(), 1e6f));
  return XII_SUCCESS;
}

// ---- Property setters ----

void xiiLargeScaleInstancedMeshComponent::SetMeshFile(xiiStringView sFile)
{
  m_hMesh = sFile.IsEmpty() ? xiiMeshResourceHandle{} : xiiResourceManager::LoadResource<xiiMeshResource>(sFile);
  InvalidateCachedRenderData();
}
xiiStringView xiiLargeScaleInstancedMeshComponent::GetMeshFile() const
{
  return m_hMesh.IsValid() ? xiiResourceManager::GetResourceIDOrDescription(m_hMesh) : xiiStringView{};
}

void xiiLargeScaleInstancedMeshComponent::SetMeshletFile(xiiStringView sFile)
{
  m_hMeshlets = sFile.IsEmpty() ? xiiMeshletResourceHandle{} : xiiResourceManager::LoadResource<xiiMeshletResource>(sFile);
  InvalidateCachedRenderData();
}
xiiStringView xiiLargeScaleInstancedMeshComponent::GetMeshletFile() const
{
  return m_hMeshlets.IsValid() ? xiiResourceManager::GetResourceIDOrDescription(m_hMeshlets) : xiiStringView{};
}

void xiiLargeScaleInstancedMeshComponent::SetMaterialFile(xiiStringView sFile)
{
  m_hMaterial = sFile.IsEmpty() ? xiiMaterialResourceHandle{} : xiiResourceManager::LoadResource<xiiMaterialResource>(sFile);
  InvalidateCachedRenderData();
}
xiiStringView xiiLargeScaleInstancedMeshComponent::GetMaterialFile() const
{
  return m_hMaterial.IsValid() ? xiiResourceManager::GetResourceIDOrDescription(m_hMaterial) : xiiStringView{};
}

void xiiLargeScaleInstancedMeshComponent::SetMaxInstances(xiiUInt32 uiMax)
{
  if (uiMax == m_uiMaxInstances)
    return;
  m_uiMaxInstances = uiMax;
  EnsureBufferCapacity(uiMax);
  InvalidateCachedRenderData();
}

void xiiLargeScaleInstancedMeshComponent::SetCustomBounds(const xiiBoundingBoxSphere& bounds)
{
  m_CustomBounds     = bounds;
  m_bHasCustomBounds = true;
  TriggerLocalBoundsUpdate();
}

// ---- Instance write API ----

void xiiLargeScaleInstancedMeshComponent::EnsureBufferCapacity(xiiUInt32 uiRequired)
{
  if (m_bExternalBuffer)
    return;

  const xiiUInt32 uiBytes = uiRequired * sizeof(xiiGPUInstanceData);

  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();
  if (!pDevice)
    return;

  if (m_hInstanceBuffer.IsValid())
    pDevice->DestroyBuffer(m_hInstanceBuffer);

  xiiGALBufferCreationDescription bd;
  bd.m_sDebugName         = "LargeScaleInstanceBuffer";
  bd.m_uiSize             = xiiMath::Max(uiBytes, 64u);
  bd.m_BindFlags          = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
  bd.m_ResourceUsage      = xiiGALResourceUsage::Dynamic;
  bd.m_CPUAccessFlags     = xiiGALCPUAccessFlags::Write;
  bd.m_Mode               = xiiGALBufferMode::Structured;
  bd.m_uiElementByteStride= sizeof(xiiGPUInstanceData);
  m_hInstanceBuffer = pDevice->CreateBuffer(bd);
}

xiiGPUInstanceData* xiiLargeScaleInstancedMeshComponent::BeginWriteInstances(xiiUInt32 uiCount)
{
  XII_ASSERT_DEV(!m_bExternalBuffer, "Cannot CPU-write an externally managed buffer");
  EnsureBufferCapacity(uiCount);
  m_uiCurrentInstanceCount = uiCount;

  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();
  if (!pDevice || !m_hInstanceBuffer.IsValid())
    return nullptr;

  return static_cast<xiiGPUInstanceData*>(pDevice->MapBuffer(m_hInstanceBuffer, xiiGALMapType::Write, xiiGALMapFlags::Discard));
}

void xiiLargeScaleInstancedMeshComponent::EndWriteInstances()
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();
  if (pDevice && m_hInstanceBuffer.IsValid())
    pDevice->UnmapBuffer(m_hInstanceBuffer, xiiGALMapType::Write);

  InvalidateCachedRenderData();
}

void xiiLargeScaleInstancedMeshComponent::SetExternalInstanceBuffer(xiiGALBufferHandle hBuffer, xiiUInt32 uiCount)
{
  m_hExternalBuffer        = hBuffer;
  m_uiCurrentInstanceCount = uiCount;
  m_bExternalBuffer        = hBuffer.IsValid();
  InvalidateCachedRenderData();
}

// ---- OnMsgExtractRenderData ----

void xiiLargeScaleInstancedMeshComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  if (m_uiCurrentInstanceCount == 0)
    return;
  if (!ref_msg.m_pView || !ref_msg.m_pExtractedRenderData)
    return;

  auto* pWM = GetWorld()->GetModule<xiiRenderWorldModule>();
  if (!pWM)
    return;

  auto* pRD = pWM->CreateRenderDataForThisFrame<xiiLargeScaleInstancedMeshRenderData>(this);

  pRD->m_GlobalTransform  = GetOwner()->GetGlobalTransform();
  pRD->m_GlobalBounds     = GetOwner()->GetGlobalBounds();
  pRD->m_hOwnerObject     = GetOwner()->GetHandle();
  pRD->m_hOwnerComponent  = GetHandle();
  pRD->m_uiSortingKey     = GetUniqueIdForRendering();

  pRD->m_hMesh            = m_hMesh;
  pRD->m_hMeshlets        = m_hMeshlets;
  pRD->m_hMaterial        = m_hMaterial;
  pRD->m_bUseMeshlets     = m_hMeshlets.IsValid();
  pRD->m_uiInstanceCount  = m_uiCurrentInstanceCount;
  pRD->m_hInstanceBuffer  = m_bExternalBuffer ? m_hExternalBuffer : m_hInstanceBuffer;

  ref_msg.AddRenderData(pRD, xiiRenderData::Caching::Never);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Render_Implementation_LargeScaleInstancedMeshComponent);
