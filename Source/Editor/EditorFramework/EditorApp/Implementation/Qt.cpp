#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/Scripting/LuaWrapper.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Logging/Log.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageUtils.h>

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
  QPalette palette;

  xiiColorGammaUB highlightColor         = xiiColorScheme::DarkUI(xiiColorScheme::Yellow);
  xiiColorGammaUB highlightColorDisabled = xiiColorScheme::DarkUI(xiiColorScheme::Yellow) * 0.5f;
  xiiColorGammaUB linkVisitedColor       = xiiColorScheme::LightUI(xiiColorScheme::Yellow);

  QApplication::setStyle(QStyleFactory::create("fusion"));

  // Base surfaces
  palette.setColor(QPalette::Window, QColor(28, 28, 30));        // Main window background
  palette.setColor(QPalette::Base, QColor(18, 18, 20));          // Input fields, scene graph
  palette.setColor(QPalette::AlternateBase, QColor(36, 36, 38)); // Alternating rows
  palette.setColor(QPalette::Shadow, QColor(0, 0, 0));           // Property grid arrays

  // Text & foreground
  palette.setColor(QPalette::WindowText, QColor(220, 220, 220));
  palette.setColor(QPalette::Text, QColor(220, 220, 220));
  palette.setColor(QPalette::BrightText, QColor(255, 85, 85)); // Alerts or emphasis
  palette.setColor(QPalette::ButtonText, QColor(220, 220, 220));
  palette.setColor(QPalette::PlaceholderText, QColor(140, 140, 140));

  // Buttons & controls
  palette.setColor(QPalette::Button, QColor(40, 40, 42)); // Toolbuttons, dashboard
  palette.setColor(QPalette::Light, QColor(60, 60, 60));  // Tab lines, gradients
  palette.setColor(QPalette::Midlight, QColor(55, 55, 55));
  palette.setColor(QPalette::Dark, QColor(35, 35, 35)); // Underlines, separators
  palette.setColor(QPalette::Mid, QColor(45, 45, 45));  // Group box outlines

  // Highlights & links
  palette.setColor(QPalette::Highlight, QColor(0, 122, 204)); // Selection blue
  palette.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
  palette.setColor(QPalette::Link, QColor(0, 122, 204));
  palette.setColor(QPalette::LinkVisited, QColor(128, 100, 162));

  // Tooltips
  palette.setColor(QPalette::ToolTipBase, QColor(255, 255, 240));
  palette.setColor(QPalette::ToolTipText, QColor(0, 0, 0));

  // Disabled state
  palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(128, 128, 128));
  palette.setColor(QPalette::Disabled, QPalette::Button, QColor(35, 35, 35));
  palette.setColor(QPalette::Disabled, QPalette::Text, QColor(105, 105, 105));
  palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(128, 128, 128));
  palette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(70, 90, 110));
  palette.setColor(QPalette::Disabled, QPalette::BrightText, QColor(255, 255, 255));

  // NoRole fallback
  palette.setBrush(QPalette::NoRole, QBrush(QColor(0, 0, 0), Qt::NoBrush));

#ifdef BUILDSYSTEM_ENABLE_LUA_SUPPORT
  if (false)
  {
    // when enabled, you can edit the palette with a Lua file
    // see xiiProjectAction::Execute(), case xiiProjectAction::ButtonType::ReloadResources:
    // to enable reloading on "reload resources"
    // Example Lua file:
    // SetColor(Base, 24, 24, 24)
    // SetDisabledColor(Base, 5, 5, 5)

    xiiOSFile file;
    if (file.Open("D:\\Style.lua", xiiFileOpenMode::Read).Succeeded())
    {
      xiiDataBuffer content;
      file.ReadAll(content);
      content.PushBack('\0');

      xiiLuaWrapper lua;
      lua.SetVariable("WindowText", QPalette::WindowText);
      lua.SetVariable("Button", QPalette::Button);
      lua.SetVariable("Light", QPalette::Light);
      lua.SetVariable("Midlight", QPalette::Midlight);
      lua.SetVariable("Dark", QPalette::Dark);
      lua.SetVariable("Mid", QPalette::Mid);
      lua.SetVariable("Text", QPalette::Text);
      lua.SetVariable("BrightText", QPalette::BrightText);
      lua.SetVariable("ButtonText", QPalette::ButtonText);
      lua.SetVariable("Base", QPalette::Base);
      lua.SetVariable("Window", QPalette::Window);
      lua.SetVariable("Shadow", QPalette::Shadow);
      lua.SetVariable("Highlight", QPalette::Highlight);
      lua.SetVariable("HighlightedText", QPalette::HighlightedText);
      lua.SetVariable("Link", QPalette::Link);
      lua.SetVariable("LinkVisited", QPalette::LinkVisited);
      lua.SetVariable("AlternateBase", QPalette::AlternateBase);
      lua.SetVariable("ToolTipBase", QPalette::ToolTipBase);
      lua.SetVariable("ToolTipText", QPalette::ToolTipText);
      lua.SetVariable("PlaceholderText", QPalette::PlaceholderText);
      lua.RegisterCFunction("SetColor", lua_SetColor, &palette);
      lua.RegisterCFunction("SetDisabledColor", lua_SetDisabledColor, &palette);

      lua.ExecuteString((const char*)content.GetData(), "", xiiLog::GetThreadLocalLogSystem()).IgnoreResult();
    }
  }
#endif

  QApplication::setPalette(palette);
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
