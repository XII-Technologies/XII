#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Shader/Shader.h>

/// brief This describes the type of shader resource variable.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALShaderResourceVariableType
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Static = 0U, ///< Shader resource bound to the variable is the same for all SRB instances. It must be set *once* directly through Pipeline State object.
    Mutable,     ///< Shader resource bound to the variable is specific to the shader resource binding instance. It must be set *once* through the shader resource binding interface. It cannot be set through the pipeline state interface, and cannot be changed once bound.
    Dynamic,     ///< Shader variable binding is dynamic. It can be set multiple times for every instance of shader resource binding. It cannot be set through the pipeline state interface.

    ENUM_COUNT,

    Default = Static
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALShaderResourceVariableType);

/// brief This describes the shader resource variable type flags.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALShaderResourceVariableTypeFlags
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None    = 0U,                                                 ///< No flags.
    Static  = XII_BIT(xiiGALShaderResourceVariableType::Static),  ///< Static variable type flag.
    Mutable = XII_BIT(xiiGALShaderResourceVariableType::Mutable), ///< Mutable variable type flag.
    Dynamic = XII_BIT(xiiGALShaderResourceVariableType::Dynamic), ///< Dynamic variable type flag.

    ENUM_COUNT = 4U,

    MutableDynamic = Mutable | Dynamic,          ///< Mutable and dynamic variable type flags.
    All            = Static | Mutable | Dynamic, ///< All variable type flags.

    Default = None
  };

  struct Bits
  {
    StorageType Static : 1;
    StorageType Mutable : 1;
    StorageType Dynamic : 1;

    StorageType MutableDynamic : 1;
    StorageType All : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiGALShaderResourceVariableTypeFlags);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALShaderResourceVariableTypeFlags);

/// brief This describes the shader resource bind flags.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALShaderResourceBindFlags
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Static  = xiiGALShaderResourceVariableTypeFlags::Static,  ///< Indicates that static shader variable bindings are to be updated.
    Mutable = xiiGALShaderResourceVariableTypeFlags::Mutable, ///< Indicates that mutable shader variable bindings are to be updated.
    Dynamic = xiiGALShaderResourceVariableTypeFlags::Dynamic, ///< Indicates that dynamic shader variable bindings are to be updated.

    ENUM_COUNT = 3U,

    All = xiiGALShaderResourceVariableTypeFlags::All, ///< Indicates that all shader variable types (static, mutable and dynamic) are to be updated.
                                                      ///<
                                                      ///< \note If none of xiiGALShaderResourceBindFlags::Static, xiiGALShaderResourceBindFlags::Mutable, and xiiGALShaderResourceBindFlags::Dynamic flags are set, all variables are updated as if xiiGALShaderResourceBindFlags::All was set.
    KeepExisting      = 0x08,                         ///< If this flag is specified, all existing bindings will be preserved and only unresolved ones will be updated. If this flag is not specified, every shader variable will be updated if the mapping contains corresponding resource.
    VerifyAllResolved = 0x10,                         ///< If this flag is specified, all shader bindings are expected to be resolved after the call. If this is not the case, debug message will be displayed.
                                                      ///<
                                                      ///< \note Only these variables are verified that are being updated by setting xiiGALShaderResourceBindFlags::Static, xiiGALShaderResourceBindFlags::Mutable, and xiiGALShaderResourceBindFlags::Dynamic.
    AllowOverWrite = 0x20,                            ///< Allow overwriting static and mutable variables.

    Default = Static
  };

  struct Bits
  {
    StorageType Static : 1;
    StorageType Mutable : 1;
    StorageType Dynamic : 1;

    StorageType All : 1;
    StorageType KeepExisting : 1;
    StorageType VerifyAllResolved : 1;
    StorageType AllowOverwrite : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiGALShaderResourceBindFlags);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALShaderResourceBindFlags);

/// brief This describes the shader setting flags.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALSetShaderResourceFlags
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None           = 0U,         ///< No flags.
    AllowOverwrite = XII_BIT(0), ///< Allow overwriting static and mutable variable bindings.
                                 ///< \remarks By default, static and mutable variables can't be changed once initialized to a non-null resource. This flag is required to explicitly allow overwriting the binding.
                                 ///<
                                 ///< Overwriting static variables does not require synchronization with GPU and does not have effect on shader resource binding objects already created from the pipeline state or resource signature.
                                 ///<
                                 ///< When overwriting a mutable variable binding in Direct3D12 and Vulkan, an application must ensure that the GPU is not accessing the SRB. This can be achieved using syncrhonization tools such as fences.
                                 ///< Synchronization with GPU is not required in Direct3D11, and Metal backends.

    ENUM_COUNT = 2U,

    Default = None
  };

  struct Bits
  {
    StorageType AllowOverwrite : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiGALSetShaderResourceFlags);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALSetShaderResourceFlags);

/// \brief This describes the shader resource variable creation description.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALShaderResourceVariableCreationDescription : public xiiHashableStruct<xiiGALShaderResourceVariableCreationDescription>
{
  XII_DECLARE_POD_TYPE();
};
#include <GraphicsFoundation/Shader/Implementation/ShaderResourceVariable_inl.h>
