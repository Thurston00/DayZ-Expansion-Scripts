class eAIDynamicPatrol : eAIPatrol
{
	static ExpansionAIPatrolSettings s_AIPatrolSettings;
	private static int s_NumberOfDynamicPatrols;

	ref ExpansionAIDynamicSpawnBase m_Config;
	vector m_Position;
	autoptr array<vector> m_Waypoints;
	eAIWaypointBehavior m_WaypointBehaviour;
	float m_MinimumRadius;
	float m_MaximumRadius;
	float m_DespawnRadius;
	float m_MovementSpeedLimit;
	float m_MovementThreatSpeedLimit;
	ref ExpansionArray<string> m_Units;
	int m_NumberOfAI;
	int m_RespawnTime; // negative respawn time = patrol won't respawn
	int m_DespawnTime; // if all players outside despawn radius, ticks up time. When despawn time reached, patrol is deleted
	ref eAIFaction m_Faction;
	ref eAIFormation m_Formation;
	float m_AccuracyMin; // zero or negative = use general setting
	float m_AccuracyMax; // zero or negative = use general setting
	float m_ThreatDistanceLimit; // zero or negative = use general setting
	float m_NoiseInvestigationDistanceLimit; // zero or negative = use general setting
	float m_DamageMultiplier; // zero or negative = use general setting
	float m_DamageReceivedMultiplier; // zero or negative = use general setting

	eAIGroup m_Group;
	float m_TimeSinceLastSpawn;
	bool m_CanSpawn;
	private bool m_IsSpawned;
	private bool m_WasGroupDestroyed;
	private eAIDynamicPatrolSphereTrigger m_Trigger;
	private string m_NameForLog;

	void ~eAIDynamicPatrol()
	{
		if (GetGame() && m_Trigger)
			GetGame().ObjectDelete(m_Trigger);
	}

	/**
	 * @brief Creates a dynamic patrol which spawns a patrol under the right conditions.
	 * 
	 * @param config
	 * @param startpos the position that the trigger distance is calculated from. If zero vector, waypoint from config is used
	 * @param autoStart
	 * 
	 * @return the patrol instance
	 */
	static eAIDynamicPatrol CreateEx(ExpansionAIDynamicSpawnBase config, vector startpos, bool autoStart = true)
	{
		return eAIDynamicPatrolT<eAIDynamicPatrol>.CreateEx(config, startpos, autoStart);
	}

	bool Setup(ExpansionAIDynamicSpawnBase config, vector startpos, bool autoStart = true)
	{
		InitSettings();

		m_Config = config;

		if (config.NumberOfAI == 0)
		{
			Log("WARNING: NumberOfAI shouldn't be set to 0, skipping this patrol...");
			return false;
		}

		m_Waypoints = config.GetWaypoints(startpos);

		if (config.Persist && config.m_BaseName)
		{
			string fileName = eAIGroup.GetStorageDirectory(config.m_BaseName) + eAIGroup.BASENAME;
			if (FileExist(fileName))
				eAIGroup.ReadPosition(fileName, startpos);
			else
				startpos = GetInitialSpawnPosition();
		}
		else
		{
			startpos = GetInitialSpawnPosition();
		}

		if (startpos == vector.Zero)
		{
			Log("!!! ERROR !!!");
			Log("Couldn't find a spawn location. First waypoint is set to 0 0 0 or could not be read by the system (validate your file with a json validator)");
			Log("!!! ERROR !!!");
			return false;
		}

		m_Position = startpos;

		if (config.RespawnTime == -2)
			m_RespawnTime = s_AIPatrolSettings.RespawnTime;
		else
			m_RespawnTime = config.RespawnTime;

		if (config.DespawnTime < 0)
			m_DespawnTime = s_AIPatrolSettings.DespawnTime;
		else
			m_DespawnTime = config.DespawnTime;

		if (config.MinDistRadius <= 0)
			m_MinimumRadius = s_AIPatrolSettings.MinDistRadius;
		else
			m_MinimumRadius = config.MinDistRadius;

		if (config.MaxDistRadius <= 0)
			m_MaximumRadius = s_AIPatrolSettings.MaxDistRadius;
		else
			m_MaximumRadius = config.MaxDistRadius;

		if (config.DespawnRadius <= 0)
			m_DespawnRadius = s_AIPatrolSettings.DespawnRadius;
		else
			m_DespawnRadius = config.DespawnRadius;
		
		if (m_MinimumRadius > m_MaximumRadius)
		{
			Log("!!! ERROR !!!");
			Log("MinDistRadius (" + m_MinimumRadius + ") should be smaller than MaxDistRadius (" + m_MaximumRadius + ")");
			Log("!!! ERROR !!!");
			float actualMax = m_MinimumRadius;
			m_MinimumRadius = m_MaximumRadius;
			m_MaximumRadius = actualMax;
		}

		float accuracyMin;
		if (config.AccuracyMin <= 0)
			accuracyMin = s_AIPatrolSettings.AccuracyMin;
		else
			accuracyMin = config.AccuracyMin;

		float accuracyMax;
		if (config.AccuracyMin <= 0)
			accuracyMax = s_AIPatrolSettings.AccuracyMax;
		else
			accuracyMax = config.AccuracyMax;

		float threatDistanceLimit;
		if (config.ThreatDistanceLimit <= 0)
			threatDistanceLimit = s_AIPatrolSettings.ThreatDistanceLimit;
		else
			threatDistanceLimit = config.ThreatDistanceLimit;

		float noiseDistanceLimit;
		if (config.NoiseInvestigationDistanceLimit <= 0)
			noiseDistanceLimit = s_AIPatrolSettings.NoiseInvestigationDistanceLimit;
		else
			noiseDistanceLimit = config.NoiseInvestigationDistanceLimit;

		float damageMultiplier;
		if (config.DamageMultiplier <= 0)
			damageMultiplier = s_AIPatrolSettings.DamageMultiplier;
		else
			damageMultiplier = config.DamageMultiplier;

		float damageReceivedMultiplier;
		if ( config.DamageReceivedMultiplier <= 0 )
			damageReceivedMultiplier = s_AIPatrolSettings.DamageReceivedMultiplier;
		else
			damageReceivedMultiplier = config.DamageReceivedMultiplier;

		SetAccuracy(accuracyMin, accuracyMax);
		SetThreatDistanceLimit(threatDistanceLimit);
		SetNoiseInvestigationDistanceLimit(noiseDistanceLimit);
		SetDamageMultiplier(damageMultiplier);
		SetDamageReceivedMultiplier(damageReceivedMultiplier);

		if (config.Units && config.Units.Count())
			SetUnits(config.Units);

		m_CanSpawn = true;

		if (autoStart) Start();

		return true;
	}

	vector GetInitialSpawnPosition()
	{
		int startPosIndex;
		ExpansionAIPatrol patrolWithWaypoints;
		//! For object patrols, we always use random waypoint as startpoint, for patrols with fixed waypoints, only if random is set
		if (!Class.CastTo(patrolWithWaypoints, m_Config) || patrolWithWaypoints.UseRandomWaypointAsStartPoint)
			startPosIndex = Math.RandomInt(0, m_Waypoints.Count());
		return m_Waypoints[startPosIndex];
	}

	static bool InitSettings()
	{
		if ( !s_AIPatrolSettings )
			s_AIPatrolSettings = GetExpansionSettings().GetAIPatrol();

		return s_AIPatrolSettings.Enabled;
	}

	void SetAccuracy(float accuracyMin, float accuracyMax)
	{
		m_AccuracyMin = accuracyMin;
		m_AccuracyMax = accuracyMax;
	}

	void SetThreatDistanceLimit(float distance)
	{
		m_ThreatDistanceLimit = distance;
	}

	void SetNoiseInvestigationDistanceLimit(float distance)
	{
		m_NoiseInvestigationDistanceLimit = distance;
	}

	void SetDamageMultiplier(float multiplier)
	{
		m_DamageMultiplier = multiplier;
	}

	void SetDamageReceivedMultiplier(float multiplier)
	{
		m_DamageReceivedMultiplier = multiplier;
	}

	void SetUnits(TStringArray units)
	{
		if (m_Units)
			m_Units.Clear();
		else
			m_Units = new ExpansionArray<string>;

		m_Units.InsertAll(units);
	}

	private eAIBase CreateAI(vector pos)
	{
	#ifdef EXTRACE_DIAG
		auto trace = EXTrace.Start(EXTrace.AI, this);
	#endif

		string unit;

		if (m_Units)
			unit = m_Units.GetQuasiRandomElementAvoidRepetition();
		else
			unit = eAISurvivor.GetQuasiRandom();

		return eAIBase.Cast(GetGame().CreateObject(unit, pos));
	}

	private eAIBase SpawnAI(vector pos)
	{
	#ifdef EXTRACE_DIAG
		auto trace = EXTrace.Start(EXTrace.AI, this);
	#endif

		pos = ExpansionAIPatrol.GetPlacementPosition(pos);

		eAIBase ai = CreateAI(pos);
		if (!ai)
			return null;

		ai.SetPosition(pos);

		ExpansionHumanLoadout.Apply(ai, m_Config.Loadout, false);

		SetupAI(ai);

		return ai;
	}

	void SetupAI(eAIBase ai)
	{
		ai.SetMovementSpeedLimits(m_MovementSpeedLimit, m_MovementThreatSpeedLimit);
		ai.Expansion_SetCanBeLooted(m_Config.CanBeLooted);
		ai.eAI_SetUnlimitedReload(m_Config.UnlimitedReload);
		ai.eAI_SetAccuracy(m_AccuracyMin, m_AccuracyMax);
		ai.eAI_SetThreatDistanceLimit(m_ThreatDistanceLimit);
		ai.eAI_SetNoiseInvestigationDistanceLimit(m_NoiseInvestigationDistanceLimit);
		ai.eAI_SetDamageMultiplier(m_DamageMultiplier);
		ai.eAI_SetDamageReceivedMultiplier(m_DamageReceivedMultiplier);
		ai.eAI_SetSniperProneDistanceThreshold(m_Config.SniperProneDistanceThreshold);
		ai.eAI_SetLootingBehavior(m_Config.GetLootingBehaviour());
	}

	bool WasGroupDestroyed()
	{
		if (!m_Group)
			return false;

		if (m_WasGroupDestroyed)
			return true;

		if (m_Group.Count())
			return false;

		m_WasGroupDestroyed = true;

		Log(m_NameForLog + " bots were wiped out (spawn position " + m_Position + ", " + (m_NumberOfAI - m_Group.Count()) + "/" + m_NumberOfAI + " deceased)");

		m_Position = GetInitialSpawnPosition();  //! Reset spawn position for next spawn

		if (s_NumberOfDynamicPatrols)
			UpdatePatrolCount(-1);

		return true;
	}

	bool CanSpawn()
	{
		if (m_Group)
			return false;

		if (!m_CanSpawn)
			return false;

		int maxPatrols = GetExpansionSettings().GetAI().MaximumDynamicPatrols;
		if (maxPatrols > -1 && s_NumberOfDynamicPatrols >= maxPatrols)
			return false;

		return true;
	}

	void Spawn()
	{
	#ifdef EXTRACE_DIAG
		auto trace = EXTrace.Start(EXTrace.AI, this);
	#endif

		if (m_Group) return;

		m_TimeSinceLastSpawn = 0;
		m_CanSpawn = false;

		bool loaded;
		if (!m_WasGroupDestroyed && m_Config.Persist && m_Config.m_BaseName)
		{
			string fileName = eAIGroup.GetStorageDirectory(m_Config.m_BaseName) + eAIGroup.BASENAME;
			if (FileExist(fileName))
				loaded = eAIGroup.Load(fileName, m_Group);
		}

		if (loaded)
		{
			m_NumberOfAI = m_Group.Count();
			m_Faction = m_Group.GetFaction();

			SetNameForLog();

			Log("Loaded " + m_NumberOfAI + " persistent " + m_NameForLog + " bots at " + m_Position);
		}
		else
		{
			if (m_Config.NumberOfAI < 0)
			{
				m_NumberOfAI = Math.RandomIntInclusive(1, -m_Config.NumberOfAI);
			} else {
				m_NumberOfAI = m_Config.NumberOfAI;
			}

			m_Faction = eAIFaction.Create(m_Config.Faction);
			if (m_Faction == null) m_Faction = new eAIFactionCivilian();
			if (m_Config.Loadout == "")
				m_Config.Loadout = m_Faction.GetDefaultLoadout();

			SetNameForLog();

			Log("Spawning " + m_NumberOfAI + " " + m_NameForLog + " bots at " + m_Position);
		}

		m_WasGroupDestroyed = false;

		m_MovementSpeedLimit = m_Config.GetSpeed();
		m_MovementThreatSpeedLimit = m_Config.GetThreatSpeed();

		eAIBase ai;
		if (!loaded)
		{
			ai = SpawnAI(m_Position);
			m_Group = ai.GetGroup();
			ai.m_eAI_GroupMemberID = m_Group.m_NextGroupMemberID++;
		}
		else
		{
			for (int i = 0; i < m_Group.Count(); i++)
			{
				if (Class.CastTo(ai, m_Group.GetMember(i)))
				{
					SetupAI(ai);
				}
			}
		}

		m_Group.m_Persist = m_Config.Persist;
		m_Group.m_BaseName = m_Config.m_BaseName;
		if (m_Group.m_Persist && m_Group.m_BaseName)
			eAIGroup.s_PersistentGroups.Insert(m_Group);

		m_Group.SetName(m_Config.Name);

		if (!loaded)
			m_Group.SetFaction(m_Faction);

		m_Formation = eAIFormation.Create(m_Config.Formation);
		if (m_Formation == null) m_Formation = new eAIFormationVee();
		m_Formation.SetLooseness(m_Config.FormationLooseness);
		m_Group.SetFormation(m_Formation);

		m_WaypointBehaviour = m_Config.GetBehaviour();
		if (!loaded && m_NumberOfAI > 1)
			m_Group.SetWaypointBehaviour(eAIWaypointBehavior.HALT);  //! Only start moving after all AI spawned
		else
			m_Group.SetWaypointBehaviour(m_WaypointBehaviour);

		foreach (int idx, vector waypoint: m_Waypoints)
		{
			m_Group.AddWaypoint(waypoint);
			if (waypoint == m_Position)
			{
				m_Group.m_CurrentWaypointIndex = idx;
				m_Group.m_CurrentWaypoint = waypoint;
				if (idx != 0 && Math.RandomIntInclusive(0, 1))
					m_Group.m_BackTracking = true;
			}
		}

		if (!loaded && m_NumberOfAI > 1)
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Call(SpawnAI_Deferred, 1);

		m_IsSpawned = true;

		UpdatePatrolCount(1);
	}

	void SpawnAI_Deferred(int i)
	{
	#ifdef EXTRACE_DIAG
		auto trace = EXTrace.Start(EXTrace.AI, this);
	#endif

		if (m_Group)
		{
			eAIBase ai = SpawnAI(m_Formation.ToWorld(m_Formation.GetPosition(i)));
			ai.SetGroup(m_Group);

			if (++i < m_NumberOfAI)
				GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Call(SpawnAI_Deferred, i);
			else
				m_Group.SetWaypointBehaviour(m_WaypointBehaviour);  //! Only start moving after all AI spawned
		}
	}

	void Despawn(bool deferDespawnUntilLoosingAggro = false)
	{
		#ifdef EXTRACE_DIAG
		auto trace = EXTrace.Start(EXTrace.AI, this);
		#endif

		if (!m_IsSpawned)
			return;

		m_IsSpawned = false;
		m_TimeSinceLastSpawn = 0;

		if (m_Group)
		{
			Log("Despawning " + m_Group.Count() + " " + m_NameForLog + " bots (spawn position " + m_Position + ", " + (m_NumberOfAI - m_Group.Count()) + "/" + m_NumberOfAI + " deceased)");

			if (m_Group.m_Persist && m_Group.Count() && m_Group.m_BaseName)
			{
				m_Group.Save(true);
				m_TimeSinceLastSpawn = m_RespawnTime;  //! Allow "respawn" instantly if persistent group wasn't killed
				m_Position = m_Group.GetFormationLeader().GetPosition();  //! Update spawn position for next spawn
			}

			m_Group.ClearAI(true, deferDespawnUntilLoosingAggro);
			m_Group = null;
		}
		else
		{
			Log("Despawning " + m_NameForLog + " patrol (spawn position " + m_Position + ")");
		}

		if (!m_WasGroupDestroyed && s_NumberOfDynamicPatrols)
			UpdatePatrolCount(-1);
	}

	override void OnUpdate()
	{
		#ifdef EAI_TRACE
		auto trace = CF_Trace_0(this, "OnUpdate");
		#endif

		if ( WasGroupDestroyed() && m_RespawnTime < 0 )
		{
			return;
		}

		if (!m_Group || m_WasGroupDestroyed)
		{
			if (m_IsSpawned && !m_WasGroupDestroyed)  //! Group is NULL but not killed so was deleted behind our back, need to do cleanup
				Despawn();

			m_TimeSinceLastSpawn += eAIPatrol.UPDATE_RATE_IN_SECONDS;
			//! https://feedback.bistudio.com/T173348
			if (!m_CanSpawn && m_RespawnTime > -1 && m_TimeSinceLastSpawn >= m_RespawnTime)
				m_CanSpawn = true;
		}

		if (!m_Group)
		{
			if (!m_Trigger && CanSpawn())
			{
				m_Trigger = eAIDynamicPatrolSphereTrigger.Cast(GetGame().CreateObjectEx("eAIDynamicPatrolSphereTrigger", m_Position, ECE_LOCAL));
				m_Trigger.eAI_SetParams(this, m_MaximumRadius, m_MinimumRadius);
			}
		}
		else
		{
			vector patrolPos = m_Position;
			DayZPlayerImplement leader = m_Group.GetFormationLeader();
			if (leader)
				patrolPos = leader.GetPosition();

			if ((m_WasGroupDestroyed && m_Group.DeceasedCount() == 0) || AvoidPlayer(patrolPos, m_DespawnRadius))
			{
				if (!m_WasGroupDestroyed)
					m_TimeSinceLastSpawn += eAIPatrol.UPDATE_RATE_IN_SECONDS;
				if (m_TimeSinceLastSpawn >= m_DespawnTime)
					Despawn();
			}
		}
	}

	bool CanBeTriggeredBy(PlayerBase player)
	{
		if (!player.IsAlive())
			return false;

		//! Actual players can always trigger
		if (player.GetIdentity())
			return true;

		if (!m_Config.CanBeTriggeredByAI)
			return false;

		//! Determine if AI can trigger
		eAIGroup group = player.GetGroup();
		if (!group)
			return false;

		//! Can't trigger ourself or prevent from despawn
		if (group == m_Group)
			return false;

		eAIFaction faction = group.GetFaction();

		if (faction.IsInvincible())
			return false;

		if (faction.IsObserver())
			return false;

		if (faction.IsPassive())
			return false;

		return true;
	}

	bool AvoidPlayer(vector patrolPos, float radius)
	{
		if (m_Config.CanBeTriggeredByAI || !GetCEApi())
		{
			set<PlayerBase> players = PlayerBase.Expansion_GetInCircle(patrolPos, radius);

			foreach (auto player: players)
			{
				if (CanBeTriggeredBy(player))
					return false;
			}

			return true;
		}

		return GetCEApi().AvoidPlayer(patrolPos, radius);
	}

	private void UpdatePatrolCount(int delta)
	{
		s_NumberOfDynamicPatrols += delta;
		Log("Global patrol count: " + s_NumberOfDynamicPatrols);
	}

	override void Debug()
	{
		super.Debug();
		
		Print(m_Group);
		Print(m_TimeSinceLastSpawn);
		Print(m_CanSpawn);
		Print(m_NumberOfAI);
		Print(WasGroupDestroyed());
	}

	private void SetNameForLog()
	{
		m_NameForLog = m_Config.Name;
		if (m_NameForLog == string.Empty)
			m_NameForLog = m_Faction.GetName();
		else
			m_NameForLog = string.Format("%1 (%2)", m_NameForLog, m_Faction.GetName());
	}

	string GetNameForLog()
	{
		return m_NameForLog;
	}

	static void Log(ExpansionAIDynamicSpawnBase config, string msg)
	{
		Error("DEPRECATED, use ExpansionAISpawnBase::Log");

		if (config)
		{
			config.Log(msg);
		}
		else
		{
			auto settings = GetExpansionSettings().GetLog();
			if (settings.AIPatrol)
				settings.PrintLog("[AI Patrol] %1", msg);
		}
	}

	void Log(string msg)
	{
		auto settings = GetExpansionSettings().GetLog();
		if (m_Config && m_Config.IsInherited(ExpansionAIObjectPatrol))
		{
			if (settings.AIObjectPatrol)
				settings.PrintLog("[AI Object Patrol %1] %2", m_ID.ToStringLen(5), msg);
		}
		else if (settings.AIPatrol)
		{
			settings.PrintLog("[AI Patrol %1] %2", m_ID.ToStringLen(5), msg);
		}
	}
};

class eAIDynamicPatrolT<Class T>
{
	static T CreateEx(ExpansionAIDynamicSpawnBase config, vector startpos, bool autoStart = true)
	{
		T patrol;
		Class.CastTo(patrol, ((typename)T).Spawn());

		if (patrol.Setup(config, startpos, autoStart))
			return patrol;

		return null;
	}
}
