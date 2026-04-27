#pragma once
#include <GraphicsCore/Components/Render/RenderComponent.h>
#include <GraphicsCore/Pipeline/RenderData.h>
struct xiiMsgExtractRenderData;

// ---- RayTracingGeometry ----
class XII_GRAPHICSCORE_DLL xiiRayTracingGeometryRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRayTracingGeometryRenderData, xiiRenderData);

public:
  xiiMeshResourceHandle m_hMesh;
  bool                  m_bDynamic        = false;
  xiiUInt8              m_uiGeometryFlags = 0;
};
using xiiRayTracingGeometryComponentManager = xiiComponentManager<class xiiRayTracingGeometryComponent, xiiBlockStorageType::Compact>;
class XII_GRAPHICSCORE_DLL xiiRayTracingGeometryComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiRayTracingGeometryComponent, xiiRenderComponent, xiiRayTracingGeometryComponentManager);

public:
  virtual void      SerializeComponent(xiiWorldWriter& s) const override;
  virtual void      DeserializeComponent(xiiWorldReader& s) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) override;
  xiiRayTracingGeometryComponent();
  ~xiiRayTracingGeometryComponent();
  void          SetMeshFile(xiiStringView s);
  xiiStringView GetMeshFile() const;
  void          SetDynamic(bool b);
  bool          GetDynamic() const { return m_bDynamic; }

protected:
  void                  OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;
  xiiMeshResourceHandle m_hMesh;
  bool                  m_bDynamic        = false;
  xiiUInt8              m_uiGeometryFlags = 0;
};

// ---- AccelerationStructure ----
class XII_GRAPHICSCORE_DLL xiiAccelerationStructureRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAccelerationStructureRenderData, xiiRenderData);

public:
  bool m_bTopLevel  = true;
  bool m_bCompacted = true;
};
using xiiAccelerationStructureComponentManager = xiiComponentManager<class xiiAccelerationStructureComponent, xiiBlockStorageType::Compact>;
class XII_GRAPHICSCORE_DLL xiiAccelerationStructureComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiAccelerationStructureComponent, xiiRenderComponent, xiiAccelerationStructureComponentManager);

public:
  virtual void      SerializeComponent(xiiWorldWriter& s) const override;
  virtual void      DeserializeComponent(xiiWorldReader& s) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) override;
  xiiAccelerationStructureComponent();
  ~xiiAccelerationStructureComponent();
  void SetTopLevel(bool b);
  bool GetTopLevel() const { return m_bTopLevel; }
  void SetCompacted(bool b);
  bool GetCompacted() const { return m_bCompacted; }

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;
  bool m_bTopLevel  = true;
  bool m_bCompacted = true;
};

// ---- GPUDriven ----
class XII_GRAPHICSCORE_DLL xiiGPUDrivenRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGPUDrivenRenderData, xiiRenderData);

public:
  xiiUInt32 m_uiMaxDraws        = 1024;
  bool      m_bFrustumCulling   = true;
  bool      m_bOcclusionCulling = true;
};
using xiiGPUDrivenComponentManager = xiiComponentManager<class xiiGPUDrivenComponent, xiiBlockStorageType::Compact>;
class XII_GRAPHICSCORE_DLL xiiGPUDrivenComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiGPUDrivenComponent, xiiRenderComponent, xiiGPUDrivenComponentManager);

public:
  virtual void      SerializeComponent(xiiWorldWriter& s) const override;
  virtual void      DeserializeComponent(xiiWorldReader& s) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) override;
  xiiGPUDrivenComponent();
  ~xiiGPUDrivenComponent();
  void      SetMaxDraws(xiiUInt32 n);
  xiiUInt32 GetMaxDraws() const { return m_uiMaxDraws; }
  void      SetFrustumCulling(bool b);
  bool      GetFrustumCulling() const { return m_bFrustumCulling; }
  void      SetOcclusionCulling(bool b);
  bool      GetOcclusionCulling() const { return m_bOcclusionCulling; }

protected:
  void      OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;
  xiiUInt32 m_uiMaxDraws        = 1024;
  bool      m_bFrustumCulling   = true;
  bool      m_bOcclusionCulling = true;
};

// ---- AsyncCompute ----
class XII_GRAPHICSCORE_DLL xiiAsyncComputeRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAsyncComputeRenderData, xiiRenderData);

public:
  xiiString m_sPassName;
  xiiUInt32 m_uiThreadGroupsX = 1;
  xiiUInt32 m_uiThreadGroupsY = 1;
  xiiUInt32 m_uiThreadGroupsZ = 1;
};
using xiiAsyncComputeComponentManager = xiiComponentManager<class xiiAsyncComputeComponent, xiiBlockStorageType::Compact>;
class XII_GRAPHICSCORE_DLL xiiAsyncComputeComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiAsyncComputeComponent, xiiRenderComponent, xiiAsyncComputeComponentManager);

public:
  virtual void      SerializeComponent(xiiWorldWriter& s) const override;
  virtual void      DeserializeComponent(xiiWorldReader& s) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) override;
  xiiAsyncComputeComponent();
  ~xiiAsyncComputeComponent();
  void          SetPassName(xiiStringView s);
  xiiStringView GetPassName() const { return m_sPassName; }
  void          SetDispatch(xiiUInt32 x, xiiUInt32 y, xiiUInt32 z);

protected:
  void      OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;
  xiiString m_sPassName;
  xiiUInt32 m_uiTGX = 1;
  xiiUInt32 m_uiTGY = 1;
  xiiUInt32 m_uiTGZ = 1;
};

// ---- RayQuery ----
class XII_GRAPHICSCORE_DLL xiiRayQueryRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRayQueryRenderData, xiiRenderData);

public:
  xiiVec3 m_vRayOrigin = xiiVec3::MakeZero();
  xiiVec3 m_vRayDir    = xiiVec3(0, 0, -1);
  float   m_fMaxT      = 100.0f;
};
using xiiRayQueryComponentManager = xiiComponentManager<class xiiRayQueryComponent, xiiBlockStorageType::Compact>;
class XII_GRAPHICSCORE_DLL xiiRayQueryComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiRayQueryComponent, xiiRenderComponent, xiiRayQueryComponentManager);

public:
  virtual void      SerializeComponent(xiiWorldWriter& s) const override;
  virtual void      DeserializeComponent(xiiWorldReader& s) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) override;
  xiiRayQueryComponent();
  ~xiiRayQueryComponent();
  void  SetMaxT(float f);
  float GetMaxT() const { return m_fMaxT; }

protected:
  void  OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;
  float m_fMaxT = 100.0f;
};
