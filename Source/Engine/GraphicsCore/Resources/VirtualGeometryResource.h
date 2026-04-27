#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/BoundingBoxSphere.h>

/// \brief Resource descriptor for a virtual geometry stream (Nanite-style DAG).
struct XII_GRAPHICSCORE_DLL xiiVirtualGeometryResourceDescriptor
{
  xiiDynamicArray<xiiUInt8> m_StreamData; // Opaque packed stream data
  xiiBoundingBoxSphere      m_Bounds;
};

/// \brief A streaming, hierarchical mesh representation (DAG) that supports
/// continuously varying LOD without discrete popping, suitable for rendering
/// billions of triangles via task/mesh shaders.
class XII_GRAPHICSCORE_DLL xiiVirtualGeometryResource : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVirtualGeometryResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiVirtualGeometryResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiVirtualGeometryResource, xiiVirtualGeometryResourceDescriptor);

public:
  xiiVirtualGeometryResource();
  ~xiiVirtualGeometryResource();

  const xiiBoundingBoxSphere& GetBounds() const { return m_Bounds; }

protected:
  virtual xiiResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDesc UpdateContent(xiiStreamReader* pStream) override;
  virtual void                ReportResourceIsMissing() override;

private:
  xiiBoundingBoxSphere m_Bounds = xiiBoundingBoxSphere::MakeInvalid();
  // GPU buffers to hold the DAG stream
};
