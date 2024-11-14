/**
 * @class eAITargetInformation
 *
 * @brief Is an abstract class
 */
class eAITargetInformation
{
	// in most circumstances an entity should only be in 1 group
	autoptr map<int, ref eAITarget> m_Groups;  //! Contains all groups actively tracking this target

	ref set<eAIBase> m_AI = new set<eAIBase>;  //! Contains all AI that know about this target (not necessarily actively tracking it)

	float m_MinDistance;

	void eAITargetInformation()
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_0(this, "eAITargetInformation");
#endif

		m_Groups = new map<int, ref eAITarget>();
	}

	void ~eAITargetInformation()
	{
		if (!GetGame())
			return;

#ifdef EXTRACE_DIAG
		auto trace = EXTrace.Start(EXTrace.AI, this);
#endif

		RemoveFromAll();
	}
	
	void DeferredInit();

	override string GetDebugName()
	{
		return ToString();
	}

	/**
	 * @brief Get the entity that this target belongs to, if it does so
	 *
	 * @return EntityAI
	 */
	EntityAI GetEntity()
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_0(this, "GetEntity");
#endif

		return null;
	}

	string GetEntityDebugName()
	{
		return "";
	}

	bool CanPutInCargo(EntityAI parent)
	{
		return false;
	}

	bool IsEntity()
	{
		return false;
	}

	bool IsExplosive()
	{
		return false;
	}

	bool IsMechanicalTrap()
	{
		return false;
	}

	bool IsInanimate()
	{
		return false;
	}

	/**
	 * @brief Abstract function. If the target is active or not.
	 *
	 * @return bool
	 */
	bool IsActive()
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_0(this, "IsActive");
#endif

		return true;
	}

	bool IsCrawling()
	{
		return false;
	}

	/**
	 * @brief Abstract function. Get the position for the AI within the target. Each AI could have their own position for the target.
	 *
	 * @param ai null default, gets the position for the AI if specified, otherwise returns a default value
	 * @return EntityAI
	 */
	vector GetPosition(eAIBase ai = null, bool actual = false)
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_1(this, "GetPosition").Add(ai);
#endif

		return "0 0 0";
	}

	/**
	 * @brief Abstract function. Get the aim offset for the AI within the target. Each AI could have their own offset for the target.
	 *
	 * @param ai null default, gets the aim offset for the AI if specified, otherwise returns a default value
	 * @return EntityAI
	 */
	vector GetAimOffset(eAIBase ai = null)
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_1(this, "GetAimOffset").Add(ai);
#endif

		return "0 0 0";
	}

	/**
	 * @brief Abstract function. Get the threat level for the AI within the target. Each AI could have their own threat level for the target.
	 *
	 * @param ai null default, if given includes the AI in threat calculation
	 * @return int
	 */
	float GetThreat(eAIBase ai = null, out eAITargetInformationState state = null, out bool created = false)
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_1(this, "GetThreat").Add(ai);
#endif

		if (!ai)
			return CalculateThreat();

		state = ai.eAI_GetTargetInformationState(this, true, created);

		if (created)
			m_AI.Insert(ai);

		state.UpdateThreat();

		return state.m_ThreatLevel;
	}

	float CalculateThreat(eAIBase ai = null)
	{
		return 0.0;
	}

	bool ShouldRemove(eAIBase ai = null)
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_1(this, "ShouldRemove").Add(ai);
#endif

		return 0;
	}

	bool ShouldAvoid(eAIBase ai = null, float distance = 0.0)
	{
		return false;
	}

	float GetMinDistance(eAIBase ai = null, float distance = 0.0)
	{
		return m_MinDistance;
	}

	float GetMinDistanceSq(eAIBase ai = null, float distance = 0.0)
	{
		float minDist = GetMinDistance(ai, distance);
		return minDist * minDist;
	}

	vector GetDirection(eAIBase ai, bool actual = false)
	{
		return vector.Zero;
	}

	/**
	 * @brief Abstract function. Get the distance to the target.
	 *
	 * @param ai
	 * @return float
	 */
	float GetDistance(eAIBase ai, bool actual = false)
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_1(this, "GetDistance").Add(ai);
#endif

		return 0;
	}

	float GetDistanceSq(eAIBase ai, bool actual = false)
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_1(this, "GetDistanceSq").Add(ai);
#endif

		return 0;
	}

	/**
	 * @brief Refreshes the target time for the group
	 *
	 * @param group the eAIGroup
	 * @param max_time the new maximum time it should be targetting for, if -1 then not updated
	 */
	void Update(eAIGroup group, int max_time = -1)
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_2(this, "Update").Add(group).Add(max_time);
#endif

		eAITarget target;
		if (!m_Groups.Find(group.GetID(), target))
			return;

		target.found_at_time = GetGame().GetTime();
		if (max_time != -1)
			target.max_time = max_time;
	}

	/**
	 * @brief Inserts the group into the target
	 *
	 * @param group_id group id of an eAIGroup
	 */
	eAITarget Insert(notnull eAIGroup group, int max_time = -1)
	{
#ifdef EXTRACE_DIAG
		auto trace = EXTrace.Start(EXTrace.AI, this, "" + group, "" + max_time);
#endif

		eAITarget target;
		target = new eAITarget(group, GetGame().GetTime(), max_time, this);
		if (!m_Groups.Insert(group.GetID(), target))
			return null;

		group.OnTargetAdded(this);

		return target;
	}

	/**
	 * @brief Tells the target that the AI is targeting it, inserting the group
	 *
	 * @param ai eAIBase object
	 * @param max_time time the eAIBase will be targetting this target for
	 */
	eAITarget Insert(eAIBase ai, int max_time = -1)
	{
#ifdef EXTRACE_DIAG
		auto trace = EXTrace.Start(EXTrace.AI, this, "" + ai, "" + max_time);
#endif

		eAITarget target;
		target = Insert(ai.GetGroup(), max_time);
		if (!target)
			return null;
		if (target.AddAI(ai))
			ai.OnAddTarget(target);
		return target;
	}

	/**
	 * @brief Tells the target that the AI is targetting it. If the group didn't know about the target, insert the group
	 *
	 * @param ai eAIBase object
	 * @param max_time time the eAIBase will be targetting this target for, if -1 then the time of the group isn't updated
	 */
	eAITarget AddAI(eAIBase ai, int max_time = -1)
	{
		eAITarget target;
		int group_id = ai.GetGroup().GetID();
		if (!m_Groups.Find(group_id, target))
			return Insert(ai, max_time);

		if (max_time != -1)
			target.max_time = max_time;

		if (target.AddAI(ai))
			ai.OnAddTarget(target);

		return target;
	}

	/**
	 * @brief Add/update target for all AI friendly to player
	 * 
	 * @param player
	 * @param update If true (default) and AI is already targeting the target, update found_at_time and max_time
	 * @param threat Initial threat level if non-zero
	 */
	void AddFriendlyAI(DayZPlayerImplement player, int max_time = -1, bool update = true, float threat = 0.0)
	{
#ifdef EXTRACE_DIAG
		auto trace = EXTrace.Start(EXTrace.AI, this, "" + player, "" + max_time, "" + update, "" + threat);
#endif

		eAIBase ai;
		if (Class.CastTo(ai, player))
			m_AI.Insert(ai);

		eAIGroup group = player.GetGroup();
		eAIFaction faction = group.GetFaction();
		eAIGroup otherGroup;
		eAITarget target;

		foreach (eAIBase other: m_AI)
		{
		#ifdef DIAG_DEVELOPER
			EXTrace.Print(EXTrace.AI, this, "" + other);
		#endif

			if (!other)
				continue;

			if (other != player)
			{
				otherGroup = other.GetGroup();

				if (otherGroup != group && !otherGroup.GetFaction().IsFriendly(faction))
					continue;
			}

			target = AddAI(other, max_time);

			if (update)
				target.Update(max_time);

			if (threat > 0.0)
			{
				auto state = other.eAI_GetTargetInformationState(this, false);
				if (threat > state.m_ThreatLevelActive)
					state.SetInitial(threat, player.GetPosition());  //! We deliberately don't use attacker position but victim position
			}
		}
	}

	/**
	 * @brief Tells the target that the AI is no longer targetting it
	 *
	 * @param ai eAIBase object
	 */
	bool RemoveAI(eAIBase ai)
	{
#ifdef EXTRACE_DIAG
		auto trace = EXTrace.Start(EXTrace.AI, this, "" + ai);
#endif

		m_AI.RemoveItem(ai);

		eAITarget target;
		int group_id = ai.GetGroup().GetID();
		if (!m_Groups.Find(group_id, target))
			return false;

		if (!target.RemoveAI(ai))
			return false;

		return true;
	}

	/**
	 * @brief Tells the target that the group and it's AI is no longer targetting it
	 *
	 * @param group_id the ID of the eAIGroup
	 */
	void Remove(int group_id)
	{
#ifdef EXTRACE_DIAG
		auto trace = EXTrace.Start(EXTrace.AI, this, "" + group_id);
#endif

		eAITarget target;
		if (!m_Groups.Find(group_id, target))
			return;

		if (target.group)
		{
		#ifdef EXTRACE_DIAG
			EXTrace.Add(trace, "leader " + target.group.GetLeader());
		#endif

			foreach (eAIBase ai : target.ai_list)
			{
				if (!ai)
					continue;
				
				ai.OnRemoveTarget(target);

				m_AI.RemoveItem(ai);
			}

			target.group.OnTargetRemoved(this);
		}

		m_Groups.Remove(group_id);
	}

	/**
	 * @brief Tells the target that the group and it's AI is no longer targetting it
	 *
	 * @param group the eAIGroup
	 */
	void Remove(eAIGroup group)
	{
		int group_id = group.GetID();

		Remove(group_id);
	}

	void RemoveFromAll()
	{
#ifdef EXTRACE_DIAG
		auto trace = EXTrace.Start(EXTrace.AI, this, GetEntityDebugName());
#endif

		foreach (int id, eAITarget target: m_Groups)
		{
			Remove(id);
		}

#ifdef DIAG_DEVELOPER
		if (m_AI.Count())
			EXTrace.Print(EXTrace.AI, this, "Removing AI that are not targeting");
#endif
		foreach (eAIBase ai: m_AI)
		{
			if (!ai)
				continue;

			ai.eAI_RemoveTargetInfoState(this);
		}
	}

	/**
	 * @brief Checks to see if any group/ai is currently targetting this target
	 *
	 * @return bool true if being targetted, false otherwise
	 */
	bool IsTargetted()
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_0(this, "IsTargetted");
#endif

		return m_Groups.Count() > 0;
	}

	/**
	 * @brief Checks to see if the group specified is currently targetting this target
	 *
	 * @param group_id the group id of the eAIGroup
	 * @return bool true if being targetted, false otherwise
	 */
	bool IsTargetted(int group_id)
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_1(this, "IsTargetted").Add(group_id);
#endif

		return m_Groups.Contains(group_id);
	}

	/**
	 * @brief Checks to see if the group specified is currently targetting this target
	 *
	 * @param group the eAIGroup
	 * @return bool true if being targetted, false otherwise
	 */
	bool IsTargetted(notnull eAIGroup group)
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_1(this, "IsTargetted").Add(group);
#endif

		return m_Groups.Contains(group.GetID());
	}

	/**
	 * @brief Checks to see if the group specified is currently targetting this target
	 *
	 * @param group_id the group id of the eAIGroup
	 * @return bool true if being targetted, false otherwise
	 */
	bool IsTargetted(int group_id, out int num_ai)
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_1(this, "IsTargetted").Add(group_id);
#endif

		eAITarget target;
		if (!m_Groups.Find(group_id, target))
			return false;

		num_ai = target.ai_list.Count();

		return true;
	}

	/**
	 * @brief Checks to see if the group specified is currently targetting this target
	 *
	 * @param group the eAIGroup
	 * @return bool true if being targetted, false otherwise
	 */
	bool IsTargetted(notnull eAIGroup group, out int num_ai)
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_1(this, "IsTargetted").Add(group);
#endif

		eAITarget target;
		if (!m_Groups.Find(group.GetID(), target))
			return false;

		num_ai = target.ai_list.Count();

		return true;
	}

	/**
	 * @brief Checks to see if the AI specified is currently targetting this target
	 *
	 * @param group the eAIGroup
	 * @return bool true if being targetted, false otherwise
	 */
	bool IsTargettedBy(eAIBase ai)
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_1(this, "IsTargettedBy").Add(ai);
#endif

		eAITarget target;
		if (!m_Groups.Find(ai.GetGroup().GetID(), target))
			return false;

		return target.FindAI(ai) != -1;
	}

	eAITarget GetTarget(notnull eAIGroup group)
	{
		return m_Groups[group.GetID()];
	}

	//! entity specific implementations for abstracted call in eAIEntityTargetInformation
	void OnDeath(Object killer)
	{
#ifdef EXTRACE_DIAG
		auto trace = EXTrace.Start(EXTrace.AI, this);
#endif

		RemoveFromAll();
	}

	void OnHit(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
	}

	void OnHealthLevelChanged(int oldLevel, int newLevel, string zone)
	{
	}

	// this is really bad but unfortunately we can't reasonably mod the Object class
	// and for some circumstances using templates just won't work.
	//
	// wherever possible, please use
	//	'eAIEntity<DayZPlayerImplement>.GetTargetInformation(GetGame().GetPlayer())`
	static eAITargetInformation GetTargetInformation(Object entity)
	{
		DayZPlayerImplement player;
		if (Class.CastTo(player, entity))
			return player.GetTargetInformation();

		ZombieBase zombie;
		if (Class.CastTo(zombie, entity))
			return zombie.GetTargetInformation();

		AnimalBase animal;
		if (Class.CastTo(animal, entity))
			return animal.GetTargetInformation();

		ItemBase item;
		if (Class.CastTo(item, entity))
			return item.GetTargetInformation();

		CarScript car;
		if (Class.CastTo(car, entity))
			return car.GetTargetInformation();

		return null;
	}
};
