/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Lighting/LightingData.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/ScreenSpacePassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/ClusteredLocalSurfaceLightingPass.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/LightClusterPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/SurfaceLightingPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Shadows/Implementation/SpotShadowPassUtils.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/Device/Device.h>
#include <Shaders/Pipeline/Passes/Lighting/LocalLightReservoirConstants.h>
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiClusteredLocalSurfaceLightingPass,2,xiiRTTIDefaultAllocator<xiiClusteredLocalSurfaceLightingPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Depth",m_Depth)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Normal",m_Normal)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Albedo",m_Albedo)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Material",m_Material)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Emissive",m_Emissive)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Camera",m_Camera)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("SpotShadows",m_Spots)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("SpotShadowAtlas",m_SpotAtlas)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("ObjectId",m_ObjectId)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Velocity", m_Velocity)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("TemporalCamera", m_TemporalCamera)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("PreviousReservoirs", m_PreviousReservoirs)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Reservoirs", m_Reservoirs)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("UseReservoirs", m_bUseReservoirs)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("Lighting",m_Output)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("ReactiveRadiance",m_Reactive)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Layout",m_Layout)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Lights",m_Lights)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Counts",m_Counts)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Candidates",m_Candidates)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("IESProfiles",m_Profiles)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("LightGrid",m_Grid)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("LightIndices",m_Indices)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Overflow",m_Overflow)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Scale",m_fScale)->AddAttributes(new xiiDefaultValueAttribute(1.0f),new xiiClampValueAttribute(0.0f,xiiVariant())),
    XII_MEMBER_PROPERTY("AsyncCompute",m_bAsyncCompute),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Lighting") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiClusteredLocalSurfaceLightingPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return xiiMath::IsFinite(m_fScale) && m_fScale >= 0 ? XII_SUCCESS : XII_FAILURE;
}
xiiResult xiiClusteredLocalSurfaceLightingPass::Build(xiiRenderPipelinePassContext& context)
{
  xiiSurfaceLightingPassUtils::Inputs surface{context.GetInputTexture(m_Depth), context.GetInputTexture(m_Normal), context.GetInputTexture(m_Albedo), context.GetInputTexture(m_Material), context.GetInputTexture(m_Emissive), context.GetInputBuffer(m_Camera), {}};
  XII_SUCCEED_OR_RETURN(xiiSurfaceLightingPassUtils::Validate(context, surface));
  const auto  frame  = context.GetInputData(m_Layout);
  const auto* layout = frame ? xiiDynamicCast<const xiiLightClusterFrameData*>(frame.Borrow()) : nullptr;
  if (!layout || layout->m_Constants.Viewport.x != surface.m_Size.width || layout->m_Constants.Viewport.y != surface.m_Size.height)
    return context.ReportFailure("Local lighting requires a current cluster layout matching the surface extent.");
  const auto  lights = context.GetInputBuffer(m_Lights), counts = context.GetInputBuffer(m_Counts), candidates = context.GetInputBuffer(m_Candidates), profiles = context.GetInputBuffer(m_Profiles), grid = context.GetInputBuffer(m_Grid), indices = context.GetInputBuffer(m_Indices), overflow = context.GetInputBuffer(m_Overflow);
  const auto* records   = context.m_Graph.GetBufferDescription(lights);
  const auto* selection = context.m_Graph.GetBufferDescription(candidates);
  const auto* ies       = context.m_Graph.GetBufferDescription(profiles);
  using xiiLightClusterPassUtils::Buffer;
  if (!records || !records->m_uiSize || records->m_uiSize % sizeof(xiiGpuLightData) || records->m_uiSize / sizeof(xiiGpuLightData) > 1048576 || !Buffer(context, lights, sizeof(xiiGpuLightData), records->m_uiSize / sizeof(xiiGpuLightData), xiiGALBindFlags::ShaderResource) || !selection || !selection->m_uiSize || selection->m_uiSize % sizeof(xiiUInt32) || selection->m_uiSize / sizeof(xiiUInt32) > records->m_uiSize / sizeof(xiiGpuLightData) || !Buffer(context, candidates, sizeof(xiiUInt32), selection->m_uiSize / sizeof(xiiUInt32), xiiGALBindFlags::ShaderResource) || !ies || !ies->m_uiSize || ies->m_uiSize % sizeof(float) || (ies->m_uiSize != sizeof(float) && ies->m_uiSize % (2048 * sizeof(float))) || !Buffer(context, profiles, sizeof(float), ies->m_uiSize / sizeof(float), xiiGALBindFlags::ShaderResource) || !Buffer(context, counts, sizeof(xiiVec4U32), 1, xiiGALBindFlags::ShaderResource) || !Buffer(context, grid, sizeof(xiiVec2U32), layout->m_Constants.ClusterCounts.w, xiiGALBindFlags::ShaderResource) || !Buffer(context, indices, sizeof(xiiUInt32), layout->m_Constants.Work.y, xiiGALBindFlags::ShaderResource) || !Buffer(context, overflow, sizeof(xiiUInt32), 1, xiiGALBindFlags::ShaderResource))
    return context.ReportFailure("Local lighting requires exact canonical light, candidate, IES and cluster-list layouts.");
  const auto shadows = context.GetInputData(m_Spots).Downcast<const xiiSpotShadowSnapshot>();
  const auto atlas = context.GetInputTexture(m_SpotAtlas), ids = context.GetInputTexture(m_ObjectId);
  XII_SUCCEED_OR_RETURN(xiiSpotShadowPassUtils::Validate(context, shadows.Borrow(), atlas));
  const auto mode = context.m_pView ? context.m_pView->GetViewRenderMode().GetValue() : xiiViewRenderMode::Default;
  // The reservoir stores each viewport dimension minus one in 16 bits.
  const bool useReservoirs = m_bUseReservoirs && mode == xiiViewRenderMode::Default && surface.m_Size.width <= 65536 && surface.m_Size.height <= 65536 &&
    selection->m_uiSize / sizeof(xiiUInt32) > XII_LOCAL_LIGHT_RESERVOIR_MIN_LIGHTS;
  if (shadows || useReservoirs)
  {
    const auto* identities = context.m_Graph.GetTextureDescription(ids);
    if (!identities || identities->m_Format != xiiGALResourceFormat::R32UInt || identities->m_Type != xiiGALResourceDimension::Texture2D || identities->m_uiSampleCount != 1 || identities->m_uiArraySizeOrDepth != 1 || identities->m_Size != surface.m_Size || !identities->m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource))
      return context.ReportFailure("Local light shadows and reservoirs require matching readable R32UInt object identities.");
  }
  xiiRenderGraphTextureHandle     velocity, previousReservoirs;
  xiiSizeU32                      tiles, reservoirSize;
  xiiLocalLightReservoirConstants reservoirSettings{};
  if (useReservoirs)
  {
    velocity            = context.GetInputTexture(m_Velocity);
    const auto temporal = context.GetInputData(m_TemporalCamera).Downcast<const xiiTemporalCameraSnapshot>();
    if (!temporal || temporal->m_Sample.m_Size != surface.m_Size || !xiiScreenSpacePassUtils::Image(context, velocity, 2, &surface.m_Size))
      return context.ReportFailure("Local light reservoirs require matching velocity and temporal camera inputs.");
    const auto device       = xiiGALDevice::GetDefaultDevice();
    const auto maxDimension = device->GetGraphicsDeviceAdapterProperties().m_TextureProperties.m_uiMaxTexture2DDimension;
    if (maxDimension < XII_LOCAL_LIGHT_RESERVOIR_TEXELS)
      return context.ReportFailure("The device cannot allocate a local light reservoir tile.");
    constexpr xiiUInt64 maxGridBytes = 8 * 1024 * 1024;
    constexpr xiiUInt32 bytesPerTile = 16 * XII_LOCAL_LIGHT_RESERVOIR_TEXELS * XII_LOCAL_LIGHT_RESERVOIR_TEXELS;
    xiiUInt32           stride       = 2;
    for (;; stride *= 2)
    {
      tiles         = {(surface.m_Size.width - 1U) / stride + 1U, (surface.m_Size.height - 1U) / stride + 1U};
      reservoirSize = {tiles.width * XII_LOCAL_LIGHT_RESERVOIR_TEXELS, tiles.height * XII_LOCAL_LIGHT_RESERVOIR_TEXELS};
      if (reservoirSize.width <= maxDimension && reservoirSize.height <= maxDimension && xiiUInt64(tiles.width) * tiles.height * bytesPerTile <= maxGridBytes)
        break;
    }
    previousReservoirs   = context.GetInputTexture(m_PreviousReservoirs);
    const auto* previous = context.m_Graph.GetTextureDescription(previousReservoirs);
    if (!temporal->m_Sample.m_bHistoryValid || !previous || previous->m_Size != reservoirSize)
      previousReservoirs.Invalidate();
    reservoirSettings.LocalReservoirGrid         = xiiVec4U32(stride, xiiUInt32(context.m_Graph.GetFrameIndex()), xiiUInt32(temporal->m_Sample.m_uiPreviousFrame), previousReservoirs.IsValid() ? 1U : 0U);
    const auto jitter                            = temporal->m_Sample.m_PreviousJitter - temporal->m_Sample.m_Jitter;
    reservoirSettings.LocalReservoirReprojection = xiiVec4(jitter.x, jitter.y, 0, 0);
    XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_SeedPipeline, "Shaders/Pipeline/Passes/Lighting/LocalLightReservoirSeed.xiiShader"));
  }
  auto&      pipeline = useReservoirs ? m_ReservoirPipeline[shadows ? 1 : 0] : (shadows ? m_ShadowedPipeline : m_Pipeline);
  const auto shader   = useReservoirs ? (shadows ? "Shaders/Pipeline/Passes/Lighting/LocalLightReservoirShadowedResolve.xiiShader" : "Shaders/Pipeline/Passes/Lighting/LocalLightReservoirResolve.xiiShader") :
                                        (shadows ? "Shaders/Pipeline/Passes/Lighting/ClusteredLocalShadowedLighting.xiiShader" : "Shaders/Pipeline/Passes/Lighting/ClusteredLocalSurfaceLighting.xiiShader");
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(pipeline, shader));
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiSurfaceLightingPassUtils::Inputs      m_Surface;
    xiiRenderGraphTextureHandle              m_Output, m_SpotAtlas, m_Ids, m_Reactive, m_Current, m_Previous, m_Velocity;
    xiiRenderGraphBufferHandle               m_SpotConstants, m_Receivers, m_Constants, m_Layout, m_Lights, m_Counts, m_Candidates, m_Profiles, m_Grid, m_Indices, m_Overflow, m_ReservoirConstants;
    xiiSurfaceLightingConstants              m_Snapshot;
    xiiLightClusterConstants                 m_ClusterSnapshot;
    xiiLocalLightReservoirConstants          m_ReservoirSnapshot;
    xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
    xiiSizeU32                               m_Dispatch;
    bool                                     m_bSeed = false;
  };
  xiiRenderGraphTextureHandle currentReservoirs;
  const auto                  setup = [&](Data& data, xiiRenderGraphBuilder& builder, bool seed) {
    data.m_bSeed    = seed;
    data.m_Dispatch = seed ? tiles : surface.m_Size;
    data.m_Surface  = xiiSurfaceLightingPassUtils::Read(surface, builder);
    data.m_Pipeline = seed ? m_SeedPipeline : pipeline;
    if (!seed)
      data.m_Output = xiiSurfaceLightingPassUtils::Output(builder, surface.m_Size);
    if ((shadows && !seed) || useReservoirs)
      data.m_Ids = builder.ReadTexture(ids, xiiGALResourceStateFlags::ShaderResource);
    if (shadows && !seed)
    {
      xiiGALTextureCreationDescription reactive;
      reactive.m_Type      = xiiGALResourceDimension::Texture2D;
      reactive.m_Size      = surface.m_Size;
      reactive.m_Format    = xiiGALResourceFormat::RGBA16Float;
      reactive.m_BindFlags = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
      data.m_Reactive      = builder.WriteTexture("Spotlight Reactive Radiance", reactive, xiiGALResourceStateFlags::UnorderedAccess);
      data.m_SpotAtlas     = builder.ReadTexture(atlas, xiiGALResourceStateFlags::ShaderResource);
      data.m_SpotConstants = builder.ReadBuffer(shadows->m_hConstants, xiiGALResourceStateFlags::ConstantBuffer);
      data.m_Receivers     = builder.ReadBuffer(shadows->m_hReceivers, xiiGALResourceStateFlags::ShaderResource);
    }
    if (useReservoirs)
    {
      data.m_ReservoirSnapshot  = reservoirSettings;
      data.m_ReservoirConstants = xiiRenderPassUtils::Constants(builder, "Local Light Reservoir Settings", sizeof(xiiLocalLightReservoirConstants));
      if (seed)
      {
        xiiGALTextureCreationDescription image;
        image.m_Type      = xiiGALResourceDimension::Texture2D;
        image.m_Size      = reservoirSize;
        image.m_Format    = xiiGALResourceFormat::RGBA32UInt;
        image.m_BindFlags = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
        data.m_Current    = builder.WriteTexture("Current Local Light Reservoirs", image, xiiGALResourceStateFlags::UnorderedAccess);
      }
      else
      {
        data.m_Current  = builder.ReadTexture(currentReservoirs, xiiGALResourceStateFlags::ShaderResource);
        data.m_Previous = previousReservoirs.IsValid() ? builder.ReadTexture(previousReservoirs, xiiGALResourceStateFlags::ShaderResource) : data.m_Current;
        data.m_Velocity = builder.ReadTexture(velocity, xiiGALResourceStateFlags::ShaderResource);
      }
    }
    data.m_Snapshot.SurfaceExtent     = xiiVec4U32(surface.m_Size.width, surface.m_Size.height, 0, 0);
    data.m_Snapshot.SurfaceEvaluation = xiiVec4(m_fScale, 0, float(mode), 0);
    data.m_Constants                  = xiiRenderPassUtils::Constants(builder, "Surface Evaluation", sizeof(xiiSurfaceLightingConstants));
    data.m_ClusterSnapshot            = layout->m_Constants;
    data.m_Layout                     = xiiRenderPassUtils::Constants(builder, "Cluster Layout", sizeof(xiiLightClusterConstants));
    data.m_Lights                     = builder.ReadBuffer(lights, xiiGALResourceStateFlags::ShaderResource);
    data.m_Counts                     = builder.ReadBuffer(counts, xiiGALResourceStateFlags::ShaderResource);
    if (!seed)
      data.m_Candidates = builder.ReadBuffer(candidates, xiiGALResourceStateFlags::ShaderResource);
    data.m_Profiles = builder.ReadBuffer(profiles, xiiGALResourceStateFlags::ShaderResource);
    data.m_Grid     = builder.ReadBuffer(grid, xiiGALResourceStateFlags::ShaderResource);
    data.m_Indices  = builder.ReadBuffer(indices, xiiGALResourceStateFlags::ShaderResource);
    data.m_Overflow = builder.ReadBuffer(overflow, xiiGALResourceStateFlags::ShaderResource);
  };
  const auto record = [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands = execute.GetCommandList();
    commands.SetPipelineState(data.m_Pipeline.Borrow());
    xiiSurfaceLightingPassUtils::Bind(data.m_Surface, execute);
    xiiSurfaceLightingPassUtils::BindConstants(execute, data.m_Constants, data.m_Snapshot);
    xiiLightClusterPassUtils::Bind(execute, data.m_Layout, data.m_ClusterSnapshot, data.m_Pipeline.Borrow());
    const auto bindBuffer = [&](xiiTempHashedString name, xiiRenderGraphBufferHandle buffer) {
      commands.ResolveAndSetShaderResourceBufferView(name, execute.GetBuffer(buffer)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    };
    const auto bindTexture = [&](xiiTempHashedString name, xiiRenderGraphTextureHandle texture) {
      commands.ResolveAndSetShaderResourceTextureView(name, execute.GetTexture(texture)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    };
    const auto bindOutput = [&](xiiTempHashedString name, xiiRenderGraphTextureHandle texture) {
      commands.ResolveAndSetUnorderedAccessTextureView(name, execute.GetTexture(texture)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    };
    bindBuffer("g_Lights", data.m_Lights);
    bindBuffer("g_Counts", data.m_Counts);
    if (!data.m_bSeed)
    {
      bindBuffer("g_Candidates", data.m_Candidates);
      bindOutput("g_Lighting", data.m_Output);
    }
    bindBuffer("g_IESProfiles", data.m_Profiles);
    bindBuffer("g_LightGrid", data.m_Grid);
    bindBuffer("g_LightIndices", data.m_Indices);
    bindBuffer("g_Overflow", data.m_Overflow);
    if (data.m_SpotAtlas.IsValid())
    {
      commands.ResolveAndSetConstantBuffer("xiiSpotShadowConstants", execute.GetBuffer(data.m_SpotConstants), xiiGALShaderType::Compute);
      bindTexture("g_SpotShadowAtlas", data.m_SpotAtlas);
      bindTexture("g_SpotObjectIds", data.m_Ids);
      bindBuffer("g_SpotReceiverFlags", data.m_Receivers);
      bindOutput("g_SpotReactive", data.m_Reactive);
    }
    if (data.m_ReservoirConstants.IsValid())
    {
      auto* buffer = execute.GetBuffer(data.m_ReservoirConstants);
      {
        xiiGALMapHelper<xiiLocalLightReservoirConstants> mapped(commands, buffer, xiiGALMapType::Write, xiiGALMapFlags::Discard);
        *mapped = data.m_ReservoirSnapshot;
      }
      xiiGALStateTransitionDescription transition;
      transition.m_pResource       = buffer;
      transition.m_NewState        = xiiGALResourceStateFlags::ConstantBuffer;
      transition.m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState;
      commands.TransitionResourceStates(xiiMakeArrayPtr(&transition, 1));
      commands.ResolveAndSetConstantBuffer("xiiLocalLightReservoirConstants", buffer, xiiGALShaderType::Compute);
      bindTexture("g_ReservoirObject", data.m_Ids);
      if (data.m_bSeed)
        bindOutput("g_SeedReservoirs", data.m_Current);
      else
      {
        bindTexture("g_CurrentReservoirs", data.m_Current);
        bindTexture("g_PreviousReservoirs", data.m_Previous);
        bindTexture("g_ReservoirVelocity", data.m_Velocity);
      }
    }
    if (commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).Failed())
    {
      execute.ReportFailure("Could not bind clustered local lighting resources.");
      return;
    }
    commands.DispatchCompute({(data.m_Dispatch.width + 7U) / 8U, (data.m_Dispatch.height + 7U) / 8U, 1});
  };
  const auto queue = m_bAsyncCompute ? xiiGALCommandQueueFlags::Compute : xiiGALCommandQueueFlags::Graphics;
  if (useReservoirs)
  {
    const auto seedSetup = [&](Data& data, xiiRenderGraphBuilder& builder) {
      setup(data, builder, true);
    };
    const auto seed   = context.m_Graph.AddPass<Data>("Seed Local Light Reservoirs", queue, seedSetup, record);
    currentReservoirs = seed.first->m_Current;
    XII_SUCCEED_OR_RETURN(context.SetOutput(m_Reservoirs, currentReservoirs));
  }
  const auto resolveSetup = [&](Data& data, xiiRenderGraphBuilder& builder) {
    setup(data, builder, false);
  };
  const auto name = useReservoirs ? (previousReservoirs.IsValid() ? "Resolve Spatial and Temporal Local Light Reservoirs" : "Resolve Spatial Local Light Reservoirs") : context.m_sPassName;
  const auto pass = context.m_Graph.AddPass<Data>(name, queue, resolveSetup, record);
  if (shadows)
    XII_SUCCEED_OR_RETURN(context.SetOutput(m_Reactive, pass.first->m_Reactive));
  return context.SetOutput(m_Output, pass.first->m_Output);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Lighting_ClusteredLocalSurfaceLightingPass);
