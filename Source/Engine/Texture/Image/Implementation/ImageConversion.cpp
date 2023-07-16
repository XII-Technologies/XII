#include <Texture/TexturePCH.h>

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/ImageConversion.h>

XII_ENUMERABLE_CLASS_IMPLEMENTATION(xiiImageConversionStep);

namespace
{
  struct TableEntry
  {
    TableEntry() = default;

    TableEntry(const xiiImageConversionStep* pStep, const xiiImageConversionEntry& entry)
    {
      m_step         = pStep;
      m_sourceFormat = entry.m_sourceFormat;
      m_targetFormat = entry.m_targetFormat;
      m_numChannels  = xiiMath::Min(xiiImageFormat::GetNumChannels(entry.m_sourceFormat), xiiImageFormat::GetNumChannels(entry.m_targetFormat));

      float sourceBpp = xiiImageFormat::GetExactBitsPerPixel(m_sourceFormat);
      float targetBpp = xiiImageFormat::GetExactBitsPerPixel(m_targetFormat);

      m_flags = entry.m_flags;

      // Base cost is amount of bits processed
      m_cost = sourceBpp + targetBpp;

      // Penalty for non-inplace conversion
      if ((m_flags & xiiImageConversionFlags::InPlace) == 0)
      {
        m_cost *= 2;
      }

      // Penalize formats that aren't aligned to powers of two
      if (!xiiImageFormat::IsCompressed(m_sourceFormat) && !xiiImageFormat::IsCompressed(m_targetFormat))
      {
        auto sourceBppInt = static_cast<xiiUInt32>(sourceBpp);
        auto targetBppInt = static_cast<xiiUInt32>(targetBpp);
        if (!xiiMath::IsPowerOf2(sourceBppInt) || !xiiMath::IsPowerOf2(targetBppInt))
        {
          m_cost *= 2;
        }
      }

      m_cost += entry.m_additionalPenalty;
    }

    const xiiImageConversionStep*        m_step         = nullptr;
    xiiImageFormat::Enum                 m_sourceFormat = xiiImageFormat::UNKNOWN;
    xiiImageFormat::Enum                 m_targetFormat = xiiImageFormat::UNKNOWN;
    xiiBitflags<xiiImageConversionFlags> m_flags;
    float                                m_cost        = xiiMath::MaxValue<float>();
    xiiUInt32                            m_numChannels = 0;

    static TableEntry chain(const TableEntry& a, const TableEntry& b)
    {
      if (xiiImageFormat::GetExactBitsPerPixel(a.m_sourceFormat) > xiiImageFormat::GetExactBitsPerPixel(a.m_targetFormat) &&
          xiiImageFormat::GetExactBitsPerPixel(b.m_sourceFormat) < xiiImageFormat::GetExactBitsPerPixel(b.m_targetFormat))
      {
        // Disallow chaining conversions which first reduce to a smaller intermediate and then go back to a larger one, since
        // we end up throwing away information.
        return {};
      }

      TableEntry entry;
      entry.m_step         = a.m_step;
      entry.m_cost         = a.m_cost + b.m_cost;
      entry.m_sourceFormat = a.m_sourceFormat;
      entry.m_targetFormat = a.m_targetFormat;
      entry.m_flags        = a.m_flags;
      entry.m_numChannels  = xiiMath::Min(a.m_numChannels, b.m_numChannels);
      return entry;
    }

    bool operator<(const TableEntry& other) const
    {
      if (m_numChannels > other.m_numChannels)
        return true;

      if (m_numChannels < other.m_numChannels)
        return false;

      return m_cost < other.m_cost;
    }

    bool isAdmissible() const
    {
      if (m_numChannels == 0)
        return false;

      return m_cost < xiiMath::MaxValue<float>();
    }
  };

  xiiMutex                            s_conversionTableLock;
  xiiHashTable<xiiUInt32, TableEntry> s_conversionTable;
  bool                                s_conversionTableValid = false;

  constexpr xiiUInt32 MakeKey(xiiImageFormat::Enum a, xiiImageFormat::Enum b) { return a * xiiImageFormat::NUM_FORMATS + b; }
  constexpr xiiUInt32 MakeTypeKey(xiiImageFormatType::Enum a, xiiImageFormatType::Enum b) { return (a << 16) + b; }

  struct IntermediateBuffer
  {
    IntermediateBuffer(xiiUInt32 uiBitsPerBlock) :
      m_bitsPerBlock(uiBitsPerBlock)
    {
    }
    xiiUInt32 m_bitsPerBlock;
  };

  xiiUInt32 allocateScratchBufferIndex(xiiHybridArray<IntermediateBuffer, 16>& ref_scratchBuffers, xiiUInt32 uiBitsPerBlock, xiiUInt32 uiExcludedIndex)
  {
    int foundIndex = -1;

    for (xiiUInt32 bufferIndex = 0; bufferIndex < xiiUInt32(ref_scratchBuffers.GetCount()); ++bufferIndex)
    {
      if (bufferIndex == uiExcludedIndex)
      {
        continue;
      }

      if (ref_scratchBuffers[bufferIndex].m_bitsPerBlock == uiBitsPerBlock)
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
  s_conversionTableValid = false;
}

xiiImageConversionStep::~xiiImageConversionStep()
{
  s_conversionTableValid = false;
}

xiiResult xiiImageConversion::BuildPath(xiiImageFormat::Enum sourceFormat, xiiImageFormat::Enum targetFormat, bool bSourceEqualsTarget, xiiHybridArray<xiiImageConversion::ConversionPathNode, 16>& ref_path_out, xiiUInt32& ref_uiNumScratchBuffers_out)
{
  XII_LOCK(s_conversionTableLock);

  ref_path_out.Clear();
  ref_uiNumScratchBuffers_out = 0;

  if (sourceFormat == targetFormat)
  {
    ConversionPathNode node;
    node.m_sourceFormat      = sourceFormat;
    node.m_targetFormat      = targetFormat;
    node.m_inPlace           = bSourceEqualsTarget;
    node.m_sourceBufferIndex = 0;
    node.m_targetBufferIndex = 0;
    node.m_step              = nullptr;
    ref_path_out.PushBack(node);
    return XII_SUCCESS;
  }

  if (!s_conversionTableValid)
  {
    RebuildConversionTable();
  }

  for (xiiImageFormat::Enum current = sourceFormat; current != targetFormat;)
  {
    xiiUInt32 currentTableIndex = MakeKey(current, targetFormat);

    TableEntry entry;

    if (!s_conversionTable.TryGetValue(currentTableIndex, entry))
    {
      return XII_FAILURE;
    }

    xiiImageConversion::ConversionPathNode step;
    step.m_sourceFormat = entry.m_sourceFormat;
    step.m_targetFormat = entry.m_targetFormat;
    step.m_inPlace      = entry.m_flags.IsAnySet(xiiImageConversionFlags::InPlace);
    step.m_step         = entry.m_step;

    current = entry.m_targetFormat;

    ref_path_out.PushBack(step);
  }

  xiiHybridArray<IntermediateBuffer, 16> scratchBuffers;
  scratchBuffers.PushBack(IntermediateBuffer(xiiImageFormat::GetBitsPerBlock(targetFormat)));

  for (int i = ref_path_out.GetCount() - 1; i >= 0; --i)
  {
    if (i == ref_path_out.GetCount() - 1)
      ref_path_out[i].m_targetBufferIndex = 0;
    else
      ref_path_out[i].m_targetBufferIndex = ref_path_out[i + 1].m_sourceBufferIndex;

    if (i > 0)
    {
      if (ref_path_out[i].m_inPlace)
      {
        ref_path_out[i].m_sourceBufferIndex = ref_path_out[i].m_targetBufferIndex;
      }
      else
      {
        xiiUInt32 bitsPerBlock = xiiImageFormat::GetBitsPerBlock(ref_path_out[i].m_sourceFormat);

        ref_path_out[i].m_sourceBufferIndex = allocateScratchBufferIndex(scratchBuffers, bitsPerBlock, ref_path_out[i].m_targetBufferIndex);
      }
    }
  }

  if (bSourceEqualsTarget)
  {
    // Enforce constraint that source == target
    ref_path_out[0].m_sourceBufferIndex = 0;

    // Did we accidentally break the in-place invariant?
    if (ref_path_out[0].m_sourceBufferIndex == ref_path_out[0].m_targetBufferIndex && !ref_path_out[0].m_inPlace)
    {
      if (ref_path_out.GetCount() == 1)
      {
        // Only a single step, so we need to add a copy step
        xiiImageConversion::ConversionPathNode copy;
        copy.m_inPlace           = false;
        copy.m_sourceFormat      = sourceFormat;
        copy.m_targetFormat      = sourceFormat;
        copy.m_sourceBufferIndex = ref_path_out[0].m_sourceBufferIndex;
        copy.m_targetBufferIndex =
          allocateScratchBufferIndex(scratchBuffers, xiiImageFormat::GetBitsPerBlock(ref_path_out[0].m_sourceFormat), ref_path_out[0].m_sourceBufferIndex);
        ref_path_out[0].m_sourceBufferIndex = copy.m_targetBufferIndex;
        copy.m_step                         = nullptr;
        ref_path_out.Insert(copy, 0);
      }
      else
      {
        // Turn second step to non-inplace
        ref_path_out[1].m_inPlace = false;
        ref_path_out[1].m_sourceBufferIndex =
          allocateScratchBufferIndex(scratchBuffers, xiiImageFormat::GetBitsPerBlock(ref_path_out[1].m_sourceFormat), ref_path_out[0].m_sourceBufferIndex);
        ref_path_out[0].m_targetBufferIndex = ref_path_out[1].m_sourceBufferIndex;
      }
    }
  }
  else
  {
    ref_path_out[0].m_sourceBufferIndex = scratchBuffers.GetCount();
  }

  ref_uiNumScratchBuffers_out = scratchBuffers.GetCount() - 1;

  return XII_SUCCESS;
}

void xiiImageConversion::RebuildConversionTable()
{
  XII_LOCK(s_conversionTableLock);

  s_conversionTable.Clear();

  // Prime conversion table with known conversions
  for (xiiImageConversionStep* conversion = xiiImageConversionStep::GetFirstInstance(); conversion; conversion = conversion->GetNextInstance())
  {
    xiiArrayPtr<const xiiImageConversionEntry> entries = conversion->GetSupportedConversions();

    for (xiiUInt32 subIndex = 0; subIndex < (xiiUInt32)entries.GetCount(); subIndex++)
    {
      const xiiImageConversionEntry& subConversion = entries[subIndex];

      if (subConversion.m_flags.IsAnySet(xiiImageConversionFlags::InPlace))
      {
        XII_ASSERT_DEV(xiiImageFormat::IsCompressed(subConversion.m_sourceFormat) == xiiImageFormat::IsCompressed(subConversion.m_targetFormat) &&
                         xiiImageFormat::GetBitsPerBlock(subConversion.m_sourceFormat) == xiiImageFormat::GetBitsPerBlock(subConversion.m_targetFormat),
                       "In-place conversions are only allowed between formats of the same number of bits per pixel and compressedness");
      }

      if (xiiImageFormat::GetType(subConversion.m_sourceFormat) == xiiImageFormatType::PLANAR)
      {
        XII_ASSERT_DEV(xiiImageFormat::GetType(subConversion.m_targetFormat) == xiiImageFormatType::LINEAR, "Conversions from planar formats must target linear formats");
      }
      else if (xiiImageFormat::GetType(subConversion.m_targetFormat) == xiiImageFormatType::PLANAR)
      {
        XII_ASSERT_DEV(xiiImageFormat::GetType(subConversion.m_sourceFormat) == xiiImageFormatType::LINEAR, "Conversions to planar formats must sourced from linear formats");
      }

      xiiUInt32 tableIndex = MakeKey(subConversion.m_sourceFormat, subConversion.m_targetFormat);

      // Use the cheapest known conversion for each combination in case there are multiple ones
      TableEntry candidate(conversion, subConversion);

      TableEntry existing;

      if (!s_conversionTable.TryGetValue(tableIndex, existing) || candidate < existing)
      {
        s_conversionTable.Insert(tableIndex, candidate);
      }
    }
  }

  for (xiiUInt32 i = 0; i < xiiImageFormat::NUM_FORMATS; i++)
  {
    const xiiImageFormat::Enum format = static_cast<xiiImageFormat::Enum>(i);
    // Add copy-conversion (from and to same format)
    s_conversionTable.Insert(
      MakeKey(format, format), TableEntry(nullptr, xiiImageConversionEntry(xiiImageConversionEntry(format, format, xiiImageConversionFlags::InPlace))));
  }

  // Straight from http://en.wikipedia.org/wiki/Floyd-Warshall_algorithm
  for (xiiUInt32 k = 1; k < xiiImageFormat::NUM_FORMATS; k++)
  {
    for (xiiUInt32 i = 1; i < xiiImageFormat::NUM_FORMATS; i++)
    {
      if (k == i)
      {
        continue;
      }

      xiiUInt32 tableIndexIK = MakeKey(static_cast<xiiImageFormat::Enum>(i), static_cast<xiiImageFormat::Enum>(k));

      TableEntry entryIK;
      if (!s_conversionTable.TryGetValue(tableIndexIK, entryIK))
      {
        continue;
      }

      for (xiiUInt32 j = 1; j < xiiImageFormat::NUM_FORMATS; j++)
      {
        if (j == i || j == k)
        {
          continue;
        }

        xiiUInt32 tableIndexIJ = MakeKey(static_cast<xiiImageFormat::Enum>(i), static_cast<xiiImageFormat::Enum>(j));
        xiiUInt32 tableIndexKJ = MakeKey(static_cast<xiiImageFormat::Enum>(k), static_cast<xiiImageFormat::Enum>(j));

        TableEntry entryKJ;
        if (!s_conversionTable.TryGetValue(tableIndexKJ, entryKJ))
        {
          continue;
        }

        TableEntry candidate = TableEntry::chain(entryIK, entryKJ);

        TableEntry existing;
        if (candidate.isAdmissible() && candidate < s_conversionTable[tableIndexIJ])
        {
          // To Convert from format I to format J, first Convert from I to K
          s_conversionTable[tableIndexIJ] = candidate;
        }
      }
    }
  }

  s_conversionTableValid = true;
}

xiiResult xiiImageConversion::Convert(const xiiImageView& source, xiiImage& ref_target, xiiImageFormat::Enum targetFormat)
{
  XII_PROFILE_SCOPE("xiiImageConversion::Convert");

  xiiImageFormat::Enum sourceFormat = source.GetImageFormat();

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

  xiiHybridArray<ConversionPathNode, 16> path;
  xiiUInt32                              numScratchBuffers = 0;
  if (BuildPath(sourceFormat, targetFormat, &source == &ref_target, path, numScratchBuffers).Failed())
  {
    return XII_FAILURE;
  }

  return Convert(source, ref_target, path, numScratchBuffers);
}

xiiResult xiiImageConversion::Convert(const xiiImageView& source, xiiImage& ref_target, xiiArrayPtr<ConversionPathNode> path, xiiUInt32 uiNumScratchBuffers)
{
  XII_ASSERT_DEV(path.GetCount() > 0, "Invalid conversion path");
  XII_ASSERT_DEV(path[0].m_sourceFormat == source.GetImageFormat(), "Invalid conversion path");

  xiiHybridArray<xiiImage, 16> intermediates;
  intermediates.SetCount(uiNumScratchBuffers);

  const xiiImageView* pSource = &source;

  for (xiiUInt32 i = 0; i < path.GetCount(); ++i)
  {
    xiiUInt32 targetIndex = path[i].m_targetBufferIndex;

    xiiImage* pTarget = targetIndex == 0 ? &ref_target : &intermediates[targetIndex - 1];

    if (ConvertSingleStep(path[i].m_step, *pSource, *pTarget, path[i].m_targetFormat).Failed())
    {
      return XII_FAILURE;
    }

    pSource = pTarget;
  }

  return XII_SUCCESS;
}

xiiResult xiiImageConversion::ConvertRaw(
  xiiConstByteBlobPtr  source,
  xiiByteBlobPtr       target,
  xiiUInt32            uiNumElements,
  xiiImageFormat::Enum sourceFormat,
  xiiImageFormat::Enum targetFormat)
{
  if (uiNumElements == 0)
  {
    return XII_SUCCESS;
  }

  // Trivial copy
  if (sourceFormat == targetFormat)
  {
    if (target.GetPtr() != source.GetPtr())
      memcpy(target.GetPtr(), source.GetPtr(), uiNumElements * xiiUInt64(xiiImageFormat::GetBitsPerPixel(sourceFormat)) / 8);
    return XII_SUCCESS;
  }

  if (xiiImageFormat::IsCompressed(sourceFormat) || xiiImageFormat::IsCompressed(targetFormat))
  {
    return XII_FAILURE;
  }

  xiiHybridArray<ConversionPathNode, 16> path;
  xiiUInt32                              numScratchBuffers;
  if (BuildPath(sourceFormat, targetFormat, source.GetPtr() == target.GetPtr(), path, numScratchBuffers).Failed())
  {
    return XII_FAILURE;
  }

  return ConvertRaw(source, target, uiNumElements, path, numScratchBuffers);
}

xiiResult xiiImageConversion::ConvertRaw(
  xiiConstByteBlobPtr             source,
  xiiByteBlobPtr                  target,
  xiiUInt32                       uiNumElements,
  xiiArrayPtr<ConversionPathNode> path,
  xiiUInt32                       uiNumScratchBuffers)
{
  XII_ASSERT_DEV(path.GetCount() > 0, "Path of length 0 is invalid.");

  if (uiNumElements == 0)
  {
    return XII_SUCCESS;
  }

  if (xiiImageFormat::IsCompressed(path.GetPtr()->m_sourceFormat) || xiiImageFormat::IsCompressed((path.GetEndPtr() - 1)->m_targetFormat))
  {
    return XII_FAILURE;
  }

  xiiHybridArray<xiiBlob, 16> intermediates;
  intermediates.SetCount(uiNumScratchBuffers);

  for (xiiUInt32 i = 0; i < path.GetCount(); ++i)
  {
    xiiUInt32 targetIndex = path[i].m_targetBufferIndex;
    xiiUInt32 targetBpp   = xiiImageFormat::GetBitsPerPixel(path[i].m_targetFormat);

    xiiByteBlobPtr stepTarget;
    if (targetIndex == 0)
    {
      stepTarget = target;
    }
    else
    {
      xiiUInt32 expectedSize = static_cast<xiiUInt32>(targetBpp * uiNumElements / 8);
      intermediates[targetIndex - 1].SetCountUninitialized(expectedSize);
      stepTarget = intermediates[targetIndex - 1].GetByteBlobPtr();
    }

    if (path[i].m_step == nullptr)
    {
      memcpy(stepTarget.GetPtr(), source.GetPtr(), uiNumElements * targetBpp / 8);
    }
    else
    {
      if (static_cast<const xiiImageConversionStepLinear*>(path[i].m_step)
            ->ConvertPixels(source, stepTarget, uiNumElements, path[i].m_sourceFormat, path[i].m_targetFormat)
            .Failed())
      {
        return XII_FAILURE;
      }
    }

    source = stepTarget;
  }

  return XII_SUCCESS;
}

xiiResult xiiImageConversion::ConvertSingleStep(
  const xiiImageConversionStep* pStep,
  const xiiImageView&           source,
  xiiImage&                     target,
  xiiImageFormat::Enum          targetFormat)
{
  if (!pStep)
  {
    target.ResetAndCopy(source);
    return XII_SUCCESS;
  }

  xiiImageFormat::Enum sourceFormat = source.GetImageFormat();

  xiiImageHeader header = source.GetHeader();
  header.SetImageFormat(targetFormat);
  target.ResetAndAlloc(header);

  switch (MakeTypeKey(xiiImageFormat::GetType(sourceFormat), xiiImageFormat::GetType(targetFormat)))
  {
    case MakeTypeKey(xiiImageFormatType::LINEAR, xiiImageFormatType::LINEAR):
    {
      // we have to do the computation in 64-bit otherwise it might overflow for very large textures (8k x 4k or bigger).
      xiiUInt64 numElements = xiiUInt64(8) * target.GetByteBlobPtr().GetCount() / (xiiUInt64)xiiImageFormat::GetBitsPerPixel(targetFormat);
      return static_cast<const xiiImageConversionStepLinear*>(pStep)->ConvertPixels(
        source.GetByteBlobPtr(), target.GetByteBlobPtr(), (xiiUInt32)numElements, sourceFormat, targetFormat);
    }

    case MakeTypeKey(xiiImageFormatType::LINEAR, xiiImageFormatType::BLOCK_COMPRESSED):
      return ConvertSingleStepCompress(source, target, sourceFormat, targetFormat, pStep);

    case MakeTypeKey(xiiImageFormatType::LINEAR, xiiImageFormatType::PLANAR):
      return ConvertSingleStepPlanarize(source, target, sourceFormat, targetFormat, pStep);

    case MakeTypeKey(xiiImageFormatType::BLOCK_COMPRESSED, xiiImageFormatType::LINEAR):
      return ConvertSingleStepDecompress(source, target, sourceFormat, targetFormat, pStep);

    case MakeTypeKey(xiiImageFormatType::PLANAR, xiiImageFormatType::LINEAR):
      return ConvertSingleStepDeplanarize(source, target, sourceFormat, targetFormat, pStep);

    default:
      XII_ASSERT_NOT_IMPLEMENTED;
      return XII_FAILURE;
  }
}

xiiResult xiiImageConversion::ConvertSingleStepDecompress(
  const xiiImageView&           source,
  xiiImage&                     target,
  xiiImageFormat::Enum          sourceFormat,
  xiiImageFormat::Enum          targetFormat,
  const xiiImageConversionStep* pStep)
{
  for (xiiUInt32 arrayIndex = 0; arrayIndex < source.GetNumArrayIndices(); arrayIndex++)
  {
    for (xiiUInt32 face = 0; face < source.GetNumFaces(); face++)
    {
      for (xiiUInt32 mipLevel = 0; mipLevel < source.GetNumMipLevels(); mipLevel++)
      {
        const xiiUInt32 width  = target.GetWidth(mipLevel);
        const xiiUInt32 height = target.GetHeight(mipLevel);

        const xiiUInt32 blockSizeX = xiiImageFormat::GetBlockWidth(sourceFormat);
        const xiiUInt32 blockSizeY = xiiImageFormat::GetBlockHeight(sourceFormat);

        const xiiUInt32 numBlocksX = source.GetNumBlocksX(mipLevel);
        const xiiUInt32 numBlocksY = source.GetNumBlocksY(mipLevel);

        const xiiUInt64 targetRowPitch      = target.GetRowPitch(mipLevel);
        const xiiUInt32 targetBytesPerPixel = xiiImageFormat::GetBitsPerPixel(targetFormat) / 8;

        // Decompress into a temp memory block so we don't have to explicitly handle the case where the image is not a multiple of the block
        // size
        xiiHybridArray<xiiUInt8, 256> tempBuffer;
        tempBuffer.SetCount(numBlocksX * blockSizeX * blockSizeY * targetBytesPerPixel);

        for (xiiUInt32 slice = 0; slice < source.GetDepth(mipLevel); slice++)
        {
          for (xiiUInt32 blockY = 0; blockY < numBlocksY; blockY++)
          {
            xiiImageView sourceRowView = source.GetRowView(mipLevel, face, arrayIndex, blockY, slice);

            if (static_cast<const xiiImageConversionStepDecompressBlocks*>(pStep)
                  ->DecompressBlocks(sourceRowView.GetByteBlobPtr(), xiiByteBlobPtr(tempBuffer.GetData(), tempBuffer.GetCount()), numBlocksX,
                                     sourceFormat, targetFormat)
                  .Failed())
            {
              return XII_FAILURE;
            }

            for (xiiUInt32 blockX = 0; blockX < numBlocksX; blockX++)
            {
              xiiUInt8* targetPointer = target.GetPixelPointer<xiiUInt8>(mipLevel, face, arrayIndex, blockX * blockSizeX, blockY * blockSizeY, slice);

              // Copy into actual target, clamping to image dimensions
              xiiUInt32 copyWidth  = xiiMath::Min(blockSizeX, width - blockX * blockSizeX);
              xiiUInt32 copyHeight = xiiMath::Min(blockSizeY, height - blockY * blockSizeY);
              for (xiiUInt32 row = 0; row < copyHeight; row++)
              {
                memcpy(targetPointer, &tempBuffer[(blockX * blockSizeX + row) * blockSizeY * targetBytesPerPixel],
                       xiiMath::SafeMultiply32(copyWidth, targetBytesPerPixel));
                targetPointer += targetRowPitch;
              }
            }
          }
        }
      }
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiImageConversion::ConvertSingleStepCompress(
  const xiiImageView&           source,
  xiiImage&                     target,
  xiiImageFormat::Enum          sourceFormat,
  xiiImageFormat::Enum          targetFormat,
  const xiiImageConversionStep* pStep)
{
  for (xiiUInt32 arrayIndex = 0; arrayIndex < source.GetNumArrayIndices(); arrayIndex++)
  {
    for (xiiUInt32 face = 0; face < source.GetNumFaces(); face++)
    {
      for (xiiUInt32 mipLevel = 0; mipLevel < source.GetNumMipLevels(); mipLevel++)
      {
        const xiiUInt32 sourceWidth  = source.GetWidth(mipLevel);
        const xiiUInt32 sourceHeight = source.GetHeight(mipLevel);

        const xiiUInt32 numBlocksX = target.GetNumBlocksX(mipLevel);
        const xiiUInt32 numBlocksY = target.GetNumBlocksY(mipLevel);

        const xiiUInt32 targetWidth  = numBlocksX * xiiImageFormat::GetBlockWidth(targetFormat);
        const xiiUInt32 targetHeight = numBlocksY * xiiImageFormat::GetBlockHeight(targetFormat);

        const xiiUInt64 sourceRowPitch      = source.GetRowPitch(mipLevel);
        const xiiUInt32 sourceBytesPerPixel = xiiImageFormat::GetBitsPerPixel(sourceFormat) / 8;

        // Pad image to multiple of block size for compression
        xiiImageHeader paddedSliceHeader;
        paddedSliceHeader.SetWidth(targetWidth);
        paddedSliceHeader.SetHeight(targetHeight);
        paddedSliceHeader.SetImageFormat(sourceFormat);

        xiiImage paddedSlice;
        paddedSlice.ResetAndAlloc(paddedSliceHeader);

        for (xiiUInt32 slice = 0; slice < source.GetDepth(mipLevel); slice++)
        {
          for (xiiUInt32 y = 0; y < targetHeight; ++y)
          {
            xiiUInt32 sourceY = xiiMath::Min(y, sourceHeight - 1);

            memcpy(paddedSlice.GetPixelPointer<void>(0, 0, 0, 0, y), source.GetPixelPointer<void>(mipLevel, face, arrayIndex, 0, sourceY, slice),
                   static_cast<size_t>(sourceRowPitch));

            for (xiiUInt32 x = sourceWidth; x < targetWidth; ++x)
            {
              memcpy(paddedSlice.GetPixelPointer<void>(0, 0, 0, x, y),
                     source.GetPixelPointer<void>(mipLevel, face, arrayIndex, sourceWidth - 1, sourceY, slice), sourceBytesPerPixel);
            }
          }

          xiiResult result = static_cast<const xiiImageConversionStepCompressBlocks*>(pStep)->CompressBlocks(paddedSlice.GetByteBlobPtr(),
                                                                                                             target.GetSliceView(mipLevel, face, arrayIndex, slice).GetByteBlobPtr(), numBlocksX, numBlocksY, sourceFormat, targetFormat);

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

xiiResult xiiImageConversion::ConvertSingleStepDeplanarize(
  const xiiImageView&           source,
  xiiImage&                     target,
  xiiImageFormat::Enum          sourceFormat,
  xiiImageFormat::Enum          targetFormat,
  const xiiImageConversionStep* pStep)
{
  for (xiiUInt32 arrayIndex = 0; arrayIndex < source.GetNumArrayIndices(); arrayIndex++)
  {
    for (xiiUInt32 face = 0; face < source.GetNumFaces(); face++)
    {
      for (xiiUInt32 mipLevel = 0; mipLevel < source.GetNumMipLevels(); mipLevel++)
      {
        const xiiUInt32 width  = target.GetWidth(mipLevel);
        const xiiUInt32 height = target.GetHeight(mipLevel);

        xiiHybridArray<xiiImageView, 2> sourcePlanes;
        for (xiiUInt32 planeIndex = 0; planeIndex < source.GetPlaneCount(); ++planeIndex)
        {
          const xiiUInt32 blockSizeX = xiiImageFormat::GetBlockWidth(sourceFormat, planeIndex);
          const xiiUInt32 blockSizeY = xiiImageFormat::GetBlockHeight(sourceFormat, planeIndex);

          if (width % blockSizeX != 0 || height % blockSizeY != 0)
          {
            // Input image must be aligned to block dimensions already.
            return XII_FAILURE;
          }

          sourcePlanes.PushBack(source.GetPlaneView(mipLevel, face, arrayIndex, planeIndex));
        }

        if (static_cast<const xiiImageConversionStepDeplanarize*>(pStep)
              ->ConvertPixels(sourcePlanes, target.GetSubImageView(mipLevel, face, arrayIndex), width, height, sourceFormat, targetFormat)
              .Failed())
        {
          return XII_FAILURE;
        }
      }
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiImageConversion::ConvertSingleStepPlanarize(
  const xiiImageView&           source,
  xiiImage&                     target,
  xiiImageFormat::Enum          sourceFormat,
  xiiImageFormat::Enum          targetFormat,
  const xiiImageConversionStep* pStep)
{
  for (xiiUInt32 arrayIndex = 0; arrayIndex < source.GetNumArrayIndices(); arrayIndex++)
  {
    for (xiiUInt32 face = 0; face < source.GetNumFaces(); face++)
    {
      for (xiiUInt32 mipLevel = 0; mipLevel < source.GetNumMipLevels(); mipLevel++)
      {
        const xiiUInt32 width  = target.GetWidth(mipLevel);
        const xiiUInt32 height = target.GetHeight(mipLevel);

        xiiHybridArray<xiiImage, 2> targetPlanes;
        for (xiiUInt32 planeIndex = 0; planeIndex < target.GetPlaneCount(); ++planeIndex)
        {
          const xiiUInt32 blockSizeX = xiiImageFormat::GetBlockWidth(targetFormat, planeIndex);
          const xiiUInt32 blockSizeY = xiiImageFormat::GetBlockHeight(targetFormat, planeIndex);

          if (width % blockSizeX != 0 || height % blockSizeY != 0)
          {
            // Input image must be aligned to block dimensions already.
            return XII_FAILURE;
          }

          targetPlanes.PushBack(target.GetPlaneView(mipLevel, face, arrayIndex, planeIndex));
        }

        if (static_cast<const xiiImageConversionStepPlanarize*>(pStep)
              ->ConvertPixels(source.GetSubImageView(mipLevel, face, arrayIndex), targetPlanes, width, height, sourceFormat, targetFormat)
              .Failed())
        {
          return XII_FAILURE;
        }
      }
    }
  }

  return XII_SUCCESS;
}

bool xiiImageConversion::IsConvertible(xiiImageFormat::Enum sourceFormat, xiiImageFormat::Enum targetFormat)
{
  XII_LOCK(s_conversionTableLock);

  if (!s_conversionTableValid)
  {
    RebuildConversionTable();
  }

  xiiUInt32 tableIndex = MakeKey(sourceFormat, targetFormat);
  return s_conversionTable.Contains(tableIndex);
}

xiiImageFormat::Enum xiiImageConversion::FindClosestCompatibleFormat(
  xiiImageFormat::Enum                    format,
  xiiArrayPtr<const xiiImageFormat::Enum> compatibleFormats)
{
  XII_LOCK(s_conversionTableLock);

  if (!s_conversionTableValid)
  {
    RebuildConversionTable();
  }

  TableEntry           bestEntry;
  xiiImageFormat::Enum bestFormat = xiiImageFormat::UNKNOWN;

  for (xiiUInt32 targetIndex = 0; targetIndex < xiiUInt32(compatibleFormats.GetCount()); targetIndex++)
  {
    xiiUInt32  tableIndex = MakeKey(format, compatibleFormats[targetIndex]);
    TableEntry candidate;
    if (s_conversionTable.TryGetValue(tableIndex, candidate) && candidate < bestEntry)
    {
      bestEntry  = candidate;
      bestFormat = compatibleFormats[targetIndex];
    }
  }

  return bestFormat;
}

XII_STATICLINK_FILE(Texture, Texture_Image_Implementation_ImageConversion);
