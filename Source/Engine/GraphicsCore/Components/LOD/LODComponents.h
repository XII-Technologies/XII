#pragma once
#include <GraphicsCore/Components/Render/RenderComponent.h>
#include <GraphicsCore/Pipeline/RenderData.h>
struct xiiMsgExtractRenderData;

// ---- LODGroup ----
class XII_GRAPHICSCORE_DLL xiiLODGroupRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLODGroupRenderData, xiiRenderData);

public:
  xiiUInt8 m_uiActiveLOD = 0;
  float    m_fLODFactor  = 1.0f;
  xiiUInt8 m_uiNumLODs   = 1;
};
using xiiLODGroupComponentManager = xiiComponentManager<class xiiLODGroupComponent, xiiBlockStorageType::Compact>;
class XII_GRAPHICSCORE_DLL xiiLODGroupComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiLODGroupComponent, xiiRenderComponent, xiiLODGroupComponentManager);

public:
  virtual void      SerializeComponent(xiiWorldWriter& s) const override;
  virtual void      DeserializeComponent(xiiWorldReader& s) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) override;
  xiiLODGroupComponent();
  ~xiiLODGroupComponent();
  void     SetLODFactor(float f);
  float    GetLODFactor() const { return m_fLODFactor; }
  void     SetNumLODs(xiiUInt8 n);
  xiiUInt8 GetNumLODs() const { return m_uiNumLODs; }

protected:
  void     OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;
  float    m_fLODFactor  = 1.0f;
  xiiUInt8 m_uiActiveLOD = 0;
  xiiUInt8 m_uiNumLODs   = 1;
};

// ---- Occlusion ----
class XII_GRAPHICSCORE_DLL xiiOcclusionRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiOcclusionRenderData, xiiRenderData);

public:
  bool m_bOccluder = true;
  bool m_bOccludee = true;
};
using xiiOcclusionComponentManager = xiiComponentManager<class xiiOcclusionComponent, xiiBlockStorageType::Compact>;
class XII_GRAPHICSCORE_DLL xiiOcclusionComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiOcclusionComponent, xiiRenderComponent, xiiOcclusionComponentManager);

public:
  virtual void      SerializeComponent(xiiWorldWriter& s) const override;
  virtual void      DeserializeComponent(xiiWorldReader& s) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) override;
  xiiOcclusionComponent();
  ~xiiOcclusionComponent();
  void SetOccluder(bool b);
  bool GetOccluder() const { return m_bOccluder; }
  void SetOccludee(bool b);
  bool GetOccludee() const { return m_bOccludee; }

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;
  bool m_bOccluder = true;
  bool m_bOccludee = true;
};

// ---- StreamingHint ----
class XII_GRAPHICSCORE_DLL xiiStreamingHintRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiStreamingHintRenderData, xiiRenderData);

public:
  float m_fPriority     = 0.5f;
  float m_fStreamRadius = 50.0f;
};
using xiiStreamingHintComponentManager = xiiComponentManager<class xiiStreamingHintComponent, xiiBlockStorageType::Compact>;
class XII_GRAPHICSCORE_DLL xiiStreamingHintComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiStreamingHintComponent, xiiRenderComponent, xiiStreamingHintComponentManager);

public:
  virtual void      SerializeComponent(xiiWorldWriter& s) const override;
  virtual void      DeserializeComponent(xiiWorldReader& s) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) override;
  xiiStreamingHintComponent();
  ~xiiStreamingHintComponent();
  void  SetPriority(float f);
  float GetPriority() const { return m_fPriority; }
  void  SetStreamRadius(float f);
  float GetStreamRadius() const { return m_fStreamRadius; }

protected:
  void  OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;
  float m_fPriority     = 0.5f;
  float m_fStreamRadius = 50.0f;
};

// ---- BatchingHint ----
class XII_GRAPHICSCORE_DLL xiiBatchingHintRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiBatchingHintRenderData, xiiRenderData);

public:
  xiiUInt32 m_uiBatchKey = 0;
  bool      m_bEligible  = true;
};
using xiiBatchingHintComponentManager = xiiComponentManager<class xiiBatchingHintComponent, xiiBlockStorageType::Compact>;
class XII_GRAPHICSCORE_DLL xiiBatchingHintComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiBatchingHintComponent, xiiRenderComponent, xiiBatchingHintComponentManager);

public:
  virtual void      SerializeComponent(xiiWorldWriter& s) const override;
  virtual void      DeserializeComponent(xiiWorldReader& s) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) override;
  xiiBatchingHintComponent();
  ~xiiBatchingHintComponent();
  void      SetBatchKey(xiiUInt32 k);
  xiiUInt32 GetBatchKey() const { return m_uiBatchKey; }
  void      SetEligible(bool b);
  bool      GetEligible() const { return m_bEligible; }

protected:
  void      OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;
  xiiUInt32 m_uiBatchKey = 0;
  bool      m_bEligible  = true;
};

// ---- DistanceField ----
class XII_GRAPHICSCORE_DLL xiiDistanceFieldRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDistanceFieldRenderData, xiiRenderData);

public:
  xiiTexture3DResourceHandle m_hSDF;
  float                      m_fWorldScale = 1.0f;
};
using xiiDistanceFieldComponentManager = xiiComponentManager<class xiiDistanceFieldComponent, xiiBlockStorageType::Compact>;
class XII_GRAPHICSCORE_DLL xiiDistanceFieldComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiDistanceFieldComponent, xiiRenderComponent, xiiDistanceFieldComponentManager);

public:
  virtual void      SerializeComponent(xiiWorldWriter& s) const override;
  virtual void      DeserializeComponent(xiiWorldReader& s) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) override;
  xiiDistanceFieldComponent();
  ~xiiDistanceFieldComponent();
  void          SetSDFFile(xiiStringView s);
  xiiStringView GetSDFFile() const;
  void          SetWorldScale(float f);
  float         GetWorldScale() const { return m_fWorldScale; }

protected:
  void                       OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;
  xiiTexture3DResourceHandle m_hSDF;
  float                      m_fWorldScale = 1.0f;
};
