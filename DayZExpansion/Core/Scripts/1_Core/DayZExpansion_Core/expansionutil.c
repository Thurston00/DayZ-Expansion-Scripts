class ExpansionUtil<Class T>
{
	static void Swap(inout T a, inout T b)
	{
		T c = a;

		a = b;
		b = c;
	}
}
