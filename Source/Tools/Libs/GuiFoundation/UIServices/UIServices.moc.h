/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Communication/Event.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Strings/FormatString.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/Status.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QApplication>
#include <QMessageBox>

class QColorDialog;
class xiiQtColorDialog;

class XII_GUIFOUNDATION_DLL xiiQtUiServices : public QObject
{
  Q_OBJECT

  XII_DECLARE_SINGLETON(xiiQtUiServices);

public:
  struct Event
  {
    enum Type
    {
      ShowDocumentTemporaryStatusBarText,
      ShowDocumentPermanentStatusBarText,
      ShowGlobalStatusBarText,
      ClickedDocumentPermanentStatusBarText,
      CheckForUpdates,
    };

    enum TextType
    {
      Info,
      Warning,
      Error
    };

    Type      m_Type;
    xiiString m_sText;
    xiiTime   m_Time;
    TextType  m_TextType = TextType::Info;
  };

  static xiiEvent<const xiiQtUiServices::Event&> s_Events;

  struct TickEvent
  {
    enum class Type
    {
      StartFrame,
      EndFrame,
    };

    Type      m_Type;
    xiiUInt32 m_uiFrame = 0;
    xiiTime   m_Time;
    double    m_fRefreshRate = 60.0;
  };

  static xiiEvent<const xiiQtUiServices::TickEvent&> s_TickEvent;

public:
  xiiQtUiServices();

  /// True if the application doesn't show any window and only works in the background
  static bool IsHeadless();

  /// Set to true if the application doesn't show any window and only works in the background
  static void SetHeadless(bool bHeadless);

  /// Shows a non-modal color dialog. The Qt slots are called when the selected color is changed or when the dialog is closed and the result
  /// accepted or rejected.
  void ShowColorDialog(const xiiColor& color, bool bAlpha, bool bHDR, QWidget* pParent, xiiStringView sSlotCurColChanged, xiiStringView sSlotAccept, xiiStringView sSlotReject);

  /// Might show a message box depending on the given status. If the status is 'failure' the sFailureMsg is shown, including the message in
  /// xiiStatus. If the status is success a message box with text sSuccessMsg is shown, but only if the status message is not empty or if
  /// bOnlySuccessMsgIfDetails is false.
  static void MessageBoxStatus(const xiiStatus& s, xiiStringView sFailureMsg, xiiStringView sSuccessMsg = {}, bool bOnlySuccessMsgIfDetails = true);

  /// Shows an information message box
  static void MessageBoxInformation(const xiiFormatString& msg);

  /// Shows an warning message box
  static void MessageBoxWarning(const xiiFormatString& msg);

  /// Shows a question message box and returns which button the user pressed
  static QMessageBox::StandardButton MessageBoxQuestion(const xiiFormatString& msg, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton);

  /// Use this if you need to display a status bar message in any/all documents. Go directly through the document, if you only want to show a
  /// message in a single document window.
  static void ShowAllDocumentsTemporaryStatusBarMessage(const xiiFormatString& msg, xiiTime timeOut);

  static void ShowAllDocumentsPermanentStatusBarMessage(const xiiFormatString& msg, Event::TextType type);

  /// Shows a 'critical' message in all container windows (in red), which does not disappear, until it is replaced with another (empty) string.
  static void ShowGlobalStatusBarMessage(const xiiFormatString& msg);

  /// Opens the given file in the program that is registered in the OS to handle that file type.
  static bool OpenFileInDefaultProgram(xiiStringView sPath);

  /// Opens the given file or folder in the Explorer
  static void OpenInExplorer(xiiStringView sPath, bool bIsFile);

  /// Shows the "Open With" dialog
  static void OpenWith(xiiStringView sPath);

  /// Attempts to launch Visual Studio Code with the given command line
  static xiiStatus OpenInVsCode(const QStringList& arguments);

  /// Loads some global state used by xiiQtUiServices from the registry. E.g. the last position of the color dialog.
  void LoadState();

  /// Saves some global state used by xiiQtUiServices to the registry.
  void SaveState();

  /// Returns a cached QIcon that was created from an internal Qt resource (e.g. 'QIcon(":QtNamespace/MyIcon.png")' ). Prevents creating the
  /// object over and over.
  ///
  /// If svgTintColor is a non-zero color, and sIdentifier points to an .SVG file, then the first time the icon is requested with that color,
  /// a copy is made, and the SVG content is modified such that white ("#FFFFFF") gets replaced by the requested color.
  /// Thus multiple tints of the same icon can be created for different use cases.
  /// Usually this is used to get different shades of the same icon, such that it looks good on the target background.
  static const QIcon& GetCachedIconResource(xiiStringView sIdentifier, xiiColor svgTintColor = xiiColor::MakeZero());

  /// Returns a cached QImage that was created from an internal Qt resource (e.g. 'QImage(":QtNamespace/MyIcon.png")' ). Prevents creating the
  /// object over and over.
  static const QImage& GetCachedImageResource(xiiStringView sIdentifier);

  /// Returns a cached QPixmap that was created from an internal Qt resource (e.g. 'QPixmap(":QtNamespace/MyIcon.png")' ). Prevents creating
  /// the object over and over.
  static const QPixmap& GetCachedPixmapResource(xiiStringView sIdentifier);

  /// Adds the pattern to the gitignore file.
  ///
  /// If the gitignore file does not exist, it is created.
  /// If the pattern is already present in the file, it is not added again.
  static xiiResult AddToGitIgnore(xiiStringView sGitIgnoreFile, xiiStringView sPattern);

  /// Raises the 'CheckForUpdates' event
  static void CheckForUpdates();

  void Init();

private Q_SLOTS:
  void TickEventHandler();

private:
  xiiQtColorDialog* m_pColorDlg;
  QByteArray        m_ColorDlgGeometry;

  static xiiMap<xiiString, QIcon>   s_IconsCache;
  static xiiMap<xiiString, QImage>  s_ImagesCache;
  static xiiMap<xiiString, QPixmap> s_PixmapsCache;
  static bool                       s_bHeadless;
  static TickEvent                  s_LastTickEvent;
  bool                              m_bIsDrawingATM = false;
};
