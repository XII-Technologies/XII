/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Material/MaterialSystem.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiMaterialRenderData, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiMaterialRenderData>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Handle", m_Handle),
    XII_MEMBER_PROPERTY("GpuOffset", m_uiGpuOffset),
    XII_MEMBER_PROPERTY("RuntimeState", m_RuntimeState),
    XII_ARRAY_MEMBER_PROPERTY("ResourceBindings", m_ResourceBindings),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiMaterialSystem::Initialize(xiiGALDevice* pDevice, const xiiMaterialGpuStorageDescription& description)
{
  Shutdown();
  XII_SUCCEED_OR_RETURN(m_GpuStorage.Initialize(pDevice, description));
  m_bInitialized = true;
  return XII_SUCCESS;
}

void xiiMaterialSystem::Shutdown()
{
  m_GpuStorage.Shutdown();
  m_uiFrameIndex = 0ULL;
  m_bInitialized = false;
}

void xiiMaterialSystem::BeginFrame(xiiUInt64 uiFrameIndex, xiiUInt64 uiCompletedFrame)
{
  XII_ASSERT_DEV(m_bInitialized, "Material system must be initialized before BeginFrame().");
  XII_ASSERT_DEV(uiCompletedFrame <= uiFrameIndex, "Completed material frame cannot be newer than the current frame.");
  m_uiFrameIndex = uiFrameIndex;
  m_GpuStorage.CollectGarbage(uiCompletedFrame);
}

xiiMaterialGpuHandle xiiMaterialSystem::RegisterMaterial(xiiSharedPtr<xiiMaterialInstance> pInstance)
{
  XII_ASSERT_DEV(m_bInitialized, "Material system must be initialized before registering instances.");
  return m_GpuStorage.RegisterMaterial(std::move(pInstance), m_uiFrameIndex);
}

void xiiMaterialSystem::UnregisterMaterial(xiiMaterialGpuHandle handle)
{
  m_GpuStorage.UnregisterMaterial(handle, m_uiFrameIndex);
}

xiiRenderGraphBufferHandle xiiMaterialSystem::AddUploadPass(xiiRenderGraph& graph)
{
  XII_ASSERT_DEV(m_bInitialized, "Material system must be initialized before adding graph passes.");
  return m_GpuStorage.AddUploadPass(graph, m_uiFrameIndex);
}

xiiResult xiiMaterialSystem::ExtractRenderData(xiiMaterialGpuHandle handle, xiiMaterialRenderData& out_renderData) const
{
  xiiSharedPtr<xiiMaterialInstance> pInstance = m_GpuStorage.GetMaterial(handle);
  if (pInstance == nullptr)
    return XII_FAILURE;

  xiiMaterialInstanceSnapshot snapshot;
  pInstance->CreateSnapshot(snapshot);
  out_renderData.m_Handle           = handle;
  out_renderData.m_uiGpuOffset      = m_GpuStorage.GetGpuOffset(handle, m_uiFrameIndex);
  out_renderData.m_RuntimeState     = snapshot.m_RuntimeState;
  out_renderData.m_ResourceBindings = std::move(snapshot.m_ResourceBindings);
  return out_renderData.m_uiGpuOffset != xiiInvalidIndex ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiMaterialSystem::CreateRuntimeMaterial(const xiiMaterialSchemaDescription& description, const xiiMaterialRuntimeState& runtimeState, xiiSharedPtr<xiiMaterialSchema>& out_pSchema, xiiSharedPtr<xiiMaterialInstance>& out_pInstance, xiiStringBuilder* out_pError)
{
  out_pSchema.Clear();
  out_pInstance.Clear();

  xiiSharedPtr<xiiMaterialSchema> pSchema = XII_DEFAULT_NEW(xiiMaterialSchema);
  XII_SUCCEED_OR_RETURN(pSchema->Build(description, out_pError));

  xiiMaterialRuntimeState resolvedState = runtimeState;
  resolvedState.m_Domain       = pSchema->GetDomain();
  resolvedState.m_ShadingModel = pSchema->GetShadingModel();
  resolvedState.m_uiLayoutHash = pSchema->GetLayoutHash();

  xiiSharedPtr<xiiMaterialInstance> pInstance = XII_DEFAULT_NEW(xiiMaterialInstance);
  XII_SUCCEED_OR_RETURN(pInstance->Initialize(pSchema, resolvedState));

  out_pSchema   = std::move(pSchema);
  out_pInstance = std::move(pInstance);
  return XII_SUCCESS;
}

