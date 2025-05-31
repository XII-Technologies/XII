#pragma once

#include <GraphicsCore/Meshes/MeshComponentBase.h>

struct xiiMsgExtractGeometry;
using xiiMeshComponentManager = xiiComponentManager<class xiiMeshComponent, xiiBlockStorageType::Compact>;

/// \brief Renders a single instance of a static mesh.
///
/// This is the main component to use for rendering regular meshes.
class XII_GRAPHICSCORE_DLL xiiMeshComponent : public xiiMeshComponentBase
{
  XII_DECLARE_COMPONENT_TYPE(xiiMeshComponent, xiiMeshComponentBase, xiiMeshComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiMeshComponent

public:
  xiiMeshComponent();
  ~xiiMeshComponent();

  /// \brief Extracts the render geometry for export etc.
  void OnMsgExtractGeometry(xiiMsgExtractGeometry& ref_msg) const; // [ msg handler ]
};
