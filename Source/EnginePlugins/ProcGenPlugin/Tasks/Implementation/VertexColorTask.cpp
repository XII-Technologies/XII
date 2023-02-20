#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <ProcGenPlugin/Components/VolumeCollection.h>
#include <ProcGenPlugin/Tasks/Utils.h>
#include <ProcGenPlugin/Tasks/VertexColorTask.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>

namespace
{
  template <typename T>
  XII_ALWAYS_INLINE xiiProcessingStream MakeStream(xiiArrayPtr<T> data, xiiUInt32 uiOffset, const xiiHashedString& sName, xiiProcessingStream::DataType dataType = xiiProcessingStream::DataType::Float)
  {
    return xiiProcessingStream(sName, data.ToByteArray().GetSubArray(uiOffset), dataType, sizeof(T));
  }

  XII_ALWAYS_INLINE float Remap(xiiEnum<xiiProcVertexColorChannelMapping> channelMapping, const xiiColor& srcColor)
  {
    if (channelMapping >= xiiProcVertexColorChannelMapping::R && channelMapping <= xiiProcVertexColorChannelMapping::A)
    {
      return (&srcColor.r)[channelMapping];
    }
    else
    {
      return channelMapping == xiiProcVertexColorChannelMapping::White ? 1.0f : 0.0f;
    }
  }
} // namespace

using namespace xiiProcGenInternal;

VertexColorTask::VertexColorTask()
{
  m_VM.RegisterFunction(xiiProcGenExpressionFunctions::s_ApplyVolumesFunc);
  m_VM.RegisterFunction(xiiProcGenExpressionFunctions::s_GetInstanceSeedFunc);
}

VertexColorTask::~VertexColorTask() = default;

void VertexColorTask::Prepare(const xiiWorld& world, const xiiMeshBufferResourceDescriptor& mbDesc, const xiiTransform& transform, xiiArrayPtr<xiiSharedPtr<const VertexColorOutput>> outputs, xiiArrayPtr<xiiProcVertexColorMapping> outputMappings, xiiArrayPtr<xiiUInt32> outputVertexColors)
{
  XII_PROFILE_SCOPE("VertexColorPrepare");

  m_InputVertices.Clear();
  m_InputVertices.Reserve(mbDesc.GetVertexCount());

  const xiiVertexDeclarationInfo& vdi            = mbDesc.GetVertexDeclaration();
  const xiiUInt8*                 pRawVertexData = mbDesc.GetVertexBufferData().GetPtr();

  const float*               pPositions   = nullptr;
  const xiiUInt8*            pNormals     = nullptr;
  xiiGALResourceFormat::Enum normalFormat = xiiGALResourceFormat::Invalid;
  const xiiColorLinearUB*    pColors      = nullptr;

  for (xiiUInt32 vs = 0; vs < vdi.m_VertexStreams.GetCount(); ++vs)
  {
    if (vdi.m_VertexStreams[vs].m_Semantic == xiiGALVertexAttributeSemantic::Position)
    {
      if (vdi.m_VertexStreams[vs].m_Format != xiiGALResourceFormat::RGBFloat)
      {
        xiiLog::Error("Unsupported CPU mesh vertex position format {0}", (int)vdi.m_VertexStreams[vs].m_Format);
        return; // other position formats are not supported
      }

      pPositions = reinterpret_cast<const float*>(pRawVertexData + vdi.m_VertexStreams[vs].m_uiOffset);
    }
    else if (vdi.m_VertexStreams[vs].m_Semantic == xiiGALVertexAttributeSemantic::Normal)
    {
      pNormals     = pRawVertexData + vdi.m_VertexStreams[vs].m_uiOffset;
      normalFormat = vdi.m_VertexStreams[vs].m_Format;
    }
    else if (vdi.m_VertexStreams[vs].m_Semantic == xiiGALVertexAttributeSemantic::Color0)
    {
      if (vdi.m_VertexStreams[vs].m_Format != xiiGALResourceFormat::RGBAUByteNormalized)
      {
        xiiLog::Error("Unsupported CPU mesh vertex color format {0}", (int)vdi.m_VertexStreams[vs].m_Format);
        return; // other color formats are not supported
      }

      pColors = reinterpret_cast<const xiiColorLinearUB*>(pRawVertexData + vdi.m_VertexStreams[vs].m_uiOffset);
    }
  }

  if (pPositions == nullptr || pNormals == nullptr)
  {
    xiiLog::Error("No position and normal stream found in CPU mesh");
    return;
  }

  xiiUInt8 dummySource[16] = {};
  xiiVec3  vNormal;
  if (xiiMeshBufferUtils::DecodeNormal(xiiMakeArrayPtr(dummySource), normalFormat, vNormal).Failed())
  {
    xiiLog::Error("Unsupported CPU mesh vertex normal format {0}", normalFormat);
    return;
  }

  xiiMat3 normalTransform = transform.GetAsMat4().GetRotationalPart();
  normalTransform.Invert(0.0f).IgnoreResult();
  normalTransform.Transpose();

  const xiiUInt32 uiElementStride = mbDesc.GetVertexDataSize();

  // write out all vertices
  for (xiiUInt32 i = 0; i < mbDesc.GetVertexCount(); ++i)
  {
    xiiMeshBufferUtils::DecodeNormal(xiiMakeArrayPtr(pNormals, sizeof(xiiVec3)), normalFormat, vNormal).IgnoreResult();

    auto& vert       = m_InputVertices.ExpandAndGetRef();
    vert.m_vPosition = transform.TransformPosition(xiiVec3(pPositions[0], pPositions[1], pPositions[2]));
    vert.m_vNormal   = normalTransform.TransformDirection(vNormal).GetNormalized();
    vert.m_Color     = pColors != nullptr ? xiiColor(*pColors) : xiiColor::ZeroColor();
    vert.m_uiIndex   = i;

    pPositions = xiiMemoryUtils::AddByteOffset(pPositions, uiElementStride);
    pNormals   = xiiMemoryUtils::AddByteOffset(pNormals, uiElementStride);
    pColors    = pColors != nullptr ? xiiMemoryUtils::AddByteOffset(pColors, uiElementStride) : nullptr;
  }

  m_Outputs            = outputs;
  m_OutputMappings     = outputMappings;
  m_OutputVertexColors = outputVertexColors;

  //////////////////////////////////////////////////////////////////////////

  // TODO:
  // xiiBoundingBox box = mbDesc.GetBounds();
  xiiBoundingBox box = xiiBoundingBox(xiiVec3(-1000), xiiVec3(1000));
  box.TransformFromOrigin(transform.GetAsMat4());

  m_VolumeCollections.Clear();
  m_GlobalData.Clear();

  for (auto& pOutput : outputs)
  {
    if (pOutput != nullptr)
    {
      xiiProcGenInternal::ExtractVolumeCollections(world, box, *pOutput, m_VolumeCollections, m_GlobalData);
    }
  }

  const xiiUInt32 uiTransformHash = xiiHashingUtils::xxHash32(&transform, sizeof(xiiTransform));
  xiiProcGenInternal::SetInstanceSeed(uiTransformHash, m_GlobalData);
}

void VertexColorTask::Execute()
{
  if (m_InputVertices.IsEmpty())
    return;

  const xiiUInt32 uiNumOutputs = m_Outputs.GetCount();
  for (xiiUInt32 uiOutputIndex = 0; uiOutputIndex < uiNumOutputs; ++uiOutputIndex)
  {
    auto& pOutput = m_Outputs[uiOutputIndex];
    if (pOutput == nullptr || pOutput->m_pByteCode == nullptr)
      continue;

    XII_PROFILE_SCOPE("ExecuteVM");

    xiiUInt32 uiNumVertices = m_InputVertices.GetCount();
    m_TempData.SetCountUninitialized(uiNumVertices);

    xiiHybridArray<xiiProcessingStream, 8> inputs;
    {
      inputs.PushBack(MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_vPosition.x), ExpressionInputs::s_sPositionX));
      inputs.PushBack(MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_vPosition.y), ExpressionInputs::s_sPositionY));
      inputs.PushBack(MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_vPosition.z), ExpressionInputs::s_sPositionZ));

      inputs.PushBack(MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_vNormal.x), ExpressionInputs::s_sNormalX));
      inputs.PushBack(MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_vNormal.y), ExpressionInputs::s_sNormalY));
      inputs.PushBack(MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_vNormal.z), ExpressionInputs::s_sNormalZ));

      inputs.PushBack(MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_Color.r), ExpressionInputs::s_sColorR));
      inputs.PushBack(MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_Color.g), ExpressionInputs::s_sColorG));
      inputs.PushBack(MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_Color.b), ExpressionInputs::s_sColorB));
      inputs.PushBack(MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_Color.a), ExpressionInputs::s_sColorA));

      inputs.PushBack(MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_uiIndex), ExpressionInputs::s_sPointIndex, xiiProcessingStream::DataType::Int));
    }

    xiiHybridArray<xiiProcessingStream, 8> outputs;
    {
      outputs.PushBack(MakeStream(m_TempData.GetArrayPtr(), offsetof(xiiColor, r), ExpressionOutputs::s_sOutColorR));
      outputs.PushBack(MakeStream(m_TempData.GetArrayPtr(), offsetof(xiiColor, g), ExpressionOutputs::s_sOutColorG));
      outputs.PushBack(MakeStream(m_TempData.GetArrayPtr(), offsetof(xiiColor, b), ExpressionOutputs::s_sOutColorB));
      outputs.PushBack(MakeStream(m_TempData.GetArrayPtr(), offsetof(xiiColor, a), ExpressionOutputs::s_sOutColorA));
    }

    // Execute expression bytecode
    m_VM.Execute(*(pOutput->m_pByteCode), inputs, outputs, uiNumVertices, m_GlobalData).IgnoreResult();

    auto& outputMapping = m_OutputMappings[uiOutputIndex];
    for (xiiUInt32 i = 0; i < uiNumVertices; ++i)
    {
      xiiColor srcColor = m_TempData[i];
      xiiColor remappedColor;
      remappedColor.r = Remap(outputMapping.m_R, srcColor);
      remappedColor.g = Remap(outputMapping.m_G, srcColor);
      remappedColor.b = Remap(outputMapping.m_B, srcColor);
      remappedColor.a = Remap(outputMapping.m_A, srcColor);

      xiiColorLinearUB vertexColor = remappedColor;

      // Store output vertex colors interleaved
      m_OutputVertexColors[i * uiNumOutputs + uiOutputIndex] = *reinterpret_cast<xiiUInt32*>(&vertexColor.r);
    }
  }
}
