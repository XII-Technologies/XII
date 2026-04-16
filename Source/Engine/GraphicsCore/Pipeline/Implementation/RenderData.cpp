#include <Foundation/Algorithm/Sorting.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Threading/Mutex.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/RenderData.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderData, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

static xiiMutex                       s_CategoryMutex;
static xiiDynamicArray<xiiStringView> s_CategoryNames;

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

  // Store persistent string for the view. Easiest way is to just keep it in another array if necessary,
  // but usually sCategoryName is a static string literal. However, to be safe, we can deep copy or assume it's stable.
  // Wait, xiiStringView doesn't own memory. If it's a static constant, it's fine.
  // If we need to own it, we should use xiiHashedString or allocate it.
  static xiiDynamicArray<xiiString> s_CategoryStringData;
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
}

// ----------------------------------------------------------------------------------------------------------------
// xiiDefaultRenderDataCategories

xiiRenderDataCategory xiiDefaultRenderDataCategories::Light             = xiiRenderDataCategory{};
xiiRenderDataCategory xiiDefaultRenderDataCategories::Decal             = xiiRenderDataCategory{};
xiiRenderDataCategory xiiDefaultRenderDataCategories::ReflectionProbe   = xiiRenderDataCategory{};
xiiRenderDataCategory xiiDefaultRenderDataCategories::Sky               = xiiRenderDataCategory{};
xiiRenderDataCategory xiiDefaultRenderDataCategories::OpaqueStatic      = xiiRenderDataCategory{};
xiiRenderDataCategory xiiDefaultRenderDataCategories::OpaqueDynamic     = xiiRenderDataCategory{};
xiiRenderDataCategory xiiDefaultRenderDataCategories::Opaque            = xiiRenderDataCategory{};
xiiRenderDataCategory xiiDefaultRenderDataCategories::MaskedStatic      = xiiRenderDataCategory{};
xiiRenderDataCategory xiiDefaultRenderDataCategories::MaskedDynamic     = xiiRenderDataCategory{};
xiiRenderDataCategory xiiDefaultRenderDataCategories::Masked            = xiiRenderDataCategory{};
xiiRenderDataCategory xiiDefaultRenderDataCategories::Transparent       = xiiRenderDataCategory{};
xiiRenderDataCategory xiiDefaultRenderDataCategories::Foreground        = xiiRenderDataCategory{};
xiiRenderDataCategory xiiDefaultRenderDataCategories::ScreenFX          = xiiRenderDataCategory{};
xiiRenderDataCategory xiiDefaultRenderDataCategories::SimpleOpaque      = xiiRenderDataCategory{};
xiiRenderDataCategory xiiDefaultRenderDataCategories::SimpleTransparent = xiiRenderDataCategory{};
xiiRenderDataCategory xiiDefaultRenderDataCategories::Selection         = xiiRenderDataCategory{};
xiiRenderDataCategory xiiDefaultRenderDataCategories::GUI               = xiiRenderDataCategory{};

void xiiDefaultRenderDataCategories::RegisterDefaultCategories()
{
  if (Light.IsValid())
    return; // Already registered

  Light             = xiiRenderData::RegisterCategory("Light");
  Decal             = xiiRenderData::RegisterCategory("Decal");
  ReflectionProbe   = xiiRenderData::RegisterCategory("ReflectionProbe");
  Sky               = xiiRenderData::RegisterCategory("Sky");
  OpaqueStatic      = xiiRenderData::RegisterCategory("OpaqueStatic");
  OpaqueDynamic     = xiiRenderData::RegisterCategory("OpaqueDynamic");
  Opaque            = xiiRenderData::RegisterCategory("Opaque");
  MaskedStatic      = xiiRenderData::RegisterCategory("MaskedStatic");
  MaskedDynamic     = xiiRenderData::RegisterCategory("MaskedDynamic");
  Masked            = xiiRenderData::RegisterCategory("Masked");
  Transparent       = xiiRenderData::RegisterCategory("Transparent");
  Foreground        = xiiRenderData::RegisterCategory("Foreground");
  ScreenFX          = xiiRenderData::RegisterCategory("ScreenFX");
  SimpleOpaque      = xiiRenderData::RegisterCategory("SimpleOpaque");
  SimpleTransparent = xiiRenderData::RegisterCategory("SimpleTransparent");
  Selection         = xiiRenderData::RegisterCategory("Selection");
  GUI               = xiiRenderData::RegisterCategory("GUI");
}

// ----------------------------------------------------------------------------------------------------------------
// xiiExtractedRenderData

#include <GraphicsCore/Pipeline/ExtractedRenderData.h>

xiiExtractedRenderData::xiiExtractedRenderData() = default;

xiiExtractedRenderData::~xiiExtractedRenderData() = default;

void xiiExtractedRenderData::AddRenderDataInternal(xiiRenderData* pRenderData, xiiRenderDataCategory category, xiiRenderData::Caching::Enum caching)
{
  if (pRenderData == nullptr)
  {
    return;
  }

  pRenderData->m_Category = category;

  if (caching == xiiRenderData::Caching::IfStatic)
  {
    m_SubmittedStaticRenderData.PushBack(pRenderData);
  }
  else
  {
    m_SubmittedDynamicRenderData.PushBack(pRenderData);
  }
}

void xiiExtractedRenderData::AddRenderData(xiiRenderData* pRenderData, xiiRenderDataCategory category, xiiRenderData::Caching::Enum caching)
{
  XII_LOCK(m_Mutex);
  AddRenderDataInternal(pRenderData, category, caching);
}

void xiiExtractedRenderData::AddRenderDataBatch(xiiRenderDataCategory category, const xiiRenderDataBatch& batch, xiiRenderData::Caching::Enum caching)
{
  XII_LOCK(m_Mutex);

  for (xiiRenderData* pRenderData : batch.m_Data)
  {
    AddRenderDataInternal(pRenderData, category, caching);
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

  for (auto& sortedData : m_SortedRenderDataByCategory)
  {
    sortedData.Clear();
  }
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

  // Build compatibility category slices from the unified list.
  m_SortedRenderDataByCategory.Clear();
  for (xiiRenderData* pRenderData : m_SortedAllRenderData)
  {
    if (pRenderData == nullptr || !pRenderData->m_Category.IsValid())
      continue;

    const xiiUInt16 uiCategory = pRenderData->m_Category.m_uiValue;
    if (uiCategory >= m_SortedRenderDataByCategory.GetCount())
    {
      m_SortedRenderDataByCategory.SetCount(uiCategory + 1);
    }

    m_SortedRenderDataByCategory[uiCategory].PushBack(pRenderData);
  }
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

xiiArrayPtr<xiiRenderData* const> xiiExtractedRenderData::GetRenderData(xiiRenderDataCategory category) const
{
  if (category.m_uiValue < m_SortedRenderDataByCategory.GetCount())
  {
    return m_SortedRenderDataByCategory[category.m_uiValue];
  }

  return xiiArrayPtr<xiiRenderData* const>();
}
