/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Texture/TexturePCH.h>

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/ImageConversion.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiImageConversionFlags, 1)
  XII_BITFLAGS_CONSTANT(xiiImageConversionFlags::InPlace),
XII_END_STATIC_REFLECTED_BITFLAGS;
// clang-format on

XII_ENUMERABLE_CLASS_IMPLEMENTATION(xiiImageConversionStep);

namespace
{
  struct TableEntry
  {
    TableEntry() = default;

    TableEntry(const xiiImageConversionStep* pStep, const xiiImageConversionEntry& entry)
    {
      m_pStep            = pStep;
      m_SourceFormat     = entry.m_SourceFormat;
      m_TargetFormat     = entry.m_TargetFormat;
      m_uiComponentCount = xiiMath::Min(xiiGALTextureUtilities::GetComponentCount(entry.m_SourceFormat), xiiGALTextureUtilities::GetComponentCount(entry.m_TargetFormat));

      float fSourceBpp = xiiGALTextureUtilities::GetExactBitsPerPixel(m_SourceFormat);
      float fTargetBpp = xiiGALTextureUtilities::GetExactBitsPerPixel(m_TargetFormat);

      m_Flags = entry.m_Flags;

      // Base cost is amount of bits processed
      m_fCost = fSourceBpp + fTargetBpp;

      // Penalty for non-inplace conversion
      if ((m_Flags & xiiImageConversionFlags::InPlace) == 0)
      {
        m_fCost *= 2;
      }

      // Penalize formats that aren't aligned to powers of two
      if (!xiiGALTextureUtilities::IsCompressed(m_SourceFormat) && !xiiGALTextureUtilities::IsCompressed(m_TargetFormat))
      {
        xiiUInt32 uiSourceBppInt = static_cast<xiiUInt32>(fSourceBpp);
        xiiUInt32 uiTargetBppInt = static_cast<xiiUInt32>(fTargetBpp);
        if (!xiiMath::IsPowerOf2(uiSourceBppInt) || !xiiMath::IsPowerOf2(uiTargetBppInt))
        {
          m_fCost *= 2;
        }
      }

      m_fCost += entry.m_fAdditionalPenalty;
    }

    const xiiImageConversionStep*        m_pStep        = nullptr;
    xiiEnum<xiiGALResourceFormat>        m_SourceFormat = xiiGALResourceFormat::Unknown;
    xiiEnum<xiiGALResourceFormat>        m_TargetFormat = xiiGALResourceFormat::Unknown;
    xiiBitflags<xiiImageConversionFlags> m_Flags;
    float                                m_fCost            = xiiMath::MaxValue<float>();
    xiiUInt32                            m_uiComponentCount = 0;

    static TableEntry chain(const TableEntry& a, const TableEntry& b)
    {
      if (xiiGALTextureUtilities::GetExactBitsPerPixel(a.m_SourceFormat) > xiiGALTextureUtilities::GetExactBitsPerPixel(a.m_TargetFormat) && xiiGALTextureUtilities::GetExactBitsPerPixel(b.m_SourceFormat) < xiiGALTextureUtilities::GetExactBitsPerPixel(b.m_TargetFormat))
      {
        // Disallow chaining conversions which first reduce to a smaller intermediate and then go back to a larger one, since
        // we end up throwing away information.
        return {};
      }

      TableEntry entry;
      entry.m_pStep            = a.m_pStep;
      entry.m_fCost            = a.m_fCost + b.m_fCost;
      entry.m_SourceFormat     = a.m_SourceFormat;
      entry.m_TargetFormat     = a.m_TargetFormat;
      entry.m_Flags            = a.m_Flags;
      entry.m_uiComponentCount = xiiMath::Min(a.m_uiComponentCount, b.m_uiComponentCount);
      return entry;
    }

    bool operator<(const TableEntry& other) const
    {
      if (m_uiComponentCount > other.m_uiComponentCount)
        return true;

      if (m_uiComponentCount < other.m_uiComponentCount)
        return false;

      return m_fCost < other.m_fCost;
    }

    bool isAdmissible() const
    {
      if (m_uiComponentCount == 0)
        return false;

      return m_fCost < xiiMath::MaxValue<float>();
    }
  };

  xiiMutex                            s_ConversionTableLock;
  xiiHashTable<xiiUInt32, TableEntry> s_ConversionTable;
  bool                                s_ConversionTableValid = false;

  constexpr xiiUInt32 MakeKey(xiiGALResourceFormat::Enum a, xiiGALResourceFormat::Enum b)
  {
    return a * xiiGALResourceFormat::ENUM_COUNT + b;
  }

  enum class xiiGALImageFormatClass : xiiUInt8
  {
    Linear,
    BlockCompressed,
    MultiPlanar
  };

  xiiGALImageFormatClass GetFormatClass(xiiEnum<xiiGALResourceFormat> format)
  {
    if (xiiGALResourceFormat::IsMultiplanar(format))
      return xiiGALImageFormatClass::MultiPlanar;

    if (xiiGALTextureUtilities::IsCompressed(format))
      return xiiGALImageFormatClass::BlockCompressed;

    return xiiGALImageFormatClass::Linear;
  }

  constexpr xiiUInt32 MakeTypeKey(xiiGALImageFormatClass a, xiiGALImageFormatClass b)
  {
    return (static_cast<xiiUInt32>(a) << 16) + static_cast<xiiUInt32>(b);
  }

  struct IntermediateBuffer
  {
    IntermediateBuffer(xiiUInt32 uiBitsPerBlock) :
      m_uiBitsPerBlock(uiBitsPerBlock)
    {
    }
    xiiUInt32 m_uiBitsPerBlock;
  };

  xiiUInt32 allocateScratchBufferIndex(xiiHybridArray<IntermediateBuffer, 16>& ref_scratchBuffers, xiiUInt32 uiBitsPerBlock, xiiUInt32 uiExcludedIndex)
  {
    xiiInt32 foundIndex = -1;

    for (xiiUInt32 bufferIndex = 0; bufferIndex < xiiUInt32(ref_scratchBuffers.GetCount()); ++bufferIndex)
    {
      if (bufferIndex == uiExcludedIndex)
      {
        continue;
      }

      if (ref_scratchBuffers[bufferIndex].m_uiBitsPerBlock == uiBitsPerBlock)
      {
        foundIndex = bufferIndex;
        break;
      }
    }

    if (foundIndex >= 0)
    {
      // Reuse existing scratch buffer
      return foundIndex;
    }
    else
    {
      // Allocate new scratch buffer
      ref_scratchBuffers.PushBack(IntermediateBuffer(uiBitsPerBlock));
      return ref_scratchBuffers.GetCount() - 1;
    }
  }
} // namespace

xiiImageConversionStep::xiiImageConversionStep()
{
  s_ConversionTableValid = false;
}

xiiImageConversionStep::~xiiImageConversionStep()
{
  s_ConversionTableValid = false;
}

xiiResult xiiImageConversion::BuildPath(xiiEnum<xiiGALResourceFormat> sourceFormat, xiiEnum<xiiGALResourceFormat> targetFormat, bool bSourceEqualsTarget, xiiHybridArray<xiiImageConversion::ConversionPathNode, 16>& out_path, xiiUInt32& out_uiNumScratchBuffers)
{
  XII_LOCK(s_ConversionTableLock);

  out_path.Clear();
  out_uiNumScratchBuffers = 0;

  if (sourceFormat == targetFormat)
  {
    ConversionPathNode node;
    node.m_SourceFormat        = sourceFormat;
    node.m_TargetFormat        = targetFormat;
    node.m_bInPlace            = bSourceEqualsTarget;
    node.m_uiSourceBufferIndex = 0;
    node.m_uiTargetBufferIndex = 0;
    node.m_pStep               = nullptr;
    out_path.PushBack(node);
    return XII_SUCCESS;
  }

  if (!s_ConversionTableValid)
  {
    RebuildConversionTable();
  }

  for (xiiEnum<xiiGALResourceFormat> current = sourceFormat; current != targetFormat;)
  {
    xiiUInt32 currentTableIndex = MakeKey(current, targetFormat);

    TableEntry entry;

    if (!s_ConversionTable.TryGetValue(currentTableIndex, entry))
    {
      return XII_FAILURE;
    }

    xiiImageConversion::ConversionPathNode step;
    step.m_SourceFormat = entry.m_SourceFormat;
    step.m_TargetFormat = entry.m_TargetFormat;
    step.m_bInPlace     = entry.m_Flags.IsAnySet(xiiImageConversionFlags::InPlace);
    step.m_pStep        = entry.m_pStep;

    current = entry.m_TargetFormat;

    out_path.PushBack(step);
  }

  xiiTemporaryHybridArray<IntermediateBuffer, 16> scratchBuffers;
  scratchBuffers.PushBack(IntermediateBuffer(xiiGALTextureUtilities::GetBitsPerBlock(targetFormat)));

  const xiiInt32 iLastPathIndex = out_path.GetCount() - 1;
  for (xiiInt32 i = iLastPathIndex; i >= 0; --i)
  {
    if (i == iLastPathIndex)
    {
      out_path[i].m_uiTargetBufferIndex = 0;
    }
    else
    {
      out_path[i].m_uiTargetBufferIndex = out_path[i + 1].m_uiSourceBufferIndex;
    }

    if (i > 0)
    {
      if (out_path[i].m_bInPlace)
      {
        out_path[i].m_uiSourceBufferIndex = out_path[i].m_uiTargetBufferIndex;
      }
      else
      {
        xiiUInt32 uiBitsPerBlock = xiiGALTextureUtilities::GetBitsPerBlock(out_path[i].m_SourceFormat);

        out_path[i].m_uiSourceBufferIndex = allocateScratchBufferIndex(scratchBuffers, uiBitsPerBlock, out_path[i].m_uiTargetBufferIndex);
      }
    }
  }

  if (bSourceEqualsTarget)
  {
    // Enforce constraint that source == target
    out_path[0].m_uiSourceBufferIndex = 0;

    // Did we accidentally break the in-place invariant?
    if (out_path[0].m_uiSourceBufferIndex == out_path[0].m_uiTargetBufferIndex && !out_path[0].m_bInPlace)
    {
      if (out_path.GetCount() == 1)
      {
        // Only a single step, so we need to add a copy step
        xiiImageConversion::ConversionPathNode copy;
        copy.m_bInPlace                   = false;
        copy.m_SourceFormat               = sourceFormat;
        copy.m_TargetFormat               = sourceFormat;
        copy.m_uiSourceBufferIndex        = out_path[0].m_uiSourceBufferIndex;
        copy.m_uiTargetBufferIndex        = allocateScratchBufferIndex(scratchBuffers, xiiGALTextureUtilities::GetBitsPerBlock(out_path[0].m_SourceFormat), out_path[0].m_uiSourceBufferIndex);
        out_path[0].m_uiSourceBufferIndex = copy.m_uiTargetBufferIndex;
        copy.m_pStep                      = nullptr;
        out_path.InsertAt(0, copy);
      }
      else
      {
        // Turn second step to non-inplace
        out_path[1].m_bInPlace            = false;
        out_path[1].m_uiSourceBufferIndex = allocateScratchBufferIndex(scratchBuffers, xiiGALTextureUtilities::GetBitsPerBlock(out_path[1].m_SourceFormat), out_path[0].m_uiSourceBufferIndex);
        out_path[0].m_uiTargetBufferIndex = out_path[1].m_uiSourceBufferIndex;
      }
    }
  }
  else
  {
    out_path[0].m_uiSourceBufferIndex = scratchBuffers.GetCount();
  }

  out_uiNumScratchBuffers = scratchBuffers.GetCount() - 1;

  return XII_SUCCESS;
}

void xiiImageConversion::RebuildConversionTable()
{
  XII_LOCK(s_ConversionTableLock);

  s_ConversionTable.Clear();

  // Prime conversion table with known conversions
  for (xiiImageConversionStep* conversion = xiiImageConversionStep::GetFirstInstance(); conversion; conversion = conversion->GetNextInstance())
  {
    xiiArrayPtr<const xiiImageConversionEntry> entries = conversion->GetSupportedConversions();

    for (xiiUInt32 uiSubIndex = 0; uiSubIndex < (xiiUInt32)entries.GetCount(); ++uiSubIndex)
    {
      const xiiImageConversionEntry& subConversion = entries[uiSubIndex];

      if (subConversion.m_Flags.IsAnySet(xiiImageConversionFlags::InPlace))
      {
        XII_ASSERT_DEV(xiiGALTextureUtilities::IsCompressed(subConversion.m_SourceFormat) == xiiGALTextureUtilities::IsCompressed(subConversion.m_TargetFormat) && xiiGALTextureUtilities::GetBitsPerBlock(subConversion.m_SourceFormat) == xiiGALTextureUtilities::GetBitsPerBlock(subConversion.m_TargetFormat), "In-place conversions are only allowed between formats of the same number of bits per pixel and compressedness");
      }

      if (GetFormatClass(subConversion.m_SourceFormat) == xiiGALImageFormatClass::MultiPlanar)
      {
        XII_ASSERT_DEV(GetFormatClass(subConversion.m_TargetFormat) == xiiGALImageFormatClass::Linear, "Conversions from planar formats must target linear formats");
      }
      else if (GetFormatClass(subConversion.m_TargetFormat) == xiiGALImageFormatClass::MultiPlanar)
      {
        XII_ASSERT_DEV(GetFormatClass(subConversion.m_SourceFormat) == xiiGALImageFormatClass::Linear, "Conversions to planar formats must sourced from linear formats");
      }

      xiiUInt32 uiTableIndex = MakeKey(subConversion.m_SourceFormat, subConversion.m_TargetFormat);

      // Use the cheapest known conversion for each combination in case there are multiple ones
      TableEntry candidate(conversion, subConversion);

      TableEntry existing;

      if (!s_ConversionTable.TryGetValue(uiTableIndex, existing) || candidate < existing)
      {
        s_ConversionTable.Insert(uiTableIndex, candidate);
      }
    }
  }

  for (xiiUInt32 i = 0; i < xiiGALResourceFormat::ENUM_COUNT; ++i)
  {
    const xiiEnum<xiiGALResourceFormat> format = static_cast<xiiGALResourceFormat::Enum>(i);

    // Add copy-conversion (from and to same format)
    s_ConversionTable.Insert(MakeKey(format, format), TableEntry(nullptr, xiiImageConversionEntry(xiiImageConversionEntry(format, format, xiiImageConversionFlags::InPlace))));
  }

  // Straight from http://en.wikipedia.org/wiki/Floyd-Warshall_algorithm
  for (xiiUInt32 k = 1; k < xiiGALResourceFormat::ENUM_COUNT; ++k)
  {
    for (xiiUInt32 i = 1; i < xiiGALResourceFormat::ENUM_COUNT; ++i)
    {
      if (k == i)
      {
        continue;
      }

      xiiUInt32 uiTableIndexIK = MakeKey(static_cast<xiiGALResourceFormat::Enum>(i), static_cast<xiiGALResourceFormat::Enum>(k));

      TableEntry entryIK;
      if (!s_ConversionTable.TryGetValue(uiTableIndexIK, entryIK))
      {
        continue;
      }

      for (xiiUInt32 j = 1; j < xiiGALResourceFormat::ENUM_COUNT; ++j)
      {
        if (j == i || j == k)
        {
          continue;
        }

        xiiUInt32 uiTableIndexIJ = MakeKey(static_cast<xiiGALResourceFormat::Enum>(i), static_cast<xiiGALResourceFormat::Enum>(j));
        xiiUInt32 uiTableIndexKJ = MakeKey(static_cast<xiiGALResourceFormat::Enum>(k), static_cast<xiiGALResourceFormat::Enum>(j));

        TableEntry entryKJ;
        if (!s_ConversionTable.TryGetValue(uiTableIndexKJ, entryKJ))
        {
          continue;
        }

        TableEntry candidate = TableEntry::chain(entryIK, entryKJ);

        TableEntry existing;
        if (candidate.isAdmissible() && candidate < s_ConversionTable[uiTableIndexIJ])
        {
          // To Convert from format I to format J, first Convert from I to K
          s_ConversionTable[uiTableIndexIJ] = candidate;
        }
      }
    }
  }

  s_ConversionTableValid = true;
}

xiiResult xiiImageConversion::Convert(const xiiImageView& source, xiiImage& ref_target, xiiEnum<xiiGALResourceFormat> targetFormat)
{
  XII_PROFILE_SCOPE("xiiImageConversion::Convert");

  xiiEnum<xiiGALResourceFormat> sourceFormat = source.GetImageFormat();

  // Trivial copy
  if (sourceFormat == targetFormat)
  {
    if (&source != &ref_target)
    {
      // copy if not already the same
      ref_target.ResetAndCopy(source);
    }
    return XII_SUCCESS;
  }

  xiiTemporaryHybridArray<ConversionPathNode, 16> path;
  xiiUInt32                                       uiScratchBufferCount = 0;
  if (BuildPath(sourceFormat, targetFormat, &source == &ref_target, path, uiScratchBufferCount).Failed())
  {
    return XII_FAILURE;
  }

  return Convert(source, ref_target, path, uiScratchBufferCount);
}

xiiResult xiiImageConversion::Convert(const xiiImageView& source, xiiImage& ref_target, xiiArrayPtr<ConversionPathNode> path, xiiUInt32 uiNumScratchBuffers)
{
  XII_ASSERT_DEV(path.GetCount() > 0, "Invalid conversion path");
  XII_ASSERT_DEV(path[0].m_SourceFormat == source.GetImageFormat(), "Invalid conversion path");

  xiiTemporaryHybridArray<xiiImage, 16> intermediates;
  intermediates.SetCount(uiNumScratchBuffers);

  const xiiImageView* pSource = &source;

  for (xiiUInt32 i = 0; i < path.GetCount(); ++i)
  {
    xiiUInt32 uiTargetIndex = path[i].m_uiTargetBufferIndex;

    xiiImage* pTarget = uiTargetIndex == 0 ? &ref_target : &intermediates[uiTargetIndex - 1];

    if (ConvertSingleStep(path[i].m_pStep, *pSource, *pTarget, path[i].m_TargetFormat).Failed())
    {
      return XII_FAILURE;
    }

    pSource = pTarget;
  }

  return XII_SUCCESS;
}

xiiResult xiiImageConversion::ConvertRaw(xiiConstByteBlobPtr source, xiiByteBlobPtr target, xiiUInt32 uiNumElements, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiEnum<xiiGALResourceFormat> targetFormat)
{
  if (uiNumElements == 0)
  {
    return XII_SUCCESS;
  }

  // Trivial copy
  if (sourceFormat == targetFormat)
  {
    if (target.GetPtr() != source.GetPtr())
    {
      memcpy(target.GetPtr(), source.GetPtr(), uiNumElements * xiiUInt64(xiiGALTextureUtilities::GetBitsPerPixel(sourceFormat)) / 8);
    }
    return XII_SUCCESS;
  }

  if (xiiGALTextureUtilities::IsCompressed(sourceFormat) || xiiGALTextureUtilities::IsCompressed(targetFormat))
  {
    return XII_FAILURE;
  }

  xiiTemporaryHybridArray<ConversionPathNode, 16> path;
  xiiUInt32                                       uiScratchBufferCount;
  if (BuildPath(sourceFormat, targetFormat, source.GetPtr() == target.GetPtr(), path, uiScratchBufferCount).Failed())
  {
    return XII_FAILURE;
  }

  return ConvertRaw(source, target, uiNumElements, path, uiScratchBufferCount);
}

xiiResult xiiImageConversion::ConvertRaw(xiiConstByteBlobPtr source, xiiByteBlobPtr target, xiiUInt32 uiNumElements, xiiArrayPtr<ConversionPathNode> path, xiiUInt32 uiNumScratchBuffers)
{
  XII_ASSERT_DEV(path.GetCount() > 0, "Path of length 0 is invalid.");

  if (uiNumElements == 0)
  {
    return XII_SUCCESS;
  }

  if (xiiGALTextureUtilities::IsCompressed(path.GetPtr()->m_SourceFormat) || xiiGALTextureUtilities::IsCompressed((path.GetEndPtr() - 1)->m_TargetFormat))
  {
    return XII_FAILURE;
  }

  xiiTemporaryHybridArray<xiiBlob, 16> intermediates;
  intermediates.SetCount(uiNumScratchBuffers);

  for (xiiUInt32 i = 0; i < path.GetCount(); ++i)
  {
    xiiUInt32 uiTargetIndex = path[i].m_uiTargetBufferIndex;
    xiiUInt32 uiTargetBpp   = xiiGALTextureUtilities::GetBitsPerPixel(path[i].m_TargetFormat);

    xiiByteBlobPtr pStepTarget;
    if (uiTargetIndex == 0)
    {
      pStepTarget = target;
    }
    else
    {
      xiiUInt32 uiExpectedSize = static_cast<xiiUInt32>(uiTargetBpp * uiNumElements / 8);
      intermediates[uiTargetIndex - 1].SetCountUninitialized(uiExpectedSize);
      pStepTarget = intermediates[uiTargetIndex - 1].GetByteBlobPtr();
    }

    if (path[i].m_pStep == nullptr)
    {
      memcpy(pStepTarget.GetPtr(), source.GetPtr(), uiNumElements * uiTargetBpp / 8);
    }
    else
    {
      if (static_cast<const xiiImageConversionStepLinear*>(path[i].m_pStep)->ConvertPixels(source, pStepTarget, uiNumElements, path[i].m_SourceFormat, path[i].m_TargetFormat).Failed())
      {
        return XII_FAILURE;
      }
    }

    source = pStepTarget;
  }

  return XII_SUCCESS;
}

xiiResult xiiImageConversion::ConvertSingleStep(const xiiImageConversionStep* pStep, const xiiImageView& source, xiiImage& target, xiiEnum<xiiGALResourceFormat> targetFormat)
{
  if (!pStep)
  {
    target.ResetAndCopy(source);
    return XII_SUCCESS;
  }

  xiiEnum<xiiGALResourceFormat> sourceFormat = source.GetImageFormat();

  xiiGALTextureCreationDescription header = source.GetDescription();
  header.m_Format                         = targetFormat;
  target.ResetAndAlloc(header);

  switch (MakeTypeKey(GetFormatClass(sourceFormat), GetFormatClass(targetFormat)))
  {
    case MakeTypeKey(xiiGALImageFormatClass::Linear, xiiGALImageFormatClass::Linear):
    {
      // we have to do the computation in 64-bit otherwise it might overflow for very large textures (8k x 4k or bigger).
      xiiUInt64 uiElementCount = xiiUInt64(8) * target.GetByteBlobPtr().GetCount() / (xiiUInt64)xiiGALTextureUtilities::GetBitsPerPixel(targetFormat);
      return static_cast<const xiiImageConversionStepLinear*>(pStep)->ConvertPixels(source.GetByteBlobPtr(), target.GetByteBlobPtr(), (xiiUInt32)uiElementCount, sourceFormat, targetFormat);
    }

    case MakeTypeKey(xiiGALImageFormatClass::Linear, xiiGALImageFormatClass::BlockCompressed):
      return ConvertSingleStepCompress(source, target, sourceFormat, targetFormat, pStep);

    case MakeTypeKey(xiiGALImageFormatClass::Linear, xiiGALImageFormatClass::MultiPlanar):
      return ConvertSingleStepPlanarize(source, target, sourceFormat, targetFormat, pStep);

    case MakeTypeKey(xiiGALImageFormatClass::BlockCompressed, xiiGALImageFormatClass::Linear):
      return ConvertSingleStepDecompress(source, target, sourceFormat, targetFormat, pStep);

    case MakeTypeKey(xiiGALImageFormatClass::MultiPlanar, xiiGALImageFormatClass::Linear):
      return ConvertSingleStepDeplanarize(source, target, sourceFormat, targetFormat, pStep);

    default:
      XII_ASSERT_NOT_IMPLEMENTED;
      return XII_FAILURE;
  }
}

xiiResult xiiImageConversion::ConvertSingleStepDecompress(const xiiImageView& source, xiiImage& target, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiEnum<xiiGALResourceFormat> targetFormat, const xiiImageConversionStep* pStep)
{
  for (xiiUInt32 uiArrayIndex = 0; uiArrayIndex < source.GetNumArrayIndices(); ++uiArrayIndex)
  {
    for (xiiUInt32 uiFace = 0; uiFace < source.GetNumFaces(); ++uiFace)
    {
      for (xiiUInt32 uiMipLevel = 0; uiMipLevel < source.GetMipLevelCount(); ++uiMipLevel)
      {
        const xiiUInt32 uiWidth  = target.GetWidth(uiMipLevel);
        const xiiUInt32 uiHeight = target.GetHeight(uiMipLevel);

        const xiiUInt32 uiBlockSizeX = xiiGALTextureUtilities::GetBlockWidth(sourceFormat);
        const xiiUInt32 uiBlockSizeY = xiiGALTextureUtilities::GetBlockHeight(sourceFormat);

        const xiiUInt32 uiNumBlocksX = source.GetNumBlocksX(uiMipLevel);
        const xiiUInt32 uiNumBlocksY = source.GetNumBlocksY(uiMipLevel);

        const xiiUInt64 uiTargetRowPitch      = target.GetRowPitch(uiMipLevel);
        const xiiUInt32 uiTargetBytesPerPixel = xiiGALTextureUtilities::GetBitsPerPixel(targetFormat) / 8;

        // Decompress into a temp memory block so we don't have to explicitly handle the case where the image is not a multiple of the block size.
        xiiTemporaryHybridArray<xiiUInt8, 256U> tempBuffer;
        tempBuffer.SetCount(uiNumBlocksX * uiBlockSizeX * uiBlockSizeY * uiTargetBytesPerPixel);

        for (xiiUInt32 uiSlice = 0; uiSlice < source.GetDepth(uiMipLevel); uiSlice++)
        {
          for (xiiUInt32 blockY = 0; blockY < uiNumBlocksY; blockY++)
          {
            xiiImageView sourceRowView = source.GetRowView(uiMipLevel, uiFace, uiArrayIndex, blockY, uiSlice);

            if (static_cast<const xiiImageConversionStepDecompressBlocks*>(pStep)->DecompressBlocks(sourceRowView.GetByteBlobPtr(), xiiByteBlobPtr(tempBuffer.GetData(), tempBuffer.GetCount()), uiNumBlocksX, sourceFormat, targetFormat).Failed())
            {
              return XII_FAILURE;
            }

            for (xiiUInt32 blockX = 0; blockX < uiNumBlocksX; blockX++)
            {
              xiiUInt8* pTargetPointer = target.GetPixelPointer<xiiUInt8>(uiMipLevel, uiFace, uiArrayIndex, blockX * uiBlockSizeX, blockY * uiBlockSizeY, uiSlice);

              // Copy into actual target, clamping to image dimensions
              xiiUInt32 uiCopyWidth  = xiiMath::Min(uiBlockSizeX, uiWidth - blockX * uiBlockSizeX);
              xiiUInt32 uiCopyHeight = xiiMath::Min(uiBlockSizeY, uiHeight - blockY * uiBlockSizeY);
              for (xiiUInt32 uiRow = 0; uiRow < uiCopyHeight; ++uiRow)
              {
                memcpy(pTargetPointer, &tempBuffer[(blockX * uiBlockSizeX + uiRow) * uiBlockSizeY * uiTargetBytesPerPixel], xiiMath::SafeMultiply32(uiCopyWidth, uiTargetBytesPerPixel));

                pTargetPointer += uiTargetRowPitch;
              }
            }
          }
        }
      }
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiImageConversion::ConvertSingleStepCompress(const xiiImageView& source, xiiImage& target, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiEnum<xiiGALResourceFormat> targetFormat, const xiiImageConversionStep* pStep)
{
  for (xiiUInt32 uiArrayIndex = 0; uiArrayIndex < source.GetNumArrayIndices(); ++uiArrayIndex)
  {
    for (xiiUInt32 uiFace = 0; uiFace < source.GetNumFaces(); ++uiFace)
    {
      for (xiiUInt32 uiMipLevel = 0; uiMipLevel < source.GetMipLevelCount(); ++uiMipLevel)
      {
        const xiiUInt32 uiSourceWidth  = source.GetWidth(uiMipLevel);
        const xiiUInt32 uiSourceHeight = source.GetHeight(uiMipLevel);

        const xiiUInt32 uiNumBlocksX = target.GetNumBlocksX(uiMipLevel);
        const xiiUInt32 uiNumBlocksY = target.GetNumBlocksY(uiMipLevel);

        const xiiUInt32 uiTargetWidth  = uiNumBlocksX * xiiGALTextureUtilities::GetBlockWidth(targetFormat);
        const xiiUInt32 uiTargetHeight = uiNumBlocksY * xiiGALTextureUtilities::GetBlockHeight(targetFormat);

        const xiiUInt64 uiSourceRowPitch      = source.GetRowPitch(uiMipLevel);
        const xiiUInt32 uiSourceBytesPerPixel = xiiGALTextureUtilities::GetBitsPerPixel(sourceFormat) / 8;

        // Pad image to multiple of block size for compression.
        xiiGALTextureCreationDescription paddedSliceHeader;
        paddedSliceHeader.m_Size.width  = uiTargetWidth;
        paddedSliceHeader.m_Size.height = uiTargetHeight;
        paddedSliceHeader.m_Format      = sourceFormat;

        xiiImage paddedSlice;
        paddedSlice.ResetAndAlloc(paddedSliceHeader);

        for (xiiUInt32 uiSlice = 0; uiSlice < source.GetDepth(uiMipLevel); ++uiSlice)
        {
          for (xiiUInt32 y = 0; y < uiTargetHeight; ++y)
          {
            xiiUInt32 uiSourceY = xiiMath::Min(y, uiSourceHeight - 1);

            memcpy(paddedSlice.GetPixelPointer<void>(0, 0, 0, 0, y), source.GetPixelPointer<void>(uiMipLevel, uiFace, uiArrayIndex, 0, uiSourceY, uiSlice), static_cast<size_t>(uiSourceRowPitch));

            for (xiiUInt32 x = uiSourceWidth; x < uiTargetWidth; ++x)
            {
              memcpy(paddedSlice.GetPixelPointer<void>(0, 0, 0, x, y), source.GetPixelPointer<void>(uiMipLevel, uiFace, uiArrayIndex, uiSourceWidth - 1, uiSourceY, uiSlice), uiSourceBytesPerPixel);
            }
          }

          xiiResult result = static_cast<const xiiImageConversionStepCompressBlocks*>(pStep)->CompressBlocks(paddedSlice.GetByteBlobPtr(), target.GetSliceView(uiMipLevel, uiFace, uiArrayIndex, uiSlice).GetByteBlobPtr(), uiNumBlocksX, uiNumBlocksY, sourceFormat, targetFormat);

          if (result.Failed())
          {
            return XII_FAILURE;
          }
        }
      }
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiImageConversion::ConvertSingleStepDeplanarize(const xiiImageView& source, xiiImage& target, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiEnum<xiiGALResourceFormat> targetFormat, const xiiImageConversionStep* pStep)
{
  for (xiiUInt32 uiArrayIndex = 0; uiArrayIndex < source.GetNumArrayIndices(); ++uiArrayIndex)
  {
    for (xiiUInt32 uiFace = 0; uiFace < source.GetNumFaces(); ++uiFace)
    {
      for (xiiUInt32 uiMipLevel = 0; uiMipLevel < source.GetMipLevelCount(); ++uiMipLevel)
      {
        const xiiUInt32 uiWidth  = target.GetWidth(uiMipLevel);
        const xiiUInt32 uiHeight = target.GetHeight(uiMipLevel);

        xiiTemporaryHybridArray<xiiImageView, 2> sourcePlanes;
        for (xiiUInt32 planeIndex = 0; planeIndex < source.GetPlaneCount(); ++planeIndex)
        {
          const xiiUInt32 uiBlockSizeX = xiiGALTextureUtilities::GetBlockWidth(sourceFormat, planeIndex);
          const xiiUInt32 uiBlockSizeY = xiiGALTextureUtilities::GetBlockHeight(sourceFormat, planeIndex);

          if (uiWidth % uiBlockSizeX != 0 || uiHeight % uiBlockSizeY != 0)
          {
            // Input image must be aligned to block dimensions already.
            return XII_FAILURE;
          }

          sourcePlanes.PushBack(source.GetPlaneView(uiMipLevel, uiFace, uiArrayIndex, planeIndex));
        }

        if (static_cast<const xiiImageConversionStepDeplanarize*>(pStep)->ConvertPixels(sourcePlanes, target.GetSubImageView(uiMipLevel, uiFace, uiArrayIndex), uiWidth, uiHeight, sourceFormat, targetFormat).Failed())
        {
          return XII_FAILURE;
        }
      }
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiImageConversion::ConvertSingleStepPlanarize(const xiiImageView& source, xiiImage& target, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiEnum<xiiGALResourceFormat> targetFormat, const xiiImageConversionStep* pStep)
{
  for (xiiUInt32 uiArrayIndex = 0; uiArrayIndex < source.GetNumArrayIndices(); ++uiArrayIndex)
  {
    for (xiiUInt32 uiFace = 0; uiFace < source.GetNumFaces(); ++uiFace)
    {
      for (xiiUInt32 uiMipLevel = 0; uiMipLevel < source.GetMipLevelCount(); ++uiMipLevel)
      {
        const xiiUInt32 uiWidth  = target.GetWidth(uiMipLevel);
        const xiiUInt32 uiHeight = target.GetHeight(uiMipLevel);

        xiiTemporaryHybridArray<xiiImage, 2> targetPlanes;
        for (xiiUInt32 planeIndex = 0; planeIndex < target.GetPlaneCount(); ++planeIndex)
        {
          const xiiUInt32 uiBlockSizeX = xiiGALTextureUtilities::GetBlockWidth(targetFormat, planeIndex);
          const xiiUInt32 uiBlockSizeY = xiiGALTextureUtilities::GetBlockHeight(targetFormat, planeIndex);

          if (uiWidth % uiBlockSizeX != 0 || uiHeight % uiBlockSizeY != 0)
          {
            // Input image must be aligned to block dimensions already.
            return XII_FAILURE;
          }

          targetPlanes.PushBack(target.GetPlaneView(uiMipLevel, uiFace, uiArrayIndex, planeIndex));
        }

        if (static_cast<const xiiImageConversionStepPlanarize*>(pStep)->ConvertPixels(source.GetSubImageView(uiMipLevel, uiFace, uiArrayIndex), targetPlanes, uiWidth, uiHeight, sourceFormat, targetFormat).Failed())
        {
          return XII_FAILURE;
        }
      }
    }
  }

  return XII_SUCCESS;
}

bool xiiImageConversion::IsConvertible(xiiEnum<xiiGALResourceFormat> sourceFormat, xiiEnum<xiiGALResourceFormat> targetFormat)
{
  XII_LOCK(s_ConversionTableLock);

  if (!s_ConversionTableValid)
  {
    RebuildConversionTable();
  }

  xiiUInt32 uiTableIndex = MakeKey(sourceFormat, targetFormat);
  return s_ConversionTable.Contains(uiTableIndex);
}

xiiEnum<xiiGALResourceFormat> xiiImageConversion::FindClosestCompatibleFormat(xiiEnum<xiiGALResourceFormat> format, xiiArrayPtr<const xiiEnum<xiiGALResourceFormat>> compatibleFormats)
{
  XII_LOCK(s_ConversionTableLock);

  if (!s_ConversionTableValid)
  {
    RebuildConversionTable();
  }

  TableEntry                    bestEntry;
  xiiEnum<xiiGALResourceFormat> bestFormat = xiiGALResourceFormat::Unknown;

  for (xiiUInt32 uiTargetIndex = 0; uiTargetIndex < xiiUInt32(compatibleFormats.GetCount()); uiTargetIndex++)
  {
    xiiUInt32  uiTableIndex = MakeKey(format, compatibleFormats[uiTargetIndex]);
    TableEntry candidate;
    if (s_ConversionTable.TryGetValue(uiTableIndex, candidate) && candidate < bestEntry)
    {
      bestEntry  = candidate;
      bestFormat = compatibleFormats[uiTargetIndex];
    }
  }

  return bestFormat;
}
