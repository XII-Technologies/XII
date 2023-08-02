#include <Core/CorePCH.h>

#include <Core/World/SpatialData.h>

xiiHybridArray<xiiSpatialData::CategoryData, 32>& xiiSpatialData::GetCategoryData()
{
  static xiiHybridArray<xiiSpatialData::CategoryData, 32> CategoryData;
  return CategoryData;
}

// static
xiiSpatialData::Category xiiSpatialData::RegisterCategory(xiiStringView sCategoryName, const xiiBitflags<Flags>& flags)
{
  if (sCategoryName.IsEmpty())
    return xiiInvalidSpatialDataCategory;

  Category oldCategory = FindCategory(sCategoryName);
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
  data.m_sName.Assign(sCategoryName);
  data.m_Flags = flags;

  return newCategory;
}

// static
xiiSpatialData::Category xiiSpatialData::FindCategory(xiiStringView sCategoryName)
{
  xiiTempHashedString categoryName(sCategoryName);

  for (xiiUInt32 uiCategoryIndex = 0; uiCategoryIndex < GetCategoryData().GetCount(); ++uiCategoryIndex)
  {
    if (GetCategoryData()[uiCategoryIndex].m_sName == categoryName)
      return Category(uiCategoryIndex);
  }

  return xiiInvalidSpatialDataCategory;
}

// static
const xiiHashedString& xiiSpatialData::GetCategoryName(Category category)
{
  if (category.m_uiValue < GetCategoryData().GetCount())
  {
    return GetCategoryData()[category.m_uiValue].m_sName;
  }

  static xiiHashedString sInvalidSpatialDataCategoryName;
  return sInvalidSpatialDataCategoryName;
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
