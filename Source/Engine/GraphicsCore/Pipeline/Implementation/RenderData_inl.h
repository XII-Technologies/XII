#include <Core/World/GameObject.h>

XII_ALWAYS_INLINE xiiRenderData::Category::Category() = default;

XII_ALWAYS_INLINE xiiRenderData::Category::Category(xiiUInt16 uiValue) :
  m_uiValue(uiValue)
{
}

XII_ALWAYS_INLINE bool xiiRenderData::Category::operator==(const Category& other) const
{
  return m_uiValue == other.m_uiValue;
}

XII_ALWAYS_INLINE bool xiiRenderData::Category::operator!=(const Category& other) const
{
  return m_uiValue != other.m_uiValue;
}

//////////////////////////////////////////////////////////////////////////

// static
XII_FORCE_INLINE const xiiRenderer* xiiRenderData::GetCategoryRenderer(Category category, const xiiRTTI* pRenderDataType)
{
  if (s_bRendererInstancesDirty)
  {
    CreateRendererInstances();
  }

  auto& categoryData = s_CategoryData[category.m_uiValue];

  xiiUInt32 uiIndex = 0;
  if (categoryData.m_TypeToRendererIndex.TryGetValue(pRenderDataType, uiIndex))
  {
    return s_RendererInstances[uiIndex].Borrow();
  }

  return nullptr;
}

// static
XII_FORCE_INLINE xiiHashedString xiiRenderData::GetCategoryName(Category category)
{
  if (category.m_uiValue < s_CategoryData.GetCount())
  {
    return s_CategoryData[category.m_uiValue].m_sName;
  }

  return xiiHashedString();
}

XII_FORCE_INLINE xiiUInt64 xiiRenderData::GetCategorySortingKey(Category category, const xiiCamera& camera) const
{
  return s_CategoryData[category.m_uiValue].m_sortingKeyFunc(this, camera);
}

//////////////////////////////////////////////////////////////////////////

template <typename T>
static T* xiiCreateRenderDataForThisFrame(const xiiGameObject* pOwner)
{
  XII_CHECK_AT_COMPILETIME(XII_IS_DERIVED_FROM_STATIC(xiiRenderData, T));

  T* pRenderData = XII_NEW(xiiFrameAllocator::GetCurrentAllocator(), T);

  if (pOwner != nullptr)
  {
    pRenderData->m_hOwner = pOwner->GetHandle();
  }

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  pRenderData->m_pOwner = pOwner;
#endif

  return pRenderData;
}
