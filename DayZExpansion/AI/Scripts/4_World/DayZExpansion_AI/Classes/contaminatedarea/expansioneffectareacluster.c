class Expansion_EffectAreas: array<EffectArea>
{
	/**
	 * @brief Find clusters of overlapping areas
	 */
	Expansion_EffectArea_Clusters FindClusters()
	{
	#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI, this);
	#endif

		Expansion_EffectArea_Clusters clusters = {};
		set<int> cluster_i;
		set<int> cluster_j;
		map<int, ref set<int>> lookup = new map<int, ref set<int>>;
		set<ref set<int>> seen = new set<ref set<int>>;
		int count = Count();
		int countLess1 = count - 1;
		EffectArea area;

		for (int i = 0; i < countLess1; i++)
		{
			area = Get(i);

			for (int j = i + 1; j < count; j++)
			{
				EffectArea other = Get(j);

				if (area.Expansion_IsOverlapping(other))
				{
					cluster_i = lookup[i];
					cluster_j = lookup[j];
	
					if (!cluster_i && !cluster_j)
					{
						InsertCluster(lookup, new set<int>, i, j);
					}
					else if (cluster_i != cluster_j)
					{
						if (cluster_i)
						{
							if (cluster_j)
							{
								foreach (int k: cluster_i)
								{
									lookup[k] = cluster_j;
								}

								cluster_j.InsertSet(cluster_i);
							}
							else
							{
								InsertCluster(lookup, cluster_i, j, -1);
							}
						}
						else
						{
							cluster_j.Insert(i);
							cluster_j.Insert(j);
							lookup[i] = cluster_j;
						}
					}
					else
					{
						cluster_i.Insert(j);
					}
				}
			}

			if (!lookup[i])
			{
				//! Not overlapping
				InsertCluster(lookup, new set<int>, i, -1);
			}
		}

		if (!lookup[countLess1])
		{
			//! Not overlapping
			InsertCluster(lookup, new set<int>, countLess1, -1);
		}

		foreach (int key, set<int> cluster: lookup)
		{
			if (seen.Find(cluster) == -1)
			{
				seen.Insert(cluster);

				Expansion_EffectAreas areas = {};

			#ifdef DIAG_DEVELOPER
				TStringArray areasInfo = {};
			#endif

				foreach (int index: cluster)
				{
					area = Get(index);
					areas.Insert(area);

				#ifdef DIAG_DEVELOPER
					areasInfo.Insert(string.Format("%1 (pos=%2 radius=%3 nheight=%4 pheight=%5)", ExpansionStatic.GetDebugInfo(area), area.m_Position.ToString(), area.m_Radius, area.m_NegativeHeight, area.m_PositiveHeight));
				#endif
				}

			#ifdef DIAG_DEVELOPER
				EXTrace.Print(EXTrace.AI, null, "  0x" + ExpansionStatic.GetInstanceID(cluster) + " {");
				foreach (string areaInfo: areasInfo)
				{
					EXTrace.Print(EXTrace.AI, null, "    " + areaInfo);
				}
				EXTrace.Print(EXTrace.AI, null, "  }");
			#endif

				clusters.Insert(areas);
			}
		}

		return clusters;
	}

	private void InsertCluster(map<int, ref set<int>> lookup, set<int> cluster, int i, int j)
	{
		cluster.Insert(i);
		lookup[i] = cluster;

		if (j != -1)
		{
			cluster.Insert(j);
			lookup[j] = cluster;
		}
	}

	void UpdateClusters(EffectArea area)
	{
		ExpansionEffectAreaMergedCluster cluster;
		int count = Count();

		for (int i = 0; i < count; i++)
		{
			EffectArea other = Get(i);

			if (cluster != other.m_Expansion_MergedCluster && area.Expansion_IsOverlapping(other))
			{
				if (!cluster)
				{
					cluster = other.m_Expansion_MergedCluster;
					cluster.m_Areas.Insert(area);
					cluster.Update();
				}
				else
				{
					cluster.m_Areas.InsertAll(other.m_Expansion_MergedCluster.m_Areas);
					cluster.Update();
				}
			}
		}

		Insert(area);

		if (!cluster)
		{
			//! Not overlapping
			Expansion_EffectAreas areas = {};
			areas.Insert(area);
			cluster = new ExpansionEffectAreaMergedCluster(areas);
		}
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
	#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI, this);
	#endif

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

#ifdef DIAG_DEVELOPER
	protected ref array<Shape> m_DebugShapes = {};
#endif

	void ExpansionEffectAreaMergedCluster(Expansion_EffectAreas cluster)
	{
		m_Areas = cluster;

		Update();
	}

#ifdef DIAG_DEVELOPER
	void ~ExpansionEffectAreaMergedCluster()
	{
		CleanupDebugShapes();
	}
#endif

	/**
	 * @brief Update the cluster. Needs to be called each time m_Areas is changed
	 */
	void Update()
	{
	#ifdef EXTRACE_DIAG
		auto trace = EXTrace.Start(EXTrace.AI, this);
	#endif

		float xSum;
		float ySum;
		float zSum;

		float minY = float.MAX;
		float maxY = -float.MAX;

		float dx;
		float dz;

		int i;
		int count = m_Areas.Count();

		EffectArea area;

	#ifdef DIAG_DEVELOPER
		TStringArray areasInfo = {};
	#ifndef SERVER
		CleanupDebugShapes();
	#endif
	#endif

		//! Calculate the center of the merged cylinder

		for (i = 0; i < count; i++)
		{
			area = m_Areas[i];

			area.m_Expansion_MergedCluster = this;

			xSum += area.m_Position[0];
			ySum += area.m_Position[1];
			zSum += area.m_Position[2];

			minY = Math.Min(minY, area.m_Position[1] - area.m_NegativeHeight);
			maxY = Math.Max(maxY, area.m_Position[1] + area.m_PositiveHeight);

		#ifdef DIAG_DEVELOPER
			areasInfo.Insert(string.Format("%1 (pos=%2 radius=%3 nheight=%4 pheight=%5)", ExpansionStatic.GetDebugInfo(area), area.m_Position.ToString(), area.m_Radius, area.m_NegativeHeight, area.m_PositiveHeight));
		#endif
		}

		m_Position[0] = xSum / count;
		m_Position[1] = ySum / count;
		m_Position[2] = zSum / count;

		m_NegativeHeight = m_Position[1] - minY;
		m_PositiveHeight = maxY - m_Position[1];

		if (count > 1)
		{
			//! Find the outermost edges of all areas in this cluster (from center of merged cylinder)

		#ifdef DIAG_DEVELOPER
			EXTrace.Print(EXTrace.AI, null, string.Format("  %1 (pos=%2)", ExpansionStatic.GetDebugInfo(this), m_Position.ToString()));
		#ifndef SERVER
			m_DebugShapes.Insert(Debug.DrawArrow(m_Position, m_Position + Vector(0, m_PositiveHeight + 10, 0), 0.5, COLOR_RED));
		#endif
		#endif

			int reach;
			TIntArray distancesFromCenter = {};
			map<int, ref Expansion_EffectAreas> areasByReach = new map<int, ref Expansion_EffectAreas>;
			Expansion_EffectAreas areas;

			for (i = 0; i < count; i++)
			{
				area = m_Areas[i];

				dx = area.m_Position[0] - m_Position[0];
				dz = area.m_Position[2] - m_Position[2];

				reach = Math.Sqrt(dx * dx + dz * dz) + area.m_Radius;

				distancesFromCenter.Insert(reach);

				if (areasByReach.Find(reach, areas))
					areas.Insert(area);
				else
					areasByReach[reach] = {area};
			}

			//! Find up to three points furthest from center

			distancesFromCenter.Sort();

			Expansion_EffectAreas outermost = {};
			vector dir;
			vector point;
			TVectorArray points = {};

			int n;

			for (i = count - 1; i >= 0; i--)
			{
				reach = distancesFromCenter[i];
	
				areas = areasByReach[reach];
				area = areas[0];
				areas.Remove(0);

				outermost.Insert(area);

				dir[0] = area.m_Position[0] - m_Position[0];
				dir[2] = area.m_Position[2] - m_Position[2];

				point = area.m_Position + dir.Normalized() * area.m_Radius;

			#ifdef DIAG_DEVELOPER
			#ifndef SERVER
				m_DebugShapes.Insert(Debug.DrawArrow(point, point + Vector(0, m_PositiveHeight + 10, 0), 0.5, COLOR_BLUE));
			#endif
			#endif

				point[1] = 0;

				points.Insert(point);

				n++;

				if (n == 3)
					break;
			}

			//! Use the found points to calculate a new, optimized center

			vector a = points[0];
			vector b = points[1];

			vector ab = b - a;

			float distSqAB = ab.LengthSq();

			vector center;

			if (n == 3)
			{
				vector c = points[2];

				vector bc = c - b;
				vector ca = a - c;

				float distSqBC = bc.LengthSq();
				float distSqCA = ca.LengthSq();

				//! Make side A-B the longest

				if (distSqBC < distSqCA)
				{
					ExpansionUtil<float>.Swap(distSqBC, distSqCA);
					ExpansionUtil<vector>.Swap(a, b);
				}

				if (distSqAB < distSqBC)
				{
					ExpansionUtil<float>.Swap(distSqAB, distSqBC);
					ExpansionUtil<vector>.Swap(c, a);
				}

				if (distSqBC + distSqCA <= distSqAB)
				{
					//! Obtuse triangle, use longest diameter

					center = (a + b) * 0.5;

					m_Position[0] = center[0];
					m_Position[2] = center[2];

					m_Radius = Math.Sqrt(distSqAB) * 0.5;
				}
				else
				{
					//! Acute triangle, use circumcenter

					center = ExpansionMath.Circumcenter(a, b, c);

					m_Position[0] = center[0];
					m_Position[2] = center[2];

					//! Find a radius that encompasses the outer edges

					for (i = 0; i < n; i++)
					{
						area = outermost[i];

						dx = area.m_Position[0] - m_Position[0];
						dz = area.m_Position[2] - m_Position[2];

					#ifdef DIAG_DEVELOPER
					#ifndef SERVER
						dir[0] = dx;
						dir[2] = dz;
						point = area.m_Position + dir.Normalized() * area.m_Radius;
						m_DebugShapes.Insert(Debug.DrawArrow(point, point + Vector(0, m_PositiveHeight + 10, 0), 0.5, COLOR_GREEN));
					#endif
					#endif

						float distanceToEdge = Math.Sqrt(dx * dx + dz * dz) + area.m_Radius;

						m_Radius = Math.Max(m_Radius, distanceToEdge);
					}
				}
			}
			else
			{
				//! Exactly two

				center = (a + b) * 0.5;

				m_Position[0] = center[0];
				m_Position[2] = center[2];

				m_Radius = Math.Sqrt(distSqAB) * 0.5;
			}
		}
		else
		{
			m_Radius = m_Areas[0].m_Radius;
		}

	#ifdef DIAG_DEVELOPER
		EXTrace.Print(EXTrace.AI, null, string.Format("  %1 (pos=%2 radius=%3 nheight=%4 pheight=%5) {", ExpansionStatic.GetDebugInfo(this), m_Position.ToString(), m_Radius, m_NegativeHeight, m_PositiveHeight));
		foreach (string areaInfo: areasInfo)
		{
			EXTrace.Print(EXTrace.AI, null, "    " + areaInfo);
		}
		EXTrace.Print(EXTrace.AI, null, "  }");
	#endif

	#ifdef DIAG_DEVELOPER
		DrawDebug();
	#endif
	}

#ifdef DIAG_DEVELOPER
	void DrawDebug()
	{
	#ifndef SERVER
		//CleanupDebugShapes();

		//if (DiagMenu.GetBool(DiagMenuIDs.TRIGGER_DEBUG))
		//{
			float height = m_NegativeHeight + m_PositiveHeight;
			vector center = Vector(m_Position[0], m_Position[1] - m_NegativeHeight + height * 0.5, m_Position[2]);

			m_DebugShapes.Insert(Debug.DrawCylinder(center, m_Radius, height, ARGB(15, 255, 255, 255), ShapeFlags.TRANSP | ShapeFlags.NOZWRITE));
		//}
	#endif
	}
	
	protected void CleanupDebugShapes()
	{
		foreach (Shape shape: m_DebugShapes)
		{
			Debug.RemoveShape(shape);
		}

		m_DebugShapes.Clear();
	}
#endif

	bool IsPointInside(vector point, float tolerance = 0.0, float heightTolerance = 0.0)
	{
		if (!Math.IsPointInCircle(m_Position, m_Radius + tolerance, point))
			return false;

		if (point[1] < m_Position[1] - m_NegativeHeight - heightTolerance || point[1] > m_Position[1] + m_PositiveHeight + heightTolerance)
			return false;

		return true;
	}

	/**
	 * @brief Find closest point outside cluster that intersect the path defined by points start and end
	 * 
	 * @param start
	 * @param [inout] end  will be altered in-place if path intersects cylinder
	 * @param avoidanceDirection  positive = left, negative = right
	 * @param [out] closestArea
	 * 
	 * @return true if path intersects cylinder, else false
	 */
	bool FindClosestPointOutsideCluster(vector start, inout vector end, float avoidanceDirection = 1.0, bool ignoreHeight = false, out EffectArea closestArea = null, DayZPlayerImplement player = null)
	{
	#ifdef DIAG_DEVELOPER
		vector dir;
	#endif

		float actualHeight = m_NegativeHeight + m_PositiveHeight;
		vector ground = m_Position;
		vector center;
		float avoidanceDistance = Math.Max(Math.Min(m_Radius * 0.1, 15.0), 3.0);
		float radius = m_Radius + avoidanceDistance;
		float margin = 3.0;

		float height;
		bool isInside;

		if (ignoreHeight)
		{
			height = 10000.0;  //! We could use ExpansionMath.IntersectRayCircle, but since it's not native, IntersectRayCylinder should be faster
			center = Vector(ground[0], ground[1] - m_NegativeHeight + actualHeight * 0.5 - height * 0.5, ground[2]);
			isInside = Math.IsPointInCircle(center, radius + margin, start);
		}
		else
		{
			height = actualHeight;
			center = Vector(ground[0], ground[1] - m_NegativeHeight, ground[2]);
			isInside = ExpansionMath.IsPointInCylinder(center, radius + margin, height, start);
		}

		//! Check if ray intersects any area cluster
		if (isInside)
		{
			if (m_Areas.Count() > 1)
			{
				//! Get closest area in cluster (distance to outer edge)

				float closestDistSq = float.MAX;
				vector toCenter;

				foreach (int i, auto area: m_Areas)
				{
				#ifdef DIAG_DEVELOPER
					dir = area.m_Position - start;
					dir[1] = 0;
					player.Expansion_DebugObject(3143 + i, area.m_Position - "0 1.5 0", "ExpansionDebugNoticeMe_Yellow", dir);
				#endif

					center[0] = area.m_Position[0];
					center[2] = area.m_Position[2];

					//! Since we already know we are inside cluster, we can simplify area check by using IsPointInCircle (we ignore area height)
					if (Math.IsPointInCircle(center, area.m_Radius, start))
					{
						//! If we are inside area, there's no point in checking distances, we need to get out!
						closestArea = area;
						break;
					}

					toCenter = area.m_Position - start;
					float distSq = vector.DistanceSq(start, area.m_Position - toCenter.Normalized() * area.m_Radius);

					if (distSq < closestDistSq)
					{
						closestArea = area;
						closestDistSq = distSq;
					}
				}
			}
			else
			{
				closestArea = m_Areas[0];
			}

			ground = closestArea.m_Position;

			//! @note we check against cluster center Y + height to make sure we stay at outside perimeter of area
			//! in case it goes partially underground
			center[0] = closestArea.m_Position[0];
			center[2] = closestArea.m_Position[2];

			avoidanceDistance = Math.Max(Math.Min(closestArea.m_Radius * 0.1, 15.0), 3.0);
			radius = closestArea.m_Radius + avoidanceDistance;

			//! Since we already know we are inside cluster, we can simplify area check by using IsPointInCircle (we ignore area height)
			//! for checking whether or not startpoint is in circle.
			//! We still need to check intersection to take into account line from start to endpoint in this case.
			if (!Math.IsPointInCircle(center, closestArea.m_Radius + 3.0, start) && !Math3D.IntersectRayCylinder(start, end, center, closestArea.m_Radius, height))
			{
			#ifdef DIAG_DEVELOPER
				//dir = end - start;
				//dir[1] = 0;
				//player.Expansion_DebugObject(3141, end - "0 1.5 0", "ExpansionDebugNoticeMe_Black", dir, start);
				player.Expansion_DebugObject(3143 + m_Areas.Count(), vector.Zero, "ExpansionDebugNoticeMe_White");
			#endif
				return false;
			}

			float perpendicularDistance = avoidanceDirection * avoidanceDistance;
			end = ExpansionMath.GetClosestPointOutsideRadius(start, end, center, radius, perpendicularDistance);

		#ifdef DIAG_DEVELOPER
			//player.Expansion_DebugObject(3141, vector.Zero, "ExpansionDebugNoticeMe_Black");
			dir = ground - start;
			dir[1] = 0;
			player.Expansion_DebugObject(3143 + m_Areas.Count(), ground, "ExpansionDebugNoticeMe_White", dir, end, 300.0, ShapeFlags.NOZBUFFER);
		#endif

			return true;
		}

		return false;
	}
}

class ExpansionEffectAreaMergedClusters: array<ref ExpansionEffectAreaMergedCluster>
{
	/**
	 * @brief Find closest point outside all clusters that intersect the path defined by points start and end
	 * 
	 * @param start
	 * @param [inout] end  will be altered in-place if path intersects cylinder
	 * @param avoidanceDirection  positive = left, negative = right
	 * @param [out] closestArea
	 * 
	 * @return true if path intersects cylinder, else false
	 */
	bool FindClosestPointOutsideAnyCluster(vector start, inout vector end, float avoidanceDirection = 1.0, bool ignoreHeight = false, out EffectArea closestArea = null, DayZPlayerImplement player = null)
	{
		bool found;

		foreach (auto cluster: this)
		{
			found = cluster.FindClosestPointOutsideCluster(start, end, avoidanceDirection, ignoreHeight, closestArea, player);

			if (found)
				break;
		}

		return found;
	}
}
