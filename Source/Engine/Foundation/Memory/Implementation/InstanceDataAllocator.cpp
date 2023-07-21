#include <Foundation/FoundationPCH.h>

#include <Foundation/Memory/InstanceDataAllocator.h>

xiiUInt32 xiiInstanceDataAllocator::AddDesc(const xiiInstanceDataDesc& desc)
{
  m_Descs.PushBack(desc);

  const xiiUInt32 uiOffset = xiiMemoryUtils::AlignSize(m_uiTotalDataSize, desc.m_uiTypeAlignment);
  m_uiTotalDataSize       = uiOffset + desc.m_uiTypeSize;

  return uiOffset;
}

void xiiInstanceDataAllocator::ClearDescs()
{
  m_Descs.Clear();
  m_uiTotalDataSize = 0;
}

xiiBlob xiiInstanceDataAllocator::AllocateAndConstruct() const
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

void xiiInstanceDataAllocator::DestructAndDeallocate(xiiBlob& ref_blob) const
{
  XII_ASSERT_DEV(ref_blob.GetByteBlobPtr().GetCount() == m_uiTotalDataSize, "Passed blob has not the expected size");

  Destruct(ref_blob.GetByteBlobPtr());

  ref_blob.Clear();
}

void xiiInstanceDataAllocator::Construct(xiiByteBlobPtr blobPtr) const
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

void xiiInstanceDataAllocator::Destruct(xiiByteBlobPtr blobPtr) const
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

XII_STATICLINK_FILE(Foundation, Foundation_Memory_Implementation_InstanceDataAllocator);
