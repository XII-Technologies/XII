#pragma once

#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>
#include <VisualScriptPlugin/Runtime/VisualScriptDataType.h>

struct xiiVisualScriptVariableType
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    Invalid = xiiVisualScriptDataType::Invalid,
    Bool    = xiiVisualScriptDataType::Bool,
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

    Resource = xiiVisualScriptDataType::Resource,

    Default = Int,
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_EDITORPLUGINVISUALSCRIPT_DLL, xiiVisualScriptVariableType);

//////////////////////////////////////////////////////////////////////////////

struct xiiVisualScriptVariableCategory
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    Member,
    Array,
    Map,
    Default = Member,
  };

  static xiiPropertyCategory::Enum GetPropertyCategory(Enum category);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_EDITORPLUGINVISUALSCRIPT_DLL, xiiVisualScriptVariableCategory);

//////////////////////////////////////////////////////////////////////////////

struct xiiVisualScriptVariableTypeDeclaration
{
  XII_DECLARE_POD_TYPE();

  xiiEnum<xiiVisualScriptVariableType>     m_Type;
  xiiEnum<xiiVisualScriptVariableCategory> m_Category;
  bool                                     m_bPublic   = false;
  xiiUInt8                                 m_uiPadding = 0;

  bool operator==(const xiiVisualScriptVariableTypeDeclaration& other) const
  {
    return m_Type == other.m_Type && m_Category == other.m_Category && m_bPublic == other.m_bPublic;
  }

  xiiVisualScriptDataType::Enum GetDataType() const;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_EDITORPLUGINVISUALSCRIPT_DLL, xiiVisualScriptVariableTypeDeclaration);
XII_DECLARE_CUSTOM_VARIANT_TYPE(xiiVisualScriptVariableTypeDeclaration);

class xiiStreamWriter;
class xiiStreamReader;

void operator<<(xiiStreamWriter& inout_stream, const xiiVisualScriptVariableTypeDeclaration& value);
void operator>>(xiiStreamReader& inout_stream, xiiVisualScriptVariableTypeDeclaration& value);

template <>
struct xiiHashHelper<xiiVisualScriptVariableTypeDeclaration>
{
  XII_ALWAYS_INLINE static xiiUInt32 Hash(const xiiVisualScriptVariableTypeDeclaration& value) { return xiiHashHelper<xiiUInt32>::Hash(*(const xiiUInt32*)&value); }

  XII_ALWAYS_INLINE static bool Equal(const xiiVisualScriptVariableTypeDeclaration& a, const xiiVisualScriptVariableTypeDeclaration& b) { return a == b; }
};

//////////////////////////////////////////////////////////////////////////

class xiiVisualScriptVariableAttribute : public xiiTypeWidgetAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptVariableAttribute, xiiTypeWidgetAttribute);
};

//////////////////////////////////////////////////////////////////////////

class xiiQtVisualScriptVariableWidget : public xiiQtVariantPropertyWidget
{
  Q_OBJECT;

public:
  xiiQtVisualScriptVariableWidget();
  virtual ~xiiQtVisualScriptVariableWidget();

  virtual void InternalSetValue(const xiiVariant& value) override;

  virtual xiiResult GetVariantTypeDisplayName(xiiVariantType::Enum type, xiiStringBuilder& out_sName) const override;
};

//////////////////////////////////////////////////////////////////////////

class xiiQtVisualScriptVariableTypeDeclarationWidget : public xiiQtStandardPropertyWidget
{
  Q_OBJECT;

public:
  xiiQtVisualScriptVariableTypeDeclarationWidget();
  virtual ~xiiQtVisualScriptVariableTypeDeclarationWidget();

  virtual void OnInit() override;
  virtual void InternalSetValue(const xiiVariant& value) override;

private:
  void ChangeType();

  QHBoxLayout* m_pLayout           = nullptr;
  QComboBox*   m_pTypeList         = nullptr;
  QMenu*       m_pCategoryList     = nullptr;
  QPushButton* m_pCategoryButton   = nullptr;
  QPushButton* m_pVisibilityButton = nullptr;
};

//////////////////////////////////////////////////////////////////////////////

struct xiiVisualScriptVariable
{
  xiiHashedString                        m_sName;
  xiiVisualScriptVariableTypeDeclaration m_TypeDecl;
  xiiVariant                             m_DefaultValue;

  bool   m_bClampRange = false;
  double m_fMinValue   = 0.0;
  double m_fMaxValue   = 1.0;

  static void ConvertDefaultValue(xiiVariant& inout_defaultValue, xiiVisualScriptVariableTypeDeclaration targetTypeDecl);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_EDITORPLUGINVISUALSCRIPT_DLL, xiiVisualScriptVariable);

//////////////////////////////////////////////////////////////////////////

struct xiiVisualScriptExpressionDataType
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    Int     = static_cast<xiiUInt8>(xiiProcessingStream::DataType::Int),
    Float   = static_cast<xiiUInt8>(xiiProcessingStream::DataType::Float),
    Vector3 = static_cast<xiiUInt8>(xiiProcessingStream::DataType::Float3),
    Color   = static_cast<xiiUInt8>(xiiProcessingStream::DataType::Float4),

    Default = Float
  };

  static xiiVisualScriptDataType::Enum GetVisualScriptDataType(Enum dataType);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_EDITORPLUGINVISUALSCRIPT_DLL, xiiVisualScriptExpressionDataType);

struct xiiVisualScriptExpressionVariable
{
  xiiHashedString                            m_sName;
  xiiEnum<xiiVisualScriptExpressionDataType> m_Type;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_EDITORPLUGINVISUALSCRIPT_DLL, xiiVisualScriptExpressionVariable);
