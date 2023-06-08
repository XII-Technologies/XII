
#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/DataProcessing/Stream/DefaultImplementations/ZeroInitializer.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>
#include <Foundation/Reflection/Reflection.h>

XII_CREATE_SIMPLE_TEST_GROUP(DataProcessing);

// Add processor

class AddOneStreamProcessor : public xiiProcessingStreamProcessor
{
  XII_ADD_DYNAMIC_REFLECTION(AddOneStreamProcessor, xiiProcessingStreamProcessor);

public:
  AddOneStreamProcessor() = default;

  void SetStreamName(xiiHashedString sStreamName) { m_sStreamName = sStreamName; }

protected:
  virtual xiiResult UpdateStreamBindings() override
  {
    m_pStream = m_pStreamGroup->GetStreamByName(m_sStreamName);

    return m_pStream ? XII_SUCCESS : XII_FAILURE;
  }

  virtual void InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements) override {}

  virtual void Process(xiiUInt64 uiNumElements) override
  {
    xiiProcessingStream::DataType Type = m_pStream->GetDataType();

    switch (Type)
    {
      case xiiProcessingStream::DataType::Float:
      {
        xiiProcessingStreamIterator<float> streamIterator(m_pStream, uiNumElements, 0);

        while (!streamIterator.HasReachedEnd())
        {
          streamIterator.Current() += 1.0f;

          streamIterator.Advance();
        }
      }
      break;

      case xiiProcessingStream::DataType::Double:
      {
        xiiProcessingStreamIterator<double> streamIterator(m_pStream, uiNumElements, 0);

        while (!streamIterator.HasReachedEnd())
        {
          streamIterator.Current() += 1.0;

          streamIterator.Advance();
        }
      }
      break;

      default:
        break;
    }
  }

  xiiHashedString      m_sStreamName;
  xiiProcessingStream* m_pStream = nullptr;
};

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(AddOneStreamProcessor, 1, xiiRTTIDefaultAllocator<AddOneStreamProcessor>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_CREATE_SIMPLE_TEST(DataProcessing, ProcessingStream)
{
  xiiProcessingStreamGroup Group;
  xiiProcessingStream*     pStream1 = Group.AddStream("Stream1", xiiProcessingStream::DataType::Float);
  xiiProcessingStream*     pStream2 = Group.AddStream("Stream2", xiiProcessingStream::DataType::Float3);
  xiiProcessingStream*     pStream3 = Group.AddStream("Stream3", xiiProcessingStream::DataType::Double);
  xiiProcessingStream*     pStream4 = Group.AddStream("Stream4", xiiProcessingStream::DataType::Double3);

  XII_TEST_BOOL(pStream1 != nullptr);
  XII_TEST_BOOL(pStream2 != nullptr);
  XII_TEST_BOOL(pStream3 != nullptr);
  XII_TEST_BOOL(pStream4 != nullptr);

  xiiProcessingStreamSpawnerZeroInitialized* pSpawner1 = XII_DEFAULT_NEW(xiiProcessingStreamSpawnerZeroInitialized);
  xiiProcessingStreamSpawnerZeroInitialized* pSpawner2 = XII_DEFAULT_NEW(xiiProcessingStreamSpawnerZeroInitialized);
  xiiProcessingStreamSpawnerZeroInitialized* pSpawner3 = XII_DEFAULT_NEW(xiiProcessingStreamSpawnerZeroInitialized);
  xiiProcessingStreamSpawnerZeroInitialized* pSpawner4 = XII_DEFAULT_NEW(xiiProcessingStreamSpawnerZeroInitialized);

  pSpawner1->SetStreamName(pStream1->GetName());
  pSpawner2->SetStreamName(pStream2->GetName());
  pSpawner3->SetStreamName(pStream3->GetName());
  pSpawner4->SetStreamName(pStream4->GetName());

  Group.AddProcessor(pSpawner1);
  Group.AddProcessor(pSpawner2);
  Group.AddProcessor(pSpawner3);
  Group.AddProcessor(pSpawner4);

  Group.SetSize(128);

  XII_TEST_INT(Group.GetNumElements(), 128);
  XII_TEST_INT(Group.GetNumActiveElements(), 0);

  Group.InitializeElements(3);

  Group.Process();

  XII_TEST_INT(Group.GetNumActiveElements(), 3);


  {
    xiiProcessingStreamIterator<float> stream1Iterator(pStream1, 3, 0);

    int iElementsVisited = 0;
    while (!stream1Iterator.HasReachedEnd())
    {
      XII_TEST_FLOAT(stream1Iterator.Current(), 0.0f, 0.0f);

      stream1Iterator.Advance();
      iElementsVisited++;
    }

    XII_TEST_INT(iElementsVisited, 3);
  }

  Group.InitializeElements(7);

  Group.Process();

  {
    xiiProcessingStreamIterator<xiiVec3> stream2Iterator(pStream2, Group.GetNumActiveElements(), 0);

    int iElementsVisited = 0;
    while (!stream2Iterator.HasReachedEnd())
    {
      XII_TEST_FLOAT(stream2Iterator.Current().x, 0.0f, 0.0f);
      XII_TEST_FLOAT(stream2Iterator.Current().y, 0.0f, 0.0f);
      XII_TEST_FLOAT(stream2Iterator.Current().z, 0.0f, 0.0f);

      stream2Iterator.Advance();
      iElementsVisited++;
    }

    XII_TEST_INT(iElementsVisited, 10);
  }

  Group.InitializeElements(3);

  Group.Process();

  {
    xiiProcessingStreamIterator<double> stream3Iterator(pStream3, 3, 0);

    int iElementsVisited = 0;
    while (!stream3Iterator.HasReachedEnd())
    {
      XII_TEST_DOUBLE(stream3Iterator.Current(), 0.0, 0.0);

      stream3Iterator.Advance();
      iElementsVisited++;
    }

    XII_TEST_INT(iElementsVisited, 3);
  }

  Group.InitializeElements(7);

  Group.Process();

  {
    xiiProcessingStreamIterator<xiiVec3d> stream4Iterator(pStream4, Group.GetNumActiveElements(), 0);

    int iElementsVisited = 0;
    while (!stream4Iterator.HasReachedEnd())
    {
      XII_TEST_DOUBLE(stream4Iterator.Current().x, 0.0, 0.0);
      XII_TEST_DOUBLE(stream4Iterator.Current().y, 0.0, 0.0);
      XII_TEST_DOUBLE(stream4Iterator.Current().z, 0.0, 0.0);

      stream4Iterator.Advance();
      iElementsVisited++;
    }

    XII_TEST_INT(iElementsVisited, 20);
  }

  XII_TEST_INT(Group.GetHighestNumActiveElements(), 20);

  Group.RemoveElement(5);
  Group.RemoveElement(7);
  Group.RemoveElement(3);
  Group.RemoveElement(7);

  Group.Process();

  XII_TEST_INT(Group.GetHighestNumActiveElements(), 20);
  XII_TEST_INT(Group.GetNumActiveElements(), 17);

  AddOneStreamProcessor* pProcessor1 = XII_DEFAULT_NEW(AddOneStreamProcessor);
  pProcessor1->SetStreamName(pStream1->GetName());

  Group.AddProcessor(pProcessor1);

  Group.Process();

  {
    xiiProcessingStreamIterator<float> stream1Iterator(pStream1, Group.GetNumActiveElements(), 0);
    while (!stream1Iterator.HasReachedEnd())
    {
      XII_TEST_FLOAT(stream1Iterator.Current(), 1.0f, 0.001f);

      stream1Iterator.Advance();
    }
  }

  Group.Process();

  {
    xiiProcessingStreamIterator<float> stream1Iterator(pStream1, Group.GetNumActiveElements(), 0);
    while (!stream1Iterator.HasReachedEnd())
    {
      XII_TEST_FLOAT(stream1Iterator.Current(), 2.0f, 0.001f);

      stream1Iterator.Advance();
    }
  }

  AddOneStreamProcessor* pProcessor3 = XII_DEFAULT_NEW(AddOneStreamProcessor);
  pProcessor3->SetStreamName(pStream3->GetName());

  Group.AddProcessor(pProcessor3);

  Group.Process();

  {
    xiiProcessingStreamIterator<double> stream3Iterator(pStream3, Group.GetNumActiveElements(), 0);
    while (!stream3Iterator.HasReachedEnd())
    {
      XII_TEST_DOUBLE(stream3Iterator.Current(), 1.0, 0.001);

      stream3Iterator.Advance();
    }
  }

  Group.Process();

  {
    xiiProcessingStreamIterator<double> stream3Iterator(pStream3, Group.GetNumActiveElements(), 0);
    while (!stream3Iterator.HasReachedEnd())
    {
      XII_TEST_DOUBLE(stream3Iterator.Current(), 2.0, 0.001);

      stream3Iterator.Advance();
    }
  }
}
