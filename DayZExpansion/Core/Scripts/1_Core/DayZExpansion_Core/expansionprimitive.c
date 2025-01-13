class ExpansionPrimitive: Managed
{
	typename GetValueType()
	{
		return typename;
	}

	string ToString(bool type = false, bool name = false, bool quotes = true)
	{
		return string.ToString(string.Empty, type, name, quotes);
	}
}

class ExpansionPrimitiveT<Class T>: ExpansionPrimitive
{
	T m_Value;

	void ExpansionPrimitiveT(T value)
	{
		m_Value = value;
	}

	T GetValue()
	{
		return m_Value;
	}

	override typename GetValueType()
	{
		return T;
	}

	override string ToString(bool type = false, bool name = false, bool quotes = true)
	{
		return string.ToString(m_Value, type, name, quotes);
	}
}
