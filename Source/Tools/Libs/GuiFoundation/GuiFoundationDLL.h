/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Uuid.h>
#include <QColor>
#include <QDataStream>
#include <QMetaType>

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
class QKeyEvent;

Q_DECLARE_METATYPE(xiiUuid);

/// Calls setUpdatesEnabled(false) on all given QObjects, and the reverse in the destructor. Can be nested.
class XII_GUIFOUNDATION_DLL xiiQtScopedUpdatesDisabled
{
public:
  xiiQtScopedUpdatesDisabled(QWidget* pWidget1, QWidget* pWidget2 = nullptr, QWidget* pWidget3 = nullptr, QWidget* pWidget4 = nullptr, QWidget* pWidget5 = nullptr, QWidget* pWidget6 = nullptr);
  ~xiiQtScopedUpdatesDisabled();

private:
  QWidget* m_pWidgets[6];
};


/// Calls blockSignals(true) on all given QObjects, and the reverse in the destructor. Can be nested.
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

XII_ALWAYS_INLINE xiiString qtToXIIString(const QString& sString)
{
  QByteArray data = sString.toUtf8();
  return xiiString(xiiStringView(data.data(), static_cast<xiiUInt32>(data.size())));
}

XII_ALWAYS_INLINE QString xiiMakeQString(xiiStringView sString)
{
  return QString::fromUtf8(sString.GetStartPointer(), sString.GetElementCount());
}

template <typename T>
void operator>>(QDataStream& inout_stream, T*& rhs)
{
  void* p   = nullptr;
  uint  len = sizeof(void*);
  inout_stream.readRawData((char*)&p, len);
  rhs = (T*)p;
}

template <typename T>
void operator<<(QDataStream& inout_stream, T* rhs)
{
  inout_stream.writeRawData((const char*)&rhs, sizeof(void*));
}

template <typename T>
void operator>>(QDataStream& inout_stream, xiiDynamicArray<T>& rhs)
{
  xiiUInt32 uiIndices = 0;
  inout_stream >> uiIndices;
  rhs.Clear();
  rhs.Reserve(uiIndices);

  for (xiiUInt32 i = 0; i < uiIndices; ++i)
  {
    T obj = {};
    inout_stream >> obj;
    rhs.PushBack(obj);
  }
}

template <typename T>
void operator<<(QDataStream& inout_stream, xiiDynamicArray<T>& rhs)
{
  xiiUInt32 uiIndices = rhs.GetCount();
  inout_stream << uiIndices;

  for (xiiUInt32 i = 0; i < uiIndices; ++i)
  {
    inout_stream << rhs[i];
  }
}

namespace xiiQtUtils
{
  /// Uses keyboard layout independent scan-codes to check whether the key of the QKeyEvent represents the desired key.
  ///
  /// Use this when the position of the key on the keyboard is the desired aspect, not the actual character.
  /// For example for navigation (WSAD) in a viewport.
  ///
  /// Assumes the standard US keyboard layout for the reference keys.
  XII_GUIFOUNDATION_DLL bool IsEquivalentQtKey(const QKeyEvent* e, Qt::Key reference);

} // namespace xiiQtUtils
