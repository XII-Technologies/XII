#include <Core/CorePCH.h>

#include <Core/Messages/DeleteObjectMessage.h>
#include <Core/World/World.h>
#include <Foundation/Types/VariantTypeRegistry.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGameObjectHandle, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGameObjectHandle>)
XII_END_STATIC_REFLECTED_TYPE;
XII_DEFINE_CUSTOM_VARIANT_TYPE(xiiGameObjectHandle);

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiComponentHandle, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiComponentHandle>)
XII_END_STATIC_REFLECTED_TYPE;
XII_DEFINE_CUSTOM_VARIANT_TYPE(xiiComponentHandle);

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiObjectMode, 1)
  XII_ENUM_CONSTANTS(xiiObjectMode::Automatic, xiiObjectMode::ForceDynamic)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiComponentMode, 1)
  XII_ENUM_CONSTANTS(xiiComponentMode::Static, xiiComponentMode::Dynamic)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiObjectMsgQueueType, 1)
  XII_ENUM_CONSTANTS(xiiObjectMsgQueueType::PostAsync, xiiObjectMsgQueueType::PostTransform, xiiObjectMsgQueueType::NextFrame, xiiObjectMsgQueueType::AfterInitialized)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiOnComponentFinishedAction, 1)
  XII_ENUM_CONSTANTS(xiiOnComponentFinishedAction::None, xiiOnComponentFinishedAction::DeleteComponent, xiiOnComponentFinishedAction::DeleteGameObject)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiOnComponentFinishedAction2, 1)
  XII_ENUM_CONSTANTS(xiiOnComponentFinishedAction2::None, xiiOnComponentFinishedAction2::DeleteComponent, xiiOnComponentFinishedAction2::DeleteGameObject, xiiOnComponentFinishedAction2::Restart)
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

//////////////////////////////////////////////////////////////////////////

void operator<<(xiiStreamWriter& ref_stream, const xiiGameObjectHandle& hValue)
{
  XII_IGNORE_UNUSED(ref_stream);
  XII_IGNORE_UNUSED(hValue);
  XII_ASSERT_DEV(false, "This function should not be called. Use xiiWorldWriter::WriteGameObjectHandle instead.");
}

void operator>>(xiiStreamReader& ref_stream, xiiGameObjectHandle& ref_hValue)
{
  XII_IGNORE_UNUSED(ref_stream);
  XII_IGNORE_UNUSED(ref_hValue);
  XII_ASSERT_DEV(false, "This function should not be called. Use xiiWorldReader::ReadGameObjectHandle instead.");
}

void operator<<(xiiStreamWriter& ref_stream, const xiiComponentHandle& hValue)
{
  XII_IGNORE_UNUSED(ref_stream);
  XII_IGNORE_UNUSED(hValue);
  XII_ASSERT_DEV(false, "This function should not be called. Use xiiWorldWriter::WriteComponentHandle instead.");
}

void operator>>(xiiStreamReader& ref_stream, xiiComponentHandle& ref_hValue)
{
  XII_IGNORE_UNUSED(ref_stream);
  XII_IGNORE_UNUSED(ref_hValue);
  XII_ASSERT_DEV(false, "This function should not be called. Use xiiWorldReader::ReadComponentHandle instead.");
}

//////////////////////////////////////////////////////////////////////////

namespace
{
  template <typename T>
  void HandleFinishedActionImpl(xiiComponent* pComponent, typename T::Enum action)
  {
    if (action == T::DeleteGameObject)
    {
      // Send a message to the owner object to check whether another component wants to delete this object later.
      // Can't use xiiGameObject::SendMessage because the object would immediately delete itself and furthermore the sender component needs to be
      // filtered out here.
      xiiMsgDeleteGameObject msg;

      for (xiiComponent* pComp : pComponent->GetOwner()->GetComponents())
      {
        if (pComp == pComponent)
          continue;

        pComp->SendMessage(msg);
        if (msg.m_bCancel)
        {
          action = T::DeleteComponent;
          break;
        }
      }

      if (action == T::DeleteGameObject)
      {
        pComponent->GetWorld()->DeleteObjectDelayed(pComponent->GetOwner()->GetHandle());
        return;
      }
    }

    if (action == T::DeleteComponent)
    {
      pComponent->DeleteComponent();
    }
  }

  template <typename T>
  void HandleDeleteObjectMsgImpl(xiiMsgDeleteGameObject& ref_msg, xiiEnum<T>& ref_action)
  {
    if (ref_action == T::DeleteComponent)
    {
      ref_msg.m_bCancel = true;
      ref_action        = T::DeleteGameObject;
    }
    else if (ref_action == T::DeleteGameObject)
    {
      ref_msg.m_bCancel = true;
    }
  }
} // namespace

//////////////////////////////////////////////////////////////////////////

void xiiOnComponentFinishedAction::HandleFinishedAction(xiiComponent* pComponent, xiiOnComponentFinishedAction::Enum action)
{
  HandleFinishedActionImpl<xiiOnComponentFinishedAction>(pComponent, action);
}

void xiiOnComponentFinishedAction::HandleDeleteObjectMsg(xiiMsgDeleteGameObject& ref_msg, xiiEnum<xiiOnComponentFinishedAction>& ref_action)
{
  HandleDeleteObjectMsgImpl(ref_msg, ref_action);
}

//////////////////////////////////////////////////////////////////////////

void xiiOnComponentFinishedAction2::HandleFinishedAction(xiiComponent* pComponent, xiiOnComponentFinishedAction2::Enum action)
{
  HandleFinishedActionImpl<xiiOnComponentFinishedAction2>(pComponent, action);
}

void xiiOnComponentFinishedAction2::HandleDeleteObjectMsg(xiiMsgDeleteGameObject& ref_msg, xiiEnum<xiiOnComponentFinishedAction2>& ref_action)
{
  HandleDeleteObjectMsgImpl(ref_msg, ref_action);
}

XII_STATICLINK_FILE(Core, Core_World_Implementation_Declarations);
