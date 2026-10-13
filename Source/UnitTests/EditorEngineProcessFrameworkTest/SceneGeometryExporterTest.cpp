/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Interfaces/NavmeshGeoWorldModule.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Core/World/WorldDesc.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>
#include <SharedPluginScene/Common/Messages.h>
#include <SharedPluginScene/Scene/SceneGeometryExporter.h>

namespace
{
  class ExportCollisionComponent;
  using ExportCollisionManager = xiiComponentManager<ExportCollisionComponent, xiiBlockStorageType::FreeList>;
  class ExportCollisionComponent : public xiiComponent
  {
    XII_DECLARE_COMPONENT_TYPE(ExportCollisionComponent, xiiComponent, ExportCollisionManager);

  public:
    bool m_bInvalid = false;
    void Build(xiiMsgBuildStaticMesh& message)
    {
      auto&      data  = *message.m_pStaticMeshDescription;
      const auto first = data.m_Vertices.GetCount();
      for (auto point : {xiiVec3(0, 0, 0), xiiVec3(1, 0, 0), xiiVec3(0, 1, 0)})
        data.m_Vertices.PushBack(GetOwner()->GetGlobalTransform().TransformPosition(point));
      xiiSmcTriangle triangle;
      triangle.m_uiVertexIndices[0] = first;
      triangle.m_uiVertexIndices[1] = first + 1;
      triangle.m_uiVertexIndices[2] = m_bInvalid ? first + 99 : first + 2;
      data.m_Triangles.PushBack(triangle);
    }
  };
  // clang-format off
  XII_BEGIN_COMPONENT_TYPE(ExportCollisionComponent, 1, xiiComponentMode::Static)
  {
    XII_BEGIN_MESSAGEHANDLERS
    {
      XII_MESSAGE_HANDLER(xiiMsgBuildStaticMesh, Build),
    }
    XII_END_MESSAGEHANDLERS;
  }
  XII_END_COMPONENT_TYPE;
  // clang-format on

  class ExportNavmeshModule : public xiiNavmeshGeoWorldModuleInterface
  {
    XII_ADD_DYNAMIC_REFLECTION(ExportNavmeshModule, xiiNavmeshGeoWorldModuleInterface);
    XII_DECLARE_WORLD_MODULE();

  public:
    ExportNavmeshModule(xiiWorld* world) : xiiNavmeshGeoWorldModuleInterface(world) {}
    void RetrieveGeometryInArea(xiiUInt32 layer, const xiiBoundingBox& bounds, xiiDynamicArray<xiiNavmeshTriangle>& triangles) const override
    {
      XII_TEST_INT(layer, 0);
      XII_TEST_BOOL(bounds.IsValid());
      auto& triangle         = triangles.ExpandAndGetRef();
      triangle.m_Vertices[0] = xiiVec3(0, 0, 0);
      triangle.m_Vertices[1] = xiiVec3(1, 0, 0);
      triangle.m_Vertices[2] = xiiVec3(0, 1, 0);
    }
  };
  // clang-format off
  XII_BEGIN_DYNAMIC_REFLECTED_TYPE(ExportNavmeshModule, 1, xiiRTTINoAllocator)
  XII_END_DYNAMIC_REFLECTED_TYPE;
  XII_IMPLEMENT_WORLD_MODULE(ExportNavmeshModule);
  // clang-format on

  struct ExportedObj
  {
    xiiDynamicArray<xiiVec3>    m_Vertices;
    xiiDynamicArray<xiiVec3U32> m_Faces;
    xiiString                   m_Text;

    xiiResult Load(xiiStringView path)
    {
      xiiFileReader reader;
      XII_SUCCEED_OR_RETURN(reader.Open(path));
      xiiStringBuilder text;
      text.ReadAll(reader);
      m_Text = text;
      m_Vertices.Clear();
      m_Faces.Clear();
      xiiHybridArray<xiiStringView, 64> lines;
      text.Split(false, lines, "\n");
      for (auto line : lines)
      {
        xiiHybridArray<xiiStringView, 4> values;
        line.Split(false, values, " ");
        if (values.GetCount() != 4)
          return XII_FAILURE;
        if (values[0] == "v")
        {
          double components[3];
          for (xiiUInt32 i = 0; i < 3; ++i)
            XII_SUCCEED_OR_RETURN(xiiConversionUtils::StringToFloat(values[i + 1], components[i]));
          m_Vertices.PushBack(xiiVec3(float(components[0]), float(components[1]), float(components[2])));
        }
        else if (values[0] == "f")
        {
          xiiVec3U32 face;
          for (xiiUInt32 i = 0; i < 3; ++i)
            XII_SUCCEED_OR_RETURN(xiiConversionUtils::StringToUInt(values[i + 1], face.GetData()[i]));
          m_Faces.PushBack(face);
        }
        else
          return XII_FAILURE;
      }
      for (const auto& face : m_Faces)
        for (xiiUInt32 i = 0; i < 3; ++i)
          if (face.GetData()[i] == 0 || face.GetData()[i] > m_Vertices.GetCount())
            return XII_FAILURE;
      return XII_SUCCESS;
    }
  };

  xiiMeshResourceHandle CreateExportMesh(xiiStringView id, bool cpuReadable, bool customInfluences = false)
  {
    xiiGeometry geometry;
    geometry.AddVertex(xiiVec3(0, 0, 0), xiiVec3(0, 0, 1));
    geometry.AddVertex(xiiVec3(1, 0, 0), xiiVec3(0, 0, 1));
    geometry.AddVertex(xiiVec3(0, 1, 0), xiiVec3(0, 0, 1));
    geometry.AddVertex(xiiVec3(1, 1, 0), xiiVec3(0, 0, 1));
    if (customInfluences)
    {
      auto& vertices            = geometry.GetVertices();
      vertices[0].m_BoneIndices = xiiVec4U16(0, 1, 65535, 65535);
      vertices[0].m_BoneWeights = xiiColorLinearUB(60, 20, 0, 0);
      vertices[1].m_BoneIndices = xiiVec4U16(65535);
      vertices[1].m_BoneWeights = xiiColorLinearUB(0, 0, 0, 0);
    }
    xiiUInt32 a[] = {0, 1, 2}, b[] = {1, 3, 2};
    geometry.AddPolygon(a, false);
    geometry.AddPolygon(b, false);
    xiiMeshResourceDescriptor mesh;
    mesh.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(geometry, xiiGALPrimitiveTopology::TriangleList, false);
    if (cpuReadable)
    {
      const xiiUInt16 indices[] = {0, 1, 2, 1, 3, 2};
      mesh.GetMeshBufferDescriptor().SetIndexData(xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(indices), sizeof(indices)), 6, xiiGALValueType::UInt16);
    }
    mesh.AddMaterialSlot("");
    mesh.AddSection(1, 0, 0);
    mesh.AddSection(1, 1, 0);
    mesh.m_UsageFlags.AddOrRemove(xiiMeshResourceUsageFlags::CpuReadable, cpuReadable);
    return xiiResourceManager::CreateResource<xiiMeshResource>(id, std::move(mesh));
  }
} // namespace

XII_CREATE_SIMPLE_TEST(EditorViews, SceneGeometryExporter)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  const auto          cpuMesh = CreateExportMesh("EditorExportCpuMesh", true);
  const auto          gpuMesh = CreateExportMesh("EditorExportGpuMesh", false);
  xiiWorldDescription description("Scene geometry export");
  xiiWorld            world(description);
  XII_LOCK(world.GetWriteMarker());
  world.SetWorldSimulationEnabled(false);
  xiiGameObjectDescription object;
  object.m_bDynamic           = true;
  object.m_LocalPosition      = xiiVec3(10, 20, 30);
  xiiGameObject*    cpuObject = nullptr;
  const auto        cpuHandle = world.CreateObject(object, cpuObject);
  xiiMeshComponent* cpu       = nullptr;
  xiiMeshComponent::CreateComponent(cpuObject, cpu);
  cpu->SetMesh(cpuMesh);
  cpu->SetSectionIndex(1);
  object.m_LocalPosition               = xiiVec3::MakeZero();
  xiiGameObject*             gpuObject = nullptr;
  const auto                 gpuHandle = world.CreateObject(object, gpuObject);
  xiiInstancedMeshComponent* gpu       = nullptr;
  xiiInstancedMeshComponent::CreateComponent(gpuObject, gpu);
  gpu->SetMesh(gpuMesh);
  gpu->AddInstance(xiiMat4::MakeTranslation(xiiVec3(2, 0, 0)));
  gpu->AddInstance(xiiMat4::MakeScaling(xiiVec3(-1, 1, 1)));
  world.Update();
  const xiiStringView path = ":imgout/SceneGeometry.obj";
  ExportedObj         result;

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Native sections, GPU-only readback, local instances and mirrored winding")
  {
    xiiResourceLock<xiiMeshResource>       mesh(gpuMesh, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    xiiResourceLock<xiiMeshBufferResource> buffer(mesh->GetMeshBuffer(), xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    XII_TEST_BOOL(buffer->GetDescriptor().GetVertexData().IsEmpty());
    XII_TEST_BOOL(buffer->GetDescriptor().GetIndexData().IsEmpty());
    XII_TEST_STATUS(xiiSceneGeometryExporter::ExportOBJ(world, path, xiiSceneGeometryExportMode::RenderMesh, xiiMat3::MakeIdentity()));
    XII_TEST_RESULT(result.Load(path));
    XII_TEST_INT(result.m_Vertices.GetCount(), 12);
    XII_TEST_INT(result.m_Faces.GetCount(), 5);
    XII_TEST_VEC3(result.m_Vertices[0], xiiVec3(10, 20, 30), 0.00001f);
    XII_TEST_VEC3(result.m_Vertices[4], xiiVec3(2, 0, 0), 0.00001f);
    XII_TEST_VEC3(result.m_Vertices[9], xiiVec3(-1, 0, 0), 0.00001f);
    XII_TEST_BOOL(result.m_Faces[0] == xiiVec3U32(2, 4, 3));
    XII_TEST_BOOL(result.m_Faces[3] == xiiVec3U32(9, 11, 10));
    const auto text = result.m_Text;
    XII_TEST_STATUS(xiiSceneGeometryExporter::ExportOBJ(world, path, xiiSceneGeometryExportMode::RenderMesh, xiiMat3::MakeIdentity()));
    XII_TEST_RESULT(result.Load(path));
    XII_TEST_STRING(text, result.m_Text);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Selection, empty selection, Editor exclusion and coordinate conversion")
  {
    xiiGameObjectHandle selection[] = {cpuHandle};
    auto                coordinates = xiiMat3::MakeIdentity();
    coordinates.SetColumn(0, xiiVec3(0, 0, -1));
    coordinates.SetColumn(1, xiiVec3(1, 0, 0));
    coordinates.SetColumn(2, xiiVec3(0, 1, 0));
    XII_TEST_STATUS(xiiSceneGeometryExporter::ExportOBJ(world, path, xiiSceneGeometryExportMode::RenderMesh, coordinates, true, selection));
    XII_TEST_RESULT(result.Load(path));
    XII_TEST_INT(result.m_Faces.GetCount(), 1);
    XII_TEST_VEC3(result.m_Vertices[0], xiiVec3(20, 30, -10), 0.00001f);
    XII_TEST_STATUS(xiiSceneGeometryExporter::ExportOBJ(world, path, xiiSceneGeometryExportMode::RenderMesh, coordinates, true));
    XII_TEST_RESULT(result.Load(path));
    XII_TEST_BOOL(result.m_Text.IsEmpty());
    gpuObject->SetTag(xiiTagRegistry::GetGlobalRegistry().RegisterTag("Editor"));
    XII_TEST_STATUS(xiiSceneGeometryExporter::ExportOBJ(world, path, xiiSceneGeometryExportMode::RenderMesh, coordinates));
    XII_TEST_RESULT(result.Load(path));
    XII_TEST_INT(result.m_Faces.GetCount(), 1);
    gpuObject->RemoveTag(xiiTagRegistry::GetGlobalRegistry().RegisterTag("Editor"));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Current skinning and failed exports preserve the published file")
  {
    xiiGameObject* skinnedObject = nullptr;
    world.CreateObject(object, skinnedObject);
    xiiSkinnedMeshComponent* skinned = nullptr;
    xiiSkinnedMeshComponent::CreateComponent(skinnedObject, skinned);
    skinned->SetMesh(cpuMesh);
    const xiiMat4 matrices[] = {xiiMat4::MakeTranslation(xiiVec3(0, 0, 7))};
    XII_TEST_RESULT(skinned->SetSkinningMatrices(matrices));
    world.Update();
    xiiGameObjectHandle selection[] = {skinnedObject->GetHandle()};
    XII_TEST_STATUS(xiiSceneGeometryExporter::ExportOBJ(world, path, xiiSceneGeometryExportMode::RenderMesh, xiiMat3::MakeIdentity(), true, selection));
    XII_TEST_RESULT(result.Load(path));
    XII_TEST_VEC3(result.m_Vertices[0], xiiVec3(0, 0, 7), 0.00001f);
    const auto previous = result.m_Text;
    cpu->SetSectionIndex(99);
    XII_TEST_BOOL(xiiSceneGeometryExporter::ExportOBJ(world, path, xiiSceneGeometryExportMode::RenderMesh, xiiMat3::MakeIdentity()).Failed());
    cpu->SetSectionIndex(1);
    const float morph[] = {1};
    skinned->SetMorphWeights(morph);
    XII_TEST_BOOL(xiiSceneGeometryExporter::ExportOBJ(world, path, xiiSceneGeometryExportMode::RenderMesh, xiiMat3::MakeIdentity(), true, selection).Failed());
    XII_TEST_BOOL(xiiSceneGeometryExporter::ExportOBJ(world, path, xiiSceneGeometryExportMode::NavmeshObstacles, xiiMat3::MakeIdentity()).Failed());
    XII_TEST_BOOL(xiiSceneGeometryExporter::ExportOBJ(world, path, xiiSceneGeometryExportMode::CollisionMesh, xiiMat3::MakeIdentity()).Failed());
    XII_TEST_BOOL(xiiSceneGeometryExporter::ExportOBJ(world, path, static_cast<xiiSceneGeometryExportMode>(99), xiiMat3::MakeIdentity()).Failed());
    XII_TEST_BOOL(xiiSceneGeometryExporter::ExportOBJ(world, path, xiiSceneGeometryExportMode::RenderMesh, xiiMat3::MakeZero()).Failed());
    XII_TEST_RESULT(result.Load(path));
    XII_TEST_STRING(result.m_Text, previous);
    world.DeleteObjectNow(gpuHandle);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Native skinning agrees for CPU data and GPU readback, including zero weights")
  {
    for (bool cpuReadable : {true, false})
    {
      const auto               mesh            = CreateExportMesh(cpuReadable ? "EditorExportWeightedCpuMesh" : "EditorExportWeightedGpuMesh", cpuReadable, true);
      xiiGameObject*           owner           = nullptr;
      const auto               handle          = world.CreateObject(object, owner);
      xiiSkinnedMeshComponent* component       = nullptr;
      const auto               componentHandle = xiiSkinnedMeshComponent::CreateComponent(owner, component);
      component->SetMesh(mesh);
      const xiiMat4 palette[] = {xiiMat4::MakeTranslation(xiiVec3(0, 0, 7)), xiiMat4::MakeTranslation(xiiVec3(0, 0, 15))};
      XII_TEST_RESULT(component->SetSkinningMatrices(palette));
      world.Update();
      if (!XII_TEST_BOOL(world.TryGetComponent(componentHandle, component)))
        return;
      const xiiGameObjectHandle selection[] = {handle};
      XII_TEST_STATUS(xiiSceneGeometryExporter::ExportOBJ(world, path, xiiSceneGeometryExportMode::RenderMesh, xiiMat3::MakeIdentity(), true, selection));
      XII_TEST_RESULT(result.Load(path));
      XII_TEST_VEC3(result.m_Vertices[0], xiiVec3(0, 0, 9), 0.00001f);
      XII_TEST_VEC3(result.m_Vertices[1], xiiVec3(1, 0, 0), 0.00001f);
      XII_TEST_VEC3(result.m_Vertices[2], xiiVec3(0, 1, 7), 0.00001f);
      const auto previous = result.m_Text;
      XII_TEST_RESULT(component->SetSkinningMatrices(xiiMakeArrayPtr(palette, 1)));
      XII_TEST_BOOL(xiiSceneGeometryExporter::ExportOBJ(world, path, xiiSceneGeometryExportMode::RenderMesh, xiiMat3::MakeIdentity(), true, selection).Failed());
      XII_TEST_RESULT(result.Load(path));
      XII_TEST_STRING(result.m_Text, previous);
      component->ClearPose();
      world.Update();
      XII_TEST_STATUS(xiiSceneGeometryExporter::ExportOBJ(world, path, xiiSceneGeometryExportMode::RenderMesh, xiiMat3::MakeIdentity(), true, selection));
      XII_TEST_RESULT(result.Load(path));
      XII_TEST_VEC3(result.m_Vertices[0], xiiVec3::MakeZero(), 0.00001f);
      XII_TEST_VEC3(result.m_Vertices[1], xiiVec3(1, 0, 0), 0.00001f);
      world.DeleteObjectNow(handle);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Geometry export IPC reports the actual engine result")
  {
    xiiExportSceneGeometryResultMsgToEditor message;
    message.m_DocumentGuid = xiiUuid::MakeUuid();
    message.m_sOutputFile  = "Export/Scene.obj";
    message.m_sFailure     = "No native collision provider.";
    xiiDefaultMemoryStreamStorage storage;
    xiiMemoryStreamWriter         writer(&storage);
    xiiReflectionSerializer::WriteObjectToBinary(writer, message.GetDynamicRTTI(), &message);
    xiiMemoryStreamReader                   reader(&storage);
    xiiExportSceneGeometryResultMsgToEditor decoded;
    xiiReflectionSerializer::ReadObjectPropertiesFromBinary(reader, *decoded.GetDynamicRTTI(), &decoded);
    XII_TEST_BOOL(!decoded.m_bSucceeded);
    XII_TEST_BOOL(decoded.m_DocumentGuid == message.m_DocumentGuid);
    XII_TEST_STRING(decoded.m_sOutputFile, message.m_sOutputFile);
    XII_TEST_STRING(decoded.m_sFailure, message.m_sFailure);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Native collision and navmesh providers, mirrored faces and provider validation")
  {
    ExportCollisionComponent* provider = nullptr;
    ExportCollisionComponent::CreateComponent(cpuObject, provider);
    world.Update();
    auto coordinates = xiiMat3::MakeIdentity();
    coordinates.SetColumn(0, xiiVec3(-1, 0, 0));
    XII_TEST_STATUS(xiiSceneGeometryExporter::ExportOBJ(world, path, xiiSceneGeometryExportMode::CollisionMesh, coordinates));
    XII_TEST_RESULT(result.Load(path));
    XII_TEST_VEC3(result.m_Vertices[0], xiiVec3(-10, 20, 30), 0.00001f);
    XII_TEST_BOOL(result.m_Faces[0] == xiiVec3U32(1, 3, 2));
    const auto previous  = result.m_Text;
    provider->m_bInvalid = true;
    XII_TEST_BOOL(xiiSceneGeometryExporter::ExportOBJ(world, path, xiiSceneGeometryExportMode::CollisionMesh, coordinates).Failed());
    XII_TEST_RESULT(result.Load(path));
    XII_TEST_STRING(result.m_Text, previous);
    const auto* navigation = world.GetOrCreateModule<ExportNavmeshModule>();
    XII_TEST_BOOL(world.GetModule<xiiNavmeshGeoWorldModuleInterface>() == navigation);
    XII_TEST_STATUS(xiiSceneGeometryExporter::ExportOBJ(world, path, xiiSceneGeometryExportMode::NavmeshObstacles, coordinates));
    XII_TEST_RESULT(result.Load(path));
    XII_TEST_INT(result.m_Faces.GetCount(), 1);
    XII_TEST_BOOL(result.m_Faces[0] == xiiVec3U32(1, 3, 2));
    XII_TEST_BOOL(xiiSceneGeometryExporter::ExportOBJ(world, path, xiiSceneGeometryExportMode::NavmeshObstacles, coordinates, true).Failed());
  }
}
