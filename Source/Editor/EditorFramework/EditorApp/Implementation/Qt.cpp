#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/Scripting/LuaWrapper.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Logging/Log.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageUtils.h>

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

void xiiQtEditorApp::SetStyleSheet()
{
  QPalette palette;

  xiiColorGammaUB highlightColor         = xiiColorScheme::DarkUI(xiiColorScheme::Yellow);
  xiiColorGammaUB highlightColorDisabled = xiiColorScheme::DarkUI(xiiColorScheme::Yellow) * 0.5f;
  xiiColorGammaUB linkVisitedColor       = xiiColorScheme::LightUI(xiiColorScheme::Yellow);

  QApplication::setStyle(QStyleFactory::create("fusion"));

  palette.setColor(QPalette::WindowText, QColor(200, 200, 200, 255));
  palette.setColor(QPalette::Button, QColor(0, 0, 0, 255));      // buttons, toolbuttons, dashboard background
  palette.setColor(QPalette::Light, QColor(60, 60, 60, 255));    // lines between tabs, inactive tab gradient
  palette.setColor(QPalette::Midlight, QColor(59, 59, 59, 255)); // unused ?
  palette.setColor(QPalette::Dark, QColor(45, 45, 45, 255));     // line below active window highlight
  palette.setColor(QPalette::Mid, QColor(45, 45, 45, 255));      // color of the box around component properties (collapsible group box)
  palette.setColor(QPalette::Text, QColor(200, 200, 200, 255));  // scene graph, values in spin boxes, checkmarks
  palette.setColor(QPalette::BrightText, QColor(37, 37, 37, 255));
  palette.setColor(QPalette::ButtonText, QColor(200, 200, 200, 255));      // // menus, comboboxes, headers
  palette.setColor(QPalette::Base, QColor(20, 20, 20, 255));               // background inside complex windows (scenegraph)
  palette.setColor(QPalette::AlternateBase, QColor(20, 20, 20, 255));      // second base color, mainly used for alternate row colors
  palette.setColor(QPalette::Window, QColor(30, 30, 30, 255));             // window borders, toolbars
  palette.setColor(QPalette::Shadow, QColor(0, 0, 0, 255));                // background color for arrays in property grids
  palette.setColor(QPalette::Highlight, QColor(103, 141, 178, 255));       // selected items
  palette.setColor(QPalette::HighlightedText, QColor(255, 255, 255, 255)); // text of selected items
  palette.setColor(QPalette::Link, QColor(0, 0, 238, 255));                // manipulator links in property grid
  palette.setColor(QPalette::LinkVisited, QColor(82, 24, 139, 255));       // manipulator links in property grid when active
  QBrush NoRoleBrush(QColor(0, 0, 0, 255), Qt::NoBrush);
  palette.setBrush(QPalette::NoRole, NoRoleBrush);
  palette.setColor(QPalette::ToolTipBase, QColor(255, 255, 220, 255));              // unused / not working ?
  palette.setColor(QPalette::ToolTipText, QColor(0, 0, 0, 255));                    // unused / not working ?
  palette.setColor(QPalette::PlaceholderText, QColor(200, 200, 200, 255).darker()); // text in search fields

  palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(128, 128, 128, 255)); // labels, tabs, property grid
  palette.setColor(QPalette::Disabled, QPalette::Button, QColor(40, 40, 40, 255));
  palette.setColor(QPalette::Disabled, QPalette::Text, QColor(105, 105, 105, 255));
  palette.setColor(QPalette::Disabled, QPalette::BrightText, QColor(255, 255, 255, 255)); // unused ?
  palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(128, 128, 128, 255));
  palette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(86, 117, 148, 255));

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

  QApplication::setPalette(palette);
}

static void QtDebugMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& sQMsg)
{
  QByteArray       localMsg = sQMsg.toLocal8Bit();
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
    QFont font = m_pQtApplication->font();
    // font.setPixelSize(11);
    m_pQtApplication->setFont(font);
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
