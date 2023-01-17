#pragma once

#include <Foundation/Memory/BlockStorage.h>
#include <Foundation/Memory/LargeBlockAllocator.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/Id.h>

#include <Core/CoreDLL.h>

#ifndef XII_WORLD_INDEX_BITS
#  define XII_WORLD_INDEX_BITS 8
#endif

#define XII_MAX_WORLDS (1 << XII_WORLD_INDEX_BITS)

class xiiWorld;
class xiiSpatialSystem;

template <typename Type>
class xiiCoordinateSystemProviderTemplate;

using xiiCoordinateSystemProvider       = xiiCoordinateSystemProviderTemplate<xiiReal>;
using xiiCoordinateSystemProviderDouble = xiiCoordinateSystemProviderTemplate<double>;
using xiiCoordinateSystemProviderFloat  = xiiCoordinateSystemProviderTemplate<float>;

namespace xiiInternal
{
  class WorldData;

  enum
  {
    DEFAULT_BLOCK_SIZE = 1024 * 16
  };

  typedef xiiLargeBlockAllocator<DEFAULT_BLOCK_SIZE> WorldLargeBlockAllocator;
} // namespace xiiInternal

class xiiGameObject;
struct xiiGameObjectDesc;

class xiiComponentManagerBase;
class xiiComponent;

struct xiiMsgDeleteGameObject;

/// \brief Internal game object id used by xiiGameObjectHandle.
struct xiiGameObjectId
{
  typedef xiiUInt64 StorageType;

  XII_DECLARE_ID_TYPE(xiiGameObjectId, 32, 8);

  static_assert(XII_WORLD_INDEX_BITS > 0 && XII_WORLD_INDEX_BITS <= 24);

  XII_FORCE_INLINE xiiGameObjectId(StorageType instanceIndex, xiiUInt8 generation, xiiUInt8 worldIndex = 0)
  {
    m_Data          = 0;
    m_InstanceIndex = static_cast<xiiUInt32>(instanceIndex);
    m_Generation    = generation;
    m_WorldIndex    = worldIndex;
  }

  union
  {
    StorageType m_Data;
    struct
    {
      StorageType m_InstanceIndex : 32;
      StorageType m_Generation : 8;
      StorageType m_WorldIndex : XII_WORLD_INDEX_BITS;
    };
  };
};

/// \brief A handle to a game object.
///
/// Never store a direct pointer to a game object. Always store a handle instead. A pointer to a game object can
/// be received by calling xiiWorld::TryGetObject with the handle.
/// Note that the object might have been deleted so always check the return value of TryGetObject.
struct xiiGameObjectHandle
{
  XII_DECLARE_HANDLE_TYPE(xiiGameObjectHandle, xiiGameObjectId);

  friend class xiiWorld;
  friend class xiiGameObject;
};

/// \brief HashHelper implementation so game object handles can be used as key in a hash table.
template <>
struct xiiHashHelper<xiiGameObjectHandle>
{
  XII_ALWAYS_INLINE static xiiUInt32 Hash(xiiGameObjectHandle value) { return xiiHashHelper<xiiUInt64>::Hash(value.GetInternalID().m_Data); }

  XII_ALWAYS_INLINE static bool Equal(xiiGameObjectHandle a, xiiGameObjectHandle b) { return a == b; }
};

/// \brief Currently not implemented as it is not needed for game object handles.
XII_CORE_DLL void operator<<(xiiStreamWriter& Stream, const xiiGameObjectHandle& Value);
XII_CORE_DLL void operator>>(xiiStreamReader& Stream, xiiGameObjectHandle& Value);

XII_DECLARE_REFLECTABLE_TYPE(XII_CORE_DLL, xiiGameObjectHandle);
XII_DECLARE_CUSTOM_VARIANT_TYPE(xiiGameObjectHandle);
#define XII_COMPONENT_TYPE_INDEX_BITS (24 - XII_WORLD_INDEX_BITS)
#define XII_MAX_COMPONENT_TYPES       (1 << XII_COMPONENT_TYPE_INDEX_BITS)

/// \brief Internal component id used by xiiComponentHandle.
struct xiiComponentId
{
  typedef xiiUInt64 StorageType;

  XII_DECLARE_ID_TYPE(xiiComponentId, 32, 8);

  static_assert(XII_COMPONENT_TYPE_INDEX_BITS > 0 && XII_COMPONENT_TYPE_INDEX_BITS <= 16);

  XII_ALWAYS_INLINE xiiComponentId(StorageType instanceIndex, xiiUInt8 generation, xiiUInt16 typeId = 0, xiiUInt8 worldIndex = 0)
  {
    m_Data          = 0;
    m_InstanceIndex = static_cast<xiiUInt32>(instanceIndex);
    m_Generation    = generation;
    m_TypeId        = typeId;
    m_WorldIndex    = worldIndex;
  }

  union
  {
    StorageType m_Data;
    struct
    {
      StorageType m_InstanceIndex : 32;
      StorageType m_Generation : 8;
      StorageType m_WorldIndex : XII_WORLD_INDEX_BITS;
      StorageType m_TypeId : XII_COMPONENT_TYPE_INDEX_BITS;
    };
  };
};

/// \brief A handle to a component.
///
/// Never store a direct pointer to a component. Always store a handle instead. A pointer to a component can
/// be received by calling xiiWorld::TryGetComponent or TryGetComponent on the corresponding component manager.
/// Note that the component might have been deleted so always check the return value of TryGetComponent.
struct xiiComponentHandle
{
  XII_DECLARE_HANDLE_TYPE(xiiComponentHandle, xiiComponentId);

  friend class xiiWorld;
  friend class xiiComponentManagerBase;
  friend class xiiComponent;
};

/// \brief HashHelper implementation so component handles can be used as key in a hashtable.
template <>
struct xiiHashHelper<xiiComponentHandle>
{
  XII_ALWAYS_INLINE static xiiUInt32 Hash(xiiComponentHandle value)
  {
    xiiComponentId id   = value.GetInternalID();
    xiiUInt64      data = *reinterpret_cast<xiiUInt64*>(&id);
    return xiiHashHelper<xiiUInt64>::Hash(data);
  }

  XII_ALWAYS_INLINE static bool Equal(xiiComponentHandle a, xiiComponentHandle b) { return a == b; }
};

/// \brief Currently not implemented as it is not needed for component handles.
XII_CORE_DLL void operator<<(xiiStreamWriter& Stream, const xiiComponentHandle& Value);
XII_CORE_DLL void operator>>(xiiStreamReader& Stream, xiiComponentHandle& Value);

XII_DECLARE_REFLECTABLE_TYPE(XII_CORE_DLL, xiiComponentHandle);
XII_DECLARE_CUSTOM_VARIANT_TYPE(xiiComponentHandle);

/// \brief Internal flags of game objects or components.
struct xiiObjectFlags
{
  typedef xiiUInt32 StorageType;

  enum Enum
  {
    None         = 0,
    Dynamic      = XII_BIT(0),            ///< Usually detected automatically. A dynamic object will not cache render data across frames.
    ForceDynamic = XII_BIT(1),            ///< Set by the user to enforce the 'Dynamic' mode. Necessary when user code (or scripts) should change
                                          ///< objects, and the automatic detection cannot know that.
    ActiveFlag              = XII_BIT(2), ///< The object/component has the 'active flag' set
    ActiveState             = XII_BIT(3), ///< The object/component and all its parents have the active flag
    Initialized             = XII_BIT(4), ///< The object/component has been initialized
    Initializing            = XII_BIT(5), ///< The object/component is currently initializing. Used to prevent recursions during initialization.
    SimulationStarted       = XII_BIT(6), ///< OnSimulationStarted() has been called on the component
    SimulationStarting      = XII_BIT(7), ///< Used to prevent recursion during OnSimulationStarted()
    UnhandledMessageHandler = XII_BIT(8), ///< For components, when a message is not handled, a virtual function is called

    ChildChangesNotifications           = XII_BIT(9),  ///< The object should send a notification message when children are added or removed.
    ComponentChangesNotifications       = XII_BIT(10), ///< The object should send a notification message when components are added or removed.
    StaticTransformChangesNotifications = XII_BIT(11), ///< The object should send a notification message if it is static and its transform changes.
    ParentChangesNotifications          = XII_BIT(12), ///< The object should send a notification message when the parent is changes.

    UserFlag0 = XII_BIT(24),
    UserFlag1 = XII_BIT(25),
    UserFlag2 = XII_BIT(26),
    UserFlag3 = XII_BIT(27),
    UserFlag4 = XII_BIT(28),
    UserFlag5 = XII_BIT(29),
    UserFlag6 = XII_BIT(30),
    UserFlag7 = XII_BIT(31),

    Default = None
  };

  struct Bits
  {
    StorageType Dynamic : 1;                             //< 0
    StorageType ForceDynamic : 1;                        //< 1
    StorageType ActiveFlag : 1;                          //< 2
    StorageType ActiveState : 1;                         //< 3
    StorageType Initialized : 1;                         //< 4
    StorageType Initializing : 1;                        //< 5
    StorageType SimulationStarted : 1;                   //< 6
    StorageType SimulationStarting : 1;                  //< 7
    StorageType UnhandledMessageHandler : 1;             //< 8
    StorageType ChildChangesNotifications : 1;           //< 9
    StorageType ComponentChangesNotifications : 1;       //< 10
    StorageType StaticTransformChangesNotifications : 1; //< 11
    StorageType ParentChangesNotifications : 1;          //< 12

    StorageType Padding : 11; // 13 - 23

    StorageType UserFlag0 : 1; //< 24
    StorageType UserFlag1 : 1; //< 25
    StorageType UserFlag2 : 1; //< 26
    StorageType UserFlag3 : 1; //< 27
    StorageType UserFlag4 : 1; //< 28
    StorageType UserFlag5 : 1; //< 29
    StorageType UserFlag6 : 1; //< 30
    StorageType UserFlag7 : 1; //< 31
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiObjectFlags);

/// \brief Specifies the mode of an object. This enum is only used in the editor.
///
/// \sa xiiObjectFlags
struct xiiObjectMode
{
  typedef xiiUInt8 StorageType;

  enum Enum : xiiUInt8
  {
    Automatic,
    ForceDynamic,

    Default = Automatic
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_CORE_DLL, xiiObjectMode);

/// \brief Specifies the mode of a component. Dynamic components may change an object's transform, static components must not.
///
/// \sa xiiObjectFlags
struct xiiComponentMode
{
  enum Enum
  {
    Static,
    Dynamic
  };
};

/// \brief Specifies at which phase the queued message should be processed.
struct xiiObjectMsgQueueType
{
  enum Enum
  {
    PostAsync,        ///< Process the message in the PostAsync phase.
    PostTransform,    ///< Process the message in the PostTransform phase.
    NextFrame,        ///< Process the message in the PreAsync phase of the next frame.
    AfterInitialized, ///< Process the message after new components have been initialized.
    COUNT
  };
};

/// \brief Certain components may delete themselves or their owner when they are finished with their main purpose
struct XII_CORE_DLL xiiOnComponentFinishedAction
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None,
    DeleteComponent,
    DeleteGameObject,

    Default = None
  };

  /// \brief Call this when a component is 'finished' with its work.
  ///
  /// Pass in the desired action (usually configured by the user) and the 'this' pointer of the component.
  /// The helper function will delete this component and maybe also attempt to delete the entire object.
  /// For that it will coordinate with other components, and delay the object deletion, if necessary,
  /// until the last component has finished it's work.
  static void HandleFinishedAction(xiiComponent* pComponent, xiiOnComponentFinishedAction::Enum action);

  /// \brief Call this function in a message handler for xiiMsgDeleteGameObject messages.
  ///
  /// This is needed to coordinate object deletion across multiple components that use the
  /// xiiOnComponentFinishedAction mechanism.
  /// Depending on the state of this component, the function will either execute the object deletion,
  /// or delay it, until its own work is done.
  static void HandleDeleteObjectMsg(xiiMsgDeleteGameObject& msg, xiiEnum<xiiOnComponentFinishedAction>& action);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_CORE_DLL, xiiOnComponentFinishedAction);

/// \brief Same as xiiOnComponentFinishedAction, but additionally includes 'Restart'
struct XII_CORE_DLL xiiOnComponentFinishedAction2
{
  typedef xiiUInt8 StorageType;

  enum Enum
  {
    None,
    DeleteComponent,
    DeleteGameObject,
    Restart,

    Default = None
  };

  /// \brief See xiiOnComponentFinishedAction::HandleFinishedAction()
  static void HandleFinishedAction(xiiComponent* pComponent, xiiOnComponentFinishedAction2::Enum action);

  /// \brief See xiiOnComponentFinishedAction::HandleDeleteObjectMsg()
  static void HandleDeleteObjectMsg(xiiMsgDeleteGameObject& msg, xiiEnum<xiiOnComponentFinishedAction2>& action);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_CORE_DLL, xiiOnComponentFinishedAction2);

/// \brief Used as return value of visitor functions to define whether calling function should stop or continue visiting.
struct xiiVisitorExecution
{
  enum Enum
  {
    Continue, ///< Continue regular iteration
    Skip,     ///< In a depth-first iteration mode this will skip the entire sub-tree below the current object
    Stop      ///< Stop the entire iteration
  };
};

typedef xiiGenericId<24, 8> xiiSpatialDataId;
class xiiSpatialDataHandle
{
  XII_DECLARE_HANDLE_TYPE(xiiSpatialDataHandle, xiiSpatialDataId);
};

#define XII_MAX_WORLD_MODULE_TYPES XII_MAX_COMPONENT_TYPES
typedef xiiUInt16 xiiWorldModuleTypeId;
static_assert(xiiMath::MaxValue<xiiWorldModuleTypeId>() >= XII_MAX_WORLD_MODULE_TYPES - 1);

typedef xiiGenericId<24, 8> xiiComponentInitBatchId;
class xiiComponentInitBatchHandle
{
  XII_DECLARE_HANDLE_TYPE(xiiComponentInitBatchHandle, xiiComponentInitBatchId);
};
