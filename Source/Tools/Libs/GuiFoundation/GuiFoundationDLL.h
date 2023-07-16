#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Types/Uuid.h>
#include <QColor>
#include <QMetaType>
#include <ToolsFoundation/ToolsFoundationDLL.h>

// Configure the DLL Import/Export Define
#if XII_ENABLED(XII_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_GUIFOUNDATION_LIB
#    define XII_GUIFOUNDATION_DLL XII_DECL_EXPORT
#  else
#    define XII_GUIFOUNDATION_DLL XII_DECL_IMPORT
#  endif
#else
#  define XII_GUIFOUNDATION_DLL
#endif

class QWidget;
class QObject;


Q_DECLARE_METATYPE(xiiUuid);

/// \brief Calls setUpdatesEnabled(false) on all given QObjects, and the reverse in the destructor. Can be nested.
class XII_GUIFOUNDATION_DLL xiiQtScopedUpdatesDisabled
{
public:
  xiiQtScopedUpdatesDisabled(QWidget* pWidget1, QWidget* pWidget2 = nullptr, QWidget* pWidget3 = nullptr, QWidget* pWidget4 = nullptr, QWidget* pWidget5 = nullptr, QWidget* pWidget6 = nullptr);
  ~xiiQtScopedUpdatesDisabled();

private:
  QWidget* m_pWidgets[6];
};


/// \brief Calls blockSignals(true) on all given QObjects, and the reverse in the destructor. Can be nested.
class XII_GUIFOUNDATION_DLL xiiQtScopedBlockSignals
{
public:
  xiiQtScopedBlockSignals(QObject* pObject1, QObject* pObject2 = nullptr, QObject* pObject3 = nullptr, QObject* pObject4 = nullptr, QObject* pObject5 = nullptr, QObject* pObject6 = nullptr);
  ~xiiQtScopedBlockSignals();

private:
  QObject* m_pObjects[6];
};

XII_ALWAYS_INLINE QColor xiiToQtColor(const xiiColorGammaUB& c)
{
  return QColor(c.r, c.g, c.b, c.a);
}

XII_ALWAYS_INLINE xiiColorGammaUB qtToXIIColor(const QColor& c)
{
  return xiiColorGammaUB(c.red(), c.green(), c.blue(), c.alpha());
}
