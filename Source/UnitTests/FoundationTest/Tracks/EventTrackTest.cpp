#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Tracks/EventTrack.h>

XII_CREATE_SIMPLE_TEST_GROUP(Tracks);

XII_CREATE_SIMPLE_TEST(Tracks, EventTrack)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Empty")
  {
    xiiEventTrack                      et;
    xiiHybridArray<xiiHashedString, 8> result;

    XII_TEST_BOOL(et.IsEmpty());
    et.Sample(xiiTime::MakeZero(), xiiTime::MakeFromSeconds(1.0), result);

    XII_TEST_BOOL(result.IsEmpty());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Sample")
  {
    xiiEventTrack                      et;
    xiiHybridArray<xiiHashedString, 8> result;

    et.AddControlPoint(xiiTime::MakeFromSeconds(3.0), "Event3");
    et.AddControlPoint(xiiTime::MakeFromSeconds(0.0), "Event0");
    et.AddControlPoint(xiiTime::MakeFromSeconds(4.0), "Event4");
    et.AddControlPoint(xiiTime::MakeFromSeconds(1.0), "Event1");
    et.AddControlPoint(xiiTime::MakeFromSeconds(2.0), "Event2");

    XII_TEST_BOOL(!et.IsEmpty());

    // sampling an empty range should yield no results, even if sampling an exact time where an event is
    {
      result.Clear();
      {
        et.Sample(xiiTime::MakeFromSeconds(0.0), xiiTime::MakeFromSeconds(0.0), result);
        XII_TEST_INT(result.GetCount(), 0);
      }

      {
        result.Clear();
        et.Sample(xiiTime::MakeFromSeconds(1.0), xiiTime::MakeFromSeconds(1.0), result);
        XII_TEST_INT(result.GetCount(), 0);
      }

      {
        result.Clear();
        et.Sample(xiiTime::MakeFromSeconds(4.0), xiiTime::MakeFromSeconds(4.0), result);
        XII_TEST_INT(result.GetCount(), 0);
      }
    }

    {
      result.Clear();
      et.Sample(xiiTime::MakeFromSeconds(0.0), xiiTime::MakeFromSeconds(1.0), result);
      XII_TEST_INT(result.GetCount(), 1);
      XII_TEST_STRING(result[0].GetString(), "Event0");
    }

    {
      result.Clear();
      et.Sample(xiiTime::MakeFromSeconds(0.0), xiiTime::MakeFromSeconds(2.0), result);
      XII_TEST_INT(result.GetCount(), 2);
      XII_TEST_STRING(result[0].GetString(), "Event0");
      XII_TEST_STRING(result[1].GetString(), "Event1");
    }

    {
      result.Clear();
      et.Sample(xiiTime::MakeFromSeconds(0.0), xiiTime::MakeFromSeconds(4.0), result);
      XII_TEST_INT(result.GetCount(), 4);
      XII_TEST_STRING(result[0].GetString(), "Event0");
      XII_TEST_STRING(result[1].GetString(), "Event1");
      XII_TEST_STRING(result[2].GetString(), "Event2");
      XII_TEST_STRING(result[3].GetString(), "Event3");
    }

    {
      result.Clear();
      et.Sample(xiiTime::MakeFromSeconds(0.0), xiiTime::MakeFromSeconds(10.0), result);
      XII_TEST_INT(result.GetCount(), 5);
      XII_TEST_STRING(result[0].GetString(), "Event0");
      XII_TEST_STRING(result[1].GetString(), "Event1");
      XII_TEST_STRING(result[2].GetString(), "Event2");
      XII_TEST_STRING(result[3].GetString(), "Event3");
      XII_TEST_STRING(result[4].GetString(), "Event4");
    }

    {
      result.Clear();
      et.Sample(xiiTime::MakeFromSeconds(-0.1), xiiTime::MakeFromSeconds(10.0), result);
      XII_TEST_INT(result.GetCount(), 5);
      XII_TEST_STRING(result[0].GetString(), "Event0");
      XII_TEST_STRING(result[1].GetString(), "Event1");
      XII_TEST_STRING(result[2].GetString(), "Event2");
      XII_TEST_STRING(result[3].GetString(), "Event3");
      XII_TEST_STRING(result[4].GetString(), "Event4");
    }

    et.Clear();
    XII_TEST_BOOL(et.IsEmpty());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Reverse Sample")
  {
    xiiEventTrack                      et;
    xiiHybridArray<xiiHashedString, 8> result;

    et.AddControlPoint(xiiTime::MakeFromSeconds(3.0), "Event3");
    et.AddControlPoint(xiiTime::MakeFromSeconds(0.0), "Event0");
    et.AddControlPoint(xiiTime::MakeFromSeconds(4.0), "Event4");
    et.AddControlPoint(xiiTime::MakeFromSeconds(1.0), "Event1");
    et.AddControlPoint(xiiTime::MakeFromSeconds(2.0), "Event2");

    {
      result.Clear();
      et.Sample(xiiTime::MakeFromSeconds(2.0), xiiTime::MakeFromSeconds(0.0), result);
      XII_TEST_INT(result.GetCount(), 2);
      XII_TEST_STRING(result[0].GetString(), "Event2");
      XII_TEST_STRING(result[1].GetString(), "Event1");
    }

    {
      result.Clear();
      et.Sample(xiiTime::MakeFromSeconds(4.0), xiiTime::MakeFromSeconds(0.0), result);
      XII_TEST_INT(result.GetCount(), 4);
      XII_TEST_STRING(result[0].GetString(), "Event4");
      XII_TEST_STRING(result[1].GetString(), "Event3");
      XII_TEST_STRING(result[2].GetString(), "Event2");
      XII_TEST_STRING(result[3].GetString(), "Event1");
    }

    {
      result.Clear();
      et.Sample(xiiTime::MakeFromSeconds(10.0), xiiTime::MakeFromSeconds(0.0), result);
      XII_TEST_INT(result.GetCount(), 4);
      XII_TEST_STRING(result[0].GetString(), "Event4");
      XII_TEST_STRING(result[1].GetString(), "Event3");
      XII_TEST_STRING(result[2].GetString(), "Event2");
      XII_TEST_STRING(result[3].GetString(), "Event1");
    }

    {
      result.Clear();
      et.Sample(xiiTime::MakeFromSeconds(10.0), xiiTime::MakeFromSeconds(-0.1), result);
      XII_TEST_INT(result.GetCount(), 5);
      XII_TEST_STRING(result[0].GetString(), "Event4");
      XII_TEST_STRING(result[1].GetString(), "Event3");
      XII_TEST_STRING(result[2].GetString(), "Event2");
      XII_TEST_STRING(result[3].GetString(), "Event1");
      XII_TEST_STRING(result[4].GetString(), "Event0");
    }
  }
}
