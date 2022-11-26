#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/AI/PointOfInterestGraph.h>

struct xiiDummyPointType
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 value;
};

void CompileDummy()
{
  xiiPointOfInterestGraph<xiiDummyPointType> graph;
  graph.Initialize(xiiVec3::ZeroVector(), xiiVec3::ZeroVector());
  auto& pt = graph.AddPoint(xiiVec3::ZeroVector());

  xiiDynamicArray<xiiUInt32> points;
  graph.FindPointsOfInterest(xiiVec3::ZeroVector(), 0, points);
}


XII_STATICLINK_FILE(GameEngine, GameEngine_AI_Implementation_PointOfInterestGraph);
