#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Resources/Resource.h>
#include <GraphicsFoundation/Shader/ShaderByteCode.h>

/// \brief This describes the shader resource type.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALShaderResourceType
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Unknown = 0U,          ///< The shader resource type is unknown.
    ConstantBuffer,        ///< Constant (uniform) buffer.
    TextureSRV,            ///< Shader resource view of a texture (sampled image).
    BufferSRV,             ///< Shader resource view of a buffer (read-only storage image).
    TextureUAV,            ///< Unordered access view of a texture (storage image).
    BufferUAV,             ///< Unordered access view of a buffer (storage buffer).
    Sampler,               ///< Sampler (separate sampler).
    InputAttachment,       ///< Input attachment in a render pass.
    AccelerationStructure, ///< Acceleration structure.

    ENUM_COUNT,

    Default = Unknown
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALShaderResourceType);

/// \brief This describes the primitive type of a shader code variable.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALShaderPrimitiveType
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Unknown = 0U, ///< The primitive type is unknown.
    Void,         ///< Void pointer.
    Bool,         ///< Boolean (bool).
    Int8,         ///< 8-bit integer (int8).
    Int16,        ///< 16-bit integer (int16).
    Int32,        ///< 32-bit integer (int).
    Int64,        ///< 64-bit integer (int64).
    UInt8,        ///< 8-bit unsigned integer (uint8).
    UInt16,       ///< 16-bit unsigned integer (uint16).
    UInt32,       ///< 32-bit unsigned integer (uint).
    UInt64,       ///< 64-bit unsigned integer (uint64).
    Float16,      ///< 16-bit floating-point number (half).
    Float32,      ///< 32-bit floating-point number (float).
    Double,       ///< Double-precision (64-bit) floating-point number (double).
    Min8Float,    ///< 8-bit float (min8float).
    Min10Float,   ///< 10-bit float (min10float).
    Min16Float,   ///< 16-bit float (min16float).
    Min12Int,     ///< 12-bit int (min12int).
    Min16Int,     ///< 16-bit int (min16int).
    Min16UInt,    ///< 16-bit unsigned int (min12uint).
    String,       ///< String (string).

    ENUM_COUNT,

    Default = Unknown
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALShaderPrimitiveType);

/// \brief This describes the class type of a shader code variable.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALShaderVariableClassType
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Unknown = 0U,  ///< The class type is unknown.
    Scalar,        ///< The variable is a scalar.
    Array,         ///< The variable is an array.
    MatrixRows,    ///< The variable is a row-major matrix.
    MatrixColumns, ///< The variable is a column-major matrix.
    Struct,        ///< The variable is a structure.

    ENUM_COUNT,

    Default = Unknown
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALShaderVariableClassType);

/// \brief This describes shader resource.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALShaderResourceDescription : public xiiHashableStruct<xiiGALShaderResourceDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiString                         m_sName;                                           ///< The shader resource name.
  xiiEnum<xiiGALShaderResourceType> m_Type        = xiiGALShaderResourceType::Unknown; ///< The shader resource type. The default is xiiGALShaderResourceType::Unknown.
  xiiUInt32                         m_uiArraySize = 0U;                                ///< The array size. For a non-array resource this value should be 1.
};

/// \brief This describes shader code variable.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALShaderVariableDescription : public xiiHashableStruct<xiiGALShaderVariableDescription>
{
  xiiString                                        m_sName;                                                  ///< The variable name.
  xiiEnum<xiiGALShaderVariableClassType>           m_Class         = xiiGALShaderVariableClassType::Unknown; ///< The variable class.
  xiiEnum<xiiGALShaderPrimitiveType>               m_PrimitiveType = xiiGALShaderPrimitiveType::Unknown;     ///< The variable primitive data type..
  xiiUInt8                                         m_uiRowCount    = 0U;                                     ///< For a matrix type, the number of rows.
  xiiUInt8                                         m_uiColumnCount = 0U;                                     ///< For a matrix type, the number of columns.
  xiiUInt8                                         m_uiOffset      = 0U;                                     ///< The offset in bytes between the start of the parent structure and this variable.
  xiiUInt8                                         m_uiArraySize   = 0U;                                     ///< The array size.
  xiiDynamicArray<xiiGALShaderVariableDescription> m_Members;                                                ///< For a structure, an array of member variables.
};

/// \brief This describes a shader constant buffer.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALShaderBufferDescription : public xiiHashableStruct<xiiGALShaderBufferDescription>
{
  xiiString                                        m_sName;       ///< The variable name.
  xiiUInt32                                        m_uiSize = 0U; ///< The size of the buffer in bytes.
  xiiDynamicArray<xiiGALShaderVariableDescription> m_Variables;   ///< An array of member variables.
};

/// \brief This describes the shader creation description.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALShaderCreationDescription : public xiiHashableStruct<xiiGALShaderCreationDescription>
{
  xiiGALShaderCreationDescription();
  ~xiiGALShaderCreationDescription();

  bool HasByteCodeForStage(xiiGALShaderStage::Enum stage) const;

  xiiStringView                  m_sName;                                                    ///< Resource name. The default is an empty string view.
  xiiBitflags<xiiGALShaderStage> m_ShaderStage                 = xiiGALShaderStage::Unknown; ///< The shader stages. The default is xiiGALShaderStage::Unknown.
  bool                           m_bUseCombinedTextureSamplers = false;                      ///< If set to true, textures will be combined with texture samplers.
                                                                                             ///<
                                                                                             ///< The m_bCombinedSamplerSuffix member defines the suffix added to the texture variable name to get corresponding sampler name.
                                                                                             ///< When using combined samplers, the sampler assigned to the shader resource view is automatically set when
                                                                                             ///< the view is bound. Otherwise, samplers need to be explicitly set similar to other shader variables.
                                                                                             ///<
                                                                                             ///< This member has no effect if the shader is used in the PSO that uses pipeline resource signature(s).

  xiiStringView m_sCombinedSamplerSuffix = "_Sampler"; ///< If m_bUseCombinedTextureSamplers is true, defines the suffix added to the texture variable name to get corresponding sampler name. For example,
                                                       ///< for the default value "_Sampler", a texture named "Tex" will be combined with the sampler named "Tex_Sampler". If m_bUseCombinedTextureSamplers is false, this member is ignored.
                                                       ///<
                                                       ///< This member has no effect if the shader is used in the PSO that uses pipeline resource signature(s).

  xiiStaticArray<xiiScopedRefPointer<xiiGALShaderByteCode>, xiiGALShaderStage::ENUM_COUNT> m_ByteCodes; ///< The shader byte code per stage.
};

/// \brief Interface that defines methods to manipulate a shader object.
class XII_GRAPHICSFOUNDATION_DLL xiiGALShader : public xiiGALResource<xiiGALShaderCreationDescription>
{
public:
  /// \brief This returns the total number of shader resources.
  virtual xiiUInt32 GetResourceCount() const = 0;

  /// \brief This returns a pointer to the array of shader resources.
  virtual void GetResourceDescription(xiiUInt32 uiIndex, xiiGALShaderResourceDescription& out_ResourceDescription) const = 0;

protected:
  friend class xiiGALDevice;

  xiiGALShader(const xiiGALShaderCreationDescription& creationDescription);

  virtual ~xiiGALShader();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) = 0;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) = 0;
};

#include <GraphicsFoundation/Shader/Implementation/Shader_inl.h>
