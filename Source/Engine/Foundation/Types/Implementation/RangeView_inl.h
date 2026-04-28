/// Copyright (c) Theophilus Eriata. All Rights Reserved.

template <typename ValueType, typename IteratorType>
xiiRangeView<ValueType, IteratorType>::xiiRangeView(BeginCallback begin, EndCallback end, NextCallback next, ValueCallback value) :
  m_Begin(begin), m_End(end), m_Next(next), m_Value(value)
{
}

template <typename ValueType, typename IteratorType>
XII_FORCE_INLINE void xiiRangeView<ValueType, IteratorType>::ConstIterator::Next()
{
  this->m_pView->m_Next(this->m_Pos);
}

template <typename ValueType, typename IteratorType>
XII_FORCE_INLINE ValueType xiiRangeView<ValueType, IteratorType>::ConstIterator::Value() const
{
  return this->m_pView->m_Value(this->m_Pos);
}

template <typename ValueType, typename IteratorType>
XII_FORCE_INLINE bool xiiRangeView<ValueType, IteratorType>::ConstIterator::operator==(
  const typename xiiRangeView<ValueType, IteratorType>::ConstIterator& it2) const
{
  return m_pView == it2.m_pView && m_Pos == it2.m_Pos;
}

template <typename ValueType, typename IteratorType>
XII_FORCE_INLINE xiiRangeView<ValueType, IteratorType>::ConstIterator::ConstIterator(const xiiRangeView<ValueType, IteratorType>* view, IteratorType pos)
{
  m_pView = view;
  m_Pos   = pos;
}

template <typename ValueType, typename IteratorType>
XII_FORCE_INLINE ValueType xiiRangeView<ValueType, IteratorType>::Iterator::Value()
{
  return this->m_View->m_value(this->m_Pos);
}

template <typename ValueType, typename IteratorType>
xiiRangeView<ValueType, IteratorType>::Iterator::Iterator(const xiiRangeView<ValueType, IteratorType>* view, IteratorType pos) :
  xiiRangeView<ValueType, IteratorType>::ConstIterator(view, pos)
{
}
