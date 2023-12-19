#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/Util/AssetUtils.h>
#include <GraphicsCore/Meshes/MeshBufferUtils.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

struct xiiPropertyMetaStateEvent;

struct xiiMeshPrimitive
{
  using StorageType = xiiInt8;

  enum Enum
  {
    File,
    Box,
    Rect,
    Cylinder,
    Cone,
    Pyramid,
    Sphere,
    HalfSphere,
    GeodesicSphere,
    Capsule,
    Torus,

    Default = File
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, xiiMeshPrimitive);

class xiiMeshAssetProperties : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMeshAssetProperties, xiiReflectedClass);

public:
  xiiMeshAssetProperties();
  ~xiiMeshAssetProperties();

  static void PropertyMetaStateEventHandler(xiiPropertyMetaStateEvent& e);

  xiiString m_sMeshFile;
  float     m_fUniformScaling = 1.0f;

  float     m_fRadius   = 0.5f;
  float     m_fRadius2  = 0.5f;
  float     m_fHeight   = 1.0f;
  xiiAngle  m_Angle     = xiiAngle::MakeFromDegree(360.0f);
  xiiUInt16 m_uiDetail  = 0;
  xiiUInt16 m_uiDetail2 = 0;
  bool      m_bCap      = true;
  bool      m_bCap2     = true;

  xiiEnum<xiiBasisAxis> m_RightDir        = xiiBasisAxis::PositiveY;
  xiiEnum<xiiBasisAxis> m_UpDir           = xiiBasisAxis::PositiveZ;
  bool                  m_bFlipForwardDir = false;

  xiiMeshPrimitive::Enum m_PrimitiveType = xiiMeshPrimitive::Default;

  bool m_bRecalculateNormals   = false;
  bool m_bRecalculateTrangents = true;
  bool m_bImportMaterials      = true;

  xiiEnum<xiiMeshNormalPrecision>   m_NormalPrecision;
  xiiEnum<xiiMeshTexCoordPrecision> m_TexCoordPrecision;

  xiiHybridArray<xiiMaterialResourceSlot, 8> m_Slots;

  xiiUInt32 m_uiVertices  = 0;
  xiiUInt32 m_uiTriangles = 0;
};
