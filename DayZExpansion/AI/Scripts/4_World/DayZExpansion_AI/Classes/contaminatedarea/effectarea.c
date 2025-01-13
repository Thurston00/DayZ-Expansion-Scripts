modded class EffectArea
{
	static ref TTypenameArray s_Expansion_DangerousAreaTypes = {ContaminatedArea_Base, GeyserArea, HotSpringArea, VolcanicArea};
	static ref Expansion_EffectAreas s_Expansion_DangerousAreas = {};

	ref ExpansionEffectAreaMergedCluster m_Expansion_MergedCluster;

	override void InitZoneServer()
	{
	#ifdef EXTRACE
		auto trace = EXTrace.StartStack(EXTrace.AI, this);
	#endif

		super.InitZoneServer();

		if (ExpansionStatic.IsAnyOf(this, s_Expansion_DangerousAreaTypes))
			s_Expansion_DangerousAreas.UpdateClusters(this);
	}

	override void OnVariablesSynchronized()
	{
	#ifdef EXTRACE
		auto trace = EXTrace.StartStack(EXTrace.AI, this);
	#endif

		super.OnVariablesSynchronized();

#ifdef DIAG_DEVELOPER
		m_Position = GetWorldPosition();

		if (m_Radius && (m_NegativeHeight || m_PositiveHeight))
		{
			//! @note can't check inheritance against dangerous area types on client since the areas are EffectAreas on client
			//! Since we are only using this to draw debug shapes it should be fine
			EXTrace.Print(EXTrace.AI, null, ExpansionStatic.GetDebugInfo(this) + " " + m_Position.ToString() + " -> UpdateClusters");
			s_Expansion_DangerousAreas.UpdateClusters(this);
		}
#endif
	}

	override void OnPlayerEnterServer(PlayerBase player, EffectTrigger trigger)
	{
		super.OnPlayerEnterServer(player, trigger);

		if (ExpansionStatic.IsAnyOf(this, s_Expansion_DangerousAreaTypes))
			player.Expansion_OnDangerousAreaEnterServer(this, trigger);
	}

	override void OnPlayerExitServer(PlayerBase player, EffectTrigger trigger)
	{
		super.OnPlayerExitServer(player, trigger);

		if (ExpansionStatic.IsAnyOf(this, s_Expansion_DangerousAreaTypes))
			player.Expansion_OnDangerousAreaExitServer(this, trigger);
	}

	void ~EffectArea()
	{
		if (!GetGame())
			return;

		if (s_Expansion_DangerousAreas)
		{
			s_Expansion_DangerousAreas.RemoveItemUnOrdered(this);

			EXTrace.Print(EXTrace.AI, this, "Removed from " + ExpansionStatic.GetDebugInfo(s_Expansion_DangerousAreas));
		}

		if (m_Expansion_MergedCluster)
		{
			//! Update cluster containing this area
			auto cluster = m_Expansion_MergedCluster;

			int areaIndex = cluster.m_Areas.Find(this);

			if (areaIndex > -1)
			{
				if (cluster.m_Areas.Count() > 1)
				{
					cluster.m_Areas.Remove(areaIndex);

					EXTrace.Print(EXTrace.AI, this, "Removed from " + cluster);

					cluster.RemoveNonOverlappingAndUpdate();
				}
			}
		}
	}

	bool Expansion_IsOverlapping(EffectArea other, bool checkVertical = true)
	{
		//! Check horizontal overlap
		float dx = m_Position[0] - other.m_Position[0];
		float dz = m_Position[2] - other.m_Position[2];
		float dist = Math.Sqrt(dx * dx + dz * dz);

		if (dist > m_Radius + other.m_Radius)
			return false;

		if (!checkVertical)
			return true;

		//! Check vertical overlap
		float minY = m_Position[1] - m_NegativeHeight;
		float maxY = m_Position[1] + m_PositiveHeight;
		float otherMinY = other.m_Position[1] - other.m_NegativeHeight;
		float otherMaxY = other.m_Position[1] + other.m_PositiveHeight;

		if (maxY < otherMinY || otherMaxY < minY)
			return false;

		return true;
	}

	bool Expansion_Contains(EffectArea other, bool checkVertical = true)
	{
		//! Check horizontal containment
		float dx = m_Position[0] - other.m_Position[0];
		float dz = m_Position[2] - other.m_Position[2];
		float dist = Math.Sqrt(dx * dx + dz * dz);

		if (m_Radius < dist + other.m_Radius)
			return false;

		if (!checkVertical)
			return true;

		//! Check vertical containment
		float minY = m_Position[1] - m_NegativeHeight;
		float maxY = m_Position[1] + m_PositiveHeight;
		float otherMinY = other.m_Position[1] - other.m_NegativeHeight;
		float otherMaxY = other.m_Position[1] + other.m_PositiveHeight;

		if (minY > otherMinY || maxY < otherMaxY)
			return false;

		return true;
	}

	/**
	 * @brief check wheter point is inside area
	 * 
	 * @param point
	 * @param tolerance   in meters (default 0)
	 */
	bool Expansion_IsPointInside(vector point, float tolerance = 0.0, float heightTolerance = 0.0)
	{
		if (!Math.IsPointInCircle(m_Position, m_Radius + tolerance, point))
			return false;

		if (point[1] < m_Position[1] - m_NegativeHeight - heightTolerance || point[1] > m_Position[1] + m_PositiveHeight + heightTolerance)
			return false;

		return true;
	}
}
