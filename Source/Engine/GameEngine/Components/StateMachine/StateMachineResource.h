/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/ResourceManager/Resource.h>
#include <GameEngine/Components/StateMachine/StateMachine.h>

using xiiStateMachineResourceHandle = xiiTypedResourceHandle<class xiiStateMachineResource>;

class XII_GAMEENGINE_DLL xiiStateMachineResource : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiStateMachineResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiStateMachineResource);

public:
  xiiStateMachineResource();
  ~xiiStateMachineResource();

  const xiiSharedPtr<const xiiStateMachineDescription>& GetDescription() const { return m_pDescription; }

  xiiUniquePtr<xiiStateMachineInstance> CreateInstance(xiiReflectedClass& ref_owner);

private:
  virtual xiiResourceLoadDescription UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDescription UpdateContent(xiiStreamReader* pStream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  xiiSharedPtr<const xiiStateMachineDescription> m_pDescription;
};
