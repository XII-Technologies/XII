#pragma once

#include <GraphicsCore/Components/Render/RenderComponent.h>
#include <GraphicsCore/Pipeline/RenderData.h>

struct xiiMsgExtractRenderData;

// ============================================================
// Post-Process Volume
// ============================================================

class XII_GRAPHICSCORE_DLL xiiPostProcessVolumeRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPostProcessVolumeRenderData, xiiRenderData);

public:
  float m_fBlendRadius = 0.0f; ///< World-space blend distance at the volume boundary.
  float m_fPriority    = 0.0f;
  bool  m_bIsGlobal    = true;
};

using xiiPostProcessVolumeComponentManager = xiiComponentManager<class xiiPostProcessVolumeComponent, xiiBlockStorageType::Compact>;

class XII_GRAPHICSCORE_DLL xiiPostProcessVolumeComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiPostProcessVolumeComponent, xiiRenderComponent, xiiPostProcessVolumeComponentManager);

public:
  virtual void      SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void      DeserializeComponent(xiiWorldReader& inout_stream) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg) override;

  xiiPostProcessVolumeComponent();
  ~xiiPostProcessVolumeComponent();

  void  SetBlendRadius(float f);
  float GetBlendRadius() const { return m_fBlendRadius; }
  void  SetPriority(float f);
  float GetPriority() const { return m_fPriority; }
  void  SetIsGlobal(bool b);
  bool  GetIsGlobal() const { return m_bIsGlobal; }

protected:
  void  OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;
  float m_fBlendRadius = 0.0f;
  float m_fPriority    = 0.0f;
  bool  m_bIsGlobal    = true;
};

// ============================================================
// Bloom
// ============================================================

class XII_GRAPHICSCORE_DLL xiiBloomRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiBloomRenderData, xiiRenderData);

public:
  float m_fThreshold = 1.0f;
  float m_fIntensity = 1.0f;
  float m_fRadius    = 0.003f;
};

using xiiBloomComponentManager = xiiComponentManager<class xiiBloomComponent, xiiBlockStorageType::Compact>;

class XII_GRAPHICSCORE_DLL xiiBloomComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiBloomComponent, xiiRenderComponent, xiiBloomComponentManager);

public:
  virtual void      SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void      DeserializeComponent(xiiWorldReader& inout_stream) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) override;

  xiiBloomComponent();
  ~xiiBloomComponent();
  void  SetThreshold(float f);
  float GetThreshold() const { return m_fThreshold; }
  void  SetIntensity(float f);
  float GetIntensity() const { return m_fIntensity; }
  void  SetRadius(float f);
  float GetRadius() const { return m_fRadius; }

protected:
  void  OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;
  float m_fThreshold = 1.0f;
  float m_fIntensity = 1.0f;
  float m_fRadius    = 0.003f;
};

// ============================================================
// Tone Mapping
// ============================================================

struct XII_GRAPHICSCORE_DLL xiiTonemapOperator
{
  using StorageType = xiiUInt8;
  enum Enum : StorageType
  {
    Reinhard = 0,
    ACES,
    AgX,
    Custom,
    ENUM_COUNT,
    Default = ACES
  };
};
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiTonemapOperator);

class XII_GRAPHICSCORE_DLL xiiToneMappingRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiToneMappingRenderData, xiiRenderData);

public:
  xiiEnum<xiiTonemapOperator> m_Operator;
  float                       m_fExposure   = 0.0f;
  float                       m_fWhitePoint = 4.0f;
};

using xiiToneMappingComponentManager = xiiComponentManager<class xiiToneMappingComponent, xiiBlockStorageType::Compact>;

class XII_GRAPHICSCORE_DLL xiiToneMappingComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiToneMappingComponent, xiiRenderComponent, xiiToneMappingComponentManager);

public:
  virtual void      SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void      DeserializeComponent(xiiWorldReader& inout_stream) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) override;

  xiiToneMappingComponent();
  ~xiiToneMappingComponent();
  void                        SetOperator(xiiEnum<xiiTonemapOperator> op);
  xiiEnum<xiiTonemapOperator> GetOperator() const { return m_Operator; }
  void                        SetExposure(float f);
  float                       GetExposure() const { return m_fExposure; }
  void                        SetWhitePoint(float f);
  float                       GetWhitePoint() const { return m_fWhitePoint; }

protected:
  void                        OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;
  xiiEnum<xiiTonemapOperator> m_Operator;
  float                       m_fExposure   = 0.0f;
  float                       m_fWhitePoint = 4.0f;
};

// ============================================================
// TAA, Motion Blur, DoF, SSR, SSAO, Color Grading
// ============================================================

class XII_GRAPHICSCORE_DLL xiiTemporalAARenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTemporalAARenderData, xiiRenderData);

public:
  float m_fJitterScale  = 1.0f;
  float m_fFeedbackMin  = 0.88f;
  float m_fFeedbackMax  = 0.97f;
  bool  m_bAntiGhosting = true;
};

using xiiTemporalAAComponentManager = xiiComponentManager<class xiiTemporalAAComponent, xiiBlockStorageType::Compact>;

class XII_GRAPHICSCORE_DLL xiiTemporalAAComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiTemporalAAComponent, xiiRenderComponent, xiiTemporalAAComponentManager);

public:
  virtual void      SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void      DeserializeComponent(xiiWorldReader& inout_stream) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) override;
  xiiTemporalAAComponent();
  ~xiiTemporalAAComponent();
  void  SetJitterScale(float f);
  float GetJitterScale() const { return m_fJitterScale; }
  void  SetFeedbackMin(float f);
  float GetFeedbackMin() const { return m_fFeedbackMin; }
  void  SetFeedbackMax(float f);
  float GetFeedbackMax() const { return m_fFeedbackMax; }
  void  SetAntiGhosting(bool b);
  bool  GetAntiGhosting() const { return m_bAntiGhosting; }

protected:
  void  OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;
  float m_fJitterScale  = 1.0f;
  float m_fFeedbackMin  = 0.88f;
  float m_fFeedbackMax  = 0.97f;
  bool  m_bAntiGhosting = true;
};

class XII_GRAPHICSCORE_DLL xiiMotionBlurRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMotionBlurRenderData, xiiRenderData);

public:
  float    m_fShutterAngle = 180.0f; ///< Degrees (0-360).
  xiiUInt8 m_uiMaxSamples  = 16;
};

using xiiMotionBlurComponentManager = xiiComponentManager<class xiiMotionBlurComponent, xiiBlockStorageType::Compact>;

class XII_GRAPHICSCORE_DLL xiiMotionBlurComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiMotionBlurComponent, xiiRenderComponent, xiiMotionBlurComponentManager);

public:
  virtual void      SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void      DeserializeComponent(xiiWorldReader& inout_stream) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) override;
  xiiMotionBlurComponent();
  ~xiiMotionBlurComponent();
  void     SetShutterAngle(float f);
  float    GetShutterAngle() const { return m_fShutterAngle; }
  void     SetMaxSamples(xiiUInt8 n);
  xiiUInt8 GetMaxSamples() const { return m_uiMaxSamples; }

protected:
  void     OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;
  float    m_fShutterAngle = 180.0f;
  xiiUInt8 m_uiMaxSamples  = 16;
};

class XII_GRAPHICSCORE_DLL xiiDepthOfFieldRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDepthOfFieldRenderData, xiiRenderData);

public:
  float    m_fFocusDistance = 5.0f;
  float    m_fNearBlur      = 0.5f;
  float    m_fFarBlur       = 1.5f;
  float    m_fBokehSize     = 0.02f;
  xiiUInt8 m_uiBokehBlades  = 6;
};

using xiiDepthOfFieldComponentManager = xiiComponentManager<class xiiDepthOfFieldComponent, xiiBlockStorageType::Compact>;

class XII_GRAPHICSCORE_DLL xiiDepthOfFieldComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiDepthOfFieldComponent, xiiRenderComponent, xiiDepthOfFieldComponentManager);

public:
  virtual void      SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void      DeserializeComponent(xiiWorldReader& inout_stream) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) override;
  xiiDepthOfFieldComponent();
  ~xiiDepthOfFieldComponent();
  void     SetFocusDistance(float f);
  float    GetFocusDistance() const { return m_fFocusDistance; }
  void     SetNearBlur(float f);
  float    GetNearBlur() const { return m_fNearBlur; }
  void     SetFarBlur(float f);
  float    GetFarBlur() const { return m_fFarBlur; }
  void     SetBokehSize(float f);
  float    GetBokehSize() const { return m_fBokehSize; }
  void     SetBokehBlades(xiiUInt8 n);
  xiiUInt8 GetBokehBlades() const { return m_uiBokehBlades; }

protected:
  void     OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;
  float    m_fFocusDistance = 5.0f;
  float    m_fNearBlur      = 0.5f;
  float    m_fFarBlur       = 1.5f;
  float    m_fBokehSize     = 0.02f;
  xiiUInt8 m_uiBokehBlades  = 6;
};

class XII_GRAPHICSCORE_DLL xiiSSRRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSSRRenderData, xiiRenderData);

public:
  xiiUInt8 m_uiMaxSteps    = 64;
  float    m_fThickness    = 0.05f;
  float    m_fMaxRoughness = 0.5f;
  bool     m_bHalfRes      = true;
};

using xiiScreenSpaceReflectionComponentManager = xiiComponentManager<class xiiScreenSpaceReflectionComponent, xiiBlockStorageType::Compact>;

class XII_GRAPHICSCORE_DLL xiiScreenSpaceReflectionComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiScreenSpaceReflectionComponent, xiiRenderComponent, xiiScreenSpaceReflectionComponentManager);

public:
  virtual void      SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void      DeserializeComponent(xiiWorldReader& inout_stream) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) override;
  xiiScreenSpaceReflectionComponent();
  ~xiiScreenSpaceReflectionComponent();
  void     SetMaxSteps(xiiUInt8 n);
  xiiUInt8 GetMaxSteps() const { return m_uiMaxSteps; }
  void     SetThickness(float f);
  float    GetThickness() const { return m_fThickness; }
  void     SetMaxRoughness(float f);
  float    GetMaxRoughness() const { return m_fMaxRoughness; }
  void     SetHalfRes(bool b);
  bool     GetHalfRes() const { return m_bHalfRes; }

protected:
  void     OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;
  xiiUInt8 m_uiMaxSteps    = 64;
  float    m_fThickness    = 0.05f;
  float    m_fMaxRoughness = 0.5f;
  bool     m_bHalfRes      = true;
};

class XII_GRAPHICSCORE_DLL xiiAORenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAORenderData, xiiRenderData);

public:
  float    m_fRadius      = 0.5f;
  xiiUInt8 m_uiNumSamples = 16;
  float    m_fStrength    = 1.0f;
  bool     m_bBentNormals = false;
};

using xiiAmbientOcclusionComponentManager = xiiComponentManager<class xiiAmbientOcclusionComponent, xiiBlockStorageType::Compact>;

class XII_GRAPHICSCORE_DLL xiiAmbientOcclusionComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiAmbientOcclusionComponent, xiiRenderComponent, xiiAmbientOcclusionComponentManager);

public:
  virtual void      SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void      DeserializeComponent(xiiWorldReader& inout_stream) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) override;
  xiiAmbientOcclusionComponent();
  ~xiiAmbientOcclusionComponent();
  void     SetRadius(float f);
  float    GetRadius() const { return m_fRadius; }
  void     SetNumSamples(xiiUInt8 n);
  xiiUInt8 GetNumSamples() const { return m_uiNumSamples; }
  void     SetStrength(float f);
  float    GetStrength() const { return m_fStrength; }
  void     SetBentNormals(bool b);
  bool     GetBentNormals() const { return m_bBentNormals; }

protected:
  void     OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;
  float    m_fRadius      = 0.5f;
  xiiUInt8 m_uiNumSamples = 16;
  float    m_fStrength    = 1.0f;
  bool     m_bBentNormals = false;
};

class XII_GRAPHICSCORE_DLL xiiColorGradingRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiColorGradingRenderData, xiiRenderData);

public:
  xiiTexture3DResourceHandle m_hLUT;
  float                      m_fLUTBlend   = 1.0f;
  float                      m_fSaturation = 1.0f;
  float                      m_fContrast   = 1.0f;
};

using xiiColorGradingComponentManager = xiiComponentManager<class xiiColorGradingComponent, xiiBlockStorageType::Compact>;

class XII_GRAPHICSCORE_DLL xiiColorGradingComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiColorGradingComponent, xiiRenderComponent, xiiColorGradingComponentManager);

public:
  virtual void      SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void      DeserializeComponent(xiiWorldReader& inout_stream) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) override;
  xiiColorGradingComponent();
  ~xiiColorGradingComponent();
  void          SetLUTFile(xiiStringView sFile);
  xiiStringView GetLUTFile() const;
  void          SetLUTBlend(float f);
  float         GetLUTBlend() const { return m_fLUTBlend; }
  void          SetSaturation(float f);
  float         GetSaturation() const { return m_fSaturation; }
  void          SetContrast(float f);
  float         GetContrast() const { return m_fContrast; }

protected:
  void                       OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;
  xiiTexture3DResourceHandle m_hLUT;
  float                      m_fLUTBlend   = 1.0f;
  float                      m_fSaturation = 1.0f;
  float                      m_fContrast   = 1.0f;
};
