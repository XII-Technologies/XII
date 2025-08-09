#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/Scripting/LuaWrapper.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Logging/Log.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageUtils.h>

#include <GuiFoundation/Style/DarkEditorStyle.moc.h>

#ifdef BUILDSYSTEM_ENABLE_LUA_SUPPORT

int lua_SetColor(lua_State* s)
{
  xiiLuaWrapper lua(s);

  QPalette* palette = (QPalette*)lua.GetFunctionLightUserData();

  const int iColor = lua.GetIntParameter(0);
  const int r      = lua.GetIntParameter(1);
  const int g      = lua.GetIntParameter(2);
  const int b      = lua.GetIntParameter(3);

  const QPalette::ColorRole role = (QPalette::ColorRole)iColor;

  palette->setColor(role, QColor(r, g, b));

  return lua.ReturnToScript();
}

int lua_SetDisabledColor(lua_State* s)
{
  xiiLuaWrapper lua(s);

  QPalette* palette = (QPalette*)lua.GetFunctionLightUserData();

  const int iColor = lua.GetIntParameter(0);
  const int r      = lua.GetIntParameter(1);
  const int g      = lua.GetIntParameter(2);
  const int b      = lua.GetIntParameter(3);

  const QPalette::ColorRole role = (QPalette::ColorRole)iColor;

  palette->setColor(QPalette::Disabled, role, QColor(r, g, b));

  return lua.ReturnToScript();
}

#endif

void xiiQtEditorApp::SetStyleSheet()
{
  QApplication::setStyle(new xiiQtDarkEditorStyle);
}

static void QtDebugMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& sQMsg)
{
  QByteArray       localMsg = sQMsg.toUtf8();
  xiiStringBuilder sMsg     = localMsg.constData();

  switch (type)
  {
    case QtDebugMsg:
      xiiLog::Debug("|Qt| {0} ({1}:{2}, {3})", sMsg, context.file, context.line, context.function);
      break;
#if QT_VERSION >= 0x050500
    case QtInfoMsg:
      xiiLog::Info("|Qt| {0} ({1}:{2}, {3})", sMsg, context.file, context.line, context.function);
      break;
#endif
    case QtWarningMsg:
    {
      // I just hate this pointless message
      if (sMsg.FindSubString("iCCP") != nullptr)
        return;

      xiiLog::Warning("|Qt| {0} ({1}:{2}, {3})", sMsg, context.file, context.line, context.function);
      break;
    }
    case QtCriticalMsg:
      // BUG in Qt 6 on Windows. Window classes are not properly unregistered so they leak into the next session and cause a warning.
      if (!sMsg.StartsWith("QApplication::regClass: Registering window class"))
      {
        xiiLog::Error("|Qt| {0} ({1}:{2}, {3})", sMsg, context.file, context.line, context.function);
      }
      break;
    case QtFatalMsg:
      XII_ASSERT_DEBUG("|Qt| {0} ({1}:{2} {3})", sMsg, context.file, context.line, context.function);
      break;
  }
}

void xiiQtEditorApp::InitQt(int iArgc, char** pArgv)
{
  qInstallMessageHandler(QtDebugMessageHandler);

  if (qApp != nullptr)
  {
    m_pQtApplication = qApp;
    bool      ok     = false;
    const int iCount = m_pQtApplication->property("Shared").toInt(&ok);
    XII_ASSERT_DEV(ok, "Existing QApplication was not constructed by XII!");
    m_pQtApplication->setProperty("Shared", QVariant::fromValue(iCount + 1));
  }
  else
  {
    m_iArgc          = iArgc;
    m_pQtApplication = new QApplication(m_iArgc, pArgv);
    m_pQtApplication->setProperty("Shared", QVariant::fromValue((int)1));

    // Locale fixes required by various third party libraries like RmlGui.
    QLocale::setDefault(QLocale::C);
    const char* locales[] = {"C.UTF-8", "C.utf8", "UTF-8"};
    for (const char* szLocale : locales)
    {
      if (setlocale(LC_ALL, szLocale) != nullptr)
        break;
    }
  }
}

void xiiQtEditorApp::DeInitQt()
{
  const int iCount = m_pQtApplication->property("Shared").toInt();
  if (iCount == 1)
  {
    delete m_pQtApplication;
  }
  else
  {
    m_pQtApplication->setProperty("Shared", QVariant::fromValue(iCount - 1));
  }
}
