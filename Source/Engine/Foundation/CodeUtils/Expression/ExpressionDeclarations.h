/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/SmallArray.h>
#include <Foundation/DataProcessing/Stream/ProcessingStream.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/SimdMath/SimdVec4f.h>
#include <Foundation/SimdMath/SimdVec4i.h>
#include <Foundation/Types/Variant.h>

class xiiStreamWriter;
class xiiStreamReader;

namespace xiiExpression
{
  struct Register
  {
    XII_DECLARE_POD_TYPE();

    Register() {}; // NOLINT: "= default" does not work here.

    union
    {
      xiiSimdVec4b b;
      xiiSimdVec4i i;
      xiiSimdVec4f f;
    };
  };

  struct RegisterType
  {
    using StorageType = xiiUInt8;

    enum Enum : StorageType
    {
      Unknown,

      Bool,
      Int,
      Float,
      Double, ///< Unsupported

      Count,

      Default    = Float,
      MaxNumBits = 4,
    };

    static const char* GetName(Enum registerType);
  };

  using Output     = xiiArrayPtr<Register>;
  using Inputs     = xiiArrayPtr<xiiArrayPtr<const Register>>; // Inputs are in SOA form, means inner array contains all values for one input parameter, one for each instance.
  using GlobalData = xiiHashTable<xiiHashedString, xiiVariant>;

  /// \brief Describes an input or output stream for a expression VM
  struct StreamDesc
  {
    xiiHashedString               m_sName;
    xiiProcessingStream::DataType m_DataType;

    bool operator==(const StreamDesc& other) const
    {
      return m_sName == other.m_sName && m_DataType == other.m_DataType;
    }

    xiiResult Serialize(xiiStreamWriter& ref_stream) const;
    xiiResult Deserialize(xiiStreamReader& ref_stream);
  };

  /// \brief Describes an expression function and its signature, e.g. how many input parameter it has and their type
  struct FunctionDesc
  {
    using TypeList = xiiSmallArray<xiiEnum<xiiExpression::RegisterType>, 8>;

    xiiHashedString                      m_sName;
    TypeList                             m_InputTypes;
    xiiUInt8                             m_uiNumRequiredInputs = 0;
    xiiEnum<xiiExpression::RegisterType> m_OutputType;

    bool operator==(const FunctionDesc& other) const
    {
      return m_sName == other.m_sName && m_InputTypes == other.m_InputTypes && m_uiNumRequiredInputs == other.m_uiNumRequiredInputs && m_OutputType == other.m_OutputType;
    }

    bool operator<(const FunctionDesc& other) const;

    xiiResult Serialize(xiiStreamWriter& ref_stream) const;
    xiiResult Deserialize(xiiStreamReader& ref_stream);

    xiiHashedString GetMangledName() const;
  };

  using Function                   = void (*)(xiiExpression::Inputs, xiiExpression::Output, const xiiExpression::GlobalData&);
  using ValidateGlobalDataFunction = xiiResult (*)(const xiiExpression::GlobalData&);

} // namespace xiiExpression

/// \brief Describes an external function that can be called in expressions.
///  These functions need to be state-less and thread-safe.
struct xiiExpressionFunction
{
  xiiExpression::FunctionDesc m_Desc;

  xiiExpression::Function m_Func;

  // Optional validation function used to validate required global data for an expression function
  xiiExpression::ValidateGlobalDataFunction m_ValidateGlobalDataFunc;
};

struct XII_FOUNDATION_DLL xiiDefaultExpressionFunctions
{
  static xiiExpressionFunction s_RandomFunc;
  static xiiExpressionFunction s_PerlinNoiseFunc;
};

/// \brief Add this attribute a string property that should be interpreted as expression source.
///
/// The Inputs/Outputs property reference another array property on the same object that contains objects
/// with a name and a type property that can be used for real time error checking of the expression source.
class XII_FOUNDATION_DLL xiiExpressionWidgetAttribute : public xiiTypeWidgetAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiExpressionWidgetAttribute, xiiTypeWidgetAttribute);

public:
  xiiExpressionWidgetAttribute() = default;
  xiiExpressionWidgetAttribute(const char* szInputsProperty, const char* szOutputProperty) :
    m_sInputsProperty(szInputsProperty), m_sOutputsProperty(szOutputProperty)
  {
  }

  const char* GetInputsProperty() const { return m_sInputsProperty; }
  const char* GetOutputsProperty() const { return m_sOutputsProperty; }

private:
  xiiUntrackedString m_sInputsProperty;
  xiiUntrackedString m_sOutputsProperty;
};
