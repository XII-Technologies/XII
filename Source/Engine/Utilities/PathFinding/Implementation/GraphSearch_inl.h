#pragma once

template <typename PathStateType>
void xiiPathSearch<PathStateType>::ClearPathStates()
{
  m_PathStates.Clear();
  m_StateQueue.Clear();
}

template <typename PathStateType>
xiiInt64 xiiPathSearch<PathStateType>::FindBestNodeToExpand(PathStateType*& out_pPathState)
{
  float fLowestEstimation = xiiMath::Infinity<float>();
  out_pPathState          = nullptr;
  xiiUInt32 iBestInQueue  = 0;

  for (xiiUInt32 i = 0; i < m_StateQueue.GetCount(); ++i)
  {
    const xiiInt64 iNodeIndex = m_StateQueue[i];

    PathStateType* pState = &m_PathStates[iNodeIndex];

    if (pState->m_fEstimatedCostToTarget < fLowestEstimation)
    {
      fLowestEstimation = pState->m_fEstimatedCostToTarget;
      out_pPathState    = pState;
      iBestInQueue      = i;
    }
  }

  XII_ASSERT_DEV(out_pPathState != nullptr, "Implementation Error");

  const xiiInt64 iBestNodeIndex = m_StateQueue[iBestInQueue];
  m_StateQueue.RemoveAtAndSwap(iBestInQueue);

  return iBestNodeIndex;
}

template <typename PathStateType>
void xiiPathSearch<PathStateType>::FillOutPathResult(xiiInt64 iEndNodeIndex, xiiDeque<PathResultData>& out_Path)
{
  out_Path.Clear();

  while (true)
  {
    const PathStateType* pCurState = &m_PathStates[iEndNodeIndex];

    PathResultData r;
    r.m_iNodeIndex = iEndNodeIndex;
    r.m_pPathState = pCurState;

    out_Path.PushFront(r);

    if (iEndNodeIndex == pCurState->m_iReachedThroughNode)
      return;

    iEndNodeIndex = pCurState->m_iReachedThroughNode;
  }
}

template <typename PathStateType>
void xiiPathSearch<PathStateType>::AddPathNode(xiiInt64 iNodeIndex, const PathStateType& NewState)
{
  XII_ASSERT_DEV(NewState.m_fCostToNode > m_CurState.m_fCostToNode,
                 "The costs must grow from one node to the next.\nStart Node Costs: {0}\nAdjacent Node Costs: {1}", xiiArgF(m_CurState.m_fCostToNode, 2),
                 xiiArgF(NewState.m_fCostToNode, 2));
  // XII_ASSERT_DEV(NewState.m_fEstimatedCostToTarget >=  m_CurState.m_fEstimatedCostToTarget, "The estimated path costs cannot go down, the
  // heuristic must be 'optimistic' regarding to the real costs.\nEstimated Costs from Current: {0}\nEstimated Costs from Adjacent: {1}",
  // xiiArgF(m_pCurPathState->m_fEstimatedCostToTarget, 2), xiiArgF(NewState.m_fEstimatedCostToTarget, 2));
  XII_ASSERT_DEV(NewState.m_fEstimatedCostToTarget >= NewState.m_fCostToNode, "Unrealistic expectations will get you nowhere.");

  PathStateType* pExistingState;

  if (m_PathStates.TryGetValue(iNodeIndex, pExistingState))
  {
    // state already exists in the hash table, and has a lower cost -> ignore the new state
    if (pExistingState->m_fCostToNode <= NewState.m_fCostToNode)
      return;

    // incoming state is better than the existing state -> update existing state
    *pExistingState                       = NewState;
    pExistingState->m_iReachedThroughNode = m_iCurNodeIndex;
    return;
  }

  // the state has not been reached before -> insert it
  pExistingState = &m_PathStates[iNodeIndex];

  *pExistingState                       = NewState;
  pExistingState->m_iReachedThroughNode = m_iCurNodeIndex;

  // put it into the queue of states that still need to be expanded
  m_StateQueue.PushBack(iNodeIndex);
}

template <typename PathStateType>
xiiResult xiiPathSearch<PathStateType>::FindPath(xiiInt64 iStartNodeIndex, const PathStateType& StartState, xiiInt64 iTargetNodeIndex, xiiDeque<PathResultData>& out_Path, float fMaxPathCost /* = Infinity */)
{
  XII_ASSERT_DEV(m_pStateGenerator != nullptr, "No Path State Generator is set.");

  ClearPathStates();

  if (iStartNodeIndex == iTargetNodeIndex)
  {
    m_PathStates.Reserve(1);

    m_PathStates[iTargetNodeIndex] = StartState;

    PathResultData r;
    r.m_iNodeIndex = iTargetNodeIndex;
    r.m_pPathState = &m_PathStates[iTargetNodeIndex];

    out_Path.Clear();
    out_Path.PushBack(r);

    return XII_SUCCESS;
  }

  m_PathStates.Reserve(10000);

  PathStateType& FirstState = m_PathStates[iStartNodeIndex];

  m_pStateGenerator->StartSearch(iStartNodeIndex, &FirstState, iTargetNodeIndex);

  // make sure the first state references itself, as that is a termination criterion
  FirstState                       = StartState;
  FirstState.m_iReachedThroughNode = iStartNodeIndex;

  // put the start state into the to-be-expanded queue
  m_StateQueue.PushBack(iStartNodeIndex);

  // while the queue is not empty, expand the next node and see where that gets us
  while (!m_StateQueue.IsEmpty())
  {
    PathStateType* pCurState;
    m_iCurNodeIndex = FindBestNodeToExpand(pCurState);

    // we have reached the target node, generate the final path result
    if (m_iCurNodeIndex == iTargetNodeIndex)
    {
      FillOutPathResult(m_iCurNodeIndex, out_Path);
      m_pStateGenerator->SearchFinished(XII_SUCCESS);
      return XII_SUCCESS;
    }

    // The heuristic may overestimate how much it takes to reach the destination
    // thus even though the heuristic tells us we may not be able to make it, we cannot rely on that, but need to look at
    // the actual costs
    if (pCurState->m_fCostToNode >= fMaxPathCost)
    {
      m_pStateGenerator->SearchFinished(XII_FAILURE);
      return XII_FAILURE;
    }

    m_CurState = *pCurState;

    // let the generate append all the nodes that we can reach from here
    m_pStateGenerator->GenerateAdjacentStates(m_iCurNodeIndex, m_CurState, this);
  }

  m_pStateGenerator->SearchFinished(XII_FAILURE);
  return XII_FAILURE;
}


template <typename PathStateType>
xiiResult xiiPathSearch<PathStateType>::FindClosest(
  xiiInt64                  iStartNodeIndex,
  const PathStateType&      StartState,
  IsSearchedObjectCallback  Callback,
  xiiDeque<PathResultData>& out_Path,
  float                     fMaxPathCost)
{
  XII_ASSERT_DEV(m_pStateGenerator != nullptr, "No Path State Generator is set.");

  ClearPathStates();

  m_PathStates.Reserve(10000);

  PathStateType& FirstState = m_PathStates[iStartNodeIndex];

  m_pStateGenerator->StartSearchForClosest(iStartNodeIndex, &FirstState);

  // make sure the first state references itself, as that is a termination criterion
  FirstState                       = StartState;
  FirstState.m_iReachedThroughNode = iStartNodeIndex;

  // put the start state into the to-be-expanded queue
  m_StateQueue.PushBack(iStartNodeIndex);

  // while the queue is not empty, expand the next node and see where that gets us
  while (!m_StateQueue.IsEmpty())
  {
    PathStateType* pCurState;
    m_iCurNodeIndex = FindBestNodeToExpand(pCurState);

    // we have reached the target node, generate the final path result
    if (Callback(m_iCurNodeIndex, *pCurState))
    {
      FillOutPathResult(m_iCurNodeIndex, out_Path);
      m_pStateGenerator->SearchFinished(XII_SUCCESS);
      return XII_SUCCESS;
    }

    // The heuristic may overestimate how much it takes to reach the destination
    // thus even though the heuristic tells us we may not be able to make it, we cannot rely on that, but need to look at
    // the actual costs
    if (pCurState->m_fCostToNode >= fMaxPathCost)
    {
      m_pStateGenerator->SearchFinished(XII_FAILURE);
      return XII_FAILURE;
    }

    m_CurState = *pCurState;

    // let the generate append all the nodes that we can reach from here
    m_pStateGenerator->GenerateAdjacentStates(m_iCurNodeIndex, m_CurState, this);
  }

  m_pStateGenerator->SearchFinished(XII_FAILURE);
  return XII_FAILURE;
}
