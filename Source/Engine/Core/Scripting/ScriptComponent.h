/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/Scripting/ScriptClassResource.h>
#include <Core/World/EventMessageHandlerComponent.h>
#include <Foundation/Types/RangeView.h>

using xiiScriptComponentManager = xiiComponentManager<class xiiScriptComponent, xiiBlockStorageType::FreeList>;

/// Component that hosts and executes a script class instance on a game object.
///
/// Manages script execution lifecycle, variable access, parameter exposure, and event handling.
/// Supports configurable update intervals and simulation-only updates.
/// Provides integration between game objects and scripting systems through the xiiScriptClassResource.
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

  void       SetScriptVariable(const xiiHashedString& sName, const xiiVariant& value); // [ scriptable ]
  xiiVariant GetScriptVariable(const xiiHashedString& sName) const;                    // [ scriptable ]

  void                                SetScriptClass(const xiiScriptClassResourceHandle& hScript); // [ property ]
  const xiiScriptClassResourceHandle& GetScriptClass() const { return m_hScriptClass; }            // [ property ]

  void    SetUpdateInterval(xiiTime interval);                   // [ property ]
  xiiTime GetUpdateInterval() const { return m_UpdateInterval; } // [ property ]

  void SetUpdateOnlyWhenSimulating(bool bUpdate);                                  // [ property ]
  bool GetUpdateOnlyWhenSimulating() const { return m_bUpdateOnlyWhenSimulating; } // [ property ]

  void BroadcastEventMsg(xiiEventMessage& ref_msg);

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

  xiiArrayMap<xiiHashedString, xiiVariant> m_Parameters;

  xiiScriptClassResourceHandle m_hScriptClass;
  xiiTime                      m_UpdateInterval            = xiiTime::MakeZero();
  bool                         m_bUpdateOnlyWhenSimulating = true;

  xiiSharedPtr<xiiScriptRTTI>     m_pScriptType;
  xiiUniquePtr<xiiScriptInstance> m_pInstance;

private:
  struct EventSender
  {
    const xiiRTTI*                         m_pMsgType = nullptr;
    xiiEventMessageSender<xiiEventMessage> m_Sender;
  };

  xiiSmallArray<EventSender, 1> m_EventSenders;
};
