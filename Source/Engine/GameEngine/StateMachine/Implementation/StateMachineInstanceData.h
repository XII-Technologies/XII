#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Foundation/Containers/Blob.h>
#include <Foundation/Containers/SmallArray.h>

/// \brief Structure to describe an instance data type for a state or transition.
///
/// Since state machine states and transitions are shared between instances they can't hold their state in member
/// variables. This structure describes the type of instance data necessary for a state or transition.
/// Instance data is then automatically allocated by the state machine instance and passed via the pInstanceData pointer
/// in the state or transition functions.
/// Use the templated Fill() method to fill the desc from an instance data type.
struct xiiStateMachineInstanceDataDesc
{
  xiiUInt32                           m_uiTypeSize          = 0;
  xiiUInt32                           m_uiTypeAlignment     = 0;
  xiiMemoryUtils::ConstructorFunction m_ConstructorFunction = nullptr;
  xiiMemoryUtils::DestructorFunction  m_DestructorFunction  = nullptr;

  template <typename T>
  XII_ALWAYS_INLINE void FillFromType()
  {
    m_uiTypeSize          = sizeof(T);
    m_uiTypeAlignment     = XII_ALIGNMENT_OF(T);
    m_ConstructorFunction = xiiMemoryUtils::MakeConstructorFunction<T>();
    m_DestructorFunction  = xiiMemoryUtils::MakeDestructorFunction<T>();
  }
};

namespace xiiStateMachineInternal
{
  /// \brief Helper class to manager instance data allocation, construction and destruction
  class XII_GAMEENGINE_DLL InstanceDataAllocator
  {
  public:
    /// \brief Adds the given desc to internal list of data that needs to be allocated and returns the byte offset.
    xiiUInt32 AddDesc(const xiiStateMachineInstanceDataDesc& desc);

    void ClearDescs();

    void Construct(const xiiByteBlobPtr& blobPtr) const;
    void Destruct(const xiiByteBlobPtr& blobPtr) const;

    xiiBlob AllocateAndConstruct() const;
    void    DestructAndDeallocate(xiiBlob& blob) const;

    xiiUInt32 GetTotalDataSize() const { return m_uiTotalDataSize; }

    XII_ALWAYS_INLINE static void* GetInstanceData(const xiiByteBlobPtr& blobPtr, xiiUInt32 uiOffset)
    {
      return (uiOffset != xiiInvalidIndex) ? blobPtr.GetPtr() + uiOffset : nullptr;
    }

  private:
    xiiDynamicArray<xiiStateMachineInstanceDataDesc> m_Descs;
    xiiUInt32                                        m_uiTotalDataSize = 0;
  };

  /// \brief Helper class to manage instance data for compound states or transitions
  struct XII_GAMEENGINE_DLL Compound
  {
    XII_ALWAYS_INLINE xiiUInt32 GetBaseOffset() const { return m_InstanceDataOffsets.GetUserData<xiiUInt32>(); }
    XII_ALWAYS_INLINE xiiUInt32 GetDataSize() const { return m_InstanceDataAllocator.GetTotalDataSize(); }

    xiiSmallArray<xiiUInt32, 2> m_InstanceDataOffsets;
    InstanceDataAllocator       m_InstanceDataAllocator;

    struct InstanceData
    {
      const Compound* m_pOwner = nullptr;

      ~InstanceData()
      {
        if (m_pOwner != nullptr)
        {
          m_pOwner->m_InstanceDataAllocator.Destruct(GetBlobPtr());
        }
      }

      XII_ALWAYS_INLINE xiiByteBlobPtr GetBlobPtr()
      {
        return xiiByteBlobPtr(xiiMemoryUtils::AddByteOffset(reinterpret_cast<xiiUInt8*>(this), m_pOwner->GetBaseOffset()), m_pOwner->GetDataSize());
      }
    };

    XII_ALWAYS_INLINE void* GetSubInstanceData(InstanceData* pData, xiiUInt32 index) const
    {
      return pData != nullptr ? m_InstanceDataAllocator.GetInstanceData(pData->GetBlobPtr(), m_InstanceDataOffsets[index]) : nullptr;
    }

    XII_FORCE_INLINE void Initialize(InstanceData* pData) const
    {
      if (pData != nullptr && pData->m_pOwner == nullptr)
      {
        pData->m_pOwner = this;
        m_InstanceDataAllocator.Construct(pData->GetBlobPtr());
      }
    }

    template <typename T>
    bool GetInstanceDataDesc(xiiArrayPtr<T*> subObjects, xiiStateMachineInstanceDataDesc& out_desc)
    {
      m_InstanceDataOffsets.Clear();
      m_InstanceDataAllocator.ClearDescs();

      xiiUInt32 uiMaxAlignment = 0;

      xiiStateMachineInstanceDataDesc instanceDataDesc;
      for (T* pSubObject : subObjects)
      {
        xiiUInt32 uiOffset = xiiInvalidIndex;
        if (pSubObject->GetInstanceDataDesc(instanceDataDesc))
        {
          uiOffset       = m_InstanceDataAllocator.AddDesc(instanceDataDesc);
          uiMaxAlignment = xiiMath::Max(uiMaxAlignment, instanceDataDesc.m_uiTypeAlignment);
        }
        m_InstanceDataOffsets.PushBack(uiOffset);
      }

      if (uiMaxAlignment > 0)
      {
        out_desc.FillFromType<InstanceData>();
        out_desc.m_ConstructorFunction = nullptr; // not needed, instance data is constructed on first OnEnter

        xiiUInt32 uiBaseOffset                         = xiiMemoryUtils::AlignSize(out_desc.m_uiTypeSize, uiMaxAlignment);
        m_InstanceDataOffsets.GetUserData<xiiUInt32>() = uiBaseOffset;

        out_desc.m_uiTypeSize      = uiBaseOffset + m_InstanceDataAllocator.GetTotalDataSize();
        out_desc.m_uiTypeAlignment = xiiMath::Max(out_desc.m_uiTypeAlignment, uiMaxAlignment);

        return true;
      }

      return false;
    }
  };
} // namespace xiiStateMachineInternal
