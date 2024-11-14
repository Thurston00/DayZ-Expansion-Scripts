modded class EffectArea
{
	bool Expansion_IsOverlapping(EffectArea other)
	{
		//! Check horizontal overlap
		float dx = m_Position[0] - other.m_Position[0];
		float dz = m_Position[2] - other.m_Position[2];
		float dist = Math.Sqrt(dx * dx + dz * dz);

		if (dist > m_Radius + other.m_Radius)
			return false;

		//! Check vertical overlap
		float minY = m_Position[1] - m_NegativeHeight;
		float maxY = m_Position[1] + m_PositiveHeight;
		float otherMinY = other.m_Position[1] - other.m_NegativeHeight;
		float otherMaxY = other.m_Position[1] + other.m_PositiveHeight;

		if (maxY < otherMinY || otherMaxY < minY)
			return false;

		return true;
	}

	bool Expansion_IsPointInside(vector point)
	{
		if (point[1] < m_Position[1] - m_NegativeHeight || point[1] > m_Position[1] + m_PositiveHeight)
			return false;

		return Math.IsPointInCircle(m_Position, m_Radius, point);
	}
}
