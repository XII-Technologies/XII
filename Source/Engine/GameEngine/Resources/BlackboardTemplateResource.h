/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <GameEngine/Components/Gameplay/BlackboardComponent.h>

using xiiBlackboardTemplateResourceHandle = xiiTypedResourceHandle<class xiiBlackboardTemplateResource>;

struct XII_GAMEENGINE_DLL xiiBlackboardTemplateResourceDescriptor
{
  xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  xiiResult Deserialize(xiiStreamReader& inout_stream);

  xiiDynamicArray<xiiBlackboardEntry> m_Entries;
};

/// \brief Describes the initial state of a blackboard.
///
/// Used by xiiBlackboardComponent to initialize its blackboard from.
class XII_GAMEENGINE_DLL xiiBlackboardTemplateResource : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiBlackboardTemplateResource, xiiResource);

  XII_RESOURCE_DECLARE_COMMON_CODE(xiiBlackboardTemplateResource);

  XII_RESOURCE_DECLARE_CREATEABLE(xiiBlackboardTemplateResource, xiiBlackboardTemplateResourceDescriptor);

public:
  xiiBlackboardTemplateResource();
  ~xiiBlackboardTemplateResource();

  const xiiBlackboardTemplateResourceDescriptor& GetDescriptor() const { return m_Descriptor; }

private:
  virtual xiiResourceLoadDescription UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDescription UpdateContent(xiiStreamReader* Stream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  xiiBlackboardTemplateResourceDescriptor m_Descriptor;
};
