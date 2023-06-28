#pragma once

#include <Core/World/GameObject.h>
#include <VisualScriptPlugin/VisualScriptPluginDLL.h>

struct XII_VISUALSCRIPTPLUGIN_DLL xiiVisualScriptDataType
{
  using StorageType = xiiUInt8;

  enum Enum : xiiUInt8
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
    GameObject,
    Component,
    TypedPointer,
    Variant,
    Array,
    Map,

    Count,
    Default = Invalid,

    Any = 0xFF,
  };

  XII_ALWAYS_INLINE static bool IsNumber(Enum dataType) { return dataType >= Bool && dataType <= Double; }

  static xiiVariantType::Enum GetVariantType(Enum dataType);
  static Enum                 FromVariantType(xiiVariantType::Enum variantType);

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
