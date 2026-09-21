/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Foundation/Containers/Blob.h>
#include <Foundation/Containers/SmallArray.h>
#include <Foundation/Memory/InstanceDataAllocator.h>

namespace xiiStateMachineInternal
{
  /// Helper class to manage instance data for compound states or transitions
  struct XII_GAMEENGINE_DLL Compound
  {
    XII_ALWAYS_INLINE xiiUInt32 GetBaseOffset() const { return m_InstanceDataOffsets.GetUserData<xiiUInt32>(); }
    XII_ALWAYS_INLINE xiiUInt32 GetDataSize() const { return m_InstanceDataAllocator.GetTotalDataSize(); }

    xiiSmallArray<xiiUInt32, 2> m_InstanceDataOffsets;
    xiiInstanceDataAllocator    m_InstanceDataAllocator;

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

    XII_ALWAYS_INLINE void* GetSubInstanceData(InstanceData* pData, xiiUInt32 uiIndex) const
    {
      return pData != nullptr ? m_InstanceDataAllocator.GetInstanceData(pData->GetBlobPtr(), m_InstanceDataOffsets[uiIndex]) : nullptr;
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
    bool GetInstanceDataDesc(xiiArrayPtr<T*> subObjects, xiiInstanceDataDesc& out_desc)
    {
      m_InstanceDataOffsets.Clear();
      m_InstanceDataAllocator.ClearDescs();

      xiiUInt32 uiMaxAlignment = 0;

      xiiInstanceDataDesc instanceDataDesc;
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
