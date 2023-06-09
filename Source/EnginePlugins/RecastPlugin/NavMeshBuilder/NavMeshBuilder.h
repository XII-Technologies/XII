#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/UniquePtr.h>
#include <RecastPlugin/RecastPluginDLL.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

class xiiRcBuildContext;
struct rcPolyMesh;
struct rcPolyMeshDetail;
class xiiWorld;
class dtNavMesh;
struct xiiRecastNavMeshResourceDescriptor;
class xiiProgress;
class xiiStreamWriter;
class xiiStreamReader;

struct XII_RECASTPLUGIN_DLL xiiRecastConfig
{
  float    m_fAgentHeight                    = 1.5f;
  float    m_fAgentRadius                    = 0.3f;
  float    m_fAgentClimbHeight               = 0.4f;
  xiiAngle m_WalkableSlope                   = xiiAngle::Degree(45);
  float    m_fCellSize                       = 0.2f;
  float    m_fCellHeight                     = 0.2f;
  float    m_fMaxEdgeLength                  = 4.0f;
  float    m_fMaxSimplificationError         = 1.3f;
  float    m_fMinRegionSize                  = 3.0f;
  float    m_fRegionMergeSize                = 20.0f;
  float    m_fDetailMeshSampleDistanceFactor = 1.0f;
  float    m_fDetailMeshSampleErrorFactor    = 1.0f;

  xiiResult Serialize(xiiStreamWriter& ref_stream) const;
  xiiResult Deserialize(xiiStreamReader& ref_stream);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_RECASTPLUGIN_DLL, xiiRecastConfig);


class XII_RECASTPLUGIN_DLL xiiRecastNavMeshBuilder
{
public:
  xiiRecastNavMeshBuilder();
  ~xiiRecastNavMeshBuilder();

  static xiiResult ExtractWorldGeometry(const xiiWorld& world, xiiWorldGeoExtractionUtil::MeshObjectList& out_worldGeo);

  xiiResult Build(const xiiRecastConfig& config, const xiiWorldGeoExtractionUtil::MeshObjectList& worldGeo, xiiRecastNavMeshResourceDescriptor& out_navMeshDesc, xiiProgress& ref_progress);

private:
  static void FillOutConfig(struct rcConfig& cfg, const xiiRecastConfig& config, const xiiBoundingBox& bbox);

  void             Clear();
  void             GenerateTriangleMeshFromDescription(const xiiWorldGeoExtractionUtil::MeshObjectList& objects);
  void             ComputeBoundingBox();
  xiiResult        BuildRecastPolyMesh(const xiiRecastConfig& config, rcPolyMesh& out_PolyMesh, xiiProgress& progress);
  static xiiResult BuildDetourNavMeshData(const xiiRecastConfig& config, const rcPolyMesh& polyMesh, xiiDataBuffer& NavmeshData);

  struct Triangle
  {
    XII_DECLARE_POD_TYPE();

    Triangle() = default;
    Triangle(xiiInt32 a, xiiInt32 b, xiiInt32 c)
    {
      m_VertexIdx[0] = a;
      m_VertexIdx[1] = b;
      m_VertexIdx[2] = c;
    }

    xiiInt32 m_VertexIdx[3];
  };

  xiiBoundingBox            m_BoundingBox;
  xiiDynamicArray<xiiVec3>  m_Vertices;
  xiiDynamicArray<Triangle> m_Triangles;
  xiiDynamicArray<xiiUInt8> m_TriangleAreaIDs;
  xiiRcBuildContext*        m_pRecastContext = nullptr;
};
