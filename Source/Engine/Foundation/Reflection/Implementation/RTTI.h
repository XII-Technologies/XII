/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Reflection/Implementation/StaticRTTI.h>

// *****************************************
// ***** Runtime Type Information Data *****

struct xiiRTTIAllocator;
class xiiAbstractProperty;
class xiiAbstractFunctionProperty;
class xiiAbstractMessageHandler;
struct xiiMessageSenderInfo;
class xiiPropertyAttribute;
class xiiMessage;

using xiiMessageId = xiiUInt16;

/// This class holds information about reflected types. Each instance represents one type that is known to the reflection
/// system.
///
/// Instances of this class are typically created through the macros from the StaticRTTI.h header.
/// Each instance represents one type. This class holds information about derivation hierarchies and exposed properties. You can thus find
/// out whether a type is derived from some base class and what properties of which types are available. Properties can then be read and
/// modified on instances of this type.
class XII_FOUNDATION_DLL xiiRTTI
{
public:
  /// The constructor requires all the information about the type that this object represents.
  xiiRTTI(xiiStringView sName, const xiiRTTI* pParentType, xiiUInt32 uiTypeSize, xiiUInt32 uiTypeVersion, xiiUInt8 uiVariantType, xiiBitflags<xiiTypeFlags> flags, xiiRTTIAllocator* pAllocator, xiiArrayPtr<const xiiAbstractProperty*> properties, xiiArrayPtr<const xiiAbstractFunctionProperty*> functions, xiiArrayPtr<const xiiPropertyAttribute*> attributes, xiiArrayPtr<xiiAbstractMessageHandler*> messageHandlers, xiiArrayPtr<xiiMessageSenderInfo> messageSenders, const xiiRTTI* (*fnVerifyParent)());

  ~xiiRTTI();

  /// Can be called in debug builds to check that all reflected objects are correctly set up.
  void VerifyCorrectness() const;

  /// Calls VerifyCorrectness() on all xiiRTTI objects.
  static void VerifyCorrectnessForAllTypes();

  /// Returns the name of this type.
  XII_ALWAYS_INLINE xiiStringView GetTypeName() const { return m_sTypeName; } // [tested]

  /// Returns the hash of the name of this type.
  XII_ALWAYS_INLINE xiiUInt64 GetTypeNameHash() const { return m_uiTypeNameHash; } // [tested]

  /// Returns the type that is the base class of this type. May be nullptr if this type has no base class.
  XII_ALWAYS_INLINE const xiiRTTI* GetParentType() const { return m_pParentType; } // [tested]

  /// Returns the corresponding variant type for this type or Invalid if there is none.
  XII_ALWAYS_INLINE xiiVariantType::Enum GetVariantType() const { return static_cast<xiiVariantType::Enum>(m_uiVariantType); }

  /// Returns true if this type is derived from the given type (or of the same type).
  XII_ALWAYS_INLINE bool IsDerivedFrom(const xiiRTTI* pBaseType) const // [tested]
  {
    const xiiUInt32 thisGeneration = m_ParentHierarchy.GetCount();
    const xiiUInt32 baseGeneration = pBaseType->m_ParentHierarchy.GetCount();
    XII_ASSERT_DEBUG(thisGeneration > 0 && baseGeneration > 0, "SetupParentHierarchy() has not been called");
    return thisGeneration >= baseGeneration && m_ParentHierarchy.GetData()[thisGeneration - baseGeneration] == pBaseType;
  }

  /// Returns true if this type is derived from or identical to the given type.
  template <typename BASE>
  XII_ALWAYS_INLINE bool IsDerivedFrom() const // [tested]
  {
    return IsDerivedFrom(xiiGetStaticRTTI<BASE>());
  }

  /// Returns the object through which instances of this type can be allocated.
  XII_ALWAYS_INLINE xiiRTTIAllocator* GetAllocator() const { return m_pAllocator; } // [tested]

  /// Returns the array of properties that this type has. Does NOT include properties from base classes.
  XII_ALWAYS_INLINE xiiArrayPtr<const xiiAbstractProperty* const> GetProperties() const { return m_Properties; } // [tested]

  XII_ALWAYS_INLINE xiiArrayPtr<const xiiAbstractFunctionProperty* const> GetFunctions() const { return m_Functions; }

  XII_ALWAYS_INLINE xiiArrayPtr<const xiiPropertyAttribute* const> GetAttributes() const { return m_Attributes; }

  /// Returns the first attribute that derives from the given type, or nullptr if nothing is found.
  template <typename Type>
  const Type* GetAttributeByType() const;

  /// Returns the list of properties that this type has, including derived properties from all base classes.
  void GetAllProperties(xiiDynamicArray<const xiiAbstractProperty*>& out_properties) const; // [tested]

  /// Returns the size (in bytes) of an instance of this type.
  XII_ALWAYS_INLINE xiiUInt32 GetTypeSize() const { return m_uiTypeSize; } // [tested]

  /// Returns the version number of this type.
  XII_ALWAYS_INLINE xiiUInt32 GetTypeVersion() const { return m_uiTypeVersion; }

  /// Returns the type flags.
  XII_ALWAYS_INLINE const xiiBitflags<xiiTypeFlags>& GetTypeFlags() const { return m_TypeFlags; } // [tested]

  /// Searches all xiiRTTI instances for the one with the given name, or nullptr if no such type exists.
  static const xiiRTTI* FindTypeByName(xiiStringView sName); // [tested]

  /// Searches all xiiRTTI instances for the one with the given hashed name, or nullptr if no such type exists.
  static const xiiRTTI* FindTypeByNameHash(xiiUInt64 uiNameHash); // [tested]
  static const xiiRTTI* FindTypeByNameHash32(xiiUInt32 uiNameHash);

  using PredicateFunc = xiiDelegate<bool(const xiiRTTI*), 48>;

  /// Searches all xiiRTTI instances for one where the given predicate function returns true.
  static const xiiRTTI* FindTypeIf(PredicateFunc func);

  /// Will iterate over all properties of this type and (optionally) the base types to search for a property with the given name.
  const xiiAbstractProperty* FindPropertyByName(xiiStringView sName, bool bSearchBaseTypes = true) const; // [tested]

  /// Returns the name of the plugin which this type is declared in.
  XII_ALWAYS_INLINE xiiStringView GetPluginName() const { return m_sPluginName; } // [tested]

  /// Returns the array of message handlers that this type has.
  XII_ALWAYS_INLINE const xiiArrayPtr<xiiAbstractMessageHandler*>& GetMessageHandlers() const { return m_MessageHandlers; }

  /// Dispatches the given message to the proper message handler, if there is one available. Returns true if so, false if no message
  /// handler for this type exists.
  bool DispatchMessage(void* pInstance, xiiMessage& ref_msg) const;

  /// Dispatches the given message to the proper message handler, if there is one available. Returns true if so, false if no message
  /// handler for this type exists.
  bool DispatchMessage(const void* pInstance, xiiMessage& ref_msg) const;

  /// Returns whether this type can handle the given message type.
  template <typename MessageType>
  XII_ALWAYS_INLINE bool CanHandleMessage() const
  {
    return CanHandleMessage(MessageType::GetTypeMsgId());
  }

  /// Returns whether this type can handle the message type with the given id.
  inline bool CanHandleMessage(xiiMessageId id) const
  {
    XII_ASSERT_DEBUG(m_uiMsgIdOffset != xiiSmallInvalidIndex, "Message handler table should have been gathered at this point.\n"
                                                              "If this assert is triggered for a type loaded from a dynamic plugin,\n"
                                                              "you may have forgotten to instantiate a xiiPlugin object inside your plugin DLL.");

    const xiiUInt32 uiIndex = id - m_uiMsgIdOffset;
    return uiIndex < m_DynamicMessageHandlers.GetCount() && m_DynamicMessageHandlers.GetData()[uiIndex] != nullptr;
  }

  XII_ALWAYS_INLINE const xiiArrayPtr<xiiMessageSenderInfo>& GetMessageSender() const { return m_MessageSenders; }

  struct ForEachOptions
  {
    using StorageType = xiiUInt8;

    enum Enum : StorageType
    {
      None                  = 0,
      ExcludeNonAllocatable = XII_BIT(0), ///< Excludes all types that cannot be allocated through xiiRTTI. They may still be creatable through regular C++, though.
      ExcludeAbstract       = XII_BIT(1), ///< Excludes all types that are marked as 'abstract'. They may not be abstract in the C++ sense, though.
      ExcludeNotConcrete    = ExcludeNonAllocatable | ExcludeAbstract,

      Default = None
    };

    struct Bits
    {
      xiiUInt8 ExcludeNonAllocatable : 1;
      xiiUInt8 ExcludeAbstract : 1;
    };
  };

  using VisitorFunc = xiiDelegate<void(const xiiRTTI*), 48>;

  static void ForEachType(VisitorFunc func, xiiBitflags<ForEachOptions> options = ForEachOptions::Default); // [tested]

  static void ForEachDerivedType(const xiiRTTI* pBaseType, VisitorFunc func, xiiBitflags<ForEachOptions> options = ForEachOptions::Default);

  template <typename T>
  static XII_ALWAYS_INLINE void ForEachDerivedType(VisitorFunc func, xiiBitflags<ForEachOptions> options = ForEachOptions::Default)
  {
    ForEachDerivedType(xiiGetStaticRTTI<T>(), func, options);
  }

protected:
  xiiStringView                                         m_sPluginName;
  xiiStringView                                         m_sTypeName;
  xiiArrayPtr<const xiiAbstractProperty* const>         m_Properties;
  xiiArrayPtr<const xiiAbstractFunctionProperty* const> m_Functions;
  xiiArrayPtr<const xiiPropertyAttribute* const>        m_Attributes;
  void                                                  UpdateType(const xiiRTTI* pParentType, xiiUInt32 uiTypeSize, xiiUInt32 uiTypeVersion, xiiUInt8 uiVariantType, xiiBitflags<xiiTypeFlags> flags);
  void                                                  RegisterType();
  void                                                  UnregisterType();

  void GatherDynamicMessageHandlers();
  void SetupParentHierarchy();

  const xiiRTTI*    m_pParentType = nullptr;
  xiiRTTIAllocator* m_pAllocator  = nullptr;

  xiiUInt32                 m_uiTypeSize     = 0;
  xiiUInt32                 m_uiTypeVersion  = 0;
  xiiUInt64                 m_uiTypeNameHash = 0;
  xiiUInt32                 m_uiTypeIndex    = 0;
  xiiBitflags<xiiTypeFlags> m_TypeFlags;
  xiiUInt8                  m_uiVariantType = 0;
  xiiUInt16                 m_uiMsgIdOffset = xiiSmallInvalidIndex;

  const xiiRTTI* (*m_VerifyParent)();

  xiiArrayPtr<xiiAbstractMessageHandler*>                                 m_MessageHandlers;
  xiiSmallArray<xiiAbstractMessageHandler*, 1, xiiStaticAllocatorWrapper> m_DynamicMessageHandlers; // Do not track this data, it won't be deallocated before shutdown.

  xiiArrayPtr<xiiMessageSenderInfo>                           m_MessageSenders;
  xiiSmallArray<const xiiRTTI*, 7, xiiStaticAllocatorWrapper> m_ParentHierarchy;

private:
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, Reflection);

  /// Assigns the given plugin name to every xiiRTTI instance that has no plugin assigned yet.
  static void AssignPlugin(xiiStringView sPluginName);

  static void SanityCheckType(xiiRTTI* pType);

  /// Handles events by xiiPlugin, to figure out which types were provided by which plugin
  static void PluginEventHandler(const xiiPluginEvent& EventData);
};

XII_DECLARE_FLAGS_OPERATORS(xiiRTTI::ForEachOptions);

// ***********************************
// ***** Object Allocator Struct *****


/// The interface for an allocator that creates instances of reflected types.
struct XII_FOUNDATION_DLL xiiRTTIAllocator
{
  virtual ~xiiRTTIAllocator();

  /// Returns whether the type that is represented by this allocator, can be dynamically allocated at runtime.
  virtual bool CanAllocate() const { return true; } // [tested]

  /// Allocates one instance.
  template <typename T>
  xiiInternal::NewInstance<T> Allocate(xiiAllocator* pAllocator = nullptr)
  {
    return AllocateInternal(pAllocator).Cast<T>();
  }

  /// Clones the given instance.
  template <typename T>
  xiiInternal::NewInstance<T> Clone(const void* pObject, xiiAllocator* pAllocator = nullptr)
  {
    return CloneInternal(pObject, pAllocator).Cast<T>();
  }

  /// Deallocates the given instance.
  virtual void Deallocate(void* pObject, xiiAllocator* pAllocator = nullptr) = 0; // [tested]

private:
  virtual xiiInternal::NewInstance<void> AllocateInternal(xiiAllocator* pAllocator) = 0;
  virtual xiiInternal::NewInstance<void> CloneInternal(const void* pObject, xiiAllocator* pAllocator)
  {
    XII_IGNORE_UNUSED(pObject);
    XII_REPORT_FAILURE("Cloning is not supported by this allocator.");
    return xiiInternal::NewInstance<void>(nullptr, pAllocator);
  }
};

/// Dummy Allocator for types that should not be allocatable through the reflection system.
struct XII_FOUNDATION_DLL xiiRTTINoAllocator : public xiiRTTIAllocator
{
  /// Returns false, because this type of allocator is used for classes that shall not be allocated dynamically.
  virtual bool CanAllocate() const override { return false; } // [tested]

  /// Will trigger an assert.
  virtual xiiInternal::NewInstance<void> AllocateInternal(xiiAllocator* pAllocator) override // [tested]
  {
    XII_REPORT_FAILURE("This function should never be called.");
    return xiiInternal::NewInstance<void>(nullptr, pAllocator);
  }

  /// Will trigger an assert.
  virtual void Deallocate(void* pObject, xiiAllocator* pAllocator) override // [tested]
  {
    XII_IGNORE_UNUSED(pObject);
    XII_IGNORE_UNUSED(pAllocator);
    XII_REPORT_FAILURE("This function should never be called.");
  }
};

/// Default implementation of xiiRTTIAllocator that allocates instances via the given allocator.
template <typename CLASS, typename AllocatorWrapper = xiiDefaultAllocatorWrapper>
struct xiiRTTIDefaultAllocator : public xiiRTTIAllocator
{
  /// Returns a new instance that was allocated with the given allocator.
  virtual xiiInternal::NewInstance<void> AllocateInternal(xiiAllocator* pAllocator) override // [tested]
  {
    if (pAllocator == nullptr)
    {
      pAllocator = AllocatorWrapper::GetAllocator();
    }

    return XII_NEW(pAllocator, CLASS);
  }

  /// Clones the given instance with the given allocator.
  virtual xiiInternal::NewInstance<void> CloneInternal(const void* pObject, xiiAllocator* pAllocator) override // [tested]
  {
    if (pAllocator == nullptr)
    {
      pAllocator = AllocatorWrapper::GetAllocator();
    }

    if constexpr (std::is_copy_constructible_v<CLASS>)
    {
      return XII_NEW(pAllocator, CLASS, *static_cast<const CLASS*>(pObject));
    }
    else
    {
      XII_REPORT_FAILURE("Clone failed since the type is not copy constructible");
      return xiiInternal::NewInstance<void>(nullptr, pAllocator);
    }
  }

  /// Deletes the given instance with the given allocator.
  virtual void Deallocate(void* pObject, xiiAllocator* pAllocator) override // [tested]
  {
    if (pAllocator == nullptr)
    {
      pAllocator = AllocatorWrapper::GetAllocator();
    }

    CLASS* pPointer = static_cast<CLASS*>(pObject);
    XII_DELETE(pAllocator, pPointer);
  }
};
