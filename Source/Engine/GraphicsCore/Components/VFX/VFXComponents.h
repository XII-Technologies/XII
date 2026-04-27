#pragma once
#include <GraphicsCore/Components/Render/RenderComponent.h>
#include <GraphicsCore/Pipeline/RenderData.h>
struct xiiMsgExtractRenderData;

// ---- ParticleEmitter ----
class XII_GRAPHICSCORE_DLL xiiParticleEmitterRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleEmitterRenderData, xiiRenderData);

public:
  xiiUInt32 m_uiActiveParticles = 0;
  float     m_fSimTime          = 0.0f;
};
using xiiParticleEmitterComponentManager = xiiComponentManager<class xiiParticleEmitterComponent, xiiBlockStorageType::Compact>;
class XII_GRAPHICSCORE_DLL xiiParticleEmitterComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiParticleEmitterComponent, xiiRenderComponent, xiiParticleEmitterComponentManager);

public:
  virtual void      SerializeComponent(xiiWorldWriter& s) const override;
  virtual void      DeserializeComponent(xiiWorldReader& s) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) override;
  xiiParticleEmitterComponent();
  ~xiiParticleEmitterComponent();
  void      SetMaxParticles(xiiUInt32 n);
  xiiUInt32 GetMaxParticles() const { return m_uiMaxParticles; }
  void      SetLooping(bool b);
  bool      GetLooping() const { return m_bLooping; }
  void      Play();
  void      Stop();

protected:
  void      OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;
  xiiUInt32 m_uiMaxParticles = 1000;
  bool      m_bLooping       = true;
  bool      m_bPlaying       = false;
  float     m_fSimTime       = 0.0f;
};

// ---- RibbonEmitter ----
class XII_GRAPHICSCORE_DLL xiiRibbonEmitterRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRibbonEmitterRenderData, xiiRenderData);

public:
  xiiUInt32                 m_uiSegments = 32;
  float                     m_fWidth     = 0.1f;
  float                     m_fLifetime  = 2.0f;
  xiiMaterialResourceHandle m_hMaterial;
};
using xiiRibbonEmitterComponentManager = xiiComponentManager<class xiiRibbonEmitterComponent, xiiBlockStorageType::Compact>;
class XII_GRAPHICSCORE_DLL xiiRibbonEmitterComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiRibbonEmitterComponent, xiiRenderComponent, xiiRibbonEmitterComponentManager);

public:
  virtual void      SerializeComponent(xiiWorldWriter& s) const override;
  virtual void      DeserializeComponent(xiiWorldReader& s) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) override;
  xiiRibbonEmitterComponent();
  ~xiiRibbonEmitterComponent();
  void          SetWidth(float f);
  float         GetWidth() const { return m_fWidth; }
  void          SetLifetime(float f);
  float         GetLifetime() const { return m_fLifetime; }
  void          SetMaterialFile(xiiStringView s);
  xiiStringView GetMaterialFile() const;

protected:
  void                      OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;
  xiiUInt32                 m_uiSegments = 32;
  float                     m_fWidth     = 0.1f;
  float                     m_fLifetime  = 2.0f;
  xiiMaterialResourceHandle m_hMaterial;
};

// ---- VolumeRenderer ----
class XII_GRAPHICSCORE_DLL xiiVolumeRendererRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVolumeRendererRenderData, xiiRenderData);

public:
  xiiTexture3DResourceHandle m_hDensity;
  float                      m_fDensityScale = 1.0f;
  xiiUInt16                  m_uiMaxSteps    = 128;
};
using xiiVolumeRendererComponentManager = xiiComponentManager<class xiiVolumeRendererComponent, xiiBlockStorageType::Compact>;
class XII_GRAPHICSCORE_DLL xiiVolumeRendererComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiVolumeRendererComponent, xiiRenderComponent, xiiVolumeRendererComponentManager);

public:
  virtual void      SerializeComponent(xiiWorldWriter& s) const override;
  virtual void      DeserializeComponent(xiiWorldReader& s) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) override;
  xiiVolumeRendererComponent();
  ~xiiVolumeRendererComponent();
  void          SetDensityFile(xiiStringView s);
  xiiStringView GetDensityFile() const;
  void          SetDensityScale(float f);
  float         GetDensityScale() const { return m_fDensityScale; }
  void          SetMaxSteps(xiiUInt16 n);
  xiiUInt16     GetMaxSteps() const { return m_uiMaxSteps; }

protected:
  void                       OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;
  xiiTexture3DResourceHandle m_hDensity;
  float                      m_fDensityScale = 1.0f;
  xiiUInt16                  m_uiMaxSteps    = 128;
};

// ---- SparseVolume ----
class XII_GRAPHICSCORE_DLL xiiSparseVolumeRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSparseVolumeRenderData, xiiRenderData);

public:
  xiiUInt32 m_uiActiveVoxels = 0;
  float     m_fVoxelSize     = 0.1f;
};
using xiiSparseVolumeComponentManager = xiiComponentManager<class xiiSparseVolumeComponent, xiiBlockStorageType::Compact>;
class XII_GRAPHICSCORE_DLL xiiSparseVolumeComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiSparseVolumeComponent, xiiRenderComponent, xiiSparseVolumeComponentManager);

public:
  virtual void      SerializeComponent(xiiWorldWriter& s) const override;
  virtual void      DeserializeComponent(xiiWorldReader& s) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) override;
  xiiSparseVolumeComponent();
  ~xiiSparseVolumeComponent();
  void  SetVoxelSize(float f);
  float GetVoxelSize() const { return m_fVoxelSize; }

protected:
  void  OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;
  float m_fVoxelSize = 0.1f;
};

// ---- VFXGraph ----
class XII_GRAPHICSCORE_DLL xiiVFXGraphRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVFXGraphRenderData, xiiRenderData);

public:
  xiiString m_sGraphAsset;
  bool      m_bPaused   = false;
  float     m_fPlayRate = 1.0f;
};
using xiiVFXGraphComponentManager = xiiComponentManager<class xiiVFXGraphComponent, xiiBlockStorageType::Compact>;
class XII_GRAPHICSCORE_DLL xiiVFXGraphComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiVFXGraphComponent, xiiRenderComponent, xiiVFXGraphComponentManager);

public:
  virtual void      SerializeComponent(xiiWorldWriter& s) const override;
  virtual void      DeserializeComponent(xiiWorldReader& s) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) override;
  xiiVFXGraphComponent();
  ~xiiVFXGraphComponent();
  void          SetGraphAsset(xiiStringView s);
  xiiStringView GetGraphAsset() const { return m_sGraphAsset; }
  void          SetPlayRate(float f);
  float         GetPlayRate() const { return m_fPlayRate; }
  void          SetPaused(bool b);
  bool          GetPaused() const { return m_bPaused; }

protected:
  void      OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;
  xiiString m_sGraphAsset;
  float     m_fPlayRate = 1.0f;
  bool      m_bPaused   = false;
};

// ---- LensFlare ----
class XII_GRAPHICSCORE_DLL xiiLensFlareRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLensFlareRenderData, xiiRenderData);

public:
  xiiTexture2DResourceHandle m_hFlareTexture;
  float                      m_fIntensity = 1.0f;
  float                      m_fSize      = 0.1f;
};
using xiiLensFlareComponentManager = xiiComponentManager<class xiiLensFlareComponent, xiiBlockStorageType::Compact>;
class XII_GRAPHICSCORE_DLL xiiLensFlareComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiLensFlareComponent, xiiRenderComponent, xiiLensFlareComponentManager);

public:
  virtual void      SerializeComponent(xiiWorldWriter& s) const override;
  virtual void      DeserializeComponent(xiiWorldReader& s) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) override;
  xiiLensFlareComponent();
  ~xiiLensFlareComponent();
  void          SetTextureFile(xiiStringView s);
  xiiStringView GetTextureFile() const;
  void          SetIntensity(float f);
  float         GetIntensity() const { return m_fIntensity; }
  void          SetSize(float f);
  float         GetSize() const { return m_fSize; }

protected:
  void                       OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;
  xiiTexture2DResourceHandle m_hFlareTexture;
  float                      m_fIntensity = 1.0f;
  float                      m_fSize      = 0.1f;
};
