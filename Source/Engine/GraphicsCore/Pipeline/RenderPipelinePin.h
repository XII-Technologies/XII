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
  using Type                                  = xiiRenderPipelinePinType::Enum;
  static constexpr Type               Texture = xiiRenderPipelinePinType::Texture;
  static constexpr Type               Buffer  = xiiRenderPipelinePinType::Buffer;
  static constexpr Type               Order   = xiiRenderPipelinePinType::Order;
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
