#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <RendererCore/AnimationSystem/Implementation/OzzUtils.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/io/archive.h>

xiiOzzArchiveData::xiiOzzArchiveData()  = default;
xiiOzzArchiveData::~xiiOzzArchiveData() = default;

xiiResult xiiOzzArchiveData::FetchRegularFile(const char* szFile)
{
  xiiFileReader file;
  XII_SUCCEED_OR_RETURN(file.Open(szFile));

  m_Storage.Clear();
  m_Storage.Reserve(file.GetFileSize());
  m_Storage.ReadAll(file);

  return XII_SUCCESS;
}

xiiResult xiiOzzArchiveData::FetchEmbeddedArchive(xiiStreamReader& ref_stream)
{
  char szTag[8] = "";

  ref_stream.ReadBytes(szTag, 8);
  szTag[7] = '\0';

  if (!xiiStringUtils::IsEqual(szTag, "xiiOzzAr"))
    return XII_FAILURE;

  /*const xiiTypeVersion version =*/ref_stream.ReadVersion(1);

  xiiUInt64 uiArchiveSize = 0;
  ref_stream >> uiArchiveSize;

  m_Storage.Clear();
  m_Storage.Reserve(uiArchiveSize);
  m_Storage.ReadAll(ref_stream, uiArchiveSize);

  if (m_Storage.GetStorageSize64() != uiArchiveSize)
    return XII_FAILURE;

  return XII_SUCCESS;
}

xiiResult xiiOzzArchiveData::StoreEmbeddedArchive(xiiStreamWriter& ref_stream) const
{
  const char szTag[8] = "xiOzzAr";

  XII_SUCCEED_OR_RETURN(ref_stream.WriteBytes(szTag, 8));

  ref_stream.WriteVersion(1);

  const xiiUInt64 uiArchiveSize = m_Storage.GetStorageSize64();

  ref_stream << uiArchiveSize;

  return m_Storage.CopyToStream(ref_stream);
}

xiiOzzStreamReader::xiiOzzStreamReader(const xiiOzzArchiveData& data) :
  m_Reader(&data.m_Storage)
{
}

bool xiiOzzStreamReader::opened() const
{
  return true;
}

size_t xiiOzzStreamReader::Read(void* p_buffer, size_t ui_size)
{
  return static_cast<size_t>(m_Reader.ReadBytes(p_buffer, ui_size));
}

size_t xiiOzzStreamReader::Write(const void* p_buffer, size_t ui_size)
{
  XII_ASSERT_NOT_IMPLEMENTED;
  return 0;
}

int xiiOzzStreamReader::Seek(int i_offset, Origin _origin)
{
  switch (_origin)
  {
    case ozz::io::Stream::kCurrent:
      m_Reader.SetReadPosition(m_Reader.GetReadPosition() + i_offset);
      break;
    case ozz::io::Stream::kEnd:
      m_Reader.SetReadPosition(m_Reader.GetByteCount64() - i_offset);
      break;
    case ozz::io::Stream::kSet:
      m_Reader.SetReadPosition(i_offset);
      break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return 0;
}

int xiiOzzStreamReader::Tell() const
{
  return static_cast<int>(m_Reader.GetReadPosition());
}

size_t xiiOzzStreamReader::Size() const
{
  return static_cast<size_t>(m_Reader.GetByteCount64());
}

xiiOzzStreamWriter::xiiOzzStreamWriter(xiiOzzArchiveData& ref_data) :
  m_Writer(&ref_data.m_Storage)
{
}

bool xiiOzzStreamWriter::opened() const
{
  return true;
}

size_t xiiOzzStreamWriter::Read(void* p_buffer, size_t ui_size)
{
  XII_ASSERT_NOT_IMPLEMENTED;
  return 0;
}

size_t xiiOzzStreamWriter::Write(const void* p_buffer, size_t ui_size)
{
  if (m_Writer.WriteBytes(p_buffer, ui_size).Failed())
    return 0;

  return ui_size;
}

int xiiOzzStreamWriter::Seek(int i_offset, Origin _origin)
{
  switch (_origin)
  {
    case ozz::io::Stream::kCurrent:
      m_Writer.SetWritePosition(m_Writer.GetWritePosition() + i_offset);
      break;
    case ozz::io::Stream::kEnd:
      m_Writer.SetWritePosition(m_Writer.GetByteCount64() - i_offset);
      break;
    case ozz::io::Stream::kSet:
      m_Writer.SetWritePosition(i_offset);
      break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return 0;
}

int xiiOzzStreamWriter::Tell() const
{
  return static_cast<int>(m_Writer.GetWritePosition());
}

size_t xiiOzzStreamWriter::Size() const
{
  return static_cast<size_t>(m_Writer.GetByteCount64());
}

void xiiOzzUtils::CopyAnimation(ozz::animation::Animation* pDst, const ozz::animation::Animation* pSrc)
{
  xiiOzzArchiveData ozzArchiveData;

  // store in ozz archive
  {
    xiiOzzStreamWriter ozzWriter(ozzArchiveData);
    ozz::io::OArchive  ozzArchive(&ozzWriter);

    ozzArchive << *pSrc;
  }

  // read it from archive again
  {
    xiiOzzStreamReader ozzReader(ozzArchiveData);
    ozz::io::IArchive  ozzArchive(&ozzReader);

    ozzArchive >> *pDst;
  }
}

XII_RENDERERCORE_DLL void xiiOzzUtils::CopySkeleton(ozz::animation::Skeleton* pDst, const ozz::animation::Skeleton* pSrc)
{
  xiiOzzArchiveData ozzArchiveData;

  // store in ozz archive
  {
    xiiOzzStreamWriter ozzWriter(ozzArchiveData);
    ozz::io::OArchive  ozzArchive(&ozzWriter);

    ozzArchive << *pSrc;
  }

  // read it from archive again
  {
    xiiOzzStreamReader ozzReader(ozzArchiveData);
    ozz::io::IArchive  ozzArchive(&ozzReader);

    ozzArchive >> *pDst;
  }
}


XII_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_Implementation_OzzUtils);
