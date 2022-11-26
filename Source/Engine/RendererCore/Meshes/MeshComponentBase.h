#pragma once

#include <Core/World/World.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Pipeline/RenderData.h>

struct xiiMsgSetColor;
struct xiiInstanceData;

class XII_RENDERERCORE_DLL xiiMeshRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMeshRenderData, xiiRenderData);

public:
  virtual void FillBatchIdAndSortingKey();

  xiiMeshResourceHandle     m_hMesh;
  xiiMaterialResourceHandle m_hMaterial;
  xiiColor                  m_Color = xiiColor::White;

  xiiUInt32 m_uiSubMeshIndex : 30;
  xiiUInt32 m_uiFlipWinding : 1;
  xiiUInt32 m_uiUniformScale : 1;

  xiiUInt32 m_uiUniqueID = 0;

protected:
  XII_FORCE_INLINE void FillBatchIdAndSortingKeyInternal(xiiUInt32 uiAdditionalBatchData)
  {
    m_uiFlipWinding  = m_GlobalTransform.ContainsNegativeScale() ? 1 : 0;
    m_uiUniformScale = m_GlobalTransform.ContainsUniformScale() ? 1 : 0;

    const xiiUInt32 uiMeshIDHash     = xiiHashingUtils::StringHashTo32(m_hMesh.GetResourceIDHash());
    const xiiUInt32 uiMaterialIDHash = m_hMaterial.IsValid() ? xiiHashingUtils::StringHashTo32(m_hMaterial.GetResourceIDHash()) : 0;

    // Generate batch id from mesh, material and part index.
    xiiUInt32 data[] = {uiMeshIDHash, uiMaterialIDHash, m_uiSubMeshIndex, m_uiFlipWinding, uiAdditionalBatchData};
    m_uiBatchId      = xiiHashingUtils::xxHash32(data, sizeof(data));

    // Sort by material and then by mesh
    m_uiSortingKey = (uiMaterialIDHash << 16) | ((uiMeshIDHash + m_uiSubMeshIndex) & 0xFFFE) | m_uiFlipWinding;
  }
};

struct XII_RENDERERCORE_DLL xiiMsgSetMeshMaterial : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgSetMeshMaterial, xiiMessage);

  void        SetMaterialFile(const char* szFile);
  const char* GetMaterialFile() const;

  xiiMaterialResourceHandle m_hMaterial;
  xiiUInt32                 m_uiMaterialSlot = 0xFFFFFFFFu;

  virtual void Serialize(xiiStreamWriter& stream) const override;
  virtual void Deserialize(xiiStreamReader& stream, xiiUInt8 uiTypeVersion) override;
};

class XII_RENDERERCORE_DLL xiiMeshComponentBase : public xiiRenderComponent
{
  XII_DECLARE_ABSTRACT_COMPONENT_TYPE(xiiMeshComponentBase, xiiRenderComponent);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiRenderComponent

public:
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& bounds, bool& bAlwaysVisible, xiiMsgUpdateLocalBounds& msg) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiRenderMeshComponent

public:
  xiiMeshComponentBase();
  ~xiiMeshComponentBase();

  void                    SetMesh(const xiiMeshResourceHandle& hMesh);
  XII_ALWAYS_INLINE const xiiMeshResourceHandle& GetMesh() const { return m_hMesh; }

  void                      SetMaterial(xiiUInt32 uiIndex, const xiiMaterialResourceHandle& hMaterial);
  xiiMaterialResourceHandle GetMaterial(xiiUInt32 uiIndex) const;

  XII_ALWAYS_INLINE void SetRenderDataCategory(xiiRenderData::Category category) { m_RenderDataCategory = category; }

  void        SetMeshFile(const char* szFile); // [ property ]
  const char* GetMeshFile() const;             // [ property ]

  void            SetColor(const xiiColor& color); // [ property ]
  const xiiColor& GetColor() const;                // [ property ]

  void OnMsgSetMeshMaterial(xiiMsgSetMeshMaterial& msg); // [ msg handler ]
  void OnMsgSetColor(xiiMsgSetColor& msg);               // [ msg handler ]

protected:
  virtual xiiMeshRenderData* CreateRenderData() const;

  xiiUInt32   Materials_GetCount() const;                               // [ property ]
  const char* Materials_GetValue(xiiUInt32 uiIndex) const;              // [ property ]
  void        Materials_SetValue(xiiUInt32 uiIndex, const char* value); // [ property ]
  void        Materials_Insert(xiiUInt32 uiIndex, const char* value);   // [ property ]
  void        Materials_Remove(xiiUInt32 uiIndex);                      // [ property ]

  void OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const;

  xiiRenderData::Category                    m_RenderDataCategory = xiiInvalidRenderDataCategory;
  xiiMeshResourceHandle                      m_hMesh;
  xiiDynamicArray<xiiMaterialResourceHandle> m_Materials;
  xiiColor                                   m_Color = xiiColor::White;
};
