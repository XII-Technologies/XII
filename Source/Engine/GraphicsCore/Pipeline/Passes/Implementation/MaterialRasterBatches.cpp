/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/World/World.h>
#include <Foundation/Time/Clock.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Meshes/MeshBufferResource.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/MaterialRasterBatches.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Materials/NativeMaterialViewConstants.h>

static_assert(offsetof(xiiMeshPackedVertex, m_vTexCoord1) == 48 && offsetof(xiiMeshPackedVertex, m_Color0) == 56,
              "Authored surface attributes must match GpuSceneSurfaceData.h.");

xiiResult xiiMaterialRasterBatches::Build(xiiRenderPipelinePassContext& context, xiiRenderGraphBufferHandle visible, xiiRenderGraphBufferHandle count,
                                          xiiDynamicArray<xiiMaterialRasterBatch>& batches, xiiRenderGraphBufferHandle& sortedVisible, xiiRenderGraphBufferHandle& draws, bool bDepth, bool bTransparent,
                                          const xiiHashTable<xiiHashedString, xiiHashedString>* pViewPermutations, bool bPicking)
{
  batches.Clear();
  const auto* scene = context.m_pView ? context.m_pView->GetGpuInstanceData() : nullptr;
  if (!scene)
    return XII_SUCCESS;
  xiiDynamicArray<xiiUInt32> bins;
  bins.SetCount(scene->GetInstanceCount(), 0u);
  xiiMap<xiiUInt32, xiiUInt32> materialBins;
  batches.ExpandAndGetRef(); // Bin zero uses the standard native surface shader.
  for (const auto index : context.m_pView->GetGpuInstanceSelection())
  {
    const auto handle = scene->GetMaterialHandle(index);
    if (handle.IsInvalidated())
      continue;
    const auto key   = handle.GetInternalID().m_InstanceIndex;
    auto       found = materialBins.Find(key);
    if (found.IsValid())
    {
      bins[index] = found.Value();
      continue;
    }
    const auto instance = xiiMaterialManager::GetGpuStorage().GetMaterial(handle);
    if (!instance)
      continue;
    xiiMaterialInstanceSnapshot snapshot;
    instance->CreateSnapshot(snapshot);
    if (!snapshot.m_pSchema || !snapshot.m_pSchema->GetShader().IsValid() || (!bPicking && snapshot.m_RuntimeState.IsTranslucent() != bTransparent))
      continue;
    auto       variables  = snapshot.m_pSchema->GetPermutations();
    const auto setDefault = [&](xiiStringView key, xiiStringView selected) {
      xiiHashedString name, value;
      name.Assign(key);
      value.Assign(selected);
      if (!variables.Contains(name))
        variables.Insert(name, value);
    };
    setDefault("BLEND_MODE", snapshot.m_RuntimeState.IsTranslucent() ? snapshot.m_RuntimeState.m_BlendMode == xiiMaterialBlendMode::Additive ? "BLEND_MODE_ADDITIVE" : snapshot.m_RuntimeState.m_BlendMode == xiiMaterialBlendMode::Modulate ? "BLEND_MODE_MODULATE" :
                                                                                                                                                                                                                                               "BLEND_MODE_TRANSPARENT" :
                 snapshot.m_RuntimeState.IsMasked()                  ? "BLEND_MODE_MASKED" :
                                                                       "BLEND_MODE_OPAQUE");
    setDefault("SHADING_MODE", snapshot.m_RuntimeState.m_ShadingModel == xiiMaterialShadingModel::Unlit ? "SHADING_MODE_FULLBRIGHT" : "SHADING_MODE_LIT");
    setDefault("TWO_SIDED", snapshot.m_RuntimeState.IsTwoSided() ? "TRUE" : "FALSE");
    for (auto name : {"FORWARD_DIRECTIONAL_SHADOWS", "FORWARD_ATMOSPHERE", "FORWARD_REFRACTION", "FORWARD_SPOT_SHADOWS"})
      setDefault(name, "FALSE");
    if (pViewPermutations)
      for (const auto& variable : *pViewPermutations)
        variables.Insert(variable.Key(), variable.Value());
    xiiHashedString passName, passValue;
    passName.Assign("RENDER_PASS");
    passValue.Assign(bPicking ? "RENDER_PASS_PICKING" : bDepth ? "RENDER_PASS_DEPTH_ONLY" :
                                                                 "RENDER_PASS_FORWARD");
    variables.Insert(passName, passValue);
    const auto permutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(snapshot.m_pSchema->GetShader(), variables, false);
    if (!permutation.IsValid())
      return context.ReportFailure("The authored material shader permutation is unavailable.");
    xiiResourceLock<xiiShaderPermutationResource> shader(permutation, xiiResourceAcquireMode::BlockTillLoaded);
    if (!shader.IsValid() || !shader->IsShaderValid())
      return context.ReportFailure("The authored material shader did not compile.");
    const auto bin    = batches.GetCount();
    auto&      cached = m_Bindings[key];
    if (cached.m_Instance != instance || cached.m_uiRevision != snapshot.m_uiRevision ||
        cached.m_Binding.m_Vertex != shader->GetGALShader(xiiGALShaderType::Vertex) || cached.m_Binding.m_Pixel != shader->GetGALShader(xiiGALShaderType::Pixel) || !cached.m_Binding.AreTexturesCurrent())
    {
      xiiMaterialShaderBinding replacement;
      const auto               status = replacement.Build(snapshot, *shader.GetPointer(), bTransparent, bPicking);
      if (status.Failed())
        return context.ReportFailure(xiiStringBuilder(snapshot.m_pSchema->GetName(), ": ", status.GetMessageString()));
      if (bDepth || bPicking)
      {
        auto depth                = replacement.m_Depth->GetDescription();
        depth.m_bDepthWriteEnable = true;
        replacement.m_Depth       = cached.m_Binding.m_Depth && cached.m_Binding.m_Depth->GetDescription() == depth ? cached.m_Binding.m_Depth : xiiGALDevice::GetDefaultDevice()->CreateDepthStencilState(depth);
        if (!replacement.m_Depth)
          return context.ReportFailure("Could not create authored surface depth state.");
      }
      cached.m_Binding    = std::move(replacement);
      cached.m_Instance   = instance;
      cached.m_uiRevision = snapshot.m_uiRevision;
    }
    batches.ExpandAndGetRef().m_Binding = cached.m_Binding;
    materialBins.Insert(key, bin);
    bins[index] = bin;
  }
  for (auto it = m_Bindings.GetIterator(); it.IsValid();)
    if (!materialBins.Contains(it.Key()))
      it = m_Bindings.Remove(it);
    else
      ++it;
  if (batches.GetCount() == 1)
  {
    batches.Clear();
    return XII_SUCCESS;
  }
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_Count, "Shaders/Geometry/MaterialBinCount.xiiShader"));
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_Prefix, "Shaders/Geometry/MaterialBinPrefix.xiiShader"));
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_Scatter, "Shaders/Geometry/MaterialBinScatter.xiiShader"));
  const auto capacity = context.m_Graph.GetBufferDescription(visible)->m_uiSize / sizeof(xiiVec2U32);
  const auto groups   = (capacity + 63u) / 64u;
  if (groups > xiiGALDevice::GetDefaultDevice()->GetGraphicsDeviceAdapterProperties().m_ComputeShaderProperties.m_uiMaxThreadGroupCountX)
    return context.ReportFailure("Authored material binning exceeds the device dispatch capacity.");
  struct Upload
  {
    xiiRenderGraphBufferHandle              m_Bins, m_Counts, m_View;
    xiiNativeMaterialViewConstants          m_ViewSnapshot;
    xiiDynamicArray<xiiUInt32>              m_Data;
    xiiDynamicArray<xiiMaterialRasterBatch> m_Batches;
  };
  const auto upload = context.m_Graph.AddPass<Upload>("Material Bin Inputs", xiiGALCommandQueueFlags::Transfer, [&](Upload& data, xiiRenderGraphBuilder& builder) {
      xiiGALBufferCreationDescription description;
      description.m_Mode = xiiGALBufferMode::Structured; description.m_uiElementByteStride = 4;
      description.m_uiSize = xiiMath::Max(1u, bins.GetCount()) * 4u;
      description.m_BindFlags = xiiGALBindFlags::ShaderResource;
      data.m_Bins = builder.WriteBuffer("Instance Material Bins", description, xiiGALResourceStateFlags::CopyDestination);
      description.m_uiSize = batches.GetCount() * 4u;
      description.m_BindFlags |= xiiGALBindFlags::UnorderedAccess;
      data.m_Counts = builder.WriteBuffer("Material Bin Counts", description, xiiGALResourceStateFlags::CopyDestination);
      data.m_Data = std::move(bins);
      data.m_View = xiiRenderPassUtils::Constants(builder, "Authored Surface View", sizeof(data.m_ViewSnapshot));
      data.m_ViewSnapshot.MaterialWorldToView = context.m_pView->GetViewMatrix(xiiCameraEye::Left);
      data.m_ViewSnapshot.MaterialViewToWorld = context.m_pView->GetViewMatrix(xiiCameraEye::Left).GetInverse();
      data.m_ViewSnapshot.MaterialViewToClip = context.m_pView->GetProjectionMatrix(xiiCameraEye::Left);
      data.m_ViewSnapshot.MaterialClipToView = context.m_pView->GetProjectionMatrix(xiiCameraEye::Left).GetInverse();
      const auto* camera = context.m_pView->GetCamera();
      data.m_ViewSnapshot.MaterialCameraPosition = camera->GetPosition().GetAsVec4(1);
      data.m_ViewSnapshot.MaterialCameraForward = camera->GetDirForwards().GetAsVec4(0);
      data.m_ViewSnapshot.MaterialCameraRight = camera->GetDirRight().GetAsVec4(0);
      data.m_ViewSnapshot.MaterialCameraUp = camera->GetDirUp().GetAsVec4(0);
      data.m_ViewSnapshot.MaterialTimes = xiiVec4(static_cast<float>(xiiClock::GetGlobalClock()->GetAccumulatedTime().GetSeconds()), static_cast<float>(context.m_pView->GetWorld()->GetClock().GetAccumulatedTime().GetSeconds()), camera->GetExposure(), 0);

      for (xiiUInt32 i = 1; i < batches.GetCount(); ++i)
      {
        auto& batch = batches[i];
        batch.m_ViewConstants = data.m_View;
        if (!batch.m_Binding.m_Constants.IsEmpty())
          {
          xiiStringBuilder name; name.SetFormat("Material Constants {}", i);
          batch.m_Constants = xiiRenderPassUtils::Constants(builder, name, batch.m_Binding.m_Constants.GetCount());
          }
      }
      data.m_Batches = batches; }, [](const Upload& data, xiiRenderGraphPassContext& execute) {
      auto& commands = execute.GetCommandList();
      {
        xiiGALMapHelper<xiiNativeMaterialViewConstants> mapped(commands, execute.GetBuffer(data.m_View), xiiGALMapType::Write, xiiGALMapFlags::Discard);
        if (!mapped) { execute.ReportFailure("Could not map authored material view constants."); return; }
        *mapped = data.m_ViewSnapshot;
      }
      if (!data.m_Data.IsEmpty()) commands.UpdateBuffer(execute.GetBuffer(data.m_Bins), 0, data.m_Data.GetByteArrayPtr());
      xiiDynamicArray<xiiUInt32> zeros;
      zeros.SetCount(data.m_Batches.GetCount(), 0u);
      commands.UpdateBuffer(execute.GetBuffer(data.m_Counts), 0, zeros.GetByteArrayPtr());
      for (const auto& batch : data.m_Batches)
        if (batch.m_Constants.IsValid())
        {
          xiiGALMapHelper<xiiUInt8> mapped(commands, execute.GetBuffer(batch.m_Constants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
          if (!mapped) { execute.ReportFailure("Could not map material constants."); return; }
          xiiMemoryUtils::Copy(static_cast<xiiUInt8*>(mapped), batch.m_Binding.m_Constants.GetData(), batch.m_Binding.m_Constants.GetCount());
        } });
  struct Data
  {
    xiiRenderGraphBufferHandle m_Bins, m_Counts, m_Draws, m_Visible, m_Count, m_Sorted;
    xiiUInt32                  m_Groups;
  };
  xiiRenderGraphBufferHandle counts = upload.first->m_Counts;
  for (xiiUInt32 phase = 0; phase < 3; ++phase)
  {
    const auto pipeline = phase == 0 ? m_Count : phase == 1 ? m_Prefix :
                                                              m_Scatter;
    const auto emitted  = context.m_Graph.AddPass<Data>(phase == 0 ? "Count Material Draws" : phase == 1 ? "Prefix Material Draws" :
                                                                                                           "Scatter Material Draws",
                                                       xiiGALCommandQueueFlags::Compute, [&](Data& data, xiiRenderGraphBuilder& builder) {
        data.m_Bins = builder.ReadBuffer(upload.first->m_Bins, xiiGALResourceStateFlags::ShaderResource);
        data.m_Visible = builder.ReadBuffer(visible, xiiGALResourceStateFlags::ShaderResource);
        data.m_Count = builder.ReadBuffer(count, xiiGALResourceStateFlags::ShaderResource);
        data.m_Counts = builder.ReadWriteBuffer(counts, xiiGALResourceStateFlags::UnorderedAccess);
        xiiGALBufferCreationDescription description;
        description.m_Mode = xiiGALBufferMode::Structured; description.m_uiElementByteStride = 16; description.m_uiSize = batches.GetCount() * 16u;
        description.m_BindFlags = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::IndirectDrawArguments;
        if (phase == 1) data.m_Draws = builder.WriteBuffer("Material Draws", description, xiiGALResourceStateFlags::UnorderedAccess);
        else if (phase == 2) data.m_Draws = builder.ReadWriteBuffer(draws, xiiGALResourceStateFlags::UnorderedAccess);
        if (phase == 2)
        {
          description.m_uiElementByteStride = 8; description.m_uiSize = capacity * 8u;
          data.m_Sorted = builder.WriteBuffer("Material Visible Meshlets", description, xiiGALResourceStateFlags::UnorderedAccess);
        }
        data.m_Groups = phase == 1 ? 1 : static_cast<xiiUInt32>(groups); }, [pipeline](const Data& data, xiiRenderGraphPassContext& execute) {
        auto& commands = execute.GetCommandList(); commands.SetPipelineState(pipeline.Borrow());
        for (const auto& binding : {std::pair<const char*, xiiRenderGraphBufferHandle>("g_InstanceBins", data.m_Bins), {"g_VisibleMeshlets", data.m_Visible}, {"g_VisibleMeshletCount", data.m_Count}})
          commands.ResolveAndSetShaderResourceBufferView(xiiTempHashedString(xiiStringView(binding.first)), execute.GetBuffer(binding.second)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
        for (const auto& binding : {std::pair<const char*, xiiRenderGraphBufferHandle>("g_BinCounts", data.m_Counts), {"g_BinDraws", data.m_Draws}, {"g_BinnedMeshlets", data.m_Sorted}})
          if (binding.second.IsValid()) commands.ResolveAndSetUnorderedAccessBufferView(xiiTempHashedString(xiiStringView(binding.first)), execute.GetBuffer(binding.second)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
        if (commands.CommitShaderResources(xiiGALStateTransitionMode::Verify).Failed()) { execute.ReportFailure("Could not bind material draw binning resources."); return; }
        commands.DispatchCompute({data.m_Groups, 1, 1}); });
    counts = emitted.first->m_Counts;
    if (phase == 1)
      draws = emitted.first->m_Draws;
    if (phase == 2)
    {
      sortedVisible = emitted.first->m_Sorted;
      draws         = emitted.first->m_Draws;
    }
  }
  return XII_SUCCESS;
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Implementation_MaterialRasterBatches);
