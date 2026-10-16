/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Uuid.h>
#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsFoundation/Declarations/GraphicsTypes.h>

struct xiiRenderPipelinePinType
{
  using StorageType = xiiUInt8;
  enum Enum : StorageType
  {
    Texture,
    Buffer,
    Order,
    AccelerationStructure,
    Default = Texture
  };
};
struct xiiRenderPipelinePinAccess
{
  using StorageType = xiiUInt8;
  enum Enum : StorageType
  {
    Automatic,
    Read,
    Write,
    ReadWrite,
    Ordering,
    Default = Automatic
  };
};
struct xiiRenderPipelinePinCardinality
{
  using StorageType = xiiUInt8;
  enum Enum : StorageType
  {
    Single,
    Multiple,
    Default = Single
  };
};
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelinePinType);
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelinePinAccess);
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelinePinCardinality);

/// Authored requirements. Zero/Undefined accepts the producer's description.
/// These constrain resources; they do not silently change an algorithm's allocations.
struct XII_GRAPHICSCORE_DLL xiiRenderPipelinePinConstraints
{
  xiiEnum<xiiGALResourceDimension> m_Dimension = xiiGALResourceDimension::Undefined;
  xiiUInt32                        m_uiSamples = 0;
  xiiUInt32                        m_uiWidth   = 0;
  xiiUInt32                        m_uiHeight  = 0;
  xiiBitflags<xiiGALBindFlags>     m_RequiredBindFlags;
};
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelinePinConstraints);

/// Read-only authoring presentation of the native pass schema. The native schema
/// remains authoritative; serialized metadata cannot redefine a pass's pins.
struct XII_GRAPHICSCORE_DLL xiiRenderPipelinePinMetadata
{
  xiiString                                m_sName;
  bool                                     m_bInput = true;
  xiiEnum<xiiRenderPipelinePinType>        m_Type;
  xiiEnum<xiiRenderPipelinePinAccess>      m_Access;
  xiiEnum<xiiRenderPipelinePinCardinality> m_Cardinality;
  bool                                     m_bOptional        = false;
  bool                                     m_bTemporalHistory = false;
  xiiEnum<xiiGALResourceFormat>            m_DeclaredFormat   = xiiGALResourceFormat::Unknown;
};
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelinePinMetadata);

class XII_GRAPHICSCORE_DLL xiiRenderPipelinePinConfiguration : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderPipelinePinConfiguration, xiiReflectedClass);

public:
  xiiUuid                         m_Id; ///< Persistent document identity, never a graph-owned handle.
  xiiRenderPipelinePinMetadata    m_Metadata;
  xiiString                       m_sDisplayName;
  xiiEnum<xiiGALResourceFormat>   m_Format = xiiGALResourceFormat::Unknown;
  xiiRenderPipelinePinConstraints m_Constraints;
};

struct xiiGALTextureCreationDescription;
struct xiiGALBufferCreationDescription;

/// Supplied by the reflected pass type, then narrowed by authored requirements.
struct XII_GRAPHICSCORE_DLL xiiRenderPipelinePin
{
  using Type                                                = xiiRenderPipelinePinType::Enum;
  static constexpr Type               Texture               = xiiRenderPipelinePinType::Texture;
  static constexpr Type               Buffer                = xiiRenderPipelinePinType::Buffer;
  static constexpr Type               Order                 = xiiRenderPipelinePinType::Order;
  static constexpr Type               AccelerationStructure = xiiRenderPipelinePinType::AccelerationStructure;
  xiiString                           m_sName;
  Type                                m_Type                   = Texture;
  bool                                m_bInput                 = true;
  bool                                m_bOptional              = false;
  xiiGALResourceFormat::Enum          m_Format                 = xiiGALResourceFormat::Unknown;
  bool                                m_bTemporalHistoryOutput = false;
  xiiRenderPipelinePinConstraints     m_Constraints;
  xiiEnum<xiiRenderPipelinePinAccess> m_Access;
  xiiUuid                             m_SourceId;
  xiiUuid                             m_Id; ///< Compiled occurrence identity, scoped by its pipeline definition.
  xiiString                           m_sDisplayName;

  xiiRenderPipelinePinMetadata GetMetadata() const;
  xiiResult                    Apply(const xiiRenderPipelinePinConfiguration& configuration, xiiStringBuilder* out_error = nullptr);
  bool                         Accepts(const xiiGALTextureCreationDescription& description) const;
  bool                         Accepts(const xiiGALBufferCreationDescription& description) const;
  bool                         IsCompatibleWith(const xiiRenderPipelinePin& other) const;
};

/// A native pass member declared with XII_MEMBER_PROPERTY. Its type fixes the
/// resource kind and direction. The compiler assigns an occurrence-local binding
/// index; this is neither a graph handle nor a persistent document identity.
/// Authored requirements remain in the document's persistent pin configurations.
class XII_GRAPHICSCORE_DLL xiiRenderPipelinePinHandle : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderPipelinePinHandle, xiiReflectedClass);

public:
  xiiUInt32 GetBindingIndex() const { return m_uiBindingIndex; }

protected:
  xiiRenderPipelinePinHandle(xiiRenderPipelinePin::Type type, bool input, bool optional,
                             xiiGALResourceFormat::Enum format, xiiBitflags<xiiGALBindFlags> bindings,
                             xiiGALResourceDimension::Enum dimension, xiiUInt32 samples,
                             xiiRenderPipelinePinAccess::Enum access, bool temporal);

private:
  friend class xiiRenderPipelinePass;
  xiiRenderPipelinePin m_Declaration;
  mutable xiiUInt32    m_uiBindingIndex = xiiInvalidIndex;
};

class XII_GRAPHICSCORE_DLL xiiRenderPipelineTextureInputPin : public xiiRenderPipelinePinHandle
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderPipelineTextureInputPin, xiiRenderPipelinePinHandle);

public:
  explicit xiiRenderPipelineTextureInputPin(xiiGALResourceFormat::Enum format = xiiGALResourceFormat::Unknown,
                                            bool optional = false, xiiBitflags<xiiGALBindFlags> bindings = {},
                                            xiiGALResourceDimension::Enum dimension = xiiGALResourceDimension::Undefined, xiiUInt32 samples = 0);
};
class XII_GRAPHICSCORE_DLL xiiRenderPipelineTextureOutputPin : public xiiRenderPipelinePinHandle
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderPipelineTextureOutputPin, xiiRenderPipelinePinHandle);

public:
  explicit xiiRenderPipelineTextureOutputPin(xiiGALResourceFormat::Enum format = xiiGALResourceFormat::Unknown,
                                             bool optional = false, xiiBitflags<xiiGALBindFlags> bindings = {},
                                             xiiGALResourceDimension::Enum dimension = xiiGALResourceDimension::Undefined, xiiUInt32 samples = 0,
                                             xiiRenderPipelinePinAccess::Enum access = xiiRenderPipelinePinAccess::Automatic, bool temporal = false);
};
class XII_GRAPHICSCORE_DLL xiiRenderPipelineBufferInputPin : public xiiRenderPipelinePinHandle
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderPipelineBufferInputPin, xiiRenderPipelinePinHandle);

public:
  explicit xiiRenderPipelineBufferInputPin(bool optional = false, xiiBitflags<xiiGALBindFlags> bindings = {});
};
class XII_GRAPHICSCORE_DLL xiiRenderPipelineBufferOutputPin : public xiiRenderPipelinePinHandle
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderPipelineBufferOutputPin, xiiRenderPipelinePinHandle);

public:
  explicit xiiRenderPipelineBufferOutputPin(bool optional = false, xiiBitflags<xiiGALBindFlags> bindings = {},
                                            xiiRenderPipelinePinAccess::Enum access = xiiRenderPipelinePinAccess::Automatic, bool temporal = false);
};
/// A TLAS plus its explicit render-graph build dependency. Neither is authoring state.
class XII_GRAPHICSCORE_DLL xiiRenderPipelineAccelerationStructureInputPin : public xiiRenderPipelinePinHandle
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderPipelineAccelerationStructureInputPin, xiiRenderPipelinePinHandle);

public:
  explicit xiiRenderPipelineAccelerationStructureInputPin(bool optional = false);
};
class XII_GRAPHICSCORE_DLL xiiRenderPipelineAccelerationStructureOutputPin : public xiiRenderPipelinePinHandle
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderPipelineAccelerationStructureOutputPin, xiiRenderPipelinePinHandle);

public:
  explicit xiiRenderPipelineAccelerationStructureOutputPin(bool optional = false, xiiRenderPipelinePinAccess::Enum access = xiiRenderPipelinePinAccess::Automatic);
};

class XII_GRAPHICSCORE_DLL xiiRenderPipelineOrderInputPin : public xiiRenderPipelinePinHandle
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderPipelineOrderInputPin, xiiRenderPipelinePinHandle);

public:
  xiiRenderPipelineOrderInputPin();
};
class XII_GRAPHICSCORE_DLL xiiRenderPipelineOrderOutputPin : public xiiRenderPipelinePinHandle
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderPipelineOrderOutputPin, xiiRenderPipelinePinHandle);

public:
  xiiRenderPipelineOrderOutputPin();
};

/// Optional serialization name for an input/output pair with the same external
/// name. RTTI member property names themselves must remain unique.
class XII_GRAPHICSCORE_DLL xiiRenderPipelinePinNameAttribute : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderPipelinePinNameAttribute, xiiPropertyAttribute);

public:
  xiiRenderPipelinePinNameAttribute() = default;
  explicit xiiRenderPipelinePinNameAttribute(const char* name) : m_sName(name) {}
  xiiString m_sName;
};

/// Read optionality from a reflected boolean pass member. Pin discovery remains
/// valid even while unrelated authored properties are temporarily invalid.
class XII_GRAPHICSCORE_DLL xiiRenderPipelinePinOptionalAttribute : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderPipelinePinOptionalAttribute, xiiPropertyAttribute);

public:
  xiiRenderPipelinePinOptionalAttribute() = default;
  explicit xiiRenderPipelinePinOptionalAttribute(const char* property) : m_sProperty(property) {}
  xiiString m_sProperty;
};

/// Declarative presence for genuinely dynamic authored pin members.
class XII_GRAPHICSCORE_DLL xiiRenderPipelinePinEnabledAttribute : public xiiPropertyAttribute
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderPipelinePinEnabledAttribute, xiiPropertyAttribute);

public:
  xiiRenderPipelinePinEnabledAttribute() = default;
  explicit xiiRenderPipelinePinEnabledAttribute(const char* property) : m_sProperty(property) {}
  xiiString m_sProperty;
};
