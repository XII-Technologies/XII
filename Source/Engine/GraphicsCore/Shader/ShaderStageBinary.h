#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Enum.h>
#include <GraphicsFoundation/Declarations/Descriptors.h>

class XII_GRAPHICSCORE_DLL xiiShaderConstantBufferLayout : public xiiRefCounted
{
public:
  struct Constant
  {
    XII_DECLARE_MEM_RELOCATABLE_TYPE();

    struct Type
    {
      using StorageType = xiiUInt8;

      enum Enum
      {
        Default,
        Float1,
        Float2,
        Float3,
        Float4,
        Int1,
        Int2,
        Int3,
        Int4,
        UInt1,
        UInt2,
        UInt3,
        UInt4,
        Mat3x3,
        Mat4x4,
        Transform,
        Bool,
        Struct,

        ENUM_COUNT
      };
    };

    static xiiUInt32 s_TypeSize[Type::ENUM_COUNT];

    Constant()
    {
      m_uiArrayElements = 0;
      m_uiOffset        = 0;
    }

    void CopyDataFormVariant(xiiUInt8* pDest, xiiVariant* pValue) const;

    xiiHashedString m_sName;
    xiiEnum<Type>   m_Type;
    xiiUInt8        m_uiArrayElements;
    xiiUInt16       m_uiOffset;
  };

private:
  friend class xiiShaderStageBinary;
  friend class xiiMemoryUtils;

  xiiShaderConstantBufferLayout();
  ~xiiShaderConstantBufferLayout();

public:
  xiiResult Write(xiiStreamWriter& inout_stream) const;
  xiiResult Read(xiiStreamReader& inout_stream);

  xiiUInt32                    m_uiTotalSize;
  xiiHybridArray<Constant, 16> m_Constants;
};

struct XII_GRAPHICSCORE_DLL xiiShaderResourceBinding
{
  XII_DECLARE_MEM_RELOCATABLE_TYPE();


  xiiShaderResourceBinding();
  ~xiiShaderResourceBinding();

  xiiEnum<xiiGALShaderResourceType>                  m_Type;
  xiiInt32                                           m_iSlot;
  xiiHashedString                                    m_sName;
  xiiScopedRefPointer<xiiShaderConstantBufferLayout> m_pLayout;
};

class XII_GRAPHICSCORE_DLL xiiShaderStageBinary
{
public:
  enum Version
  {
    Version0,

    ENUM_COUNT,
    VersionCurrent = ENUM_COUNT - 1
  };

  xiiShaderStageBinary();
  ~xiiShaderStageBinary();

  xiiResult Write(xiiStreamWriter& inout_stream) const;
  xiiResult Read(xiiStreamReader& inout_stream);

  xiiDynamicArray<xiiUInt8>& GetByteCode();

  void                                        AddShaderResourceBinding(const xiiShaderResourceBinding& binding);
  xiiArrayPtr<const xiiShaderResourceBinding> GetShaderResourceBindings() const;
  const xiiShaderResourceBinding*             GetShaderResourceBinding(const xiiTempHashedString& sName) const;

  xiiShaderConstantBufferLayout* CreateConstantBufferLayout() const;

private:
  friend class xiiRenderContext;
  friend class xiiShaderCompiler;
  friend class xiiShaderPermutationResource;
  friend class xiiShaderPermutationResourceLoader;

  xiiUInt32                                   m_uiSourceHash = 0;
  xiiBitflags<xiiGALShaderStage>              m_Stage        = xiiGALShaderStage::ENUM_COUNT;
  xiiDynamicArray<xiiUInt8>                   m_ByteCode;
  xiiScopedRefPointer<xiiGALShaderByteCode>   m_GALByteCode;
  xiiHybridArray<xiiShaderResourceBinding, 8> m_ShaderResourceBindings;
  bool                                        m_bWasCompiledWithDebug = false;

  xiiResult                    WriteStageBinary(xiiLogInterface* pLog) const;
  static xiiShaderStageBinary* LoadStageBinary(xiiBitflags<xiiGALShaderStage> Stage, xiiUInt32 uiHash);

  static void OnEngineShutdown();

  static xiiMap<xiiUInt32, xiiShaderStageBinary> s_ShaderStageBinaries[xiiGALShaderStage::ENUM_COUNT];
};
