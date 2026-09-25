/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

#include <Core/Graphics/Geometry.h>
#include <GraphicsCore/Meshes/MeshBufferResource.h>

XII_CREATE_SIMPLE_TEST_GROUP(Meshes);

XII_CREATE_SIMPLE_TEST(Meshes, MeshletConstruction)
{
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
}
