#pragma once

#include <Core/World/SettingsComponent.h>
#include <Core/World/SettingsComponentManager.h>
#include <Foundation/Types/SharedPtr.h>
#include <GraphicsCore/BakedProbes/BakingInterface.h>
#include <GraphicsCore/Declarations.h>
#include <GraphicsFoundation/GraphicsFoundationDLL.h>

struct xiiMsgUpdateLocalBounds;
struct xiiMsgExtractRenderData;
struct xiiRenderWorldRenderEvent;
class xiiAbstractObjectNode;

class XII_RENDERERCORE_DLL xiiBakedProbesComponentManager : public xiiSettingsComponentManager<class xiiBakedProbesComponent>
{
public:
  xiiBakedProbesComponentManager(xiiWorld* pWorld);
  ~xiiBakedProbesComponentManager();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  xiiMeshResourceHandle     m_hDebugSphere;
  xiiMaterialResourceHandle m_hDebugMaterial;

private:
  void RenderDebug(const xiiWorldModule::UpdateContext& updateContext);
  void OnRenderEvent(const xiiRenderWorldRenderEvent& e);
  void CreateDebugResources();
};

class XII_RENDERERCORE_DLL xiiBakedProbesComponent : public xiiSettingsComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiBakedProbesComponent, xiiSettingsComponent, xiiBakedProbesComponentManager);

public:
  xiiBakedProbesComponent();
  ~xiiBakedProbesComponent();

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  xiiBakingSettings m_Settings; // [ property ]

  void SetShowDebugOverlay(bool bShow);                            // [ property ]
  bool GetShowDebugOverlay() const { return m_bShowDebugOverlay; } // [ property ]

  void SetShowDebugProbes(bool bShow);                           // [ property ]
  bool GetShowDebugProbes() const { return m_bShowDebugProbes; } // [ property ]

  void SetUseTestPosition(bool bUse);                            // [ property ]
  bool GetUseTestPosition() const { return m_bUseTestPosition; } // [ property ]

  void           SetTestPosition(const xiiVec3& vPos);               // [ property ]
  const xiiVec3& GetTestPosition() const { return m_vTestPosition; } // [ property ]

  void OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& ref_msg);
  void OnExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;

  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

private:
  void RenderDebugOverlay();
  void OnObjectCreated(const xiiAbstractObjectNode& node);

  xiiHashedString m_sProbeTreeResourcePrefix;

  bool    m_bShowDebugOverlay = false;
  bool    m_bShowDebugProbes  = false;
  bool    m_bUseTestPosition  = false;
  xiiVec3 m_vTestPosition     = xiiVec3::MakeZero();

  struct RenderDebugViewTask;
  xiiSharedPtr<RenderDebugViewTask> m_pRenderDebugViewTask;

  xiiGALTextureHandle m_hDebugViewTexture;
};
