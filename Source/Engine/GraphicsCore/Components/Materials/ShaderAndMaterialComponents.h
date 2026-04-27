#pragma once

#include <GraphicsCore/Components/Render/RenderComponent.h>
#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Pipeline/RenderData.h>
#include <GraphicsFoundation/ShaderCompiler/Descriptors.h>

struct xiiMsgExtractRenderData;

// ---- SubsurfaceScatteringComponent ----

/// \brief Predefined scatter profile presets.
struct XII_GRAPHICSCORE_DLL xiiSSSProfile
{
  using StorageType = xiiUInt8;
  enum Enum : StorageType
  {
    Skin = 0,
    Wax,
    Marble,
    Foliage,
    Custom,
    ENUM_COUNT,
    Default = Skin
  };
};
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiSSSProfile);

class XII_GRAPHICSCORE_DLL xiiSSSRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSSSRenderData, xiiRenderData);

public:
  xiiColor               m_ScatterColor   = xiiColor::White;
  float                  m_fScatterRadius = 0.1f;
  xiiEnum<xiiSSSProfile> m_Profile;
};

using xiiSubsurfaceScatteringComponentManager = xiiComponentManager<class xiiSubsurfaceScatteringComponent, xiiBlockStorageType::Compact>;

class XII_GRAPHICSCORE_DLL xiiSubsurfaceScatteringComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiSubsurfaceScatteringComponent, xiiRenderComponent, xiiSubsurfaceScatteringComponentManager);

public:
  virtual void      SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void      DeserializeComponent(xiiWorldReader& inout_stream) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg) override;

  xiiSubsurfaceScatteringComponent();
  ~xiiSubsurfaceScatteringComponent();

  void                   SetScatterRadius(float f);                            // [ property ]
  float                  GetScatterRadius() const { return m_fScatterRadius; } // [ property ]
  void                   SetScatterColor(const xiiColor& c);                   // [ property ]
  xiiColor               GetScatterColor() const { return m_ScatterColor; }    // [ property ]
  void                   SetProfile(xiiEnum<xiiSSSProfile> p);                 // [ property ]
  xiiEnum<xiiSSSProfile> GetProfile() const { return m_Profile; }              // [ property ]

protected:
  void                   OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;
  xiiColor               m_ScatterColor   = xiiColor::White;
  float                  m_fScatterRadius = 0.1f;
  xiiEnum<xiiSSSProfile> m_Profile;
};

// ---- DecalMaterialComponent ----

class XII_GRAPHICSCORE_DLL xiiDecalMaterialRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDecalMaterialRenderData, xiiRenderData);

public:
  xiiMaterialResourceHandle m_hDecalMaterial;
  xiiUInt8                  m_uiDecalTypeFlags = 0xFF;
};

using xiiDecalMaterialComponentManager = xiiComponentManager<class xiiDecalMaterialComponent, xiiBlockStorageType::Compact>;

class XII_GRAPHICSCORE_DLL xiiDecalMaterialComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiDecalMaterialComponent, xiiRenderComponent, xiiDecalMaterialComponentManager);

public:
  virtual void      SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void      DeserializeComponent(xiiWorldReader& inout_stream) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg) override;

  xiiDecalMaterialComponent();
  ~xiiDecalMaterialComponent();

  void          SetDecalMaterialFile(xiiStringView sFile); // [ property ]
  xiiStringView GetDecalMaterialFile() const;              // [ property ]

protected:
  void                      OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;
  xiiMaterialResourceHandle m_hDecalMaterial;
  xiiUInt8                  m_uiDecalTypeFlags = 0xFF;
};

// ---- ShaderPermutationComponent ----

class XII_GRAPHICSCORE_DLL xiiShaderPermutationRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiShaderPermutationRenderData, xiiRenderData);

public:
  xiiDynamicArray<xiiGALPermutationVariable> m_PermutationOverrides;
};

using xiiShaderPermutationComponentManager = xiiComponentManager<class xiiShaderPermutationComponent, xiiBlockStorageType::Compact>;

/// \brief Overrides shader permutation variables (keywords) for a single object.
class XII_GRAPHICSCORE_DLL xiiShaderPermutationComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiShaderPermutationComponent, xiiRenderComponent, xiiShaderPermutationComponentManager);

public:
  virtual void      SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void      DeserializeComponent(xiiWorldReader& inout_stream) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg) override;

  xiiShaderPermutationComponent();
  ~xiiShaderPermutationComponent();

  void                                              SetPermutation(xiiStringView sKey, xiiStringView sValue);
  void                                              ClearPermutations();
  const xiiDynamicArray<xiiGALPermutationVariable>& GetPermutations() const { return m_Permutations; }

protected:
  void                                       OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;
  xiiDynamicArray<xiiGALPermutationVariable> m_Permutations;
};
