/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/World/World.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/UniquePtr.h>

class xiiStringDeduplicationReadContext;
class xiiProgress;
class xiiProgressRange;

struct xiiPrefabInstantiationOptions
{
  xiiGameObjectHandle m_hParent;

  xiiDynamicArray<xiiGameObject*>* m_pCreatedRootObjectsOut  = nullptr;
  xiiDynamicArray<xiiGameObject*>* m_pCreatedChildObjectsOut = nullptr;
  const xiiUInt16*                 m_pOverrideTeamID         = nullptr;

  bool m_bForceDynamic = false;

  /// If the prefab has a single root node with this non-empty name, rather than creating a new object, instead the m_hParent object is used.
  xiiTempHashedString m_ReplaceNamedRootWithParent;

  enum class RandomSeedMode
  {
    DeterministicFromParent, ///< xiiWorld::CreateObject() will either derive a deterministic value from the parent object, or assign a random value, if no parent exists.
    CompletelyRandom,        ///< xiiWorld::CreateObject() will assign a random value to this object.
    FixedFromSerialization,  ///< Keep deserialized random seed value.
    CustomRootValue,         ///< Use the given seed root value to assign a deterministic (but different) value to each game object.
  };

  RandomSeedMode m_RandomSeedMode              = RandomSeedMode::DeterministicFromParent;
  xiiUInt32      m_uiCustomRandomSeedRootValue = 0;

  xiiTime m_MaxStepTime = xiiTime::MakeZero();

  xiiProgress* m_pProgress = nullptr;
};

/// Reads a world description from a stream. Allows to instantiate that world multiple times
///        in different locations and different xiiWorld's.
///
/// The reader will ignore unknown component types and skip them during instantiation.
class XII_CORE_DLL xiiWorldReader
{
public:
  /// A context object is returned from InstantiateWorld or InstantiatePrefab if a maxStepTime greater than zero is specified.
  ///
  /// Call the Step() function periodically to complete the instantiation.
  /// Each step will try to spend not more than the given maxStepTime.
  /// E.g. this is useful if the instantiation cost of large prefabs needs to be distributed over multiple frames.
  class InstantiationContextBase
  {
  public:
    enum class StepResult
    {
      Continue,          ///< The available time slice is used up. Call Step() again to continue the process.
      ContinueNextFrame, ///< The process has reached a point where you need to call xiiWorld::Update(). Otherwise no further progress can be made.
      Finished,          ///< The instantiation is finished and you can delete the context. Don't call 'Step()' on it again.
    };

    virtual ~InstantiationContextBase() = default;

    /// \Brief Advance the instantiation by one step
    /// \return Whether the operation is finished or needs to be repeated.
    virtual StepResult Step() = 0;

    /// \Brief Cancel the instantiation. This might lead to inconsistent states and must be used with care.
    virtual void Cancel() = 0;
  };

  xiiWorldReader();
  ~xiiWorldReader();

  /// Reads all information about the world from the given stream.
  ///
  /// Call this once to populate xiiWorldReader with information how to instantiate the world.
  /// Afterwards \a stream can be deleted.
  /// Call InstantiateWorld() or InstantiatePrefab() afterwards as often as you like
  /// to actually get an objects into a xiiWorld.
  /// By default, the method will warn if it skips bytes in the stream that are of unknown
  /// types. The warnings can be suppressed by setting warningOnUnknownSkip to false.
  xiiResult ReadWorldDescription(xiiStreamReader& ref_stream, bool bWarningOnUnknownSkip = true);

  /// Creates one instance of the world that was previously read by ReadWorldDescription().
  ///
  /// This is identical to calling InstantiatePrefab() with identity values, however, it is a bit
  /// more efficient, as unnecessary computations are skipped.
  ///
  /// If pOverrideTeamID is not null, every instantiated game object will get it passed in as its new value.
  /// This can be used to identify that the object belongs to a specific player or team.
  ///
  /// If maxStepTime is not zero the function will return a valid ptr to an InstantiationContextBase.
  /// This context will only spend the given amount of time in its Step() function.
  /// The function has to be periodically called until it returns true to complete the instantiation.
  ///
  /// If pProgress is a valid pointer it is used to track the progress of the instantiation. The xiiProgress object
  /// has to be valid as long as the instantiation is in progress.
  xiiUniquePtr<InstantiationContextBase> InstantiateWorld(xiiWorld& ref_world, const xiiUInt16* pOverrideTeamID = nullptr, xiiTime maxStepTime = xiiTime::MakeZero(), xiiProgress* pProgress = nullptr);

  /// Creates one instance of the world that was previously read by ReadWorldDescription().
  ///
  /// \param rootTransform is an additional transform that is applied to all root objects.
  /// \param hParent allows to attach the newly created objects immediately to a parent
  /// \param out_CreatedRootObjects If this is valid, all pointers the to created root objects are stored in this array
  ///
  /// If pOverrideTeamID is not null, every instantiated game object will get it passed in as its new value.
  /// This can be used to identify that the object belongs to a specific player or team.
  ///
  /// If maxStepTime is not zero the function will return a valid ptr to an InstantiationContextBase.
  /// This context will only spend the given amount of time in its Step() function.
  /// The function has to be periodically called until it returns true to complete the instantiation.
  ///
  /// If pProgress is a valid pointer it is used to track the progress of the instantiation. The xiiProgress object
  /// has to be valid as long as the instantiation is in progress.
  xiiUniquePtr<InstantiationContextBase> InstantiatePrefab(xiiWorld& ref_world, const xiiTransform& rootTransform, const xiiPrefabInstantiationOptions& options);

  /// Gives access to the stream of data. Use this inside component deserialization functions to read data.
  xiiStreamReader& GetStream() const;

  /// Used during component deserialization to read a handle to a game object.
  xiiGameObjectHandle ReadGameObjectHandle();

  /// Used during component deserialization to read a handle to a component.
  void ReadComponentHandle(xiiComponentHandle& out_hComponent);

  /// Used during component deserialization to query the actual version number with which the
  /// given component type was written. The version number is given through the XII_BEGIN_COMPONENT_TYPE
  /// macro. Whenever the serialization of a component changes, that number should be increased.
  xiiUInt32 GetComponentTypeVersion(const xiiRTTI* pRtti) const;

  /// Returns whether world contains a component of given type.
  bool HasComponentOfType(const xiiRTTI* pRtti) const;

  /// Clears all data.
  void ClearAndCompact();

  /// Returns the amount of bytes that are currently allocated on the heap.
  xiiUInt64 GetHeapMemoryUsage() const;

  using FindComponentTypeCallback = xiiDelegate<const xiiRTTI*(xiiStringView sTypeName)>;

  /// An optional callback to redirect the lookup of a component type name to a xiiRTTI type.
  ///
  /// If specified, this is used by ALL world readers. The intention is to use this either for logging purposes,
  /// or to implement a whitelist or blacklist for specific component types.
  /// E.g. if the callback returns nullptr, the component type is 'unknown' and skipped by the world reader.
  /// Thus one can remove unwanted component types.
  /// Theoretically one could also redirect an old (or renamed) component type to a new one,
  /// given that their deserialization code is compatible.
  static FindComponentTypeCallback s_FindComponentTypeCallback;

  xiiUInt32 GetRootObjectCount() const;
  xiiUInt32 GetChildObjectCount() const;

  static void    SetMaxStepTime(InstantiationContextBase* pContext, xiiTime maxStepTime);
  static xiiTime GetMaxStepTime(InstantiationContextBase* pContext);

private:
  struct GameObjectToCreate
  {
    xiiGameObjectDescription m_Desc;
    xiiString                m_sGlobalKey;
    xiiUInt32                m_uiParentHandleIdx;
  };

  void ReadGameObjectDesc(GameObjectToCreate& godesc);
  void ReadComponentTypeInfo(xiiUInt32 uiComponentTypeIdx);
  void ReadComponentDataToMemStream(bool warningOnUnknownSkip = true);

  xiiUniquePtr<InstantiationContextBase> Instantiate(xiiWorld& world, bool bUseTransform, const xiiTransform& rootTransform, const xiiPrefabInstantiationOptions& options);

  xiiStreamReader* m_pReadStream = nullptr;
  xiiUInt8         m_uiVersion   = 0;

  xiiDynamicArray<GameObjectToCreate> m_RootObjectsToCreate;
  xiiDynamicArray<GameObjectToCreate> m_ChildObjectsToCreate;

  struct ComponentTypeInfo
  {
    const xiiRTTI* m_pRtti               = nullptr;
    xiiUInt32      m_uiNumComponents     = 0;
    xiiUInt32      m_uiComponentDataSize = 0;
  };

  xiiDynamicArray<ComponentTypeInfo>      m_ComponentTypes;
  xiiHashTable<const xiiRTTI*, xiiUInt32> m_ComponentTypeVersions;
  xiiDefaultMemoryStreamStorage           m_ComponentCreationStream;
  xiiDefaultMemoryStreamStorage           m_ComponentDataStream;
  xiiUInt64                               m_uiTotalNumComponents = 0;

  xiiUniquePtr<xiiStringDeduplicationReadContext> m_pStringDedupReadContext;

  class InstantiationContext : public InstantiationContextBase
  {
  public:
    InstantiationContext(xiiWorldReader& ref_worldReader, xiiWorld* pWorld, bool bUseTransform, const xiiTransform& rootTransform, const xiiPrefabInstantiationOptions& options, xiiAllocator* pAllocator);
    ~InstantiationContext();

    virtual StepResult Step() override;
    virtual void       Cancel() override;

    template <bool UseTransform>
    bool CreateGameObjects(const xiiDynamicArray<GameObjectToCreate>& objects, xiiGameObjectHandle hParent, xiiDynamicArray<xiiGameObject*>* out_pCreatedObjects, xiiTime endTime);

    bool CreateComponents(xiiTime endTime);
    bool DeserializeComponents(xiiTime endTime);
    bool AddComponentsToBatch(xiiTime endTime);

    void    SetMaxStepTime(xiiTime stepTime);
    xiiTime GetMaxStepTime() const;

  private:
    void BeginNextProgressStep(xiiStringView sName);
    void SetSubProgressCompletion(double fCompletion);

    friend class xiiWorldReader;
    xiiWorldReader& m_WorldReader;

    xiiWorld* m_pWorld = nullptr;

    bool         m_bUseTransform = false;
    xiiTransform m_RootTransform;

    xiiPrefabInstantiationOptions m_Options;

    struct ComponentTypeState
    {
      ComponentTypeState(xiiAllocator* pAllocator) :
        m_ComponentIndexToHandle(pAllocator)
      {
      }

      xiiUInt64                           m_uiDataReadOffset = 0;
      xiiDynamicArray<xiiComponentHandle> m_ComponentIndexToHandle;
    };

    xiiDynamicArray<xiiGameObjectHandle> m_IndexToGameObjectHandle;
    xiiDynamicArray<ComponentTypeState>  m_ComponentTypeStates;

    xiiComponentInitBatchHandle m_hComponentInitBatch;

    // Current state
    struct Phase
    {
      enum Enum
      {
        Invalid = -1,
        CreateRootObjects,
        CreateChildObjects,
        CreateComponents,
        DeserializeComponents,
        AddComponentsToBatch,
        InitComponents,

        Count
      };
    };

    Phase::Enum           m_Phase                           = Phase::Invalid;
    xiiUInt32             m_uiCurrentIndex                  = 0; // Object or Component.
    xiiUInt32             m_uiCurrentComponentTypeIndex     = 0;
    xiiUInt64             m_uiCurrentNumComponentsProcessed = 0;
    xiiMemoryStreamReader m_CurrentReader;

    xiiUniquePtr<xiiProgressRange> m_pOverallProgressRange;
    xiiUniquePtr<xiiProgressRange> m_pSubProgressRange;
  };
};
