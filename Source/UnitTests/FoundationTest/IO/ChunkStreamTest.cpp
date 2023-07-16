#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/Deque.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/MemoryStream.h>

XII_CREATE_SIMPLE_TEST(IO, ChunkStream)
{
  xiiDefaultMemoryStreamStorage StreamStorage;

  xiiMemoryStreamWriter MemoryWriter(&StreamStorage);
  xiiMemoryStreamReader MemoryReader(&StreamStorage);

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Write Format")
  {
    xiiChunkStreamWriter writer(MemoryWriter);

    writer.BeginStream(1);

    {
      writer.BeginChunk("Chunk1", 1);

      writer << (xiiUInt32)4;
      writer << (float)5.6f;
      writer << (double)7.8;
      writer << "nine";
      writer << xiiVec3(10, 11.2f, 13.4f);

      writer.EndChunk();
    }

    {
      writer.BeginChunk("Chunk2", 2);

      writer << "chunk 2 content";

      writer.EndChunk();
    }

    {
      writer.BeginChunk("Chunk3", 3);

      writer << "chunk 3 content";

      writer.EndChunk();
    }

    writer.EndStream();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Read Format")
  {
    xiiChunkStreamReader reader(MemoryReader);

    reader.BeginStream();

    // Chunk 1
    {
      XII_TEST_BOOL(reader.GetCurrentChunk().m_bValid);
      XII_TEST_STRING(reader.GetCurrentChunk().m_sChunkName.GetData(), "Chunk1");
      XII_TEST_INT(reader.GetCurrentChunk().m_uiChunkVersion, 1);
      XII_TEST_INT(reader.GetCurrentChunk().m_uiChunkBytes, reader.GetCurrentChunk().m_uiUnreadChunkBytes);
      XII_TEST_INT(reader.GetCurrentChunk().m_uiChunkBytes, 36);

      xiiUInt32 i;
      float     f;
      double    d;
      xiiString s;

      reader >> i;
      reader >> f;
      reader >> d;
      reader >> s;

      XII_TEST_INT(i, 4);
      XII_TEST_FLOAT(f, 5.6f, 0);
      XII_TEST_DOUBLE(d, 7.8, 0);
      XII_TEST_STRING(s.GetData(), "nine");

      XII_TEST_INT(reader.GetCurrentChunk().m_uiUnreadChunkBytes, 12);
      reader.NextChunk();
    }

    // Chunk 2
    {
      XII_TEST_BOOL(reader.GetCurrentChunk().m_bValid);
      XII_TEST_STRING(reader.GetCurrentChunk().m_sChunkName.GetData(), "Chunk2");
      XII_TEST_INT(reader.GetCurrentChunk().m_uiChunkVersion, 2);
      XII_TEST_INT(reader.GetCurrentChunk().m_uiChunkBytes, reader.GetCurrentChunk().m_uiUnreadChunkBytes);
      XII_TEST_INT(reader.GetCurrentChunk().m_uiChunkBytes, 19);

      xiiString s;

      reader >> s;

      XII_TEST_STRING(s.GetData(), "chunk 2 content");

      XII_TEST_INT(reader.GetCurrentChunk().m_uiUnreadChunkBytes, 0);
      reader.NextChunk();
    }

    // Chunk 3
    {
      XII_TEST_BOOL(reader.GetCurrentChunk().m_bValid);
      XII_TEST_STRING(reader.GetCurrentChunk().m_sChunkName.GetData(), "Chunk3");
      XII_TEST_INT(reader.GetCurrentChunk().m_uiChunkVersion, 3);
      XII_TEST_INT(reader.GetCurrentChunk().m_uiChunkBytes, reader.GetCurrentChunk().m_uiUnreadChunkBytes);
      XII_TEST_INT(reader.GetCurrentChunk().m_uiChunkBytes, 19);

      xiiString s;

      reader >> s;

      XII_TEST_STRING(s.GetData(), "chunk 3 content");

      XII_TEST_INT(reader.GetCurrentChunk().m_uiUnreadChunkBytes, 0);
      reader.NextChunk();
    }

    XII_TEST_BOOL(!reader.GetCurrentChunk().m_bValid);

    reader.SetEndChunkFileMode(xiiChunkStreamReader::EndChunkFileMode::SkipToEnd);
    reader.EndStream();

    xiiUInt8 Temp[1024];
    XII_TEST_INT(MemoryReader.ReadBytes(Temp, 1024), 0); // nothing left to read
  }
}
