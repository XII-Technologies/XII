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

xiiCollectionComponent::xiiCollectionComponent()  = default;
xiiCollectionComponent::~xiiCollectionComponent() = default;

void xiiCollectionComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_hCollection;
}

void xiiCollectionComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_hCollection;
}

void xiiCollectionComponent::SetCollectionFile(const char* szFile)
{
  xiiCollectionResourceHandle hResource;

  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = xiiResourceManager::LoadResource<xiiCollectionResource>(szFile);
    xiiResourceManager::PreloadResource(hResource);
  }

  SetCollection(hResource);
}

const char* xiiCollectionComponent::GetCollectionFile() const
{
  if (!m_hCollection.IsValid())
    return "";

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
    }
  }
}

XII_STATICLINK_FILE(Core, Core_Collection_Implementation_CollectionComponent);
