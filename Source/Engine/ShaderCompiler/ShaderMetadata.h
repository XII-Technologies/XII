#pragma once

#include <ShaderCompiler/ShaderCompilerDLL.h>

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/Stream.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

/// \brief Shader Descriptor Layout Binding.
struct XII_SHADERCOMPILER_DLL xiiShaderDescriptorSetLayoutBinding
{
  XII_DECLARE_POD_TYPE();

  enum ResourceType : xiiUInt8
  {
    ConstantBuffer,
    ResourceView,
    UnorderedAccessView,
    Sampler,
  };

  xiiStringView               m_sName;                                              ///< Used to match the same descriptor use across multiple stages.
  xiiUInt8                    m_uiBinding         = 0;                              ///< Target descriptor binding slot.
  xiiUInt8                    m_uiVirtualBinding  = 0;                              ///< Virtual binding slot in the high level renderer interface.
  xiiShaderResourceType::Enum m_xiiType           = xiiShaderResourceType::Unknown; ///< XII shader resource type, needed to find compatible fallback resources.
  ResourceType                m_Type              = ResourceType::ConstantBuffer;   ///< Resource type, used to map to the correct XII resource type.
  xiiUInt16                   m_uiDescriptorType  = 0;                              ///< Maps to vk::DescriptorType
  xiiUInt32                   m_uiDescriptorCount = 1;                              ///< For now, this must be 1 as XII does not support descriptor arrays currently.
  xiiUInt32                   m_uiWordOffset      = 0;                              ///< Offset of the location in the spirv or hlsl code where the binding index is located to allow changing it at runtime.
};

/// \brief Shader Descriptor Set Layout.
struct XII_SHADERCOMPILER_DLL xiiShaderDescriptorSetLayout
{
  xiiUInt32                                              m_uiSet = 0;
  xiiHybridArray<xiiShaderDescriptorSetLayoutBinding, 6> Bindings;
};

/// \brief Shader Vertex Input Attributes.
struct XII_SHADERCOMPILER_DLL xiiShaderVertexInputAttribute
{
  xiiGALVertexAttributeSemantic::Enum m_eSemantic       = xiiGALVertexAttributeSemantic::Position;
  xiiUInt8                            m_uiSemanticIndex = 0;
  xiiGALResourceFormat::Enum          m_eFormat         = xiiGALResourceFormat::XYZFloat;
};

class XII_SHADERCOMPILER_DLL xiiShaderMetaData
{
public:
  static constexpr xiiUInt32 x_uiMetaDataTag = 0x58494958; // XIIX

  enum MetaDataVersion
  {
    Version1 = 1,
    Version2 = 2, ///< m_uiVirtualBinding, m_xiiType added
    Version3 = 3, ///< Vertex input binding

    ENUM_COUNT,

    CurrentVerion = ENUM_COUNT - 1
  };

  /// \brief Writes the custom shader bytecode format to a stream.
  static void Write(xiiStreamWriter& stream, const xiiArrayPtr<xiiUInt8>& shaderCode, const xiiDynamicArray<xiiShaderDescriptorSetLayout>& sets, const xiiDynamicArray<xiiShaderVertexInputAttribute>& vertexInputAttributes);

  /// \brief Reads Shader code and meta data from a data buffer. Note that 'data' must be kept alive for the lifetime of the shader as this functions stores views into this memory in its out parameters.
  /// \param data Raw data buffer to read the shader code and meta data from.
  /// \param out_shaderCode Will be filled with a view into data that contains the shader byte code.
  /// \param out_sets Will be filled with shader meta data. Note that this array contains string views into 'data'.
  static void Read(const xiiArrayPtr<const xiiUInt8> data, xiiArrayPtr<const xiiUInt8>& out_shaderCode, xiiDynamicArray<xiiShaderDescriptorSetLayout>& out_sets, xiiDynamicArray<xiiShaderVertexInputAttribute>& out_vertexInputAttributes);
};
