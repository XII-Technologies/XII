#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/RenderData.h>
#include <GraphicsCore/Pipeline/Renderer.h>
#include <GraphicsCore/Pipeline/SortingFunctions.h>

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsCore, RenderData)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    xiiRenderData::UpdateRendererTypes();

    xiiPlugin::Events().AddEventHandler(xiiRenderData::PluginEventHandler);
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    xiiPlugin::Events().RemoveEventHandler(xiiRenderData::PluginEventHandler);

    xiiRenderData::ClearRendererInstances();
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderData, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderer, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgExtractRenderData);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgExtractRenderData, 1, xiiRTTIDefaultAllocator<xiiMsgExtractRenderData>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiExcludeFromScript()
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiHybridArray<xiiRenderData::CategoryData, 32> xiiRenderData::s_CategoryData;

xiiHybridArray<const xiiRTTI*, 16>         xiiRenderData::s_RendererTypes;
xiiDynamicArray<xiiUniquePtr<xiiRenderer>> xiiRenderData::s_RendererInstances;
bool                                       xiiRenderData::s_bRendererInstancesDirty = false;

// static
xiiRenderData::Category xiiRenderData::RegisterCategory(xiiStringView sCategoryName, SortingKeyFunc sortingKeyFunc)
{
  xiiHashedString sCategoryNameHash;
  sCategoryNameHash.Assign(sCategoryName);

  Category oldCategory = FindCategory(sCategoryNameHash);
  if (oldCategory != xiiInvalidRenderDataCategory)
    return oldCategory;

  Category newCategory = Category(static_cast<xiiUInt16>(s_CategoryData.GetCount()));

  auto& data            = s_CategoryData.ExpandAndGetRef();
  data.m_sName          = sCategoryNameHash;
  data.m_SortingKeyFunc = sortingKeyFunc;

  return newCategory;
}

// static
xiiRenderData::Category xiiRenderData::RegisterDerivedCategory(xiiStringView sCategoryName, Category baseCategory)
{
  auto& baseCategoryData = s_CategoryData[baseCategory.m_uiValue];

  Category derivedCategory                                 = RegisterCategory(sCategoryName, baseCategoryData.m_SortingKeyFunc);
  s_CategoryData[derivedCategory.m_uiValue].m_BaseCategory = baseCategory;

  return derivedCategory;
}

// static
xiiRenderData::Category xiiRenderData::RegisterRedirectedCategory(xiiStringView sCategoryName, Category staticCategory, Category dynamicCategory)
{
  Category newCategory                                    = RegisterCategory(sCategoryName, nullptr);
  s_CategoryData[newCategory.m_uiValue].m_StaticCategory  = staticCategory;
  s_CategoryData[newCategory.m_uiValue].m_DynamicCategory = dynamicCategory;

  return newCategory;
}

// static
xiiRenderData::Category xiiRenderData::FindCategory(xiiTempHashedString sCategoryName)
{
  for (xiiUInt32 uiCategoryIndex = 0; uiCategoryIndex < s_CategoryData.GetCount(); ++uiCategoryIndex)
  {
    if (s_CategoryData[uiCategoryIndex].m_sName == sCategoryName)
      return Category(static_cast<xiiUInt16>(uiCategoryIndex));
  }

  return xiiInvalidRenderDataCategory;
}

// static
xiiRenderData::Category xiiRenderData::ResolveCategory(Category category, bool bDynamic)
{
  auto& categoryData = s_CategoryData[category.m_uiValue];
  if (categoryData.m_StaticCategory != xiiInvalidRenderDataCategory)
  {
    return bDynamic ? categoryData.m_DynamicCategory : categoryData.m_StaticCategory;
  }
  return category;
}

// static
void xiiRenderData::GetAllCategoryNames(xiiDynamicArray<xiiHashedString>& out_categoryNames)
{
  out_categoryNames.Clear();

  for (auto& data : s_CategoryData)
  {
    out_categoryNames.PushBack(data.m_sName);
  }
}

// static
void xiiRenderData::PluginEventHandler(const xiiPluginEvent& e)
{
  switch (e.m_EventType)
  {
    case xiiPluginEvent::AfterPluginChanges:
      UpdateRendererTypes();
      break;

    default:
      break;
  }
}

// static
void xiiRenderData::UpdateRendererTypes()
{
  s_RendererTypes.Clear();

  xiiRTTI::ForEachDerivedType<xiiRenderer>([](const xiiRTTI* pRtti) { s_RendererTypes.PushBack(pRtti); }, xiiRTTI::ForEachOptions::ExcludeNonAllocatable);

  s_bRendererInstancesDirty = true;
}

// static
void xiiRenderData::CreateRendererInstances()
{
  ClearRendererInstances();

  for (auto pRendererType : s_RendererTypes)
  {
    XII_ASSERT_DEV(pRendererType->IsDerivedFrom(xiiGetStaticRTTI<xiiRenderer>()), "Renderer type '{}' must be derived from xiiRenderer.", pRendererType->GetTypeName());

    auto pRenderer = pRendererType->GetAllocator()->Allocate<xiiRenderer>();

    xiiUInt32 uiIndex = s_RendererInstances.GetCount();
    s_RendererInstances.PushBack(pRenderer);

    xiiHybridArray<Category, 8U> supportedCategories;
    pRenderer->GetSupportedRenderDataCategories(supportedCategories);

    xiiHybridArray<const xiiRTTI*, 8U> supportedTypes;
    pRenderer->GetSupportedRenderDataTypes(supportedTypes);

    for (auto pType : supportedTypes)
    {
      for (Category category : supportedCategories)
      {
        auto& categoryData = s_CategoryData[category.m_uiValue];
        if (categoryData.m_StaticCategory != xiiInvalidRenderDataCategory)
        {
          s_CategoryData[categoryData.m_StaticCategory.m_uiValue].m_TypeToRendererIndex.Insert(pType, uiIndex);
          s_CategoryData[categoryData.m_DynamicCategory.m_uiValue].m_TypeToRendererIndex.Insert(pType, uiIndex);
        }
        else
        {
          categoryData.m_TypeToRendererIndex.Insert(pType, uiIndex);
        }
      }
    }
  }

  // Copy the renderer types to derived categories.
  for (auto& categoryData : s_CategoryData)
  {
    if (categoryData.m_BaseCategory == xiiInvalidRenderDataCategory)
      continue;

    categoryData.m_TypeToRendererIndex = s_CategoryData[categoryData.m_BaseCategory.m_uiValue].m_TypeToRendererIndex;
  }

  s_bRendererInstancesDirty = false;
}

// static
void xiiRenderData::ClearRendererInstances()
{
  s_RendererInstances.Clear();

  for (auto& categoryData : s_CategoryData)
  {
    categoryData.m_TypeToRendererIndex.Clear();
  }
}

//////////////////////////////////////////////////////////////////////////

xiiRenderData::Category xiiDefaultRenderDataCategories::Light           = xiiRenderData::RegisterCategory("Light", &xiiRenderSortingFunctions::ByRenderDataThenFrontToBack);
xiiRenderData::Category xiiDefaultRenderDataCategories::Decal           = xiiRenderData::RegisterCategory("Decal", &xiiRenderSortingFunctions::ByRenderDataThenFrontToBack);
xiiRenderData::Category xiiDefaultRenderDataCategories::ReflectionProbe = xiiRenderData::RegisterCategory("ReflectionProbe", &xiiRenderSortingFunctions::ByRenderDataThenFrontToBack);
xiiRenderData::Category xiiDefaultRenderDataCategories::Sky             = xiiRenderData::RegisterCategory("Sky", &xiiRenderSortingFunctions::ByRenderDataThenFrontToBack);

xiiRenderData::Category xiiDefaultRenderDataCategories::OpaqueStatic  = xiiRenderData::RegisterCategory("OpaqueStatic", &xiiRenderSortingFunctions::ByRenderDataThenFrontToBack);
xiiRenderData::Category xiiDefaultRenderDataCategories::OpaqueDynamic = xiiRenderData::RegisterCategory("OpaqueDynamic", &xiiRenderSortingFunctions::ByRenderDataThenFrontToBack);
xiiRenderData::Category xiiDefaultRenderDataCategories::Opaque        = xiiRenderData::RegisterRedirectedCategory("Opaque", xiiDefaultRenderDataCategories::OpaqueStatic, xiiDefaultRenderDataCategories::OpaqueDynamic);


xiiRenderData::Category xiiDefaultRenderDataCategories::MaskedStatic  = xiiRenderData::RegisterCategory("MaskedStatic", &xiiRenderSortingFunctions::ByRenderDataThenFrontToBack);
xiiRenderData::Category xiiDefaultRenderDataCategories::MaskedDynamic = xiiRenderData::RegisterCategory("MaskedDynamic", &xiiRenderSortingFunctions::ByRenderDataThenFrontToBack);
xiiRenderData::Category xiiDefaultRenderDataCategories::Masked        = xiiRenderData::RegisterRedirectedCategory("Masked", xiiDefaultRenderDataCategories::MaskedStatic, xiiDefaultRenderDataCategories::MaskedDynamic);

xiiRenderData::Category xiiDefaultRenderDataCategories::Transparent = xiiRenderData::RegisterCategory("Transparent", &xiiRenderSortingFunctions::BackToFrontThenByRenderData);

xiiRenderData::Category xiiDefaultRenderDataCategories::Foreground = xiiRenderData::RegisterCategory("Foreground", &xiiRenderSortingFunctions::ByRenderDataThenFrontToBack);
xiiRenderData::Category xiiDefaultRenderDataCategories::ScreenFX   = xiiRenderData::RegisterCategory("ScreenFX", &xiiRenderSortingFunctions::BackToFrontThenByRenderData);

xiiRenderData::Category xiiDefaultRenderDataCategories::SimpleOpaque      = xiiRenderData::RegisterCategory("SimpleOpaque", &xiiRenderSortingFunctions::ByRenderDataThenFrontToBack);
xiiRenderData::Category xiiDefaultRenderDataCategories::SimpleTransparent = xiiRenderData::RegisterCategory("SimpleTransparent", &xiiRenderSortingFunctions::BackToFrontThenByRenderData);

xiiRenderData::Category xiiDefaultRenderDataCategories::Selection = xiiRenderData::RegisterCategory("Selection", &xiiRenderSortingFunctions::ByRenderDataThenFrontToBack);
xiiRenderData::Category xiiDefaultRenderDataCategories::GUI       = xiiRenderData::RegisterCategory("GUI", &xiiRenderSortingFunctions::BackToFrontThenByRenderData);

//////////////////////////////////////////////////////////////////////////

void xiiMsgExtractRenderData::AddRenderData(const xiiRenderData* pRenderData, xiiRenderData::Category category, xiiRenderData::Caching::Enum cachingBehavior)
{
  auto& cached         = m_ExtractedRenderData.ExpandAndGetRef();
  cached.m_pRenderData = pRenderData;
  cached.m_Category    = xiiRenderData::ResolveCategory(category, pRenderData->m_Flags.IsSet(xiiRenderData::Flags::Dynamic));

  if (cachingBehavior == xiiRenderData::Caching::IfStatic)
  {
    ++m_uiNumCacheIfStatic;
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_RenderData);
