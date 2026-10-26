/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Pipeline/Passes/Reflections/RadiancePyramidPass.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Tools/TextureReadback.h>
#include <GraphicsFoundationTest/Pipeline/PipelineTestPasses.h>
#include <GraphicsFoundationTest/TestingEnvironment/RenderGraphTestContext.h>

static xiiVec4 PyramidGradient(xiiUInt32 x, xiiUInt32 y, xiiSizeU32 size)
{
  return {float(x) + (x + 1 == size.width ? 10.0f : 0), float(y) + (y + 1 == size.height ? 20.0f : 0), float((x + y * 3) % 7), 1};
}
class xiiRadianceGradientSource : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRadianceGradientSource, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureOutputPin m_Color{xiiGALResourceFormat::RGBA32Float};
  xiiResult                         Build(xiiRenderPipelinePassContext& context) override
  {
    const auto*      width  = context.m_Parameters.GetValue("Native.Width");
    const auto*      height = context.m_Parameters.GetValue("Native.Height");
    const xiiSizeU32 size{width ? width->ConvertTo<xiiUInt32>() : 7, height ? height->ConvertTo<xiiUInt32>() : 5};
    struct Data
    {
      xiiRenderGraphTextureHandle m_Color;
      xiiSizeU32                  m_Size;
      xiiDynamicArray<xiiVec4>    m_Pixels;
    };
    const auto pass = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Transfer, [&](Data& data, xiiRenderGraphBuilder& builder) {
      xiiGALTextureCreationDescription d;d.m_Type=xiiGALResourceDimension::Texture2D;d.m_Size=size;d.m_Format=xiiGALResourceFormat::RGBA32Float;d.m_BindFlags=xiiGALBindFlags::ShaderResource;
      data.m_Color=builder.WriteTexture("Odd HDR gradient",d,xiiGALResourceStateFlags::CopyDestination);data.m_Size=size;
      data.m_Pixels.SetCount(size.width*size.height);
      for(xiiUInt32 y=0;y<size.height;++y)for(xiiUInt32 x=0;x<size.width;++x)data.m_Pixels[y*size.width+x]=PyramidGradient(x,y,size); }, [](const Data& data, xiiRenderGraphPassContext& execute) {
      xiiGALTextureSubResourceData upload;upload.m_pData=xiiConstByteBlobPtr(reinterpret_cast<const xiiUInt8*>(data.m_Pixels.GetData()),data.m_Pixels.GetCount()*sizeof(xiiVec4));upload.m_uiStride=data.m_Size.width*sizeof(xiiVec4);
      execute.GetCommandList().UpdateTexture(execute.GetTexture(data.m_Color),{},xiiBoundingBoxU32::MakeFromMinMax({0,0,0},{data.m_Size.width,data.m_Size.height,1}),upload); });
    return context.SetOutput(m_Color, pass.first->m_Color);
  }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRadianceGradientSource, 1, xiiRTTIDefaultAllocator<xiiRadianceGradientSource>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Color", m_Color)->AddFlags(xiiPropertyFlags::ReadOnly),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_CREATE_SIMPLE_TEST_GROUP(NativeRadiancePyramid);
XII_CREATE_SIMPLE_TEST(NativeRadiancePyramid, OddAreaWeightedMipsAndOwnerRetirement)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false, false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiRenderPipelineResourceDescriptor descriptor;
  const auto                          source  = descriptor.AddPass("Linear HDR gradient", xiiGetStaticRTTI<xiiRadianceGradientSource>());
  const auto                          pyramid = descriptor.AddPass("HDR footprint integration", xiiGetStaticRTTI<xiiRadiancePyramidPass>());
  const auto                          sink    = descriptor.AddPass("Read HDR hierarchy", xiiGetStaticRTTI<xiiOpaqueTestExport>());
  XII_TEST_RESULT(descriptor.Connect(source, "Color", pyramid, "Color"));
  XII_TEST_RESULT(descriptor.Connect(pyramid, "RadiancePyramid", sink, "Input"));
  descriptor.m_Interface.PushBack({"Pyramid", false, descriptor.m_Passes[sink].m_Id, "Output"});
  const auto                resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("RadiancePyramid.OddFootprints", std::move(descriptor));
  xiiRenderPipelineInstance instance;
  if (!XII_TEST_RESULT(instance.Synchronize(resource)))
    return;
  xiiRenderGraphTestContext context;
  if (!XII_TEST_RESULT(context.Initialize("Linear HDR mip readbacks")))
    return;
  for (const xiiSizeU32 initial : {xiiSizeU32{7, 5}, xiiSizeU32{65, 33}, xiiSizeU32{1, 9}, xiiSizeU32{9, 1}, xiiSizeU32{1, 1}})
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
    parameters.Insert("Native.Width", initial.width);
    parameters.Insert("Native.Height", initial.height);
    if (!XII_TEST_RESULT(instance.Build(nullptr, graph, context.GetBlackboard(), parameters)))
      return;
    const auto output = instance.GetInterfaceBinding(instance.GetPipeline()->FindInterface("Pyramid", false))->m_hTexture;
    graph.EndSetup();
    if (initial.width == 1 && initial.height == 1)
      instance.Clear();
    xiiStringBuilder error;
    if (!XII_TEST_RESULT_MSG(graph.Compile({}, &error), "%s", error.GetData()) || !XII_TEST_RESULT_MSG(graph.Execute(environment.GetDevice(), nullptr, &context.GetBlackboard(), &cache, nullptr, &error), "%s", error.GetData()))
      return;
    const auto image = graph.GetExportedTexture(output);
    if (!XII_TEST_BOOL(image != nullptr))
      return;
    xiiDynamicArray<xiiVec4> expected;
    expected.SetCount(initial.width * initial.height);
    for (xiiUInt32 y = 0; y < initial.height; ++y)
      for (xiiUInt32 x = 0; x < initial.width; ++x)
        expected[y * initial.width + x] = PyramidGradient(x, y, initial);
    auto                  size = initial;
    xiiGALTextureReadback readback(environment.GetDevice());
    auto                  commands = environment.GetDevice()->CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
    for (xiiUInt32 mip = 0; mip < image->GetDescription().m_uiMipLevels; ++mip)
    {
      commands->Begin();
      const auto state = image->GetResourceState();
      if (!XII_TEST_RESULT(readback.Enqueue(commands, {.m_pTexture = image.Borrow(), .m_uiMipLevel = mip})))
        return;
      xiiGALStateTransitionDescription transition;
      transition.m_pResource       = image.Borrow();
      transition.m_NewState        = state;
      transition.m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState;
      commands->TransitionResourceStates(xiiMakeArrayPtr(&transition, 1));
      commands->End();
      environment.GetDevice()->GetCommandQueue()->WaitForFenceValue(environment.GetDevice()->GetCommandQueue()->Submit(commands));
      readback.WaitForNextCompleted();
      const auto capture = readback.GetCompleted();
      if (!XII_TEST_BOOL(static_cast<bool>(capture)))
        return;
      XII_SCOPE_EXIT(readback.RecycleStagingTexture(capture.m_pStagingTexture));
      commands->Begin();
      xiiGALMappedTextureSubresource mapped;
      if (!XII_TEST_RESULT(commands->MapTextureSubresource(capture.m_pStagingTexture, {}, xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, nullptr, mapped)))
        return;
      XII_TEST_INT(capture.m_uiWidth, size.width);
      XII_TEST_INT(capture.m_uiHeight, size.height);
      for (xiiUInt32 y = 0; y < size.height; ++y)
      {
        const auto* row = xiiMemoryUtils::AddByteOffset(static_cast<const xiiVec4*>(mapped.m_pData), y * mapped.m_uiStride);
        for (xiiUInt32 x = 0; x < size.width; ++x)
          for (xiiUInt32 channel = 0; channel < 4; ++channel)
            XII_TEST_FLOAT(row[x].GetData()[channel], expected[y * size.width + x].GetData()[channel], 0.0001f);
      }
      commands->UnmapTextureSubresource(capture.m_pStagingTexture, {}).AssertSuccess();
      commands->End();
      commands->Reset();
      const xiiSizeU32         next{xiiMath::Max(size.width >> 1U, 1U), xiiMath::Max(size.height >> 1U, 1U)};
      xiiDynamicArray<xiiVec4> reduced;
      reduced.SetCount(next.width * next.height);
      for (xiiUInt32 y = 0; y < next.height; ++y)
        for (xiiUInt32 x = 0; x < next.width; ++x)
        {
          const double left = double(x) * size.width / next.width, right = double(x + 1) * size.width / next.width, top = double(y) * size.height / next.height, bottom = double(y + 1) * size.height / next.height;
          auto         value = xiiVec4::MakeZero();
          for (xiiUInt32 sy = xiiUInt32(top); sy < xiiUInt32(xiiMath::Ceil(bottom)); ++sy)
            for (xiiUInt32 sx = xiiUInt32(left); sx < xiiUInt32(xiiMath::Ceil(right)); ++sx)
            {
              const double weight = (xiiMath::Min(right, double(sx + 1)) - xiiMath::Max(left, double(sx))) * (xiiMath::Min(bottom, double(sy + 1)) - xiiMath::Max(top, double(sy)));
              value += expected[sy * size.width + sx] * float(weight);
            }
          reduced[y * next.width + x] = value / float((right - left) * (bottom - top));
        }
      expected = std::move(reduced);
      size     = next;
    }
  }
}
