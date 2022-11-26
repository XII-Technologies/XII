#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Strings/String.h>

namespace
{
  typedef xiiGenericId<32, 16>   Id;
  typedef xiiConstructionCounter st;

  struct TestObject
  {
    int       x;
    xiiString s;
  };
} // namespace

XII_CREATE_SIMPLE_TEST(Containers, IdTable)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor")
  {
    xiiIdTable<Id, xiiInt32> table;

    XII_TEST_BOOL(table.GetCount() == 0);
    XII_TEST_BOOL(table.IsEmpty());

    xiiUInt32 counter = 0;
    for (xiiIdTable<Id, xiiInt32>::ConstIterator it = table.GetIterator(); it.IsValid(); ++it)
    {
      ++counter;
    }
    XII_TEST_INT(counter, 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Copy Constructor/Assignment/Iterator")
  {
    XII_TEST_BOOL(st::HasAllDestructed());
    {
      xiiIdTable<Id, st> table1;

      for (xiiInt32 i = 0; i < 200; ++i)
      {
        table1.Insert(st(i));
      }

      XII_TEST_BOOL(table1.Remove(Id(0, 1)));

      for (xiiInt32 i = 0; i < 99; ++i)
      {
        Id id;
        id.m_Generation = 1;

        do
        {
          id.m_InstanceIndex = rand() % 200;
        } while (!table1.Contains(id));

        XII_TEST_BOOL(table1.Remove(id));
      }

      xiiIdTable<Id, st> table2;
      table2 = table1;
      xiiIdTable<Id, st> table3(table1);

      XII_TEST_BOOL(table2.IsFreelistValid());
      XII_TEST_BOOL(table3.IsFreelistValid());

      XII_TEST_INT(table1.GetCount(), 100);
      XII_TEST_INT(table2.GetCount(), 100);
      XII_TEST_INT(table3.GetCount(), 100);

      xiiUInt32 uiCounter = 0;
      for (xiiIdTable<Id, st>::ConstIterator it = table1.GetIterator(); it.IsValid(); ++it)
      {
        st value;

        XII_TEST_BOOL(table2.TryGetValue(it.Id(), value));
        XII_TEST_BOOL(it.Value() == value);

        XII_TEST_BOOL(table3.TryGetValue(it.Id(), value));
        XII_TEST_BOOL(it.Value() == value);

        ++uiCounter;
      }
      XII_TEST_INT(uiCounter, table1.GetCount());

      for (xiiIdTable<Id, st>::Iterator it = table1.GetIterator(); it.IsValid(); ++it)
      {
        it.Value() = st(42);
      }

      for (xiiIdTable<Id, st>::ConstIterator it = table1.GetIterator(); it.IsValid(); ++it)
      {
        st value;

        XII_TEST_BOOL(table1.TryGetValue(it.Id(), value));
        XII_TEST_BOOL(it.Value() == value);
        XII_TEST_BOOL(value.m_iData == 42);
      }
    }
    XII_TEST_BOOL(st::HasAllDestructed());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Verify 0 is never valid")
  {
    xiiIdTable<Id, TestObject> table;

    xiiUInt32 count1 = 0, count2 = 0;

    TestObject x = {11, "Test"};

    while (true)
    {
      Id id = table.Insert(x);
      XII_TEST_BOOL(id.m_Generation != 0);

      XII_TEST_BOOL(table.Remove(id));

      if (id.m_Generation > 1) // until all elements in generation 1 have been used up
        break;

      ++count1;
    }

    XII_TEST_BOOL(!table.Contains(Id(0, 0)));

    while (true)
    {
      Id id = table.Insert(x);
      XII_TEST_BOOL(id.m_Generation != 0);

      XII_TEST_BOOL(table.Remove(id));

      if (id.m_Generation == 1) // wrap around
        break;

      ++count2;
    }

    XII_TEST_BOOL(!table.Contains(Id(0, 0)));

    XII_TEST_INT(count1, 32);
    XII_TEST_INT(count2, 2097087);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Insert/Remove")
  {
    xiiIdTable<Id, TestObject> table;

    for (int i = 0; i < 100; i++)
    {
      TestObject x  = {rand(), "Test"};
      Id         id = table.Insert(x);
      XII_TEST_INT(id.m_InstanceIndex, i);
      XII_TEST_INT(id.m_Generation, 1);

      XII_TEST_BOOL(table.Contains(id));

      TestObject y = table[id];
      XII_TEST_INT(x.x, y.x);
      XII_TEST_BOOL(x.s == y.s);
    }
    XII_TEST_INT(table.GetCount(), 100);

    Id ids[10] = {Id(13, 1), Id(0, 1), Id(16, 1), Id(34, 1), Id(56, 1), Id(57, 1), Id(79, 1), Id(85, 1), Id(91, 1), Id(97, 1)};


    for (int i = 0; i < 10; i++)
    {
      bool res = table.Remove(ids[i]);
      XII_TEST_BOOL(res);
      XII_TEST_BOOL(!table.Contains(ids[i]));
    }
    XII_TEST_INT(table.GetCount(), 90);

    for (int i = 0; i < 40; i++)
    {
      TestObject x     = {1000, "Bla. This is a very long string which does not fit into 32 byte and will cause memory allocations."};
      Id         newId = table.Insert(x);

      XII_TEST_BOOL(table.Contains(newId));

      TestObject y = table[newId];
      XII_TEST_INT(x.x, y.x);
      XII_TEST_BOOL(x.s == y.s);

      TestObject* pObj;
      XII_TEST_BOOL(table.TryGetValue(newId, pObj));
      XII_TEST_BOOL(pObj->s == x.s);
    }
    XII_TEST_INT(table.GetCount(), 130);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Crash test")
  {
    xiiIdTable<Id, TestObject> table;
    xiiDynamicArray<Id>        ids;

    for (xiiUInt32 i = 0; i < 100000; ++i)
    {
      int action = rand() % 2;
      if (action == 0)
      {
        TestObject x = {rand(), "Test"};
        ids.PushBack(table.Insert(x));
      }
      else
      {
        if (ids.GetCount() > 0)
        {
          xiiUInt32 index = rand() % ids.GetCount();
          XII_TEST_BOOL(table.Remove(ids[index]));
          ids.RemoveAtAndSwap(index);
        }
      }

      XII_TEST_BOOL(table.IsFreelistValid());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Clear")
  {
    XII_TEST_BOOL(st::HasAllDestructed());

    xiiIdTable<Id, st> m1;
    Id                 id0 = m1.Insert(st(1));
    XII_TEST_BOOL(st::HasDone(2, 1)); // for inserting new elements 1 temporary is created (and destroyed)

    Id id1 = m1.Insert(st(3));
    XII_TEST_BOOL(st::HasDone(2, 1)); // for inserting new elements 1 temporary is created (and destroyed)

    m1[id0] = st(2);
    XII_TEST_BOOL(st::HasDone(1, 1)); // nothing new to create, so only the one temporary is used

    m1.Clear();
    XII_TEST_BOOL(st::HasDone(0, 2));
    XII_TEST_BOOL(st::HasAllDestructed());

    XII_TEST_BOOL(!m1.Contains(id0));
    XII_TEST_BOOL(!m1.Contains(id1));
    XII_TEST_BOOL(m1.IsFreelistValid());
  }

  /*XII_TEST_BLOCK(xiiTestBlock::Enabled, "Remove/Compact")
  {
    xiiIdTable<Id, st> a;

    for (xiiInt32 i = 0; i < 1000; ++i)
    {
      a.Insert(i);
      XII_TEST_INT(a.GetCount(), i + 1);
    }

    a.Compact();
    XII_TEST_BOOL(a.IsFreelistValid());

    {
      xiiUInt32 i = 0;
      for (xiiIdTable<Id, st>::Iterator it = a.GetIterator(); it.IsValid(); ++it)
      {
        XII_TEST_INT(a[it.Id()].m_iData, i);
        ++i;
      }
    }

    for (xiiInt32 i = 500; i < 1000; ++i)
    {
      st oldValue;
      XII_TEST_BOOL(a.Remove(Id(i, 0), &oldValue));
      XII_TEST_INT(oldValue.m_iData, i);
    }

    a.Compact();
    XII_TEST_BOOL(a.IsFreelistValid());

    {
      xiiUInt32 i = 0;
      for (xiiIdTable<Id, st>::Iterator it = a.GetIterator(); it.IsValid(); ++it)
      {
        XII_TEST_INT(a[it.Id()].m_iData, i);
        ++i;
      }
    }
  }*/
}
