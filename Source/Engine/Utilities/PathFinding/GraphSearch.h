/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Math/Math.h>
#include <Utilities/PathFinding/PathState.h>
#include <Utilities/UtilitiesDLL.h>

/// Implements a directed breadth-first search through a graph (A*).
///
/// You can search for a path to a specific location using FindPath() or to the closest node that fulfills some arbitrary criteria
/// using FindClosest().
///
/// PathStateType must be derived from xiiPathState and can be used for keeping track of certain state along a path and to modify
/// the path search dynamically.
template <typename PathStateType>
class xiiPathSearch
{
public:
  /// Used by FindClosest() to query whether the currently visited node fulfills the termination criteria.
  typedef bool (*IsSearchedObjectCallback)(xiiInt64 iStartNodeIndex, const PathStateType& StartState);

  /// FindPath() and FindClosest() return an array of these objects as the path result.
  struct PathResultData
  {
    XII_DECLARE_POD_TYPE();

    /// The index of the node that was visited.
    xiiInt64 m_iNodeIndex;

    /// Pointer to the path state that was active at that step along the path.
    const PathStateType* m_pPathState;
  };

  /// Sets the xiiPathStateGenerator that should be used by this xiiPathSearch object.
  void SetPathStateGenerator(xiiPathStateGenerator<PathStateType>* pStateGenerator) { m_pStateGenerator = pStateGenerator; }

  /// Searches for a path that starts at the graph node \a iStartNodeIndex with the start state \a StartState and shall terminate
  /// when the graph node \a iTargetNodeIndex was reached.
  ///
  /// Returns XII_FAILURE if no path could be found.
  /// Returns the path result as a list of PathResultData objects in \a out_Path.
  ///
  /// The path search is stopped (and thus fails) if the path reaches costs of \a fMaxPathCost or higher.
  xiiResult FindPath(xiiInt64 iStartNodeIndex, const PathStateType& StartState, xiiInt64 iTargetNodeIndex, xiiDeque<PathResultData>& out_Path, float fMaxPathCost = xiiMath::Infinity<float>());

  /// Searches for a path that starts at the graph node \a iStartNodeIndex with the start state \a StartState and shall terminate
  /// when a graph node is reached for which \a Callback return true.
  ///
  /// Returns XII_FAILURE if no path could be found.
  /// Returns the path result as a list of PathResultData objects in \a out_Path.
  ///
  /// The path search is stopped (and thus fails) if the path reaches costs of \a fMaxPathCost or higher.
  xiiResult FindClosest(xiiInt64 iStartNodeIndex, const PathStateType& StartState, IsSearchedObjectCallback Callback, xiiDeque<PathResultData>& out_Path, float fMaxPathCost = xiiMath::Infinity<float>());

  /// Needs to be called by the used xiiPathStateGenerator to add nodes to evaluate.
  void AddPathNode(xiiInt64 iNodeIndex, const PathStateType& NewState);

private:
  void     ClearPathStates();
  xiiInt64 FindBestNodeToExpand(PathStateType*& out_pPathState);
  void     FillOutPathResult(xiiInt64 iEndNodeIndex, xiiDeque<PathResultData>& out_Path);

  xiiPathStateGenerator<PathStateType>* m_pStateGenerator;

  xiiHashTable<xiiInt64, PathStateType> m_PathStates;

  xiiDeque<xiiInt64> m_StateQueue;

  xiiInt64      m_iCurNodeIndex;
  PathStateType m_CurState;
};



#include <Utilities/PathFinding/Implementation/GraphSearch_inl.h>
