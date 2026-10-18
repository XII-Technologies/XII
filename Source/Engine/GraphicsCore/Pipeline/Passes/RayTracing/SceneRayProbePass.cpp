/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/RayTracing/SceneRayProbePass.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/RayTracing/RayTracingMaterialData.h>
#include <Shaders/Pipeline/Passes/RayTracing/SceneRayProbeConstants.h>
namespace
{
  bool ValidRay(const xiiVec3& origin, const xiiVec3& direction, float minimum, float maximum, xiiUInt32 mask)
  {
    return origin.IsValid() && direction.IsValid() && direction.GetLengthSquared() > 1e-12f && xiiMath::IsFinite(direction.GetLengthSquared()) && xiiMath::IsFinite(minimum) && xiiMath::IsFinite(maximum) && minimum >= 0 && maximum > minimum && mask <= 255;
  }
  xiiGALBufferCreationDescription ResultDescription()
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize = desc.m_uiElementByteStride = sizeof(xiiSceneRayProbeResult);
    desc.m_Mode                                = xiiGALBufferMode::Structured;
    desc.m_Usage                               = xiiGALResourceUsage::Mutable;
    desc.m_BindFlags                           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    return desc;
  }
} // namespace
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSceneRayProbePass, 1, xiiRTTIDefaultAllocator<xiiSceneRayProbePass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Scene", m_Scene)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Materials", m_Materials)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Result", m_Result)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Origin", m_Origin)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3(0, 0, 100))),
    XII_MEMBER_PROPERTY("Direction", m_Direction)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3(0, 0, -1))),
    XII_MEMBER_PROPERTY("MinimumDistance", m_fMinimumDistance)->AddAttributes(new xiiDefaultValueAttribute(0.001f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("MaximumDistance", m_fMaximumDistance)->AddAttributes(new xiiDefaultValueAttribute(1000.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("InstanceMask", m_uiInstanceMask)->AddAttributes(new xiiDefaultValueAttribute(255U), new xiiClampValueAttribute(0U, 255U)),
    XII_MEMBER_PROPERTY("AsyncCompute", m_bAsyncCompute)->AddAttributes(new xiiDefaultValueAttribute(true)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/RayTracing") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiSceneRayProbePass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return ValidRay(m_Origin, m_Direction, m_fMinimumDistance, m_fMaximumDistance, m_uiInstanceMask) ? XII_SUCCESS : XII_FAILURE;
}
xiiResult xiiSceneRayProbePass::Build(xiiRenderPipelinePassContext& context)
{
  if (!xiiGALDevice::HasDefaultDevice() || xiiGALDevice::GetDefaultDevice()->GetFeatures().m_RayTracing != xiiGALDeviceFeatureState::Enabled)
    return context.ReportFailure("Scene ray probes require hardware ray-tracing support.");
  auto origin = m_Origin, direction = m_Direction;
  auto minimum = m_fMinimumDistance, maximum = m_fMaximumDistance;
  auto mask = m_uiInstanceMask;
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::ReadParameter(context, "RayProbeOrigin", origin));
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::ReadParameter(context, "RayProbeDirection", direction));
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::ReadParameter(context, "RayProbeMinimumDistance", minimum));
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::ReadParameter(context, "RayProbeMaximumDistance", maximum));
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::ReadParameter(context, "RayProbeInstanceMask", mask));
  if (!ValidRay(origin, direction, minimum, maximum, mask))
    return context.ReportFailure("Invalid scene ray origin, direction, range or instance mask.");
  direction.Normalize();
  const auto                 scene = context.GetInputAccelerationStructure(m_Scene);
  xiiRenderPassResourceScope scope(context);
  if (scene.IsEmpty())
  {
    struct Empty
    {
      xiiRenderGraphBufferHandle m_Result;
      xiiSceneRayProbeResult     m_Miss;
    };
    const auto pass = context.m_Graph.AddPass<Empty>(context.m_sPassName, xiiGALCommandQueueFlags::Transfer, [&](Empty& data, xiiRenderGraphBuilder& builder) {
      data.m_Result=builder.WriteBuffer("Ray Probe Result",ResultDescription(),xiiGALResourceStateFlags::CopyDestination);
      data.m_Miss.Identity=xiiVec4U32(0,xiiInvalidIndex,xiiInvalidIndex,0); data.m_Miss.Intersection=xiiVec4(maximum,0,0,0); }, [](const Empty& data, xiiRenderGraphPassContext& execute) { execute.GetCommandList().UpdateBuffer(execute.GetBuffer(data.m_Result), 0, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(&data.m_Miss), sizeof(data.m_Miss))); });
    return context.SetOutput(m_Result, pass.first->m_Result);
  }
  if (!scene.IsValid() || !context.m_Graph.GetBufferDescription(scene.m_Dependency))
    return context.ReportFailure("The supplied TLAS has no valid current-setup build dependency.");
  const auto  materials           = context.GetInputBuffer(m_Materials);
  const auto* materialDescription = context.m_Graph.GetBufferDescription(materials);
  if (!materialDescription || materialDescription->m_Mode != xiiGALBufferMode::Structured || materialDescription->m_uiElementByteStride != sizeof(xiiRayTracingMaterialData) || materialDescription->m_uiSize < sizeof(xiiRayTracingMaterialData) || !materialDescription->m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource))
    return context.ReportFailure("A nonempty scene ray probe requires canonical material hit records carrying world GPU identities.");
  const auto& limits = xiiGALDevice::GetDefaultDevice()->GetGraphicsDeviceAdapterProperties().m_RayTracingProperties;
  if (!limits.m_uiShaderGroupHandleSize || !xiiMath::IsPowerOf2(limits.m_uiShaderGroupBaseAlignment) || !xiiMath::IsPowerOf2(limits.m_uiShaderGroupHandleAlignment) || !limits.m_uiMaxRecursionDepth)
    return context.ReportFailure("Invalid ray-tracing shader-record capabilities.");
  const xiiUInt64 stride = xiiMemoryUtils::AlignSize(xiiUInt64(limits.m_uiShaderGroupHandleSize), xiiUInt64(xiiMath::Max(limits.m_uiShaderGroupBaseAlignment, limits.m_uiShaderGroupHandleAlignment)));
  if (!stride || stride > limits.m_uiMaxShaderRecordStride || stride > xiiMath::MaxValue<xiiUInt32>() / 3U)
    return context.ReportFailure("Scene ray probe shader records exceed device limits.");
  if (!m_Shader.IsValid())
    m_Shader = xiiShaderPermutationUtilities::PreloadSinglePermutation(xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/RayTracing/SceneRayProbe.xiiShader"), {}, false);
  xiiResourceLock<xiiShaderPermutationResource> shader(m_Shader, xiiResourceAcquireMode::BlockTillLoaded);
  if (!shader.IsValid() || !shader->IsShaderValid())
    return context.ReportFailure("Scene ray probe shader is unavailable.");
  xiiGALRayTracingPipelineStateCreationDescription description;
  description.m_pPipelineResourceSignature               = shader->GetPipelineResourceSignature();
  description.m_RayTracingPipeline.m_uiMaxRecursionDepth = 1;
  description.m_uiMaximumPayloadSize                     = sizeof(xiiSceneRayProbeResult);
  description.m_uiMaximumAttributeSize                   = 2 * sizeof(float);
  auto& generation                                       = description.m_GeneralShaders.ExpandAndGetRef();
  generation.m_sName.Assign("SceneRayProbeGeneration");
  generation.m_pShader = shader->GetGALShader(xiiGALShaderType::RayGeneration);
  auto& miss           = description.m_GeneralShaders.ExpandAndGetRef();
  miss.m_sName.Assign("SceneRayProbeMiss");
  miss.m_pShader = shader->GetGALShader(xiiGALShaderType::RayMiss);
  auto& hit      = description.m_TriangleHitShaders.ExpandAndGetRef();
  hit.m_sName.Assign("SceneRayProbeClosestTriangle");
  hit.m_pClosestHitShader = shader->GetGALShader(xiiGALShaderType::RayClosestHit);
  const auto pipeline     = xiiGALPipelineCache::GetPipeline(description);
  if (!pipeline)
    return context.ReportFailure("Could not create the scene ray probe pipeline.");
  struct Data
  {
    xiiRenderGraphBufferHandle                  m_Dependency, m_Result, m_Constants, m_Records, m_Materials;
    xiiSceneRayProbeConstants                   m_Snapshot;
    xiiSharedPtr<xiiGALTopLevelAS>              m_Scene;
    xiiSharedPtr<xiiGALRayTracingPipelineState> m_Pipeline;
    xiiUInt64                                   m_uiStride;
  };
  const auto pass = context.m_Graph.AddPass<Data>(context.m_sPassName, m_bAsyncCompute ? xiiGALCommandQueueFlags::Compute : xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Materials=builder.ReadBuffer(materials,xiiGALResourceStateFlags::ShaderResource);
    data.m_Dependency=builder.ReadBuffer(scene.m_Dependency,xiiGALResourceStateFlags::BuildASRead); data.m_Scene=scene.m_pStructure;
    data.m_Result=builder.WriteBuffer("Ray Probe Result",ResultDescription(),xiiGALResourceStateFlags::UnorderedAccess);
    data.m_Constants=xiiRenderPassUtils::Constants(builder,"Ray Probe Parameters",sizeof(data.m_Snapshot));
    data.m_Snapshot.OriginAndMinimumDistance=xiiVec4(origin.x,origin.y,origin.z,minimum);
    data.m_Snapshot.DirectionAndMaximumDistance=xiiVec4(direction.x,direction.y,direction.z,maximum); data.m_Snapshot.InstanceMaskAndPadding=xiiVec4U32(mask,0,0,0);
    xiiGALBufferCreationDescription records; records.m_uiSize=3*stride; records.m_Mode=xiiGALBufferMode::Raw; records.m_Usage=xiiGALResourceUsage::Mutable; records.m_BindFlags=xiiGALBindFlags::RayTracing;
    data.m_Records=builder.WriteBuffer("Ray Probe Shader Records",records,xiiGALResourceStateFlags::RayTracing);
    data.m_uiStride=stride; data.m_Pipeline=pipeline; }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands=execute.GetCommandList();
    { xiiGALMapHelper<xiiSceneRayProbeConstants> mapped(commands,execute.GetBuffer(data.m_Constants),xiiGALMapType::Write,xiiGALMapFlags::Discard); *mapped=data.m_Snapshot; }
    xiiGALStateTransitionDescription transition; transition.m_pResource=execute.GetBuffer(data.m_Constants); transition.m_NewState=xiiGALResourceStateFlags::ConstantBuffer; transition.m_TransitionFlags=xiiGALStateTransitionFlags::UpdateState;
    commands.TransitionResourceStates(xiiMakeArrayPtr(&transition,1));
    commands.SetPipelineState(data.m_Pipeline.Borrow());
    commands.ResolveAndSetAccelerationStructure("g_Scene",data.m_Scene.Borrow(),xiiGALShaderType::RayGeneration);
    commands.ResolveAndSetConstantBuffer("xiiSceneRayProbeConstants",execute.GetBuffer(data.m_Constants),xiiGALShaderType::RayGeneration);
    commands.ResolveAndSetUnorderedAccessBufferView("g_Result",execute.GetBuffer(data.m_Result)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess),xiiGALShaderType::RayGeneration);
    commands.ResolveAndSetShaderResourceBufferView("g_InstanceMaterials",execute.GetBuffer(data.m_Materials)->GetDefaultView(xiiGALBufferViewType::ShaderResource),xiiGALShaderType::RayClosestHit);
    commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).AssertSuccess();
    xiiGALUpdateSBTDescription records; records.m_pPipelineState=data.m_Pipeline.Borrow(); records.m_pShaderBindingTable=execute.GetBuffer(data.m_Records);
    records.m_RayGenerationTable={0,data.m_uiStride,data.m_uiStride}; records.m_MissTable={data.m_uiStride,data.m_uiStride,data.m_uiStride}; records.m_HitTable={2*data.m_uiStride,data.m_uiStride,data.m_uiStride};
    commands.UpdateSBT(records);
    xiiGALTraceRaysDescription trace(records.m_pShaderBindingTable,1,1,1); trace.m_RayGenerationTable=records.m_RayGenerationTable; trace.m_MissTable=records.m_MissTable; trace.m_HitTable=records.m_HitTable;
    commands.TraceRays(trace); });
  return context.SetOutput(m_Result, pass.first->m_Result);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_RayTracing_SceneRayProbePass);
