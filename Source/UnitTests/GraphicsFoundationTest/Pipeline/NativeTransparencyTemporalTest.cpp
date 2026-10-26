/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Pipeline/Passes/Temporal/TemporalAntiAliasingPass.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsFoundationTest/Pipeline/PipelineTestPasses.h>
#include <GraphicsFoundationTest/TestingEnvironment/RenderGraphTestContext.h>
#include <GraphicsFoundationTest/TestingEnvironment/TextureReadback.h>

class xiiTransparencyTrustTestSource final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTransparencyTrustTestSource, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureOutputPin m_Color{xiiGALResourceFormat::RGBA32Float};
  xiiRenderPipelineTextureOutputPin m_Velocity{xiiGALResourceFormat::RG16Float};
  xiiRenderPipelineTextureOutputPin m_Depth{xiiGALResourceFormat::R32Float};
  xiiRenderPipelineTextureOutputPin m_Trust{xiiGALResourceFormat::R32Float, true};
  xiiResult                         Build(xiiRenderPipelinePassContext& context) override
  {
    const auto*     parameter = context.m_Parameters.GetValue("Transparency.Phase");
    const xiiUInt32 phase     = parameter ? parameter->ConvertTo<xiiUInt32>() : 0;
    struct Data
    {
      xiiRenderGraphTextureHandle m_Images[4];
      xiiDynamicArray<xiiVec4>    m_Color;
      xiiDynamicArray<xiiUInt32>  m_Velocity;
      xiiDynamicArray<float>      m_Depth, m_Trust;
    };
    const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Transfer, [&](Data& data, xiiRenderGraphBuilder& builder) {
      const xiiGALResourceFormat::Enum formats[]={xiiGALResourceFormat::RGBA32Float,xiiGALResourceFormat::RG16Float,xiiGALResourceFormat::R32Float,xiiGALResourceFormat::R32Float};
      for(xiiUInt32 i=0;i<4;++i)
      {
        if(i==3 && phase==2)continue;
        xiiGALTextureCreationDescription image;image.m_Type=xiiGALResourceDimension::Texture2D;image.m_Size={9,11};image.m_Format=formats[i];image.m_BindFlags=xiiGALBindFlags::ShaderResource;
        xiiStringBuilder name;name.SetFormat("Known temporal input {}",i);data.m_Images[i]=builder.WriteTexture(name,image,xiiGALResourceStateFlags::CopyDestination);
      }
      data.m_Color.SetCount(99);data.m_Velocity.SetCount(99);data.m_Depth.SetCount(99);data.m_Trust.SetCount(99);
      for(xiiUInt32 pixel=0;pixel<99;++pixel)
      {
        data.m_Color[pixel]=((pixel%9+phase)&1U)?xiiVec4(0.1f,0.2f,0.8f,1):xiiVec4(0.8f,0.2f,0.1f,1);
        data.m_Velocity[pixel]=0;data.m_Depth[pixel]=0.5f;data.m_Trust[pixel]=phase==1?0.5f:1.0f;
      } }, [](const Data& data, xiiRenderGraphPassContext& execute) {
      const auto upload=[&](xiiUInt32 index,const void* pixels,xiiUInt32 stride){
        if(!data.m_Images[index].IsValid())return;
        const auto alignedStride=(stride+15U)&~15U;
        xiiDynamicArray<xiiUInt8> padded;padded.SetCount(alignedStride*11);
        for(xiiUInt32 row=0;row<11;++row)xiiMemoryUtils::Copy(padded.GetData()+row*alignedStride,static_cast<const xiiUInt8*>(pixels)+row*stride,stride);
        xiiGALTextureSubResourceData source;source.m_pData=xiiConstByteBlobPtr(padded.GetData(),padded.GetCount());source.m_uiStride=alignedStride;
        execute.GetCommandList().UpdateTexture(execute.GetTexture(data.m_Images[index]),{},xiiBoundingBoxU32::MakeFromMinMax({0,0,0},{9,11,1}),source);
      };
      upload(0,data.m_Color.GetData(),9*sizeof(xiiVec4));upload(1,data.m_Velocity.GetData(),9*sizeof(xiiUInt32));upload(2,data.m_Depth.GetData(),9*sizeof(float));upload(3,data.m_Trust.GetData(),9*sizeof(float)); });
    XII_SUCCEED_OR_RETURN(context.SetOutput(m_Color, emitted.first->m_Images[0]));
    XII_SUCCEED_OR_RETURN(context.SetOutput(m_Velocity, emitted.first->m_Images[1]));
    XII_SUCCEED_OR_RETURN(context.SetOutput(m_Depth, emitted.first->m_Images[2]));
    if (phase != 2)
      XII_SUCCEED_OR_RETURN(context.SetOutput(m_Trust, emitted.first->m_Images[3]));
    return XII_SUCCESS;
  }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTransparencyTrustTestSource, 1, xiiRTTIDefaultAllocator<xiiTransparencyTrustTestSource>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Color", m_Color)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("Velocity", m_Velocity)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("Depth", m_Depth)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("Trust", m_Trust)->AddFlags(xiiPropertyFlags::ReadOnly),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_CREATE_SIMPLE_TEST(NativeTransparency, TemporalCoverageAndDisappearedLayerRejection)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false, false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiRenderPipelineResourceDescriptor definition;
  const auto                          source  = definition.AddPass("Alternating real GPU colors", xiiGetStaticRTTI<xiiTransparencyTrustTestSource>());
  const auto                          resolve = definition.AddPass("Temporal reconstruction", xiiGetStaticRTTI<xiiTemporalAntiAliasingPass>());
  XII_TEST_RESULT(definition.Connect(source, "Color", resolve, "HDRSceneColor"));
  XII_TEST_RESULT(definition.Connect(source, "Velocity", resolve, "DilatedVelocityBuffer"));
  XII_TEST_RESULT(definition.Connect(source, "Depth", resolve, "SceneDepth"));
  XII_TEST_RESULT(definition.Connect(source, "Trust", resolve, "HistoryTrust"));
  for (const auto node : {source, resolve})
  {
    const auto sink = definition.AddPass(node == source ? "Current color" : "Reconstructed color", xiiGetStaticRTTI<xiiOpaqueTestExport>());
    XII_TEST_RESULT(definition.Connect(node, node == source ? "Color" : "TAAResolved", sink, "Input"));
    definition.m_Interface.PushBack({node == source ? "Current" : "Output", false, definition.m_Passes[sink].m_Id, "Output"});
  }
  const auto                resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Transparency.TemporalCoverage", std::move(definition));
  xiiRenderPipelineInstance instance;
  xiiRenderGraphTestContext context;
  if (!XII_TEST_RESULT(instance.Synchronize(resource)) || !XII_TEST_RESULT(context.Initialize("Temporal transparent trust")))
    return;
  xiiDynamicArray<xiiVec4> previous;
  for (xiiUInt32 phase = 0; phase < 4; ++phase)
  {
    xiiRenderGraphFrame frame;
    if (!XII_TEST_RESULT(frame.Begin()))
      return;
    auto& graph = context.GetGraph();
    auto& cache = context.GetResourceCache();
    cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
    XII_SCOPE_EXIT(cache.EndFrame(); frame.End());
    graph.BeginSetup(frame.GetFrameIndex());
    xiiVariantDictionary parameters;
    parameters.Insert("Transparency.Phase", phase);
    if (!XII_TEST_RESULT_MSG(instance.Build(nullptr, graph, context.GetBlackboard(), parameters), "%s", instance.GetLastBuildError().GetStartPointer()))
      return;
    const auto current  = instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface("Current", false))->m_hTexture;
    const auto resolved = instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface("Output", false))->m_hTexture;
    graph.EndSetup();
    xiiStringBuilder error;
    if (!XII_TEST_RESULT_MSG(graph.Compile({}, &error), "%s", error.GetData()) || !XII_TEST_RESULT_MSG(graph.Execute(environment.GetDevice(), nullptr, &context.GetBlackboard(), &cache, nullptr, &error), "%s", error.GetData()))
      return;
    xiiDynamicArray<xiiVec4> raw, output;
    if (!xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), graph.GetExportedTexture(current), raw) || !xiiGpuTestReadback::ReadTexturePixels(*environment.GetDevice(), graph.GetExportedTexture(resolved), output))
      return;
    for (xiiUInt32 pixel = 0; pixel < output.GetCount(); ++pixel)
      if (phase < 3)
        XII_TEST_BOOL(output[pixel] == raw[pixel]);
      else
        for (xiiUInt32 channel = 0; channel < 3; ++channel)
          XII_TEST_FLOAT(output[pixel].GetData()[channel], previous[pixel].GetData()[channel] * 0.92f + raw[pixel].GetData()[channel] * 0.08f, 0.00001f);
    if (phase == 3)
      XII_TEST_BOOL(output != raw);
    previous = output;
  }
}
