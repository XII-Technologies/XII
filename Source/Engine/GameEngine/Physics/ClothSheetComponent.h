#pragma once

#include <Core/World/ComponentManager.h>
#include <GameEngine/Physics/ClothSheetSimulator.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/Renderer.h>

using xiiMaterialResourceHandle          = xiiTypedResourceHandle<class xiiMaterialResource>;
using xiiDynamicMeshBufferResourceHandle = xiiTypedResourceHandle<class xiiDynamicMeshBufferResource>;

//////////////////////////////////////////////////////////////////////////

class XII_GAMEENGINE_DLL xiiClothSheetComponentManager : public xiiComponentManager<class xiiClothSheetComponent, xiiBlockStorageType::FreeList>
{
public:
  xiiClothSheetComponentManager(xiiWorld* pWorld);
  ~xiiClothSheetComponentManager();

  virtual void Initialize() override;

private:
  void Update(const xiiWorldModule::UpdateContext& context);
  void UpdateBounds(const xiiWorldModule::UpdateContext& context);
};

//////////////////////////////////////////////////////////////////////////

class XII_GAMEENGINE_DLL xiiClothSheetRenderData final : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiClothSheetRenderData, xiiRenderData);

public:
  xiiUInt32              m_uiUniqueID = 0;
  xiiArrayPtr<xiiVec3>   m_Positions;
  xiiArrayPtr<xiiUInt16> m_Indices;
  xiiUInt16              m_uiVerticesX;
  xiiUInt16              m_uiVerticesY;
  xiiColor               m_Color;

  xiiMaterialResourceHandle m_hMaterial;
};

class XII_GAMEENGINE_DLL xiiClothSheetRenderer : public xiiRenderer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiClothSheetRenderer, xiiRenderer);
  XII_DISALLOW_COPY_AND_ASSIGN(xiiClothSheetRenderer);

public:
  xiiClothSheetRenderer();
  ~xiiClothSheetRenderer();

  virtual void GetSupportedRenderDataCategories(xiiHybridArray<xiiRenderData::Category, 8>& categories) const override;
  virtual void GetSupportedRenderDataTypes(xiiHybridArray<const xiiRTTI*, 8>& types) const override;
  virtual void RenderBatch(const xiiRenderViewContext& renderContext, const xiiRenderPipelinePass* pPass, const xiiRenderDataBatch& batch) const override;


protected:
  void CreateVertexBuffer();

  xiiDynamicMeshBufferResourceHandle m_hDynamicMeshBuffer;
};

struct XII_GAMEENGINE_DLL xiiClothSheetFlags
{
  using StorageType = xiiUInt16;

  enum Enum
  {
    FixedCornerTopLeft     = XII_BIT(0),
    FixedCornerTopRight    = XII_BIT(1),
    FixedCornerBottomRight = XII_BIT(2),
    FixedCornerBottomLeft  = XII_BIT(3),
    FixedEdgeTop           = XII_BIT(4),
    FixedEdgeRight         = XII_BIT(5),
    FixedEdgeBottom        = XII_BIT(6),
    FixedEdgeLeft          = XII_BIT(7),

    Default = FixedEdgeTop
  };

  struct Bits
  {
    StorageType FixedCornerTopLeft : 1;
    StorageType FixedCornerTopRight : 1;
    StorageType FixedCornerBottomRight : 1;
    StorageType FixedCornerBottomLeft : 1;
    StorageType FixedEdgeTop : 1;
    StorageType FixedEdgeRight : 1;
    StorageType FixedEdgeBottom : 1;
    StorageType FixedEdgeLeft : 1;
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GAMEENGINE_DLL, xiiClothSheetFlags);

class XII_GAMEENGINE_DLL xiiClothSheetComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiClothSheetComponent, xiiRenderComponent, xiiClothSheetComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& stream) override;

  virtual void OnActivated() override;
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiRenderComponent

public:
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& bounds, bool& bAlwaysVisible, xiiMsgUpdateLocalBounds& msg) override;

private:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const;

  //////////////////////////////////////////////////////////////////////////
  // xiiClothSheetComponent

public:
  xiiClothSheetComponent();
  ~xiiClothSheetComponent();

  void    SetSize(xiiVec2 val);               // [ property ]
  xiiVec2 GetSize() const { return m_vSize; } // [ property ]

  void    SetSlack(xiiVec2 val);                // [ property ]
  xiiVec2 GetSlack() const { return m_vSlack; } // [ property ]

  void       SetSegments(xiiVec2U32 val);                // [ property ]
  xiiVec2U32 GetSegments() const { return m_vSegments; } // [ property ]

  float    m_fWindInfluence = 0.3f;            // [ property ]
  float    m_fDamping       = 0.5f;            // [ property ]
  xiiColor m_Color          = xiiColor::White; // [ property ]

  void                            SetFlags(xiiBitflags<xiiClothSheetFlags> flags); // [ property ]
  xiiBitflags<xiiClothSheetFlags> GetFlags() const { return m_Flags; }             // [ property ]

  void        SetMaterialFile(const char* szFile); // [ property ]
  const char* GetMaterialFile() const;             // [ property ]

  xiiMaterialResourceHandle m_hMaterial; // [ property ]

private:
  void Update();
  void SetupCloth();

  xiiVec2                         m_vSize;
  xiiVec2                         m_vSlack;
  xiiVec2U32                      m_vSegments;
  xiiBitflags<xiiClothSheetFlags> m_Flags;

  xiiUInt8          m_uiSleepCounter            = 0;
  mutable xiiUInt8  m_uiVisibleCounter          = 0;
  xiiUInt8          m_uiCheckEquilibriumCounter = 0;
  xiiClothSimulator m_Simulator;

  xiiBoundingBox m_Bbox;
};
