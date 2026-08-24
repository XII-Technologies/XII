/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Core/Graphics/Camera.h>
#include <Core/World/World.h>
#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Pipeline/Declarations.h>

class xiiView;
struct xiiResourceEvent;
struct xiiViewEvent;

class XII_GRAPHICSCORE_DLL xiiCameraComponentManager : public xiiComponentManager<class xiiCameraComponent, xiiBlockStorageType::Compact>
{
public:
  xiiCameraComponentManager(xiiWorld* pWorld);
  ~xiiCameraComponentManager();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  void Update(const xiiWorldModule::UpdateContext& context);

  void ReinitializeAllRenderTargetCameras();

  const xiiCameraComponent* GetCameraByUsageHint(xiiCameraUsageHint::Enum usageHint) const;
  xiiCameraComponent*       GetCameraByUsageHint(xiiCameraUsageHint::Enum usageHint);

private:
  friend class xiiCameraComponent;

  void AddRenderTargetCamera(xiiCameraComponent* pComponent);
  void RemoveRenderTargetCamera(xiiCameraComponent* pComponent);

  void OnViewCreated(const xiiViewEvent& viewEvent);
  void OnCameraConfigsChanged(void* dummy);

  xiiDynamicArray<xiiComponentHandle> m_ModifiedCameras;
  xiiDynamicArray<xiiComponentHandle> m_RenderTargetCameras;
};

class XII_GRAPHICSCORE_DLL xiiCameraComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiCameraComponent, xiiComponent, xiiCameraComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiCameraComponent

public:
  xiiCameraComponent();
  ~xiiCameraComponent();

  xiiEnum<xiiCameraUsageHint> GetUsageHint() const { return m_UsageHint; }   // [ property ]
  void                        SetUsageHint(xiiEnum<xiiCameraUsageHint> val); // [ property ]

  void    SetRenderTargetRectOffset(xiiVec2 value);                               // [ property ]
  xiiVec2 GetRenderTargetRectOffset() const { return m_vRenderTargetRectOffset; } // [ property ]

  void    SetRenderTargetRectSize(xiiVec2 value);                             // [ property ]
  xiiVec2 GetRenderTargetRectSize() const { return m_vRenderTargetRectSize; } // [ property ]

  xiiEnum<xiiCameraMode> GetCameraMode() const { return m_Mode; }   // [ property ]
  void                   SetCameraMode(xiiEnum<xiiCameraMode> val); // [ property ]

  float GetNearPlane() const { return m_fNearPlane; } // [ property ]
  void  SetNearPlane(float fVal);                     // [ property ]

  float GetFarPlane() const { return m_fFarPlane; } // [ property ]
  void  SetFarPlane(float fVal);                    // [ property ]

  float GetFieldOfView() const { return m_fPerspectiveFieldOfView; } // [ property ]
  void  SetFieldOfView(float fVal);                                  // [ property ]

  float GetOrthoDimension() const { return m_fOrthoDimension; } // [ property ]
  void  SetOrthoDimension(float fVal);                          // [ property ]

  /// \brief Per-camera render scale baseline (1.0 = native resolution).
  float GetRenderScale() const { return m_fRenderScale; } // [ property ]
  void  SetRenderScale(float fVal);                       // [ property ]

  xiiViewHandle GetRenderTargetView() const;

  float GetAperture() const { return m_fAperture; } // [ property ]
  void  SetAperture(float fAperture);               // [ property ]

  xiiTime GetShutterTime() const { return m_ShutterTime; } // [ property ]
  void    SetShutterTime(xiiTime shutterTime);             // [ property ]

  float GetISO() const { return m_fISO; } // [ property ]
  void  SetISO(float fISO);               // [ property ]

  float GetExposureCompensation() const { return m_fExposureCompensation; } // [ property ]
  void  SetExposureCompensation(float fEC);                                 // [ property ]

  float GetEV100() const;    // [ property ]
  float GetExposure() const; // [ property ]

  xiiTagSet m_IncludeTags; // [ property ]
  xiiTagSet m_ExcludeTags; // [ property ]

  void ApplySettingsToView(xiiView* pView) const;

private:
  void UpdateRenderTargetCamera();
  void ShowStats(xiiView* pView);

  void ResourceChangeEventHandler(const xiiResourceEvent& e);

  xiiEnum<xiiCameraUsageHint>        m_UsageHint;
  xiiEnum<xiiCameraMode>             m_Mode;
  float                              m_fNearPlane              = 0.25f;
  float                              m_fFarPlane               = 1000.0f;
  float                              m_fPerspectiveFieldOfView = 60.0f;
  float                              m_fOrthoDimension         = 10.0f;
  float                              m_fRenderScale            = 1.0f;

  float   m_fAperture             = 1.0f;
  xiiTime m_ShutterTime           = xiiTime::MakeFromSeconds(1.0f);
  float   m_fISO                  = 100.0f;
  float   m_fExposureCompensation = 0.0f;

  void MarkAsModified();
  void MarkAsModified(xiiCameraComponentManager* pCameraManager);

  bool m_bIsModified              = false;
  bool m_bShowStats               = false;
  bool m_bRenderTargetInitialized = false;

  // -1 for none, 0 to 9 for ALT+Number
  xiiInt8 m_iEditorShortcut = -1; // [ property ]

  void ActivateRenderToTexture();
  void DeactivateRenderToTexture();

  xiiViewHandle m_hRenderTargetView;
  xiiVec2       m_vRenderTargetRectOffset = xiiVec2(0.0f);
  xiiVec2       m_vRenderTargetRectSize   = xiiVec2(1.0f);
  xiiCamera     m_RenderTargetCamera;
};
