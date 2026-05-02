/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/World/Declarations.h>
#include <Texture/Image/Image.h>

struct xiiImageDataResourceDescriptor
{
  xiiImage m_Image;

  // xiiResult Serialize(xiiStreamWriter& stream) const;
  // xiiResult Deserialize(xiiStreamReader& stream);
};

class XII_GAMEENGINE_DLL xiiImageDataResource : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiImageDataResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiImageDataResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiImageDataResource, xiiImageDataResourceDescriptor);

public:
  xiiImageDataResource();
  ~xiiImageDataResource();

  const xiiImageDataResourceDescriptor& GetDescriptor() const { return *m_pDescriptor; }

private:
  virtual xiiResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDesc UpdateContent(xiiStreamReader* Stream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  xiiUniquePtr<xiiImageDataResourceDescriptor> m_pDescriptor;
};

using xiiImageDataResourceHandle = xiiTypedResourceHandle<xiiImageDataResource>;
