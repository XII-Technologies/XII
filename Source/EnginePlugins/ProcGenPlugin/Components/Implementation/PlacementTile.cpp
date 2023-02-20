#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/Prefabs/PrefabReferenceComponent.h>
#include <Core/World/World.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <ProcGenPlugin/Components/Implementation/PlacementTile.h>
#include <ProcGenPlugin/Tasks/PlacementData.h>

using namespace xiiProcGenInternal;

PlacementTile::PlacementTile() :
  m_pOutput(nullptr), m_State(State::Invalid)
{
}

PlacementTile::PlacementTile(PlacementTile&& other)
{
  m_Desc    = other.m_Desc;
  m_pOutput = other.m_pOutput;

  m_State       = other.m_State;
  other.m_State = State::Invalid;

  m_PlacedObjects = std::move(other.m_PlacedObjects);
}

PlacementTile::~PlacementTile()
{
  XII_ASSERT_DEV(m_State == State::Invalid, "Implementation error");
}

void PlacementTile::Initialize(const PlacementTileDesc& desc, xiiSharedPtr<const PlacementOutput>& pOutput)
{
  m_Desc    = desc;
  m_pOutput = pOutput;

  m_State = State::Initialized;
}

void PlacementTile::Deinitialize(xiiWorld& world)
{
  for (auto hObject : m_PlacedObjects)
  {
    world.DeleteObjectDelayed(hObject);
  }
  m_PlacedObjects.Clear();

  m_Desc.m_hComponent.Invalidate();
  m_pOutput = nullptr;
  m_State   = State::Invalid;
}

bool PlacementTile::IsValid() const
{
  return !m_Desc.m_hComponent.IsInvalidated() && m_pOutput != nullptr;
}

const PlacementTileDesc& PlacementTile::GetDesc() const
{
  return m_Desc;
}

const PlacementOutput* PlacementTile::GetOutput() const
{
  return m_pOutput;
}

xiiArrayPtr<const xiiGameObjectHandle> PlacementTile::GetPlacedObjects() const
{
  return m_PlacedObjects;
}

xiiBoundingBox PlacementTile::GetBoundingBox() const
{
  return m_Desc.GetBoundingBox();
}

xiiColor PlacementTile::GetDebugColor() const
{
  switch (m_State)
  {
    case State::Initialized:
      return xiiColor::Orange;
    case State::Scheduled:
      return xiiColor::Yellow;
    case State::Finished:
      return xiiColor::Green;
    default:
      return xiiColor::DarkRed;
  }
}

void PlacementTile::PreparePlacementData(const xiiWorld* pWorld, const xiiPhysicsWorldModuleInterface* pPhysicsModule, PlacementData& placementData)
{
  const xiiUInt64 uiOutputNameHash = m_pOutput->m_sName.GetHash();
  xiiUInt32       hashData[]       = {
    static_cast<xiiUInt32>(m_Desc.m_iPosX),
    static_cast<xiiUInt32>(m_Desc.m_iPosY),
    static_cast<xiiUInt32>(uiOutputNameHash),
    static_cast<xiiUInt32>(uiOutputNameHash >> 32),
  };

  placementData.m_pPhysicsModule             = pPhysicsModule;
  placementData.m_pWorld                     = pWorld;
  placementData.m_pOutput                    = m_pOutput;
  placementData.m_uiTileSeed                 = xiiHashingUtils::xxHash32(hashData, sizeof(hashData));
  placementData.m_TileBoundingBox            = GetBoundingBox();
  placementData.m_GlobalToLocalBoxTransforms = m_Desc.m_GlobalToLocalBoxTransforms;

  m_State = State::Scheduled;
}

xiiUInt32 PlacementTile::PlaceObjects(xiiWorld& world, xiiArrayPtr<const PlacementTransform> objectTransforms)
{
  XII_PROFILE_SCOPE("PlacementTile::PlaceObjects");

  xiiGameObjectDesc desc;
  auto&             objectsToPlace = m_pOutput->m_ObjectsToPlace;

  xiiHybridArray<xiiPrefabResource*, 4> prefabs;
  prefabs.SetCount(objectsToPlace.GetCount());



  for (auto& objectTransform : objectTransforms)
  {
    const xiiUInt32    uiObjectIndex = objectTransform.m_uiObjectIndex;
    xiiPrefabResource* pPrefab       = prefabs[uiObjectIndex];

    if (pPrefab == nullptr)
    {
      pPrefab                = xiiResourceManager::BeginAcquireResource(objectsToPlace[uiObjectIndex], xiiResourceAcquireMode::BlockTillLoaded);
      prefabs[uiObjectIndex] = pPrefab;
    }

    xiiTransform                      transform = xiiSimdConversion::ToTransform(objectTransform.m_Transform);
    xiiHybridArray<xiiGameObject*, 8> rootObjects;

    xiiPrefabInstantiationOptions options;
    options.m_pCreatedRootObjectsOut = &rootObjects;

    pPrefab->InstantiatePrefab(world, transform, options);

    // only send the color message, if we actually have a custom color
    if (objectTransform.m_bHasValidColor)
    {
      for (auto pRootObject : rootObjects)
      {
        // Set the color
        xiiMsgSetColor msg;
        msg.m_Color = objectTransform.m_ObjectColor.ToLinearFloat();
        pRootObject->PostMessageRecursive(msg, xiiTime::Zero(), xiiObjectMsgQueueType::AfterInitialized);
      }
    }

    for (auto pRootObject : rootObjects)
    {
      m_PlacedObjects.PushBack(pRootObject->GetHandle());
    }
  }

  for (auto pPrefab : prefabs)
  {
    if (pPrefab != nullptr)
    {
      xiiResourceManager::EndAcquireResource(pPrefab);
    }
  }

  m_State = State::Finished;

  return m_PlacedObjects.GetCount();
}
