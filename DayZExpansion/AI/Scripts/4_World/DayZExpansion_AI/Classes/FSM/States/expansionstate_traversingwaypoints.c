class ExpansionState_TraversingWaypoints: eAIState
{
	int index;
	float threshold=1.0;
	float previousDistance;
	vector waypoint;
	vector previousWaypoint;
	bool gotUp;

	override void OnEntry(string Event, ExpansionState From)
	{
		gotUp = false;
		unit.eAI_LeaveCover();
		
		eAIWaypointBehavior behaviour = unit.GetGroup().GetWaypointBehaviour();
		if (behaviour == eAIWaypointBehavior.ROAMING && waypoint == vector.Zero)
		{
			waypoint = unit.GetGroup().FindClosestRoamingLocationPosition();
		}
	}

	override void OnExit(string Event, bool Aborted, ExpansionState To)
	{
		unit.SetMovementSpeedLimit(unit.m_eAI_SpeedLimitPreference);  //! Restore speed limit
	}

	override int OnUpdate(float DeltaTime, int SimulationPrecision)
	{
		eAIWaypointBehavior behaviour = unit.GetGroup().GetWaypointBehaviour();
		
		TVectorArray path;
		if (behaviour != eAIWaypointBehavior.ROAMING)
		{
			path = unit.GetGroup().GetWaypoints();
			
			if (path.Count() == 0)
			{
				unit.Expansion_DeleteDebugObjects();
				return EXIT;
				//path = { unit.GetPosition() + unit.GetDirection() * unit.Expansion_GetMovementSpeed() };
			}
			
			index = unit.GetGroup().m_CurrentWaypointIndex;
			if (index >= path.Count())
				index = 0;
			
			waypoint = path[index];
		}
		
		if (waypoint == vector.Zero)
			return EXIT;
		
		if (!gotUp && unit.Expansion_GetMovementSpeed() > 0)
		{
			unit.Expansion_GetUp(true);
			gotUp = true;
		}
		
		unit.eAI_SetBestSpeedLimit(unit.m_eAI_SpeedLimitPreference);
		
		vector targetPosition;
		auto pathFinding = unit.GetPathFinding();
		if (pathFinding.m_TargetPosition == waypoint && !pathFinding.m_Recalculate)
		{
			//! If we only have two or less points total, we can be sure that endpoint is reachable and closest to target as possible
			if (pathFinding.m_Count <= 2)
				targetPosition = pathFinding.GetEnd();
			else
				targetPosition = pathFinding.GetTarget();
		}
		else
		{
			targetPosition = waypoint;
		}
		
		vector position = unit.GetPosition();
		position[1] = targetPosition[1];
		float distance = vector.DistanceSq(position, targetPosition);
		bool isFinal;
		bool waypointReached;
		if (distance < threshold)
			waypointReached = true;
		
		if (waypointReached || (pathFinding.m_IsUnreachable && pathFinding.m_IsTargetUnreachable))
		{
			threshold = 1.0;
			
			if (behaviour == eAIWaypointBehavior.ROAMING)
			{
				unit.GetGroup().SetRoamingLocationReached(waypointReached);
				previousWaypoint = waypoint;
				waypoint = unit.GetGroup().FindClosestRoamingLocationPosition();
			}
			else if (behaviour == eAIWaypointBehavior.HALT || path.Count() == 1 || pathFinding.m_IsUnreachable)
			{
				isFinal = true;
			}
			else
			{
				bool backtracking = unit.GetGroup().m_BackTracking;
				previousWaypoint = waypoint;
				if (backtracking) index--;
				else index++;
				if (unit.Expansion_GetMovementSpeed() > 0)
					pathFinding.ForceRecalculate(true);
				
				if (index < 0) 
				{
					if (behaviour == eAIWaypointBehavior.ALTERNATE)
					{
						backtracking = false;
						index = 1;
					}
					else if (behaviour == eAIWaypointBehavior.LOOP)
					{
						index = path.Count() - 1;
					}
					else
					{
						index = 0;
					}
				}
				else if (index >= path.Count())
				{
					if (behaviour == eAIWaypointBehavior.ALTERNATE)
					{
						backtracking = true;
						index = path.Count() - 2;
					} 
					else if (behaviour == eAIWaypointBehavior.LOOP)
					{
						index = 0;
					}
					else
					{
						index = path.Count() - 1;
					}
				}
				
				waypoint = path[index];
		
				unit.GetGroup().m_CurrentWaypointIndex = index;
				unit.GetGroup().m_BackTracking = backtracking;
				
				if (behaviour != eAIWaypointBehavior.LOOP && (index == 0 || index == path.Count() - 1))
					isFinal = true;
			}
		}
		else if (Math.AbsFloat(distance - previousDistance) < 0.0016)
		{
			//! We seem to not be getting closer to the waypoint, possibly the path is blocked.
			//! Increase threshold until we are within distance.
			threshold += 0.01;
		}
		
		previousDistance = distance;
		
		unit.OverrideTargetPosition(waypoint, isFinal);
		unit.OverrideMovementDirection(false, 0);
		unit.OverrideMovementSpeed(false, 0);
		
		vector direction;
		if (previousWaypoint != vector.Zero || (path && path.Count() > 1))
		{
			if (previousWaypoint == vector.Zero)
				previousWaypoint = path[0] - vector.Direction(path[0], path[1]);
			direction = vector.Direction(previousWaypoint, waypoint).VectorToAngles();
			direction[1] = 0.0;
			direction[2] = 0.0;
			direction = direction.AnglesToVector();
		}
		else
		{
			direction = unit.GetDirection();
		}
		unit.Expansion_DebugObject_Deferred(21 + index, waypoint - "0 1.5 0", "ExpansionDebugNoticeMe_Purple", direction);
		if (pathFinding.m_IsTargetUnreachable)
			unit.Expansion_DebugObject_Deferred(20, "0 0 0", "ExpansionDebugNoticeMe");
		else
			unit.Expansion_DebugObject_Deferred(20, waypoint, "ExpansionDebugNoticeMe", unit.GetDirection());
		
		return EXIT;
	}
};
