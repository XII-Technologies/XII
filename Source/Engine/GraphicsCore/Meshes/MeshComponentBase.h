#pragma once

#include <Core/World/World.h>
#include <GraphicsCore/Components/RenderComponent.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Meshes/MeshResource.h>

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

/// \brief This message is used to replace the material on a mesh.
struct XII_GRAPHICSCORE_DLL xiiMsgSetMeshMaterial : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgSetMeshMaterial, xiiMessage);

  // Adds SetMaterialFile() and GetMaterialFile() for convenience.
  XII_ADD_RESOURCEHANDLE_ACCESSORS(Material, m_hMaterial);

  /// The material to be used.
  xiiMaterialResourceHandle m_hMaterial; // [ property ]

  /// The slot on the mesh component where the material should be set.
  xiiUInt32 m_uiMaterialSlot = 0; // [ property ]

  virtual void Serialize(xiiStreamWriter& inout_stream) const override;
  virtual void Deserialize(xiiStreamReader& inout_stream, xiiUInt8 uiTypeVersion) override;
};

/// \brief Base class for components that render static or animated meshes.
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

  /// \brief Changes which mesh to render.
  void                                           SetMesh(const xiiMeshResourceHandle& hMesh); // [ property ]
  XII_ALWAYS_INLINE const xiiMeshResourceHandle& GetMesh() const { return m_hMesh; }          // [ property ]

  // adds SetMeshFile() and GetMeshFile() for convenience
  XII_ADD_RESOURCEHANDLE_ACCESSORS_WITH_SETTER(Mesh, m_hMesh, SetMesh);

  /// \brief Sets the material that should be used for the sub-mesh with the given index.
  void                      SetMaterial(xiiUInt32 uiIndex, const xiiMaterialResourceHandle& hMaterial); // [ property ]
  xiiMaterialResourceHandle GetMaterial(xiiUInt32 uiIndex) const;                                       // [ property ]

  /// \brief An additional tint color passed to the renderer to modify the mesh.
  void            SetColor(const xiiColor& color); // [ property ]
  const xiiColor& GetColor() const;                // [ property ]

  /// \brief The sorting depth offset allows to tweak the order in which this mesh is rendered relative to other meshes.
  ///
  /// This is mainly useful for transparent objects to render them before or after other meshes.
  void  SetSortingDepthOffset(float fOffset); // [ property ]
  float GetSortingDepthOffset() const;        // [ property ]

  void OnMsgSetMeshMaterial(xiiMsgSetMeshMaterial& ref_msg); // [ msg handler ]
  void OnMsgSetColor(xiiMsgSetColor& ref_msg);               // [ msg handler ]

protected:
  virtual xiiMeshRenderData* CreateRenderData() const;

  xiiUInt32     Materials_GetCount() const;                                  // [ property ]
  xiiStringView Materials_GetValue(xiiUInt32 uiIndex) const;                 // [ property ]
  void          Materials_SetValue(xiiUInt32 uiIndex, xiiStringView sValue); // [ property ]
  void          Materials_Insert(xiiUInt32 uiIndex, xiiStringView sValue);   // [ property ]
  void          Materials_Remove(xiiUInt32 uiIndex);                         // [ property ]

  void OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const;

  xiiMeshResourceHandle                      m_hMesh;
  xiiDynamicArray<xiiMaterialResourceHandle> m_Materials;
  xiiColor                                   m_Color               = xiiColor::White;
  float                                      m_fSortingDepthOffset = 0.0f;
};
