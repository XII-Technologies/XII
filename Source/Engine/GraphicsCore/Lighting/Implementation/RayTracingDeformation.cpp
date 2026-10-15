/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Lighting/Implementation/RayTracingDeformation.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Shader/ShaderPermutationResource.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/RayTracing/RayTracingDeformationConstants.h>
#include <bit>

xiiRayTracingDeformedVertices::~xiiRayTracingDeformedVertices()
{
  if (m_Srv.IsValid() && xiiGALBindlessResourceTable::IsInitialized())
    xiiGALBindlessResourceTable::RetireBufferSRV(m_Srv, m_LastUse);
}

xiiResult xiiAddRayTracingDeformation(xiiRenderGraph& graph, xiiUInt64 frame, xiiStringView name,
                                      xiiRayTracingDeformedVertices& cache, xiiSharedPtr<xiiGALComputePipelineState>& pipeline,
                                      const xiiGpuInstanceData& instances, xiiUInt32 drawIndex, bool& dispatched)
{
  dispatched = false;
  // Extraction is immutable throughout a graph setup, including repeated sections/views.
  if (cache.m_SetupSerial == graph.GetSetupSerial())
    return XII_SUCCESS;
  xiiResourceLock<xiiMeshBufferResource> source(instances.GetMeshBufferResource(drawIndex), xiiResourceAcquireMode::PointerOnly);
  if (source.GetAcquireResult() != xiiResourceAcquireResult::Final || !source->GetVertexBuffer())
    return XII_FAILURE;
  const auto&                       record  = instances.GetInstances()[drawIndex];
  const auto                        weights = instances.GetMorphWeights(drawIndex);
  xiiRayTracingDeformationConstants constants;
  constants.VertexLayout = xiiVec4U32(source->GetVertexCount(), source->GetVertexStride(), xiiInvalidIndex, xiiInvalidIndex);
  constants.SkinLayout   = xiiVec4U32(xiiInvalidIndex, record.m_uiBoneIndicesOffset, record.m_uiBoneWeightsOffset, record.m_uiSkinningCount);
  constants.Deformation  = xiiVec4U32(weights.GetCount(), record.m_uiSkinningOffset, 0, 0);
  for (const auto& stream : source->GetVertexStreams())
  {
    if (stream.m_uiStride != source->GetVertexStride())
      return XII_FAILURE;
    if (stream.m_Semantic == xiiMeshVertexSemantic::Position && stream.m_Format == xiiMeshVertexStreamFormat::Float3)
      constants.VertexLayout.z = stream.m_uiOffset;
    if (stream.m_Semantic == xiiMeshVertexSemantic::Normal && stream.m_Format == xiiMeshVertexStreamFormat::Float3)
      constants.VertexLayout.w = stream.m_uiOffset;
    if (stream.m_Semantic == xiiMeshVertexSemantic::Tangent && stream.m_Format == xiiMeshVertexStreamFormat::Float4)
      constants.SkinLayout.x = stream.m_uiOffset;
  }
  const auto targets = instances.GetMorphTargets(drawIndex);
  if (constants.VertexLayout.z == xiiInvalidIndex || (source->GetVertexStride() & 3) != 0 || weights.GetCount() > targets.GetCount())
    return XII_FAILURE;
  const auto validStream = [stride = source->GetVertexStride()](xiiUInt32 offset, xiiUInt32 size) {
    return offset == xiiInvalidIndex || (stride >= size && offset <= stride - size && (offset & 3) == 0);
  };
  if (!validStream(constants.VertexLayout.z, 12) || !validStream(constants.VertexLayout.w, 12) || !validStream(constants.SkinLayout.x, 16))
    return XII_FAILURE;
  const auto                  pose = instances.GetSkinningMatrices().GetSubArray(record.m_uiSkinningOffset, record.m_uiSkinningCount);
  xiiDynamicArray<xiiVec4U32> morphs;
  for (xiiUInt32 i = 0; i < weights.GetCount(); ++i)
  {
    const auto& target = targets[i];
    if (!xiiMath::IsFinite(weights[i]) || target.m_uiVertexOffset > source->GetVertexCount() || target.m_uiVertexCount > source->GetVertexCount() - target.m_uiVertexOffset)
      return XII_FAILURE;
    morphs.PushBack(xiiVec4U32(target.m_uiVertexOffset, target.m_uiVertexCount, std::bit_cast<xiiUInt32>(weights[i]), 0));
  }
  const bool newSource = cache.m_Source != source->GetVertexBuffer();
  const bool changed   = newSource || cache.m_MeshRevision != source->GetCurrentResourceChangeCounter() || cache.m_Pose.GetArrayPtr() != pose || cache.m_Morphs != morphs;
  if (changed)
  {
    cache.m_Source       = source->GetVertexBuffer();
    cache.m_MeshRevision = source->GetCurrentResourceChangeCounter();
    cache.m_Pose         = pose;
    cache.m_Morphs       = morphs;
    ++cache.m_Revision;
  }
  cache.m_LastUse = frame;
  if (newSource || !cache.m_Buffer || !cache.m_Srv.IsValid())
  {
    if (cache.m_Srv.IsValid())
      xiiGALBindlessResourceTable::RetireBufferSRV(cache.m_Srv, frame);
    cache.m_Srv = {};
    xiiGALBufferCreationDescription description;
    description.m_uiSize    = source->GetVertexBuffer()->GetDescription().m_uiSize;
    description.m_Mode      = xiiGALBufferMode::Raw;
    description.m_BindFlags = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::RayTracing;
    description.m_Usage     = xiiGALResourceUsage::Mutable;
    cache.m_Buffer          = xiiGALDevice::GetDefaultDevice()->CreateBuffer(description);
    if (!cache.m_Buffer)
      return XII_FAILURE;
    cache.m_Buffer->SetDebugName(name);
    cache.m_Srv = xiiGALBindlessResourceTable::RegisterBufferSRV(cache.m_Buffer->GetDefaultView(xiiGALBufferViewType::ShaderResource));
    if (!cache.m_Srv.IsValid())
      return XII_FAILURE;
    cache.m_ReadyRevision = 0;
  }
  const bool needsDispatch = cache.m_ReadyRevision != cache.m_Revision;
  if (needsDispatch && !pipeline)
  {
    const auto                                     shader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/RayTracingDeformation.xiiShader");
    xiiHashTable<xiiHashedString, xiiHashedString> variables;
    const auto                                     permutationHandle = xiiShaderPermutationUtilities::PreloadSinglePermutation(shader, variables, false);
    if (!permutationHandle.IsValid())
      return XII_FAILURE;
    xiiResourceLock<xiiShaderPermutationResource> permutation(permutationHandle, xiiResourceAcquireMode::BlockTillLoaded);
    if (!permutation.IsValid() || !permutation->IsShaderValid())
      return XII_FAILURE;
    xiiGALComputePipelineStateCreationDescription description;
    description.m_pComputeShader             = permutation->GetGALShader(xiiGALShaderType::Compute);
    description.m_pPipelineResourceSignature = permutation->GetPipelineResourceSignature();
    pipeline                                 = xiiGALPipelineCache::GetPipeline(description);
    if (!pipeline)
      return XII_FAILURE;
  }
  const auto skinning = needsDispatch ? instances.AddUploadPass(graph, frame).m_Skinning : xiiRenderGraphBufferHandle{};
  struct Pass
  {
    xiiRenderGraphBufferHandle               m_Output, m_Source, m_Skinning, m_Morphs, m_Constants;
    xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
    xiiDynamicArray<xiiVec4U32>              m_Targets;
    xiiRayTracingDeformationConstants        m_Settings;
    xiiUInt64                                m_Revision = 0;
    bool                                     m_Dispatch = false;
  };
  auto pass           = graph.AddPass<Pass>("Ray Tracing Vertex Deformation", xiiGALCommandQueueFlags::Compute, [&](Pass& data, xiiRenderGraphBuilder& builder) {
    data.m_Output = builder.ImportBuffer(name, cache.m_Buffer, cache.m_Buffer->GetResourceState());
    data.m_Dispatch = needsDispatch;
    data.m_Revision = cache.m_Revision;
    if (!needsDispatch) return;
    data.m_Pipeline = pipeline;
    data.m_Settings = constants;
    data.m_Targets = morphs;
    if (data.m_Targets.IsEmpty()) data.m_Targets.PushBack(xiiVec4U32::MakeZero());
    data.m_Output = builder.WriteBuffer(data.m_Output, xiiGALResourceStateFlags::UnorderedAccess);
    xiiStringBuilder resourceName(name);
    resourceName.Append(" Source");
    data.m_Source = builder.ReadBuffer(builder.ImportBuffer(resourceName, cache.m_Source, cache.m_Source->GetResourceState()), xiiGALResourceStateFlags::ShaderResource | xiiGALResourceStateFlags::BuildASRead);
    data.m_Skinning = builder.ReadBuffer(skinning, xiiGALResourceStateFlags::ShaderResource);
    xiiGALBufferCreationDescription description;
    description.m_uiSize = data.m_Targets.GetCount() * sizeof(xiiVec4U32);
    description.m_uiElementByteStride = sizeof(xiiVec4U32);
    description.m_Mode = xiiGALBufferMode::Structured;
    description.m_BindFlags = xiiGALBindFlags::ShaderResource;
    description.m_Usage = xiiGALResourceUsage::Mutable;
    resourceName.Set(name); resourceName.Append(" Morphs");
    data.m_Morphs = builder.WriteBuffer(resourceName, description, xiiGALResourceStateFlags::ShaderResource);
    description.m_uiSize = sizeof(xiiRayTracingDeformationConstants);
    description.m_uiElementByteStride = 0;
    description.m_Mode = xiiGALBufferMode::Undefined;
    description.m_BindFlags = xiiGALBindFlags::UniformBuffer;
    description.m_Usage = xiiGALResourceUsage::Dynamic;
    description.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
    resourceName.Set(name); resourceName.Append(" Constants");
    data.m_Constants = builder.WriteBuffer(resourceName, description, xiiGALResourceStateFlags::ConstantBuffer); }, [target = &cache](const Pass& data, xiiRenderGraphPassContext& context) {
    if (!data.m_Dispatch) return;
    auto& commands = context.GetCommandList();
    commands.UpdateBuffer(context.GetBuffer(data.m_Morphs), 0, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(data.m_Targets.GetData()), data.m_Targets.GetCount() * sizeof(xiiVec4U32)));
    {
      xiiGALMapHelper<xiiRayTracingDeformationConstants> constants(commands, context.GetBuffer(data.m_Constants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      *constants = data.m_Settings;
    }
    commands.SetPipelineState(data.m_Pipeline.Borrow());
    commands.ResolveAndSetConstantBuffer("xiiRayTracingDeformationConstants", context.GetBuffer(data.m_Constants), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceBufferView("g_SourceVertices", context.GetBuffer(data.m_Source)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceBufferView("g_SkinningMatrices", context.GetBuffer(data.m_Skinning)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceBufferView("g_MorphTargets", context.GetBuffer(data.m_Morphs)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetUnorderedAccessBufferView("g_DeformedVertices", context.GetBuffer(data.m_Output)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    if (commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).Failed()) return;
    commands.DispatchCompute({(data.m_Settings.VertexLayout.x + 63) / 64, 1, 1});
    target->m_ReadyRevision = data.m_Revision; });
  cache.m_Handle      = pass.first->m_Output;
  cache.m_SetupSerial = graph.GetSetupSerial();
  dispatched          = needsDispatch;
  return XII_SUCCESS;
}
