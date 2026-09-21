/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/World/Declarations.h>
#include <Utilities/UtilitiesDLL.h>

class xiiWorld;
class xiiDGMLGraph;

/// This class encapsulates creating graphs from various core engine structures (like the game object graph etc.)
class XII_UTILITIES_DLL xiiDGMLGraphCreator
{
public:
  /// Adds the world hierarchy (game objects and components) to the given graph object.
  static void FillGraphFromWorld(xiiWorld* pWorld, xiiDGMLGraph& ref_graph);
};
