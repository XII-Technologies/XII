#pragma once

#include <Foundation/Time/Time.h>
#include <GameEngine/AI/PointOfInterestGraph.h>
#include <RecastPlugin/RecastPluginDLL.h>

struct rcPolyMesh;

struct XII_RECASTPLUGIN_DLL xiiNavMeshPointsOfInterest
{
  xiiVec3   m_vFloorPosition;
  xiiUInt32 m_uiVisibleMarker = 0;
};

class XII_RECASTPLUGIN_DLL xiiNavMeshPointOfInterestGraph
{
public:
  xiiNavMeshPointOfInterestGraph();
  ~xiiNavMeshPointOfInterestGraph();

  void ExtractInterestPointsFromMesh(const rcPolyMesh& mesh, bool bReinitialize = true /* bad interface design */);

  xiiUInt32 GetCheckVisibilityTimeStamp() const { return m_uiCheckVisibilityTimeStamp; }
  void      IncreaseCheckVisibiblityTimeStamp(xiiTime tNow);

  xiiPointOfInterestGraph<xiiNavMeshPointsOfInterest>&       GetGraph() { return m_NavMeshPointGraph; }
  const xiiPointOfInterestGraph<xiiNavMeshPointsOfInterest>& GetGraph() const { return m_NavMeshPointGraph; }

protected:
  xiiTime                                             m_LastTimeStampStep;
  xiiUInt32                                           m_uiCheckVisibilityTimeStamp = 100;
  xiiPointOfInterestGraph<xiiNavMeshPointsOfInterest> m_NavMeshPointGraph;
};
