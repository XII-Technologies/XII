/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsCore/Pipeline/Passes/Geometry/DepthRasterPass.h>
#include <GraphicsCore/Pipeline/Passes/Geometry/GBufferRasterPass.h>
#include <GraphicsCore/Pipeline/Passes/Geometry/MeshDrawCommandsPass.h>
#include <GraphicsCore/Pipeline/Passes/Geometry/VertexPullDrawCommandsPass.h>
#include <GraphicsCore/Pipeline/Passes/Preparation/WorldInstancesUploadPass.h>
#include <GraphicsCore/Pipeline/Passes/Temporal/BufferHistoryPass.h>
#include <GraphicsCore/Pipeline/Passes/Utility/CounterResetPass.h>
#include <GraphicsCore/Pipeline/Passes/Visibility/InstanceCullPass.h>
#include <GraphicsCore/Pipeline/Passes/Visibility/LodHistoryResetPass.h>
#include <GraphicsCore/Pipeline/Passes/Visibility/MeshletCullPass.h>
#include <GraphicsCore/Pipeline/Passes/Visibility/MeshletDispatchBoundsPass.h>
#include <GraphicsCore/Pipeline/Passes/Visibility/MeshletDispatchPass.h>
#include <GraphicsCore/Pipeline/Passes/Visibility/VisibilityViewPass.h>
#include <GraphicsCore/Pipeline/Passes/Visualization/SurfaceQueryPass.h>

#include <Core/Graphics/Camera.h>
#include <Core/Graphics/Geometry.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Meshes/GpuInstanceData.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Visibility/GpuVisibilityTypes.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundationTest/TestingEnvironment/BufferReadback.h>
#include <GraphicsFoundationTest/TestingEnvironment/RenderGraphTestContext.h>
#include <GraphicsFoundationTest/TestingEnvironment/TextureReadback.h>
#include <bit>

class xiiNativeCullSource : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiNativeCullSource, xiiRenderPipelinePass);

public:
  xiiRenderPipelineBufferOutputPin m_Scene{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin m_Geometry{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin m_View{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin m_Candidates{false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin m_Meshlets{false, xiiGALBindFlags::ShaderResource};
  xiiUInt32                        m_uiBaseId   = 41;
  bool                             m_bSelectOne = false;
  xiiResult                        Build(xiiRenderPipelinePassContext& context) override
  {
    const auto*         parameter = context.m_Parameters.GetValue("Phase");
    const auto          phase     = parameter ? parameter->Get<xiiUInt32>() : 0U;
    const float         distance  = phase == 0 ? 4.5f : phase == 3 ? 4.1f :
                                                                     3.9f;
    const xiiUInt32     capacity  = phase >= 6 ? 129 : 67;
    xiiGpuSceneInstance records[129];
    for (xiiUInt32 i = 0; i < capacity; ++i)
    {
      records[i].m_uiGeometryIndex    = 0;
      records[i].m_uiObjectId         = m_uiBaseId + i + (phase >= 3 ? 100 : 0);
      records[i].m_uiFlags            = i == 65 ? 0 : xiiGpuInstanceFlags::Enabled;
      records[i].m_uiVisibilityMask   = i == 64 ? 0 : 1;
      records[i].m_BoundsCenterRadius = xiiVec4(i == 66 ? 102.0f : distance, 0, 0, 1);
    }
    if (phase == 2)
    {
      records[0].m_uiFirstPrimitive = 6;
      records[0].m_uiPrimitiveCount = 2;
    }
    if (phase == 6)
    {
      records[0].m_GlobalTransform = xiiMat4::MakeScaling(xiiVec3(2, 1, 1));
      records[1].m_GlobalTransform = xiiMat4::MakeRotationZ(xiiAngle::MakeFromDegree(45)) *
        xiiMat4::MakeRotationY(xiiAngle::MakeFromDegree(35.2643897f)) * xiiMat4::MakeScaling(xiiVec3(2, 1, 1));
      records[3].m_GlobalTransform = xiiMat4::MakeScaling(xiiVec3(-1, 1, 1));
    }
    if (phase == 7)
      for (xiiUInt32 i = 0; i < capacity; ++i)
        records[i].m_uiSkinningCount = 1;
    xiiGpuGeometryRecord geometry;
    geometry.m_uiGeneration                      = phase >= 2 ? 8 : 7;
    geometry.m_uiLodCount                        = 2;
    geometry.m_uiResidentLodMask                 = 3;
    geometry.m_Lods[0].m_fMinimumScreenCoverage  = 16;
    geometry.m_Lods[1].m_fMinimumScreenCoverage  = 0;
    geometry.m_Lods[0].m_uiMeshletCount          = 129;
    geometry.m_Lods[1].m_uiMeshletCount          = 65;
    geometry.m_Lods[1].m_uiMeshletMetadataOffset = 129;
    for (xiiUInt32 i = 0; i < 2; ++i)
    {
      geometry.m_Lods[i].m_uiVertexBufferIndex           = 1;
      geometry.m_Lods[i].m_uiMeshletRemapBufferIndex     = 1;
      geometry.m_Lods[i].m_uiMeshletPrimitiveBufferIndex = 1;
      geometry.m_Lods[i].m_uiIndexCount                  = geometry.m_Lods[i].m_uiMeshletCount * 3;
    }
    xiiMeshlet metadata[194] = {};
    for (xiiUInt32 lod = 0; lod < 2; ++lod)
    {
      const xiiUInt32 base = lod == 0 ? 0 : 129;
      for (xiiUInt32 i = 0; i < (lod == 0 ? 129U : 65U); ++i)
      {
        metadata[base + i].m_uiFirstPrimitive = i;
        metadata[base + i].m_uiPrimitiveCount = i == 5 ? 4 : 1;
        metadata[base + i].m_uiVertexCount    = 3;
        metadata[base + i].m_uiLodIndex       = static_cast<xiiUInt16>(lod);
      }
      metadata[base].m_Bounds          = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3(102, 0, 0), 0.1f);
      metadata[base + 1].m_Bounds      = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3(1, 0, 0), 0.1f);
      metadata[base + 1].m_vConeAxis   = xiiVec3(1, 0, 0);
      metadata[base + 1].m_fConeCutoff = 1;
    }
    xiiGpuVisibilityView view;
    view.m_ViewProjectionMatrix          = xiiMat4::MakeIdentity();
    view.m_OcclusionViewProjectionMatrix = xiiMat4::MakeIdentity();
    for (auto& plane : view.m_FrustumPlanes)
      plane = xiiVec4::MakeZero();
    view.m_FrustumPlanes[0] = xiiVec4(1, 0, 0, -100);
    view.m_CameraPosition   = xiiVec4::MakeZero();
    view.m_ViewportAndHiZ   = xiiVec4(64, 64, 0, 0);
    const auto token        = xiiUInt32(context.m_Graph.GetFrameIndex() % 65535U);
    view.m_LodParameters    = xiiVec4(0.1f, float(token), float((token + 65534U) % 65535U), 0);
    view.m_uiInstanceCount  = phase == 5 ? 0 : capacity;
    view.m_uiVisibilityMask = 1;
    view.m_uiUseCandidates  = m_bSelectOne ? 1 : 0;
    view.m_uiCandidateCount = phase == 5 ? 0 : m_bSelectOne ? 1 :
                                                              capacity;
    xiiUInt32 selection[129];
    for (xiiUInt32 i = 0; i < capacity; ++i)
      selection[i] = i;
    xiiRenderPassResourceScope scope(context);
    const auto                 upload = [&](xiiStringView name, xiiUInt32 stride, xiiArrayPtr<const xiiUInt8> bytes) {
      struct Data
      {
        xiiRenderGraphBufferHandle m_Output;
        xiiDynamicArray<xiiUInt8>  m_Bytes;
      };
      const auto emitted = context.m_Graph.AddPass<Data>(name, xiiGALCommandQueueFlags::Transfer, [&](Data& data, xiiRenderGraphBuilder& builder) {
          xiiGALBufferCreationDescription description;
          description.m_uiSize = bytes.GetCount();
          description.m_uiElementByteStride = stride;
          description.m_Mode = xiiGALBufferMode::Structured;
          description.m_BindFlags = xiiGALBindFlags::ShaderResource;
          data.m_Output = builder.WriteBuffer(name, description, xiiGALResourceStateFlags::CopyDestination);
          data.m_Bytes = bytes; }, [](const Data& data, xiiRenderGraphPassContext& execute) { execute.GetCommandList().UpdateBuffer(execute.GetBuffer(data.m_Output), 0, data.m_Bytes); });
      return emitted.first->m_Output;
    };
    XII_SUCCEED_OR_RETURN(context.SetOutput(m_Scene, upload("Synthetic Instances", sizeof(xiiGpuSceneInstance), xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(records), capacity * sizeof(xiiGpuSceneInstance)))));
    XII_SUCCEED_OR_RETURN(context.SetOutput(m_Geometry, upload("Synthetic Residency", sizeof(geometry), xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(&geometry), sizeof(geometry)))));
    XII_SUCCEED_OR_RETURN(context.SetOutput(m_View, upload("Synthetic Culling View", sizeof(view), xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(&view), sizeof(view)))));
    XII_SUCCEED_OR_RETURN(context.SetOutput(m_Meshlets, upload("Synthetic Meshlets", sizeof(xiiMeshlet), xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(metadata), sizeof(metadata)))));
    return context.SetOutput(m_Candidates, upload("Synthetic Candidates", sizeof(xiiUInt32), xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(selection), (m_bSelectOne ? 1 : capacity) * sizeof(xiiUInt32))));
  }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiNativeCullSource, 1, xiiRTTIDefaultAllocator<xiiNativeCullSource>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("SceneInstances", m_Scene)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("GeometryMetadata", m_Geometry)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("VisibilityView", m_View)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("Candidates", m_Candidates)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("MeshletMetadata", m_Meshlets)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("BaseId", m_uiBaseId),
      XII_MEMBER_PROPERTY("SelectOne", m_bSelectOne),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

class xiiNativeCullExport : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiNativeCullExport, xiiRenderPipelinePass);

public:
  xiiRenderPipelineBufferInputPin m_Visible, m_Count, m_Lod;
  xiiRenderPipelineBufferInputPin m_Previous{true};
  xiiResult                       Build(xiiRenderPipelinePassContext& context) override
  {
    context.m_Graph.AddPass<xiiUInt32>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](auto&, xiiRenderGraphBuilder& builder) {
        for (auto handle : {context.GetInputBuffer(m_Visible), context.GetInputBuffer(m_Count), context.GetInputBuffer(m_Lod), context.GetInputBuffer(m_Previous)})
          if (handle.IsValid()) builder.ExportBuffer(builder.ReadBuffer(handle, xiiGALResourceStateFlags::ShaderResource), xiiGALResourceStateFlags::ShaderResource); }, [](const auto&, xiiRenderGraphPassContext&) {}, true);
    return XII_SUCCESS;
  }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiNativeCullExport, 1, xiiRTTIDefaultAllocator<xiiNativeCullExport>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Visible", m_Visible)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("Count", m_Count)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("Lod", m_Lod)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("Previous", m_Previous)->AddFlags(xiiPropertyFlags::ReadOnly),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

class xiiNativeMeshletWorkExport : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiNativeMeshletWorkExport, xiiRenderPipelinePass);

public:
  xiiRenderPipelineBufferInputPin m_Arguments, m_Meshlets, m_Count, m_Overflow;
  xiiRenderPipelineBufferInputPin m_Maximum{true}, m_DrawCommands{true}, m_DrawCount{true}, m_MeshCommands{true}, m_MeshCount{true};
  xiiResult                       Build(xiiRenderPipelinePassContext& context) override
  {
    context.m_Graph.AddPass<xiiUInt32>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](auto&, xiiRenderGraphBuilder& builder) {
        for (auto handle : {context.GetInputBuffer(m_Arguments), context.GetInputBuffer(m_Maximum), context.GetInputBuffer(m_Meshlets), context.GetInputBuffer(m_Count), context.GetInputBuffer(m_Overflow), context.GetInputBuffer(m_DrawCommands), context.GetInputBuffer(m_DrawCount), context.GetInputBuffer(m_MeshCommands), context.GetInputBuffer(m_MeshCount)})
          if (handle.IsValid()) builder.ExportBuffer(builder.ReadBuffer(handle, xiiGALResourceStateFlags::ShaderResource), xiiGALResourceStateFlags::ShaderResource); }, [](const auto&, xiiRenderGraphPassContext&) {}, true);
    return XII_SUCCESS;
  }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiNativeMeshletWorkExport, 1, xiiRTTIDefaultAllocator<xiiNativeMeshletWorkExport>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Arguments", m_Arguments)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("Maximum", m_Maximum)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("DrawCommands", m_DrawCommands)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("DrawCount", m_DrawCount)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("MeshCommands", m_MeshCommands)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("MeshCount", m_MeshCount)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("Meshlets", m_Meshlets)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("Count", m_Count)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("Overflow", m_Overflow)->AddFlags(xiiPropertyFlags::ReadOnly),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

class xiiNativeSurfaceExport : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiNativeSurfaceExport, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureInputPin m_Surface{xiiGALResourceFormat::RGBA32UInt, true}, m_Depth{xiiGALResourceFormat::D32Float};
  xiiRenderPipelineTextureInputPin m_Albedo{xiiGALResourceFormat::RGBA8UNormalized, true}, m_Normal{xiiGALResourceFormat::RG16SNormalized, true};
  xiiRenderPipelineTextureInputPin m_Material{xiiGALResourceFormat::RGBA8UNormalized, true}, m_Emissive{xiiGALResourceFormat::RGBA16Float, true};
  xiiRenderPipelineTextureInputPin m_NormalRoughness{xiiGALResourceFormat::RGBA8UNormalized, true}, m_Velocity{xiiGALResourceFormat::RG16Float, true}, m_ObjectId{xiiGALResourceFormat::R32UInt, true};
  xiiResult                        Build(xiiRenderPipelinePassContext& context) override
  {
    context.m_Graph.AddPass<xiiUInt32>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](auto&, xiiRenderGraphBuilder& builder) {
      const auto exportOptional = [&](const xiiRenderPipelineTextureInputPin& pin) {
        if (context.GetInputTexture(pin).IsValid()) builder.ExportTexture(builder.ReadTexture(context.GetInputTexture(pin), xiiGALResourceStateFlags::ShaderResource), xiiGALResourceStateFlags::ShaderResource);
      };
      exportOptional(m_Surface);
      exportOptional(m_Albedo); exportOptional(m_Normal); exportOptional(m_Material); exportOptional(m_Emissive);
      exportOptional(m_NormalRoughness); exportOptional(m_Velocity); exportOptional(m_ObjectId);
      builder.ExportTexture(builder.ReadTexture(context.GetInputTexture(m_Depth), xiiGALResourceStateFlags::ShaderResource), xiiGALResourceStateFlags::ShaderResource); }, [](const auto&, xiiRenderGraphPassContext&) {}, true);
    return XII_SUCCESS;
  }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiNativeSurfaceExport, 1, xiiRTTIDefaultAllocator<xiiNativeSurfaceExport>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Surface", m_Surface)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("Albedo", m_Albedo)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("Normal", m_Normal)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("Material", m_Material)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("Emissive", m_Emissive)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("NormalRoughness", m_NormalRoughness)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("Velocity", m_Velocity)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("ObjectId", m_ObjectId)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("SceneDepth", m_Depth)->AddFlags(xiiPropertyFlags::ReadOnly),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

namespace
{
  struct MeshletWork
  {
    xiiUInt32 m_uiDispatch, m_uiCull, m_uiDraw, m_uiMeshDraw;
  };
  MeshletWork AddMeshletWork(xiiRenderPipelineResourceDescriptor& definition, xiiStringView label, xiiUInt32 prepared, xiiUInt32 culled)
  {
    const auto reset    = definition.AddPass(xiiStringBuilder(label, " Reset"), xiiCounterResetPass().GetDynamicRTTI());
    const auto bounds   = definition.AddPass(xiiStringBuilder(label, " Bound"), xiiMeshletDispatchBoundsPass().GetDynamicRTTI());
    const auto dispatch = definition.AddPass(xiiStringBuilder(label, " Dispatch"), xiiMeshletDispatchPass().GetDynamicRTTI());
    const auto output   = definition.AddPass(xiiStringBuilder(label, " Export"), xiiNativeMeshletWorkExport().GetDynamicRTTI());
    for (auto pin : {"SceneInstances", "GeometryMetadata", "MeshletMetadata", "VisibilityView"})
      XII_TEST_RESULT(definition.Connect(prepared, pin, bounds, pin));
    for (auto pin : {"VisibleInstances", "VisibleInstanceCount", "LodHistory"})
      XII_TEST_RESULT(definition.Connect(culled, pin, bounds, pin));
    XII_TEST_RESULT(definition.Connect(reset, "Counter", bounds, "InitializedMaximum"));
    XII_TEST_RESULT(definition.Connect(bounds, "MaximumMeshletGroups", dispatch, "MaximumMeshletGroups"));
    XII_TEST_RESULT(definition.Connect(prepared, "MeshletMetadata", dispatch, "MeshletMetadata"));
    XII_TEST_RESULT(definition.Connect(culled, "VisibleInstances", dispatch, "VisibleInstances"));
    XII_TEST_RESULT(definition.Connect(culled, "VisibleInstanceCount", dispatch, "VisibleInstanceCount"));
    XII_TEST_RESULT(definition.Connect(dispatch, "DispatchArguments", output, "Arguments"));
    XII_TEST_RESULT(definition.Connect(bounds, "MaximumMeshletGroups", output, "Maximum"));
    const auto countReset    = definition.AddPass(xiiStringBuilder(label, " Count Reset"), xiiCounterResetPass().GetDynamicRTTI());
    const auto overflowReset = definition.AddPass(xiiStringBuilder(label, " Overflow Reset"), xiiCounterResetPass().GetDynamicRTTI());
    const auto meshlet       = definition.AddPass(xiiStringBuilder(label, " Cull"), xiiMeshletCullPass().GetDynamicRTTI());
    definition.m_Passes[meshlet].m_Properties.Insert("MaxVisibleMeshlets", 16384U);
    for (auto pin : {"SceneInstances", "GeometryMetadata", "MeshletMetadata", "VisibilityView"})
      XII_TEST_RESULT(definition.Connect(prepared, pin, meshlet, pin));
    for (auto pin : {"VisibleInstances", "VisibleInstanceCount", "LodHistory"})
      XII_TEST_RESULT(definition.Connect(culled, pin, meshlet, pin));
    XII_TEST_RESULT(definition.Connect(dispatch, "DispatchArguments", meshlet, "DispatchArguments"));
    XII_TEST_RESULT(definition.Connect(countReset, "Counter", meshlet, "InitialCount"));
    XII_TEST_RESULT(definition.Connect(overflowReset, "Counter", meshlet, "InitialOverflow"));
    XII_TEST_RESULT(definition.Connect(meshlet, "VisibleMeshlets", output, "Meshlets"));
    XII_TEST_RESULT(definition.Connect(meshlet, "VisibleMeshletCount", output, "Count"));
    XII_TEST_RESULT(definition.Connect(meshlet, "VisibleMeshletOverflow", output, "Overflow"));
    const auto draw = definition.AddPass(xiiStringBuilder(label, " Draw Commands"), xiiVertexPullDrawCommandsPass().GetDynamicRTTI());
    for (auto pin : {"SceneInstances", "GeometryMetadata", "MeshletMetadata", "VisibilityView"})
      XII_TEST_RESULT(definition.Connect(prepared, pin, draw, pin));
    XII_TEST_RESULT(definition.Connect(meshlet, "VisibleMeshlets", draw, "VisibleMeshlets"));
    XII_TEST_RESULT(definition.Connect(meshlet, "VisibleMeshletCount", draw, "VisibleMeshletCount"));
    XII_TEST_RESULT(definition.Connect(draw, "DrawCommands", output, "DrawCommands"));
    XII_TEST_RESULT(definition.Connect(draw, "DrawCommandCount", output, "DrawCount"));
    const auto meshDraw = definition.AddPass(xiiStringBuilder(label, " Mesh Draw Commands"), xiiMeshDrawCommandsPass().GetDynamicRTTI());
    definition.m_Passes[meshDraw].m_Properties.Insert("MeshGroupCountX", 4U);
    definition.m_Passes[meshDraw].m_Properties.Insert("MeshGroupCountY", 2U);
    XII_TEST_RESULT(definition.Connect(meshlet, "VisibleMeshlets", meshDraw, "VisibleMeshlets"));
    XII_TEST_RESULT(definition.Connect(meshlet, "VisibleMeshletCount", meshDraw, "VisibleMeshletCount"));
    XII_TEST_RESULT(definition.Connect(meshDraw, "DrawCommands", output, "MeshCommands"));
    XII_TEST_RESULT(definition.Connect(meshDraw, "DrawCommandCount", output, "MeshCount"));
    return {dispatch, meshlet, draw, meshDraw};
  }
} // namespace

XII_CREATE_SIMPLE_TEST_GROUP(NativeVisibility);
XII_CREATE_SIMPLE_TEST(NativeVisibility, InstanceCullingAndExplicitHistory)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiRenderPipelineResourceDescriptor leaf;
  leaf.AddPass("Input snapshots", xiiNativeCullSource().GetDynamicRTTI());
  leaf.AddPass("Invalidate current LOD", xiiLodHistoryResetPass().GetDynamicRTTI());
  leaf.AddPass("Reset visible count", xiiCounterResetPass().GetDynamicRTTI());
  leaf.AddPass("Cull instances", xiiInstanceCullPass().GetDynamicRTTI());
  leaf.AddPass("Retain LOD history", xiiBufferHistoryPass().GetDynamicRTTI());
  leaf.AddPass("Read back", xiiNativeCullExport().GetDynamicRTTI());
  for (auto pin : {"SceneInstances", "GeometryMetadata", "VisibilityView", "Candidates"})
    XII_TEST_RESULT(leaf.Connect(0, pin, 3, pin));
  XII_TEST_RESULT(leaf.Connect(0, "SceneInstances", 1, "SceneInstances"));
  XII_TEST_RESULT(leaf.Connect(1, "LodHistory", 3, "InitializedLodHistory"));
  XII_TEST_RESULT(leaf.Connect(2, "Counter", 3, "Counter"));
  XII_TEST_RESULT(leaf.Connect(3, "LodHistory", 4, "Buffer"));
  XII_TEST_RESULT(leaf.Connect(4, "PreviousBuffer", 3, "PreviousLodHistory"));
  XII_TEST_RESULT(leaf.Connect(3, "VisibleInstances", 5, "Visible"));
  XII_TEST_RESULT(leaf.Connect(3, "VisibleInstanceCount", 5, "Count"));
  XII_TEST_RESULT(leaf.Connect(3, "LodHistory", 5, "Lod"));
  XII_TEST_RESULT(leaf.Connect(4, "PreviousBuffer", 5, "Previous"));
  const auto         work       = AddMeshletWork(leaf, "Selected LOD Work", 0, 3);
  const auto         dispatchId = leaf.m_Passes[work.m_uiDispatch].m_Id;
  const auto         cullId = leaf.m_Passes[3].m_Id, historyId = leaf.m_Passes[4].m_Id;
  xiiGraphDefinition definition;
  XII_TEST_RESULT(leaf.MakeGraphDefinition(definition));
  xiiRenderPipelineResourceDescriptor root;
  root.m_Definitions.PushBack(definition);
  root.AddSubgraph("Full candidate set", definition.m_Id);
  root.AddSubgraph("One selected component", definition.m_Id);
  const auto selectedId = root.m_Passes[1].m_Id;
  auto&      override   = root.m_Passes[1].m_Overrides.ExpandAndGetRef();
  override.m_Path.PushBack(leaf.m_Passes[0].m_Id);
  override.m_Properties.Insert("BaseId", 1001U);
  override.m_Properties.Insert("SelectOne", true);
  auto& budget = root.m_Passes[1].m_Overrides.ExpandAndGetRef();
  budget.m_Path.PushBack(leaf.m_Passes[work.m_uiCull].m_Id);
  budget.m_Properties.Insert("MaxVisibleMeshlets", 16U);
  xiiContiguousMemoryStreamStorage bytes;
  xiiMemoryStreamWriter            writer(&bytes);
  XII_TEST_RESULT(root.Serialize(writer));
  xiiMemoryStreamReader               reader(&bytes);
  xiiRenderPipelineResourceDescriptor decoded;
  XII_TEST_RESULT(decoded.Deserialize(reader));
  xiiSharedPtr<const xiiRenderPipeline> compiled;
  if (!XII_TEST_RESULT(xiiRenderPipeline::Compile(decoded, compiled)))
    return;
  XII_TEST_BOOL(compiled->GetRuntimeFeatures().IsNoFlagSet() && compiled->GetExtractionRequirements().IsNoFlagSet());
  xiiRenderPipelineInstance instances[2];
  xiiRenderGraphTestContext contexts[2];
  for (xiiUInt32 i = 0; i < 2; ++i)
  {
    if (!XII_TEST_RESULT(instances[i].Initialize(compiled)) || !XII_TEST_RESULT(contexts[i].Initialize("Native culling view")))
      return;
  }
  for (xiiUInt32 phase = 0; phase < 8; ++phase)
  {
    if (phase == 4)
      instances[0].InvalidateTemporalHistory();
    xiiRenderGraphFrame frame;
    if (!XII_TEST_RESULT(frame.Begin()))
      return;
    xiiSharedPtr<xiiGALBuffer> histories[4];
    xiiUInt32                  retained = 0;
    xiiUInt32                  checked  = 0;
    for (xiiUInt32 view = 0; view < 2; ++view)
    {
      auto& graph = contexts[view].GetGraph();
      auto& cache = contexts[view].GetResourceCache();
      cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
      XII_SCOPE_EXIT(cache.EndFrame());
      graph.BeginSetup(frame.GetFrameIndex());
      xiiVariantDictionary parameters;
      parameters.Insert("Phase", phase);
      if (!XII_TEST_RESULT(instances[view].Build(nullptr, graph, contexts[view].GetBlackboard(), parameters)))
        return;
      graph.EndSetup();
      xiiStringBuilder error;
      if (!XII_TEST_RESULT_MSG(graph.Compile({.m_bEnableAsyncQueues = (phase & 1U) != 0}, &error), "%s", error.GetData()) || !XII_TEST_RESULT_MSG(graph.Execute(environment.GetDevice(), nullptr, &contexts[view].GetBlackboard(), &cache, nullptr, &error), "%s", error.GetData()))
        return;
      for (xiiUInt32 i = 0; i < compiled->GetDescriptor().m_Passes.GetCount(); ++i)
      {
        const auto& source   = compiled->GetProvenance()[i];
        const bool  selected = source.m_InstancePath[0] == selectedId;
        if (source.m_Element == dispatchId)
        {
          const auto buffer = graph.GetExportedBuffer(instances[view].GetOutputBinding(i, "DispatchArguments")->m_hBuffer);
          xiiVec3U32 arguments;
          if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), buffer.Borrow(), 0, arguments))
            return;
          const bool      coarse = phase <= 1 || phase == 3 || (phase == 4 && view == 1);
          const xiiUInt32 count  = phase == 5 ? 0 : selected ? 1 :
            phase >= 6                                       ? 126 :
                                                               64;
          XII_TEST_INT(arguments.x, count == 0 ? 0 : coarse ? 2 :
                                                              3);
          XII_TEST_INT(arguments.y, count);
          XII_TEST_INT(arguments.z, count == 0 ? 0 : 1);
        }
        if (source.m_Element == leaf.m_Passes[work.m_uiCull].m_Id)
        {
          auto      countBuffer    = graph.GetExportedBuffer(instances[view].GetOutputBinding(i, "VisibleMeshletCount")->m_hBuffer);
          auto      overflowBuffer = graph.GetExportedBuffer(instances[view].GetOutputBinding(i, "VisibleMeshletOverflow")->m_hBuffer);
          xiiUInt32 count, overflow;
          if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), countBuffer.Borrow(), 0, count) || !xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), overflowBuffer.Borrow(), 0, overflow))
            return;
          const bool      coarse   = phase <= 1 || phase == 3 || (phase == 4 && view == 1);
          const xiiUInt32 visible  = phase == 5 ? 0 : selected ? 1 :
            phase >= 6                                         ? 126 :
                                                                 64;
          xiiUInt32       expected = visible * (coarse ? 63 : 127);
          if (phase == 2)
            expected = (visible - 1) * 127 + 3;
          if (phase == 6)
            expected += selected ? 1 : 4;
          if (phase == 7)
            expected = visible * 129;
          const xiiUInt32 capacity = selected ? 16 : 16384;
          XII_TEST_INT(count, xiiMath::Min(expected, capacity));
          XII_TEST_INT(overflow, expected > capacity ? 1 : 0);
          if (count != 0)
          {
            auto pairs = graph.GetExportedBuffer(instances[view].GetOutputBinding(i, "VisibleMeshlets")->m_hBuffer);
            for (xiiUInt32 position = 0; position < count; position += selected ? 1 : xiiMath::Max(count - 1, 1U))
            {
              xiiVec2U32 pair;
              if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), pairs.Borrow(), xiiUInt64(position) * sizeof(pair), pair))
                return;
              XII_TEST_BOOL(pair.x < (selected ? 1U : phase >= 6 ? 129U :
                                                                   67U));
              XII_TEST_BOOL(pair.x != 64 && pair.x != 65 && pair.x != 66);
              const xiiUInt32 local = pair.y - (coarse ? 129 : 0);
              XII_TEST_BOOL(local < (coarse ? 65U : 129U));
              if (phase != 7)
              {
                XII_TEST_BOOL(local != 0 || (phase == 6 && pair.x == 3));
                XII_TEST_BOOL(local != 1 || (phase == 6 && (pair.x == 0 || pair.x == 1 || pair.x == 3)));
              }
              if (phase == 2 && pair.x == 0)
                XII_TEST_BOOL(local >= 5 && local < 8);
              xiiUInt32 drawIndex = xiiInvalidIndex;
              for (xiiUInt32 j = 0; j < compiled->GetProvenance().GetCount(); ++j)
                if (compiled->GetProvenance()[j].m_Element == leaf.m_Passes[work.m_uiDraw].m_Id && compiled->GetProvenance()[j].m_InstancePath == source.m_InstancePath)
                  drawIndex = j;
              if (!XII_TEST_BOOL(drawIndex != xiiInvalidIndex))
                return;
              auto       commandBuffer = graph.GetExportedBuffer(instances[view].GetOutputBinding(drawIndex, "DrawCommands")->m_hBuffer);
              xiiVec4U32 command;
              if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), commandBuffer.Borrow(), xiiUInt64(position) * sizeof(command), command))
                return;
              const bool clipped = phase == 2 && pair.x == 0 && local == 5;
              XII_TEST_INT(command.x, clipped ? 6 : local == 5 ? 12 :
                                                                 3);
              XII_TEST_INT(command.y, 1);
              XII_TEST_INT(command.z, clipped ? 3 : 0);
              XII_TEST_INT(command.w, position);
            }
          }
        }
        if (source.m_Element == leaf.m_Passes[work.m_uiMeshDraw].m_Id)
        {
          auto      arguments = graph.GetExportedBuffer(instances[view].GetOutputBinding(i, "DrawCommands")->m_hBuffer);
          auto      counter   = graph.GetExportedBuffer(instances[view].GetOutputBinding(i, "DrawCommandCount")->m_hBuffer);
          xiiUInt32 cullIndex = xiiInvalidIndex;
          for (xiiUInt32 j = 0; j < compiled->GetProvenance().GetCount(); ++j)
            if (compiled->GetProvenance()[j].m_Element == leaf.m_Passes[work.m_uiCull].m_Id && compiled->GetProvenance()[j].m_InstancePath == source.m_InstancePath)
              cullIndex = j;
          if (!XII_TEST_BOOL(cullIndex != xiiInvalidIndex))
            return;
          auto       meshlets = graph.GetExportedBuffer(instances[view].GetOutputBinding(cullIndex, "VisibleMeshletCount")->m_hBuffer);
          xiiUInt32  count, draws;
          xiiVec3U32 command;
          if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), arguments.Borrow(), 0, command) || !xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), counter.Borrow(), 0, draws) || !xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), meshlets.Borrow(), 0, count))
            return;
          const xiiUInt32 rows = (count + 3) / 4;
          XII_TEST_INT(command.x, xiiMath::Min(count, 4U));
          XII_TEST_INT(command.y, xiiMath::Min(rows, 2U));
          XII_TEST_INT(command.z, (rows + 1) / 2);
          XII_TEST_INT(draws, count ? 1 : 0);
          XII_TEST_BOOL(xiiUInt64(command.x) * command.y * command.z >= count);
        }
        if (source.m_Element == leaf.m_Passes[work.m_uiDraw].m_Id)
        {
          auto      counter       = graph.GetExportedBuffer(instances[view].GetOutputBinding(i, "DrawCommandCount")->m_hBuffer);
          auto      commandBuffer = graph.GetExportedBuffer(instances[view].GetOutputBinding(i, "DrawCommands")->m_hBuffer);
          xiiUInt32 count;
          if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), counter.Borrow(), 0, count))
            return;
          const xiiUInt32 capacity = selected ? 16 : 16384;
          XII_TEST_BOOL(count <= capacity);
          if (phase == 5)
          {
            XII_TEST_INT(count, 0);
          }
          if (count < capacity)
          {
            xiiVec4U32 unused;
            if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), commandBuffer.Borrow(), xiiUInt64(count) * sizeof(unused), unused))
              return;
            XII_TEST_BOOL(unused == xiiVec4U32::MakeZero());
          }
        }
        if (source.m_Element == cullId)
        {
          ++checked;
          auto      count        = graph.GetExportedBuffer(instances[view].GetOutputBinding(i, "VisibleInstanceCount")->m_hBuffer);
          auto      lod          = graph.GetExportedBuffer(instances[view].GetOutputBinding(i, "LodHistory")->m_hBuffer);
          xiiUInt32 visibleCount = xiiInvalidIndex;
          if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), count.Borrow(), 0, visibleCount))
            return;
          const xiiUInt32 expectedCount = phase == 5 ? 0 : selected ? 1 :
            phase >= 6                                              ? 126 :
                                                                      64;
          XII_TEST_INT(visibleCount, expectedCount);
          XII_TEST_INT(lod->GetDescription().m_uiSize, (phase >= 6 ? 129 : 67) * sizeof(xiiVec4U32));
          xiiVec4U32 choice;
          if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), lod.Borrow(), 0, choice))
            return;
          const auto expected = phase == 5 ? xiiInvalidIndex : phase <= 1 || phase == 3 || (phase == 4 && view == 1) ? 1U :
                                                                                                                       0U;
          XII_TEST_INT(choice.x, expected);
          if (phase != 5)
          {
            XII_TEST_INT(choice.y, phase >= 2 ? 8 : 7);
            XII_TEST_INT(choice.z, (selected ? 1001 : 41) + (phase >= 3 ? 100 : 0));
            XII_TEST_INT(choice.w, frame.GetFrameIndex() % 65535U);
          }
          if (selected)
          {
            if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), lod.Borrow(), sizeof(xiiVec4U32), choice))
              return;
            XII_TEST_INT(choice.x, xiiInvalidIndex);
          }
          else if (phase >= 6)
          {
            if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), lod.Borrow(), 128 * sizeof(xiiVec4U32), choice))
              return;
            XII_TEST_INT(choice.x, 0);
            XII_TEST_INT(choice.z, 41 + 128 + 100);
          }
        }
        if (source.m_Element == historyId)
        {
          const auto* binding = instances[view].GetOutputBinding(i, "PreviousBuffer");
          const bool  missing = phase == 0 || (phase == 4 && view == 0);
          XII_TEST_BOOL(binding && binding->m_hBuffer.IsValid() != missing);
          if (missing)
            continue;
          histories[retained] = graph.GetExportedBuffer(binding->m_hBuffer);
          if (!XII_TEST_BOOL(histories[retained] != nullptr))
            return;
          XII_TEST_INT(histories[retained]->GetDescription().m_uiSize, (phase >= 7 ? 129 : 67) * sizeof(xiiVec4U32));
          ++retained;
        }
      }
    }
    XII_TEST_INT(checked, 4);
    XII_TEST_INT(retained, phase == 0 ? 0 : phase == 4 ? 2 :
                                                         4);
    for (xiiUInt32 i = 0; i < retained; ++i)
      for (xiiUInt32 j = 0; j < i; ++j)
        XII_TEST_BOOL(histories[i] != histories[j]);
    frame.End();
    environment.GetDevice()->WaitIdle();
    XII_TEST_BOOL(!xiiGeometryResidencyManager::IsInitialized() && !xiiMaterialManager::IsInitialized());
  }
}

static void TestAuthoredWorldVisibility(bool depthOnly, bool materialTargets = false)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  const auto                                 asset = xiiLoadPipelineLibraryResource(materialTargets ? "GBuffer" : depthOnly ? "DepthOnly" :
                                                                                                                              "SurfaceQuery");
  xiiResourceLock<xiiRenderPipelineResource> loaded(asset, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(loaded.IsValid() && loaded->GetAuthoring() && loaded->GetPipeline()))
    return;
  const auto               authoring = loaded->GetAuthoring();
  xiiUuid                  cullId, uploadId, dispatchId, meshletId, drawId, surfaceId, materialId;
  xiiDynamicArray<xiiUuid> viewPath;
  const auto               nested = loaded->GetPipeline();
  for (xiiUInt32 i = 0; i < nested->GetDescriptor().m_Passes.GetCount(); ++i)
  {
    const auto& pass   = nested->GetDescriptor().m_Passes[i];
    const auto& source = nested->GetProvenance()[i];
    if (pass.m_pType == xiiInstanceCullPass().GetDynamicRTTI())
      cullId = source.m_Element;
    if (pass.m_pType == xiiWorldInstancesUploadPass().GetDynamicRTTI())
      uploadId = source.m_Element;
    if (pass.m_pType == xiiMeshletDispatchPass().GetDynamicRTTI())
      dispatchId = source.m_Element;
    if (pass.m_pType == xiiMeshletCullPass().GetDynamicRTTI())
      meshletId = source.m_Element;
    if (pass.m_pType == xiiVertexPullDrawCommandsPass().GetDynamicRTTI())
      drawId = source.m_Element;
    if (pass.m_pType == (depthOnly ? xiiDepthRasterPass().GetDynamicRTTI() : xiiSurfaceQueryPass().GetDynamicRTTI()))
      surfaceId = source.m_Element;
    if (pass.m_pType == xiiGBufferRasterPass().GetDynamicRTTI())
      materialId = source.m_Element;
    if (pass.m_pType == xiiVisibilityViewPass().GetDynamicRTTI())
    {
      viewPath = source.m_InstancePath;
      viewPath.PushBack(source.m_Element);
    }
  }
  if (materialTargets && !XII_TEST_BOOL(materialId.IsValid()))
    return;
  if (!XII_TEST_BOOL(cullId.IsValid() && uploadId.IsValid() && dispatchId.IsValid() && meshletId.IsValid() && drawId.IsValid() && surfaceId.IsValid() && viewPath.GetCount() == 4))
    return;
  xiiRenderPipelineResourceDescriptor root;
  root.m_References.PushBack({authoring->m_Descriptor.m_Id, asset.GetResourceID()});
  root.AddSubgraph("Visible world", authoring->m_Descriptor.m_Id);
  root.AddSubgraph("Filtered world", authoring->m_Descriptor.m_Id);
  const auto filteredId = root.m_Passes[1].m_Id;
  auto&      override   = root.m_Passes[1].m_Overrides.ExpandAndGetRef();
  override.m_Path       = viewPath;
  override.m_Properties.Insert("VisibilityMask", 0U);
  for (xiiUInt32 i = 0; i < 2; ++i)
  {
    const auto pixels = root.AddPass(i == 0 ? "Visible surface readback" : "Filtered surface readback", xiiNativeSurfaceExport().GetDynamicRTTI());
    if (!depthOnly)
    {
      XII_TEST_RESULT(root.Connect(i, "Surface", pixels, "Surface"));
    }
    XII_TEST_RESULT(root.Connect(i, "SceneDepth", pixels, "SceneDepth"));
    if (materialTargets)
      for (auto pin : {"Albedo", "Normal", "Material", "Emissive", "NormalRoughness", "Velocity", "ObjectId"})
        XII_TEST_RESULT(root.Connect(i, pin, pixels, pin));
    const auto work = root.AddPass(i == 0 ? "Visible meshlet readback" : "Filtered meshlet readback", xiiNativeMeshletWorkExport().GetDynamicRTTI());
    XII_TEST_RESULT(root.Connect(i, "DispatchArguments", work, "Arguments"));
    XII_TEST_RESULT(root.Connect(i, "VisibleMeshlets", work, "Meshlets"));
    XII_TEST_RESULT(root.Connect(i, "VisibleMeshletCount", work, "Count"));
    XII_TEST_RESULT(root.Connect(i, "VisibleMeshletOverflow", work, "Overflow"));
    XII_TEST_RESULT(root.Connect(i, "DrawCommands", work, "DrawCommands"));
    XII_TEST_RESULT(root.Connect(i, "DrawCommandCount", work, "DrawCount"));
    const auto sink = root.AddPass(i == 0 ? "Visible readback" : "Filtered readback", xiiNativeCullExport().GetDynamicRTTI());
    XII_TEST_RESULT(root.Connect(i, "VisibleInstances", sink, "Visible"));
    XII_TEST_RESULT(root.Connect(i, "VisibleInstanceCount", sink, "Count"));
    XII_TEST_RESULT(root.Connect(i, "LodHistory", sink, "Lod"));
  }
  const auto  pipeline = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(materialTargets ? "Authored independent material instances" : depthOnly ? "Authored independent depth instances" :
                                                                                                                                                                 "Authored world visibility instances",
                                                                                       std::move(root));
  xiiGeometry box;
  box.AddBox(xiiVec3(1), true);
  xiiMeshResourceDescriptor meshDescription;
  meshDescription.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(box, xiiGALPrimitiveTopology::TriangleList, false);
  meshDescription.AddSection(12, 0, 0);
  meshDescription.ComputeBounds();
  const auto          mesh = xiiResourceManager::CreateResource<xiiMeshResource>("NativeVisibility.WorldMesh", std::move(meshDescription));
  xiiWorldDescription description("Authored world visibility");
  xiiWorld            world(description);
  XII_LOCK(world.GetWriteMarker());
  world.SetWorldSimulationEnabled(false);
  xiiGameObject*          object       = nullptr;
  const auto              objectHandle = world.CreateObject({}, object);
  xiiStaticMeshComponent* component    = nullptr;
  xiiStaticMeshComponent::CreateComponent(object, component);
  component->SetMesh(mesh);
  auto*     module = world.GetOrCreateModule<xiiRenderWorldModule>();
  xiiCamera camera;
  camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  camera.LookAt({-5, 0, 0}, {0, 0, 0}, {0, 0, 1});
  xiiView* views[2];
  for (auto& view : views)
  {
    module->CreateView("Authored native visibility", view);
    view->SetCamera(&camera);
    view->SetViewport({0, 0, 64, 64});
    view->SetRenderPipelineResource(pipeline);
  }
  bool      sawResident = false;
  xiiUInt32 identity    = 0;
  for (xiiUInt32 phase = 0; phase < 5; ++phase)
  {
    if (phase == 4)
      world.DeleteObjectNow(objectHandle);
    world.Update();
    environment.GetDevice()->WaitIdle();
    const auto* instances = module->GetGpuInstanceData();
    if (!XII_TEST_BOOL(instances != nullptr))
      return;
    XII_TEST_INT(instances->GetInstanceCount(), phase == 4 ? 0 : 1);
    if (phase != 4)
    {
      identity = instances->GetInstances()[0].m_uiObjectId;
      xiiGpuInstanceSource source;
      XII_TEST_RESULT(instances->Resolve(identity, source));
      XII_TEST_BOOL(source.m_hOwnerObject == objectHandle);
    }
    else
    {
      xiiGpuInstanceSource source;
      XII_TEST_BOOL(instances->Resolve(identity, source).Failed());
    }
    const bool resident = phase != 4 && xiiGeometryResidencyManager::GetStats().m_uiResidentGeometryCount > 0;
    sawResident |= resident;
    xiiUInt32 checked = 0;
    for (auto* view : views)
    {
      const auto& execution = view->GetRenderPipelineInstance();
      const auto* compiled  = execution.GetPipeline();
      if (!XII_TEST_BOOL(compiled != nullptr))
        return;
      XII_TEST_BOOL(view->GetExtractionRequirements() == xiiRenderExtractionCategory::Geometry);
      xiiRenderGraphBufferHandle sharedUpload;
      xiiSharedPtr<xiiGALBuffer> history[2];
      xiiUInt32                  histories = 0;
      for (xiiUInt32 i = 0; i < compiled->GetDescriptor().m_Passes.GetCount(); ++i)
      {
        const auto& provenance = compiled->GetProvenance()[i];
        if (provenance.m_Element == dispatchId)
        {
          const bool filtered = provenance.m_InstancePath[0] == filteredId;
          const auto buffer   = view->GetRenderGraph()->GetExportedBuffer(execution.GetOutputBinding(i, "DispatchArguments")->m_hBuffer);
          xiiVec3U32 arguments;
          if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), buffer.Borrow(), 0, arguments))
            return;
          const xiiUInt32 count = resident && !filtered ? 1 : 0;
          XII_TEST_INT(arguments.x, count);
          XII_TEST_INT(arguments.y, count);
          XII_TEST_INT(arguments.z, count);
        }
        if (provenance.m_Element == meshletId)
        {
          const bool filtered       = provenance.m_InstancePath[0] == filteredId;
          auto       countBuffer    = view->GetRenderGraph()->GetExportedBuffer(execution.GetOutputBinding(i, "VisibleMeshletCount")->m_hBuffer);
          auto       overflowBuffer = view->GetRenderGraph()->GetExportedBuffer(execution.GetOutputBinding(i, "VisibleMeshletOverflow")->m_hBuffer);
          xiiUInt32  count, overflow;
          if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), countBuffer.Borrow(), 0, count) || !xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), overflowBuffer.Borrow(), 0, overflow))
            return;
          XII_TEST_INT(count, resident && !filtered ? 1 : 0);
          XII_TEST_INT(overflow, 0);
        }
        if (provenance.m_Element == drawId)
        {
          auto       commands = view->GetRenderGraph()->GetExportedBuffer(execution.GetOutputBinding(i, "DrawCommands")->m_hBuffer);
          auto       counter  = view->GetRenderGraph()->GetExportedBuffer(execution.GetOutputBinding(i, "DrawCommandCount")->m_hBuffer);
          xiiUInt32  count;
          xiiVec4U32 command;
          if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), counter.Borrow(), 0, count) || !xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), commands.Borrow(), 0, command))
            return;
          const bool visible = provenance.m_InstancePath[0] != filteredId;
          XII_TEST_INT(count, resident && visible ? 1 : 0);
          XII_TEST_INT(command.x, count ? 36 : 0);
          XII_TEST_INT(command.y, count ? 1 : 0);
          XII_TEST_INT(command.z, 0);
          XII_TEST_INT(command.w, 0);
        }
        if (materialTargets && provenance.m_Element == materialId)
        {
          const bool covered = resident && provenance.m_InstancePath[0] != filteredId;
          const auto texture = [&](xiiStringView pin) {
            return view->GetRenderGraph()->GetExportedTexture(execution.GetOutputBinding(i, pin)->m_hTexture);
          };
          xiiUInt32 objectId, albedo, normal, material, normalRoughness;
          struct Half2
          {
            XII_DECLARE_POD_TYPE();
            xiiFloat16 x, y;
          } velocity;
          struct Half4
          {
            XII_DECLARE_POD_TYPE();
            xiiFloat16 x, y, z, w;
          } emissive;
          if (!xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), texture("ObjectId").Borrow(), 32, 32, objectId) || !xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), texture("Albedo").Borrow(), 32, 32, albedo) || !xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), texture("Normal").Borrow(), 32, 32, normal) || !xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), texture("Material").Borrow(), 32, 32, material) || !xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), texture("NormalRoughness").Borrow(), 32, 32, normalRoughness) || !xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), texture("Velocity").Borrow(), 32, 32, velocity) || !xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), texture("Emissive").Borrow(), 32, 32, emissive))
            return;
          XII_TEST_INT(objectId, covered ? identity : 0);
          XII_TEST_FLOAT(float(velocity.x), 0, 0.0001f);
          XII_TEST_FLOAT(float(velocity.y), 0, 0.0001f);
          XII_TEST_BOOL(xiiMath::IsFinite(float(emissive.x)) && xiiMath::IsFinite(float(emissive.y)) && xiiMath::IsFinite(float(emissive.z)) && xiiMath::IsFinite(float(emissive.w)));
          if (covered)
          {
            XII_TEST_BOOL((albedo & 0xFFFFFFU) != 0);
            XII_TEST_BOOL((material & 0xFFU) >= 2);
            XII_TEST_BOOL(normal != 0 && normalRoughness != 0);
          }
          else
          {
            XII_TEST_INT(albedo, 0xFF000000U);
            XII_TEST_FLOAT(float(emissive.w), 0, 0);
          }
        }
        if (provenance.m_Element == surfaceId)
        {
          const bool filtered = provenance.m_InstancePath[0] == filteredId;
          auto       depth    = view->GetRenderGraph()->GetExportedTexture(execution.GetOutputBinding(i, "SceneDepth")->m_hTexture);
          float      depthPixel;
          if (!xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), depth.Borrow(), 32, 32, depthPixel))
            return;
          if (depthOnly)
          {
            const auto projected = view->GetViewProjectionMatrix(xiiCameraEye::Left) * xiiVec4(-0.5f, 0, 0, 1);
            XII_TEST_FLOAT(depthPixel, resident && !filtered ? projected.z / projected.w : 0.0f, 0.00001f);
          }
          else
          {
            auto       surface = view->GetRenderGraph()->GetExportedTexture(execution.GetOutputBinding(i, "Surface")->m_hTexture);
            xiiVec4U32 pixel;
            if (!xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), surface.Borrow(), 32, 32, pixel))
              return;
            XII_TEST_INT(pixel.x, resident && !filtered ? identity : 0);
            if (pixel.x != 0)
            {
              XII_TEST_FLOAT(depthPixel, std::bit_cast<float>(pixel.y), 0.00001f);
              XII_TEST_BOOL(depthPixel > 0 && depthPixel < 1);
              XII_TEST_BOOL(xiiMath::IsFinite(std::bit_cast<float>(pixel.z)) && xiiMath::IsFinite(std::bit_cast<float>(pixel.w)));
            }
            else
            {
              XII_TEST_FLOAT(depthPixel, 0, 0);
            }
          }
        }
        if (provenance.m_Element == uploadId)
        {
          const auto buffer = execution.GetOutputBinding(i, "SceneInstances")->m_hBuffer;
          if (!sharedUpload.IsValid())
            sharedUpload = buffer;
          XII_TEST_BOOL(buffer == sharedUpload);
        }
        if (provenance.m_Element != cullId)
          continue;
        const bool filtered = provenance.m_InstancePath[0] == filteredId;
        const auto count    = view->GetRenderGraph()->GetExportedBuffer(execution.GetOutputBinding(i, "VisibleInstanceCount")->m_hBuffer);
        xiiUInt32  visible  = xiiInvalidIndex;
        if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), count.Borrow(), 0, visible))
          return;
        XII_TEST_INT(visible, resident && !filtered ? 1 : 0);
        history[histories++] = view->GetRenderGraph()->GetExportedBuffer(execution.GetOutputBinding(i, "LodHistory")->m_hBuffer);
        ++checked;
      }
      XII_TEST_INT(histories, 2);
      XII_TEST_BOOL(history[0] && history[1] && history[0] != history[1]);
    }
    XII_TEST_INT(checked, 4);
  }
  XII_TEST_BOOL(sawResident);
}

XII_CREATE_SIMPLE_TEST(NativeVisibility, AuthoredWorldVisibilityInstances) { TestAuthoredWorldVisibility(false); }
XII_CREATE_SIMPLE_TEST(NativeVisibility, AuthoredDepthOnlyInstances) { TestAuthoredWorldVisibility(true); }

XII_CREATE_SIMPLE_TEST(NativeVisibility, AuthoredGBufferInstances) { TestAuthoredWorldVisibility(true, true); }
