/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/Variant.h>

class xiiStreamReader;
class xiiStreamWriter;

/// Flags for entries in xiiBlackboard.
struct XII_CORE_DLL xiiBlackboardEntryFlags
{
  using StorageType = xiiUInt16;

  enum Enum
  {
    None          = 0,
    Save          = XII_BIT(0), ///< Include the entry during serialization
    OnChangeEvent = XII_BIT(1), ///< Broadcast the 'ValueChanged' event when this entry's value is modified

    UserFlag0 = XII_BIT(7),
    UserFlag1 = XII_BIT(8),
    UserFlag2 = XII_BIT(9),
    UserFlag3 = XII_BIT(10),
    UserFlag4 = XII_BIT(11),
    UserFlag5 = XII_BIT(12),
    UserFlag6 = XII_BIT(13),
    UserFlag7 = XII_BIT(14),

    Invalid = XII_BIT(15),

    Default = None
  };

  struct Bits
  {
    StorageType Save : 1;
    StorageType OnChangeEvent : 1;
    StorageType Reserved : 5;
    StorageType UserFlag0 : 1;
    StorageType UserFlag1 : 1;
    StorageType UserFlag2 : 1;
    StorageType UserFlag3 : 1;
    StorageType UserFlag4 : 1;
    StorageType UserFlag5 : 1;
    StorageType UserFlag6 : 1;
    StorageType UserFlag7 : 1;
    StorageType Invalid : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiBlackboardEntryFlags);
XII_DECLARE_REFLECTABLE_TYPE(XII_CORE_DLL, xiiBlackboardEntryFlags);


/// A blackboard is a key/value store that provides OnChange events to be informed when a value changes.
///
/// Blackboards are used to gather typically small pieces of data. Some systems write the data, other systems read it.
/// Through the blackboard, arbitrary systems can interact.
///
/// For example this is commonly used in game AI, where some system gathers interesting pieces of data about the environment,
/// and then NPCs might use that information to make decisions.
class XII_CORE_DLL xiiBlackboard : public xiiRefCounted
{
private:
  xiiBlackboard(bool bIsGlobal);

public:
  ~xiiBlackboard();

  bool IsGlobalBlackboard() const { return m_bIsGlobal; }

  /// Factory method to create a new blackboard.
  ///
  /// Since blackboards use shared ownership we need to make sure that blackboards are created in xiiCore.dll.
  /// Some compilers (MSVC) create local v-tables which can become stale if a blackboard was registered as global but the DLL
  /// which created the blackboard is already unloaded.
  ///
  /// See https://groups.google.com/g/microsoft.public.vc.language/c/atSh_2VSc2w/m/EgJ3r_7OzVUJ?pli=1
  static xiiSharedPtr<xiiBlackboard> Create(xiiAllocator* pAllocator = xiiFoundation::GetDefaultAllocator());

  /// Factory method to get access to a globally registered blackboard.
  ///
  /// If a blackboard with that name was already created globally before, its reference is returned.
  /// Otherwise it will be created and permanently registered under that name.
  /// Global blackboards cannot be removed. Although you can change their name via "SetName()",
  /// the name under which they are registered globally will not change.
  ///
  /// If at some point you want to "remove" a global blackboard, instead call UnregisterAllEntries() to
  /// clear all its values.
  static xiiSharedPtr<xiiBlackboard> GetOrCreateGlobal(const xiiHashedString& sBlackboardName, xiiAllocator* pAllocator = xiiFoundation::GetDefaultAllocator());

  /// Finds a global blackboard with the given name.
  static xiiSharedPtr<xiiBlackboard> FindGlobal(const xiiTempHashedString& sBlackboardName);

  /// Changes the name of the blackboard.
  ///
  /// \note For global blackboards this has no effect under which name they are found. A global blackboard continues to
  /// be found by the name under which it was originally registered.
  void                   SetName(xiiStringView sName);
  xiiStringView          GetName() const { return m_sName; }
  const xiiHashedString& GetNameHashed() const { return m_sName; }

  struct Entry
  {
    xiiVariant                           m_Value;
    xiiBitflags<xiiBlackboardEntryFlags> m_Flags;
    xiiUInt8                             m_uiEditorIndex = 0xFFU;

    /// The change counter is increased every time the entry's value changes.
    /// Read this and compare it to a previous known value, to detect whether the value was changed since the last check.
    xiiUInt32 m_uiChangeCounter = 0;
  };

  struct EntryEvent
  {
    xiiHashedString m_sName;
    xiiVariant      m_OldValue;
    const Entry*    m_pEntry;
  };

  /// Removes the named entry. Does nothing, if no such entry exists.
  void RemoveEntry(const xiiHashedString& sName);

  ///  Removes all entries.
  void RemoveAllEntries();

  /// Returns whether an entry with the given name already exists.
  bool HasEntry(const xiiTempHashedString& sName) const;

  /// Sets the value of the named entry. If the entry doesn't exist, yet, it will be created with default flags.
  ///
  /// If the 'OnChangeEvent' flag is set for this entry, OnEntryEvent() will be broadcast.
  /// However, if the new value is no different to the old, no event will be broadcast.
  ///
  /// For new entries, no OnEntryEvent() is sent.
  ///
  /// For best efficiency, cache the entry name in a xiiHashedString and use the other overload of this function.
  /// DO NOT RECREATE the xiiHashedString every time, though.
  void SetEntryValue(xiiStringView sName, const xiiVariant& value);

  /// Overload of SetEntryValue() that takes a xiiHashedString rather than a xiiStringView.
  ///
  /// Using this function is more efficient, if you access the blackboard often, but you must ensure
  /// to only create the xiiHashedString once and cache it for reuse.
  /// Assigning a value to a xiiHashedString is an expensive operation, so if you do not cache the string,
  /// prefer to use the other overload.
  void SetEntryValue(const xiiHashedString& sName, const xiiVariant& value);

  /// Returns a pointer to the named entry, or nullptr if no such entry was registered.
  const Entry* GetEntry(const xiiTempHashedString& sName) const;

  /// Returns the flags of the named entry, or xiiBlackboardEntryFlags::Invalid, if no such entry was registered.
  xiiBitflags<xiiBlackboardEntryFlags> GetEntryFlags(const xiiTempHashedString& sName) const;

  /// Sets the flags of an existing entry. Returns XII_FAILURE, if it wasn't created via SetEntryValue() or SetEntryValue() before.
  xiiResult SetEntryFlags(const xiiTempHashedString& sName, xiiBitflags<xiiBlackboardEntryFlags> flags);

  /// Returns the value of the named entry, or the fallback xiiVariant, if no such entry was registered.
  xiiVariant GetEntryValue(const xiiTempHashedString& sName, const xiiVariant& fallback = xiiVariant()) const;

  /// For the editor to know what index an element had, so that it can pass through exposed properties (which are given by index).
  xiiResult SetEditorIndex(const xiiTempHashedString& sName, xiiUInt8 uiEditorIndex);

  /// Searches for the first item that has the previously set index. Returns an empty string, if none was found.
  xiiHashedString FindNameForEditorIndex(xiiUInt8 uiEditorIndex) const;

  /// Increments the value of the named entry. Returns the incremented value or an invalid variant if the entry does not exist or is not a number type.
  xiiVariant IncrementEntryValue(const xiiTempHashedString& sName);

  /// Decrements the value of the named entry. Returns the decremented value or an invalid variant if the entry does not exist or is not a number type.
  xiiVariant DecrementEntryValue(const xiiTempHashedString& sName);

  /// Grants read access to the entire map of entries.
  const xiiHashTable<xiiHashedString, Entry>& GetAllEntries() const { return m_Entries; }

  /// Allows you to register to the OnEntryEvent. This is broadcast whenever an entry is modified that has the flag xiiBlackboardEntryFlags::OnChangeEvent.
  const xiiEvent<const EntryEvent&>& OnEntryEvent() const { return m_EntryEvents; }

  /// This counter is increased every time an entry is added or removed (but not when it is modified).
  ///
  /// Comparing this value to a previous known value allows to quickly detect whether the set of entries has changed.
  xiiUInt32 GetBlackboardChangeCounter() const { return m_uiBlackboardChangeCounter; }

  /// This counter is increased every time any entry's value is modified.
  ///
  /// Comparing this value to a previous known value allows to quickly detect whether any entry has changed recently.
  xiiUInt32 GetBlackboardEntryChangeCounter() const { return m_uiBlackboardEntryChangeCounter; }

  /// Stores all entries that have the 'Save' flag in the stream.
  xiiResult Serialize(xiiStreamWriter& ref_stream) const;

  /// Restores entries from the stream.
  ///
  /// If the blackboard already contains entries, the deserialized data is ADDED to the blackboard.
  /// If deserialized entries overlap with existing ones, the deserialized entries will overwrite the existing ones (both values and flags).
  xiiResult Deserialize(xiiStreamReader& ref_stream);

private:
  XII_ALLOW_PRIVATE_PROPERTIES(xiiBlackboard);

  static xiiBlackboard* Reflection_GetOrCreateGlobal(const xiiHashedString& sName);
  static xiiBlackboard* Reflection_FindGlobal(xiiTempHashedString sName);
  void                  Reflection_SetEntryValue(xiiStringView sName, const xiiVariant& value);

  void ImplSetEntryValue(const xiiHashedString& sName, Entry& entry, const xiiVariant& value);

  bool                                 m_bIsGlobal = false;
  xiiHashedString                      m_sName;
  xiiEvent<const EntryEvent&>          m_EntryEvents;
  xiiUInt32                            m_uiBlackboardChangeCounter      = 0;
  xiiUInt32                            m_uiBlackboardEntryChangeCounter = 0;
  xiiHashTable<xiiHashedString, Entry> m_Entries;

  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(Core, Blackboard);
  static xiiMutex                                                   s_GlobalBlackboardsMutex;
  static xiiHashTable<xiiHashedString, xiiSharedPtr<xiiBlackboard>> s_GlobalBlackboards;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_CORE_DLL, xiiBlackboard);

//////////////////////////////////////////////////////////////////////////

struct XII_CORE_DLL xiiBlackboardCondition
{
  xiiHashedString                m_sEntryName;
  double                         m_fComparisonValue = 0.0;
  xiiEnum<xiiComparisonOperator> m_Operator;

  bool IsConditionMet(const xiiBlackboard& blackboard) const;

  xiiResult Serialize(xiiStreamWriter& ref_stream) const;
  xiiResult Deserialize(xiiStreamReader& ref_stream);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_CORE_DLL, xiiBlackboardCondition);
