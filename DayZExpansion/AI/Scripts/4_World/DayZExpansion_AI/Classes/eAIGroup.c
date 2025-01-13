// TODO: sync to the client automatically within DayZPlayerImplement
//  only data that needs to be known is just the id, members and faction
class eAIGroup
{
	static const int VERSION = 1;
	static const string BASENAME = "group.bin";

	private static autoptr array<ref eAIGroup> s_AllGroups = new array<ref eAIGroup>();
	static ref array<eAIGroup> s_PersistentGroups = {};
	static int s_eAI_CurrentPersistIndex = -1;

	private static int s_IDCounter = 0;

	private autoptr array<eAITargetInformation> m_Targets = {};
	private int m_ID;
	int m_NextGroupMemberID;

	//! Refer to eAIGroup::GetTargetInformation
	private autoptr eAIGroupTargetInformation m_TargetInformation;

	// Ordered array of group members. 0 is the leader.
	private autoptr array<DayZPlayerImplement> m_Members = {};
	private autoptr array<DayZPlayerImplement> m_Members_Deceased = {};
	private int m_MemberCount = 1;

	// What formation the group should keep
	private autoptr eAIFormation m_Form;

	// Group identity
	private ref eAIFaction m_Faction;
	private string m_Name;
	bool m_Persist;
	string m_BaseName;

	private autoptr array<vector> m_Waypoints;
	private eAIWaypointBehavior m_WaypointBehaviour = eAIWaypointBehavior.ALTERNATE;
	int m_CurrentWaypointIndex;
	bool m_BackTracking;
	vector m_CurrentWaypoint;

	ref array<ref ExpansionLocatorArray> m_RoamingLocations = {};
	//vector m_CurrentRoamingLocationPosition;
	ref set<BuildingBase> m_VisitedBuildings = new set<BuildingBase>;
	ref set<Object> m_VisitedCrashSites =  new set<Object>;
	ref ExpansionLocatorArray m_RoamingLocation;
	float m_RoamingLocationReachedTimestamp;
	int m_ClearVisitedBuildingsOnLocationReached;

	private eAIGroupFormationState m_FormationState = eAIGroupFormationState.IN;

	bool m_UpdateSearchPosition;
	protected bool m_IsInCombat;
	bool m_DangerousAreaAvoidanceDirection;

	// return the group owned by leader, otherwise create a new one.
	static eAIGroup GetGroupByLeader(DayZPlayerImplement leader, bool createIfNoneExists = true, eAIFaction faction = null, bool autoDeleteFormerGroupIfEmpty = true)
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_2("eAIGroup", "eAIGroup::GetGroupByLeader").Add(leader).Add(createIfNoneExists).Add(faction);
#endif

		for (int i = 0; i < s_AllGroups.Count(); i++)
			if (s_AllGroups[i].GetLeader() == leader)
				return s_AllGroups[i];

		if (!createIfNoneExists)
			return null;

		eAIGroup group = CreateGroup(faction);
		leader.SetGroup(group, autoDeleteFormerGroupIfEmpty);
		return group;
	}

	override string GetDebugName()
	{
		string str = ToString();
		
		str += ", ";
		str += "faction=" + m_Faction;

		return str;
	}

	static array<ref eAIGroup> GetAll()
	{
		return s_AllGroups;
	}

	static eAIGroup GetGroupByID(int id, bool createIfNoneExists = false)
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_2("eAIGroup", "eAIGroup::GetGroupByID").Add(id).Add(createIfNoneExists);
#endif

		for (int i = 0; i < s_AllGroups.Count(); i++)
			if (s_AllGroups[i].GetID() == id)
				return s_AllGroups[i];

		if (!createIfNoneExists)
			return null;

		eAIGroup group = new eAIGroup();
		group.m_ID = id;
		return group;
	}

	static eAIGroup CreateGroup(eAIFaction faction = null)
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_0("eAIGroup", "eAIGroup::CreateGroup");
#endif

		eAIGroup group = new eAIGroup(faction);

		s_IDCounter++;
		group.m_ID = s_IDCounter;

		return group;
	}

	static void DeleteGroup(eAIGroup group)
	{
#ifdef EXTRACE_DIAG
		auto trace = EXTrace.Start(EXTrace.AI, eAIGroup, "" + group);
#endif

		int index = s_AllGroups.Find(group);
		if (index > -1)
		{
		#ifdef DIAG_DEVELOPER
			EXTrace.Print(EXTrace.AI, eAIGroup, "Removing group from s_AllGroups");
		#endif
			s_AllGroups.Remove(index);
		}

		if (group && group.m_Persist && group.m_BaseName)
			s_PersistentGroups.RemoveItem(group);
	}

	private void eAIGroup(eAIFaction faction = null)
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_0(this, "eAIGroup");
#endif

		m_TargetInformation = new eAIGroupTargetInformation(this);

		m_Form = new eAIFormationVee(this);

		if (faction)
			m_Faction = faction;
		else
			m_Faction = new eAIFactionRaiders();

		m_Waypoints = new array<vector>();

		s_AllGroups.Insert(this);
	}

	/*private*/ void ~eAIGroup()
	{
		if (!GetGame())
			return;

	#ifdef DIAG_DEVELOPER
		EXTrace.Print(EXTrace.AI, this, "~eAIGroup");
	#endif

		DeleteGroup(this);
	}

	void Delete()
	{
#ifdef EXTRACE_DIAG
		auto trace = EXTrace.Start(EXTrace.AI, this);
#endif

		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Call(DeleteGroup, this);
	}

	void AddWaypoint(vector pos)
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_0(this, "AddWaypoint");
#endif

		m_Waypoints.Insert(pos);
	}

	array<vector> GetWaypoints()
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_0(this, "GetWaypoints");
#endif

		return m_Waypoints;
	}

	void SetCurrentWaypoint(vector pos)
	{
		m_CurrentWaypoint = pos;
	}

	vector GetCurrentWaypoint()
	{
		return m_CurrentWaypoint;
	}

	void ClearWaypoints()
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_0(this, "ClearWaypoints");
#endif

		m_Waypoints.Clear();
		m_CurrentWaypointIndex = 0;
	}

	void SetWaypointBehaviour(eAIWaypointBehavior bhv)
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_0(this, "SetWaypointBehaviour");
#endif

		switch (bhv)
		{
			case eAIWaypointBehavior.HALT_OR_LOOP:
				if (Math.RandomIntInclusive(0, 1))
					bhv = eAIWaypointBehavior.HALT;
				else
					bhv = eAIWaypointBehavior.LOOP;
				break;
			case eAIWaypointBehavior.HALT_OR_ALTERNATE:
				if (Math.RandomIntInclusive(0, 1))
					bhv = eAIWaypointBehavior.HALT;
				else
					bhv = eAIWaypointBehavior.ALTERNATE;
				break;
		}

		m_WaypointBehaviour = bhv;
	}

	/**
	 * @brief Set waypoint behavior automatically based on proximity of 1st and last waypoint to one another
	 * 
	 * If 1st and last waypoints are close (within 1.5 m) to one another, waypoint behavior will be set to LOOP.
	 * If they are not close, it will be set to the passed in behavior (default ALTERNATE).
	 */
	void SetWaypointBehaviourAuto(eAIWaypointBehavior bhv = eAIWaypointBehavior.ALTERNATE)
	{
		int count = m_Waypoints.Count();

		if (count > 1 && vector.DistanceSq(m_Waypoints[0], m_Waypoints[count - 1]) <= 2.25)
			SetWaypointBehaviour(eAIWaypointBehavior.LOOP);
		else
			SetWaypointBehaviour(bhv);
	}

	eAIWaypointBehavior GetWaypointBehaviour()
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_0(this, "GetWaypointBehaviour");
#endif

		return m_WaypointBehaviour;
	}

	/**
	 * @brief Find roaming location that is closest to formation leader
	 */
	vector FindClosestRoamingLocationPosition()
	{
		if (Count() == 0)
			return vector.Zero;

		auto leader = GetFormationLeader();
		vector position = leader.GetPosition();

		//if (m_CurrentRoamingLocationPosition != vector.Zero)
		//{
			//if (vector.DistanceSq(position, m_CurrentRoamingLocationPosition) > 0.3)
				//return m_CurrentRoamingLocationPosition;
		//}

		auto settings = GetExpansionSettings().GetAI();

		float distSq;
		int distKey;
		TFloatArray distances = {};
		map<int, BuildingBase> buildingsByDistance;

		//! Excluded buildings are structures where the pathfinding won't generate a good path.
		//! TODO: Possibly move excludes to cover obj discovery in eAIBase::UpdateTargets?
		//!       Testing necessary. Is that even a problem for taking cover (since it's temporary) or just
		//!       for roaming (target position won't change unless reached)?
		//! @note classnames can be for groups of buildings (e.g. Land_House_1W09 has a yellow variant)
		TStringArray excludedBuildings = {
			"Land_CementWorks_Hall2_Grey",  //! Unsuitable path endpoint
			"Land_Factory_Small",  //! Unsuitable path endpoint
			"Land_House_1W09",  //! Path endpoint between opened door and wall leading to AI running in circles trying to reach point
			"Land_House_2W03",  //! Unsuitable path endpoint
			"Land_HouseBlock_1F4",  //! Path endpoint behind unopenable lattice door
			"Land_Boathouse",  //! When AI falls into water it will likely get stuck under pier due to path always leading under it
			"Land_Mine_Building",  //! Stairs
			"Land_Tenement_Big"  //! AI can get stuck on upper floors when climbing the broken stairs
		};

		excludedBuildings.InsertAll(settings.ExcludedRoamingBuildings);

		//! When looting buildings, stay at most 10 minutes (600 seconds) in the area before moving on to next map location
		float locationTime;
		if (m_RoamingLocationReachedTimestamp > 0.0)
			locationTime = GetGame().GetTickTime() - m_RoamingLocationReachedTimestamp;

		eAIBase ai;
		if (Class.CastTo(ai, leader) && ai.m_eAI_PotentialCoverObjects.Count() > 0 && locationTime < Math.RandomFloat(300.0, 600.0))
		{
			buildingsByDistance = new map<int, BuildingBase>;

			foreach (Object obj: ai.m_eAI_PotentialCoverObjects)
			{
				BuildingBase building;
				if (!Class.CastTo(building, obj) || building.GetDoorCount() == 0)
					continue;

				if (ExpansionStatic.IsAnyOf(obj, excludedBuildings))
					continue;

				if (m_VisitedBuildings.Find(building) > -1)
					continue;

				distSq = vector.DistanceSq(position, obj.GetPosition());

				distKey = distSq;
				distances.Insert(distKey);
				buildingsByDistance[distKey] = building;
			}
		}

		vector destinationPosition;

	#ifdef DIAG_DEVELOPER
		string destinationName;
	#endif

		if (distances.Count() > 0)
		{
			distances.Sort();

			BuildingBase destinationBuilding = buildingsByDistance[distances[0]];

			m_VisitedBuildings.Insert(destinationBuilding);

			destinationPosition = destinationBuilding.GetPosition();

			vector minMax[2];
			if (!destinationBuilding.Expansion_IsEnterable() && destinationBuilding.GetCollisionBox(minMax))
			{
				minMax[0][1] = 0.0;
				minMax[1][1] = 0.0;
				float extension = vector.Distance(minMax[0], minMax[1]) * 0.5 + 1.0;

				position[1] = destinationPosition[1];
				vector dir = vector.Direction(position, destinationPosition);
				destinationPosition = destinationPosition - dir.Normalized() * extension;
			}

			if (ai && ai.GetMovementSpeedLimit() >= 2)
				ai.SetMovementSpeedLimit(2, true);

			m_RoamingLocation = new ExpansionLocatorArray(destinationPosition, destinationBuilding.GetType(), "", "Building", destinationBuilding);

		#ifdef DIAG_DEVELOPER
			destinationName = destinationBuilding.GetType();
		#endif
		}
		else
		{
			map<int, ref ExpansionLocatorArray> locationsByDistance = new map<int, ref ExpansionLocatorArray>;

			foreach (ExpansionLocatorArray location: m_RoamingLocations)
			{
				if (location.position == vector.Zero)
					continue;  //! excluded

				//! @note we shave off a zero by taking the 10% value to make it less likely to run into 32-bit integer limits
				distSq = vector.DistanceSq(position, location.position) * 0.1;

				//if (location.type.Contains("ViewPoint") || location.type.Contains("Hill"))
					//distSq *= Math.RandomFloat(9.0, 10.0);  //! Make less likely to be picked by increasing distance by some large random factor
				//else
					distSq *= Math.RandomFloat(0.64, 1.44);  //! Bit of randomization (+/- 20%)

				distKey = distSq;
				distances.Insert(distKey);
				locationsByDistance[distKey] = location;
			}

			if (distances.Count() < 2)
				ExpansionArray<ExpansionLocatorArray>.RefCopy(ExpansionWorld.Cast(GetDayZGame().GetExpansionGame()).GetAIRoamingLocations(), m_RoamingLocations);

			for (int i = m_VisitedCrashSites.Count() - 1; i >= 0; i--)
			{
				if (!m_VisitedCrashSites[i])
					m_VisitedCrashSites.Remove(i);
			}

			CF_DoublyLinkedNode_WeakRef<CrashBase> node = CrashBase.s_Expansion_HeliCrashes.m_Head;
			while (node)
			{
				CrashBase crash = node.m_Value;

				node = node.m_Next;

				if (m_VisitedCrashSites.Find((Object)crash) > -1)
					continue;

				//! @note we shave off a zero by taking the 10% value to make it less likely to run into 32-bit integer limits
				distSq = vector.DistanceSq(position, crash.GetPosition()) * 0.1;
				distSq *= Math.RandomFloat(0.64, 1.44);  //! Bit of randomization (+/- 20%)

				distKey = distSq;
				distances.Insert(distKey);
				locationsByDistance[distKey] = new ExpansionLocatorArray(crash.GetPosition(), crash.GetType(), "", "StaticHeliCrash", crash);
			}

			if (distances.Count() == 0)
				return GetFormationLeader().GetPosition();

			distances.Sort();

			ExpansionLocatorArray destination = locationsByDistance[distances[0]];

			if (destination.type != "StaticHeliCrash")
			{
				float radius = ExpansionLocatorStatic.GetRadius(destination.type) * 0.25;
				destinationPosition = ExpansionMath.GetRandomPointInCircle(destination.position, radius);
			}
			else
			{
				m_VisitedCrashSites.Insert(destination.object);
				destinationPosition = ExpansionMath.GetRandomPointInRing(destination.position, 5.0, 20.0);
			}

			//! TODO: For map locations, use SceneGetEntitiesInBox to get closest building in radius (and add to visited buildings)
			destinationPosition = ExpansionStatic.GetSurfaceRoadPosition(destinationPosition[0], destinationPosition[2], RoadSurfaceDetection.CLOSEST);

			//! If next destination is > 500 m from formation leader position, "forget" visited buildings
			if (vector.DistanceSq(position, destinationPosition) > 250000.0)
				m_ClearVisitedBuildingsOnLocationReached = m_VisitedBuildings.Count();
			else
				m_ClearVisitedBuildingsOnLocationReached = 0;

			if (ai && ai.GetMovementSpeedLimit() >= 2)
				ai.SetMovementSpeedLimit(3, true);

			m_RoamingLocation = destination;

		#ifdef DIAG_DEVELOPER
			destinationName = destination.classname;
			if (destination.name)
				destinationName += " (" + destination.name + ")";
		#endif
		}

	#ifdef DIAG_DEVELOPER
		string msg = "Selected " + destinationName + " at " + ExpansionStatic.VectorToString(destinationPosition);
		EXTrace.Print(EXTrace.AI, leader, msg);
		ExpansionStatic.MessageNearPlayers(position, 100.0, leader.ToString() + " " + msg);
	#endif

		return destinationPosition;
	}

	void SetRoamingLocationReached(bool clearVisitedBuildings = false)
	{
		if (m_RoamingLocation && m_RoamingLocation.type != "Building")
		{
			bool clearVisitedBuildingsOnLocationReached;

			if (m_ClearVisitedBuildingsOnLocationReached && clearVisitedBuildings)
				clearVisitedBuildingsOnLocationReached = true;

			if (clearVisitedBuildingsOnLocationReached)
			{
				if (m_ClearVisitedBuildingsOnLocationReached >= m_VisitedBuildings.Count())
				{
					EXTrace.Print(EXTrace.AI, this, "Clearing " + m_VisitedBuildings.Count() + "/" + m_ClearVisitedBuildingsOnLocationReached + " visited buildings");
					m_VisitedBuildings.Clear();
				}
				else
				{
					EXTrace.Print(EXTrace.AI, this, "Clearing " + m_ClearVisitedBuildingsOnLocationReached + "/" + m_VisitedBuildings.Count() + " visited buildings");
					for (int i = m_ClearVisitedBuildingsOnLocationReached - 1; i >= 0; i--)
					{
						m_VisitedBuildings.Remove(i);
					}
				}
			}

			if (m_VisitedBuildings.Count() == 0 || clearVisitedBuildingsOnLocationReached)
				m_RoamingLocationReachedTimestamp = GetGame().GetTickTime();

			//! Remove destination from roaming locations if not a helicrash (helicrashes are events and not in roaming locations)
			if (m_RoamingLocation.type != "StaticHeliCrash")
				m_RoamingLocations.RemoveItemUnOrdered(m_RoamingLocation);
		}
	}

	void SetFormationState(eAIGroupFormationState state)
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_0(this, "SetFormationState");
#endif

		m_FormationState = state;
	}

	eAIGroupFormationState GetFormationState()
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_0(this, "GetFormationState");
#endif

		return m_FormationState;
	}

	void SetFaction(eAIFaction f)
	{
#ifdef EXTRACE_DIAG
		auto trace = EXTrace.StartStack(EXTrace.AI, this, "" + f);
#endif

		if (!f)
		{
			Error("Cannot set NULL faction");
			return;
		}

		int oldFactionTypeID = m_Faction.GetTypeID();
		int newFactionTypeID = f.GetTypeID();

		m_Faction = f;

		if (newFactionTypeID == oldFactionTypeID)
			return;

		foreach (auto member: m_Members)
		{
			if (member)
				member.eAI_SetFactionTypeID(newFactionTypeID);
		}
	}

	eAIFaction GetFaction()
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_0(this, "GetFaction");
#endif

		return m_Faction;
	}

	void SetName(string name)
	{
		m_Name = name;
	}

	string GetName()
	{
		return m_Name;
	}

	/**
	 * @brief The unique ID for this group
	 *
	 * @return int
	 */
	int GetID()
	{
		return m_ID;
	}

	/**
	 * @brief Add/update target for all group members
	 * 
	 * @param info Target information
	 * @param update If true (default) and member is already targeting the target, update found_at_time and max_time
	 * @param threat Initial threat level if non-zero
	 */
	void AddTarget(DayZPlayerImplement player, eAITargetInformation info, int max_time = -1, bool update = true, float threat = 0.0)
	{
#ifdef EXTRACE_DIAG
		auto trace = EXTrace.Start(EXTrace.AI, this, info.GetDebugName(), "" + max_time, "" + update, "" + threat);
#endif

		eAIBase ai;
		eAITarget target;
		foreach (DayZPlayerImplement member: m_Members)
		{
			if (Class.CastTo(ai, member))
			{
				target = info.AddAI(ai, max_time);

				if (update)
				{
					target.Update(max_time);
					update = false;  //! Since targets are per-group, we only need to do this once per loop
				}

				if (threat > 0.0)
				{
					auto state = ai.eAI_GetTargetInformationState(info, false);
					if (threat > state.m_ThreatLevelActive)
						state.SetInitial(threat, player.GetPosition());  //! We deliberately don't use attacker position but victim position
				}
			}
		}
	}

	/**
	 * @brief Internal event fired when this group needs to know that is now targetting something
	 *
	 * @param target The target being added
	 */
	void OnTargetAdded(eAITargetInformation target)
	{
#ifdef DIAG_DEVELOPER
		EXTrace.Print(EXTrace.AI, this, "OnTargetAdded " + target.GetDebugName());
#endif

		m_Targets.Insert(target);
	}

	/**
	 * @brief Return true if at least one member in group is targeting target
	 */
	bool IsTargeting(eAITargetInformation target)
	{
		if (m_Targets.Find(target) > -1)
			return true;

		return false;
	}

	/**
	 * @brief Internal event fired when this group needs to know that is no longer targetting something
	 *
	 * @param target The target being removed
	 */
	void OnTargetRemoved(eAITargetInformation target)
	{
#ifdef DIAG_DEVELOPER
		EXTrace.Print(EXTrace.AI, this, "OnTargetRemoved " + target.GetDebugName());
#endif

		m_Targets.RemoveItem(target);
	}

	/**
	 * @brief This target is both used by the owned AI's and enemy groups.
	 * The owned AI's will use this to get the position they should move to
	 * The enemy AI's will use this similar to a normal entity if they are
	 * targetting the group as a whole and not a singular AI. If they are
	 * targetting a singular AI then they would use GetTargetInformation.
	 *
	 * @return the target
	 */
	eAIGroupTargetInformation GetTargetInformation()
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_0(this, "GetTargetInformation");
#endif

		return m_TargetInformation;
	}

	//! DEPRECATED - group will never contain NULL or members that aren't alive
	int GetMemberIndex(eAIBase ai)
	{
		EXError.WarnOnce(this, "GetMemberIndex() is deprecated. Please use GetIndex().");

		return GetIndex(ai);
	}

	vector GetFormationPosition(eAIBase ai)
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_1(this, "GetFormationPosition").Add(ai);
#endif

		float time = GetGame().GetTickTime();
		bool isInitialUpdate = ai.m_eAI_FormationPositionUpdateTime == 0.0;
		if (time - ai.m_eAI_FormationPositionUpdateTime > Math.RandomFloat(2.0, 4.0) || isInitialUpdate)
		{
			int index = GetIndex(ai);
			if (index != -1)
			{
				auto leader = GetFormationLeader();
				if (index > 0 && leader != GetLeader())
					index--;
				vector position = m_Form.GetPosition(index);
				vector direction = m_Form.GetDirection(index);
				
				float extension;
				if (leader)
					extension = leader.Expansion_GetMovementSpeed() * 0.333333;
				if (extension > 0.0 || isInitialUpdate)
				{
					for (int i = 0; i < 3; i++)
					{
						position[i] = position[i] + m_Form.GetLooseness();
						if (i == 2)
							position[i] = position[i] + extension;  //! Compensate for head start of leader
					}
				}
				
				ai.m_eAI_FormationPosition = position;
				ai.m_eAI_FormationDirection = direction;
				ai.m_eAI_FormationPositionUpdateTime = time;
			}
		}
			
		return m_Form.ToWorld(ai.m_eAI_FormationPosition);
	}

	void SetLeader(DayZPlayerImplement leader)
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_1(this, "SetLeader").Add(leader);
#endif

		if (!IsMember(leader))
			AddMember(leader);

		DayZPlayerImplement temp = m_Members[0];
		if (temp == leader)
			return;

		m_Members[0] = leader;
		m_Members[0].SetGroupMemberIndex(0);

		for (int i = 1; i < Count(); i++)
		{
			if (m_Members[i] && m_Members[i] == leader)
			{
				m_Members[i] = temp;
				m_Members[i].SetGroupMemberIndex(i);
				return;
			}
		}
	}

	DayZPlayerImplement GetLeader()
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_0(this, "GetLeader");
#endif

		return m_Members[0];
	}

	eAIFormation GetFormation()
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_0(this, "GetFormation");
#endif

		return m_Form;
	}

	DayZPlayerImplement GetFormationLeader()
	{
		auto leader = GetLeader();

		if (leader && !leader.IsAI() && m_Members.Count() > 1 && m_Waypoints.Count() > 0)
			return m_Members[1];

		return leader;
	}

	void SetFormation(eAIFormation f)
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_0(this, "SetFormation");
#endif

		f.SetGroup(this);
		m_Form = f;
	}

	bool IsMember(DayZPlayerImplement member)
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_1(this, "IsMember").Add(member);
#endif

		return m_Members.Find(member) != -1;
	}

	int AddMember(DayZPlayerImplement member)
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_1(this, "AddMember").Add(member);
#endif

		int index = m_Members.Insert(member);

		Send_SetInGroup();

		return index;
	}

	void Client_SetMemberIndex(DayZPlayerImplement member, int index)
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_2(this, "Client_SetMemberIndex").Add(member).Add(index);
#endif

		if (index >= m_Members.Count())
		{
			m_Members.Resize(index + 1);
		}

		m_Members[index] = member;

		int removeFrom = m_Members.Count();

		for (int i = m_Members.Count() - 1; i > index; i--)
		{
			if (m_Members[i] != null)
				break;
			removeFrom = i;
		}

		m_Members.Resize(removeFrom);
	}

	bool RemoveMember(DayZPlayerImplement member, bool autoDelete = true)
	{
#ifdef EXTRACE_DIAG
		auto trace = EXTrace.Start(EXTrace.AI, this, "" + member, "" + autoDelete);
#endif

		return RemoveMember(m_Members.Find(member), autoDelete);
	}

	bool RemoveMember(int i, bool autoDelete = true)
	{
#ifdef EXTRACE_DIAG
		auto trace = EXTrace.Start(EXTrace.AI, this, "" + i, "" + autoDelete);
#endif

		if (i < 0 || i >= m_Members.Count())
			return false;

		auto member = m_Members[i];

		m_Members.RemoveOrdered(i);

		if (autoDelete && m_Members.Count() == 0)
		{
			Delete();
		}
		else
		{
			Send_SetInGroup();

			if (member && !member.IsAlive())
			{
				m_Members_Deceased.Insert(member);

				if (m_Persist && m_BaseName && m_Members.Count() == 0)
					DeletePersistentFiles();
			}
		}

		return true;
	}

	bool RemoveDeceased(DayZPlayerImplement member)
	{
#ifdef EXTRACE_DIAG
		auto trace = EXTrace.Start(EXTrace.AI, this, "" + member);
#endif

		return RemoveDeceased(m_Members_Deceased.Find(member));
	}

	bool RemoveDeceased(int i)
	{
#ifdef EXTRACE_DIAG
		auto trace = EXTrace.Start(EXTrace.AI, this, "" + i);
#endif

		if (i < 0 || i >= m_Members_Deceased.Count())
			return false;

		m_Members_Deceased.RemoveOrdered(i);

		return true;
	}

	/**
	 * @brief send whether leader is in group with more than one other member to client
	 * 
	 * @note This is necessary because on client, group can only contain the members in network bubble,
	 *       so the count does not reflect the value on server.
	 */
	void Send_SetInGroup()
	{
	#ifdef SERVER
	#ifdef EXTRACE_DIAG
		auto trace = EXTrace.Start(EXTrace.AI, this);
	#endif

		if (m_Members.Count() <= 2 && m_Members.Count() != m_MemberCount)
		{
			m_MemberCount = m_Members.Count();

			auto leader = GetLeader();
			if (leader && leader.GetIdentity())
			{
				auto rpc = GetDayZGame().GetExpansionGame().GetRPCManager().CreateRPC("RPC_SetInGroup");
				if (m_MemberCount > 1)
					rpc.Write(true);
				else
					rpc.Write(false);
				rpc.Expansion_Send(true, leader.GetIdentity());
			}
		}
	#else
		if (!GetGame().IsMultiplayer())
		{
			if (m_Members.Count() > 1)
				GetDayZGame().GetExpansionGame().SetInGroup(true);
			else
				GetDayZGame().GetExpansionGame().SetInGroup(false);
		}
	#endif
	}

	void SetIsInCombat(bool state)
	{
		m_IsInCombat = state;
	}

	bool IsInCombat()
	{
		return m_IsInCombat;
	}

	DayZPlayerImplement GetMember(int i)
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_1(this, "GetMember").Add(i);
#endif

		return m_Members[i];
	}

	int GetIndex(DayZPlayerImplement player)
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_1(this, "GetIndex").Add(player);
#endif

		return m_Members.Find(player);
	}

	/**
	 * @brief Return random member excluding player. If group size is 1, return null.
	 * 
	 * @param player Player to exclude
	 */
	DayZPlayerImplement GetRandomMemberExcluding(DayZPlayerImplement player)
	{
		int count = Count();

		if (count == 1)
			return null;

		int index = Math.RandomInt(0, count);
		DayZPlayerImplement member = m_Members[index];

		if (member == player)
		{
			if (index + 1 == count)
				index--;
			else
				index++;

			member = m_Members[index];
		}

		return member;
	}

	int Count()
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_0(this, "Count");
#endif

		return m_Members.Count();
	}

	int DeceasedCount()
	{
		return m_Members_Deceased.Count();
	}

	//! DEPRECATED - group will never contain NULL or members that aren't alive
	int GetAliveCount()
	{
		EXError.WarnOnce(this, "GetAliveCount() is deprecated. Please use Count().");

		return Count();
	}

	//! Clears ALL AI from the server
	static void Admin_ClearAllAI()
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_0("eAIGroup", "Admin_ClearAllAI");
#endif

		foreach (eAIGroup group : s_AllGroups)
		{
			group.ClearAI();
		}
	}

	void ClearAI(bool autodelete = true, bool deferDespawnUntilLoosingAggro = false)
	{
	#ifdef EXTRACE_DIAG
		auto trace = EXTrace.Start(EXTrace.AI, this);
	#endif

		int i;
		eAIBase ai;

		for (i = Count() - 1; i > -1; i--)
		{
			if (!Class.CastTo(ai, GetMember(i)) || ai.IsInherited(eAINPCBase))
				continue;

			if (deferDespawnUntilLoosingAggro && ai.GetThreatToSelf() >= 0.4)
				ai.eAI_SetDespawnOnLoosingAggro(true);
			else
				ai.eAI_Despawn();
		}

		for (i = m_Members_Deceased.Count() - 1; i > -1; i--)
		{
			if (!Class.CastTo(ai, m_Members_Deceased[i]))
				continue;

			ai.eAI_Despawn();
		}
	}

	/**
	 * @brief Enable swimming for the whole group so they can follow the leader
	 */
	void EnableSwimming(bool enable = true)
	{
		eAIBase ai;
		foreach (DayZPlayerImplement member: m_Members)
		{
			if (Class.CastTo(ai, member))
			{
				if (enable || !ai.IsSwimming())
					ai.GetPathFinding().EnableSwimming(enable);
			}
		}
	}

	bool IsFormationLeaderSwimming()
	{
		DayZPlayerImplement leader = GetFormationLeader();
		if (leader)
			return leader.IsSwimming();

		return false;
	}

	TStringArray DumpState(PlayerIdentity sender, bool includeMembers = true, string indent = string.Empty)
	{
		TStringArray report = {};

		report.Insert(indent + string.Format("Group ID %1 %2", m_ID, m_Name));

		if (indent)
			indent.Replace("-", " ");

		report.Insert(indent + string.Format("|- Faction %1", m_Faction.GetName()));

		string form = m_Form.ClassName().Substring(12, m_Form.ClassName().Length() - 12);
		report.Insert(indent + string.Format("|- Formation %1 %2", form, typename.EnumToString(eAIGroupFormationState, m_FormationState)));

		string lineSegment;
		if (includeMembers)
			lineSegment = "|";
		else
			lineSegment = "\\";

		report.Insert(indent + string.Format(lineSegment + "- Waypoints %1 %2", m_Waypoints.Count(), typename.EnumToString(eAIWaypointBehavior, m_WaypointBehaviour)));

		if (includeMembers)
		{
			eAIBase ai;
			string subIndent = "|  ";
			lineSegment = "|";
			foreach (int i, DayZPlayerImplement member: m_Members)
			{
				if (Class.CastTo(ai, member))
				{
					if (i == Count() - 1)
					{
						subIndent = ".  ";
						lineSegment = "\\";
					}

					report.Insert(indent + string.Format(lineSegment + "- AI group member %1", i));
					report.InsertAll(ai.eAI_DumpState(sender, false, indent + subIndent + lineSegment + "- "));
				}
			}
		}

		return report;
	}

    string GetStorageDirectory()
    {
		return GetStorageDirectory(m_BaseName);
    }

	static string GetStorageDirectory(string baseName)
	{
		return "$mission:storage_" + GetDayZGame().GetExpansionGame().m_InstanceId + "\\expansion\\ai\\" + baseName + "\\";
	}

	void Save(bool saveAI = false)
	{
#ifdef EXTRACE
		auto trace = EXTrace.StartStack(EXTrace.AI, eAIGroup, m_BaseName, m_Name);
#endif 
		
		if (!m_BaseName)
		{
			EXError.Error(this, "No basename set");
			return;
		}

		string basePath = GetStorageDirectory(m_BaseName);
		if (!FileExist(basePath) && !ExpansionStatic.MakeDirectoryRecursive(basePath))
			return;

		string fileName = basePath + eAIGroup.BASENAME;
		FileSerializer serializer = new FileSerializer();
		if (serializer.Open(fileName, FileMode.WRITE))
		{
			serializer.Write(VERSION);

			DayZPlayerImplement leader = GetFormationLeader();
			if (leader)
				serializer.Write(leader.GetPosition());
			else
				serializer.Write(vector.Zero);

		#ifdef DIAG_DEVELOPER
			EXTrace.Print(EXTrace.AI, eAIGroup, "Save - writing faction " + GetFaction() + " ID " + GetFaction().GetTypeID());
		#endif

			serializer.Write(GetFaction().GetTypeID());
			serializer.Write(m_Name);

			serializer.Write(Count());

			eAIBase ai;
			string aiPath;
			foreach (DayZPlayerImplement member: m_Members)
			{
				if (Class.CastTo(ai, member))
				{
					serializer.Write(member.m_eAI_GroupMemberID);

					if (saveAI)
					{
						aiPath = basePath + member.m_eAI_GroupMemberID.ToString() + ".bin";
						ExpansionEntityStorageModule.SaveToFile(ai, aiPath);
					}
				}
			}

			serializer.Write(m_RoamingLocations.Count());
			foreach (ExpansionLocatorArray location: m_RoamingLocations)
			{
				serializer.Write(location.index);
			}

			serializer.Close();
		}
		else
		{
			EXError.Error(this, "Couldn't open " + fileName + " for writing");
		}
	}

	static void SaveAllPersistentGroups()
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI, eAIGroup);
#endif 
		
		foreach (eAIGroup group: s_PersistentGroups)
		{
			if (group.Count() > 0)
				group.Save(true);
		}
	}

	static bool Load(string fileName, out eAIGroup group)
	{
#ifdef EXTRACE
		auto trace = EXTrace.StartStack(EXTrace.AI, eAIGroup);
#endif 
		
		FileSerializer serializer = new FileSerializer();
		if (serializer.Open(fileName, FileMode.READ))
		{
			int version;
			if (!serializer.Read(version))
				return EXError.ErrorFalse(null, "Couldn't read version from " + fileName);

			vector position;
			if (!serializer.Read(position))
				return EXError.ErrorFalse(null, "Couldn't read group leader position from " + fileName);

			int factionTypeID;
			if (!serializer.Read(factionTypeID))
				return EXError.ErrorFalse(null, "Couldn't read factionTypeID from " + fileName);

			eAIFaction faction = eAIFaction.CreateByID(factionTypeID);

		#ifdef DIAG_DEVELOPER
			EXTrace.Print(EXTrace.AI, eAIGroup, "Load - Read faction " + faction + " ID " + factionTypeID);
		#endif

			group = CreateGroup(faction);

			string name;
			if (!serializer.Read(name))
				return EXError.ErrorFalse(null, "Couldn't read group name from " + fileName);
			else
				group.SetName(name);
	
			int count;
			if (!serializer.Read(count))
				return EXError.ErrorFalse(null, "Couldn't read count from " + fileName);

			int groupMemberID;
			string basePath = ExpansionString.DirName(fileName) + "\\";
			string aiPath;
			EntityAI entity;
			eAIBase ai;
			while (count--)
			{
				if (!serializer.Read(groupMemberID))
					return EXError.ErrorFalse(null, "Couldn't read groupMemberID from " + fileName);

				if (groupMemberID >= group.m_NextGroupMemberID)
					group.m_NextGroupMemberID = groupMemberID + 1;

				aiPath = basePath + groupMemberID.ToString() + ".bin";
				entity = null;
				if (ExpansionEntityStorageModule.RestoreFromFile(aiPath, entity, null, null, false) && Class.CastTo(ai, entity))
				{
					ai.SetGroup(group);
					ai.m_eAI_GroupMemberID = groupMemberID;
				#ifdef DIAG_DEVELOPER
					EXTrace.Print(EXTrace.AI, eAIGroup, "Restored AI " + ai + " at position " + ai.GetPosition());
					vector surface = ExpansionStatic.GetSurfacePosition(ai.GetPosition());
					if (ai.GetPosition()[1] < surface[1])
						ai.SetPosition(surface);
				#endif
				}
			}

			int roamingCount;
			if (!serializer.Read(roamingCount))
			{
				EXError.Error(null, "Couldn't read roamingCount from " + fileName);
			}
			else if (roamingCount > 0)
			{
				TIntArray locIndices = {};

				int index;
				while (roamingCount--)
				{
					if (!serializer.Read(index))
					{
						EXError.Error(null, "Couldn't read location index from " + fileName);
						break;
					}

					locIndices.Insert(index);
				}

				array<ref ExpansionLocatorArray> locations = ExpansionWorld.Cast(GetDayZGame().GetExpansionGame()).GetAIRoamingLocations();

				foreach (int locIdx: locIndices)
				{
					ExpansionLocatorArray location = locations[locIdx];
					if (location)
						group.m_RoamingLocations.Insert(location);
				}
			}

			serializer.Close();

			return true;
		}
		else
		{
			EXError.Error(null, "Couldn't open " + fileName + " for reading");
		}

		return false;
	}

	static bool ReadPosition(string fileName, out vector position)
	{
#ifdef EXTRACE
		auto trace = EXTrace.StartStack(EXTrace.AI, eAIGroup);
#endif 
		
		FileSerializer serializer = new FileSerializer();
		if (serializer.Open(fileName, FileMode.READ))
		{
			int version;
			if (!serializer.Read(version))
				return EXError.ErrorFalse(null, "Couldn't read version from " + fileName);

			if (!serializer.Read(position))
				return EXError.ErrorFalse(null, "Couldn't read position from " + fileName);

			serializer.Close();

#ifdef EXTRACE
			EXTrace.Add(trace, position);
#endif 
			
			return true;
		}
		else
		{
			EXError.Error(null, "Couldn't open " + fileName + " for reading");
		}

		return false;
	}

	void DeletePersistentFiles()
	{
		string path = eAIGroup.GetStorageDirectory(m_BaseName);
		ExpansionStatic.DeleteDirectoryStructureRecursive(path, ".bin");
	}

	void RandomizeDangerousAreaAvoidanceDirection()
	{
		if (Math.RandomInt(0, 2))
			m_DangerousAreaAvoidanceDirection = 1.0;
		else
			m_DangerousAreaAvoidanceDirection = -1.0;
	}
};
