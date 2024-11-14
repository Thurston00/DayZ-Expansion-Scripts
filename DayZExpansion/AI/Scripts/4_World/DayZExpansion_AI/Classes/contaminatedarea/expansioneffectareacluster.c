class Expansion_EffectAreas: array<EffectArea>
{
	/**
	 * @brief Find clusters of overlapping areas
	 */
	Expansion_EffectArea_Clusters FindClusters()
	{
		Expansion_EffectArea_Clusters clusters = {};
		set<int> cluster;
		set<int> cluster_i;
		set<int> cluster_j;
		map<int, ref set<int>> lookup = new map<int, ref set<int>>;
		set<ref set<int>> seen = new set<ref set<int>>;

		for (int i = 0; i < Count() - 1; i++)
		{
			EffectArea area = Get(i);

			for (int j = i + 1; j < Count(); j++)
			{
				EffectArea other = Get(j);

				if (area.Expansion_IsOverlapping(other))
				{
					cluster_i = lookup[i];
					cluster_j = lookup[j];
	
					if (!cluster_i && !cluster_j)
					{
						cluster = new set<int>;
						cluster.Insert(i);
						cluster.Insert(j);
						lookup[i] = cluster;
						lookup[j] = cluster;
					}
					else if (cluster_i != cluster_j)
					{
						if (cluster_i)
						{
							if (cluster_j)
							{
								foreach (int k: cluster_i)
								{
									lookup.Remove(k);
								}

								cluster_j.InsertSet(cluster_i);
							}
							else
							{
								cluster = cluster_i;
								lookup[j] = cluster;
								cluster.Insert(j);
							}
						}
						else
						{
							cluster = cluster_j;
							cluster.Insert(i);
							cluster.Insert(j);
						}
					}
					else
					{
						cluster = cluster_i;
						cluster.Insert(j);
					}
				}
			}
		}

		foreach (int key, set<int> indices: lookup)
		{
			if (seen.Find(indices) == -1)
			{
				seen.Insert(indices);

				Expansion_EffectAreas areas = {};

				foreach (int index: indices)
				{
					areas.Insert(Get(index));
				}

				clusters.Insert(areas);
			}
		}

		return clusters;
	}

	/**
	 * @brief Find closest point outside all areas that intersect the path defined by points start and end
	 * 
	 * @param start
	 * @param [inout] end  will be altered in-place if path intersects cylinder
	 * @param perpendicularDistance  positive = left, negative = right
	 * 
	 * @return true if path intersects cylinder, else false
	 */
	bool FindClosestPointOutsideAnyArea(vector start, inout vector end, float perpendicularDistance = 15.0)
	{
		bool found;

		foreach (auto area: this)
		{
			float height = area.m_NegativeHeight + area.m_PositiveHeight;
			vector center = Vector(area.m_Position[0], area.m_Position[1] - area.m_NegativeHeight + height * 0.5, area.m_Position[2]);
			float radius = area.m_Radius + 15.0;

			if (ExpansionMath.FindClosestPointOutsideCylinder(start, end, center, radius, height, perpendicularDistance))
				found = true;
		}

		return found;
	}
}

class Expansion_EffectArea_Clusters: array<ref Expansion_EffectAreas>
{
	/**
	 * @brief Merge cluster of effect areas
	 * 
	 * @note a merged cluster is simply a large cylinder encompassing all the overlapping areas
	 */
	ExpansionEffectAreaMergedClusters MergeClusters()
	{
		ExpansionEffectAreaMergedClusters mergedClusters = {};

		foreach (auto cluster: this)
		{
			mergedClusters.Insert(new ExpansionEffectAreaMergedCluster(cluster));
		}

		return mergedClusters;
	}
}

class ExpansionEffectAreaMergedCluster
{
	ref Expansion_EffectAreas m_Areas;
	vector m_Position;
	float m_Radius;
	float m_NegativeHeight;
	float m_PositiveHeight;

	void ExpansionEffectAreaMergedCluster(Expansion_EffectAreas cluster)
	{
		m_Areas = cluster;

		Update();
	}

	/**
	 * @brief Update the cluster. Needs to be called each time m_Areas is changed
	 */
	void Update()
	{
	#ifdef EXTRACE_DIAG
		auto trace = EXTrace.StartStack(EXTrace.AI, this);
	#endif

		float xSum = 0, ySum = 0, zSum = 0;
		float minY = float.MAX;
		float maxY = -float.MAX;

		int i;
		int count = m_Areas.Count();

		EffectArea area;

		//! Calculate the center of the merged cylinder

		for (i = 0; i < count; i++)
		{
			area = m_Areas[i];

			xSum += area.m_Position[0];
			ySum += area.m_Position[1];
			zSum += area.m_Position[2];
		}

		m_Position[0] = xSum / count;
		m_Position[1] = ySum / count;
		m_Position[2] = zSum / count;

		//! Calculate a radius that encompasses all areas, starting from center of merged cylinder, and neg/pos height

		for (i = 0; i < count; i++)
		{
			area = m_Areas[i];

			float dx = area.m_Position[0] - m_Position[0];
			float dz = area.m_Position[2] - m_Position[2];
			float distanceToEdge = Math.Sqrt(dx * dx + dz * dz) + area.m_Radius;

			m_Radius = Math.Max(m_Radius, distanceToEdge);

			minY = Math.Min(minY, area.m_Position[1] - area.m_NegativeHeight);
			maxY = Math.Max(maxY, area.m_Position[1] + area.m_PositiveHeight);
		}

		m_NegativeHeight = m_Position[1] - minY;
		m_PositiveHeight = maxY - m_Position[1];

		EXTrace.Print(EXTrace.AI, this, "::Update areas=" + count + " pos=" + m_Position + " radius=" + m_Radius + " nheight=" + m_NegativeHeight + " pheight=" + m_PositiveHeight);
	}
}

class ExpansionEffectAreaMergedClusters: array<ref ExpansionEffectAreaMergedCluster>
{
	/**
	 * @brief Find closest point outside all clusters that intersect the path defined by points start and end
	 * 
	 * @param start
	 * @param [inout] end  will be altered in-place if path intersects cylinder
	 * @param perpendicularDistance  positive = left, negative = right
	 * @param [out] closestArea
	 * 
	 * @return true if path intersects cylinder, else false
	 */
	bool FindClosestPointOutsideAnyCluster(vector start, inout vector end, float perpendicularDistance = 15.0, out EffectArea closestArea = null, DayZPlayerImplement player = null)
	{
		bool found;

		foreach (auto cluster: this)
		{
			float height = cluster.m_NegativeHeight + cluster.m_PositiveHeight;
			vector ground = cluster.m_Position;
			vector center = Vector(ground[0], ground[1] - cluster.m_NegativeHeight + height * 0.5, ground[2]);
			float radius = cluster.m_Radius + 15.0;

			//! Check if ray intersects cluster
			if (ExpansionMath.IsPointInCylinder(center, radius, height, start) || Math3D.IntersectRayCylinder(start, end, center, radius, height))
			{
				if (cluster.m_Areas.Count() > 1)
				{
					//! Get closest area in cluster (distance to outer edge)

					float closestDistSq = float.MAX;
					vector toCenter;

					foreach (int i, auto area: cluster.m_Areas)
					{
						toCenter = area.m_Position - start;
						float distSq = vector.DistanceSq(start, area.m_Position - toCenter.Normalized() * area.m_Radius);

						if (distSq < closestDistSq)
						{
							closestArea = area;
							closestDistSq = distSq;
						}

					#ifdef DIAG_DEVELOPER
						player.Expansion_DebugObject(3143 + i, area.m_Position + "0 1.5 0", "ExpansionDebugNoticeMe_Yellow");
					#endif
					}

					height = closestArea.m_NegativeHeight + closestArea.m_PositiveHeight;
					ground = closestArea.m_Position;
					center = Vector(ground[0], ground[1] - closestArea.m_NegativeHeight + height * 0.5, ground[2]);
					radius = closestArea.m_Radius + 15.0;

					if (!ExpansionMath.IsPointInCylinder(center, radius, height, start) && !Math3D.IntersectRayCylinder(start, end, center, radius, height))
						break;
				}
				else
				{
					closestArea = cluster.m_Areas[0];
				}

				found = true;

				end = ExpansionMath.GetClosestPointOutsideRadius(start, end, center, radius, perpendicularDistance);

			#ifdef DIAG_DEVELOPER
				player.Expansion_DebugObject(3143 + cluster.m_Areas.Count(), ground + "0 3.0 0", "ExpansionDebugNoticeMe_White", vector.Zero, end, 300.0, ShapeFlags.NOZBUFFER);
			#endif

				break;
			}
		}

		return found;
	}
}
