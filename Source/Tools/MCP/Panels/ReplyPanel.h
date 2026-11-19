/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#ifdef XII_USE_QT

#  include <Foundation/Containers/Deque.h>
#  include <Foundation/Strings/String.h>
#  include <Foundation/Types/Variant.h>
#  include <QWidget>

class QComboBox;
class xiiMCPJsonView;

class xiiMCPReplyPanel final : public QWidget
{
public:
  xiiMCPReplyPanel();

  void AppendReplies(const xiiDeque<xiiString>& replies);

private:
  struct Reply
  {
    xiiString  m_sResponse;
    xiiVariant m_Payload;
    bool       m_bHasJson = false;
  };

  void ShowReply(int iIndex);

  xiiDeque<Reply> m_Replies;
  QComboBox*      m_pHistory = nullptr;
  xiiMCPJsonView* m_pView    = nullptr;
};

#endif
