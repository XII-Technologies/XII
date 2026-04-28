/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Memory/EndianHelper.h>
#include <Foundation/Memory/MemoryUtils.h>

void xiiEndianHelper::SwitchStruct(void* pDataPointer, const char* szFormat)
{
  XII_ASSERT_DEBUG(pDataPointer != nullptr, "Data necessary!");
  XII_ASSERT_DEBUG((szFormat != nullptr) && (szFormat[0] != '\0'), "Struct format description necessary!");

  xiiUInt8* pWorkPointer    = static_cast<xiiUInt8*>(pDataPointer);
  char      cCurrentElement = *szFormat;

  while (cCurrentElement != '\0')
  {
    switch (cCurrentElement)
    {
      case 'c':
      case 'b':
        pWorkPointer++;
        break;

      case 's':
      case 'w':
      {
        xiiUInt16* pWordElement = reinterpret_cast<xiiUInt16*>(pWorkPointer);
        *pWordElement           = Switch(*pWordElement);
        pWorkPointer += sizeof(xiiUInt16);
      }
      break;

      case 'd':
      {
        xiiUInt32* pDWordElement = reinterpret_cast<xiiUInt32*>(pWorkPointer);
        *pDWordElement           = Switch(*pDWordElement);
        pWorkPointer += sizeof(xiiUInt32);
      }
      break;

      case 'q':
      {
        xiiUInt64* pQWordElement = reinterpret_cast<xiiUInt64*>(pWorkPointer);
        *pQWordElement           = Switch(*pQWordElement);
        pWorkPointer += sizeof(xiiUInt64);
      }
      break;
    }

    szFormat++;
    cCurrentElement = *szFormat;
  }
}

void xiiEndianHelper::SwitchStructs(void* pDataPointer, const char* szFormat, xiiUInt32 uiStride, xiiUInt32 uiCount)
{
  XII_ASSERT_DEBUG(pDataPointer != nullptr, "Data necessary!");
  XII_ASSERT_DEBUG((szFormat != nullptr) && (szFormat[0] != '\0'), "Struct format description necessary!");
  XII_ASSERT_DEBUG(uiStride > 0, "Struct size necessary!");

  for (xiiUInt32 i = 0; i < uiCount; i++)
  {
    SwitchStruct(pDataPointer, szFormat);
    pDataPointer = xiiMemoryUtils::AddByteOffset(pDataPointer, uiStride);
  }
}
