/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/Messages/EventMessage.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/Utils/Blackboard.h>
#include <GameEngine/GameEngineDLL.h>

struct xiiMsgUpdateLocalBounds;
struct xiiMsgExtractRenderData;

using xiiBlackboardTemplateResourceHandle = xiiTypedResourceHandle<class xiiBlackboardTemplateResource>;

struct xiiBlackboardEntry
{
  xiiHashedString                      m_sName;
  xiiVariant                           m_InitialValue;
  xiiBitflags<xiiBlackboardEntryFlags> m_Flags;

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  xiiResult Deserialize(xiiStreamReader& inout_stream);
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

/// This base component represents a xiiBlackboard, which can be used to share state between multiple components and objects.
///
/// The derived implementations may either create their own blackboards or reference other blackboards.
class XII_GAMEENGINE_DLL xiiBlackboardComponent : public xiiComponent
{
  XII_DECLARE_ABSTRACT_COMPONENT_TYPE(xiiBlackboardComponent, xiiComponent);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiBlackboardComponent

public:
  xiiBlackboardComponent();
  ~xiiBlackboardComponent();

  /// Try to find a xiiBlackboardComponent on pSearchObject or its parents with the given name and returns its blackboard.
  ///
  /// The blackboard name is only checked if the given name is not empty. If no matching blackboard component is found,
  /// the function will call xiiBlackboard::GetOrCreateGlobal() with the given name. Thus if you provide a name, you will always get a result, either from a component or from the global storage.
  ///
  /// \sa xiiBlackboard::GetOrCreateGlobal()
  static xiiSharedPtr<xiiBlackboard> FindBlackboard(xiiGameObject* pSearchObject, xiiStringView sBlackboardName = xiiStringView());

  /// Returns the blackboard owned by this component
  const xiiSharedPtr<xiiBlackboard>& GetBoard();
  xiiSharedPtr<const xiiBlackboard>  GetBoard() const;

  void SetShowDebugInfo(bool bShow); // [ property ]
  bool GetShowDebugInfo() const;     // [ property ]

  void       SetEntryValue(const char* szName, const xiiVariant& value); // [ scriptable ]
  xiiVariant GetEntryValue(const char* szName) const;                    // [ scriptable ]

protected:
  static xiiBlackboard* Reflection_FindBlackboard(xiiGameObject* pSearchObject, xiiStringView sBlackboardName);

  void OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& msg) const;
  void OnExtractRenderData(xiiMsgExtractRenderData& msg) const;

  xiiSharedPtr<xiiBlackboard> m_pBoard;

  xiiBlackboardTemplateResourceHandle m_hTemplate;
};

//////////////////////////////////////////////////////////////////////////

using xiiLocalBlackboardComponentManager = xiiComponentManager<class xiiLocalBlackboardComponent, xiiBlockStorageType::Compact>;

/// This component creates its own xiiBlackboard, and thus locally holds state.
class XII_GAMEENGINE_DLL xiiLocalBlackboardComponent : public xiiBlackboardComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiLocalBlackboardComponent, xiiBlackboardComponent, xiiLocalBlackboardComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

protected:
  virtual void Initialize() override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiBlackboardComponent

public:
  xiiLocalBlackboardComponent();
  xiiLocalBlackboardComponent(xiiLocalBlackboardComponent&& other);
  ~xiiLocalBlackboardComponent();

  xiiLocalBlackboardComponent& operator=(xiiLocalBlackboardComponent&& other);

  void SetSendEntryChangedMessage(bool bSend); // [ property ]
  bool GetSendEntryChangedMessage() const;     // [ property ]

  void          SetBlackboardName(xiiStringView sName); // [ property ]
  xiiStringView GetBlackboardName() const;              // [ property ]

private:
  xiiUInt32          Entries_GetCount() const;
  xiiBlackboardEntry Entries_GetValue(xiiUInt32 uiIndex) const;
  void               Entries_SetValue(xiiUInt32 uiIndex, xiiBlackboardEntry entry);
  void               Entries_Insert(xiiUInt32 uiIndex, xiiBlackboardEntry entry);
  void               Entries_Remove(xiiUInt32 uiIndex);

  void OnEntryChanged(const xiiBlackboard::EntryEvent& e);
  void InitializeFromTemplate();
  bool IsEditor() const;

  // this array is not held during runtime, it is only needed during editor time until the component is serialized out
  xiiDynamicArray<xiiBlackboardEntry> m_InitialEntries;

  xiiEventMessageSender<xiiMsgBlackboardEntryChanged> m_EntryChangedSender; // [ event ]
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

struct xiiGlobalBlackboardInitMode
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    EnsureEntriesExist,    ///< Brief only adds entries to the blackboard, that haven't been added before. Doesn't change the values of existing entries.
    ResetEntryValues,      ///< Overwrites values of existing entries, to reset them to the start value defined in the template.
    ClearEntireBlackboard, ///< Removes all entries from the blackboard and only adds the ones from the template. This also gets rid of temporary values.

    Default = ClearEntireBlackboard
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GAMEENGINE_DLL, xiiGlobalBlackboardInitMode);

using xiiGlobalBlackboardComponentManager = xiiComponentManager<class xiiGlobalBlackboardComponent, xiiBlockStorageType::Compact>;

/// This component references a global blackboard by name. If necessary, the blackboard will be created.
///
/// This allows to initialize a global blackboard with known values.
class XII_GAMEENGINE_DLL xiiGlobalBlackboardComponent : public xiiBlackboardComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiGlobalBlackboardComponent, xiiBlackboardComponent, xiiGlobalBlackboardComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

protected:
  virtual void Initialize() override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiGlobalBlackboardComponent

public:
  xiiGlobalBlackboardComponent();
  xiiGlobalBlackboardComponent(xiiGlobalBlackboardComponent&& other);
  ~xiiGlobalBlackboardComponent();

  xiiGlobalBlackboardComponent& operator=(xiiGlobalBlackboardComponent&& other);

  void        SetBlackboardName(const char* szName); // [ property ]
  const char* GetBlackboardName() const;             // [ property ]

  xiiEnum<xiiGlobalBlackboardInitMode> m_InitMode; // [ property ]

private:
  void InitializeFromTemplate();

  xiiHashedString m_sName;
};
