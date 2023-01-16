#pragma once

#include <TypeScriptPlugin/TsBinding/TsBinding.h>

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/Scripting/DuktapeContext.h>
#include <Core/World/Component.h>
#include <Core/World/EventMessageHandlerComponent.h>
#include <Core/World/World.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Types/RangeView.h>
#include <TypeScriptPlugin/Transpiler/Transpiler.h>

class xiiTypeScriptBinding;

struct XII_TYPESCRIPTPLUGIN_DLL xiiMsgTypeScriptMsgProxy : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgTypeScriptMsgProxy, xiiMessage);

  xiiUInt32 m_uiTypeNameHash = 0;
  xiiUInt32 m_uiStashIndex   = 0;
};

class XII_TYPESCRIPTPLUGIN_DLL xiiTypeScriptComponentManager : public xiiComponentManager<class xiiTypeScriptComponent, xiiBlockStorageType::FreeList>
{
  using SUPER = xiiComponentManager<class xiiTypeScriptComponent, xiiBlockStorageType::FreeList>;

public:
  xiiTypeScriptComponentManager(xiiWorld* pWorld);
  ~xiiTypeScriptComponentManager();

  virtual void Initialize() override;
  virtual void Deinitialize() override;
  virtual void OnSimulationStarted() override;

  xiiTypeScriptBinding& GetTsBinding() const { return m_TsBinding; }

private:
  void Update(const xiiWorldModule::UpdateContext& context);

  mutable xiiTypeScriptBinding m_TsBinding;
};

//////////////////////////////////////////////////////////////////////////

class XII_TYPESCRIPTPLUGIN_DLL xiiTypeScriptComponent : public xiiEventMessageHandlerComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiTypeScriptComponent, xiiEventMessageHandlerComponent, xiiTypeScriptComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

protected:
  virtual void SerializeComponent(xiiWorldWriter& stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& stream) override;
  virtual void Initialize() override;
  virtual void Deinitialize() override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;
  virtual void OnSimulationStarted() override;

  virtual bool HandlesMessage(const xiiMessage& msg) const override;
  virtual bool OnUnhandledMessage(xiiMessage& msg, bool bWasPostedMsg) override;
  virtual bool OnUnhandledMessage(xiiMessage& msg, bool bWasPostedMsg) const override;

  bool HandleUnhandledMessage(xiiMessage& msg, bool bWasPostedMsg);

  //////////////////////////////////////////////////////////////////////////
  // xiiTypeScriptComponent

public:
  xiiTypeScriptComponent();
  ~xiiTypeScriptComponent();

  void BroadcastEventMsg(xiiEventMessage& msg);

  void SetUpdateInterval(xiiTime interval) { m_UpdateInterval = interval; }

  void           SetTypeScriptComponentGuid(const xiiUuid& hResource);
  const xiiUuid& GetTypeScriptComponentGuid() const;

private:
  struct EventSender
  {
    const xiiRTTI*                         m_pMsgType = nullptr;
    xiiEventMessageSender<xiiEventMessage> m_Sender;
  };

  xiiHybridArray<EventSender, 2> m_EventSenders;

  bool CallTsFunc(const char* szFuncName);
  void Update(xiiTypeScriptBinding& script);
  void SetExposedVariables();

  xiiTypeScriptBinding::TsComponentTypeInfo m_ComponentTypeInfo;

  void        SetTypeScriptComponentFile(const char* szFile); // [ property ]
  const char* GetTypeScriptComponentFile() const;             // [ property ]

  void OnMsgTypeScriptMsgProxy(xiiMsgTypeScriptMsgProxy& msg); // [ message handler ]

  enum UserFlag
  {
    InitializedTS = 0,
    OnActivatedTS = 1,
    NoTsTick      = 2,
    SimStartedTS  = 3,
    ScriptFailure = 4,
  };

private:
  xiiUuid m_TypeScriptComponentGuid;
  xiiTime m_LastUpdate;
  xiiTime m_UpdateInterval = xiiTime::Seconds(-1); // deactivated by default

  //////////////////////////////////////////////////////////////////////////
  // Exposed Parameters

public:
  const xiiRangeView<const char*, xiiUInt32> GetParameters() const;
  void                                       SetParameter(const char* szKey, const xiiVariant& value);
  void                                       RemoveParameter(const char* szKey);
  bool                                       GetParameter(const char* szKey, xiiVariant& out_value) const;

private:
  xiiArrayMap<xiiHashedString, xiiVariant> m_Parameters;
};
