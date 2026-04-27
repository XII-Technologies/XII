#pragma once
#include <GraphicsCore/Components/Render/RenderComponent.h>
#include <GraphicsCore/Pipeline/RenderData.h>
struct xiiMsgExtractRenderData;

// ---- SpectralMaterial ----
class XII_GRAPHICSCORE_DLL xiiSpectralMaterialRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSpectralMaterialRenderData, xiiRenderData);

public:
  xiiUInt8 m_uiWavelengthSamples = 16;
  float    m_fDispersion         = 0.01f;
};
using xiiSpectralMaterialComponentManager = xiiComponentManager<class xiiSpectralMaterialComponent, xiiBlockStorageType::Compact>;
class XII_GRAPHICSCORE_DLL xiiSpectralMaterialComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiSpectralMaterialComponent, xiiRenderComponent, xiiSpectralMaterialComponentManager);

public:
  virtual void      SerializeComponent(xiiWorldWriter& s) const override;
  virtual void      DeserializeComponent(xiiWorldReader& s) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) override;
  xiiSpectralMaterialComponent();
  ~xiiSpectralMaterialComponent();
  void  SetDispersion(float f);
  float GetDispersion() const { return m_fDispersion; }

protected:
  void     OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;
  xiiUInt8 m_uiWavelengthSamples = 16;
  float    m_fDispersion         = 0.01f;
};

// ---- HybridPathTracer ----
class XII_GRAPHICSCORE_DLL xiiHybridPathTracerRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiHybridPathTracerRenderData, xiiRenderData);

public:
  xiiUInt16 m_uiSPP         = 1;
  xiiUInt8  m_uiMaxBounces  = 4;
  float     m_fFireflyClamp = 10.0f;
};
using xiiHybridPathTracerComponentManager = xiiComponentManager<class xiiHybridPathTracerComponent, xiiBlockStorageType::Compact>;
class XII_GRAPHICSCORE_DLL xiiHybridPathTracerComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiHybridPathTracerComponent, xiiRenderComponent, xiiHybridPathTracerComponentManager);

public:
  virtual void      SerializeComponent(xiiWorldWriter& s) const override;
  virtual void      DeserializeComponent(xiiWorldReader& s) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) override;
  xiiHybridPathTracerComponent();
  ~xiiHybridPathTracerComponent();
  void      SetSPP(xiiUInt16 n);
  xiiUInt16 GetSPP() const { return m_uiSPP; }
  void      SetMaxBounces(xiiUInt8 n);
  xiiUInt8  GetMaxBounces() const { return m_uiMaxBounces; }
  void      SetFireflyClamp(float f);
  float     GetFireflyClamp() const { return m_fFireflyClamp; }

protected:
  void      OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;
  xiiUInt16 m_uiSPP         = 1;
  xiiUInt8  m_uiMaxBounces  = 4;
  float     m_fFireflyClamp = 10.0f;
};

// ---- NeuralMaterial ----
class XII_GRAPHICSCORE_DLL xiiNeuralMaterialRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiNeuralMaterialRenderData, xiiRenderData);

public:
  xiiString m_sModelPath;
  xiiUInt16 m_uiLatentDim = 32;
};
using xiiNeuralMaterialComponentManager = xiiComponentManager<class xiiNeuralMaterialComponent, xiiBlockStorageType::Compact>;
class XII_GRAPHICSCORE_DLL xiiNeuralMaterialComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiNeuralMaterialComponent, xiiRenderComponent, xiiNeuralMaterialComponentManager);

public:
  virtual void      SerializeComponent(xiiWorldWriter& s) const override;
  virtual void      DeserializeComponent(xiiWorldReader& s) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) override;
  xiiNeuralMaterialComponent();
  ~xiiNeuralMaterialComponent();
  void          SetModelPath(xiiStringView s);
  xiiStringView GetModelPath() const { return m_sModelPath; }
  void          SetLatentDim(xiiUInt16 n);
  xiiUInt16     GetLatentDim() const { return m_uiLatentDim; }

protected:
  void      OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;
  xiiString m_sModelPath;
  xiiUInt16 m_uiLatentDim = 32;
};

// ---- DifferentiableRender ----
class XII_GRAPHICSCORE_DLL xiiDifferentiableRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDifferentiableRenderData, xiiRenderData);

public:
  xiiUInt8 m_uiDerivOrder = 1;
  bool     m_bEnabled     = true;
};
using xiiDifferentiableRenderComponentManager = xiiComponentManager<class xiiDifferentiableRenderComponent, xiiBlockStorageType::Compact>;
class XII_GRAPHICSCORE_DLL xiiDifferentiableRenderComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiDifferentiableRenderComponent, xiiRenderComponent, xiiDifferentiableRenderComponentManager);

public:
  virtual void      SerializeComponent(xiiWorldWriter& s) const override;
  virtual void      DeserializeComponent(xiiWorldReader& s) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) override;
  xiiDifferentiableRenderComponent();
  ~xiiDifferentiableRenderComponent();
  void SetEnabled(bool b);
  bool GetEnabled() const { return m_bEnabled; }

protected:
  void     OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;
  xiiUInt8 m_uiDerivOrder = 1;
  bool     m_bEnabled     = true;
};

// ---- ProceduralTerrain ----
class XII_GRAPHICSCORE_DLL xiiProceduralTerrainRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiProceduralTerrainRenderData, xiiRenderData);

public:
  float    m_fAmplitude = 100.0f;
  float    m_fFrequency = 0.01f;
  xiiUInt8 m_uiOctaves  = 6;
};
using xiiProceduralTerrainComponentManager = xiiComponentManager<class xiiProceduralTerrainComponent, xiiBlockStorageType::Compact>;
class XII_GRAPHICSCORE_DLL xiiProceduralTerrainComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiProceduralTerrainComponent, xiiRenderComponent, xiiProceduralTerrainComponentManager);

public:
  virtual void      SerializeComponent(xiiWorldWriter& s) const override;
  virtual void      DeserializeComponent(xiiWorldReader& s) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) override;
  xiiProceduralTerrainComponent();
  ~xiiProceduralTerrainComponent();
  void     SetAmplitude(float f);
  float    GetAmplitude() const { return m_fAmplitude; }
  void     SetFrequency(float f);
  float    GetFrequency() const { return m_fFrequency; }
  void     SetOctaves(xiiUInt8 n);
  xiiUInt8 GetOctaves() const { return m_uiOctaves; }

protected:
  void     OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;
  float    m_fAmplitude = 100.0f;
  float    m_fFrequency = 0.01f;
  xiiUInt8 m_uiOctaves  = 6;
};

// ---- Cloth ----
class XII_GRAPHICSCORE_DLL xiiClothRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiClothRenderData, xiiRenderData);

public:
  xiiUInt32                 m_uiVertexCount = 0;
  float                     m_fStiffness    = 0.8f;
  xiiMaterialResourceHandle m_hMaterial;
};
using xiiClothComponentManager = xiiComponentManager<class xiiClothComponent, xiiBlockStorageType::Compact>;
class XII_GRAPHICSCORE_DLL xiiClothComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiClothComponent, xiiRenderComponent, xiiClothComponentManager);

public:
  virtual void      SerializeComponent(xiiWorldWriter& s) const override;
  virtual void      DeserializeComponent(xiiWorldReader& s) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) override;
  xiiClothComponent();
  ~xiiClothComponent();
  void          SetStiffness(float f);
  float         GetStiffness() const { return m_fStiffness; }
  void          SetMaterialFile(xiiStringView s);
  xiiStringView GetMaterialFile() const;

protected:
  void                      OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;
  float                     m_fStiffness = 0.8f;
  xiiMaterialResourceHandle m_hMaterial;
};

// ---- HairFur ----
class XII_GRAPHICSCORE_DLL xiiHairFurRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiHairFurRenderData, xiiRenderData);

public:
  float     m_fLength       = 0.05f;
  float     m_fThickness    = 0.001f;
  xiiUInt32 m_uiStrandCount = 10000;
  xiiColor  m_BaseColor     = xiiColor::White;
};
using xiiHairFurComponentManager = xiiComponentManager<class xiiHairFurComponent, xiiBlockStorageType::Compact>;
class XII_GRAPHICSCORE_DLL xiiHairFurComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiHairFurComponent, xiiRenderComponent, xiiHairFurComponentManager);

public:
  virtual void      SerializeComponent(xiiWorldWriter& s) const override;
  virtual void      DeserializeComponent(xiiWorldReader& s) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) override;
  xiiHairFurComponent();
  ~xiiHairFurComponent();
  void      SetLength(float f);
  float     GetLength() const { return m_fLength; }
  void      SetThickness(float f);
  float     GetThickness() const { return m_fThickness; }
  void      SetStrandCount(xiiUInt32 n);
  xiiUInt32 GetStrandCount() const { return m_uiStrandCount; }
  void      SetBaseColor(const xiiColor& c);
  xiiColor  GetBaseColor() const { return m_BaseColor; }

protected:
  void      OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;
  float     m_fLength       = 0.05f;
  float     m_fThickness    = 0.001f;
  xiiUInt32 m_uiStrandCount = 10000;
  xiiColor  m_BaseColor     = xiiColor::White;
};
