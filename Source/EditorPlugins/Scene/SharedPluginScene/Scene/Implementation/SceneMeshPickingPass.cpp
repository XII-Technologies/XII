/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <SharedPluginScene/SharedPluginScenePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Visibility/GpuSceneRaster.h>
#include <SharedPluginScene/Scene/Implementation/SceneMeshPickingState.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSceneMeshPickingPass, 2, xiiRTTIDefaultAllocator<xiiSceneMeshPickingPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("GPUGeometryMetadata", m_InputGPUGeometryMetadata)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("GPU Geometry Metadata")),
    XII_MEMBER_PROPERTY("GPUMeshletMetadata", m_InputGPUMeshletMetadata)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("GPU Meshlet Metadata")),
    XII_MEMBER_PROPERTY("SurfaceMaterialGPUStorage", m_InputSurfaceMaterialGPUStorage)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("Surface Material GPU Storage")),
    XII_MEMBER_PROPERTY("Surface", m_OutputSurface)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("SelectionOnly", m_bSelectionOnly)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("UseMeshShaders", m_bUseMeshShaders)->AddAttributes(new xiiDefaultValueAttribute(true)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiSceneMeshPickingPass::xiiSceneMeshPickingPass()  = default;
xiiSceneMeshPickingPass::~xiiSceneMeshPickingPass() = default;



void xiiSceneMeshPickingPass::InvalidateTemporalHistory()
{
  if (m_pState)
    m_pState->Invalidate();
}

xiiResult xiiSceneMeshPickingPass::Build(xiiRenderPipelinePassContext& context)
{
  bool valid = false;
  XII_SCOPE_EXIT(if (!valid) InvalidateTemporalHistory());
  const auto* view = context.m_pView;
  if (!view)
    return XII_FAILURE;
  bool enabled = true, selected = true, transparent = true;
  auto parameter = [&](xiiStringView name, bool& value) -> xiiResult {
    if (const auto* setting = context.m_Parameters.GetValue(name))
    {
      xiiResult result = XII_FAILURE;
      value            = setting->ConvertTo<bool>(&result);
      XII_SUCCEED_OR_RETURN(result);
    }
    return XII_SUCCESS;
  };
  XII_SUCCEED_OR_RETURN(parameter(m_bSelectionOnly ? "Editor.SelectionOverlay" : "Editor.Picking", enabled));
  XII_SUCCEED_OR_RETURN(parameter("Editor.PickSelected", selected));
  XII_SUCCEED_OR_RETURN(parameter("Editor.PickTransparent", transparent));
  if (!enabled || view->GetCameraUsageHint() != xiiCameraUsageHint::EditorView)
    return XII_SUCCESS;
  if (!view->IsValid() || !view->GetExtractedRenderData() || !xiiGpuSceneRasterManager::IsPickingSupported())
    return XII_FAILURE;

  if (!m_pState)
    m_pState = XII_DEFAULT_NEW(xiiSceneMeshPickingState);
  auto& state = *m_pState;
  XII_SUCCEED_OR_RETURN(state.Poll());
  state.m_Selected.Clear();
  if (m_bSelectionOnly || !selected)
  {
    if (const auto* objects = context.m_Parameters.GetValue("Editor.SelectedObjects"))
    {
      if (!objects->IsA<xiiVariantArray>())
        return XII_FAILURE;
      for (const auto& object : objects->Get<xiiVariantArray>())
      {
        if (!object.IsA<xiiUInt64>())
          return XII_FAILURE;
        xiiGameObjectId id;
        id.m_Data = object.Get<xiiUInt64>();
        state.m_Selected.Insert(xiiGameObjectHandle(id));
      }
    }
  }
  state.m_Packets.Clear();
  xiiUInt64 fingerprint = transparent ? 1 : 0;
  for (auto* packet : view->GetExtractedRenderData()->GetAllRenderData())
  {
    const auto* mesh = xiiDynamicCast<const xiiMeshRenderData*>(packet);
    if (!mesh || (m_bSelectionOnly ? !state.m_Selected.Contains(mesh->m_hOwnerObject) : state.m_Selected.Contains(mesh->m_hOwnerObject)))
      continue;
    const xiiGameObject* object    = nullptr;
    const xiiComponent*  component = nullptr;
    if (!view->GetWorld()->TryGetObject(mesh->m_hOwnerObject, object) || !view->GetWorld()->TryGetComponent(mesh->m_hOwnerComponent, component) || component->GetOwner() != object || !component->IsActive() || !object->IsActive())
      continue;
    const auto& tags = object->GetTags();
    if (tags.IsAnySet(view->m_ExcludeTags) || (!view->m_IncludeTags.IsEmpty() && !tags.IsAnySet(view->m_IncludeTags)))
      continue;
    state.m_Packets.PushBack(packet);
    // Hash explicit initialized fields, never object padding or pointers. A topology, material,
    // transform or packet change invalidates both background and owner pixels before reuse.
    const xiiUInt64 fields[] = {mesh->m_hOwnerObject.GetInternalID().m_Data, mesh->m_hOwnerComponent.GetInternalID().m_Data,
                                mesh->m_uiUniqueID, mesh->m_uiSectionIndex, mesh->m_hMesh.GetResourceIDHash(), mesh->m_uiLODIndex,
                                mesh->m_uiFirstPrimitive, mesh->m_uiPrimitiveCount, mesh->m_Flags.GetValue(), static_cast<xiiUInt64>(mesh->m_RangeMode)};
    fingerprint              = xiiHashingUtils::xxHash64(fields, sizeof(fields), fingerprint);
    fingerprint              = xiiHashingUtils::xxHash64(&mesh->m_fLodBias, sizeof(mesh->m_fLodBias), fingerprint);
    const auto transform     = mesh->m_GlobalTransform.GetAsMat4();
    fingerprint              = xiiHashingUtils::xxHash64(transform.m_fElementsCM, sizeof(transform.m_fElementsCM), fingerprint);
    for (const auto& material : mesh->m_hMaterials)
    {
      const auto id = material.GetResourceIDHash();
      fingerprint   = xiiHashingUtils::xxHash64(&id, sizeof(id), fingerprint);
    }
  }
  if (fingerprint != state.m_uiFingerprint)
  {
    state.Invalidate();
    state.m_uiFingerprint = fingerprint;
  }
  const auto* instancesData = view->GetGpuInstanceData();
  if (!instancesData)
    return XII_FAILURE;
  instancesData->SelectInstances(state.m_Packets, state.m_Candidates);
  if (m_bSelectionOnly && state.m_Packets.IsEmpty())
    return XII_SUCCESS;

  xiiGeometryResidencyManager::UploadHandles geometry;
  geometry.m_hGeometryMetadata = context.GetInputBuffer(m_InputGPUGeometryMetadata);
  geometry.m_hMeshletMetadata  = context.GetInputBuffer(m_InputGPUMeshletMetadata);
  xiiMaterialGpuStorage::UploadHandles materials;
  materials.m_hSurfaceData = context.GetInputBuffer(m_InputSurfaceMaterialGPUStorage);
  if (!geometry.m_hGeometryMetadata.IsValid() || !geometry.m_hMeshletMetadata.IsValid() || !materials.m_hSurfaceData.IsValid() || !context.m_Blackboard.TryGet(xiiRGBlackboardKeys::k_GpuGeometryBaseIndex, geometry.m_uiGeometryBaseIndex) || !context.m_Blackboard.TryGet(xiiRGBlackboardKeys::k_GpuMaximumResidentMeshlets, geometry.m_uiMaximumResidentMeshletCount) || !context.m_Blackboard.TryGet(xiiRGBlackboardKeys::k_GpuSurfaceMaterialBaseIndex, materials.m_uiSurfaceBaseIndex))
    return XII_FAILURE;

  const auto      instances = instancesData->GetInstanceCount();
  const xiiUInt64 meshlets  = instancesData->GetMaximumMeshletCount();
  XII_SUCCEED_OR_RETURN(state.ResizeVisibility(instances, meshlets));
  const auto& viewport = view->GetViewport();
  // Full viewport resolution, independent of dynamic scene resolution. The query target starts
  // at zero; the public queries account for a nonzero window viewport origin.
  if (!xiiMath::IsFinite(viewport.width) || !xiiMath::IsFinite(viewport.height) || viewport.width <= 0 || viewport.height <= 0 || viewport.width >= static_cast<float>(xiiMath::MaxValue<xiiUInt32>()) || viewport.height >= static_cast<float>(xiiMath::MaxValue<xiiUInt32>()))
    return XII_FAILURE;
  const xiiUInt32 width          = static_cast<xiiUInt32>(xiiMath::Ceil(viewport.width));
  const xiiUInt32 height         = static_cast<xiiUInt32>(xiiMath::Ceil(viewport.height));
  const auto&     matrix         = view->GetViewProjectionMatrix(xiiCameraEye::Left);
  const auto      frustum        = xiiFrustum::MakeFromMVP(matrix, xiiClipSpaceDepthRange::ZeroToOne, xiiHandedness::LeftHanded);
  auto            visibilityView = xiiGpuVisibilitySystem::BuildView(matrix, frustum, view->GetCamera()->GetCenterPosition(), width, height, 0, instances);
  if (!transparent && !m_bSelectionOnly)
    visibilityView.m_uiExcludedFlags |= xiiGpuInstanceFlags::Transparent;
  xiiGpuVisibilityPassDescription visibilityDescription;
  visibilityDescription.m_sName              = context.m_sPassName;
  visibilityDescription.m_Purpose            = xiiGpuVisibilityPurpose::Editor;
  visibilityDescription.m_bEnableLodFeedback = false;
  visibilityDescription.m_bUseCandidates     = true;
  visibilityDescription.m_Candidates         = state.m_Candidates;
  const auto visibility                      = xiiGpuVisibilityManager::AddPasses(state.m_hVisibility, context.m_Graph, context.m_Graph.GetFrameIndex(), *instancesData, visibilityView, geometry, visibilityDescription);

  xiiGpuSceneGBufferRasterDescription raster;
  raster.m_ViewProjectionMatrix      = matrix;
  raster.m_uiWidth                   = width;
  raster.m_uiHeight                  = height;
  raster.m_uiVertexStride            = sizeof(xiiMeshPackedVertex);
  raster.m_uiNormalOffset            = offsetof(xiiMeshPackedVertex, m_vNormal);
  raster.m_uiTangentOffset           = offsetof(xiiMeshPackedVertex, m_vTangent);
  raster.m_uiTexCoordOffset          = offsetof(xiiMeshPackedVertex, m_vTexCoord0);
  raster.m_uiMeshDispatchGroupCountX = xiiGpuVisibilityManager::GetMeshDispatchGroupCountX(state.m_hVisibility);
  raster.m_uiMeshDispatchGroupCountY = xiiGpuVisibilityManager::GetMeshDispatchGroupCountY(state.m_hVisibility);
  xiiRenderGraphTextureHandle surface;
  XII_SUCCEED_OR_RETURN(xiiGpuSceneRasterManager::AddPickingPass(context.m_Graph, context.m_sPassName, visibility, geometry, materials, raster, surface, m_bUseMeshShaders));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputSurface, surface));
  if (!m_bSelectionOnly)
    XII_SUCCEED_OR_RETURN(state.Capture(context, surface));
  valid = true;
  return XII_SUCCESS;
}

xiiResult xiiSceneMeshPickingPass::Pick(const xiiView& view, xiiUInt32 x, xiiUInt32 y, PickingResult& out_hit) const
{
  out_hit = {};
  return m_pState ? m_pState->Pick(view, x, y, out_hit) : XII_FAILURE;
}

xiiResult xiiSceneMeshPickingPass::PickObjectsInRect(const xiiView& view, const xiiRectU32& rectangle, xiiDynamicArray<xiiGameObjectHandle>& out_objects) const
{
  out_objects.Clear();
  return m_pState ? m_pState->Marquee(view, rectangle, out_objects) : XII_FAILURE;
}
