#include <Core/CorePCH.h>

#include <Core/Prefabs/PrefabReferenceComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiPrefabReferenceComponent, 4, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Prefab", GetPrefabFile, SetPrefabFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Prefab")),
    XII_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new xiiExposedParametersAttribute("Prefab")),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("General"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

enum PrefabComponentFlags
{
  SelfDeletion = 1
};

xiiPrefabReferenceComponent::xiiPrefabReferenceComponent()  = default;
xiiPrefabReferenceComponent::~xiiPrefabReferenceComponent() = default;

void xiiPrefabReferenceComponent::SerializePrefabParameters(const xiiWorld& world, xiiWorldWriter& ref_stream, xiiArrayMap<xiiHashedString, xiiVariant> parameters)
{
  // we need a copy of the parameters here, therefore we don't take it by reference

  auto&           s         = ref_stream.GetStream();
  const xiiUInt32 numParams = parameters.GetCount();

  xiiHybridArray<xiiGameObjectHandle, 8> GoReferences;

  // Version 4
  {
    // to support game object references as exposed parameters (which are currently exposed as strings)
    // we need to remap the string from an 'editor uuid' to something that can be interpreted as a proper xiiGameObjectHandle at runtime

    // so first we get the resolver and try to map any string parameter to a valid xiiGameObjectHandle
    auto resolver = world.GetGameObjectReferenceResolver();

    if (resolver.IsValid())
    {
      xiiStringBuilder tmp;

      for (xiiUInt32 i = 0; i < numParams; ++i)
      {
        // if this is a string parameter
        xiiVariant& var = parameters.GetValue(i);
        if (var.IsA<xiiString>())
        {
          // and the resolver CAN map this string to a game object handle
          xiiGameObjectHandle hObject = resolver(var.Get<xiiString>().GetData(), xiiComponentHandle(), nullptr);
          if (!hObject.IsInvalidated())
          {
            // write the handle properly to file (this enables correct remapping during deserialization)
            // and discard the string's value, and instead write a string that specifies the index of the serialized handle to use

            // local game object reference - index into GoReferences
            tmp.Format("#!LGOR-{}", GoReferences.GetCount());
            var = tmp.GetData();

            GoReferences.PushBack(hObject);
          }
        }
      }
    }

    // now write all the xiiGameObjectHandle's such that during deserialization the xiiWorldReader will remap it as needed
    const xiiUInt8 numRefs = static_cast<xiiUInt8>(GoReferences.GetCount());
    s << numRefs;

    for (xiiUInt8 i = 0; i < numRefs; ++i)
    {
      ref_stream.WriteGameObjectHandle(GoReferences[i]);
    }
  }

  // Version 2
  s << numParams;
  for (xiiUInt32 i = 0; i < numParams; ++i)
  {
    s << parameters.GetKey(i);
    s << parameters.GetValue(i); // this may contain modified strings now, to map the game object handle references
  }
}

void xiiPrefabReferenceComponent::DeserializePrefabParameters(xiiArrayMap<xiiHashedString, xiiVariant>& out_parameters, xiiWorldReader& ref_stream)
{
  out_parameters.Clear();

  // versioning of this stuff is tied to the version number of xiiPrefabReferenceComponent
  const xiiUInt32 uiVersion = ref_stream.GetComponentTypeVersion(xiiGetStaticRTTI<xiiPrefabReferenceComponent>());
  auto&           s         = ref_stream.GetStream();

  // temp array to hold (and remap) the serialized game object handles
  xiiHybridArray<xiiGameObjectHandle, 8> GoReferences;

  if (uiVersion >= 4)
  {
    xiiUInt8 numRefs = 0;
    s >> numRefs;
    GoReferences.SetCountUninitialized(numRefs);

    // just read them all, this will remap as necessary to the xiiWorldReader
    for (xiiUInt8 i = 0; i < numRefs; ++i)
    {
      GoReferences[i] = ref_stream.ReadGameObjectHandle();
    }
  }

  if (uiVersion >= 2)
  {
    xiiUInt32 numParams = 0;
    s >> numParams;

    out_parameters.Reserve(numParams);

    xiiHashedString  key;
    xiiVariant       value;
    xiiStringBuilder tmp;

    for (xiiUInt32 i = 0; i < numParams; ++i)
    {
      s >> key;
      s >> value;

      if (value.IsA<xiiString>())
      {
        // if we find a string parameter, check if it is a 'local game object reference'
        const xiiString& str = value.Get<xiiString>();
        if (str.StartsWith("#!LGOR-"))
        {
          // if so, extract the index into the GoReferences array
          xiiInt32 idx;
          if (xiiConversionUtils::StringToInt(str.GetData() + 7, idx).Succeeded())
          {
            // now we can lookup the remapped xiiGameObjectHandle from our array
            const xiiGameObjectHandle hObject = GoReferences[idx];

            // and stringify the handle into a 'global game object reference', ie. one that contains the internal integer data of the handle
            // a regular runtime world has a reference resolver that is capable to reverse this stringified format to a handle again
            // which will happen once 'InstantiatePrefab' passes the m_Parameters list to the newly created objects
            tmp.Format("#!GGOR-{}", hObject.GetInternalID().m_Data);

            // map local game object reference to global game object reference
            value = tmp.GetData();
          }
        }
      }

      out_parameters.Insert(key, value);
    }
  }
}

void xiiPrefabReferenceComponent::SerializeComponent(xiiWorldWriter& ref_stream) const
{
  SUPER::SerializeComponent(ref_stream);
  auto& s = ref_stream.GetStream();

  s << m_hPrefab;

  xiiPrefabReferenceComponent::SerializePrefabParameters(*GetWorld(), ref_stream, m_Parameters);
}

void xiiPrefabReferenceComponent::DeserializeComponent(xiiWorldReader& ref_stream)
{
  SUPER::DeserializeComponent(ref_stream);
  const xiiUInt32 uiVersion = ref_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto&           s         = ref_stream.GetStream();

  s >> m_hPrefab;

  if (uiVersion < 3)
  {
    bool bDummy;
    s >> bDummy;
  }

  xiiPrefabReferenceComponent::DeserializePrefabParameters(m_Parameters, ref_stream);
}

void xiiPrefabReferenceComponent::SetPrefabFile(xiiStringView sFile)
{
  xiiPrefabResourceHandle hResource;

  if (!sFile.IsEmpty())
  {
    hResource = xiiResourceManager::LoadResource<xiiPrefabResource>(sFile);
    xiiResourceManager::PreloadResource(hResource);
  }

  SetPrefab(hResource);
}

const char* xiiPrefabReferenceComponent::GetPrefabFile() const
{
  if (!m_hPrefab.IsValid())
    return "";

  return m_hPrefab.GetResourceID();
}

void xiiPrefabReferenceComponent::SetPrefab(const xiiPrefabResourceHandle& hPrefab)
{
  if (m_hPrefab == hPrefab)
    return;

  m_hPrefab = hPrefab;

  if (IsActiveAndInitialized())
  {
    // only add to update list, if not yet activated,
    // since OnActivate will do the instantiation anyway

    GetWorld()->GetComponentManager<xiiPrefabReferenceComponentManager>()->AddToUpdateList(this);
  }
}

void xiiPrefabReferenceComponent::InstantiatePrefab()
{
  // now instantiate the prefab
  if (m_hPrefab.IsValid())
  {
    xiiResourceLock<xiiPrefabResource> pResource(m_hPrefab, xiiResourceAcquireMode::AllowLoadingFallback);

    xiiTransform id;
    id.SetIdentity();

    xiiPrefabInstantiationOptions options;
    options.m_hParent                    = GetOwner()->GetHandle();
    options.m_ReplaceNamedRootWithParent = "<Prefab-Root>";
    options.m_pOverrideTeamID            = &GetOwner()->GetTeamID();

    // if this ID is valid, this prefab is instantiated at editor runtime
    // replicate the same ID across all instantiated sub components to get correct picking behavior
    if (GetUniqueID() != xiiInvalidIndex)
    {
      xiiHybridArray<xiiGameObject*, 8>  createdRootObjects;
      xiiHybridArray<xiiGameObject*, 16> createdChildObjects;

      options.m_pCreatedRootObjectsOut  = &createdRootObjects;
      options.m_pCreatedChildObjectsOut = &createdChildObjects;

      xiiUInt32 uiPrevComponentCount = GetOwner()->GetComponents().GetCount();

      pResource->InstantiatePrefab(*GetWorld(), id, options, &m_Parameters);

      auto FixComponent = [](xiiGameObject* pChild, xiiUInt32 uiUniqueID) {
        // while exporting a scene all game objects with this flag are ignored and not exported
        // set this flag on all game objects that were created by instantiating this prefab
        // instead it should be instantiated at runtime again
        // only do this at editor time though, at regular runtime we do want to fully serialize the entire sub tree
        pChild->SetCreatedByPrefab();

        for (auto pComponent : pChild->GetComponents())
        {
          pComponent->SetUniqueID(uiUniqueID);
          pComponent->SetCreatedByPrefab();
        }
      };

      const xiiUInt32 uiUniqueID = GetUniqueID();

      for (xiiGameObject* pChild : createdRootObjects)
      {
        FixComponent(pChild, uiUniqueID);
      }

      for (xiiGameObject* pChild : createdChildObjects)
      {
        FixComponent(pChild, uiUniqueID);
      }

      for (; uiPrevComponentCount < GetOwner()->GetComponents().GetCount(); ++uiPrevComponentCount)
      {
        GetOwner()->GetComponents()[uiPrevComponentCount]->SetUniqueID(GetUniqueID());
        GetOwner()->GetComponents()[uiPrevComponentCount]->SetCreatedByPrefab();
      }
    }
    else
    {
      pResource->InstantiatePrefab(*GetWorld(), id, options, &m_Parameters);
    }
  }
}

void xiiPrefabReferenceComponent::OnActivated()
{
  SUPER::OnActivated();

  // instantiate the prefab right away, such that game play code can access it as soon as possible
  // additionally the manager may update the instance later on, to properly enable editor work flows
  InstantiatePrefab();
}

void xiiPrefabReferenceComponent::OnDeactivated()
{
  // if this was created procedurally during editor runtime, we do not need to clear specific nodes
  // after simulation, the scene is deleted anyway

  ClearPreviousInstances();

  SUPER::OnDeactivated();
}

void xiiPrefabReferenceComponent::ClearPreviousInstances()
{
  if (GetUniqueID() != xiiInvalidIndex)
  {
    // if this is in the editor, and the 'activate' flag is toggled,
    // get rid of all our created child objects

    xiiArrayPtr<xiiComponent* const> components = GetOwner()->GetComponents();

    for (xiiUInt32 ip1 = components.GetCount(); ip1 > 0; --ip1)
    {
      const xiiUInt32 i = ip1 - 1;

      if (components[i] != this && components[i]->WasCreatedByPrefab())
      {
        components[i]->GetOwningManager()->DeleteComponent(components[i]);
      }
    }

    for (auto it = GetOwner()->GetChildren(); it.IsValid(); ++it)
    {
      if (it->WasCreatedByPrefab())
      {
        GetWorld()->DeleteObjectNow(it->GetHandle());
      }
    }
  }
}

void xiiPrefabReferenceComponent::Deinitialize()
{
  if (GetUserFlag(PrefabComponentFlags::SelfDeletion))
  {
    // do nothing, ie do not call OnDeactivated()
    // we do want to keep the created child objects around when this component gets destroyed during simulation
    // that's because the component actually deletes itself when simulation starts
    return;
  }

  // remove the children (through Deactivate)
  OnDeactivated();
}

void xiiPrefabReferenceComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (GetUniqueID() == xiiInvalidIndex)
  {
    SetUserFlag(PrefabComponentFlags::SelfDeletion, true);

    // remove the prefab reference component, to prevent issues after another serialization/deserialization
    // and also to save some memory
    DeleteComponent();
  }
}

const xiiRangeView<const char*, xiiUInt32> xiiPrefabReferenceComponent::GetParameters() const
{
  return xiiRangeView<const char*, xiiUInt32>([]() -> xiiUInt32 { return 0; },
                                              [this]() -> xiiUInt32 { return m_Parameters.GetCount(); },
                                              [](xiiUInt32& ref_uiIt) { ++ref_uiIt; },
                                              [this](const xiiUInt32& uiIt) -> const char* { return m_Parameters.GetKey(uiIt).GetString(); });
}

void xiiPrefabReferenceComponent::SetParameter(xiiStringView sKey, const xiiVariant& value)
{
  xiiHashedString hs;
  hs.Assign(sKey);

  auto it = m_Parameters.Find(hs);
  if (it != xiiInvalidIndex && m_Parameters.GetValue(it) == value)
    return;

  m_Parameters[hs] = value;

  if (IsActiveAndInitialized())
  {
    // only add to update list, if not yet activated,
    // since OnActivate will do the instantiation anyway
    GetWorld()->GetComponentManager<xiiPrefabReferenceComponentManager>()->AddToUpdateList(this);
  }
}

void xiiPrefabReferenceComponent::RemoveParameter(xiiStringView sKey)
{
  if (m_Parameters.RemoveAndCopy(xiiTempHashedString(sKey)))
  {
    if (IsActiveAndInitialized())
    {
      // only add to update list, if not yet activated,
      // since OnActivate will do the instantiation anyway
      GetWorld()->GetComponentManager<xiiPrefabReferenceComponentManager>()->AddToUpdateList(this);
    }
  }
}

bool xiiPrefabReferenceComponent::GetParameter(xiiStringView sKey, xiiVariant& out_value) const
{
  xiiUInt32 it = m_Parameters.Find(sKey);

  if (it == xiiInvalidIndex)
    return false;

  out_value = m_Parameters.GetValue(it);
  return true;
}

//////////////////////////////////////////////////////////////////////////

xiiPrefabReferenceComponentManager::xiiPrefabReferenceComponentManager(xiiWorld* pWorld) :
  xiiComponentManager<ComponentType, xiiBlockStorageType::Compact>(pWorld)
{
  xiiResourceManager::GetResourceEvents().AddEventHandler(xiiMakeDelegate(&xiiPrefabReferenceComponentManager::ResourceEventHandler, this));
}


xiiPrefabReferenceComponentManager::~xiiPrefabReferenceComponentManager()
{
  xiiResourceManager::GetResourceEvents().RemoveEventHandler(xiiMakeDelegate(&xiiPrefabReferenceComponentManager::ResourceEventHandler, this));
}

void xiiPrefabReferenceComponentManager::Initialize()
{
  auto desc = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiPrefabReferenceComponentManager::Update, this);

  RegisterUpdateFunction(desc);
}

void xiiPrefabReferenceComponentManager::ResourceEventHandler(const xiiResourceEvent& e)
{
  if (e.m_Type == xiiResourceEvent::Type::ResourceContentUnloading && e.m_pResource->GetDynamicRTTI()->IsDerivedFrom<xiiPrefabResource>())
  {
    xiiPrefabResourceHandle hPrefab((xiiPrefabResource*)(e.m_pResource));

    for (auto it = GetComponents(); it.IsValid(); it.Next())
    {
      if (it->m_hPrefab == hPrefab)
      {
        AddToUpdateList(it);
      }
    }
  }
}

void xiiPrefabReferenceComponentManager::Update(const xiiWorldModule::UpdateContext& context)
{
  for (auto hComp : m_ComponentsToUpdate)
  {
    xiiPrefabReferenceComponent* pComponent;
    if (!TryGetComponent(hComp, pComponent))
      continue;

    pComponent->m_bInUpdateList = false;
    if (!pComponent->IsActive())
      continue;

    pComponent->ClearPreviousInstances();
    pComponent->InstantiatePrefab();
  }

  m_ComponentsToUpdate.Clear();
}

void xiiPrefabReferenceComponentManager::AddToUpdateList(xiiPrefabReferenceComponent* pComponent)
{
  if (!pComponent->m_bInUpdateList)
  {
    m_ComponentsToUpdate.PushBack(pComponent->GetHandle());
    pComponent->m_bInUpdateList = true;
  }
}


XII_STATICLINK_FILE(Core, Core_Prefabs_Implementation_PrefabReferenceComponent);
