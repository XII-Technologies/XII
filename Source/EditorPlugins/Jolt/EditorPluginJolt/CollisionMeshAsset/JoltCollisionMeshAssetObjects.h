#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

struct xiiPropertyMetaStateEvent;

struct xiiJoltSurfaceResourceSlot
{
  xiiString m_sLabel;
  xiiString m_sResource;
  bool      m_bExclude = false;
};

struct xiiJoltCollisionMeshType
{
  typedef xiiInt8 StorageType;

  enum Enum
  {
    ConvexHull,
    TriangleMesh,
    Cylinder,

    Default = TriangleMesh
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, xiiJoltCollisionMeshType);

struct xiiJoltConvexCollisionMeshType
{
  typedef xiiInt8 StorageType;

  enum Enum
  {
    ConvexHull,
    Cylinder,
    ConvexDecomposition,

    Default = ConvexHull
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, xiiJoltConvexCollisionMeshType);

XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, xiiJoltSurfaceResourceSlot);

class xiiJoltCollisionMeshAssetProperties : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiJoltCollisionMeshAssetProperties, xiiReflectedClass);

public:
  xiiJoltCollisionMeshAssetProperties();
  ~xiiJoltCollisionMeshAssetProperties();

  static void PropertyMetaStateEventHandler(xiiPropertyMetaStateEvent& e);

  xiiString m_sMeshFile;
  float     m_fUniformScaling = 1.0f;
  xiiString m_sConvexMeshSurface;

  xiiEnum<xiiBasisAxis>                   m_RightDir        = xiiBasisAxis::PositiveX;
  xiiEnum<xiiBasisAxis>                   m_UpDir           = xiiBasisAxis::PositiveY;
  bool                                    m_bFlipForwardDir = false;
  bool                                    m_bIsConvexMesh   = false;
  xiiEnum<xiiJoltConvexCollisionMeshType> m_ConvexMeshType;
  xiiUInt16                               m_uiMaxConvexPieces = 2;

  // Cylinder
  float    m_fRadius  = 0.5f;
  float    m_fRadius2 = 0.5f;
  float    m_fHeight  = 1.0f;
  xiiUInt8 m_uiDetail = 1;

  xiiHybridArray<xiiJoltSurfaceResourceSlot, 8> m_Slots;

  xiiUInt32 m_uiVertices  = 0;
  xiiUInt32 m_uiTriangles = 0;
};
