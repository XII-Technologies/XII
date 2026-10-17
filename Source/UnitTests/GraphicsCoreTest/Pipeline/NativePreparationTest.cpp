/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <Foundation/IO/MemoryStream.h>
#include <GraphicsCore/Pipeline/Passes/Geometry/DepthRasterPass.h>
#include <GraphicsCore/Pipeline/Passes/Geometry/GBufferRasterPass.h>
#include <GraphicsCore/Pipeline/Passes/Geometry/MatrixRasterConstantsPass.h>
#include <GraphicsCore/Pipeline/Passes/Geometry/MeshDrawCommandsPass.h>
#include <GraphicsCore/Pipeline/Passes/Geometry/SurfaceRasterConstantsPass.h>
#include <GraphicsCore/Pipeline/Passes/Geometry/VertexPullDrawCommandsPass.h>
#include <GraphicsCore/Pipeline/Passes/Preparation/WorldInstancesUploadPass.h>
#include <GraphicsCore/Pipeline/Passes/Residency/GeometryUploadPass.h>
#include <GraphicsCore/Pipeline/Passes/Residency/MaterialUploadPass.h>
#include <GraphicsCore/Pipeline/Passes/Streaming/GeometryLodFeedbackPass.h>
#include <GraphicsCore/Pipeline/Passes/Streaming/GeometryLodFeedbackReadbackPass.h>
#include <GraphicsCore/Pipeline/Passes/Streaming/GeometryLodFeedbackResetPass.h>
#include <GraphicsCore/Pipeline/Passes/Temporal/BufferHistoryPass.h>
#include <GraphicsCore/Pipeline/Passes/Temporal/TextureHistoryPass.h>
#include <GraphicsCore/Pipeline/Passes/Utility/CounterResetPass.h>
#include <GraphicsCore/Pipeline/Passes/Utility/DepthRegionClearPass.h>
#include <GraphicsCore/Pipeline/Passes/Utility/DepthTargetPass.h>
#include <GraphicsCore/Pipeline/Passes/Visibility/DepthPyramidPass.h>
#include <GraphicsCore/Pipeline/Passes/Visibility/InstanceCullPass.h>
#include <GraphicsCore/Pipeline/Passes/Visibility/InstanceOcclusionPass.h>
#include <GraphicsCore/Pipeline/Passes/Visibility/LodHistoryResetPass.h>
#include <GraphicsCore/Pipeline/Passes/Visibility/MatrixVisibilityViewPass.h>
#include <GraphicsCore/Pipeline/Passes/Visibility/MeshletCullPass.h>
#include <GraphicsCore/Pipeline/Passes/Visibility/MeshletDispatchBoundsPass.h>
#include <GraphicsCore/Pipeline/Passes/Visibility/MeshletDispatchPass.h>
#include <GraphicsCore/Pipeline/Passes/Visibility/MeshletOcclusionPass.h>
#include <GraphicsCore/Pipeline/Passes/Visibility/VisibilityViewPass.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

XII_CREATE_SIMPLE_TEST_GROUP(NativePreparation);
XII_CREATE_SIMPLE_TEST(NativePreparation, ReflectedDataProducers)
{
  xiiWorldInstancesUploadPass instances;
  xiiGeometryUploadPass       geometry;
  xiiMaterialUploadPass       materials;
  xiiVisibilityViewPass       visibility;
  for (xiiRenderPipelinePass* pass : {static_cast<xiiRenderPipelinePass*>(&instances), static_cast<xiiRenderPipelinePass*>(&geometry), static_cast<xiiRenderPipelinePass*>(&materials), static_cast<xiiRenderPipelinePass*>(&visibility)})
  {
    xiiDynamicArray<xiiRenderPipelinePin> pins;
    pass->GetPins(pins);
    XII_TEST_RESULT(pass->InitializePinBindings(pins));
    XII_TEST_BOOL(pass->GetDynamicRTTI()->FindPropertyByName("Stage") == nullptr);
    for (const auto& pin : pins)
      if (pin.m_Type == xiiRenderPipelinePin::Buffer)
        XII_TEST_BOOL((!pin.m_bInput || pin.m_bOptional) && pin.m_Constraints.m_RequiredBindFlags.IsSet(xiiGALBindFlags::ShaderResource));
  }
  XII_TEST_BOOL(instances.GetExtractionRequirements() == xiiRenderExtractionCategory::Geometry);
  XII_TEST_BOOL(geometry.GetExtractionRequirements().IsNoFlagSet() && materials.GetExtractionRequirements().IsNoFlagSet());
  visibility.m_uiVisibilityMask = 0x42;
  visibility.m_fLodScreenScale  = 2;
  xiiVariantDictionary properties;
  XII_TEST_RESULT(visibility.GetProperties(properties));
  XII_TEST_BOOL(!properties.Contains("VisibilityView") && !properties.Contains("Candidates"));
  xiiVisibilityViewPass copy;
  XII_TEST_RESULT(copy.Configure(properties));
  XII_TEST_INT(copy.m_uiVisibilityMask, 0x42);
  XII_TEST_FLOAT(copy.m_fLodScreenScale, 2, 0);
  properties.Insert("LodHysteresis", 1.0f);
  XII_TEST_BOOL(copy.Configure(properties).Failed());
  xiiRenderPipelineResourceDescriptor leaf;
  leaf.AddPass("Instances", "xiiWorldInstancesUploadPass");
  leaf.AddPass("Geometry", "xiiGeometryUploadPass");
  leaf.AddPass("Materials", "xiiMaterialUploadPass");
  xiiGraphDefinition definition;
  XII_TEST_RESULT(leaf.MakeGraphDefinition(definition));
  xiiRenderPipelineResourceDescriptor root;
  root.m_Definitions.PushBack(definition);
  root.AddSubgraph("First", definition.m_Id);
  root.AddSubgraph("Second", definition.m_Id);
  xiiSharedPtr<const xiiRenderPipeline> compiled;
  XII_TEST_RESULT(xiiRenderPipeline::Compile(root, compiled));
  if (!compiled)
    return;
  XII_TEST_INT(compiled->GetExecutionOrder().GetCount(), 6);
  XII_TEST_BOOL(compiled->GetExtractionRequirements() == xiiRenderExtractionCategory::Geometry);
  XII_TEST_BOOL(compiled->GetRuntimeFeatures() == xiiRenderPipelineFeature::GpuScene);
  xiiContiguousMemoryStreamStorage bytes;
  xiiMemoryStreamWriter            writer(&bytes);
  XII_TEST_RESULT(root.Serialize(writer));
  xiiMemoryStreamReader               reader(&bytes);
  xiiRenderPipelineResourceDescriptor restored;
  XII_TEST_RESULT(restored.Deserialize(reader));
  xiiSharedPtr<const xiiRenderPipeline> repeated;
  XII_TEST_RESULT(xiiRenderPipeline::Compile(restored, repeated));
  if (repeated)
    for (xiiUInt32 i = 0; i < compiled->GetDescriptor().m_Passes.GetCount(); ++i)
      XII_TEST_BOOL(compiled->GetDescriptor().m_Passes[i].m_Id == repeated->GetDescriptor().m_Passes[i].m_Id);
}

XII_CREATE_SIMPLE_TEST(NativePreparation, IndependentCounterConfiguration)
{
  xiiCounterResetPass counter;
  counter.m_uiValue = 17;
  xiiVariantDictionary properties;
  XII_TEST_RESULT(counter.GetProperties(properties));
  XII_TEST_INT(properties["Value"].Get<xiiUInt32>(), 17);
  XII_TEST_BOOL(!properties.Contains("Counter"));
  xiiCounterResetPass copy;
  XII_TEST_RESULT(copy.Configure(properties));
  XII_TEST_INT(copy.m_uiValue, 17);
  XII_TEST_BOOL(copy.GetRuntimeFeatures().IsNoFlagSet() && copy.GetExtractionRequirements().IsNoFlagSet());
  xiiDynamicArray<xiiRenderPipelinePin> pins;
  copy.GetPins(pins);
  XII_TEST_RESULT(copy.InitializePinBindings(pins));
  const auto& pin = pins[copy.m_Counter.GetBindingIndex()];
  XII_TEST_BOOL(!pin.m_bInput && pin.m_Type == xiiRenderPipelinePin::Buffer);
  XII_TEST_BOOL(pin.m_Constraints.m_RequiredBindFlags.IsSet(xiiGALBindFlags::IndirectDrawArguments));
}

XII_CREATE_SIMPLE_TEST(NativePreparation, ExplicitLodHistoryTopology)
{
  xiiInstanceCullPass    cull;
  xiiLodHistoryResetPass reset;
  xiiBufferHistoryPass   history;
  for (xiiRenderPipelinePass* pass : {static_cast<xiiRenderPipelinePass*>(&cull), static_cast<xiiRenderPipelinePass*>(&reset), static_cast<xiiRenderPipelinePass*>(&history)})
  {
    xiiDynamicArray<xiiRenderPipelinePin> pins;
    pass->GetPins(pins);
    XII_TEST_RESULT(pass->InitializePinBindings(pins));
    XII_TEST_BOOL(pass->GetExtractionRequirements().IsNoFlagSet() && pass->GetRuntimeFeatures().IsNoFlagSet());
    XII_TEST_BOOL(pass->GetDynamicRTTI()->FindPropertyByName("Stage") == nullptr);
  }
  xiiRenderPipelineResourceDescriptor definition;
  definition.AddPass("Cull supplied instances", "xiiInstanceCullPass");
  definition.AddPass("Retain LOD choices", "xiiBufferHistoryPass");
  XII_TEST_RESULT(definition.Connect(0, "LodHistory", 1, "Buffer"));
  XII_TEST_RESULT(definition.Connect(1, "PreviousBuffer", 0, "PreviousLodHistory"));
  xiiDynamicArray<xiiRenderPipelinePin> pins;
  cull.GetPins(pins);
  for (const auto& pin : pins)
    if (pin.m_bInput && pin.m_sName != "PreviousLodHistory" && pin.m_Type == xiiRenderPipelinePin::Buffer)
      definition.m_Interface.PushBack({pin.m_sName, true, definition.m_Passes[0].m_Id, pin.m_sName});
  xiiSharedPtr<const xiiRenderPipeline> compiled;
  XII_TEST_RESULT(xiiRenderPipeline::Compile(definition, compiled));
  if (!compiled)
    return;
  XII_TEST_INT(compiled->GetExecutionOrder().GetCount(), 2);
  auto intraFrame                        = definition;
  intraFrame.m_Connections[1].m_uiSource = 0;
  intraFrame.m_Connections[1].m_sOutput  = "LodHistory";
  XII_TEST_BOOL(xiiRenderPipeline::Compile(intraFrame, compiled).Failed());
}

XII_CREATE_SIMPLE_TEST(NativePreparation, ReflectedMeshletWorkPreparation)
{
  xiiMeshletDispatchBoundsPass bounds;
  xiiMeshletDispatchPass       dispatch;
  for (xiiRenderPipelinePass* pass : {static_cast<xiiRenderPipelinePass*>(&bounds), static_cast<xiiRenderPipelinePass*>(&dispatch)})
  {
    xiiDynamicArray<xiiRenderPipelinePin> pins;
    pass->GetPins(pins);
    XII_TEST_RESULT(pass->InitializePinBindings(pins));
    XII_TEST_BOOL(pass->GetExtractionRequirements().IsNoFlagSet() && pass->GetRuntimeFeatures().IsNoFlagSet());
    xiiVariantDictionary properties;
    XII_TEST_RESULT(pass->GetProperties(properties));
    XII_TEST_BOOL(properties.Contains("AsyncCompute") && !properties.Contains("VisibleInstances"));
  }
  xiiDynamicArray<xiiRenderPipelinePin> pins;
  dispatch.GetPins(pins);
  XII_TEST_RESULT(dispatch.InitializePinBindings(pins));
  XII_TEST_BOOL(pins[dispatch.m_Arguments.GetBindingIndex()].m_Constraints.m_RequiredBindFlags.IsSet(xiiGALBindFlags::IndirectDrawArguments));
  xiiRenderPipelineResourceDescriptor definition;
  definition.AddPass("Reduce selected LODs", "xiiMeshletDispatchBoundsPass");
  definition.AddPass("Prepare indirect work", "xiiMeshletDispatchPass");
  XII_TEST_RESULT(definition.Connect(0, "MaximumMeshletGroups", 1, "MaximumMeshletGroups"));
  for (xiiUInt32 index = 0; index < 2; ++index)
  {
    pins.Clear();
    (index == 0 ? static_cast<xiiRenderPipelinePass*>(&bounds) : static_cast<xiiRenderPipelinePass*>(&dispatch))->GetPins(pins);
    for (const auto& pin : pins)
      if (pin.m_bInput && pin.m_Type == xiiRenderPipelinePin::Buffer && !(index == 1 && pin.m_sName == "MaximumMeshletGroups"))
        definition.m_Interface.PushBack({pin.m_sName, true, definition.m_Passes[index].m_Id, pin.m_sName});
  }
  xiiSharedPtr<const xiiRenderPipeline> compiled;
  XII_TEST_RESULT(xiiRenderPipeline::Compile(definition, compiled));
  if (compiled)
    XII_TEST_INT(compiled->GetExecutionOrder().GetCount(), 2);
}

XII_CREATE_SIMPLE_TEST(NativePreparation, ReflectedMeshletCullingBudget)
{
  xiiMeshletCullPass   cull;
  xiiVariantDictionary properties;
  properties.Insert("MaxVisibleMeshlets", 16U);
  properties.Insert("AsyncCompute", false);
  XII_TEST_RESULT(cull.Configure(properties));
  XII_TEST_INT(cull.m_uiMaxVisibleMeshlets, 16);
  XII_TEST_BOOL(!cull.m_bAsyncCompute && cull.GetExtractionRequirements().IsNoFlagSet() && cull.GetRuntimeFeatures().IsNoFlagSet());
  properties.Insert("MaxVisibleMeshlets", 0U);
  XII_TEST_BOOL(cull.Configure(properties).Failed());
  xiiDynamicArray<xiiRenderPipelinePin> pins;
  cull.GetPins(pins);
  XII_TEST_RESULT(cull.InitializePinBindings(pins));
  XII_TEST_BOOL(pins[cull.m_Arguments.GetBindingIndex()].m_Constraints.m_RequiredBindFlags.IsSet(xiiGALBindFlags::IndirectDrawArguments));
  XII_TEST_BOOL(!pins[cull.m_Overflow.GetBindingIndex()].m_bInput && !pins[cull.m_Overflow.GetBindingIndex()].m_bOptional);
}

XII_CREATE_SIMPLE_TEST(NativePreparation, IndependentVertexPullCommands)
{
  xiiVertexPullDrawCommandsPass pass;
  xiiVariantDictionary          properties;
  properties.Insert("AsyncCompute", false);
  XII_TEST_RESULT(pass.Configure(properties));
  XII_TEST_BOOL(!pass.m_bAsyncCompute && pass.GetExtractionRequirements().IsNoFlagSet() && pass.GetRuntimeFeatures().IsNoFlagSet());
  xiiDynamicArray<xiiRenderPipelinePin> pins;
  pass.GetPins(pins);
  XII_TEST_RESULT(pass.InitializePinBindings(pins));
  XII_TEST_BOOL(pins[pass.m_Visible.GetBindingIndex()].m_bInput && !pins[pass.m_Visible.GetBindingIndex()].m_bOptional);
  XII_TEST_BOOL(pins[pass.m_Commands.GetBindingIndex()].m_Constraints.m_RequiredBindFlags.IsSet(xiiGALBindFlags::IndirectDrawArguments));
  XII_TEST_BOOL(pins[pass.m_CommandCount.GetBindingIndex()].m_Constraints.m_RequiredBindFlags.IsSet(xiiGALBindFlags::IndirectDrawArguments));
  XII_TEST_RESULT(pass.GetProperties(properties));
  XII_TEST_BOOL(properties.Contains("AsyncCompute") && !properties.Contains("DrawCommands"));
}

XII_CREATE_SIMPLE_TEST(NativePreparation, ExplicitSurfaceRasterConstants)
{
  xiiSurfaceRasterConstantsPass constants;
  xiiVariantDictionary          properties;
  XII_TEST_RESULT(constants.GetProperties(properties));
  XII_TEST_BOOL(!properties.Contains("RasterConstants"));
  properties.Insert("MeshGroupCountX", 1048576U);
  properties.Insert("MeshGroupCountY", 1U);
  XII_TEST_RESULT(constants.Configure(properties));
  properties.Insert("MeshGroupCountX", xiiMath::MaxValue<xiiUInt32>());
  properties.Insert("MeshGroupCountY", 2U);
  XII_TEST_BOOL(constants.Configure(properties).Failed());
  properties.Insert("MeshGroupCountX", 1U);
  properties.Insert("MeshGroupCountY", 1U);
  properties.Insert("VertexStride", 0U);
  XII_TEST_BOOL(constants.Configure(properties).Failed());
  properties.Insert("VertexStride", xiiUInt32(sizeof(xiiMeshPackedVertex)));
  properties.Insert("NormalOffset", xiiMath::MaxValue<xiiUInt32>());
  XII_TEST_BOOL(constants.Configure(properties).Failed());
  properties.Insert("NormalOffset", xiiUInt32(offsetof(xiiMeshPackedVertex, m_vNormal)));
  properties.Insert("VertexStride", xiiUInt32(sizeof(xiiMeshPackedVertex)) + 1U);
  XII_TEST_BOOL(constants.Configure(properties).Failed());
}

XII_CREATE_SIMPLE_TEST(NativePreparation, IndependentDepthRaster)
{
  xiiDepthRasterPass   depth;
  xiiVariantDictionary properties;
  XII_TEST_RESULT(depth.GetProperties(properties));
  XII_TEST_BOOL(!properties.Contains("SceneDepth") && properties.Contains("UseMeshShaders"));
  XII_TEST_BOOL(depth.GetExtractionRequirements().IsNoFlagSet() && depth.GetRuntimeFeatures().IsNoFlagSet());
  XII_TEST_BOOL(!depth.GetRequiredDeviceFeatures().IsSet(xiiRenderPipelineDeviceFeature::MeshShaders));
  depth.m_bUseMeshShaders = true;
  XII_TEST_BOOL(depth.GetRequiredDeviceFeatures().IsSet(xiiRenderPipelineDeviceFeature::MeshShaders));
  xiiDynamicArray<xiiRenderPipelinePin> pins;
  depth.GetPins(pins);
  XII_TEST_RESULT(depth.InitializePinBindings(pins));
  XII_TEST_BOOL(!pins[depth.m_OutputDepth.GetBindingIndex()].m_bOptional && pins[depth.m_OutputDepth.GetBindingIndex()].m_Format == xiiGALResourceFormat::D32Float);
  properties.Insert("FillMode", xiiInt64(xiiGALFillMode::Undefined));
  XII_TEST_BOOL(depth.Configure(properties).Failed());
}

XII_CREATE_SIMPLE_TEST(NativePreparation, IndependentMeshDrawCommands)
{
  xiiMeshDrawCommandsPass commands;
  xiiVariantDictionary    properties;
  properties.Insert("MeshGroupCountX", 4U);
  properties.Insert("MeshGroupCountY", 2U);
  XII_TEST_RESULT(commands.Configure(properties));
  XII_TEST_RESULT(commands.GetProperties(properties));
  XII_TEST_BOOL(!properties.Contains("DrawCommands"));
  XII_TEST_BOOL(commands.GetExtractionRequirements().IsNoFlagSet() && commands.GetRuntimeFeatures().IsNoFlagSet());
  properties.Insert("MeshGroupCountX", 0U);
  XII_TEST_BOOL(commands.Configure(properties).Failed());
  properties.Insert("MeshGroupCountX", xiiMath::MaxValue<xiiUInt32>());
  XII_TEST_BOOL(commands.Configure(properties).Failed());
  xiiDynamicArray<xiiRenderPipelinePin> pins;
  commands.GetPins(pins);
  XII_TEST_RESULT(commands.InitializePinBindings(pins));
  XII_TEST_BOOL(pins[commands.m_Commands.GetBindingIndex()].m_Constraints.m_RequiredBindFlags.IsSet(xiiGALBindFlags::IndirectDrawArguments));
}

XII_CREATE_SIMPLE_TEST(NativePreparation, IndependentMaterialRaster)
{
  xiiGBufferRasterPass pass;
  xiiVariantDictionary properties;
  XII_TEST_RESULT(pass.GetProperties(properties));
  XII_TEST_BOOL(properties.Contains("UseMeshShaders") && !properties.Contains("Albedo"));
  XII_TEST_BOOL(pass.GetExtractionRequirements().IsNoFlagSet() && pass.GetRuntimeFeatures().IsNoFlagSet());
  XII_TEST_BOOL(!pass.GetRequiredDeviceFeatures().IsSet(xiiRenderPipelineDeviceFeature::MeshShaders));
  pass.m_bUseMeshShaders = true;
  XII_TEST_BOOL(pass.GetRequiredDeviceFeatures().IsSet(xiiRenderPipelineDeviceFeature::MeshShaders));
  xiiDynamicArray<xiiRenderPipelinePin> pins;
  pass.GetPins(pins);
  XII_TEST_RESULT(pass.InitializePinBindings(pins));
  XII_TEST_BOOL(pins[pass.m_InputDepth.GetBindingIndex()].m_bInput && !pins[pass.m_InputDepth.GetBindingIndex()].m_bOptional);
  XII_TEST_BOOL(!pins[pass.m_OutputVelocity.GetBindingIndex()].m_bInput && pins[pass.m_OutputVelocity.GetBindingIndex()].m_Format == xiiGALResourceFormat::RG16Float);
  XII_TEST_BOOL(pins[pass.m_OutputObjectId.GetBindingIndex()].m_Format == xiiGALResourceFormat::R32UInt);
  properties.Insert("FillMode", xiiInt64(xiiGALFillMode::Undefined));
  XII_TEST_BOOL(pass.Configure(properties).Failed());
}

XII_CREATE_SIMPLE_TEST(NativePreparation, IndependentDepthPyramidAndTextureHistory)
{
  xiiDepthPyramidPass   pyramid;
  xiiTextureHistoryPass history;
  xiiVariantDictionary  properties;
  properties.Insert("AsyncCompute", false);
  XII_TEST_RESULT(pyramid.Configure(properties));
  XII_TEST_BOOL(!pyramid.m_bAsyncCompute && pyramid.GetRequiredDeviceFeatures().IsSet(xiiRenderPipelineDeviceFeature::Compute));
  XII_TEST_BOOL(pyramid.GetExtractionRequirements().IsNoFlagSet() && pyramid.GetRuntimeFeatures().IsNoFlagSet());
  xiiDynamicArray<xiiRenderPipelinePin> pins;
  pyramid.GetPins(pins);
  XII_TEST_RESULT(pyramid.InitializePinBindings(pins));
  XII_TEST_BOOL(!pins[pyramid.m_Pyramid.GetBindingIndex()].m_bTemporalHistoryOutput);
  pins.Clear();
  history.GetPins(pins);
  if (!XII_TEST_RESULT(history.InitializePinBindings(pins)))
    return;
  XII_TEST_BOOL(pins[history.m_Previous.GetBindingIndex()].m_bTemporalHistoryOutput && pins[history.m_Previous.GetBindingIndex()].m_bOptional);
  XII_TEST_BOOL(pins[history.m_Previous.GetBindingIndex()].m_Access == xiiRenderPipelinePinAccess::Read);
  XII_TEST_RESULT(history.GetProperties(properties));
  XII_TEST_BOOL(!properties.Contains("Texture") && !properties.Contains("PreviousTexture"));
}

XII_CREATE_SIMPLE_TEST(NativePreparation, IndependentInstanceOcclusion)
{
  xiiInstanceOcclusionPass pass;
  xiiVariantDictionary     properties;
  XII_TEST_RESULT(pass.GetProperties(properties));
  XII_TEST_BOOL(properties.Contains("DepthBias") && properties.Contains("AsyncCompute") && !properties.Contains("Candidates"));
  XII_TEST_BOOL(pass.GetRuntimeFeatures().IsNoFlagSet() && pass.GetExtractionRequirements().IsNoFlagSet());
  XII_TEST_BOOL(pass.GetDynamicRTTI()->FindPropertyByName("Stage") == nullptr);
  properties.Insert("DepthBias", -0.01f);
  XII_TEST_BOOL(pass.Configure(properties).Failed());
  properties.Insert("DepthBias", 1.01f);
  XII_TEST_BOOL(pass.Configure(properties).Failed());
  properties.Insert("DepthBias", xiiMath::NaN<float>());
  XII_TEST_BOOL(pass.Configure(properties).Failed());
  properties.Insert("DepthBias", 0.0f);
  XII_TEST_RESULT(pass.Configure(properties));
  xiiDynamicArray<xiiRenderPipelinePin> pins;
  pass.GetPins(pins);
  if (!XII_TEST_RESULT(pass.InitializePinBindings(pins)))
    return;
  XII_TEST_BOOL(pins[pass.m_Depth.GetBindingIndex()].m_bInput && !pins[pass.m_Depth.GetBindingIndex()].m_bOptional);
  XII_TEST_BOOL(pins[pass.m_Depth.GetBindingIndex()].m_Format == xiiGALResourceFormat::R32Float);
  XII_TEST_BOOL(pins[pass.m_Count.GetBindingIndex()].m_Access == xiiRenderPipelinePinAccess::ReadWrite);
  xiiRenderPipelineResourceDescriptor definition;
  definition.AddPass("Occlusion", "xiiInstanceOcclusionPass");
  definition.AddPass("Reset", "xiiCounterResetPass");
  XII_TEST_RESULT(definition.Connect(1, "Counter", 0, "InitialCount"));
  for (const auto& pin : pins)
    if (pin.m_Type != xiiRenderPipelinePin::Order && pin.m_sName != "InitialCount")
      definition.m_Interface.PushBack({pin.m_sName, pin.m_bInput, definition.m_Passes[0].m_Id, pin.m_sName});
  xiiGraphDefinition reusable;
  XII_TEST_RESULT(definition.MakeGraphDefinition(reusable));
  xiiRenderPipelineResourceDescriptor root;
  root.m_Definitions.PushBack(reusable);
  for (xiiUInt32 instance = 0; instance < 2; ++instance)
  {
    const auto node     = root.AddSubgraph(instance ? "Strict" : "Biased", reusable.m_Id);
    auto&      override = root.m_Passes[node].m_Overrides.ExpandAndGetRef();
    override.m_Path.PushBack(definition.m_Passes[0].m_Id);
    override.m_Properties.Insert("DepthBias", instance ? 0.0f : 0.0005f);
    for (const auto& pin : pins)
      if (pin.m_bInput && pin.m_Type != xiiRenderPipelinePin::Order && pin.m_sName != "InitialCount")
        root.m_Interface.PushBack({xiiStringBuilder(instance ? "Strict/" : "Biased/", pin.m_sName), true, root.m_Passes[node].m_Id, pin.m_sName});
  }
  xiiSharedPtr<const xiiRenderPipeline> compiled;
  XII_TEST_RESULT(xiiRenderPipeline::Compile(root, compiled));
  if (compiled)
  {
    XII_TEST_INT(compiled->GetExecutionOrder().GetCount(), 4);
  }
}

XII_CREATE_SIMPLE_TEST(NativePreparation, IndependentMeshletOcclusion)
{
  xiiMeshletOcclusionPass pass;
  xiiVariantDictionary    properties;
  XII_TEST_RESULT(pass.GetProperties(properties));
  XII_TEST_BOOL(properties.Contains("DepthBias") && properties.Contains("AsyncCompute") && !properties.Contains("Candidates"));
  XII_TEST_BOOL(pass.GetRuntimeFeatures().IsNoFlagSet() && pass.GetExtractionRequirements().IsNoFlagSet());
  XII_TEST_BOOL(pass.GetDynamicRTTI()->FindPropertyByName("Stage") == nullptr);
  properties.Insert("DepthBias", -0.01f);
  XII_TEST_BOOL(pass.Configure(properties).Failed());
  properties.Insert("DepthBias", 1.01f);
  XII_TEST_BOOL(pass.Configure(properties).Failed());
  properties.Insert("DepthBias", xiiMath::NaN<float>());
  XII_TEST_BOOL(pass.Configure(properties).Failed());
  properties.Insert("DepthBias", 0.0f);
  XII_TEST_RESULT(pass.Configure(properties));
  xiiDynamicArray<xiiRenderPipelinePin> pins;
  pass.GetPins(pins);
  if (!XII_TEST_RESULT(pass.InitializePinBindings(pins)))
    return;
  XII_TEST_BOOL(pins[pass.m_Depth.GetBindingIndex()].m_bInput && !pins[pass.m_Depth.GetBindingIndex()].m_bOptional);
  XII_TEST_BOOL(pins[pass.m_Depth.GetBindingIndex()].m_Format == xiiGALResourceFormat::R32Float);
  XII_TEST_BOOL(pins[pass.m_Count.GetBindingIndex()].m_Access == xiiRenderPipelinePinAccess::ReadWrite);
  xiiRenderPipelineResourceDescriptor definition;
  definition.AddPass("Occlusion", "xiiMeshletOcclusionPass");
  definition.AddPass("Reset", "xiiCounterResetPass");
  XII_TEST_RESULT(definition.Connect(1, "Counter", 0, "InitialCount"));
  for (const auto& pin : pins)
    if (pin.m_Type != xiiRenderPipelinePin::Order && pin.m_sName != "InitialCount")
      definition.m_Interface.PushBack({pin.m_sName, pin.m_bInput, definition.m_Passes[0].m_Id, pin.m_sName});
  xiiGraphDefinition reusable;
  XII_TEST_RESULT(definition.MakeGraphDefinition(reusable));
  xiiRenderPipelineResourceDescriptor root;
  root.m_Definitions.PushBack(reusable);
  for (xiiUInt32 instance = 0; instance < 2; ++instance)
  {
    const auto node     = root.AddSubgraph(instance ? "Strict" : "Biased", reusable.m_Id);
    auto&      override = root.m_Passes[node].m_Overrides.ExpandAndGetRef();
    override.m_Path.PushBack(definition.m_Passes[0].m_Id);
    override.m_Properties.Insert("DepthBias", instance ? 0.0f : 0.0005f);
    for (const auto& pin : pins)
      if (pin.m_bInput && pin.m_Type != xiiRenderPipelinePin::Order && pin.m_sName != "InitialCount")
        root.m_Interface.PushBack({xiiStringBuilder(instance ? "Strict/" : "Biased/", pin.m_sName), true, root.m_Passes[node].m_Id, pin.m_sName});
  }
  xiiSharedPtr<const xiiRenderPipeline> compiled;
  XII_TEST_RESULT(xiiRenderPipeline::Compile(root, compiled));
  if (compiled)
  {
    XII_TEST_INT(compiled->GetExecutionOrder().GetCount(), 4);
  }
}

XII_CREATE_SIMPLE_TEST(NativePreparation, IndependentGeometryStreamingFeedback)
{
  xiiGeometryLodFeedbackResetPass    reset;
  xiiGeometryLodFeedbackPass         generate;
  xiiGeometryLodFeedbackReadbackPass readback;
  for (xiiRenderPipelinePass* pass : {static_cast<xiiRenderPipelinePass*>(&reset), static_cast<xiiRenderPipelinePass*>(&generate), static_cast<xiiRenderPipelinePass*>(&readback)})
  {
    xiiDynamicArray<xiiRenderPipelinePin> pins;
    pass->GetPins(pins);
    XII_TEST_RESULT(pass->InitializePinBindings(pins));
    XII_TEST_BOOL(pass->GetExtractionRequirements().IsNoFlagSet() && pass->GetRuntimeFeatures().IsNoFlagSet());
    XII_TEST_BOOL(pass->GetDynamicRTTI()->FindPropertyByName("Stage") == nullptr);
  }
  xiiVariantDictionary properties;
  properties.Insert("Capacity", 3U);
  XII_TEST_BOOL(reset.Configure(properties).Failed());
  properties.Insert("Capacity", 0U);
  XII_TEST_BOOL(reset.Configure(properties).Failed());
  properties.Insert("Capacity", 65537U);
  XII_TEST_BOOL(reset.Configure(properties).Failed());
  properties.Insert("Capacity", 2U);
  XII_TEST_RESULT(reset.Configure(properties));
  properties.Clear();
  properties.Insert("PriorityTier", 4U);
  XII_TEST_BOOL(generate.Configure(properties).Failed());
  properties.Clear();
  properties.Insert("MaxPendingReadbacks", 0U);
  XII_TEST_BOOL(readback.Configure(properties).Failed());
  properties.Insert("MaxPendingReadbacks", 17U);
  XII_TEST_BOOL(readback.Configure(properties).Failed());
  properties.Insert("MaxPendingReadbacks", 1U);
  XII_TEST_RESULT(readback.Configure(properties));
  for (const xiiGeometryId id : {xiiGeometryId(0U, 0U), xiiGeometryId(0x100001U, 7U), xiiGeometryId(0xFFFFFEU, 255U)})
  {
    xiiGpuGeometryLodRequest request;
    request.m_uiKey = id.m_Data + 1;
    XII_TEST_BOOL(request.GetGeometry().GetInternalID() == id);
  }
  XII_TEST_BOOL(xiiGpuGeometryLodRequest().GetGeometry().IsInvalidated());
  xiiRenderPipelineResourceDescriptor definition;
  definition.AddPass("Initialize", "xiiGeometryLodFeedbackResetPass");
  definition.AddPass("Generate", "xiiGeometryLodFeedbackPass");
  definition.AddPass("Readback", "xiiGeometryLodFeedbackReadbackPass");
  XII_TEST_RESULT(definition.Connect(0, "Requests", 1, "InitializedRequests"));
  XII_TEST_RESULT(definition.Connect(1, "Requests", 2, "Requests"));
  xiiDynamicArray<xiiRenderPipelinePin> pins;
  generate.GetPins(pins);
  for (const auto& pin : pins)
    if (pin.m_Type != xiiRenderPipelinePin::Order && pin.m_sName != "InitializedRequests")
      definition.m_Interface.PushBack({pin.m_sName, pin.m_bInput, definition.m_Passes[1].m_Id, pin.m_sName});
  xiiGraphDefinition reusable;
  XII_TEST_RESULT(definition.MakeGraphDefinition(reusable));
  xiiRenderPipelineResourceDescriptor root;
  root.m_Definitions.PushBack(reusable);
  for (xiiUInt32 i = 0; i < 2; ++i)
  {
    const auto node     = root.AddSubgraph(i ? "Camera" : "Shadow", reusable.m_Id);
    auto&      override = root.m_Passes[node].m_Overrides.ExpandAndGetRef();
    override.m_Path.PushBack(definition.m_Passes[1].m_Id);
    override.m_Properties.Insert("PriorityTier", i ? 3U : 1U);
    for (const auto& pin : pins)
      if (pin.m_bInput && pin.m_Type != xiiRenderPipelinePin::Order && pin.m_sName != "InitializedRequests")
        root.m_Interface.PushBack({pin.m_sName, true, root.m_Passes[node].m_Id, pin.m_sName});
  }
  xiiContiguousMemoryStreamStorage storage;
  xiiMemoryStreamWriter            writer(&storage);
  XII_TEST_RESULT(root.Serialize(writer));
  xiiRenderPipelineResourceDescriptor restored;
  xiiMemoryStreamReader               reader(&storage);
  XII_TEST_RESULT(restored.Deserialize(reader));
  xiiSharedPtr<const xiiRenderPipeline> compiled;
  XII_TEST_RESULT(xiiRenderPipeline::Compile(restored, compiled));
  if (compiled)
  {
    XII_TEST_INT(compiled->GetExecutionOrder().GetCount(), 6);
  }
}

XII_CREATE_SIMPLE_TEST(NativePreparation, ExplicitDepthTargetAndProjection)
{
  xiiDepthTargetPass           target;
  xiiDepthRegionClearPass      clear;
  xiiMatrixRasterConstantsPass constants;
  for (xiiRenderPipelinePass* pass : {static_cast<xiiRenderPipelinePass*>(&target), static_cast<xiiRenderPipelinePass*>(&clear), static_cast<xiiRenderPipelinePass*>(&constants)})
  {
    xiiDynamicArray<xiiRenderPipelinePin> pins;
    pass->GetPins(pins);
    XII_TEST_RESULT(pass->InitializePinBindings(pins));
    XII_TEST_BOOL(pass->GetRuntimeFeatures().IsNoFlagSet() && pass->GetExtractionRequirements().IsNoFlagSet());
    XII_TEST_BOOL(pass->GetDynamicRTTI()->FindPropertyByName("Stage") == nullptr);
  }
  xiiVariantDictionary properties;
  properties.Insert("Depth", -0.1f);
  XII_TEST_BOOL(target.Configure(properties).Failed());
  properties.Insert("Depth", xiiMath::NaN<float>());
  XII_TEST_BOOL(target.Configure(properties).Failed());
  properties.Clear();
  properties.Insert("Viewport", xiiVec4U32(1, 0, 0, 0));
  XII_TEST_BOOL(clear.Configure(properties).Failed());
  properties.Insert("Viewport", xiiVec4U32(1, 1, 8, 0));
  XII_TEST_BOOL(clear.Configure(properties).Failed());
  properties.Insert("Viewport", xiiVec4U32(1, 1, 8, 8));
  XII_TEST_RESULT(clear.Configure(properties));
  properties.Clear();
  properties.Insert("Width", 0U);
  XII_TEST_BOOL(constants.Configure(properties).Failed());
  properties.Insert("Width", 16U);
  properties.Insert("ViewProjection", xiiMat4::MakeZero());
  XII_TEST_RESULT(constants.Configure(properties));
  xiiMat4 invalid       = xiiMat4::MakeIdentity();
  invalid.Element(0, 0) = xiiMath::NaN<float>();
  properties.Insert("ViewProjection", invalid);
  XII_TEST_BOOL(constants.Configure(properties).Failed());
  xiiDepthRasterPass                    raster;
  xiiDynamicArray<xiiRenderPipelinePin> pins;
  raster.GetPins(pins);
  if (!XII_TEST_RESULT(raster.InitializePinBindings(pins)))
    return;
  XII_TEST_BOOL(pins[raster.m_InputDepth.GetBindingIndex()].m_bInput && !pins[raster.m_InputDepth.GetBindingIndex()].m_bOptional);
  XII_TEST_BOOL(pins[raster.m_OutputDepth.GetBindingIndex()].m_Access == xiiRenderPipelinePinAccess::ReadWrite);
  properties.Clear();
  properties.Insert("Viewport", xiiVec4U32(0, 0, 8, 0));
  XII_TEST_BOOL(raster.Configure(properties).Failed());
  xiiRenderGraph graph;
  graph.BeginSetup(1);
  const auto image = graph.AddPass<xiiRenderGraphTextureHandle>("CPU atlas contract", xiiGALCommandQueueFlags::Graphics, [](auto& image, xiiRenderGraphBuilder& builder) {
    xiiGALTextureCreationDescription description; description.m_Type = xiiGALResourceDimension::Texture2D; description.m_Size = {32, 16}; description.m_Format = xiiGALResourceFormat::D32Float; description.m_BindFlags = xiiGALBindFlags::DepthStencil;
    image = builder.WriteTexture("CPU atlas", description, xiiGALResourceStateFlags::DepthWrite); }, [](const auto&, xiiRenderGraphPassContext&) {});
  pins.Clear();
  clear.GetPins(pins);
  if (!XII_TEST_RESULT(clear.InitializePinBindings(pins)))
    return;
  xiiDynamicArray<xiiRenderPipelineBinding> bindings;
  bindings.SetCount(pins.GetCount());
  bindings[clear.m_Input.GetBindingIndex()].m_hTexture = *image.first;
  xiiRenderGraphBlackboard     blackboard;
  xiiVariantDictionary         parameters;
  xiiStringBuilder             error;
  xiiRenderPipelinePassContext context{nullptr, graph, blackboard, parameters, pins, bindings, "Out of bounds clear"};
  context.m_pBuildError = &error;
  clear.m_Viewport      = xiiVec4U32(31, 0, 4, 4);
  XII_TEST_BOOL(clear.Build(context).Failed());
  XII_TEST_BOOL(error.FindSubString("outside") != nullptr);
  parameters.Insert("RasterViewport", 1.0f);
  error.Clear();
  XII_TEST_BOOL(clear.Build(context).Failed());
  XII_TEST_BOOL(error.FindSubString("RasterViewport") != nullptr);
  parameters.Insert("RasterViewport", xiiVec4U32(1, 0, 0, 0));
  error.Clear();
  XII_TEST_BOOL(clear.Build(context).Failed());
  XII_TEST_BOOL(error.FindSubString("nonempty rectangle") != nullptr);

  xiiDepthTargetPass dynamicTarget;
  pins.Clear();
  dynamicTarget.GetPins(pins);
  XII_TEST_RESULT(dynamicTarget.InitializePinBindings(pins));
  bindings.Clear();
  bindings.SetCount(pins.GetCount());
  parameters.Clear();
  parameters.Insert("RasterWidth", 64.0f);
  parameters.Insert("RasterHeight", 16U);
  xiiRenderPipelinePassContext targetContext{nullptr, graph, blackboard, parameters, pins, bindings, "Runtime depth target"};
  targetContext.m_pBuildError = &error;
  error.Clear();
  XII_TEST_BOOL(dynamicTarget.Build(targetContext).Failed());
  XII_TEST_BOOL(error.FindSubString("RasterWidth") != nullptr);
  parameters.Insert("RasterWidth", 64U);
  XII_TEST_RESULT(dynamicTarget.Build(targetContext));
  const auto* allocated = graph.GetTextureDescription(bindings[dynamicTarget.m_Depth.GetBindingIndex()].m_hTexture);
  if (XII_TEST_BOOL(allocated != nullptr))
    XII_TEST_INT(allocated->m_Size.width, 64);
  XII_TEST_INT(dynamicTarget.m_uiWidth, 0); // Runtime dimensions do not change authored properties.
  graph.EndSetup();
}

XII_CREATE_SIMPLE_TEST(NativePreparation, MatrixQueryReflection)
{
  xiiMatrixVisibilityViewPass           pass;
  xiiDynamicArray<xiiRenderPipelinePin> pins;
  pass.GetPins(pins);
  XII_TEST_RESULT(pass.InitializePinBindings(pins));
  XII_TEST_BOOL(pass.GetRuntimeFeatures() == xiiRenderPipelineFeature::GpuScene);
  XII_TEST_BOOL(!pass.m_bUseViewSelection);
  xiiVariantDictionary properties;
  XII_TEST_RESULT(pass.GetProperties(properties));
  XII_TEST_BOOL(properties.Contains("ViewProjection") && properties.Contains("CameraPosition") && properties.Contains("RequiredFlags"));
  properties.Insert("Width", 0U);
  XII_TEST_BOOL(pass.Configure(properties).Failed());
  properties.Insert("Width", 16U);
  properties.Insert("CameraPosition", xiiVec3(xiiMath::NaN<float>(), 0, 0));
  XII_TEST_BOOL(pass.Configure(properties).Failed());
}
