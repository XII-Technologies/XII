/// Copyright (c) Theophilus Eriata. All Rights Reserved.

template <typename T>
xiiCustomDataResource<T>::xiiCustomDataResource() = default;

template <typename T>
xiiCustomDataResource<T>::~xiiCustomDataResource() = default;

template <typename T>
void xiiCustomDataResource<T>::CreateAndLoadData(xiiAbstractObjectGraph& ref_graph, xiiRttiConverterContext& ref_context, const xiiAbstractObjectNode* pRootNode)
{
  T* pData = reinterpret_cast<T*>(m_Data);

  if (GetLoadingState() == xiiResourceState::Loaded)
  {
    xiiMemoryUtils::Destruct(pData);
  }

  xiiMemoryUtils::Construct<SkipTrivialTypes>(pData);

  if (pRootNode)
  {
    // pRootNode is empty when the resource file is empty
    // no need to attempt to load it then
    pData->Load(ref_graph, ref_context, pRootNode);
  }
}

template <typename T>
xiiResourceLoadDescription xiiCustomDataResource<T>::UnloadData(Unload WhatToUnload)
{
  if (GetData() != nullptr)
  {
    xiiMemoryUtils::Destruct(GetData());
  }

  return xiiCustomDataResourceBase::UnloadData(WhatToUnload);
}

template <typename T>
xiiResourceLoadDescription xiiCustomDataResource<T>::UpdateContent(xiiStreamReader* Stream)
{
  return UpdateContent_Internal(Stream, *xiiGetStaticRTTI<T>());
}

template <typename T>
void xiiCustomDataResource<T>::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(xiiCustomDataResource<T>);
  out_NewMemoryUsage.m_uiMemoryGPU = 0U;
}
