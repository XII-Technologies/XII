#pragma once

#include <Core/Scripting/ScriptClassResource.h>
#include <Core/World/EventMessageHandlerComponent.h>
#include <Foundation/Types/RangeView.h>

using xiiScriptComponentManager = xiiComponentManager<class xiiScriptComponent, xiiBlockStorageType::FreeList>;

class XII_CORE_DLL xiiScriptComponent : public xiiEventMessageHandlerComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiScriptComponent, xiiEventMessageHandlerComponent, xiiScriptComponentManager);

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

  /*virtual bool OnUnhandledMessage(xiiMessage& msg, bool bWasPostedMsg) override;
  virtual bool OnUnhandledMessage(xiiMessage& msg, bool bWasPostedMsg) const override;

  bool HandleUnhandledMessage(xiiMessage& msg, bool bWasPostedMsg);*/

  //////////////////////////////////////////////////////////////////////////
  // xiiEventMessageHandlerComponent

protected:
  // virtual bool HandlesEventMessage(const xiiEventMessage& msg) const override;

  //////////////////////////////////////////////////////////////////////////
  // xiiScriptComponent
public:
  xiiScriptComponent();
  ~xiiScriptComponent();

  void BroadcastEventMsg(xiiEventMessage& inout_msg);

  void                                SetScriptClass(const xiiScriptClassResourceHandle& hScript);
  const xiiScriptClassResourceHandle& GetScriptClass() const { return m_hScriptClass; }

  void        SetScriptClassFile(xiiStringView sFile); // [ property ]
  xiiStringView GetScriptClassFile() const;             // [ property ]

  void    SetUpdateInterval(xiiTime interval); // [ property ]
  xiiTime GetUpdateInterval() const;           // [ property ]

  //////////////////////////////////////////////////////////////////////////
  // Exposed Parameters
  const xiiRangeView<xiiStringView, xiiUInt32> GetParameters() const;
  void                                       SetParameter(xiiStringView sKey, const xiiVariant& value);
  void                                       RemoveParameter(xiiStringView sKey);
  bool                                       GetParameter(xiiStringView sKey, xiiVariant& out_value) const;

private:
  void InstantiateScript(bool bActivate);
  void ClearInstance(bool bDeactivate);
  void UpdateScheduling();

  const xiiAbstractFunctionProperty* GetScriptFunction(xiiUInt32 uiFunctionIndex);
  void                               CallScriptFunction(xiiUInt32 uiFunctionIndex);

  void ReloadScript();

  struct EventSender
  {
    const xiiRTTI*                         m_pMsgType = nullptr;
    xiiEventMessageSender<xiiEventMessage> m_Sender;
  };

  xiiHybridArray<EventSender, 2> m_EventSenders;

  xiiArrayMap<xiiHashedString, xiiVariant> m_Parameters;

  xiiScriptClassResourceHandle m_hScriptClass;
  xiiTime                      m_UpdateInterval = xiiTime::Zero();

  xiiSharedPtr<xiiScriptRTTI>     m_pScriptType;
  xiiUniquePtr<xiiScriptInstance> m_pInstance;
};
