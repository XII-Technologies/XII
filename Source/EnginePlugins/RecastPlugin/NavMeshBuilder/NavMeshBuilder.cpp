#include <RecastPlugin/RecastPluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <Foundation/Utilities/Progress.h>
#include <Recast/DetourNavMesh.h>
#include <Recast/DetourNavMeshBuilder.h>
#include <Recast/Recast.h>
#include <RecastPlugin/NavMeshBuilder/NavMeshBuilder.h>
#include <RecastPlugin/Resources/RecastNavMeshResource.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRecastConfig, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiRecastConfig>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("AgentHeight", m_fAgentHeight)->AddAttributes(new xiiDefaultValueAttribute(1.5f)),
    XII_MEMBER_PROPERTY("AgentRadius", m_fAgentRadius)->AddAttributes(new xiiDefaultValueAttribute(0.3f)),
    XII_MEMBER_PROPERTY("AgentClimbHeight", m_fAgentClimbHeight)->AddAttributes(new xiiDefaultValueAttribute(0.4f)),
    XII_MEMBER_PROPERTY("WalkableSlope", m_WalkableSlope)->AddAttributes(new xiiDefaultValueAttribute(xiiAngle::Degree(45))),
    XII_MEMBER_PROPERTY("CellSize", m_fCellSize)->AddAttributes(new xiiDefaultValueAttribute(0.2f)),
    XII_MEMBER_PROPERTY("CellHeight", m_fCellHeight)->AddAttributes(new xiiDefaultValueAttribute(0.2f)),
    XII_MEMBER_PROPERTY("MinRegionSize", m_fMinRegionSize)->AddAttributes(new xiiDefaultValueAttribute(3.0f)),
    XII_MEMBER_PROPERTY("RegionMergeSize", m_fRegionMergeSize)->AddAttributes(new xiiDefaultValueAttribute(20.0f)),
    XII_MEMBER_PROPERTY("SampleDistanceFactor", m_fDetailMeshSampleDistanceFactor)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_MEMBER_PROPERTY("SampleErrorFactor", m_fDetailMeshSampleErrorFactor)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_MEMBER_PROPERTY("MaxSimplification", m_fMaxSimplificationError)->AddAttributes(new xiiDefaultValueAttribute(1.3f)),
    XII_MEMBER_PROPERTY("MaxEdgeLength", m_fMaxEdgeLength)->AddAttributes(new xiiDefaultValueAttribute(4.0f)),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

class xiiRcBuildContext : public rcContext
{
public:
  xiiRcBuildContext() = default;

protected:
  virtual void doLog(const rcLogCategory category, const char* msg, const int len)
  {
    switch (category)
    {
      case RC_LOG_ERROR:
        xiiLog::Error("Recast: {0}", msg);
        return;
      case RC_LOG_WARNING:
        xiiLog::Warning("Recast: {0}", msg);
        return;
      case RC_LOG_PROGRESS:
        xiiLog::Debug("Recast: {0}", msg);
        return;

      default:
        xiiLog::Error("Unknwon recast log: {0}", msg);
        return;
    }
  }
};

xiiRecastNavMeshBuilder::xiiRecastNavMeshBuilder()  = default;
xiiRecastNavMeshBuilder::~xiiRecastNavMeshBuilder() = default;

void xiiRecastNavMeshBuilder::Clear()
{
  m_BoundingBox.SetInvalid();
  m_Vertices.Clear();
  m_Triangles.Clear();
  m_TriangleAreaIDs.Clear();
  m_pRecastContext = nullptr;
}

xiiResult xiiRecastNavMeshBuilder::ExtractWorldGeometry(const xiiWorld& world, xiiWorldGeoExtractionUtil::MeshObjectList& out_worldGeo)
{
  xiiWorldGeoExtractionUtil::ExtractWorldGeometry(out_worldGeo, world, xiiWorldGeoExtractionUtil::ExtractionMode::NavMeshGeneration);

  return XII_SUCCESS;
}

xiiResult xiiRecastNavMeshBuilder::Build(const xiiRecastConfig& config, const xiiWorldGeoExtractionUtil::MeshObjectList& geo, xiiRecastNavMeshResourceDescriptor& out_navMeshDesc, xiiProgress& ref_progress)
{
  XII_LOG_BLOCK("xiiRecastNavMeshBuilder::Build");

  xiiProgressRange pg("Generating NavMesh", 4, true, &ref_progress);
  pg.SetStepWeighting(0, 0.1f);
  pg.SetStepWeighting(1, 0.1f);
  pg.SetStepWeighting(2, 0.6f);
  pg.SetStepWeighting(3, 0.2f);

  Clear();
  out_navMeshDesc.Clear();

  xiiUniquePtr<xiiRcBuildContext> recastContext = XII_DEFAULT_NEW(xiiRcBuildContext);
  m_pRecastContext                              = recastContext.Borrow();

  if (!pg.BeginNextStep("Triangulate Mesh"))
    return XII_FAILURE;

  GenerateTriangleMeshFromDescription(geo);

  if (m_Vertices.IsEmpty())
  {
    xiiLog::Debug("Navmesh is empty");
    return XII_SUCCESS;
  }

  if (!pg.BeginNextStep("Compute AABB"))
    return XII_FAILURE;

  ComputeBoundingBox();

  if (!pg.BeginNextStep("Build Poly Mesh"))
    return XII_FAILURE;

  out_navMeshDesc.m_pNavMeshPolygons = XII_DEFAULT_NEW(rcPolyMesh);

  if (BuildRecastPolyMesh(config, *out_navMeshDesc.m_pNavMeshPolygons, ref_progress).Failed())
    return XII_FAILURE;

  if (!pg.BeginNextStep("Build NavMesh"))
    return XII_FAILURE;

  if (BuildDetourNavMeshData(config, *out_navMeshDesc.m_pNavMeshPolygons, out_navMeshDesc.m_DetourNavmeshData).Failed())
    return XII_FAILURE;

  return XII_SUCCESS;
}

void xiiRecastNavMeshBuilder::GenerateTriangleMeshFromDescription(const xiiWorldGeoExtractionUtil::MeshObjectList& objects)
{
  XII_LOG_BLOCK("xiiRecastNavMeshBuilder::GenerateTriangleMesh");

  m_Triangles.Clear();
  m_TriangleAreaIDs.Clear();
  m_Vertices.Clear();


  xiiUInt32 uiVertexOffset = 0;
  for (const xiiWorldGeoExtractionUtil::MeshObject& object : objects)
  {
    xiiResourceLock<xiiCpuMeshResource> pCpuMesh(object.m_hMeshResource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pCpuMesh.GetAcquireResult() != xiiResourceAcquireResult::Final)
    {
      continue;
    }

    const auto& meshBufferDesc = pCpuMesh->GetDescriptor().MeshBufferDesc();

    m_Triangles.Reserve(m_Triangles.GetCount() + meshBufferDesc.GetPrimitiveCount());
    m_Vertices.Reserve(m_Vertices.GetCount() + meshBufferDesc.GetVertexCount());

    const xiiVec3* pPositions      = nullptr;
    xiiUInt32      uiElementStride = 0;
    if (xiiMeshBufferUtils::GetPositionStream(meshBufferDesc, pPositions, uiElementStride).Failed())
    {
      continue;
    }

    // convert from XII convention (Z up) to recast convention (Y up)
    xiiMat3 m;
    m.SetRow(0, xiiVec3(1, 0, 0));
    m.SetRow(1, xiiVec3(0, 0, 1));
    m.SetRow(2, xiiVec3(0, 1, 0));

    xiiMat4 transform = xiiMat4::IdentityMatrix();
    transform.SetRotationalPart(m);
    transform = transform * object.m_GlobalTransform.GetAsMat4();

    // collect all vertices
    for (xiiUInt32 i = 0; i < meshBufferDesc.GetVertexCount(); ++i)
    {
      xiiVec3 pos = transform.TransformPosition(*pPositions);

      m_Vertices.PushBack(pos);

      pPositions = xiiMemoryUtils::AddByteOffset(pPositions, uiElementStride);
    }

    // collect all indices
    bool flip = xiiGraphicsUtils::IsTriangleFlipRequired(transform.GetRotationalPart());

    if (meshBufferDesc.HasIndexBuffer())
    {
      if (meshBufferDesc.Uses32BitIndices())
      {
        const xiiUInt32* pTypedIndices = reinterpret_cast<const xiiUInt32*>(meshBufferDesc.GetIndexBufferData().GetPtr());

        for (xiiUInt32 p = 0; p < meshBufferDesc.GetPrimitiveCount(); ++p)
        {
          auto& triangle          = m_Triangles.ExpandAndGetRef();
          triangle.m_VertexIdx[0] = pTypedIndices[p * 3 + (flip ? 2 : 0)] + uiVertexOffset;
          triangle.m_VertexIdx[1] = pTypedIndices[p * 3 + 1] + uiVertexOffset;
          triangle.m_VertexIdx[2] = pTypedIndices[p * 3 + (flip ? 0 : 2)] + uiVertexOffset;
        }
      }
      else
      {
        const xiiUInt16* pTypedIndices = reinterpret_cast<const xiiUInt16*>(meshBufferDesc.GetIndexBufferData().GetPtr());

        for (xiiUInt32 p = 0; p < meshBufferDesc.GetPrimitiveCount(); ++p)
        {
          auto& triangle          = m_Triangles.ExpandAndGetRef();
          triangle.m_VertexIdx[0] = pTypedIndices[p * 3 + (flip ? 2 : 0)] + uiVertexOffset;
          triangle.m_VertexIdx[1] = pTypedIndices[p * 3 + 1] + uiVertexOffset;
          triangle.m_VertexIdx[2] = pTypedIndices[p * 3 + (flip ? 0 : 2)] + uiVertexOffset;
        }
      }
    }
    else
    {
      xiiUInt32 uiVertexIdx = uiVertexOffset;

      for (xiiUInt32 p = 0; p < meshBufferDesc.GetPrimitiveCount(); ++p)
      {
        auto& triangle          = m_Triangles.ExpandAndGetRef();
        triangle.m_VertexIdx[0] = uiVertexIdx + 0;
        triangle.m_VertexIdx[1] = uiVertexIdx + (flip ? 2 : 1);
        triangle.m_VertexIdx[2] = uiVertexIdx + (flip ? 1 : 2);

        uiVertexIdx += 3;
      }
    }

    uiVertexOffset += meshBufferDesc.GetVertexCount();
  }

  // initialize the IDs to zero
  m_TriangleAreaIDs.SetCount(m_Triangles.GetCount());

  xiiLog::Debug("Vertices: {0}, Triangles: {1}", m_Vertices.GetCount(), m_Triangles.GetCount());
}


void xiiRecastNavMeshBuilder::ComputeBoundingBox()
{
  if (!m_Vertices.IsEmpty())
  {
    m_BoundingBox.SetFromPoints(m_Vertices.GetData(), m_Vertices.GetCount());
  }
}

void xiiRecastNavMeshBuilder::FillOutConfig(rcConfig& cfg, const xiiRecastConfig& config, const xiiBoundingBox& bbox)
{
  xiiMemoryUtils::ZeroFill(&cfg, 1);
  cfg.bmin[0]                = bbox.m_vMin.x;
  cfg.bmin[1]                = bbox.m_vMin.y;
  cfg.bmin[2]                = bbox.m_vMin.z;
  cfg.bmax[0]                = bbox.m_vMax.x;
  cfg.bmax[1]                = bbox.m_vMax.y;
  cfg.bmax[2]                = bbox.m_vMax.z;
  cfg.ch                     = config.m_fCellHeight;
  cfg.cs                     = config.m_fCellSize;
  cfg.walkableSlopeAngle     = config.m_WalkableSlope.GetDegree();
  cfg.walkableHeight         = (int)ceilf(config.m_fAgentHeight / cfg.ch);
  cfg.walkableClimb          = (int)floorf(config.m_fAgentClimbHeight / cfg.ch);
  cfg.walkableRadius         = (int)ceilf(config.m_fAgentRadius / cfg.cs);
  cfg.maxEdgeLen             = (int)(config.m_fMaxEdgeLength / cfg.cs);
  cfg.maxSimplificationError = config.m_fMaxSimplificationError;
  cfg.minRegionArea          = (int)xiiMath::Square(config.m_fMinRegionSize);
  cfg.mergeRegionArea        = (int)xiiMath::Square(config.m_fRegionMergeSize);
  cfg.maxVertsPerPoly        = 6;
  cfg.detailSampleDist       = config.m_fDetailMeshSampleDistanceFactor < 0.9f ? 0 : cfg.cs * config.m_fDetailMeshSampleDistanceFactor;
  cfg.detailSampleMaxError   = cfg.ch * config.m_fDetailMeshSampleErrorFactor;

  rcCalcGridSize(cfg.bmin, cfg.bmax, cfg.cs, &cfg.width, &cfg.height);
}

xiiResult xiiRecastNavMeshBuilder::BuildRecastPolyMesh(const xiiRecastConfig& config, rcPolyMesh& out_PolyMesh, xiiProgress& progress)
{
  xiiProgressRange pgRange("Build Poly Mesh", 13, true, &progress);

  rcConfig cfg;
  FillOutConfig(cfg, config, m_BoundingBox);

  xiiRcBuildContext* pContext   = m_pRecastContext;
  const float*       pVertices  = &m_Vertices[0].x;
  const xiiInt32*    pTriangles = &m_Triangles[0].m_VertexIdx[0];

  rcHeightfield* heightfield = rcAllocHeightfield();
  XII_SCOPE_EXIT(rcFreeHeightField(heightfield));

  if (!pgRange.BeginNextStep("Creating Heightfield"))
    return XII_FAILURE;

  if (!rcCreateHeightfield(pContext, *heightfield, cfg.width, cfg.height, cfg.bmin, cfg.bmax, cfg.cs, cfg.ch))
  {
    pContext->log(RC_LOG_ERROR, "Could not create solid heightfield");
    return XII_FAILURE;
  }

  if (!pgRange.BeginNextStep("Mark Walkable Area"))
    return XII_FAILURE;

  // TODO Instead of this, it should use area IDs and then clear the non-walkable triangles
  rcMarkWalkableTriangles(
    pContext, cfg.walkableSlopeAngle, pVertices, m_Vertices.GetCount(), pTriangles, m_Triangles.GetCount(), m_TriangleAreaIDs.GetData());

  if (!pgRange.BeginNextStep("Rasterize Triangles"))
    return XII_FAILURE;

  if (!rcRasterizeTriangles(
        pContext, pVertices, m_Vertices.GetCount(), pTriangles, m_TriangleAreaIDs.GetData(), m_Triangles.GetCount(), *heightfield, cfg.walkableClimb))
  {
    pContext->log(RC_LOG_ERROR, "Could not rasterize triangles");
    return XII_FAILURE;
  }

  // Optional stuff
  {
    if (!pgRange.BeginNextStep("Filter Low Hanging Obstacles"))
      return XII_FAILURE;

    // if (m_filterLowHangingObstacles)
    rcFilterLowHangingWalkableObstacles(pContext, cfg.walkableClimb, *heightfield);

    if (!pgRange.BeginNextStep("Filter Ledge Spans"))
      return XII_FAILURE;

    // if (m_filterLedgeSpans)
    rcFilterLedgeSpans(pContext, cfg.walkableHeight, cfg.walkableClimb, *heightfield);

    if (!pgRange.BeginNextStep("Filter Low Height Spans"))
      return XII_FAILURE;

    // if (m_filterWalkableLowHeightSpans)
    rcFilterWalkableLowHeightSpans(pContext, cfg.walkableHeight, *heightfield);
  }

  if (!pgRange.BeginNextStep("Build Compact Heightfield"))
    return XII_FAILURE;

  rcCompactHeightfield* compactHeightfield = rcAllocCompactHeightfield();
  XII_SCOPE_EXIT(rcFreeCompactHeightfield(compactHeightfield));

  if (!rcBuildCompactHeightfield(pContext, cfg.walkableHeight, cfg.walkableClimb, *heightfield, *compactHeightfield))
  {
    pContext->log(RC_LOG_ERROR, "Could not build compact data");
    return XII_FAILURE;
  }

  if (!pgRange.BeginNextStep("Erode Walkable Area"))
    return XII_FAILURE;

  if (!rcErodeWalkableArea(pContext, cfg.walkableRadius, *compactHeightfield))
  {
    pContext->log(RC_LOG_ERROR, "Could not erode with character radius");
    return XII_FAILURE;
  }

  // (Optional) Mark areas.
  //{
  //  const ConvexVolume* vols = m_geom->getConvexVolumes();
  //  for (int i = 0; i < m_geom->getConvexVolumeCount(); ++i)
  //    rcMarkConvexPolyArea(pContext, vols[i].verts, vols[i].nverts, vols[i].hmin, vols[i].hmax, (unsigned char)vols[i].area,
  //    *compactHeightfield);
  //}


  // Partition the heightfield so that we can use simple algorithm later to triangulate the walkable areas.
  // Default algorithm is 'Watershed'
  {
    // PARTITION_WATERSHED
    {
      if (!pgRange.BeginNextStep("Build Distance Field"))
        return XII_FAILURE;

      // Prepare for region partitioning, by calculating distance field along the walkable surface.
      if (!rcBuildDistanceField(pContext, *compactHeightfield))
      {
        pContext->log(RC_LOG_ERROR, "Could not build distance field.");
        return XII_FAILURE;
      }

      if (!pgRange.BeginNextStep("Build Regions"))
        return XII_FAILURE;

      // Partition the walkable surface into simple regions without holes.
      if (!rcBuildRegions(pContext, *compactHeightfield, 0, cfg.minRegionArea, cfg.mergeRegionArea))
      {
        pContext->log(RC_LOG_ERROR, "Could not build watershed regions.");
        return XII_FAILURE;
      }
    }

    //// PARTITION_MONOTONE
    //{
    //  // Partition the walkable surface into simple regions without holes.
    //  // Monotone partitioning does not need distance field.
    //  if (!rcBuildRegionsMonotone(pContext, *compactHeightfield, 0, cfg.minRegionArea, cfg.mergeRegionArea))
    //  {
    //    pContext->log(RC_LOG_ERROR, "Could not build monotone regions.");
    //    return XII_FAILURE;
    //  }
    //}

    //// PARTITION_LAYERS
    //{
    //  // Partition the walkable surface into simple regions without holes.
    //  if (!rcBuildLayerRegions(pContext, *compactHeightfield, 0, cfg.minRegionArea))
    //  {
    //    pContext->log(RC_LOG_ERROR, "Could not build layer regions.");
    //    return XII_FAILURE;
    //  }
    //}
  }

  if (!pgRange.BeginNextStep("Build Contours"))
    return XII_FAILURE;

  rcContourSet* contourSet = rcAllocContourSet();
  XII_SCOPE_EXIT(rcFreeContourSet(contourSet));

  if (!rcBuildContours(pContext, *compactHeightfield, cfg.maxSimplificationError, cfg.maxEdgeLen, *contourSet))
  {
    pContext->log(RC_LOG_ERROR, "Could not create contours");
    return XII_FAILURE;
  }

  if (!pgRange.BeginNextStep("Build Poly Mesh"))
    return XII_FAILURE;

  if (!rcBuildPolyMesh(pContext, *contourSet, cfg.maxVertsPerPoly, out_PolyMesh))
  {
    pContext->log(RC_LOG_ERROR, "Could not triangulate contours");
    return XII_FAILURE;
  }

  //////////////////////////////////////////////////////////////////////////
  // Detour Navmesh

  if (!pgRange.BeginNextStep("Set Area Flags"))
    return XII_FAILURE;

  // TODO modify area IDs and flags

  for (int i = 0; i < out_PolyMesh.npolys; ++i)
  {
    if (out_PolyMesh.areas[i] == RC_WALKABLE_AREA)
    {
      out_PolyMesh.flags[i] = 0xFFFF;
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiRecastNavMeshBuilder::BuildDetourNavMeshData(const xiiRecastConfig& config, const rcPolyMesh& polyMesh, xiiDataBuffer& NavmeshData)
{
  dtNavMeshCreateParams params;
  xiiMemoryUtils::ZeroFill(&params, 1);

  params.verts          = polyMesh.verts;
  params.vertCount      = polyMesh.nverts;
  params.polys          = polyMesh.polys;
  params.polyAreas      = polyMesh.areas;
  params.polyFlags      = polyMesh.flags;
  params.polyCount      = polyMesh.npolys;
  params.nvp            = polyMesh.nvp;
  params.walkableHeight = config.m_fAgentHeight;
  params.walkableRadius = config.m_fAgentRadius;
  params.walkableClimb  = config.m_fAgentClimbHeight;
  rcVcopy(params.bmin, polyMesh.bmin);
  rcVcopy(params.bmax, polyMesh.bmax);
  params.cs          = config.m_fCellSize;
  params.ch          = config.m_fCellHeight;
  params.buildBvTree = true;

  xiiUInt8* navData     = nullptr;
  xiiInt32  navDataSize = 0;

  if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
  {
    xiiLog::Error("Could not build Detour navmesh.");
    return XII_FAILURE;
  }

  NavmeshData.SetCountUninitialized(navDataSize);
  xiiMemoryUtils::Copy(NavmeshData.GetData(), navData, navDataSize);

  dtFree(navData);
  return XII_SUCCESS;
}

xiiResult xiiRecastConfig::Serialize(xiiStreamWriter& ref_stream) const
{
  ref_stream.WriteVersion(1);

  ref_stream << m_fAgentHeight;
  ref_stream << m_fAgentRadius;
  ref_stream << m_fAgentClimbHeight;
  ref_stream << m_WalkableSlope;
  ref_stream << m_fCellSize;
  ref_stream << m_fCellHeight;
  ref_stream << m_fMaxEdgeLength;
  ref_stream << m_fMaxSimplificationError;
  ref_stream << m_fMinRegionSize;
  ref_stream << m_fRegionMergeSize;
  ref_stream << m_fDetailMeshSampleDistanceFactor;
  ref_stream << m_fDetailMeshSampleErrorFactor;

  return XII_SUCCESS;
}

xiiResult xiiRecastConfig::Deserialize(xiiStreamReader& ref_stream)
{
  ref_stream.ReadVersion(1);

  ref_stream >> m_fAgentHeight;
  ref_stream >> m_fAgentRadius;
  ref_stream >> m_fAgentClimbHeight;
  ref_stream >> m_WalkableSlope;
  ref_stream >> m_fCellSize;
  ref_stream >> m_fCellHeight;
  ref_stream >> m_fMaxEdgeLength;
  ref_stream >> m_fMaxSimplificationError;
  ref_stream >> m_fMinRegionSize;
  ref_stream >> m_fRegionMergeSize;
  ref_stream >> m_fDetailMeshSampleDistanceFactor;
  ref_stream >> m_fDetailMeshSampleErrorFactor;

  return XII_SUCCESS;
}
