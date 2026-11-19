/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MCP/MCPPCH.h>

#ifdef XII_USE_QT

#  include <Foundation/Containers/Deque.h>
#  include <Foundation/Math/Math.h>
#  include <Foundation/Strings/String.h>
#  include <Foundation/Strings/StringBuilder.h>
#  include <Foundation/Strings/StringView.h>
#  include <Foundation/Types/Types.h>
#  include <Foundation/Types/Variant.h>
#  include <MCP/Panels/JsonView.h>
#  include <MCP/Panels/ReplyPanel.h>
#  include <MCPPlugin/Json.h>

#  include <QComboBox>
#  include <QSignalBlocker>
#  include <QVBoxLayout>
#  include <utility>

namespace
{
  constexpr xiiUInt32 MaximumReplies = 16;

  QString ReplyText(xiiStringView sText)
  {
    return QString::fromUtf8(sText.GetStartPointer(), static_cast<qsizetype>(sText.GetElementCount()));
  }

  xiiVariant ReplyPayload(const xiiVariant& response, QString& ref_label)
  {
    if (!response.IsA<xiiVariantDictionary>())
    {
      return response;
    }

    const auto& envelope = response.Get<xiiVariantDictionary>();
    if (const auto* pId = envelope.GetValue("id"))
    {
      xiiStringBuilder sId;
      if (pId->IsValid() && xiiMCPJson::Write(*pId, sId).Succeeded())
      {
        ref_label = "Request " + ReplyText(sId);
      }
    }
    if (const auto* pError = envelope.GetValue("error"))
    {
      ref_label += QStringLiteral(" · Error");
      return *pError;
    }
    const auto* pResult = envelope.GetValue("result");
    if (pResult == nullptr)
    {
      return response;
    }
    if (!pResult->IsA<xiiVariantDictionary>())
    {
      return *pResult;
    }

    const auto& result = pResult->Get<xiiVariantDictionary>();
    if (const auto* pError = result.GetValue("isError"); pError != nullptr && pError->IsA<bool>() && pError->Get<bool>())
    {
      ref_label += QStringLiteral(" · Error");
    }
    if (const auto* pStructured = result.GetValue("structuredContent"))
    {
      return *pStructured;
    }

    const auto* pContents = result.GetValue("contents");
    if (pContents != nullptr && pContents->IsA<xiiVariantArray>())
    {
      const auto& contents = pContents->Get<xiiVariantArray>();
      if (contents.GetCount() == 1 && contents[0].IsA<xiiVariantDictionary>())
      {
        const auto& resource = contents[0].Get<xiiVariantDictionary>();
        xiiVariant  value;
        if (xiiMCPJson::GetString(resource, "mimeType") == "application/json" &&
            xiiMCPJson::Parse(xiiMCPJson::GetString(resource, "text"), value).Succeeded())
        {
          return value;
        }
      }
    }
    return *pResult;
  }
} // namespace

xiiMCPReplyPanel::xiiMCPReplyPanel() :
  m_pHistory(new QComboBox(this)),
  m_pView(new xiiMCPJsonView("Result"))
{
  auto* pLayout = new QVBoxLayout(this);
  pLayout->setContentsMargins(0, 0, 0, 0);
  m_pHistory->setAccessibleName("Response history");
  m_pHistory->setPlaceholderText("Response history");
  m_pHistory->setEnabled(false);
  m_pHistory->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
  pLayout->addWidget(m_pHistory);

  m_pView->Clear("Invoke a capability to view its reply.");
  pLayout->addWidget(m_pView, 1);
  connect(m_pHistory, &QComboBox::currentIndexChanged, this, &xiiMCPReplyPanel::ShowReply);
}

void xiiMCPReplyPanel::AppendReplies(const xiiDeque<xiiString>& replies)
{
  if (replies.IsEmpty())
  {
    return;
  }

  const QSignalBlocker blocker(m_pHistory);
  const int            iPrevious     = m_pHistory->currentIndex();
  const bool           bFollowLatest = iPrevious < 0 || iPrevious == m_pHistory->count() - 1;
  int                  iRemoved      = 0;
  for (const auto& sResponse : replies)
  {
    if (m_Replies.GetCount() == MaximumReplies)
    {
      m_Replies.PopFront();
      m_pHistory->removeItem(0);
      ++iRemoved;
    }

    Reply reply;
    reply.m_sResponse = sResponse;
    xiiVariant response;
    QString    label = "Gateway reply";
    reply.m_bHasJson = xiiMCPJson::Parse(sResponse, response).Succeeded();
    if (reply.m_bHasJson)
    {
      reply.m_Payload = ReplyPayload(response, label);
    }
    m_Replies.PushBack(std::move(reply));
    m_pHistory->addItem(label);
  }
  const int iSelected = bFollowLatest ? m_pHistory->count() - 1 : xiiMath::Max(0, iPrevious - iRemoved);
  m_pHistory->setEnabled(true);
  m_pHistory->setCurrentIndex(iSelected);
  if (bFollowLatest || iPrevious < iRemoved)
  {
    ShowReply(iSelected);
  }
}

void xiiMCPReplyPanel::ShowReply(int iIndex)
{
  if (iIndex < 0)
  {
    return;
  }

  const auto& reply = m_Replies[static_cast<xiiUInt32>(iIndex)];
  if (reply.m_bHasJson)
  {
    m_pView->SetDocument(reply.m_Payload, reply.m_sResponse);
  }
  else
  {
    m_pView->SetMessage(reply.m_sResponse.IsEmpty() ? QString("The gateway returned an empty reply.") : ReplyText(reply.m_sResponse));
  }
}

#endif
