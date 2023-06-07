#pragma once

#include <Core/ActorSystem/ActorPluginWindow.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Reflection/Reflection.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

class xiiXRInterface;

/// \brief XR Window base implementation. Optionally wraps a companion window.
class XII_GAMEENGINE_DLL xiiWindowXR : public xiiWindowBase
{
public:
  xiiWindowXR(xiiXRInterface* pVrInterface, xiiUniquePtr<xiiWindowBase> pCompanionWindow);
  virtual ~xiiWindowXR();

  virtual xiiSizeU32 GetClientAreaSize() const override;

  virtual xiiWindowHandle GetNativeWindowHandle() const override;

  virtual bool IsFullscreenWindow(bool bOnlyProperFullscreenMode) const override;

  virtual void ProcessWindowMessages() override;

  virtual void AddReference() override { m_iReferenceCount.Increment(); }
  virtual void RemoveReference() override { m_iReferenceCount.Decrement(); }

  /// \brief Returns the companion window if present.
  const xiiWindowBase* GetCompanionWindow() const;

private:
  xiiXRInterface*             m_pVrInterface = nullptr;
  xiiUniquePtr<xiiWindowBase> m_pCompanionWindow;
  xiiAtomicInteger32          m_iReferenceCount = 0;
};

/// \brief XR Window output target base implementation. Optionally wraps a companion window output target.
class XII_GAMEENGINE_DLL xiiWindowOutputTargetXR : public xiiWindowOutputTargetBase
{
public:
  xiiWindowOutputTargetXR(xiiXRInterface* pVrInterface, xiiUniquePtr<xiiWindowOutputTargetGAL> pCompanionWindowOutputTarget);
  ~xiiWindowOutputTargetXR();

  virtual void      Present(bool bEnableVSync) override;
  void              RenderCompanionView(bool bThrottleCompanionView = true);
  virtual xiiResult CaptureImage(xiiImage& out_image) override;

  /// \brief Returns the companion window output target if present.
  const xiiWindowOutputTargetBase* GetCompanionWindowOutputTarget() const;

private:
  xiiXRInterface*                        m_pXrInterface = nullptr;
  xiiTime                                m_LastPresent;
  xiiUniquePtr<xiiWindowOutputTargetGAL> m_pCompanionWindowOutputTarget;
  xiiConstantBufferStorageHandle         m_hCompanionConstantBuffer;
  xiiShaderResourceHandle                m_hCompanionShader;
};

/// \brief XR actor plugin window base implementation. Optionally wraps a companion window and output target.
class XII_GAMEENGINE_DLL xiiActorPluginWindowXR : public xiiActorPluginWindow
{
  XII_ADD_DYNAMIC_REFLECTION(xiiActorPluginWindowXR, xiiActorPluginWindow);

public:
  xiiActorPluginWindowXR(xiiXRInterface* pVrInterface, xiiUniquePtr<xiiWindowBase> pCompanionWindow, xiiUniquePtr<xiiWindowOutputTargetGAL> pCompanionWindowOutput);
  ~xiiActorPluginWindowXR();
  void Initialize();

  virtual xiiWindowBase*             GetWindow() const override;
  virtual xiiWindowOutputTargetBase* GetOutputTarget() const override;

protected:
  virtual void Update() override;

private:
  xiiXRInterface*                       m_pVrInterface = nullptr;
  xiiUniquePtr<xiiWindowXR>             m_pWindow;
  xiiUniquePtr<xiiWindowOutputTargetXR> m_pWindowOutputTarget;
};
