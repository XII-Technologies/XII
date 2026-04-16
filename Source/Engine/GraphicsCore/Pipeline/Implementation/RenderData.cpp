#include <Foundation/Algorithm/Sorting.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Threading/Mutex.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/RenderData.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderData, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

static xiiMutex                       s_CategoryMutex;
static xiiDynamicArray<xiiStringView> s_CategoryNames;
static xiiDynamicArray<xiiString>     s_CategoryStringData;

// ----------------------------------------------------------------------------------------------------------------
// xiiRenderData

xiiRenderDataCategory xiiRenderData::RegisterCategory(xiiStringView sCategoryName)
{
  XII_LOCK(s_CategoryMutex);

  for (xiiUInt32 i = 0; i < s_CategoryNames.GetCount(); ++i)
  {
    if (s_CategoryNames[i].IsEqual_NoCase(sCategoryName))
    {
      return xiiRenderDataCategory{static_cast<xiiUInt16>(i)};
    }
  }

  // Create new category
  xiiUInt32 uiNewIndex = s_CategoryNames.GetCount();
  XII_ASSERT_DEV(uiNewIndex < 0xFFFF, "Maximum number of render data categories reached.");

  s_CategoryStringData.PushBack(sCategoryName);
  s_CategoryNames.PushBack(s_CategoryStringData.PeekBack());

  return xiiRenderDataCategory{static_cast<xiiUInt16>(uiNewIndex)};
}

xiiRenderDataCategory xiiRenderData::FindCategory(xiiStringView sCategoryName)
{
  XII_LOCK(s_CategoryMutex);

  for (xiiUInt32 i = 0; i < s_CategoryNames.GetCount(); ++i)
  {
    if (s_CategoryNames[i].IsEqual_NoCase(sCategoryName))
    {
      return xiiRenderDataCategory{static_cast<xiiUInt16>(i)};
    }
  }

  return xiiRenderDataCategory{0xFFFF}; // Invalid
}

xiiStringView xiiRenderData::GetCategoryName(xiiRenderDataCategory category)
{
  XII_LOCK(s_CategoryMutex);
  if (category.m_uiValue < s_CategoryNames.GetCount())
  {
    return s_CategoryNames[category.m_uiValue];
  }
  return xiiStringView();
}

const xiiArrayPtr<xiiStringView> xiiRenderData::GetAllCategoryNames()
{
  // Warning: Access is technically not thread-safe if returning raw pointer while array could grow.
  // But usually categories are registered once at startup.
  return s_CategoryNames;
}

void xiiRenderData::ClearAllCategories()
{
  XII_LOCK(s_CategoryMutex);
  s_CategoryNames.Clear();
  s_CategoryStringData.Clear();
}

xiiBitflags<xiiRenderDataRoutingFlags> xiiRenderData::RoutingFlagsFromLegacyCategory(xiiRenderDataCategory category)
{
  const xiiStringView sCategoryName = GetCategoryName(category);
  if (!sCategoryName.IsEmpty())
  {
    if (sCategoryName.IsEqual_NoCase("Light"))
      return xiiRenderDataRoutingFlags::Light;
    if (sCategoryName.IsEqual_NoCase("Decal"))
      return xiiRenderDataRoutingFlags::Decal;
    if (sCategoryName.IsEqual_NoCase("ReflectionProbe"))
      return xiiRenderDataRoutingFlags::ReflectionProbe;
    if (sCategoryName.IsEqual_NoCase("Sky"))
      return xiiRenderDataRoutingFlags::Sky;
    if (sCategoryName.IsEqual_NoCase("Opaque") || sCategoryName.IsEqual_NoCase("OpaqueStatic") || sCategoryName.IsEqual_NoCase("OpaqueDynamic") || sCategoryName.IsEqual_NoCase("LitOpaqueWithoutSelection"))
      return xiiRenderDataRoutingFlags::Opaque;
    if (sCategoryName.IsEqual_NoCase("Masked") || sCategoryName.IsEqual_NoCase("MaskedStatic") || sCategoryName.IsEqual_NoCase("MaskedDynamic") || sCategoryName.IsEqual_NoCase("LitMaskedWithoutSelection"))
      return xiiRenderDataRoutingFlags::Masked;
    if (sCategoryName.IsEqual_NoCase("Transparent") || sCategoryName.IsEqual_NoCase("LitTransparentWithoutSelection"))
      return xiiRenderDataRoutingFlags::Transparent;
    if (sCategoryName.IsEqual_NoCase("Foreground"))
      return xiiRenderDataRoutingFlags::Foreground;
    if (sCategoryName.IsEqual_NoCase("ScreenFX"))
      return xiiRenderDataRoutingFlags::ScreenFX;
    if (sCategoryName.IsEqual_NoCase("SimpleOpaque"))
      return xiiRenderDataRoutingFlags::SimpleOpaque;
    if (sCategoryName.IsEqual_NoCase("SimpleTransparent") || sCategoryName.IsEqual_NoCase("SimpleTransparentWithoutSelection"))
      return xiiRenderDataRoutingFlags::SimpleTransparent;
    if (sCategoryName.IsEqual_NoCase("Selection"))
      return xiiRenderDataRoutingFlags::Selection;
    if (sCategoryName.IsEqual_NoCase("GUI"))
      return xiiRenderDataRoutingFlags::GUI;
  }

  // Legacy fallback for default category IDs when names are unavailable.
  switch (category.m_uiValue)
  {
    case 0:  return xiiRenderDataRoutingFlags::Light;
    case 1:  return xiiRenderDataRoutingFlags::Decal;
    case 2:  return xiiRenderDataRoutingFlags::ReflectionProbe;
    case 3:  return xiiRenderDataRoutingFlags::Sky;
    case 4:
    case 5:
    case 6:  return xiiRenderDataRoutingFlags::Opaque;
    case 7:
    case 8:
    case 9:  return xiiRenderDataRoutingFlags::Masked;
    case 10: return xiiRenderDataRoutingFlags::Transparent;
    case 11: return xiiRenderDataRoutingFlags::Foreground;
    case 12: return xiiRenderDataRoutingFlags::ScreenFX;
    case 13: return xiiRenderDataRoutingFlags::SimpleOpaque;
    case 14: return xiiRenderDataRoutingFlags::SimpleTransparent;
    case 15: return xiiRenderDataRoutingFlags::Selection;
    case 16: return xiiRenderDataRoutingFlags::GUI;
    default: break;
  }

  return xiiRenderDataRoutingFlags::None;
}

// ----------------------------------------------------------------------------------------------------------------
// xiiExtractedRenderData

#include <GraphicsCore/Pipeline/ExtractedRenderData.h>

xiiExtractedRenderData::xiiExtractedRenderData() = default;

xiiExtractedRenderData::~xiiExtractedRenderData() = default;

void xiiExtractedRenderData::AddRenderDataInternal(xiiRenderData* pRenderData, xiiRenderData::Caching::Enum caching)
{
  if (pRenderData == nullptr)
  {
    return;
  }

  if (caching == xiiRenderData::Caching::IfStatic)
  {
    m_SubmittedStaticRenderData.PushBack(pRenderData);
  }
  else
  {
    m_SubmittedDynamicRenderData.PushBack(pRenderData);
  }
}

void xiiExtractedRenderData::AddRenderData(xiiRenderData* pRenderData, xiiRenderData::Caching::Enum caching)
{
  XII_LOCK(m_Mutex);
  AddRenderDataInternal(pRenderData, caching);
}

void xiiExtractedRenderData::AddRenderData(xiiRenderData* pRenderData, xiiRenderDataCategory category, xiiRenderData::Caching::Enum caching)
{
  XII_LOCK(m_Mutex);

  if (pRenderData != nullptr)
  {
    pRenderData->m_RoutingFlags = xiiRenderData::RoutingFlagsFromLegacyCategory(category);
  }

  AddRenderDataInternal(pRenderData, caching);
}

void xiiExtractedRenderData::AddRenderDataBatch(const xiiRenderDataBatch& batch, xiiRenderData::Caching::Enum caching)
{
  XII_LOCK(m_Mutex);

  for (xiiRenderData* pRenderData : batch.m_Data)
  {
    AddRenderDataInternal(pRenderData, caching);
  }
}

void xiiExtractedRenderData::AddRenderDataBatch(xiiRenderDataCategory category, const xiiRenderDataBatch& batch, xiiRenderData::Caching::Enum caching)
{
  XII_LOCK(m_Mutex);

  for (xiiRenderData* pRenderData : batch.m_Data)
  {
    if (pRenderData != nullptr)
    {
      pRenderData->m_RoutingFlags = xiiRenderData::RoutingFlagsFromLegacyCategory(category);
    }

    AddRenderDataInternal(pRenderData, caching);
  }
}

void xiiExtractedRenderData::Clear()
{
  XII_LOCK(m_Mutex);

  m_SubmittedStaticRenderData.Clear();
  m_SubmittedDynamicRenderData.Clear();

  m_SortedStaticRenderData.Clear();
  m_SortedDynamicRenderData.Clear();
  m_SortedAllRenderData.Clear();
}

void xiiExtractedRenderData::SortAndBatches()
{
  XII_LOCK(m_Mutex);

  xiiDynamicArray<xiiRenderData*> sortScratchBuffer;

  auto sortByKey = [&sortScratchBuffer](xiiDynamicArray<xiiRenderData*>& data) {
    if (data.IsEmpty())
      return;

    if (sortScratchBuffer.GetCount() != data.GetCount())
    {
      sortScratchBuffer.SetCountUninitialized(data.GetCount());
    }

    xiiArrayPtr<xiiRenderData*> dataPtr = data;
    xiiSorting::RadixSort(dataPtr, sortScratchBuffer, [](const xiiRenderData* pRenderData) -> xiiUInt64 {
      return pRenderData->m_uiSortingKey;
    });
  };

  m_SortedStaticRenderData  = m_SubmittedStaticRenderData;
  m_SortedDynamicRenderData = m_SubmittedDynamicRenderData;

  sortByKey(m_SortedStaticRenderData);
  sortByKey(m_SortedDynamicRenderData);

  m_SortedAllRenderData = m_SortedStaticRenderData;
  m_SortedAllRenderData.PushBackRange(m_SortedDynamicRenderData);
  sortByKey(m_SortedAllRenderData);
}

xiiArrayPtr<xiiRenderData* const> xiiExtractedRenderData::GetAllRenderData() const
{
  return m_SortedAllRenderData;
}

xiiArrayPtr<xiiRenderData* const> xiiExtractedRenderData::GetStaticRenderData() const
{
  return m_SortedStaticRenderData;
}

xiiArrayPtr<xiiRenderData* const> xiiExtractedRenderData::GetDynamicRenderData() const
{
  return m_SortedDynamicRenderData;
}
