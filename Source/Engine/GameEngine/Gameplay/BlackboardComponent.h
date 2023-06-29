#pragma once

#include <Core/Messages/EventMessage.h>
#include <Core/Utils/Blackboard.h>
#include <GameEngine/GameEngineDLL.h>

struct xiiBlackboardEntry
{
  xiiHashedString                      m_sName;
  xiiVariant                           m_InitialValue;
  xiiBitflags<xiiBlackboardEntryFlags> m_Flags;

  void        SetName(const char* szName) { m_sName.Assign(szName); }
  const char* GetName() const { return m_sName; }

  xiiResult Serialize(xiiStreamWriter& ref_stream) const;
  xiiResult Deserialize(xiiStreamReader& ref_stream);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GAMEENGINE_DLL, xiiBlackboardEntry);

//////////////////////////////////////////////////////////////////////////

struct XII_GAMEENGINE_DLL xiiMsgBlackboardEntryChanged : public xiiEventMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgBlackboardEntryChanged, xiiEventMessage);

  xiiHashedString m_sName;
  xiiVariant      m_OldValue;
  xiiVariant      m_NewValue;

private:
  const char* GetName() const { return m_sName; }
  void        SetName(const char* szName) { m_sName.Assign(szName); }
};

//////////////////////////////////////////////////////////////////////////

struct xiiMsgUpdateLocalBounds;
struct xiiMsgExtractRenderData;

using xiiBlackboardComponentManager = xiiComponentManager<class xiiBlackboardComponent, xiiBlockStorageType::Compact>;

/// \brief This component holds a xiiBlackboard which can be used to share state between multiple components.
class XII_GAMEENGINE_DLL xiiBlackboardComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiBlackboardComponent, xiiComponent, xiiBlackboardComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& ref_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& ref_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiBlackboardComponent

public:
  xiiBlackboardComponent();
  xiiBlackboardComponent(xiiBlackboardComponent&& other);
  ~xiiBlackboardComponent();

  xiiBlackboardComponent& operator=(xiiBlackboardComponent&& other);

  /// \brief Try to find a xiiBlackboardComponent on pSearchObject or its parents with the given name and returns its blackboard.
  ///
  /// The blackboard name is only checked if the given name is not empty. If no matching blackboard component is found,
  /// the function will call xiiBlackboard::GetOrCreateGlobal with the given name. This if you provide a name, you will always get a result, either from a component or from the global storage.
  ///
  /// \sa xiiBlackboard::GetOrCreateGlobal()
  static xiiSharedPtr<xiiBlackboard> FindBlackboard(xiiGameObject* pSearchObject, xiiStringView sBlackboardName = xiiStringView());

  /// \brief Returns the blackboard owned by this component
  const xiiSharedPtr<xiiBlackboard>& GetBoard();
  xiiSharedPtr<const xiiBlackboard>  GetBoard() const;

  void SetShowDebugInfo(bool bShow); // [ property ]
  bool GetShowDebugInfo() const;     // [ property ]

  void SetSendEntryChangedMessage(bool bSend); // [ property ]
  bool GetSendEntryChangedMessage() const;     // [ property ]

  void        SetBlackboardName(const char* szName); // [ property ]
  const char* GetBlackboardName() const;             // [ property ]

  void       SetEntryValue(const char* szName, const xiiVariant& value); // [ scriptable ]
  xiiVariant GetEntryValue(const char* szName) const;                    // [ scriptable ]

private:
  xiiUInt32                 Entries_GetCount() const;
  const xiiBlackboardEntry& Entries_GetValue(xiiUInt32 uiIndex) const;
  void                      Entries_SetValue(xiiUInt32 uiIndex, const xiiBlackboardEntry& entry);
  void                      Entries_Insert(xiiUInt32 uiIndex, const xiiBlackboardEntry& entry);
  void                      Entries_Remove(xiiUInt32 uiIndex);

  static xiiBlackboard* Reflection_FindBlackboard(xiiGameObject* pSearchObject, xiiStringView sBlackboardName);

  void OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& msg) const;
  void OnExtractRenderData(xiiMsgExtractRenderData& msg) const;
  void OnEntryChanged(const xiiBlackboard::EntryEvent& e);

  xiiSharedPtr<xiiBlackboard> m_pBoard;

  // this array is not held during runtime, it is only needed during editor time until the component is serialized out
  xiiDynamicArray<xiiBlackboardEntry> m_InitialEntries;

  xiiEventMessageSender<xiiMsgBlackboardEntryChanged> m_EntryChangedSender; // [ event ]
};
