#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>
#include <GraphicsCore/Pipeline/Extractor.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiExtractor, 1, xiiRTTINoAllocator)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)),
      XII_ACCESSOR_PROPERTY("Name", GetName, SetName),
    }
    XII_END_PROPERTIES;
    XII_BEGIN_ATTRIBUTES
    {
      new xiiColorAttribute(xiiColorScheme::DarkUI(xiiColorScheme::Red)),
      new xiiCategoryAttribute("Extractors")
    }
    XII_END_ATTRIBUTES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiExtractor::xiiExtractor(xiiStringView sName)
{
  m_bActive = true;
  m_sName.Assign(sName);

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  m_uiNumCachedRenderData   = 0;
  m_uiNumUncachedRenderData = 0;
#endif
}

xiiExtractor::~xiiExtractor() = default;

void xiiExtractor::SetName(xiiStringView sName)
{
  if (!sName.IsEmpty())
  {
    m_sName.Assign(sName);
  }
}

xiiStringView xiiExtractor::GetName() const
{
  return m_sName.GetView();
}

bool xiiExtractor::FilterByViewTags(const xiiView& view, const xiiGameObject* pObject) const
{
  if (!view.m_ExcludeTags.IsEmpty() && view.m_ExcludeTags.IsAnySet(pObject->GetTags()))
    return true;

  if (!view.m_IncludeTags.IsEmpty() && !view.m_IncludeTags.IsAnySet(pObject->GetTags()))
    return true;

  return false;
}

void xiiExtractor::ExtractRenderData(const xiiView& view, const xiiGameObject* pObject, xiiMsgExtractRenderData& msg, xiiExtractedRenderData& extractedRenderData) const
{
  auto AddRenderDataFromMessage = [&](const xiiMsgExtractRenderData& msg) {
    if (msg.m_OverrideCategory != xiiInvalidRenderDataCategory)
    {
      for (auto& data : msg.m_ExtractedRenderData)
      {
        extractedRenderData.AddRenderData(data.m_pRenderData, msg.m_OverrideCategory);
      }
    }
    else
    {
      for (auto& data : msg.m_ExtractedRenderData)
      {
        extractedRenderData.AddRenderData(data.m_pRenderData, data.m_Category);
      }
    }

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
    m_uiNumUncachedRenderData += msg.m_ExtractedRenderData.GetCount();
#endif
  };

  if (pObject->IsStatic())
  {
    xiiUInt16 uiComponentVersion = pObject->GetComponentVersion();

    auto cachedRenderData = xiiRenderWorld::GetCachedRenderData(view, pObject->GetHandle(), uiComponentVersion);

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
    for (xiiUInt32 i = 1; i < cachedRenderData.GetCount(); ++i)
    {
      XII_ASSERT_DEBUG(cachedRenderData[i - 1].m_uiComponentIndex <= cachedRenderData[i].m_uiComponentIndex, "Cached render data needs to be sorted");
      if (cachedRenderData[i - 1].m_uiComponentIndex == cachedRenderData[i].m_uiComponentIndex)
      {
        XII_ASSERT_DEBUG(cachedRenderData[i - 1].m_uiPartIndex < cachedRenderData[i].m_uiPartIndex, "Cached render data needs to be sorted");
      }
    }
#endif

    xiiUInt32 uiCacheIndex = 0;

    auto            components      = pObject->GetComponents();
    const xiiUInt32 uiNumComponents = components.GetCount();
    for (xiiUInt32 uiComponentIndex = 0; uiComponentIndex < uiNumComponents; ++uiComponentIndex)
    {
      bool bCacheFound = false;
      while (uiCacheIndex < cachedRenderData.GetCount() && cachedRenderData[uiCacheIndex].m_uiComponentIndex == uiComponentIndex)
      {
        const xiiInternal::RenderDataCacheEntry& cacheEntry = cachedRenderData[uiCacheIndex];
        if (cacheEntry.m_pRenderData != nullptr)
        {
          extractedRenderData.AddRenderData(cacheEntry.m_pRenderData, msg.m_OverrideCategory != xiiInvalidRenderDataCategory ? msg.m_OverrideCategory : xiiRenderData::Category(cacheEntry.m_uiCategory));

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
          ++m_uiNumCachedRenderData;
#endif
        }
        ++uiCacheIndex;

        bCacheFound = true;
      }

      if (bCacheFound)
      {
        continue;
      }

      const xiiComponent* pComponent = components[uiComponentIndex];

      msg.m_ExtractedRenderData.Clear();
      msg.m_uiNumCacheIfStatic = 0;

      if (pComponent->SendMessage(msg))
      {
        // Only cache render data if all parts should be cached otherwise the cache is incomplete and we won't call SendMessage again
        if (msg.m_uiNumCacheIfStatic > 0 && msg.m_ExtractedRenderData.GetCount() == msg.m_uiNumCacheIfStatic)
        {
          xiiHybridArray<xiiInternal::RenderDataCacheEntry, 16> newCacheEntries(xiiFrameAllocator::GetCurrentAllocator());

          for (xiiUInt32 uiPartIndex = 0; uiPartIndex < msg.m_ExtractedRenderData.GetCount(); ++uiPartIndex)
          {
            auto& newCacheEntry              = newCacheEntries.ExpandAndGetRef();
            newCacheEntry.m_pRenderData      = msg.m_ExtractedRenderData[uiPartIndex].m_pRenderData;
            newCacheEntry.m_uiCategory       = msg.m_ExtractedRenderData[uiPartIndex].m_Category.m_uiValue;
            newCacheEntry.m_uiComponentIndex = static_cast<xiiUInt16>(uiComponentIndex);
            newCacheEntry.m_uiPartIndex      = static_cast<xiiUInt16>(uiPartIndex);
          }

          xiiRenderWorld::CacheRenderData(view, pObject->GetHandle(), pComponent->GetHandle(), uiComponentVersion, newCacheEntries);
        }

        AddRenderDataFromMessage(msg);
      }
      else if (pComponent->IsActiveAndInitialized()) // component does not handle extract message at all
      {
        XII_ASSERT_DEV(pComponent->GetDynamicRTTI()->CanHandleMessage<xiiMsgExtractRenderData>() == false, "");

        // Create a dummy cache entry so we don't call send message next time
        xiiInternal::RenderDataCacheEntry dummyEntry;
        dummyEntry.m_pRenderData      = nullptr;
        dummyEntry.m_uiCategory       = xiiInvalidRenderDataCategory.m_uiValue;
        dummyEntry.m_uiComponentIndex = static_cast<xiiUInt16>(uiComponentIndex);

        xiiRenderWorld::CacheRenderData(view, pObject->GetHandle(), pComponent->GetHandle(), uiComponentVersion, xiiMakeArrayPtr(&dummyEntry, 1));
      }
    }
  }
  else
  {
    msg.m_ExtractedRenderData.Clear();
    pObject->SendMessage(msg);

    AddRenderDataFromMessage(msg);
  }
}

xiiResult xiiExtractor::Serialize(xiiStreamWriter& inout_stream) const
{
  inout_stream << m_bActive;
  inout_stream << m_sName;
  return XII_SUCCESS;
}

xiiResult xiiExtractor::Deserialize(xiiStreamReader& inout_stream)
{
  const xiiUInt32 uiVersion = xiiTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  XII_ASSERT_DEBUG(uiVersion == 1, "Unknown version encountered");

  inout_stream >> m_bActive;
  inout_stream >> m_sName;
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_Extractor);
