#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/VisualScript/VisualScriptInstance.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode::xiiVisualScriptNode() {}
xiiVisualScriptNode::~xiiVisualScriptNode() {}


xiiInt32 xiiVisualScriptNode::HandlesMessagesWithID() const
{
  return -1;
}

void xiiVisualScriptNode::HandleMessage(xiiMessage* pMsg) {}

bool xiiVisualScriptNode::IsManuallyStepped() const
{
  xiiHybridArray<xiiAbstractProperty*, 32> properties;
  GetDynamicRTTI()->GetAllProperties(properties);

  for (auto prop : properties)
  {
    if (prop->GetAttributeByType<xiiVisScriptExecPinOutAttribute>() != nullptr)
      return true;

    if (prop->GetAttributeByType<xiiVisScriptExecPinInAttribute>() != nullptr)
      return true;
  }

  return false;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiVisualScriptDataPinType, 1)
XII_ENUM_CONSTANTS(xiiVisualScriptDataPinType::None, xiiVisualScriptDataPinType::Number, xiiVisualScriptDataPinType::Boolean, xiiVisualScriptDataPinType::Vec3, xiiVisualScriptDataPinType::String)
XII_ENUM_CONSTANTS(xiiVisualScriptDataPinType::GameObjectHandle, xiiVisualScriptDataPinType::ComponentHandle, xiiVisualScriptDataPinType::Variant)
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

// static
xiiVisualScriptDataPinType::Enum xiiVisualScriptDataPinType::GetDataPinTypeForType(const xiiRTTI* pType)
{
  auto varType = pType->GetVariantType();
  if (varType >= xiiVariant::Type::Int8 && varType <= xiiVariant::Type::Double)
  {
    return xiiVisualScriptDataPinType::Number;
  }

  switch (varType)
  {
    case xiiVariantType::Bool:
      return xiiVisualScriptDataPinType::Boolean;

    case xiiVariantType::Vector3:
      return xiiVisualScriptDataPinType::Vec3;

    case xiiVariantType::String:
      return xiiVisualScriptDataPinType::String;

    default:
      return pType == xiiGetStaticRTTI<xiiVariant>() ? xiiVisualScriptDataPinType::Variant : xiiVisualScriptDataPinType::None;
  }
}

// static
void xiiVisualScriptDataPinType::EnforceSupportedType(xiiVariant& var)
{
  switch (var.GetType())
  {
    case xiiVariantType::Int8:
    case xiiVariantType::UInt8:
    case xiiVariantType::Int16:
    case xiiVariantType::UInt16:
    case xiiVariantType::Int32:
    case xiiVariantType::UInt32:
    case xiiVariantType::Int64:
    case xiiVariantType::UInt64:
    case xiiVariantType::Float:
    {
      const double value = var.ConvertTo<double>();
      var                = value;
      return;
    }

    default:
      return;
  }
}

static xiiUInt32 s_StorageSizes[] = {
  xiiInvalidIndex,             // None
  sizeof(double),              // Number
  sizeof(bool),                // Boolean
  sizeof(xiiVec3),             // Vec3
  sizeof(xiiString),           // String
  sizeof(xiiGameObjectHandle), // GameObjectHandle
  sizeof(xiiComponentHandle),  // ComponentHandle
  sizeof(xiiVariant),          // Variant
};

// static
xiiUInt32 xiiVisualScriptDataPinType::GetStorageByteSize(Enum dataPinType)
{
  if (dataPinType >= Number && dataPinType <= Variant)
  {
    XII_CHECK_AT_COMPILETIME(XII_ARRAY_SIZE(s_StorageSizes) == Variant + 1);
    return s_StorageSizes[dataPinType];
  }

  XII_ASSERT_NOT_IMPLEMENTED;
  return xiiInvalidIndex;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisScriptExecPinOutAttribute, 1, xiiRTTIDefaultAllocator<xiiVisScriptExecPinOutAttribute>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Slot", m_uiPinSlot)
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisScriptExecPinInAttribute, 1, xiiRTTIDefaultAllocator<xiiVisScriptExecPinInAttribute>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Slot", m_uiPinSlot)
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisScriptDataPinInAttribute, 1, xiiRTTIDefaultAllocator<xiiVisScriptDataPinInAttribute>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Slot", m_uiPinSlot),
    XII_ENUM_MEMBER_PROPERTY("Type", xiiVisualScriptDataPinType, m_DataType)
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisScriptDataPinOutAttribute, 1, xiiRTTIDefaultAllocator<xiiVisScriptDataPinOutAttribute>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Slot", m_uiPinSlot),
    XII_ENUM_MEMBER_PROPERTY("Type", xiiVisualScriptDataPinType, m_DataType)
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

XII_STATICLINK_FILE(GameEngine, GameEngine_VisualScript_Implementation_VisualScriptNode);
