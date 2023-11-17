#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Memory/InstanceDataAllocator.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraph.h>

class xiiAnimGraphInstance;
class xiiAnimGraphNode;

//////////////////////////////////////////////////////////////////////////

using xiiAnimGraphResourceHandle = xiiTypedResourceHandle<class xiiAnimGraphResource>;

struct XII_GRAPHICSCORE_DLL xiiAnimationClipMapping : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimationClipMapping, xiiReflectedClass);

  xiiHashedString                m_sClipName;
  xiiAnimationClipResourceHandle m_hClip;

  const char* GetClipName() const { return m_sClipName.GetData(); }
  void        SetClipName(const char* szName) { m_sClipName.Assign(szName); }

  const char* GetClip() const;
  void        SetClip(const char* szName);
};

class XII_GRAPHICSCORE_DLL xiiAnimGraphResource : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimGraphResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiAnimGraphResource);

public:
  xiiAnimGraphResource();
  ~xiiAnimGraphResource();

  const xiiAnimGraph& GetAnimationGraph() const { return m_AnimGraph; }

  xiiArrayPtr<const xiiString>                    GetIncludeGraphs() const { return m_IncludeGraphs; }
  const xiiDynamicArray<xiiAnimationClipMapping>& GetAnimationClipMapping() const { return m_AnimationClipMapping; }

private:
  virtual xiiResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDesc UpdateContent(xiiStreamReader* Stream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  xiiDynamicArray<xiiString>               m_IncludeGraphs;
  xiiDynamicArray<xiiAnimationClipMapping> m_AnimationClipMapping;
  xiiAnimGraph                             m_AnimGraph;
};
