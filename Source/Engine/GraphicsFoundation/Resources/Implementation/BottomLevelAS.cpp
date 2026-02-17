#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Resources/BottomLevelAS.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiGALRayTracingBuildASFlags, 1)
  XII_BITFLAGS_CONSTANT(xiiGALRayTracingBuildASFlags::AllowUpdate),
  XII_BITFLAGS_CONSTANT(xiiGALRayTracingBuildASFlags::AllowCompaction),
  XII_BITFLAGS_CONSTANT(xiiGALRayTracingBuildASFlags::PreferFastTrace),
  XII_BITFLAGS_CONSTANT(xiiGALRayTracingBuildASFlags::PreferFastBuild),
  XII_BITFLAGS_CONSTANT(xiiGALRayTracingBuildASFlags::LowMemory),
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALBottomLevelAS, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALBottomLevelAS::xiiGALBottomLevelAS(xiiSharedPtr<xiiGALDevice> pDevice, const xiiGALBottomLevelASCreationDescription& creationDescription) :
  xiiGALResource(std::move(pDevice)), m_Description(creationDescription)
{
  // Build name->index map for fast lookups. Uses shader-binding ordering (triangles first, then boxes).
  const xiiUInt32 uiTriangleCount = m_Description.m_Triangles.GetCount();

  for (xiiUInt32 i = 0U; i < uiTriangleCount; ++i)
  {
    const xiiString& sName = m_Description.m_Triangles[i].m_sGeometryName;

    if (!sName.IsEmpty())
    {
      m_NameToIndex.Insert(sName.GetView(), i);
    }
  }

  for (xiiUInt32 i = 0; i < m_Description.m_BoundingBoxes.GetCount(); ++i)
  {
    const xiiString& sName = m_Description.m_BoundingBoxes[i].m_sGeometryName;

    if (!sName.IsEmpty())
    {
      m_NameToIndex.Insert(sName.GetView(), uiTriangleCount + i);
    }
  }
}

xiiGALBottomLevelAS::~xiiGALBottomLevelAS() = default;

xiiUInt32 xiiGALBottomLevelAS::GetGeometryDescriptionIndex(xiiStringView sName) const
{
  XII_ASSERT_DEV(!sName.IsEmpty(), "Geometry name must not be empty.");

  xiiUInt32 uiIndex = 0U;
  if (m_NameToIndex.TryGetValue(sName, uiIndex))
  {
    const xiiUInt32 uiTriangleCount = m_Description.m_Triangles.GetCount();

    if (uiIndex < uiTriangleCount)
    {
      return uiIndex; // Triangle description index.
    }
    return uiIndex - uiTriangleCount; // Bounding box description index.
  }
  return xiiInvalidIndex;
}

xiiUInt32 xiiGALBottomLevelAS::GetGeometryIndex(xiiStringView sName) const
{
  XII_ASSERT_DEV(!sName.IsEmpty(), "Geometry name must not be empty.");

  xiiUInt32 uiIndex = 0U;
  if (m_NameToIndex.TryGetValue(sName, uiIndex))
  {
    return uiIndex;
  }
  return xiiInvalidIndex;
}

xiiUInt32 xiiGALBottomLevelAS::GetActualGeometryCount() const
{
  return m_Description.m_Triangles.GetCount() + m_Description.m_BoundingBoxes.GetCount();
}

const xiiGALScratchBufferSizeDescription& xiiGALBottomLevelAS::GetScratchBufferSizeDescription() const
{
  return m_ScratchBufferSizeDescription;
}

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Resources_Implementation_BottomLevelAS);
