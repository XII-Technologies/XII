/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/ActorSystem/ActorPlugin.h>

#include <Core/GameApplication/WindowOutputTargetBase.h>
#include <Core/System/Window.h>

class xiiActor;
class xiiWindowOutputTargetBase;
class xiiWindowBase;

class XII_CORE_DLL xiiActorPluginWindow : public xiiActorPlugin
{
  XII_ADD_DYNAMIC_REFLECTION(xiiActorPluginWindow, xiiActorPlugin);

public:
  virtual xiiWindowBase*             GetWindow() const       = 0;
  virtual xiiWindowOutputTargetBase* GetOutputTarget() const = 0;

protected:
  virtual void Update() override;
};

class XII_CORE_DLL xiiActorPluginWindowOwner : public xiiActorPluginWindow
{
  XII_ADD_DYNAMIC_REFLECTION(xiiActorPluginWindowOwner, xiiActorPluginWindow);

public:
  virtual ~xiiActorPluginWindowOwner();
  virtual xiiWindowBase*             GetWindow() const override;
  virtual xiiWindowOutputTargetBase* GetOutputTarget() const override;

  xiiUniquePtr<xiiWindowBase>             m_pWindow;
  xiiUniquePtr<xiiWindowOutputTargetBase> m_pWindowOutputTarget;
};

class XII_CORE_DLL xiiActorPluginWindowShared : public xiiActorPluginWindow
{
  XII_ADD_DYNAMIC_REFLECTION(xiiActorPluginWindowShared, xiiActorPluginWindow);

public:
  virtual xiiWindowBase*             GetWindow() const override;
  virtual xiiWindowOutputTargetBase* GetOutputTarget() const override;

  xiiWindowBase*             m_pWindow             = nullptr;
  xiiWindowOutputTargetBase* m_pWindowOutputTarget = nullptr;
};
