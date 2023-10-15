#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Resources/BottomLevelASVulkan.h>

xiiGALBottomLevelASVulkan::xiiGALBottomLevelASVulkan(const xiiGALBottomLevelASCreationDescription& creationDescription) :
  xiiGALBottomLevelAS(creationDescription)
{
}

xiiGALBottomLevelASVulkan::~xiiGALBottomLevelASVulkan() = default;

xiiResult xiiGALBottomLevelASVulkan::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(pDevice);

  Diligent::BottomLevelASDesc bottomLevelASDescription;
  bottomLevelASDescription.Name                 = m_Description.m_sName.GetStartPointer();
  bottomLevelASDescription.Flags                = xiiDiligentTypeConversions::GetRayTracingBuildASFlags(m_Description.m_BuildASFlags);
  bottomLevelASDescription.CompactedSize        = m_Description.m_uiCompactedSize;
  bottomLevelASDescription.ImmediateContextMask = m_Description.m_uiImmediateContextMask;

  const xiiUInt32 uiTriangleCount = m_Description.m_Triangles.GetCount();

  xiiHybridArray<Diligent::BLASTriangleDesc, 16U> triangles;
  triangles.SetCount(uiTriangleCount);

  for (xiiUInt32 i = 0; i < uiTriangleCount; ++i)
  {
    const auto& xiiBLASTriangle = m_Description.m_Triangles[i];
    auto&       blasTriangle    = triangles[i];

    blasTriangle.GeometryName         = xiiBLASTriangle.m_sGeometryName.GetStartPointer();
    blasTriangle.MaxVertexCount       = xiiBLASTriangle.m_uiMaxVertexCount;
    blasTriangle.VertexValueType      = xiiDiligentTypeConversions::GetValueType(xiiBLASTriangle.m_VertexValueType);
    blasTriangle.VertexComponentCount = xiiBLASTriangle.m_uiVertexComponentCount;
    blasTriangle.MaxPrimitiveCount    = xiiBLASTriangle.m_uiMaxPrimitiveCount;
    blasTriangle.IndexType            = xiiDiligentTypeConversions::GetValueType(xiiBLASTriangle.m_IndexType);
  }
  bottomLevelASDescription.TriangleCount = uiTriangleCount;
  bottomLevelASDescription.pTriangles    = triangles.GetData();

  const xiiUInt32 uiBoxCount = m_Description.m_BoundingBoxes.GetCount();

  xiiHybridArray<Diligent::BLASBoundingBoxDesc, 16U> boxes;
  boxes.SetCount(uiBoxCount);

  for (xiiUInt32 i = 0; i < uiBoxCount; ++i)
  {
    const auto& xiiBox = m_Description.m_BoundingBoxes[i];
    auto&       box    = boxes[i];

    box.GeometryName = xiiBox.m_sGeometryName.GetStartPointer();
    box.MaxBoxCount  = xiiBox.m_uiMaxBoxCount;
  }
  bottomLevelASDescription.BoxCount = uiBoxCount;
  bottomLevelASDescription.pBoxes   = boxes.GetData();

  pDeviceVulkan->GetDevice()->CreateBLAS(bottomLevelASDescription, &m_pBottomLevelAS);

  return m_pBottomLevelAS == nullptr ? XII_FAILURE : XII_SUCCESS;
}

xiiResult xiiGALBottomLevelASVulkan::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DILIGENT_REF_RELEASE(m_pBottomLevelAS);

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Resources_Implementation_BottomLevelASVulkan);
