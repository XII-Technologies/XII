#pragma once

#include <Core/World/World.h>
#include <GraphicsCore/Components/RenderComponent.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Pipeline/RenderData.h>

struct xiiMsgSetColor;
struct xiiInstanceData;

class XII_GRAPHICSCORE_DLL xiiMeshRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMeshRenderData, xiiRenderData);

public:
  void         FillSortingKey();
  virtual bool CanBatch(const xiiRenderData& other) const override;

  xiiMeshResourceHandle     m_hMesh;
  xiiMaterialResourceHandle m_hMaterial;
  xiiColor                  m_Color = xiiColor::White;

  xiiUInt32 m_uiSubMeshIndex : 30;
  xiiUInt32 m_uiFlipWinding : 1;
  xiiUInt32 m_uiUniformScale : 1;

  xiiUInt32 m_uiUniqueID = 0;
};

struct XII_GRAPHICSCORE_DLL xiiMsgSetMeshMaterial : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgSetMeshMaterial, xiiMessage);

  void        SetMaterialFile(const char* szFile);
  const char* GetMaterialFile() const;

  xiiMaterialResourceHandle m_hMaterial;
  xiiUInt32                 m_uiMaterialSlot = 0xFFFFFFFFu;

  virtual void Serialize(xiiStreamWriter& inout_stream) const override;
  virtual void Deserialize(xiiStreamReader& inout_stream, xiiUInt8 uiTypeVersion) override;
};

class XII_GRAPHICSCORE_DLL xiiMeshComponentBase : public xiiRenderComponent
{
  XII_DECLARE_ABSTRACT_COMPONENT_TYPE(xiiMeshComponentBase, xiiRenderComponent);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiRenderComponent

public:
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiRenderMeshComponent

public:
  xiiMeshComponentBase();
  ~xiiMeshComponentBase();

  void                    SetMesh(const xiiMeshResourceHandle& hMesh);
  XII_ALWAYS_INLINE const xiiMeshResourceHandle& GetMesh() const { return m_hMesh; }

  void                      SetMaterial(xiiUInt32 uiIndex, const xiiMaterialResourceHandle& hMaterial);
  xiiMaterialResourceHandle GetMaterial(xiiUInt32 uiIndex) const;

  void        SetMeshFile(const char* szFile); // [ property ]
  const char* GetMeshFile() const;             // [ property ]

  void            SetColor(const xiiColor& color); // [ property ]
  const xiiColor& GetColor() const;                // [ property ]

  void  SetSortingDepthOffset(float fOffset); // [ property ]
  float GetSortingDepthOffset() const;        // [ property ]

  void OnMsgSetMeshMaterial(xiiMsgSetMeshMaterial& ref_msg); // [ msg handler ]
  void OnMsgSetColor(xiiMsgSetColor& ref_msg);               // [ msg handler ]

protected:
  virtual xiiMeshRenderData* CreateRenderData() const;

  xiiUInt32   Materials_GetCount() const;                               // [ property ]
  const char* Materials_GetValue(xiiUInt32 uiIndex) const;              // [ property ]
  void        Materials_SetValue(xiiUInt32 uiIndex, const char* value); // [ property ]
  void        Materials_Insert(xiiUInt32 uiIndex, const char* value);   // [ property ]
  void        Materials_Remove(xiiUInt32 uiIndex);                      // [ property ]

  void OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const;

  xiiMeshResourceHandle                      m_hMesh;
  xiiDynamicArray<xiiMaterialResourceHandle> m_Materials;
  xiiColor                                   m_Color               = xiiColor::White;
  float                                      m_fSortingDepthOffset = 0.0f;
};
