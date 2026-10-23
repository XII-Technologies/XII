/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/GUI/EditorSplashScreen.h>
#include <Foundation/System/SystemInformation.h>
#include <QApplication>
#include <QDialog>
#include <QFontDatabase>
#include <QKeyEvent>
#include <QPainter>
#include <QScreen>
#include <QThread>
#include <QWindow>
#include <limits>
#include <xiiBuildInfo.h>

xiiQtEditorSplashScreen::xiiQtEditorSplashScreen(QScreen* pScreen) :
  QWidget(nullptr, Qt::SplashScreen | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus),
  m_Logo(QStringLiteral(":/Splash/Splash/Logo.svg")),
  m_Progress(this)
{
  XII_ASSERT_DEV(QThread::currentThread() == qApp->thread(), "Splash presentation requires the GUI thread.");
  setObjectName(QStringLiteral("XIIEditorSplash"));
  setWindowTitle(QStringLiteral("Starting XII Editor"));
  setAttribute(Qt::WA_ShowWithoutActivating);
  setAttribute(Qt::WA_QuitOnClose, false);
  setAttribute(Qt::WA_OpaquePaintEvent);
  setAccessibleName(QStringLiteral("XII Editor startup"));
  setFont(QFontDatabase::systemFont(QFontDatabase::GeneralFont));
  setFixedSize(720, 412);
  if (pScreen == nullptr)
    pScreen = QGuiApplication::screenAt(QCursor::pos());
  if (pScreen == nullptr)
    pScreen = QGuiApplication::primaryScreen();
  if (pScreen)
  {
    setScreen(pScreen);
    const QRect available = pScreen->availableGeometry();
    // Small/very highly scaled desktops retain the whole composition, without cropping text.
    const qreal scale = qMin(1.0, qMin(available.width() / 720.0, available.height() / 412.0));
    setFixedSize(qRound(720 * scale), qRound(412 * scale));
    move(available.center() - rect().center());
  }
  const auto& system = xiiSystemInformation::Get();
  m_sBuild           = QStringLiteral("%1  /  %2  /  %3-bit").arg(QString::fromUtf8(system.GetBuildConfiguration()), QString::fromUtf8(system.GetPlatformName())).arg(sizeof(void*) * 8);
  m_sRevision        = QStringLiteral("Build %1").arg(QStringLiteral(XII_PP_STRINGIFY(XII_GIT_COMMIT_HASH_SHORT)));
  m_Progress.setTextVisible(false);
  m_Progress.setAccessibleName(QStringLiteral("Current startup stage progress"));
  m_Progress.setStyleSheet(QStringLiteral("QProgressBar { border: 0; border-radius: 2px; background: #2c353f; } QProgressBar::chunk { background: #79c6c8; border-radius: 2px; }"));
  SetStage(QStringLiteral("Preparing the editor"));
  // Never let a splash become a modal dialog's parent or obscure an actionable failure prompt.
  qApp->installEventFilter(this);
}

void xiiQtEditorSplashScreen::SetEditorWindow(QWidget* pWindow)
{
  m_pEditorWindow = pWindow;
  if (windowHandle())
    windowHandle()->setTransientParent(pWindow ? pWindow->windowHandle() : nullptr);
  // Stay above our own active container, without a global topmost flag or repeated raising.
  if (isVisible() && pWindow && pWindow->isActiveWindow())
    raise();
}

void xiiQtEditorSplashScreen::showEvent(QShowEvent* pEvent)
{
  QWidget::showEvent(pEvent);
  SetEditorWindow(m_pEditorWindow);
}

void xiiQtEditorSplashScreen::SetStage(const QString& sStage, const QString& sDetail, xiiUInt32 uiCompleted, xiiUInt32 uiTotal)
{
  m_sStage  = sStage;
  m_sDetail = sDetail;
  if (uiTotal > 0 && uiTotal <= static_cast<xiiUInt32>(std::numeric_limits<int>::max()))
  {
    m_Progress.setRange(0, static_cast<int>(uiTotal));
    m_Progress.setValue(static_cast<int>(qMin(uiCompleted, uiTotal)));
    m_Progress.setAccessibleDescription(QStringLiteral("%1 of %2 items completed").arg(qMin(uiCompleted, uiTotal)).arg(uiTotal));
    m_sDetail += QStringLiteral("  —  %1 of %2 complete").arg(qMin(uiCompleted, uiTotal)).arg(uiTotal);
  }
  else
  {
    m_Progress.setRange(0, 0);
    m_Progress.setAccessibleDescription(QStringLiteral("Duration unknown"));
  }
  setAccessibleDescription(sStage + QStringLiteral(". ") + sDetail);
  repaint(); // QWidget repaint does not dispatch unrelated queued commands as QSplashScreen does.
}

void xiiQtEditorSplashScreen::paintEvent(QPaintEvent*)
{
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  const qreal scale = width() / 720.0;
  painter.scale(scale, scale);
  painter.fillRect(QRectF(0, 0, 720, 412), QColor("#151b22"));
  painter.setPen(QColor("#35414c"));
  painter.drawRect(QRectF(0.5, 0.5, 719, 411));
  painter.fillRect(QRectF(36, 0, 64, 3), QColor("#79c6c8"));

  auto text = [&](int iSize, QFont::Weight weight, QColor color, const QRectF& rect, const QString& sText, int flags = Qt::AlignLeft | Qt::AlignVCenter) {
    QFont font = this->font();
    font.setPixelSize(iSize);
    font.setWeight(weight);
    painter.setFont(font);
    painter.setPen(color);
    painter.drawText(rect, flags, sText);
  };
  text(12, QFont::DemiBold, QColor("#a6bac4"), QRectF(36, 28, 450, 24), QStringLiteral("XII ENGINE  /  DEVELOPMENT TOOLS"));
  text(43, QFont::DemiBold, QColor("#f1f5f7"), QRectF(34, 85, 450, 64), QStringLiteral("XII Editor"));
  text(15, QFont::Normal, QColor("#a6b1bc"), QRectF(36, 153, 440, 26), QStringLiteral("Real-time development environment"));
  text(14, QFont::Medium, QColor("#79c6c8"), QRectF(36, 203, 420, 24), QStringLiteral("Version %1").arg(QCoreApplication::applicationVersion()));

  // Restrained construction lines frame the existing wordmark, without another rendering dependency.
  painter.setPen(QPen(QColor("#2a3742"), 1));
  painter.drawLine(QPointF(507, 72), QPointF(667, 72));
  painter.drawLine(QPointF(507, 214), QPointF(667, 214));
  painter.drawLine(QPointF(507, 72), QPointF(507, 214));
  painter.drawLine(QPointF(667, 72), QPointF(667, 214));
  painter.setPen(QPen(QColor("#607b86"), 2));
  painter.drawLine(QPointF(507, 72), QPointF(525, 72));
  painter.drawLine(QPointF(507, 72), QPointF(507, 90));
  painter.drawLine(QPointF(649, 214), QPointF(667, 214));
  painter.drawLine(QPointF(667, 196), QPointF(667, 214));
  m_Logo.render(&painter, QRectF(525, 117, 124, 52));

  painter.setPen(QColor("#2c353f"));
  painter.drawLine(QPointF(36, 250), QPointF(684, 250));
  QFont statusFont = font();
  statusFont.setPixelSize(15);
  statusFont.setWeight(QFont::Medium);
  const QFontMetrics statusMetrics(statusFont);
  text(15, QFont::Medium, QColor("#e2eaf0"), QRectF(36, 266, 648, 28), statusMetrics.elidedText(m_sStage, Qt::ElideRight, 648));
  QFont detailFont = font();
  detailFont.setPixelSize(12);
  const QFontMetrics details(detailFont);
  text(12, QFont::Normal, QColor("#9cabb8"), QRectF(36, 294, 648, 22), details.elidedText(m_sDetail, Qt::ElideMiddle, 648));
  m_Progress.setGeometry(qRound(36 * scale), qRound(327 * scale), qRound(648 * scale), qMax(3, qRound(4 * scale)));
  text(11, QFont::Normal, QColor("#8c9ba7"), QRectF(36, 356, 540, 20), m_sBuild);
  text(11, QFont::Normal, QColor("#8c9ba7"), QRectF(36, 376, 540, 20), QStringLiteral("© Theophilus Eriata. All rights reserved."));
  text(11, QFont::Normal, QColor("#8c9ba7"), QRectF(540, 356, 144, 20), m_sRevision, Qt::AlignRight | Qt::AlignVCenter);
}

void xiiQtEditorSplashScreen::mousePressEvent(QMouseEvent*)
{
  hide(); // Preserve the established click-to-dismiss behavior; initialization continues.
}

void xiiQtEditorSplashScreen::keyPressEvent(QKeyEvent* pEvent)
{
  if (pEvent->key() == Qt::Key_Escape)
    hide();
  else
    QWidget::keyPressEvent(pEvent);
}

bool xiiQtEditorSplashScreen::eventFilter(QObject* pObject, QEvent* pEvent)
{
  if (pEvent->type() == QEvent::Show && qobject_cast<QDialog*>(pObject))
    hide();
  return false;
}
