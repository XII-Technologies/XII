
#include <GraphicsCore/../../../Data/Base/Shaders/Common/ObjectConstants.h>

template <typename T>
T* xiiRenderDataManager::CreateRenderDataForThisFrame(const xiiGameObject* pOwner) const
{
  static_assert(XII_IS_DERIVED_FROM_STATIC(xiiRenderData, T));

  T* pRenderData = XII_NEW(xiiFrameAllocator::GetCurrentAllocator(), T);

  if (pOwner != nullptr)
  {
    pRenderData->m_Flags.AddOrRemove(xiiRenderData::Flags::Dynamic, pOwner->IsDynamic());
    pRenderData->m_Flags.AddOrRemove(xiiRenderData::Flags::FlipWinding, pOwner->GetGlobalTransformSimd().HasMirrorScaling());

    pRenderData->m_vGlobalPosition = pOwner->GetGlobalPosition();

    pRenderData->m_hOwner = pOwner->GetHandle();
  }

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  pRenderData->m_pOwner = pOwner;
#endif

  return pRenderData;
}

// static
XII_FORCE_INLINE void xiiRenderDataManager::FillPerInstanceData(xiiPerInstanceData& out_perInstanceData, const xiiGameObject* pObject, const xiiTransform& globalTransform, xiiUInt32 uiUniqueID /*= 0*/, const xiiColor& color /*= xiiColor::White*/, float fBoundingSphereRadius /*= 1.0f*/, xiiUInt32 uiRandomSeed /*= 0*/)
{
  xiiMat4 objectToWorld             = globalTransform.GetAsMat4();
  out_perInstanceData.ObjectToWorld = objectToWorld;

  if (globalTransform.ContainsUniformScale())
  {
    out_perInstanceData.ObjectToWorldNormal = objectToWorld;
  }
  else
  {
    xiiMat3 mInverse = objectToWorld.GetRotationalPart();
    mInverse.Invert(0.0f).IgnoreResult();
    // we explicitly ignore the return value here (success / failure)
    // because when we have a scale of 0 (which happens temporarily during editing) that would be annoying

    out_perInstanceData.ObjectToWorldNormal = mInverse.GetTranspose();
  }

  if (pObject != nullptr)
  {
    out_perInstanceData.BoundingSphereRadius = pObject->GetGlobalBounds().m_fSphereRadius;
  }
  else
  {
    out_perInstanceData.BoundingSphereRadius = fBoundingSphereRadius;
  }

  out_perInstanceData.GameObjectID = uiUniqueID;
  out_perInstanceData.Reserved     = 0;

  out_perInstanceData.Color = color;
}

XII_FORCE_INLINE xiiSharedPtr<xiiGALDynamicBuffer> xiiRenderDataManager::GetOrCreateInstanceDataAndFill(const xiiComponent& ownerComponent, bool bDynamic, const xiiTransform& globalTransform, xiiInstanceDataOffset& inout_instanceDataOffset, xiiUInt32 uiUniqueID /*= 0*/, const xiiColor& color /*= xiiColor::White*/) const
{
  xiiSharedPtr<xiiGALDynamicBuffer> pInstanceDataBuffer;
  auto                              pInstanceData = GetOrCreateInstanceData(&ownerComponent, bDynamic, pInstanceDataBuffer, inout_instanceDataOffset);
  FillPerInstanceData(pInstanceData[0], ownerComponent.GetOwner(), globalTransform, uiUniqueID, color);

  return pInstanceDataBuffer;
}

template <typename T>
XII_ALWAYS_INLINE xiiArrayPtr<T> xiiRenderDataManager::GetOrCreateCustomInstanceData(xiiUInt32 uiCustomDataIndex, const xiiComponent* pOwnerComponent, xiiSharedPtr<xiiGALDynamicBuffer>& out_hBuffer, xiiCustomInstanceDataOffset& inout_instanceDataOffset, xiiUInt32 uiCount /*= 1*/) const
{
  xiiByteArrayPtr pData = GetOrCreateCustomInstanceData(uiCustomDataIndex, sizeof(T), pOwnerComponent, out_hBuffer, inout_instanceDataOffset, uiCount);
  return xiiArrayPtr<T>(reinterpret_cast<T*>(pData.GetPtr()), pData.GetCount() / sizeof(T));
}

template <typename T>
XII_FORCE_INLINE xiiSharedPtr<xiiGALDynamicBuffer> xiiRenderDataManager::GetOrCreateCustomInstanceDataAndFill(xiiUInt32 uiCustomDataIndex, const xiiComponent& ownerComponent, xiiCustomInstanceDataOffset& inout_instanceDataOffset, const T& data) const
{
  xiiSharedPtr<xiiGALDynamicBuffer> pInstanceDataBuffer;
  auto                              pInstanceData = GetOrCreateCustomInstanceData<T>(uiCustomDataIndex, &ownerComponent, pInstanceDataBuffer, inout_instanceDataOffset);
  pInstanceData[0]                                = data;

  return pInstanceDataBuffer;
}

XII_ALWAYS_INLINE xiiSharedPtr<xiiGALDynamicBuffer> xiiRenderDataManager::GetCustomInstanceDataBuffer(xiiUInt32 uiCustomDataIndex) const
{
  return m_Buffers[uiCustomDataIndex];
}
