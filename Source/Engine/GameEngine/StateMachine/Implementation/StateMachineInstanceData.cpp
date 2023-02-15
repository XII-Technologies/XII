#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/StateMachine/Implementation/StateMachineInstanceData.h>

namespace xiiStateMachineInternal
{
  xiiUInt32 InstanceDataAllocator::AddDesc(const xiiStateMachineInstanceDataDesc& desc)
  {
    m_Descs.PushBack(desc);

    const xiiUInt32 uiOffset = xiiMemoryUtils::AlignSize(m_uiTotalDataSize, desc.m_uiTypeAlignment);
    m_uiTotalDataSize        = uiOffset + desc.m_uiTypeSize;

    return uiOffset;
  }

  void InstanceDataAllocator::ClearDescs()
  {
    m_Descs.Clear();
    m_uiTotalDataSize = 0;
  }

  xiiBlob InstanceDataAllocator::AllocateAndConstruct() const
  {
    xiiBlob blob;
    if (m_uiTotalDataSize > 0)
    {
      blob.SetCountUninitialized(m_uiTotalDataSize);
      blob.ZeroFill();

      Construct(blob.GetByteBlobPtr());
    }

    return blob;
  }

  void InstanceDataAllocator::DestructAndDeallocate(xiiBlob& blob) const
  {
    XII_ASSERT_DEV(blob.GetByteBlobPtr().GetCount() == m_uiTotalDataSize, "Passed blob has not the expected size");
    Destruct(blob.GetByteBlobPtr());

    blob.Clear();
  }

  void InstanceDataAllocator::Construct(const xiiByteBlobPtr& blobPtr) const
  {
    xiiUInt32 uiOffset = 0;
    for (auto& desc : m_Descs)
    {
      uiOffset = xiiMemoryUtils::AlignSize(uiOffset, desc.m_uiTypeAlignment);

      if (desc.m_ConstructorFunction != nullptr)
      {
        desc.m_ConstructorFunction(GetInstanceData(blobPtr, uiOffset));
      }

      uiOffset += desc.m_uiTypeSize;
    }
  }

  void InstanceDataAllocator::Destruct(const xiiByteBlobPtr& blobPtr) const
  {
    xiiUInt32 uiOffset = 0;
    for (auto& desc : m_Descs)
    {
      uiOffset = xiiMemoryUtils::AlignSize(uiOffset, desc.m_uiTypeAlignment);

      if (desc.m_DestructorFunction != nullptr)
      {
        desc.m_DestructorFunction(GetInstanceData(blobPtr, uiOffset));
      }

      uiOffset += desc.m_uiTypeSize;
    }
  }

} // namespace xiiStateMachineInternal


XII_STATICLINK_FILE(GameEngine, GameEngine_StateMachine_Implementation_StateMachineInstanceData);
