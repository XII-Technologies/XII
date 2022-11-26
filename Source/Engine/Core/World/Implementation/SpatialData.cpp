#include <Core/CorePCH.h>

#include <Core/World/SpatialData.h>

xiiHybridArray<xiiSpatialData::CategoryData, 32>& xiiSpatialData::GetCategoryData()
{
  static xiiHybridArray<xiiSpatialData::CategoryData, 32> CategoryData;
  return CategoryData;
}

// static
xiiSpatialData::Category xiiSpatialData::RegisterCategory(const char* szCategoryName, const xiiBitflags<Flags>& flags)
{
  Category oldCategory = FindCategory(szCategoryName);
  if (oldCategory != xiiInvalidSpatialDataCategory)
  {
    XII_ASSERT_DEV(GetCategoryFlags(oldCategory) == flags, "Category registered with different flags");
    return oldCategory;
  }

  if (GetCategoryData().GetCount() == 32)
  {
    XII_REPORT_FAILURE("Too many spatial data categories");
    return xiiInvalidSpatialDataCategory;
  }

  Category newCategory = Category(GetCategoryData().GetCount());

  auto& data = GetCategoryData().ExpandAndGetRef();
  data.m_sName.Assign(szCategoryName);
  data.m_Flags = flags;

  return newCategory;
}

// static
xiiSpatialData::Category xiiSpatialData::FindCategory(const char* szCategoryName)
{
  xiiTempHashedString categoryName(szCategoryName);

  for (xiiUInt32 uiCategoryIndex = 0; uiCategoryIndex < GetCategoryData().GetCount(); ++uiCategoryIndex)
  {
    if (GetCategoryData()[uiCategoryIndex].m_sName == categoryName)
      return Category(uiCategoryIndex);
  }

  return xiiInvalidSpatialDataCategory;
}

// static
const xiiBitflags<xiiSpatialData::Flags>& xiiSpatialData::GetCategoryFlags(Category category)
{
  return GetCategoryData()[category.m_uiValue].m_Flags;
}

//////////////////////////////////////////////////////////////////////////

xiiSpatialData::Category xiiDefaultSpatialDataCategories::RenderStatic     = xiiSpatialData::RegisterCategory("RenderStatic", xiiSpatialData::Flags::None);
xiiSpatialData::Category xiiDefaultSpatialDataCategories::RenderDynamic    = xiiSpatialData::RegisterCategory("RenderDynamic", xiiSpatialData::Flags::FrequentChanges);
xiiSpatialData::Category xiiDefaultSpatialDataCategories::OcclusionStatic  = xiiSpatialData::RegisterCategory("OcclusionStatic", xiiSpatialData::Flags::None);
xiiSpatialData::Category xiiDefaultSpatialDataCategories::OcclusionDynamic = xiiSpatialData::RegisterCategory("OcclusionDynamic", xiiSpatialData::Flags::FrequentChanges);


XII_STATICLINK_FILE(Core, Core_World_Implementation_SpatialData);
