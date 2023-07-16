#pragma once

#include <Core/World/Declarations.h>
#include <Utilities/UtilitiesDLL.h>

class xiiWorld;
class xiiDGMLGraph;

/// \brief This class encapsulates creating graphs from various core engine structures (like the game object graph etc.)
class XII_UTILITIES_DLL xiiDGMLGraphCreator
{
public:
  /// \brief Adds the world hierarchy (game objects and components) to the given graph object.
  static void FillGraphFromWorld(xiiWorld* pWorld, xiiDGMLGraph& ref_graph);
};
