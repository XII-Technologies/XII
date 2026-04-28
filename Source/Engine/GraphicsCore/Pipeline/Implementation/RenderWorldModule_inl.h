/// Copyright (c) Theophilus Eriata. All Rights Reserved.

template <typename T>
T* xiiRenderWorldModule::CreateRenderDataForThisFrame(const xiiComponent* pComponent) const
{
  static_assert(XII_IS_DERIVED_FROM_STATIC(xiiRenderData, T));

  T* pRenderData = XII_NEW(xiiFrameAllocator::GetCurrentAllocator(), T);

  if (pComponent != nullptr)
  {
    pRenderData->m_hOwnerObject    = pComponent->GetOwner()->GetHandle();
    pRenderData->m_hOwnerComponent = pComponent->GetHandle();
    pRenderData->m_GlobalTransform = pComponent->GetOwner()->GetGlobalTransform();
    pRenderData->m_GlobalBounds    = pComponent->GetOwner()->GetGlobalBounds();
  }

  return pRenderData;
}
