#include <Core/CorePCH.h>

#include <Core/Collection/CollectionComponent.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiCollectionComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Collection", GetCollectionFile, SetCollectionFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_AssetCollection", xiiDependencyFlags::Package)),
    XII_MEMBER_PROPERTY("RegisterNames", m_bRegisterNames),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Utilities"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiCollectionComponent::xiiCollectionComponent()  = default;
xiiCollectionComponent::~xiiCollectionComponent() = default;

void xiiCollectionComponent::SerializeComponent(xiiWorldWriter& ref_stream) const
{
  SUPER::SerializeComponent(ref_stream);
  auto& s = ref_stream.GetStream();

  s << m_hCollection;
  s << m_bRegisterNames;
}

void xiiCollectionComponent::DeserializeComponent(xiiWorldReader& ref_stream)
{
  SUPER::DeserializeComponent(ref_stream);
  const xiiUInt32 uiVersion = ref_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto&           s         = ref_stream.GetStream();
  
  XII_IGNORE_UNUSED(uiVersion);

  s >> m_hCollection;
  s >> m_bRegisterNames;
}

void xiiCollectionComponent::SetCollectionFile(xiiStringView sFile)
{
  xiiCollectionResourceHandle hResource;

  if (!sFile.IsEmpty())
  {
    hResource = xiiResourceManager::LoadResource<xiiCollectionResource>(sFile);
    xiiResourceManager::PreloadResource(hResource);
  }

  SetCollection(hResource);
}

xiiStringView xiiCollectionComponent::GetCollectionFile() const
{
  if (!m_hCollection.IsValid())
    return {};

  return m_hCollection.GetResourceID();
}

void xiiCollectionComponent::SetCollection(const xiiCollectionResourceHandle& hCollection)
{
  m_hCollection = hCollection;

  if (IsActiveAndSimulating())
  {
    InitiatePreload();
  }
}

void xiiCollectionComponent::OnSimulationStarted()
{
  InitiatePreload();
}

void xiiCollectionComponent::InitiatePreload()
{
  if (m_hCollection.IsValid())
  {
    xiiResourceLock<xiiCollectionResource> pCollection(m_hCollection, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);

    if (pCollection.GetAcquireResult() == xiiResourceAcquireResult::Final)
    {
      pCollection->PreloadResources();

      if (m_bRegisterNames)
      {
        pCollection->RegisterNames();
      }
    }
  }
}

XII_STATICLINK_FILE(Core, Core_Collection_Implementation_CollectionComponent);
