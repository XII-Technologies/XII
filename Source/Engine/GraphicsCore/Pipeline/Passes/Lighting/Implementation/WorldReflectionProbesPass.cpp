/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/World/World.h>
#include <Foundation/Profiling/Profiling.h>
#include <GraphicsCore/Components/Lights/ReflectionCaptureComponent.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/WorldReflectionProbesPass.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/Tools/MapHelper.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiReflectionProbeFrameData, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiWorldReflectionProbesPass, 1, xiiRTTIDefaultAllocator<xiiWorldReflectionProbesPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Enabled", m_bEnabled)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("ReflectionProbes", m_Probes)->AddFlags(xiiPropertyFlags::ReadOnly),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Lighting") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiWorldReflectionProbesPass::Build(xiiRenderPipelinePassContext& context)
{
  if (!m_bEnabled)
    return context.SetOutput(m_Probes, XII_DEFAULT_NEW(xiiReflectionProbeFrameData, context.m_Graph.GetSetupSerial(), xiiRenderGraphBufferHandle(), xiiRenderGraphBufferHandle(), xiiReflectionProbeFrameData::Textures()));
  if (!context.m_pView || !context.m_pView->GetWorld() || !context.m_pView->GetExtractedRenderData())
    return context.ReportFailure("Reflection probes require an extracted authoritative view.");
  XII_PROFILE_SCOPE("World reflection probe extraction");
  const auto&                                                                      world = *context.m_pView->GetWorld();
  xiiHybridArray<const xiiReflectionCaptureRenderData*, XII_MAX_REFLECTION_PROBES> selected;
  for (const auto* packet : context.m_pView->GetExtractedRenderData()->GetAllRenderData())
  {
    const auto* probe = xiiDynamicCast<const xiiReflectionCaptureRenderData*>(packet);
    if (!probe)
      continue;
    const xiiComponent* component = nullptr;
    if (probe->m_hOwnerComponent.GetInternalID().m_WorldIndex != world.GetIndex() || !world.TryGetComponent(probe->m_hOwnerComponent, component) || !component->IsActiveAndInitialized() || !component->GetOwner()->IsActive() || component->GetOwner()->GetHandle() != probe->m_hOwnerObject)
      continue;
    bool duplicate = false;
    for (const auto* previous : selected)
      duplicate |= previous->m_hOwnerComponent == probe->m_hOwnerComponent;
    if (duplicate)
      continue;
    if (selected.GetCount() == XII_MAX_REFLECTION_PROBES)
      return context.ReportFailure("The extracted reflection probes exceed the supported descriptor capacity.");
    selected.PushBack(probe);
  }
  selected.Sort([](const auto* a, const auto* b) { return a->m_hOwnerComponent.GetInternalID().m_Data < b->m_hOwnerComponent.GetInternalID().m_Data; });
  xiiHybridArray<xiiGPUReflectionProbe, XII_MAX_REFLECTION_PROBES> records;
  xiiReflectionProbeFrameData::Textures                            textures;
  xiiReflectionProbeFrameData::Environments                        environments;
  for (const auto* probe : selected)
  {
    const auto& transform = probe->m_GlobalTransform;
    if (!transform.IsValid() || xiiMath::Abs(transform.m_vScale.x) < 1e-6f || xiiMath::Abs(transform.m_vScale.y) < 1e-6f || xiiMath::Abs(transform.m_vScale.z) < 1e-6f ||
        !probe->m_vHalfExtents.IsValid() || probe->m_vHalfExtents.x <= 0 || probe->m_vHalfExtents.y <= 0 || probe->m_vHalfExtents.z <= 0 ||
        !xiiMath::IsFinite(probe->m_fSphereRadius) || probe->m_fSphereRadius <= 0 || !xiiMath::IsFinite(probe->m_fBlendDistance) || probe->m_fBlendDistance < 0 ||
        !xiiMath::IsFinite(probe->m_fIntensity) || probe->m_fIntensity < 0 || !xiiMath::IsFinite(probe->m_fSaturation) || probe->m_fSaturation < 0 || probe->m_fSaturation > 1 ||
        (probe->m_InfluenceShape != xiiReflectionProbeInfluenceShape::Sphere && probe->m_InfluenceShape != xiiReflectionProbeInfluenceShape::Box) || !probe->m_hReflectionMap.IsValid())
      return context.ReportFailure("Invalid extracted reflection probe transform, influence or radiance settings.");
    xiiResourceLock<xiiTextureCubeResource> resource(probe->m_hReflectionMap, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!resource.IsValid())
      return context.ReportFailure("An authored reflection cubemap could not be loaded.");
    xiiSharedPtr<const xiiEnvironmentMap> environment;
    const auto                            prepared = resource->GetEnvironmentMap(environment);
    if (prepared.Failed())
      return context.ReportFailure(prepared.GetMessageString());
    xiiGPUReflectionProbe record{};
    const auto            inverse = transform.GetAsMat4().GetInverse();
    if (!inverse.IsValid())
      return context.ReportFailure("Reflection probe transform could not be inverted.");
    record.WorldToProbe = inverse;
    const auto  scale   = transform.m_vScale.Abs();
    const float radius  = probe->m_InfluenceShape == xiiReflectionProbeInfluenceShape::Sphere ? probe->m_fSphereRadius * xiiMath::Max(scale.x, scale.y, scale.z) : probe->m_vHalfExtents.CompMul(scale).GetLength();
    if (!xiiMath::IsFinite(radius))
      return context.ReportFailure("Reflection probe world bounds exceed the supported numeric range.");
    record.PositionAndRadius   = xiiVec4(transform.m_vPosition, radius);
    record.HalfExtentsAndBlend = xiiVec4(probe->m_vHalfExtents, probe->m_fBlendDistance);
    record.ProbeParameters     = xiiVec4(probe->m_fSphereRadius, probe->m_fIntensity, probe->m_fSaturation, probe->m_InfluenceShape == xiiReflectionProbeInfluenceShape::Sphere ? float(XII_REFLECTION_PROBE_SHAPE_SPHERE) : float(XII_REFLECTION_PROBE_SHAPE_BOX));
    record.Metadata            = xiiVec4U32(records.GetCount(), records.GetCount(), static_cast<xiiUInt32>(probe->m_iPriority), probe->m_bParallaxCorrected ? 1 : 0);
    records.PushBack(record);
    textures.PushBack(environment->m_Specular);
    environments.PushBack(std::move(environment));
  }
  xiiRenderGraphBufferHandle recordBuffer, constants;
  if (!records.IsEmpty())
  {
    xiiRenderPassResourceScope scope(context);
    struct Data
    {
      xiiHybridArray<xiiGPUReflectionProbe, XII_MAX_REFLECTION_PROBES> m_Records;
      xiiRenderGraphBufferHandle                                       m_Buffer, m_Constants;
    };
    const auto uploaded = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Transfer, [&](Data& data, xiiRenderGraphBuilder& builder) {
      data.m_Records = std::move(records);
      xiiGALBufferCreationDescription description;
      description.m_uiElementByteStride = sizeof(xiiGPUReflectionProbe);
      description.m_uiSize = data.m_Records.GetCount() * sizeof(xiiGPUReflectionProbe);
      description.m_Mode = xiiGALBufferMode::Structured;
      description.m_BindFlags = xiiGALBindFlags::ShaderResource;
      data.m_Buffer = builder.WriteBuffer("Reflection Probe Records", description, xiiGALResourceStateFlags::CopyDestination);
      data.m_Constants = xiiRenderPassUtils::Constants(builder, "Reflection Probe Counts", sizeof(xiiReflectionProbeConstants)); }, [](const Data& data, xiiRenderGraphPassContext& execute) {
      auto& commands = execute.GetCommandList();
      commands.UpdateBuffer(execute.GetBuffer(data.m_Buffer), 0, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(data.m_Records.GetData()), data.m_Records.GetCount() * sizeof(xiiGPUReflectionProbe)));
      xiiGALMapHelper<xiiReflectionProbeConstants> mapped(commands, execute.GetBuffer(data.m_Constants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      *mapped = {};
      mapped->ActiveProbeCount = data.m_Records.GetCount(); });
    recordBuffer        = uploaded.first->m_Buffer;
    constants           = uploaded.first->m_Constants;
  }
  return context.SetOutput(m_Probes, XII_DEFAULT_NEW(xiiReflectionProbeFrameData, context.m_Graph.GetSetupSerial(), recordBuffer, constants, std::move(textures), std::move(environments)));
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Lighting_Implementation_WorldReflectionProbesPass);
