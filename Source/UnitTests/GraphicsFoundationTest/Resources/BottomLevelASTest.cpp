/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsFoundation/Resources/BottomLevelAS.h>

XII_CREATE_SIMPLE_TEST(Resources, BottomLevelAS)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Descriptor defaults")
  {
    xiiGALBottomLevelASCreationDescription description;
    XII_TEST_BOOL(description.m_Triangles.IsEmpty());
    XII_TEST_BOOL(description.m_BoundingBoxes.IsEmpty());
    XII_TEST_BOOL(description.m_BuildASFlags.IsNoFlagSet());
    XII_TEST_INT(description.m_uiCompactedSize, 0U);

    xiiGALBLASTriangleDescription triangle;
    XII_TEST_BOOL(triangle.m_sGeometryName.IsEmpty());
    XII_TEST_BOOL(triangle.m_VertexValueType == xiiGALValueType::Undefined);
    XII_TEST_BOOL(triangle.m_IndexType == xiiGALValueType::Undefined);
    XII_TEST_INT(triangle.m_uiMaxVertexCount, 0U);
    XII_TEST_INT(triangle.m_uiMaxPrimitiveCount, 0U);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Feature-gated device creation and geometry lookup")
  {
    for (xiiUInt32 uiImplementation = 0; uiImplementation < xiiGetGPUTestingEnvironmentCount(); ++uiImplementation)
    {
      xiiGPUTestingEnvironment environment(xiiGetGPUTestingEnvironmentName(uiImplementation));
      XII_TEST_BOOL(environment.Initialize().Succeeded());
      if (environment.GetDevice() == nullptr)
        continue;

      if (environment.GetDevice()->GetGraphicsDeviceAdapterProperties().m_Features.m_RayTracing != xiiGALDeviceFeatureState::Enabled)
        continue;

      xiiGALBottomLevelASCreationDescription triangleDescription;
      triangleDescription.m_BuildASFlags = xiiGALRayTracingBuildASFlags::PreferFastTrace | xiiGALRayTracingBuildASFlags::AllowUpdate;
      auto& triangle                     = triangleDescription.m_Triangles.ExpandAndGetRef();
      triangle.m_sGeometryName           = "TriangleGeometry";
      triangle.m_uiMaxVertexCount        = 3U;
      triangle.m_VertexValueType         = xiiGALValueType::Float32;
      triangle.m_uiVertexComponentCount  = 3U;
      triangle.m_uiMaxPrimitiveCount     = 1U;
      triangle.m_IndexType               = xiiGALValueType::Undefined;

      xiiSharedPtr<xiiGALBottomLevelAS> pTriangles = environment.GetDevice()->CreateBottomLevelAS(triangleDescription);
      XII_TEST_BOOL(pTriangles != nullptr);
      if (pTriangles != nullptr)
      {
        const xiiGALBottomLevelASCreationDescription& retained = pTriangles->GetDescription();
        XII_TEST_INT(retained.m_Triangles.GetCount(), 1U);
        XII_TEST_STRING(retained.m_Triangles[0].m_sGeometryName, "TriangleGeometry");
        XII_TEST_INT(retained.m_Triangles[0].m_uiMaxVertexCount, 3U);
        XII_TEST_BOOL(retained.m_BuildASFlags == triangleDescription.m_BuildASFlags);
        XII_TEST_INT(pTriangles->GetActualGeometryCount(), 1U);
        XII_TEST_INT(pTriangles->GetGeometryDescriptionIndex("TriangleGeometry"), 0U);
        XII_TEST_INT(pTriangles->GetGeometryIndex("TriangleGeometry"), 0U);
        XII_TEST_INT(pTriangles->GetGeometryIndex("MissingGeometry"), xiiInvalidIndex);
        XII_TEST_BOOL(pTriangles->GetScratchBufferSizeDescription().m_uiBuild > 0U);
        pTriangles->SetDebugName("Unit Test Triangle BLAS");
        XII_TEST_STRING(pTriangles->GetDebugName(), "Unit Test Triangle BLAS");
      }

      xiiGALBottomLevelASCreationDescription boxDescription;
      auto&                                  box = boxDescription.m_BoundingBoxes.ExpandAndGetRef();
      box.m_sGeometryName                        = "Bounds";
      box.m_uiMaxBoxCount                        = 2U;
      xiiSharedPtr<xiiGALBottomLevelAS> pBoxes   = environment.GetDevice()->CreateBottomLevelAS(boxDescription);
      XII_TEST_BOOL(pBoxes != nullptr);
      if (pBoxes != nullptr)
      {
        XII_TEST_INT(pBoxes->GetActualGeometryCount(), 1U);
        XII_TEST_INT(pBoxes->GetGeometryDescriptionIndex("Bounds"), 0U);
        XII_TEST_INT(pBoxes->GetGeometryIndex("Bounds"), 0U);
        XII_TEST_BOOL(pBoxes->GetScratchBufferSizeDescription().m_uiBuild > 0U);
      }
    }
  }
}
