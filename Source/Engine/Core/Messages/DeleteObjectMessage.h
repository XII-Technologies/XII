/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Communication/Message.h>

struct XII_CORE_DLL xiiMsgDeleteGameObject : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgDeleteGameObject, xiiMessage);

  /// If set to true, any parent/ancestor that has no other children or components will also be deleted.
  bool m_bDeleteEmptyParents = true;

  /// This is used by xiiOnComponentFinishedAction to orchestrate when an object shall really be deleted.
  bool m_bCancel = false;
};
