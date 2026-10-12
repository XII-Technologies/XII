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

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSceneMeshPickingPass, 1, xiiRTTIDefaultAllocator<xiiSceneMeshPickingPass>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiSceneMeshPickingPass::xiiSceneMeshPickingPass()  = default;
xiiSceneMeshPickingPass::~xiiSceneMeshPickingPass() = default;

void xiiSceneMeshPickingPass::GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const
{
  pins.PushBack({"Before", xiiRenderPipelinePin::Order, true, true});
  pins.PushBack({"GPU Geometry Metadata", xiiRenderPipelinePin::Buffer, true, false});
  pins.PushBack({"GPU Meshlet Metadata", xiiRenderPipelinePin::Buffer, true, false});
  pins.PushBack({"Surface Material GPU Storage", xiiRenderPipelinePin::Buffer, true, false});
  pins.PushBack({"Surface", xiiRenderPipelinePin::Texture, false, true, xiiGALResourceFormat::RGBA32UInt});
  pins.PushBack({"After", xiiRenderPipelinePin::Order, false, true});
}

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
  XII_SUCCEED_OR_RETURN(parameter("Editor.Picking", enabled));
  XII_SUCCEED_OR_RETURN(parameter("Editor.PickSelected", selected));
  XII_SUCCEED_OR_RETURN(parameter("Editor.PickTransparent", transparent));
  if (!enabled || view->GetCameraUsageHint() != xiiCameraUsageHint::EditorView)
    return XII_SUCCESS;
  if (!view->IsValid() || !view->GetExtractedRenderData() || !xiiGpuSceneRasterManager::IsSupported())
    return XII_FAILURE;

  if (!m_pState)
    m_pState = XII_DEFAULT_NEW(xiiSceneMeshPickingState);
  auto& state = *m_pState;
  XII_SUCCEED_OR_RETURN(state.Poll());
  state.m_Selected.Clear();
  if (!selected)
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
    if (!mesh || state.m_Selected.Contains(mesh->m_hOwnerObject))
      continue;
    const xiiGameObject* object    = nullptr;
    const xiiComponent*  component = nullptr;
    if (!view->GetWorld()->TryGetObject(mesh->m_hOwnerObject, object) || !view->GetWorld()->TryGetComponent(mesh->m_hOwnerComponent, component) ||
        component->GetOwner() != object || !component->IsActive() || !object->IsActive())
      continue;
    const auto& tags = object->GetTags();
    if (tags.IsAnySet(view->m_ExcludeTags) || (!view->m_IncludeTags.IsEmpty() && !tags.IsAnySet(view->m_IncludeTags)))
      continue;
    state.m_Packets.PushBack(packet);
    // Hash explicit initialized fields, never object padding or pointers. A topology, material,
    // transform or packet change invalidates both background and owner pixels before reuse.
    const xiiUInt64 fields[] = {mesh->m_hOwnerObject.GetInternalID().m_Data, mesh->m_hOwnerComponent.GetInternalID().m_Data,
                                mesh->m_uiUniqueID, mesh->m_uiSectionIndex, mesh->m_hMesh.GetResourceIDHash(), mesh->m_uiLODIndex,
                                mesh->m_uiFirstPrimitive, mesh->m_uiPrimitiveCount, mesh->m_Flags.GetValue()};
    fingerprint              = xiiHashingUtils::xxHash64(fields, sizeof(fields), fingerprint);
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
  if (!state.m_Scene.IsInitialized())
    XII_SUCCEED_OR_RETURN(state.m_Scene.Initialize());
  XII_SUCCEED_OR_RETURN(state.m_Scene.SynchronizeExtractedMeshes(state.m_Packets, context.m_Graph.GetFrameIndex()));

  xiiGeometryResidencyManager::UploadHandles geometry;
  geometry.m_hGeometryMetadata = context.GetInputBuffer("GPU Geometry Metadata");
  geometry.m_hMeshletMetadata  = context.GetInputBuffer("GPU Meshlet Metadata");
  xiiMaterialGpuStorage::UploadHandles materials;
  materials.m_hSurfaceData = context.GetInputBuffer("Surface Material GPU Storage");
  if (!geometry.m_hGeometryMetadata.IsValid() || !geometry.m_hMeshletMetadata.IsValid() || !materials.m_hSurfaceData.IsValid() ||
      !context.m_Blackboard.TryGet(xiiRGBlackboardKeys::k_GpuGeometryBaseIndex, geometry.m_uiGeometryBaseIndex) ||
      !context.m_Blackboard.TryGet(xiiRGBlackboardKeys::k_GpuMaximumResidentMeshlets, geometry.m_uiMaximumResidentMeshletCount) ||
      !context.m_Blackboard.TryGet(xiiRGBlackboardKeys::k_GpuSurfaceMaterialBaseIndex, materials.m_uiSurfaceBaseIndex))
    return XII_FAILURE;

  const auto instances = state.m_Scene.GetDatabase().GetObjectCount();
  xiiUInt64  meshlets  = 0;
  XII_SUCCEED_OR_RETURN(xiiSceneDatabaseManager::GetExtractedMeshletCapacity(state.m_Scene.GetHandle(), meshlets));
  XII_SUCCEED_OR_RETURN(state.ResizeVisibility(instances, meshlets));
  const auto& viewport = view->GetViewport();
  // Full viewport resolution, independent of dynamic scene resolution. The query target starts
  // at zero; the public queries account for a nonzero window viewport origin.
  if (!xiiMath::IsFinite(viewport.width) || !xiiMath::IsFinite(viewport.height) || viewport.width <= 0 || viewport.height <= 0 ||
      viewport.width >= static_cast<float>(xiiMath::MaxValue<xiiUInt32>()) || viewport.height >= static_cast<float>(xiiMath::MaxValue<xiiUInt32>()))
    return XII_FAILURE;
  const xiiUInt32 width          = static_cast<xiiUInt32>(xiiMath::Ceil(viewport.width));
  const xiiUInt32 height         = static_cast<xiiUInt32>(xiiMath::Ceil(viewport.height));
  const auto&     matrix         = view->GetViewProjectionMatrix(xiiCameraEye::Left);
  const auto      frustum        = xiiFrustum::MakeFromMVP(matrix, xiiClipSpaceDepthRange::ZeroToOne, xiiHandedness::LeftHanded);
  auto            visibilityView = xiiGpuVisibilitySystem::BuildView(matrix, frustum, view->GetCamera()->GetCenterPosition(), width, height, 0, instances);
  if (!transparent)
    visibilityView.m_uiExcludedFlags |= xiiSceneObjectFlags::Transparent;
  xiiGpuVisibilityPassDescription visibilityDescription;
  visibilityDescription.m_sName              = context.m_sPassName;
  visibilityDescription.m_Purpose            = xiiGpuVisibilityPurpose::Editor;
  visibilityDescription.m_bEnableLodFeedback = false;
  const auto visibility                      = xiiGpuVisibilityManager::AddPasses(state.m_hVisibility, context.m_Graph, context.m_Graph.GetFrameIndex(),
                                                                                  state.m_Scene.GetHandle(), visibilityView, geometry, visibilityDescription);

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
  XII_SUCCEED_OR_RETURN(xiiGpuSceneRasterManager::AddPickingPass(context.m_Graph, context.m_sPassName, visibility, geometry, materials, raster, surface));
  XII_SUCCEED_OR_RETURN(context.SetOutput("Surface", surface));
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
