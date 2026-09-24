/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Tracks/ColorGradient.h>

struct XII_CORE_DLL xiiColorGradientResourceDescriptor
{
  xiiColorGradient m_Gradient;

  void Save(xiiStreamWriter& ref_stream) const;
  void Load(xiiStreamReader& ref_stream);
};

using xiiColorGradientResourceHandle = xiiTypedResourceHandle<class xiiColorGradientResource>;

/// A resource that stores a single color gradient. The data is stored in the descriptor.
class XII_CORE_DLL xiiColorGradientResource : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiColorGradientResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiColorGradientResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiColorGradientResource, xiiColorGradientResourceDescriptor);

public:
  xiiColorGradientResource();

  /// Returns all the data that is stored in this resource.
  const xiiColorGradientResourceDescriptor& GetDescriptor() const { return m_Descriptor; }

  inline xiiColor Evaluate(double x) const
  {
    xiiColor result;
    m_Descriptor.m_Gradient.Evaluate(x, result);
    return result;
  }

private:
  virtual xiiResourceLoadDescription UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDescription UpdateContent(xiiStreamReader* Stream) override;
  virtual void                       UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  xiiColorGradientResourceDescriptor m_Descriptor;
};
