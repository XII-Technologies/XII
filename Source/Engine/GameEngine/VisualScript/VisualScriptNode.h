#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <GameEngine/GameEngineDLL.h>

class xiiVisualScriptInstance;

class XII_GAMEENGINE_DLL xiiVisualScriptNode : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode, xiiReflectedClass);

public:
  xiiVisualScriptNode();
  ~xiiVisualScriptNode();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) = 0;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin)                          = 0;

  /// \brief Should return the message ID of the message type that this node wants to handle
  ///
  /// ie. xiiMessage::GetTypeMsgId() of the desired message type
  virtual xiiInt32 HandlesMessagesWithID() const;

  /// \brief If HandlesMessagesWithID() returns a valid message ID, messages of that type may be delivered through this function
  virtual void HandleMessage(xiiMessage* pMsg);

  /// \brief Whether the node has an execution pin (input or output) and thus must be stepped manually. Otherwise it will be implicitly executed on
  /// demand.
  ///
  /// By default this is determined by checking the properties of the xiiVisualScriptNode for attributes of type xiiVisScriptExecPinOutAttribute
  /// and xiiVisScriptExecPinInAttribute. If those exist, it is a manually stepped node. However, derived types can override this to use other
  /// criteria.
  virtual bool IsManuallyStepped() const;

protected:
  /// When this is set to true (e.g. in a message handler, the node will be stepped during the next script update)
  bool m_bStepNode = false;
  /// Set to true whenever the input values have been modified before 'Execute' is called. Automatically set to false afterwards.
  bool m_bInputValuesChanged = true;

private:
  friend class xiiVisualScriptInstance;

  xiiUInt16 m_uiNodeID;
};


#define XII_INPUT_EXECUTION_PIN(name, slot)   XII_CONSTANT_PROPERTY(name, 0)->AddAttributes(new xiiVisScriptExecPinInAttribute(slot))
#define XII_OUTPUT_EXECUTION_PIN(name, slot)  XII_CONSTANT_PROPERTY(name, 0)->AddAttributes(new xiiVisScriptExecPinOutAttribute(slot))
#define XII_INPUT_DATA_PIN(name, slot, type)  XII_CONSTANT_PROPERTY(name, 0)->AddAttributes(new xiiVisScriptDataPinInAttribute(slot, type))
#define XII_OUTPUT_DATA_PIN(name, slot, type) XII_CONSTANT_PROPERTY(name, 0)->AddAttributes(new xiiVisScriptDataPinOutAttribute(slot, type))
#define XII_INPUT_DATA_PIN_AND_PROPERTY(name, slot, type, member) \
  XII_MEMBER_PROPERTY(name, member)->AddAttributes(new xiiVisScriptDataPinInAttribute(slot, type))

//////////////////////////////////////////////////////////////////////////

struct XII_GAMEENGINE_DLL xiiVisualScriptDataPinType
{
  typedef xiiUInt8 StorageType;

  enum Enum
  {
    None,
    Number, ///< Numbers are represented as doubles
    Boolean,
    Vec3,
    String,
    GameObjectHandle, ///< xiiGameObjectHandle
    ComponentHandle,  ///< xiiComponentHandle
    // ResourceHandle, ///< xiiTypelessResourceHandle ?
    Variant,
    Default = None,
  };

  /// \brief Returns the corresponding data pin type for the given type or None if the type is not supported
  static Enum GetDataPinTypeForType(const xiiRTTI* pType);

  /// \brief Returns whether the given type is supported by visual script
  XII_ALWAYS_INLINE static bool IsTypeSupported(const xiiRTTI* pType) { return GetDataPinTypeForType(pType) != None; }

  /// \brief Enforces the given variant to be a supported type, ie. mostly doubles for number types
  static void EnforceSupportedType(xiiVariant& var);

  /// \brief Returns how much storage an object of the given type would need
  static xiiUInt32 GetStorageByteSize(Enum dataPinType);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GAMEENGINE_DLL, xiiVisualScriptDataPinType);

//////////////////////////////////////////////////////////////////////////

class XII_GAMEENGINE_DLL xiiVisScriptExecPinOutAttribute : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisScriptExecPinOutAttribute, xiiPropertyAttribute);

public:
  explicit xiiVisScriptExecPinOutAttribute(xiiUInt8 uiSlot = 0xFF) { m_uiPinSlot = uiSlot; }

  xiiUInt8 m_uiPinSlot;
};

class XII_GAMEENGINE_DLL xiiVisScriptExecPinInAttribute : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisScriptExecPinInAttribute, xiiPropertyAttribute);

public:
  explicit xiiVisScriptExecPinInAttribute(xiiUInt8 uiSlot = 0) { m_uiPinSlot = uiSlot; }

  xiiUInt8 m_uiPinSlot;
};

class XII_GAMEENGINE_DLL xiiVisScriptDataPinInAttribute : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisScriptDataPinInAttribute, xiiPropertyAttribute);

public:
  xiiVisScriptDataPinInAttribute()
  {
    m_uiPinSlot = 0xff;
    m_DataType  = xiiVisualScriptDataPinType::None;
  }
  xiiVisScriptDataPinInAttribute(xiiUInt8 uiSlot, xiiVisualScriptDataPinType::Enum dataType)
  {
    m_uiPinSlot = uiSlot;
    m_DataType  = dataType;
  }

  xiiUInt8                            m_uiPinSlot;
  xiiEnum<xiiVisualScriptDataPinType> m_DataType;
};

class XII_GAMEENGINE_DLL xiiVisScriptDataPinOutAttribute : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisScriptDataPinOutAttribute, xiiPropertyAttribute);

public:
  xiiVisScriptDataPinOutAttribute()
  {
    m_uiPinSlot = 0xff;
    m_DataType  = xiiVisualScriptDataPinType::None;
  }
  xiiVisScriptDataPinOutAttribute(xiiUInt8 uiSlot, xiiVisualScriptDataPinType::Enum dataType)
  {
    m_uiPinSlot = uiSlot;
    m_DataType  = dataType;
  }

  xiiUInt8                            m_uiPinSlot;
  xiiEnum<xiiVisualScriptDataPinType> m_DataType;
};
