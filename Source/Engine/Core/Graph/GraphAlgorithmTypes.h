/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/CoreDLL.h>

#include <Core/Graph/GraphHandles.h>
#include <Foundation/Types/Delegate.h>

/// Optional edge selection, e.g. connection category, enabled state or execution/data flow. Empty means all edges.
/// Selection must remain consistent during a call; algorithms may examine an edge more than once.
using xiiGraphEdgeFilter = xiiDelegate<bool(xiiGraphEdgeHandle)>;

/// Consumer-defined weight/capacity. Callbacks must not mutate the topology or reenter the same workspace.
/// Returned values must remain consistent during a call. Algorithms document their numeric restrictions.
using xiiGraphEdgeWeight = xiiDelegate<double(xiiGraphEdgeHandle)>;
