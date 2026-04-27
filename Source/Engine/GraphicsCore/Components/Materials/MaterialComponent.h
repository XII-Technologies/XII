#pragma once

#include <GraphicsCore/Components/Render/RenderComponent.h>
#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Pipeline/RenderData.h>

struct xiiMsgExtractRenderData;

/// \brief Render data for a material assignment component.
class XII_GRAPHICSCORE_DLL xiiMaterialRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMaterialRenderData, xiiRenderData);

public:
  xiiDynamicArray<xiiMaterialResourceHandle> m_Materials;
};

using xiiMaterialComponentManager = xiiComponentManager<class xiiMaterialComponent, xiiBlockStorageType::Compact>;

/// \brief Overrides material slots on a co-located mesh component.
class XII_GRAPHICSCORE_DLL xiiMaterialComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiMaterialComponent, xiiRenderComponent, xiiMaterialComponentManager);

public:
  virtual void      SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void      DeserializeComponent(xiiWorldReader& inout_stream) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg) override;

  xiiMaterialComponent();
  ~xiiMaterialComponent();

  xiiUInt32                 GetMaterialCount() const;
  void                      SetMaterial(xiiUInt32 uiSlot, const xiiMaterialResourceHandle& hMat);
  xiiMaterialResourceHandle GetMaterial(xiiUInt32 uiSlot) const;
  void                      SetMaterialFile(xiiUInt32 uiSlot, xiiStringView sFile);
  xiiStringView             GetMaterialFile(xiiUInt32 uiSlot) const;

  // Property slot 0 shim
  void          SetMaterial0File(xiiStringView s); // [ property ]
  xiiStringView GetMaterial0File() const;          // [ property ]

protected:
  void                                       OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;
  xiiDynamicArray<xiiMaterialResourceHandle> m_Materials;
};

// ---- MaterialVariantComponent ----

/// \brief A named material variant entry.
struct XII_GRAPHICSCORE_DLL xiiMaterialVariantEntry
{
  xiiHashedString           m_sName;
  xiiMaterialResourceHandle m_hMaterial;
};

/// \brief Render data for a material variant component.
class XII_GRAPHICSCORE_DLL xiiMaterialVariantRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMaterialVariantRenderData, xiiRenderData);

public:
  xiiHashedString m_sActiveVariant;
  xiiUInt32       m_uiActiveIndex = 0;
};

using xiiMaterialVariantComponentManager = xiiComponentManager<class xiiMaterialVariantComponent, xiiBlockStorageType::Compact>;

/// \brief Switches between named material variants at runtime.
class XII_GRAPHICSCORE_DLL xiiMaterialVariantComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiMaterialVariantComponent, xiiRenderComponent, xiiMaterialVariantComponentManager);

public:
  virtual void      SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void      DeserializeComponent(xiiWorldReader& inout_stream) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg) override;

  xiiMaterialVariantComponent();
  ~xiiMaterialVariantComponent();

  void          SetActiveVariant(xiiStringView sName); // [ property ]
  xiiStringView GetActiveVariant() const;              // [ property ]

  void                      AddVariant(xiiStringView sName, const xiiMaterialResourceHandle& hMat);
  xiiMaterialResourceHandle GetVariantMaterial(xiiStringView sName) const;

protected:
  void                                     OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;
  xiiDynamicArray<xiiMaterialVariantEntry> m_Variants;
  xiiHashedString                          m_sActiveVariant;
};
