/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Tracks/Curve1D.h>

/// A curve resource can contain more than one curve, but all of the same type.
struct XII_CORE_DLL xiiCurve1DResourceDescriptor
{
  xiiDynamicArray<xiiCurve1D> m_Curves;

  void Save(xiiStreamWriter& ref_stream) const;
  void Load(xiiStreamReader& ref_stream);
};

using xiiCurve1DResourceHandle = xiiTypedResourceHandle<class xiiCurve1DResource>;

/// A resource that stores 1D curves. The curves are stored in the descriptor.
class XII_CORE_DLL xiiCurve1DResource : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCurve1DResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiCurve1DResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiCurve1DResource, xiiCurve1DResourceDescriptor);

public:
  xiiCurve1DResource();

  /// Returns all the data that is stored in this resource.
  const xiiCurve1DResourceDescriptor& GetDescriptor() const { return m_Descriptor; }

private:
  virtual xiiResourceLoadDescription UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDescription UpdateContent(xiiStreamReader* Stream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  xiiCurve1DResourceDescriptor m_Descriptor;
};
