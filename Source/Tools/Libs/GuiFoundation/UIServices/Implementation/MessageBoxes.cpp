/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/Logging/Log.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

void xiiQtUiServices::MessageBoxStatus(const xiiStatus& s, xiiStringView sFailureMsg, xiiStringView sSuccessMsg, bool bOnlySuccessMsgIfDetails)
{
  xiiStringBuilder sResult;

  if (s.Succeeded())
  {
    if (sSuccessMsg.IsEmpty())
      return;

    if (bOnlySuccessMsgIfDetails && s.GetMessageString().IsEmpty())
      return;

    sResult = sSuccessMsg;

    if (!s.GetMessageString().IsEmpty())
    {
      sResult.AppendFormat("\n\nDetails:\n{0}", s.GetMessageString());
    }

    MessageBoxInformation(sResult);
  }
  else
  {
    sResult = sFailureMsg;

    if (!s.GetMessageString().IsEmpty())
    {
      sResult.AppendFormat("\n\nDetails:\n{0}", s.GetMessageString());
    }

    MessageBoxWarning(sResult);
  }
}

void xiiQtUiServices::MessageBoxInformation(const xiiFormatString& msg)
{
  xiiStringBuilder sTmp;

  if (s_bHeadless)
  {
    xiiLog::Info(msg.GetText(sTmp));
  }
  else
  {
    QMessageBox::information(QApplication::activeWindow(), xiiApplication::GetApplicationInstance()->GetApplicationName().GetData(), QString::fromUtf8(msg.GetTextCStr(sTmp)), QMessageBox::StandardButton::Ok);
  }
}

void xiiQtUiServices::MessageBoxWarning(const xiiFormatString& msg)
{
  xiiStringBuilder sTmp;

  if (s_bHeadless)
    xiiLog::Warning(msg.GetText(sTmp));
  else
  {
    QMessageBox::warning(QApplication::activeWindow(), xiiApplication::GetApplicationInstance()->GetApplicationName().GetData(), QString::fromUtf8(msg.GetTextCStr(sTmp)), QMessageBox::StandardButton::Ok);
  }
}

QMessageBox::StandardButton xiiQtUiServices::MessageBoxQuestion(const xiiFormatString& msg, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton)
{
  if (s_bHeadless)
  {
    return defaultButton;
  }
  else
  {
    xiiStringBuilder sTmp;

    return QMessageBox::question(QApplication::activeWindow(), xiiApplication::GetApplicationInstance()->GetApplicationName().GetData(), QString::fromUtf8(msg.GetTextCStr(sTmp)), buttons, defaultButton);
  }
}
