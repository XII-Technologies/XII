/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/World/World.h>
#include <GraphicsCore/Material/MaterialGpuStorage.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Particles/ParticleSystem.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Particles/ParticleBirthUploadPass.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <GraphicsFoundation/Resources/BindlessResourceTable.h>
#include <Shaders/Pipeline/Passes/Particles/BallisticParticleConstants.h>

namespace
{
  // Each immutable descriptor lease is retained by both the occurrence and its prepared recordings.
  // Streaming creates a new lease, never overwrites an index which an older recording can still read.
  struct ParticleSamplerLease : xiiRefCounted
  {
    xiiSharedPtr<xiiGALSampler>  m_Sampler;
    xiiGALBindlessResourceHandle m_Index;
    xiiUInt64                    m_uiLastUse = 0;
    ~ParticleSamplerLease()
    {
      if (m_Index.IsValid())
        xiiGALBindlessResourceTable::RetireSampler(m_Index, m_uiLastUse);
    }
  };
  struct ParticleTextureLease : xiiRefCounted
  {
    xiiSharedPtr<xiiGALTextureView>    m_View;
    xiiSharedPtr<ParticleSamplerLease> m_Sampler;
    xiiGALBindlessResourceHandle       m_TextureIndex;
    xiiUInt64                          m_uiLastUse = 0;
    ~ParticleTextureLease()
    {
      if (m_TextureIndex.IsValid())
        xiiGALBindlessResourceTable::RetireTextureSRV(m_TextureIndex, m_uiLastUse);
    }
  };
  template <typename T>
  void Upload(xiiRenderGraphPassContext& context, xiiRenderGraphBufferHandle handle, const xiiDynamicArray<T, xiiAlignedAllocatorWrapper>& values)
  {
    context.GetCommandList().UpdateBuffer(context.GetBuffer(handle), 0, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(values.GetData()), values.GetCount() * sizeof(T)));
  }
} // namespace
struct xiiParticleBirthUploadPass::State
{
  xiiDynamicArray<xiiSharedPtr<ParticleTextureLease>> m_Textures;
  xiiDynamicArray<xiiSharedPtr<ParticleSamplerLease>> m_Samplers;
};
xiiParticleBirthUploadPass::xiiParticleBirthUploadPass() : m_pState(XII_DEFAULT_NEW(State)) {}
xiiParticleBirthUploadPass::~xiiParticleBirthUploadPass() = default;
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleBirthUploadPass,1,xiiRTTIDefaultAllocator<xiiParticleBirthUploadPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Seeds",m_Seeds)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Batches",m_Batches)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Materials",m_Materials)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Enabled",m_bEnabled)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("MaximumParticles",m_uiMaximumParticles)->AddAttributes(new xiiDefaultValueAttribute(1048576U)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Particles") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiParticleBirthUploadPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return m_uiMaximumParticles && m_uiMaximumParticles <= xiiParticleSystemConstants::s_uiMaxSupportedParticles ? XII_SUCCESS : XII_FAILURE;
}
xiiResult xiiParticleBirthUploadPass::Build(xiiRenderPipelinePassContext& context)
{
  if (!m_bEnabled)
  {
    m_pState->m_Textures.Clear();
    m_pState->m_Samplers.Clear();
    return XII_SUCCESS;
  }
  if (!context.m_pView || !context.m_pView->GetWorld() || !context.m_pView->GetExtractedRenderData())
    return context.ReportFailure("Particle births require authoritative world extraction.");
  struct Data
  {
    xiiDynamicArray<xiiParticleGPUState, xiiAlignedAllocatorWrapper>        m_Seeds;
    xiiDynamicArray<xiiParticleTrajectoryBatch, xiiAlignedAllocatorWrapper> m_Batches;
    xiiDynamicArray<xiiGpuSurfaceMaterial, xiiAlignedAllocatorWrapper>      m_Materials;
    xiiDynamicArray<xiiSharedPtr<ParticleTextureLease>>                     m_Textures;
    xiiRenderGraphBufferHandle                                              m_hSeeds, m_hBatches, m_hMaterials;
  };
  Data            snapshot;
  const xiiWorld& world = *context.m_pView->GetWorld();
  for (const auto* extracted : context.m_pView->GetExtractedRenderData()->GetAllRenderData())
  {
    const auto* packet = xiiDynamicCast<const xiiParticleRenderData*>(extracted);
    if (!packet)
      continue;
    const xiiParticleSystemComponent* owner = nullptr;
    if (!world.TryGetComponent(packet->m_hOwnerComponent, owner) || owner->GetOwner()->GetHandle() != packet->m_hOwnerObject)
      return context.ReportFailure("Particle snapshot belongs to a different or retired world component.");
    if (packet->m_Renderers.IsEmpty())
      continue;
    if (!packet->m_sEmissionError.IsEmpty())
      return context.ReportFailure(packet->m_sEmissionError);
    const auto flags = packet->m_Descriptor.m_Flags;
    if (packet->m_bHasAdditionalSimulation || flags.IsAnySet(xiiParticleSystemFlags::SortByDepth | xiiParticleSystemFlags::EnableEvents | xiiParticleSystemFlags::EnableReadback | xiiParticleSystemFlags::MolecularDynamics | xiiParticleSystemFlags::NeighborSearch))
      return context.ReportFailure("Ballistic billboards do not execute graph, force, collision, sort, event, readback or molecular workloads.");
    for (const auto& renderer : packet->m_Renderers)
    {
      if (renderer.m_Type != xiiParticleRendererType::Billboard || renderer.m_bReceiveLighting || renderer.m_bCastShadows || renderer.m_bSortByDepth || renderer.m_Mesh.IsValid())
        return context.ReportFailure("Ballistic billboard rendering requires an unlit, unsorted, non-shadow-casting billboard renderer.");
      if (!renderer.m_Tint.IsValid() || !xiiMath::IsFinite(renderer.m_fSize) || renderer.m_fSize <= 0 || !xiiMath::IsFinite(renderer.m_fSoftParticleDistance) || renderer.m_fSoftParticleDistance < 0)
        return context.ReportFailure("Invalid particle tint, size or soft-intersection distance.");
      const auto first = snapshot.m_Seeds.GetCount();
      for (const auto& birth : packet->m_Births)
        if (birth.m_SystemBinding == renderer.m_SystemBinding)
        {
          if (snapshot.m_Seeds.GetCount() >= m_uiMaximumParticles)
            return context.ReportFailure("Particle view exceeds its authored particle budget.");
          auto seed        = birth.m_InitialState;
          seed.m_uiEmitter = snapshot.m_Batches.GetCount();
          snapshot.m_Seeds.PushBack(seed);
        }
      if (first == snapshot.m_Seeds.GetCount())
        continue;
      xiiMaterialInstanceSnapshot material;
      if (renderer.m_Material.IsValid())
      {
        xiiResourceLock<xiiMaterialResource> resource(renderer.m_Material, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
        const auto                           instance = resource.IsValid() && resource.GetAcquireResult() == xiiResourceAcquireResult::Final ? resource->GetDefaultInstance() : nullptr;
        if (!instance)
          return context.ReportFailure("Particle material could not be loaded.");
        instance->CreateSnapshot(material);
        if (!material.m_RuntimeState.IsTranslucent())
          return context.ReportFailure("Authored particle materials must explicitly select a transparent blend mode.");
        for (auto& binding : material.m_ResourceBindings)
        {
          binding.m_uiBindlessIndex = binding.m_uiSamplerIndex = xiiInvalidIndex;
          if (binding.m_hTextureCube.IsValid())
            return context.ReportFailure("Billboard surface materials require 2D textures.");
          if (!binding.m_hTexture2D.IsValid())
            continue;
          xiiResourceLock<xiiTexture2DResource> texture(binding.m_hTexture2D, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
          const auto                            gpuTexture = texture.IsValid() && texture.GetAcquireResult() == xiiResourceAcquireResult::Final ? texture->GetGALTexture() : nullptr;
          const auto                            view       = gpuTexture ? gpuTexture->GetDefaultView(xiiGALTextureViewType::ShaderResource) : nullptr;
          const auto                            sampler    = texture.IsValid() ? texture->GetGALSampler() : nullptr;
          if (!view || !sampler)
            return context.ReportFailure("Particle texture or authored sampler is unavailable.");
          xiiSharedPtr<ParticleTextureLease> lease;
          for (const auto& cached : m_pState->m_Textures)
            if (cached->m_View == view && cached->m_Sampler->m_Sampler == sampler)
            {
              lease = cached;
              break;
            }
          if (!lease)
          {
            lease         = XII_DEFAULT_NEW(ParticleTextureLease);
            lease->m_View = view;
            for (const auto& cached : m_pState->m_Samplers)
              if (cached->m_Sampler == sampler)
              {
                lease->m_Sampler = cached;
                break;
              }
            if (!lease->m_Sampler)
            {
              lease->m_Sampler            = XII_DEFAULT_NEW(ParticleSamplerLease);
              lease->m_Sampler->m_Sampler = sampler;
              lease->m_Sampler->m_Index   = xiiGALBindlessResourceTable::RegisterSampler(sampler);
              if (!lease->m_Sampler->m_Index.IsValid())
                return context.ReportFailure("Particle sampler descriptor capacity is exhausted.");
              m_pState->m_Samplers.PushBack(lease->m_Sampler);
            }
            lease->m_TextureIndex = xiiGALBindlessResourceTable::RegisterTextureSRV(view);
            if (!lease->m_TextureIndex.IsValid())
              return context.ReportFailure("Particle bindless descriptor capacity is exhausted.");
            m_pState->m_Textures.PushBack(lease);
          }
          lease->m_uiLastUse        = xiiMath::Max(lease->m_uiLastUse, context.m_Graph.GetFrameIndex());
          binding.m_uiBindlessIndex = lease->m_TextureIndex.m_uiIndex;
          binding.m_uiSamplerIndex  = lease->m_Sampler->m_Index.m_uiIndex;
          if (!snapshot.m_Textures.Contains(lease))
            snapshot.m_Textures.PushBack(lease);
        }
      }
      auto gpuMaterial = xiiResolveGpuSurfaceMaterial(material);
      if (!renderer.m_Material.IsValid())
      {
        gpuMaterial.Metadata.z = xiiMaterialAlphaMode::Blend;
        gpuMaterial.Metadata.w = xiiMaterialBlendMode::Translucent;
      }
      // Billboard orientation has two visible sides; authored shading is evaluated as unlit VFX.
      gpuMaterial.Metadata.y |= 1U << 10U;
      auto& batch           = snapshot.m_Batches.ExpandAndGetRef();
      batch.LocalToWorld    = packet->m_GlobalTransform.GetAsMat4();
      batch.Tint            = xiiVec4(renderer.m_Tint.r, renderer.m_Tint.g, renderer.m_Tint.b, renderer.m_Tint.a);
      const auto scale      = packet->m_GlobalTransform.m_vScale.Abs();
      batch.SizeAndSoftness = xiiVec4(renderer.m_fSize * 0.5f, renderer.m_fSoftParticleDistance, xiiMath::Max(scale.x, scale.y, scale.z), 0);
      batch.Policy          = xiiVec4U32(packet->m_Descriptor.m_SimulationSpace == xiiParticleSimulationSpace::Local ? 1 : 0, snapshot.m_Materials.GetCount(), 0, 0);
      snapshot.m_Materials.PushBack(gpuMaterial);
    }
  }
  // Drop only the occurrence's references. Prepared recordings retain all their descriptor leases.
  m_pState->m_Textures = snapshot.m_Textures;
  m_pState->m_Samplers.Clear();
  for (const auto& lease : snapshot.m_Textures)
    if (!m_pState->m_Samplers.Contains(lease->m_Sampler))
      m_pState->m_Samplers.PushBack(lease->m_Sampler);
  if (snapshot.m_Seeds.IsEmpty())
    return XII_SUCCESS;
  xiiRenderPassResourceScope scope(context);
  const auto                 emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Transfer, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data = std::move(snapshot);
    const auto buffer = [&](xiiStringView name, xiiUInt32 count, xiiUInt32 stride) {
      xiiGALBufferCreationDescription d; d.m_uiSize=xiiUInt64(count)*stride; d.m_uiElementByteStride=stride; d.m_Mode=xiiGALBufferMode::Structured; d.m_Usage=xiiGALResourceUsage::Mutable; d.m_BindFlags=xiiGALBindFlags::ShaderResource;
      return builder.WriteBuffer(name,d,xiiGALResourceStateFlags::CopyDestination);
    };
    data.m_hSeeds=buffer("Particle births",data.m_Seeds.GetCount(),sizeof(xiiParticleGPUState));
    data.m_hBatches=buffer("Particle trajectories",data.m_Batches.GetCount(),sizeof(xiiParticleTrajectoryBatch));
    data.m_hMaterials=buffer("Particle materials",data.m_Materials.GetCount(),sizeof(xiiGpuSurfaceMaterial)); }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    const auto frame=xiiRenderGraphManager::GetActiveFrameIndex();
    if(!data.m_Textures.IsEmpty()&&!frame){execute.ReportFailure("Particle descriptors require an active shared submission frame.");return;}
    for(const auto& lease:data.m_Textures){lease->m_uiLastUse=xiiMath::Max(lease->m_uiLastUse,frame);lease->m_Sampler->m_uiLastUse=xiiMath::Max(lease->m_Sampler->m_uiLastUse,frame);}
    Upload(execute,data.m_hSeeds,data.m_Seeds); Upload(execute,data.m_hBatches,data.m_Batches); Upload(execute,data.m_hMaterials,data.m_Materials); });
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_Seeds, emitted.first->m_hSeeds));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_Batches, emitted.first->m_hBatches));
  return context.SetOutput(m_Materials, emitted.first->m_hMaterials);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Particles_Implementation_ParticleBirthUploadPass);
