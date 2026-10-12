/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Foundation/IO/MemoryStream.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Meshes/MeshBufferResource.h>

XII_CREATE_SIMPLE_TEST_GROUP(Meshes);

XII_CREATE_SIMPLE_TEST(Meshes, MeshletConstruction)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Residency LOD ranges are reflected and default to the complete buffer")
  {
    xiiGeometryLodSource source;
    XII_TEST_INT(source.m_uiFirstMeshlet, 0U);
    XII_TEST_INT(source.m_uiMeshletCount, 0U);

    const xiiRTTI* pType = xiiGetStaticRTTI<xiiGeometryLodSource>();
    XII_TEST_BOOL(pType != nullptr);
    XII_TEST_BOOL(pType->FindPropertyByName("FirstMeshlet") != nullptr);
    XII_TEST_BOOL(pType->FindPropertyByName("MeshletCount") != nullptr);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Optimized cluster stream invariants")
  {
    xiiGeometry geometry;
    geometry.AddGeodesicSphere(2.0f, 3U);

    xiiMeshBufferResourceDescriptor descriptor;
    descriptor.AllocateStreamsFromGeometry(geometry, xiiGALPrimitiveTopology::TriangleList, true);

    XII_TEST_BOOL(descriptor.m_Meshlets.GetCount() > 1U);
    xiiUInt32 uiPrimitiveCount = 0U;
    for (const xiiMeshlet& meshlet : descriptor.m_Meshlets)
    {
      XII_TEST_BOOL(meshlet.m_uiVertexCount <= xiiMeshlet::s_uiMaxVertices);
      XII_TEST_BOOL(meshlet.m_uiPrimitiveCount <= xiiMeshlet::s_uiMaxPrimitives);
      XII_TEST_BOOL(meshlet.m_Bounds.IsValid());
      XII_TEST_BOOL(meshlet.m_uiVertexRemapOffset + meshlet.m_uiVertexCount <= descriptor.m_MeshletVertexRemap.GetCount());
      XII_TEST_BOOL(meshlet.m_uiPrimitiveIndexOffset + meshlet.m_uiPrimitiveCount * 3U <= descriptor.m_MeshletPrimitiveIndices.GetCount());

      for (xiiUInt32 i = 0U; i < meshlet.m_uiPrimitiveCount * 3U; ++i)
        XII_TEST_BOOL(descriptor.m_MeshletPrimitiveIndices[meshlet.m_uiPrimitiveIndexOffset + i] < meshlet.m_uiVertexCount);

      uiPrimitiveCount += meshlet.m_uiPrimitiveCount;
    }

    XII_TEST_INT(uiPrimitiveCount, geometry.CalculateTriangleCount());
    XII_TEST_INT(descriptor.m_MeshletMaterialIndices.GetCount(), descriptor.m_Meshlets.GetCount());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Closed clusters disable their degenerate normal cones")
  {
    for (const float size : {1.0f, 2.0f})
    {
      xiiGeometry geometry;
      geometry.AddBox(xiiVec3(size), true);
      xiiMeshBufferResourceDescriptor descriptor;
      descriptor.AllocateStreamsFromGeometry(geometry, xiiGALPrimitiveTopology::TriangleList, true);
      XII_TEST_INT(descriptor.m_Meshlets.GetCount(), 1);
      for (const auto& meshlet : descriptor.m_Meshlets)
        XII_TEST_FLOAT(meshlet.m_fConeCutoff, -1.0f, 0.0f);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GPU padding stays transient in existing mesh asset serialization")
  {
    xiiMeshlet source;
    source.m_uiFirstPrimitive = 17;
    source.m_uiPrimitiveCount = 12;
    source.m_uiVertexCount    = 24;
    source.m_uiGpuPadding[0] = source.m_uiGpuPadding[1] = 0xFFFFFFFFU;
    source.m_Bounds                                     = xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3(3, 4, 5), 2);
    source.m_vConeAxis                                  = xiiVec3::MakeAxisX();
    source.m_fConeCutoff                                = 0.5f;
    xiiDefaultMemoryStreamStorage storage;
    xiiMemoryStreamWriter         writer(&storage);
    XII_TEST_RESULT(source.Serialize(writer));
    XII_TEST_INT(storage.GetStorageSize64(), 56); // Existing portable asset payload, without GPU alignment.
    xiiMemoryStreamReader reader(&storage);
    xiiMeshlet            loaded = source;
    XII_TEST_RESULT(loaded.Deserialize(reader));
    XII_TEST_INT(loaded.m_uiGpuPadding[0], 0);
    XII_TEST_INT(loaded.m_uiGpuPadding[1], 0);
    XII_TEST_INT(loaded.m_uiFirstPrimitive, 17);
    XII_TEST_INT(loaded.m_uiPrimitiveCount, 12);
    XII_TEST_INT(loaded.m_uiVertexCount, 24);
    XII_TEST_VEC3(loaded.m_Bounds.m_vCenter, xiiVec3(3, 4, 5), 0);
    XII_TEST_FLOAT(loaded.m_Bounds.m_fRadius, 2, 0);
    XII_TEST_VEC3(loaded.m_vConeAxis, xiiVec3::MakeAxisX(), 0);
    XII_TEST_FLOAT(loaded.m_fConeCutoff, 0.5f, 0);
  }
}
