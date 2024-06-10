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

  //////////////////////////////////////////////////////////////////////////
  // xiiScriptComponent
public:
  xiiScriptComponent();
  ~xiiScriptComponent();

  bool SendEventMessage(xiiMessage& ref_msg);
  void PostEventMessage(xiiMessage& ref_msg, xiiTime delay);

  void                                SetScriptClass(const xiiScriptClassResourceHandle& hScript);
  const xiiScriptClassResourceHandle& GetScriptClass() const { return m_hScriptClass; }

  void          SetScriptClassFile(xiiStringView sFile); // [ property ]
  xiiStringView GetScriptClassFile() const;              // [ property ]

  void    SetUpdateInterval(xiiTime interval); // [ property ]
  xiiTime GetUpdateInterval() const;           // [ property ]

  //////////////////////////////////////////////////////////////////////////
  // Exposed Parameters
  const xiiRangeView<xiiStringView, xiiUInt32> GetParameters() const;
  void                                         SetParameter(xiiStringView sKey, const xiiVariant& value);
  void                                         RemoveParameter(xiiStringView sKey);
  bool                                         GetParameter(xiiStringView sKey, xiiVariant& out_value) const;

  XII_ALWAYS_INLINE xiiScriptInstance* GetScriptInstance() { return m_pInstance.Borrow(); }

private:
  void InstantiateScript(bool bActivate);
  void ClearInstance(bool bDeactivate);
  void AddUpdateFunctionToSchedule();
  void RemoveUpdateFunctionToSchedule();

  const xiiAbstractFunctionProperty* GetScriptFunction(xiiUInt32 uiFunctionIndex);
  void                               CallScriptFunction(xiiUInt32 uiFunctionIndex);

  void ReloadScript();

  xiiEventMessageSender<xiiMessage>& FindSender(xiiMessage& ref_msg);

  struct EventSender
  {
    const xiiRTTI*                    m_pMsgType = nullptr;
    xiiEventMessageSender<xiiMessage> m_Sender;
  };

  xiiHybridArray<EventSender, 2> m_EventSenders;

  xiiArrayMap<xiiHashedString, xiiVariant> m_Parameters;

  xiiScriptClassResourceHandle m_hScriptClass;
  xiiTime                      m_UpdateInterval = xiiTime::MakeZero();

  xiiSharedPtr<xiiScriptRTTI>     m_pScriptType;
  xiiUniquePtr<xiiScriptInstance> m_pInstance;
};
