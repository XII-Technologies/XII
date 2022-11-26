#include <BakingPlugin/BakingPluginPCH.h>

#include <BakingPlugin/BakingScene.h>
#include <BakingPlugin/Tracer/TracerEmbree.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>

#include <embree3/rtcore.h>

namespace
{
  static RTCDevice                                                                                          s_rtcDevice;
  static xiiHashTable<xiiHashedString, RTCScene, xiiHashHelper<xiiHashedString>, xiiStaticAllocatorWrapper> s_rtcMeshCache;

  const char* rtcErrorCodeToString[] = {
    "RTC_NO_ERROR",
    "RTC_UNKNOWN_ERROR",
    "RTC_INVALID_ARGUMENT",
    "RTC_INVALID_OPERATION",
    "RTC_OUT_OF_MEMORY",
    "RTC_UNSUPPORTED_CPU",
    "RTC_CANCELLED"};

  const char* GetStringFromRTCErrorCode(RTCError code)
  {
    return (code >= 0 && code < XII_ARRAY_SIZE(rtcErrorCodeToString)) ? rtcErrorCodeToString[code] : "RTC invalid error code";
  }

  static void ErrorCallback(void* userPtr, RTCError code, const char* str)
  {
    xiiLog::Error("Embree: {}: {}", GetStringFromRTCErrorCode(code), str);
  }

  static xiiResult InitDevice()
  {
    if (s_rtcDevice == nullptr)
    {
      if (s_rtcDevice = rtcNewDevice("threads=1"))
      {
        xiiLog::Info("Created new Embree Device (Version {})", RTC_VERSION_STRING);

        rtcSetDeviceErrorFunction(s_rtcDevice, &ErrorCallback, nullptr);

        bool bRay4Supported      = rtcGetDeviceProperty(s_rtcDevice, RTC_DEVICE_PROPERTY_NATIVE_RAY4_SUPPORTED);
        bool bRay8Supported      = rtcGetDeviceProperty(s_rtcDevice, RTC_DEVICE_PROPERTY_NATIVE_RAY8_SUPPORTED);
        bool bRay16Supported     = rtcGetDeviceProperty(s_rtcDevice, RTC_DEVICE_PROPERTY_NATIVE_RAY16_SUPPORTED);
        bool bRayStreamSupported = rtcGetDeviceProperty(s_rtcDevice, RTC_DEVICE_PROPERTY_RAY_STREAM_SUPPORTED);

        xiiLog::Info("Supported ray packets: Ray4:{}, Ray8:{}, Ray16:{}, RayStream:{}", bRay4Supported, bRay8Supported, bRay16Supported, bRayStreamSupported);
      }
      else
      {
        xiiLog::Error("Failed to create Embree Device. Error: {}", GetStringFromRTCErrorCode(rtcGetDeviceError(nullptr)));
        return XII_FAILURE;
      }
    }

    return XII_SUCCESS;
  }

  static void DeinitDevice()
  {
    for (auto it : s_rtcMeshCache)
    {
      rtcReleaseScene(it.Value());
    }
    s_rtcMeshCache.Clear();

    rtcReleaseDevice(s_rtcDevice);
    s_rtcDevice = nullptr;
  }

  static RTCScene GetOrCreateMesh(const xiiCpuMeshResourceHandle& hMeshResource)
  {
    xiiHashedString sResourceId;
    sResourceId.Assign(hMeshResource.GetResourceID());

    RTCScene scene = nullptr;
    if (s_rtcMeshCache.TryGetValue(sResourceId, scene))
    {
      return scene;
    }

    xiiResourceLock<xiiCpuMeshResource> pCpuMesh(hMeshResource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pCpuMesh.GetAcquireResult() != xiiResourceAcquireResult::Final)
    {
      xiiLog::Warning("Failed to retrieve CPU mesh '{}'", sResourceId);
      return nullptr;
    }

    RTCGeometry triangleMesh = rtcNewGeometry(s_rtcDevice, RTC_GEOMETRY_TYPE_TRIANGLE);
    {
      const auto& mbDesc = pCpuMesh->GetDescriptor().MeshBufferDesc();

      const xiiVec3*             pPositions      = nullptr;
      const xiiUInt8*            pNormals        = nullptr;
      xiiGALResourceFormat::Enum normalFormat    = xiiGALResourceFormat::Invalid;
      xiiUInt32                  uiElementStride = 0;
      if (xiiMeshBufferUtils::GetPositionAndNormalStream(mbDesc, pPositions, pNormals, normalFormat, uiElementStride).Failed())
      {
        return nullptr;
      }

      xiiVec3* rtcPositions = static_cast<xiiVec3*>(rtcSetNewGeometryBuffer(triangleMesh, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(xiiVec3), mbDesc.GetVertexCount()));

      rtcSetGeometryVertexAttributeCount(triangleMesh, 1);
      xiiVec3* rtcNormals = static_cast<xiiVec3*>(rtcSetNewGeometryBuffer(triangleMesh, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0, RTC_FORMAT_FLOAT3, sizeof(xiiVec3), mbDesc.GetVertexCount()));

      // write out all vertices
      xiiVec3 vNormal;
      for (xiiUInt32 i = 0; i < mbDesc.GetVertexCount(); ++i)
      {
        xiiMeshBufferUtils::DecodeNormal(xiiMakeArrayPtr(pNormals, sizeof(xiiVec3)), normalFormat, vNormal).IgnoreResult();

        rtcPositions[i] = *pPositions;
        rtcNormals[i]   = vNormal;

        pPositions = xiiMemoryUtils::AddByteOffset(pPositions, uiElementStride);
        pNormals   = xiiMemoryUtils::AddByteOffset(pNormals, uiElementStride);
      }

      xiiVec3U32* rtcIndices = static_cast<xiiVec3U32*>(rtcSetNewGeometryBuffer(triangleMesh, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(xiiVec3U32), mbDesc.GetPrimitiveCount()));

      bool flip = false;
      if (mbDesc.Uses32BitIndices())
      {
        const xiiUInt32* pTypedIndices = reinterpret_cast<const xiiUInt32*>(mbDesc.GetIndexBufferData().GetPtr());

        for (xiiUInt32 p = 0; p < mbDesc.GetPrimitiveCount(); ++p)
        {
          rtcIndices[p].x = pTypedIndices[p * 3 + (flip ? 2 : 0)];
          rtcIndices[p].y = pTypedIndices[p * 3 + 1];
          rtcIndices[p].z = pTypedIndices[p * 3 + (flip ? 0 : 2)];
        }
      }
      else
      {
        const xiiUInt16* pTypedIndices = reinterpret_cast<const xiiUInt16*>(mbDesc.GetIndexBufferData().GetPtr());

        for (xiiUInt32 p = 0; p < mbDesc.GetPrimitiveCount(); ++p)
        {
          rtcIndices[p].x = pTypedIndices[p * 3 + (flip ? 2 : 0)];
          rtcIndices[p].y = pTypedIndices[p * 3 + 1];
          rtcIndices[p].z = pTypedIndices[p * 3 + (flip ? 0 : 2)];
        }
      }

      rtcCommitGeometry(triangleMesh);
    }

    scene = rtcNewScene(s_rtcDevice);
    {
      XII_VERIFY(rtcAttachGeometry(scene, triangleMesh) == 0, "Geometry id must be 0");
      rtcReleaseGeometry(triangleMesh);

      rtcCommitScene(scene);
    }

    s_rtcMeshCache.Insert(sResourceId, scene);
    return scene;
  }

} // namespace

struct xiiTracerEmbree::Data
{
  ~Data()
  {
    ClearScene();
  }

  void ClearScene()
  {
    m_rtcInstancedGeometry.Clear();

    if (m_rtcScene != nullptr)
    {
      rtcReleaseScene(m_rtcScene);
      m_rtcScene = nullptr;
    }
  }

  RTCScene m_rtcScene = nullptr;

  struct InstancedGeometry
  {
    RTCGeometry  m_mesh;
    xiiSimdVec4f m_normalTransform0;
    xiiSimdVec4f m_normalTransform1;
    xiiSimdVec4f m_normalTransform2;
  };

  xiiDynamicArray<InstancedGeometry, xiiAlignedAllocatorWrapper> m_rtcInstancedGeometry;
};

xiiTracerEmbree::xiiTracerEmbree()
{
  m_pData = XII_DEFAULT_NEW(Data);
}

xiiTracerEmbree::~xiiTracerEmbree() = default;

xiiResult xiiTracerEmbree::BuildScene(const xiiBakingScene& scene)
{
  XII_SUCCEED_OR_RETURN(InitDevice());

  m_pData->ClearScene();
  m_pData->m_rtcScene = rtcNewScene(s_rtcDevice);

  for (auto& meshObject : scene.GetMeshObjects())
  {
    RTCScene mesh = GetOrCreateMesh(meshObject.m_hMeshResource);
    if (mesh == nullptr)
    {
      continue;
    }

    xiiMat4 transform = meshObject.m_GlobalTransform.GetAsMat4();

    RTCGeometry instance = rtcNewGeometry(s_rtcDevice, RTC_GEOMETRY_TYPE_INSTANCE);
    {
      rtcSetGeometryInstancedScene(instance, mesh);
      rtcSetGeometryTransform(instance, 0, RTC_FORMAT_FLOAT4X4_COLUMN_MAJOR, &transform);

      rtcCommitGeometry(instance);
    }

    xiiUInt32 uiInstanceID = rtcAttachGeometry(m_pData->m_rtcScene, instance);
    rtcReleaseGeometry(instance);

    xiiMat3 normalTransform = transform.GetRotationalPart().GetInverse(0.0f).GetTranspose();

    XII_ASSERT_DEBUG(uiInstanceID == m_pData->m_rtcInstancedGeometry.GetCount(), "");
    auto& instancedGeometry              = m_pData->m_rtcInstancedGeometry.ExpandAndGetRef();
    instancedGeometry.m_mesh             = rtcGetGeometry(mesh, 0);
    instancedGeometry.m_normalTransform0 = xiiSimdConversion::ToVec3(normalTransform.GetColumn(0));
    instancedGeometry.m_normalTransform1 = xiiSimdConversion::ToVec3(normalTransform.GetColumn(1));
    instancedGeometry.m_normalTransform2 = xiiSimdConversion::ToVec3(normalTransform.GetColumn(2));
  }

  rtcCommitScene(m_pData->m_rtcScene);

  return XII_SUCCESS;
}

XII_DEFINE_AS_POD_TYPE(RTCRayHit);

void xiiTracerEmbree::TraceRays(xiiArrayPtr<const Ray> rays, xiiArrayPtr<Hit> hits)
{
  const xiiUInt32 uiNumRays = rays.GetCount();

  xiiHybridArray<RTCRayHit, 256, xiiAlignedAllocatorWrapper> rtcRayHits;
  rtcRayHits.SetCountUninitialized(uiNumRays);

  for (xiiUInt32 i = 0; i < uiNumRays; ++i)
  {
    auto& ray       = rays[i];
    auto& rtcRayHit = rtcRayHits[i];

    rtcRayHit.ray.org_x = ray.m_vStartPos.x;
    rtcRayHit.ray.org_y = ray.m_vStartPos.y;
    rtcRayHit.ray.org_z = ray.m_vStartPos.z;
    rtcRayHit.ray.tnear = 0.0f;

    rtcRayHit.ray.dir_x = ray.m_vDir.x;
    rtcRayHit.ray.dir_y = ray.m_vDir.y;
    rtcRayHit.ray.dir_z = ray.m_vDir.z;
    rtcRayHit.ray.time  = 0.0f;

    rtcRayHit.ray.tfar  = ray.m_fDistance;
    rtcRayHit.ray.mask  = 0;
    rtcRayHit.ray.id    = i;
    rtcRayHit.ray.flags = 0;

    rtcRayHit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
  }

  RTCIntersectContext context;
  rtcInitIntersectContext(&context);

  rtcIntersect1M(m_pData->m_rtcScene, &context, rtcRayHits.GetData(), uiNumRays, sizeof(RTCRayHit));

  for (xiiUInt32 i = 0; i < uiNumRays; ++i)
  {
    auto& rtcRayHit = rtcRayHits[i];
    auto& ray       = rays[i];
    auto& hit       = hits[i];

    if (rtcRayHit.hit.geomID != RTC_INVALID_GEOMETRY_ID)
    {
      auto& instancedGeometry = m_pData->m_rtcInstancedGeometry[rtcRayHit.hit.instID[0]];

      xiiSimdVec4f objectSpaceNormal;
      rtcInterpolate0(instancedGeometry.m_mesh, rtcRayHit.hit.primID, rtcRayHit.hit.u, rtcRayHit.hit.v, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0, reinterpret_cast<float*>(&objectSpaceNormal), 3);

      xiiSimdVec4f worldSpaceNormal = instancedGeometry.m_normalTransform0 * objectSpaceNormal.x();
      worldSpaceNormal += instancedGeometry.m_normalTransform1 * objectSpaceNormal.y();
      worldSpaceNormal += instancedGeometry.m_normalTransform2 * objectSpaceNormal.z();

      hit.m_vNormal   = xiiSimdConversion::ToVec3(worldSpaceNormal.GetNormalized<3>());
      hit.m_fDistance = rtcRayHit.ray.tfar;
      hit.m_vPosition = ray.m_vStartPos + ray.m_vDir * hit.m_fDistance;
    }
    else
    {
      hit.m_vPosition.SetZero();
      hit.m_vNormal.SetZero();
      hit.m_fDistance = -1.0f;
    }
  }
}
