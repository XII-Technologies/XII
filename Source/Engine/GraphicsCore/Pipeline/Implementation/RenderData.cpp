#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/RenderData.h>
#include <Foundation/Algorithm/Sorting.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Threading/Mutex.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderData, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

static xiiMutex s_CategoryMutex;
static xiiDynamicArray<xiiStringView> s_CategoryNames;

// ----------------------------------------------------------------------------------------------------------------
// xiiRenderData

xiiRenderDataCategory xiiRenderData::RegisterCategory(const char* szCategoryName)
{
  XII_LOCK(s_CategoryMutex);

  for (xiiUInt32 i = 0; i < s_CategoryNames.GetCount(); ++i)
  {
    if (s_CategoryNames[i].IsEqual_NoCase(szCategoryName))
    {
      return xiiRenderDataCategory{ static_cast<xiiUInt16>(i) };
    }
  }

  // Create new category
  xiiUInt32 newIdx = s_CategoryNames.GetCount();
  XII_ASSERT_DEV(newIdx < 0xFFFF, "Maximum number of render data categories reached.");

  // Store persistent string for the view. Easiest way is to just keep it in another array if necessary,
  // but usually szCategoryName is a static string literal. However, to be safe, we can deep copy or assume it's stable.
  // Wait, xiiStringView doesn't own memory. If it's a static constant, it's fine.
  // If we need to own it, we should use xiiHashedString or allocate it.
  static xiiDynamicArray<xiiString> s_CategoryStringData;
  s_CategoryStringData.PushBack(szCategoryName);
  s_CategoryNames.PushBack(s_CategoryStringData.PeekBack());

  return xiiRenderDataCategory{ static_cast<xiiUInt16>(newIdx) };
}

xiiRenderDataCategory xiiRenderData::FindCategory(const char* szCategoryName)
{
  XII_LOCK(s_CategoryMutex);

  for (xiiUInt32 i = 0; i < s_CategoryNames.GetCount(); ++i)
  {
    if (s_CategoryNames[i].IsEqual_NoCase(szCategoryName))
    {
      return xiiRenderDataCategory{ static_cast<xiiUInt16>(i) };
    }
  }

  return xiiRenderDataCategory{ 0xFFFF }; // Invalid
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

xiiRenderDataCategory xiiDefaultRenderDataCategories::Light = xiiRenderDataCategory{};
xiiRenderDataCategory xiiDefaultRenderDataCategories::Decal = xiiRenderDataCategory{};
xiiRenderDataCategory xiiDefaultRenderDataCategories::ReflectionProbe = xiiRenderDataCategory{};
xiiRenderDataCategory xiiDefaultRenderDataCategories::Sky = xiiRenderDataCategory{};
xiiRenderDataCategory xiiDefaultRenderDataCategories::OpaqueStatic = xiiRenderDataCategory{};
xiiRenderDataCategory xiiDefaultRenderDataCategories::OpaqueDynamic = xiiRenderDataCategory{};
xiiRenderDataCategory xiiDefaultRenderDataCategories::Opaque = xiiRenderDataCategory{};
xiiRenderDataCategory xiiDefaultRenderDataCategories::MaskedStatic = xiiRenderDataCategory{};
xiiRenderDataCategory xiiDefaultRenderDataCategories::MaskedDynamic = xiiRenderDataCategory{};
xiiRenderDataCategory xiiDefaultRenderDataCategories::Masked = xiiRenderDataCategory{};
xiiRenderDataCategory xiiDefaultRenderDataCategories::Transparent = xiiRenderDataCategory{};
xiiRenderDataCategory xiiDefaultRenderDataCategories::Foreground = xiiRenderDataCategory{};
xiiRenderDataCategory xiiDefaultRenderDataCategories::ScreenFX = xiiRenderDataCategory{};
xiiRenderDataCategory xiiDefaultRenderDataCategories::SimpleOpaque = xiiRenderDataCategory{};
xiiRenderDataCategory xiiDefaultRenderDataCategories::SimpleTransparent = xiiRenderDataCategory{};
xiiRenderDataCategory xiiDefaultRenderDataCategories::Selection = xiiRenderDataCategory{};
xiiRenderDataCategory xiiDefaultRenderDataCategories::GUI = xiiRenderDataCategory{};

void xiiDefaultRenderDataCategories::RegisterDefaultCategories()
{
  if (Light.IsValid())
    return; // Already registered

  Light = xiiRenderData::RegisterCategory("Light");
  Decal = xiiRenderData::RegisterCategory("Decal");
  ReflectionProbe = xiiRenderData::RegisterCategory("ReflectionProbe");
  Sky = xiiRenderData::RegisterCategory("Sky");
  OpaqueStatic = xiiRenderData::RegisterCategory("OpaqueStatic");
  OpaqueDynamic = xiiRenderData::RegisterCategory("OpaqueDynamic");
  Opaque = xiiRenderData::RegisterCategory("Opaque");
  MaskedStatic = xiiRenderData::RegisterCategory("MaskedStatic");
  MaskedDynamic = xiiRenderData::RegisterCategory("MaskedDynamic");
  Masked = xiiRenderData::RegisterCategory("Masked");
  Transparent = xiiRenderData::RegisterCategory("Transparent");
  Foreground = xiiRenderData::RegisterCategory("Foreground");
  ScreenFX = xiiRenderData::RegisterCategory("ScreenFX");
  SimpleOpaque = xiiRenderData::RegisterCategory("SimpleOpaque");
  SimpleTransparent = xiiRenderData::RegisterCategory("SimpleTransparent");
  Selection = xiiRenderData::RegisterCategory("Selection");
  GUI = xiiRenderData::RegisterCategory("GUI");
}

// ----------------------------------------------------------------------------------------------------------------
// xiiExtractedRenderData

#include <GraphicsCore/Pipeline/ExtractedRenderData.h>

xiiExtractedRenderData::xiiExtractedRenderData()
{
}

xiiExtractedRenderData::~xiiExtractedRenderData()
{
}

void xiiExtractedRenderData::AddRenderDataBatch(xiiRenderDataCategory category, const xiiRenderDataBatch& batch)
{
  XII_LOCK(m_Mutex);
  if (category.m_uiValue >= m_BatchesPerCategory.GetCount())
  {
    m_BatchesPerCategory.SetCount(category.m_uiValue + 1);
  }
  m_BatchesPerCategory[category.m_uiValue].PushBack(batch);
}

void xiiExtractedRenderData::Clear()
{
  XII_LOCK(m_Mutex);
  for (auto& batches : m_BatchesPerCategory)
  {
    batches.Clear();
  }
  for (auto& sortedData : m_SortedRenderData)
  {
    sortedData.Clear();
  }
}

void xiiExtractedRenderData::SortAndBatches()
{
  XII_LOCK(m_Mutex);

  // Resize sorted data array to match the maximum category ID we have received batches for
  m_SortedRenderData.SetCount(m_BatchesPerCategory.GetCount());

  xiiDynamicArray<xiiRenderData*> sortScratchBuffer;

  for (xiiUInt32 i = 0; i < m_BatchesPerCategory.GetCount(); ++i)
  {
    auto& batches = m_BatchesPerCategory[i];
    auto& sortedData = m_SortedRenderData[i];

    sortedData.Clear();

    // Flatten
    xiiUInt32 uiTotalElements = 0;
    for (const auto& batch : batches)
    {
      uiTotalElements += batch.m_Data.GetCount();
    }

    if (uiTotalElements > 0)
    {
      sortedData.Reserve(uiTotalElements);
      for (const auto& batch : batches)
      {
        sortedData.PushBackRange(batch.m_Data);
      }

      // Sort
      xiiArrayPtr<xiiRenderData*> sortedDataPtr = sortedData;
      xiiSorting::RadixSort(sortedDataPtr, sortScratchBuffer, [](const xiiRenderData* pRenderData) -> xiiUInt64
        {
          return pRenderData->m_uiSortingKey;
        });
    }
  }
}

xiiArrayPtr<xiiRenderData* const> xiiExtractedRenderData::GetRenderData(xiiRenderDataCategory category) const
{
  if (category.m_uiValue < m_SortedRenderData.GetCount())
  {
    return m_SortedRenderData[category.m_uiValue];
  }
  return xiiArrayPtr<xiiRenderData* const>();
}
