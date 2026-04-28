/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/HTMLWriter.h>
#include <Foundation/Time/Timestamp.h>

xiiLogWriter::HTML::~HTML()
{
  EndLog();
}

void xiiLogWriter::HTML::BeginLog(xiiStringView sFile, xiiStringView sAppTitle)
{
  const xiiUInt32 uiLogCache = 1024 * 10;

  xiiStringBuilder sNewName;
  if (m_File.Open(sFile.GetData(sNewName), uiLogCache, xiiFileShareMode::SharedReads) == XII_FAILURE)
  {
    for (xiiUInt32 i = 1; i < 32; ++i)
    {
      const xiiStringBuilder sName = xiiPathUtils::GetFileName(sFile);

      sNewName.SetFormat("{0}_{1}", sName, i);

      xiiStringBuilder sPath = sFile;
      sPath.ChangeFileName(sNewName);

      if (m_File.Open(sPath.GetData(), uiLogCache) == XII_SUCCESS)
        break;
    }
  }

  if (!m_File.IsOpen())
  {
    xiiLog::Error("Could not open Log-File \"{0}\".", sFile);
    return;
  }

  xiiStringBuilder sText;
  sText.SetFormat("<HTML><HEAD><META HTTP-EQUIV=\"Content-Type\" content=\"text/html; charset=utf-8\"><TITLE>Log - {0}</TITLE></HEAD><BODY>", sAppTitle);

  m_File.WriteBytes(sText.GetData(), sizeof(char) * sText.GetElementCount()).IgnoreResult();
}

void xiiLogWriter::HTML::EndLog()
{
  if (!m_File.IsOpen())
    return;

  WriteString("", 0);
  WriteString("", 0);
  WriteString(" <<< HTML-Log End >>> ", 0);
  WriteString("", 0);
  WriteString("", 0);

  xiiStringBuilder sText;
  sText.SetFormat("</BODY></HTML>");

  m_File.WriteBytes(sText.GetData(), sizeof(char) * sText.GetElementCount()).IgnoreResult();

  m_File.Close();
}

const xiiFileWriter& xiiLogWriter::HTML::GetOpenedLogFile() const
{
  return m_File;
}

void xiiLogWriter::HTML::SetTimestampMode(xiiLog::TimestampMode mode)
{
  m_TimestampMode = mode;
}

void xiiLogWriter::HTML::LogMessageHandler(const xiiLoggingEventData& eventData)
{
  if (!m_File.IsOpen())
    return;

  xiiStringBuilder sOriginalText = eventData.m_sText;

  xiiStringBuilder sTag = eventData.m_sTag;

  // Cannot write <, > or & to HTML, must be escaped
  sOriginalText.ReplaceAll("&", "&amp;");
  sOriginalText.ReplaceAll("<", "&lt;");
  sOriginalText.ReplaceAll(">", "&gt;");
  sOriginalText.ReplaceAll("\n", "<br>\n");

  sTag.ReplaceAll("&", "&amp;");
  sTag.ReplaceAll("<", "&lt;");
  sTag.ReplaceAll(">", "&gt;");

  xiiStringBuilder sTimestamp;
  xiiLog::GenerateFormattedTimestamp(m_TimestampMode, sTimestamp);

  bool bFlushWriteCache = false;

  xiiStringBuilder sText;

  switch (eventData.m_EventType)
  {
    case xiiLogMsgType::Flush:
      bFlushWriteCache = true;
      break;

    case xiiLogMsgType::BeginGroup:
      sText.SetFormat("<br><font color=\"#8080FF\"><b> <<< <u>{0}</u> >>> </b> ({1}) </font><br><table width=100%% border=0><tr width=100%%><td "
                      "width=10></td><td width=*>\n",
                      sOriginalText, sTag);
      break;

    case xiiLogMsgType::EndGroup:
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
      sText.SetFormat("</td></tr></table><font color=\"#8080FF\"><b> <<< {0} ({1} sec)>>> </b></font><br><br>\n", sOriginalText, xiiArgF(eventData.m_fSeconds, 4));
#else
      sText.SetFormat("</td></tr></table><font color=\"#8080FF\"><b> <<< {0} ({1})>>> </b></font><br><br>\n", sOriginalText, "timing info not available");
#endif
      break;

    case xiiLogMsgType::ErrorMsg:
      bFlushWriteCache = true;
      sText.SetFormat("{0}<font color=\"#FF0000\"><b><u>Error:</u> {1}</b></font><br>\n", sTimestamp, sOriginalText);
      break;

    case xiiLogMsgType::SeriousWarningMsg:
      bFlushWriteCache = true;
      sText.SetFormat("{0}<font color=\"#FF4000\"><b><u>Seriously:</u> {1}</b></font><br>\n", sTimestamp, sOriginalText);
      break;

    case xiiLogMsgType::WarningMsg:
      sText.SetFormat("{0}<font color=\"#FF8000\"><u>Warning:</u> {1}</font><br>\n", sTimestamp, sOriginalText);
      break;

    case xiiLogMsgType::SuccessMsg:
      sText.SetFormat("{0}<font color=\"#009000\">{1}</font><br>\n", sTimestamp, sOriginalText);
      break;

    case xiiLogMsgType::InfoMsg:
      sText.SetFormat("{0}<font color=\"#000000\">{1}</font><br>\n", sTimestamp, sOriginalText);
      break;

    case xiiLogMsgType::DevMsg:
      sText.SetFormat("{0}<font color=\"#3030F0\">{1}</font><br>\n", sTimestamp, sOriginalText);
      break;

    case xiiLogMsgType::DebugMsg:
      sText.SetFormat("{0}<font color=\"#A000FF\">{1}</font><br>\n", sTimestamp, sOriginalText);
      break;

    default:
      sText.SetFormat("{0}<font color=\"#A0A0A0\">{1}</font><br>\n", sTimestamp, sOriginalText);

      xiiLog::Warning("Unknown Message Type {1}", eventData.m_EventType);
      break;
  }

  if (!sText.IsEmpty())
  {
    m_File.WriteBytes(sText.GetData(), sizeof(char) * sText.GetElementCount()).IgnoreResult();
  }

  if (bFlushWriteCache)
  {
    m_File.Flush().IgnoreResult();
  }
}

void xiiLogWriter::HTML::WriteString(xiiStringView sText, xiiUInt32 uiColor)
{
  xiiStringBuilder sTemp;
  sTemp.SetFormat("<font color=\"#{0}\">{1}</font>", xiiArgU(uiColor, 1, false, 16, true), sText);

  m_File.WriteBytes(sTemp.GetData(), sizeof(char) * sTemp.GetElementCount()).IgnoreResult();
}


XII_STATICLINK_FILE(Foundation, Foundation_Logging_Implementation_HTMLWriter);
