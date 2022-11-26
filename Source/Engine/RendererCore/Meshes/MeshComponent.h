#pragma once

#include <RendererCore/Meshes/MeshComponentBase.h>

struct xiiMsgExtractGeometry;
typedef xiiComponentManager<class xiiMeshComponent, xiiBlockStorageType::Compact> xiiMeshComponentManager;

class XII_RENDERERCORE_DLL xiiMeshComponent : public xiiMeshComponentBase
{
  XII_DECLARE_COMPONENT_TYPE(xiiMeshComponent, xiiMeshComponentBase, xiiMeshComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiMeshComponent

public:
  xiiMeshComponent();
  ~xiiMeshComponent();

  /// \brief Extracts the render geometry for export etc.
  void OnMsgExtractGeometry(xiiMsgExtractGeometry& msg) const; // [ msg handler ]
};
