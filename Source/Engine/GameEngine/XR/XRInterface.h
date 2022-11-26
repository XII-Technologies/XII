#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/ActorSystem/Actor.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <GameEngine/XR/Declarations.h>
#include <RendererFoundation/RendererFoundationDLL.h>

using xiiRenderPipelineResourceHandle = xiiTypedResourceHandle<class xiiRenderPipelineResource>;
class xiiViewHandle;
class xiiCamera;
class xiiGALTextureHandle;
class xiiWorld;
class xiiView;
class xiiXRInputDevice;
class xiiWindowBase;
class xiiWindowOutputTargetBase;

/// \brief XR singleton interface. Needs to be initialized to be used for VR or AR purposes.
///
/// To be used in a project the project needs to have an enabled xiiXRConfig with a
/// set render pipeline in the platform profile.
/// To then use the interface, Initialize must be called first and on success CreateActor.
/// Everything else is optional.
/// Aquire interface via xiiSingletonRegistry::GetSingletonInstance<xiiXRInterface>().
class xiiXRInterface
{
public:
  /// \brief Returns whether an HMD is available. Can be used to decide
  /// whether it makes sense to call Initialize at all.
  virtual bool IsHmdPresent() const = 0;

  /// \name Setup
  ///@{

  /// \brief Initializes the XR system. This can be quite time consuming
  /// as it will generally start supporting applications needed to run
  /// and start up the HMD if it went to sleep.
  virtual xiiResult Initialize() = 0;
  /// \brief Shuts down the XR system again.
  virtual void Deinitialize() = 0;
  /// \brief Returns whether the XR system is initialized.
  virtual bool IsInitialized() const = 0;

  ///@}
  /// \name Devices
  ///@{

  /// \brief Returns general HMD information.
  virtual const xiiHMDInfo& GetHmdInfo() const = 0;

  /// \brief Returns the XR input device.
  virtual xiiXRInputDevice& GetXRInput() const = 0;

  ///@}
  /// \name View
  ///@{

  /// \brief Returns true if a companion window can be passed into CreateActor.
  virtual bool SupportsCompanionView() = 0;

  /// \brief Creates a XR actor by trying to startup an XR session.
  ///
  /// If SupportsCompanionView is true (VR only), a normal window and window output can be passed in.
  /// The window will be used to blit the VR output into the window.
  virtual xiiUniquePtr<xiiActor> CreateActor(xiiView* pView, xiiGALMSAASampleCount::Enum msaaCount = xiiGALMSAASampleCount::None, xiiUniquePtr<xiiWindowBase> companionWindow = nullptr, xiiUniquePtr<xiiWindowOutputTargetGAL> companionWindowOutput = nullptr) = 0;

  ///@}
  /// \name Internal
  ///@{

  /// \brief Called by xiiWindowOutputTargetXR::RenderCompanionView
  /// Returns the color texture to be used by the companion view if enabled, otherwise an invalid handle.
  virtual xiiGALTextureHandle GetCurrentTexture() = 0;

  /// \brief Called when the actor created by 'CreateActor' is destroyed.
  virtual void OnActorDestroyed() = 0;

  ///@}
};
