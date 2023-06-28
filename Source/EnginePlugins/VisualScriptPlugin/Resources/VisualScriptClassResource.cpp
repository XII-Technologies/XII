#include <VisualScriptPlugin/VisualScriptPluginPCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <VisualScriptPlugin/Resources/VisualScriptClassResource.h>
#include <VisualScriptPlugin/Runtime/VisualScriptInstance.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptClassResource, 1, xiiRTTIDefaultAllocator<xiiVisualScriptClassResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;
XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiVisualScriptClassResource);

XII_BEGIN_SUBSYSTEM_DECLARATION(TypeScript, Resource)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ResourceManager" 
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP 
  {
    xiiResourceManager::RegisterResourceForAssetType("VisualScriptClass", xiiGetStaticRTTI<xiiVisualScriptClassResource>());
    xiiResourceManager::RegisterResourceOverrideType(xiiGetStaticRTTI<xiiVisualScriptClassResource>(), [](const xiiStringBuilder& sResourceID) -> bool  {
        return sResourceID.HasExtension(".xiiVisualScriptClassBin");
      });
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiResourceManager::UnregisterResourceOverrideType(xiiGetStaticRTTI<xiiVisualScriptClassResource>());
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

xiiVisualScriptClassResource::xiiVisualScriptClassResource() = default;
xiiVisualScriptClassResource::~xiiVisualScriptClassResource() = default;

xiiResourceLoadDesc xiiVisualScriptClassResource::UnloadData(Unload WhatToUnload)
{
  DeleteScriptType();

  xiiResourceLoadDesc ld;
  ld.m_State = xiiResourceState::Unloaded;
  ld.m_uiQualityLevelsDiscardable = 0;
  ld.m_uiQualityLevelsLoadable = 0;

  return ld;
}

xiiResourceLoadDesc xiiVisualScriptClassResource::UpdateContent(xiiStreamReader* pStream)
{
  xiiResourceLoadDesc ld;
  ld.m_uiQualityLevelsDiscardable = 0;
  ld.m_uiQualityLevelsLoadable = 0;
  ld.m_State = xiiResourceState::LoadedResourceMissing;

  if (pStream == nullptr)
  {
    return ld;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    xiiString sAbsFilePath;
    (*pStream) >> sAbsFilePath;
  }

  // skip the asset file header at the start of the file
  xiiAssetFileHeader AssetHash;
  AssetHash.Read(*pStream).IgnoreResult();

  xiiString sScriptClassName;
  const xiiRTTI* pBaseClassType = nullptr;
  xiiScriptRTTI::FunctionList functions;
  xiiScriptRTTI::MessageHandlerList messageHandlers;
  {
    xiiStringDeduplicationReadContext stringDedup(*pStream);

    xiiChunkStreamReader chunk(*pStream);
    chunk.SetEndChunkFileMode(xiiChunkStreamReader::EndChunkFileMode::JustClose);

    chunk.BeginStream();

    // skip all chunks that we don't know
    while (chunk.GetCurrentChunk().m_bValid)
    {
      if (chunk.GetCurrentChunk().m_sChunkName == "Header")
      {
        xiiString sBaseClassName;
        chunk >> sBaseClassName;
        chunk >> sScriptClassName;
        pBaseClassType = xiiRTTI::FindTypeByName(sBaseClassName);
        if (pBaseClassType == nullptr)
        {
          xiiLog::Error("Invalid base class '{}' for Visual Script Class '{}'", sBaseClassName, sScriptClassName);
          return ld;
        }
      }
      else if (chunk.GetCurrentChunk().m_sChunkName == "FunctionGraphs")
      {
        xiiUInt32 uiNumFunctions;
        chunk >> uiNumFunctions;

        for (xiiUInt32 i = 0; i < uiNumFunctions; ++i)
        {
          xiiString sFunctionName;
          xiiEnum<xiiVisualScriptNodeDescription::Type> functionType;
          chunk >> sFunctionName;
          chunk >> functionType;

          xiiUniquePtr<xiiVisualScriptGraphDescription> pDesc = XII_DEFAULT_NEW(xiiVisualScriptGraphDescription);
          if (pDesc->Deserialize(chunk).Failed())
          {
            xiiLog::Error("Invalid visual script desc");
            return ld;
          }

          if (functionType == xiiVisualScriptNodeDescription::Type::EntryCall)
          {
            xiiUniquePtr<xiiVisualScriptFunctionProperty> pFunctionProperty = XII_DEFAULT_NEW(xiiVisualScriptFunctionProperty, sFunctionName, std::move(pDesc));
            functions.PushBack(std::move(pFunctionProperty));
          }
          else if (functionType == xiiVisualScriptNodeDescription::Type::MessageHandler)
          {
          }
          else
          {
            xiiLog::Error("Invalid event handler type {} for event handler '{}'", functionType, sFunctionName);
            return ld;
          }
        }
      }
      else if (chunk.GetCurrentChunk().m_sChunkName == "ConstantData")
      {
        xiiSharedPtr<xiiVisualScriptDataDescription> pConstantDataDesc = XII_DEFAULT_NEW(xiiVisualScriptDataDescription);
        if (pConstantDataDesc->Deserialize(chunk).Failed())
        {
          return ld;
        }

        xiiSharedPtr<xiiVisualScriptDataStorage> pConstantDataStorage = XII_DEFAULT_NEW(xiiVisualScriptDataStorage, pConstantDataDesc);
        if (pConstantDataStorage->Deserialize(chunk).Succeeded())
        {
          m_pConstantDataStorage = pConstantDataStorage;
        }
      }
      else if (chunk.GetCurrentChunk().m_sChunkName == "VariableDataDesc")
      {
        xiiSharedPtr<xiiVisualScriptDataDescription> pVariableDataDesc = XII_DEFAULT_NEW(xiiVisualScriptDataDescription);
        if (pVariableDataDesc->Deserialize(chunk).Succeeded())
        {
          m_pVariableDataDesc = pVariableDataDesc;
        }
      }

      chunk.NextChunk();
    }
  }

  CreateScriptType(sScriptClassName, pBaseClassType, std::move(functions), std::move(messageHandlers));

  ld.m_State = xiiResourceState::Loaded;
  return ld;
}

void xiiVisualScriptClassResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = (xiiUInt32)sizeof(xiiVisualScriptClassResource);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

xiiUniquePtr<xiiScriptInstance> xiiVisualScriptClassResource::Instantiate(xiiReflectedClass& owner, xiiWorld* pWorld) const
{
  return XII_DEFAULT_NEW(xiiVisualScriptInstance, owner, pWorld, m_pConstantDataStorage, m_pVariableDataDesc);
}
