#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <KrautPlugin/KrautDeclarations.h>

using xiiMeshResourceHandle      = xiiTypedResourceHandle<class xiiMeshResource>;
using xiiKrautTreeResourceHandle = xiiTypedResourceHandle<class xiiKrautTreeResource>;
using xiiMaterialResourceHandle  = xiiTypedResourceHandle<class xiiMaterialResource>;
using xiiSurfaceResourceHandle   = xiiTypedResourceHandle<class xiiSurfaceResource>;

struct XII_KRAUTPLUGIN_DLL xiiKrautTreeResourceDetails
{
  xiiBoundingBoxSphere m_Bounds;
  xiiVec3              m_vLeafCenter;
  float                m_fStaticColliderRadius;
  xiiString            m_sSurfaceResource;
};

struct XII_KRAUTPLUGIN_DLL xiiKrautTreeResourceDescriptor
{
  void      Save(xiiStreamWriter& ref_stream) const;
  xiiResult Load(xiiStreamReader& ref_stream);

  struct VertexData
  {
    XII_DECLARE_POD_TYPE();

    xiiVec3  m_vPosition;
    xiiVec3  m_vTexCoord; // U,V and Q
    float    m_fAmbientOcclusion = 1.0f;
    xiiVec3  m_vNormal;
    xiiVec3  m_vTangent;
    xiiUInt8 m_uiColorVariation = 0;

    // to compute wind
    xiiUInt8 m_uiBranchLevel  = 0; // 0 = trunk, 1 = main branches, 2 = twigs, ...
    xiiUInt8 m_uiFlutterPhase = 0; // phase shift for the flutter effect
    xiiVec3  m_vBendAnchor;
    float    m_fAnchorBendStrength     = 0;
    float    m_fBendAndFlutterStrength = 0;
  };

  struct TriangleData
  {
    XII_DECLARE_POD_TYPE();

    xiiUInt32 m_uiVertexIndex[3];
  };

  struct SubMeshData
  {
    xiiUInt16 m_uiFirstTriangle = 0;
    xiiUInt16 m_uiNumTriangles  = 0;
    xiiUInt8  m_uiMaterialIndex = 0xFF;
  };

  struct LodData
  {
    float           m_fMinLodDistance = 0;
    float           m_fMaxLodDistance = 0;
    xiiKrautLodType m_LodType         = xiiKrautLodType::None;

    xiiDynamicArray<VertexData>   m_Vertices;
    xiiDynamicArray<TriangleData> m_Triangles;
    xiiDynamicArray<SubMeshData>  m_SubMeshes;
  };

  struct MaterialData
  {
    xiiKrautMaterialType m_MaterialType;
    xiiString            m_sMaterial;
    xiiColorGammaUB      m_VariationColor = xiiColor::White; // currently done through the material
  };

  xiiKrautTreeResourceDetails     m_Details;
  xiiStaticArray<LodData, 5>      m_Lods;
  xiiHybridArray<MaterialData, 8> m_Materials;
};

class XII_KRAUTPLUGIN_DLL xiiKrautTreeResource : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiKrautTreeResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiKrautTreeResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiKrautTreeResource, xiiKrautTreeResourceDescriptor);

public:
  xiiKrautTreeResource();

  const xiiKrautTreeResourceDetails& GetDetails() const { return m_Details; }

  struct TreeLod
  {
    xiiMeshResourceHandle m_hMesh;
    float                 m_fMinLodDistance = 0;
    float                 m_fMaxLodDistance = 0;
    xiiKrautLodType       m_LodType         = xiiKrautLodType::None;
  };

  xiiArrayPtr<const TreeLod> GetTreeLODs() const { return m_TreeLODs.GetArrayPtr(); }

private:
  virtual xiiResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDesc UpdateContent(xiiStreamReader* Stream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  xiiKrautTreeResourceDetails m_Details;
  xiiStaticArray<TreeLod, 5>  m_TreeLODs;
};
