#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/Logging/Log.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

void xiiQtUiServices::MessageBoxStatus(const xiiStatus& s, xiiStringView sFailureMsg, xiiStringView sSuccessMsg, bool bOnlySuccessMsgIfDetails)
{
  xiiStringBuilder sResult;

  if (s.m_Result.Succeeded())
  {
    if (sSuccessMsg.IsEmpty())
      return;

    if (bOnlySuccessMsgIfDetails && s.m_sMessage.IsEmpty())
      return;

    sResult = sSuccessMsg;

    if (!s.m_sMessage.IsEmpty())
      sResult.AppendFormat("\n\nDetails:\n{0}", s.m_sMessage);

    MessageBoxInformation(sResult);
  }
  else
  {
    sResult = sFailureMsg;

    if (!s.m_sMessage.IsEmpty())
      sResult.AppendFormat("\n\nDetails:\n{0}", s.m_sMessage);

    MessageBoxWarning(sResult);
  }
}

void xiiQtUiServices::MessageBoxInformation(const xiiFormatString& msg)
{
  xiiStringBuilder tmp;

  if (s_bHeadless)
    xiiLog::Info(msg.GetText(tmp));
  else
  {
    QMessageBox::information(QApplication::activeWindow(), xiiApplication::GetApplicationInstance()->GetApplicationName().GetData(), QString::fromUtf8(msg.GetTextCStr(tmp)), QMessageBox::StandardButton::Ok);
  }
}

void xiiQtUiServices::MessageBoxWarning(const xiiFormatString& msg)
{
  xiiStringBuilder tmp;

  if (s_bHeadless)
    xiiLog::Warning(msg.GetText(tmp));
  else
  {
    QMessageBox::warning(QApplication::activeWindow(), xiiApplication::GetApplicationInstance()->GetApplicationName().GetData(), QString::fromUtf8(msg.GetTextCStr(tmp)), QMessageBox::StandardButton::Ok);
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
    xiiStringBuilder tmp;

    return QMessageBox::question(QApplication::activeWindow(), xiiApplication::GetApplicationInstance()->GetApplicationName().GetData(), QString::fromUtf8(msg.GetTextCStr(tmp)), buttons, defaultButton);
  }
}
