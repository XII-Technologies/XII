/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/World/GameObject.h>
#include <Foundation/DataProcessing/Stream/ProcessingStream.h>
#include <VisualScriptPlugin/VisualScriptPluginDLL.h>

/// Data types that are available in visual script. These are a subset of xiiVariantType.
///
/// Like with xiiVariantType, the order of these types is important as they are used to determine
/// if a type is "bigger" during type deduction. Also the enum values are serialized in visual script files.
struct XII_VISUALSCRIPTPLUGIN_DLL xiiVisualScriptDataType
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Invalid = 0,

    Bool,
    Byte,
    Int,
    Int64,
    Float,
    Double,
    Color,
    Vector3,
    Quaternion,
    Transform,
    Time,
    Angle,
    String,
    HashedString,
    GameObject,
    Component,
    TypedPointer,
    Variant,
    Array,
    Map,
    Coroutine,

    Count,

    EnumValue,
    BitflagValue,
    Resource,

    ExtendedCount,

    AnyPointer = 0xFE,
    Any        = 0xFF,

    Default = Invalid,
  };

  XII_ALWAYS_INLINE static bool IsNumber(Enum dataType) { return dataType >= Byte && dataType <= Double; }
  XII_ALWAYS_INLINE static bool IsNumberOrBool(Enum dataType) { return dataType == Bool || IsNumber(dataType); }
  XII_ALWAYS_INLINE static bool IsPointer(Enum dataType) { return (dataType >= GameObject && dataType <= TypedPointer) || dataType == Coroutine; }

  static xiiVariantType::Enum GetVariantType(Enum dataType);
  static Enum                 FromVariantType(xiiVariantType::Enum variantType);

  static xiiProcessingStream::DataType GetStreamDataType(Enum dataType);

  static const xiiRTTI* GetRtti(Enum dataType);
  static Enum           FromRtti(const xiiRTTI* pRtti);

  static xiiUInt32 GetStorageSize(Enum dataType);
  static xiiUInt32 GetStorageAlignment(Enum dataType);

  static const char* GetName(Enum dataType);

  static bool CanConvertTo(Enum sourceDataType, Enum targetDataType);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_VISUALSCRIPTPLUGIN_DLL, xiiVisualScriptDataType);

struct XII_VISUALSCRIPTPLUGIN_DLL xiiVisualScriptGameObjectHandle
{
  xiiGameObjectHandle    m_Handle;
  mutable xiiGameObject* m_Ptr;
  mutable xiiUInt32      m_uiExecutionCounter;

  void AssignHandle(const xiiGameObjectHandle& hObject)
  {
    m_Handle             = hObject;
    m_Ptr                = nullptr;
    m_uiExecutionCounter = 0;
  }

  void AssignPtr(xiiGameObject* pObject, xiiUInt32 uiExecutionCounter)
  {
    m_Handle             = pObject != nullptr ? pObject->GetHandle() : xiiGameObjectHandle();
    m_Ptr                = pObject;
    m_uiExecutionCounter = uiExecutionCounter;
  }

  xiiGameObject* GetPtr(xiiUInt32 uiExecutionCounter) const;
};

struct XII_VISUALSCRIPTPLUGIN_DLL xiiVisualScriptComponentHandle
{
  xiiComponentHandle    m_Handle;
  mutable xiiComponent* m_Ptr;
  mutable xiiUInt32     m_uiExecutionCounter;

  void AssignHandle(const xiiComponentHandle& hComponent)
  {
    m_Handle             = hComponent;
    m_Ptr                = nullptr;
    m_uiExecutionCounter = 0;
  }

  void AssignPtr(xiiComponent* pComponent, xiiUInt32 uiExecutionCounter)
  {
    m_Handle             = pComponent != nullptr ? pComponent->GetHandle() : xiiComponentHandle();
    m_Ptr                = pComponent;
    m_uiExecutionCounter = uiExecutionCounter;
  }

  xiiComponent* GetPtr(xiiUInt32 uiExecutionCounter) const;
};
