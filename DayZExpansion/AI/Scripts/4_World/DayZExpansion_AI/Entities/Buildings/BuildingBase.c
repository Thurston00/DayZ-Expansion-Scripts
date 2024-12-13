[eAIRegisterDynamicPatrolSpawner(BuildingBase)]
modded class BuildingBase
{
	static ref TStringArray s_eAI_PreventClimb;

	ref eAIDynamicPatrolSpawner<BuildingBase> m_eAI_DynamicPatrolSpawner;
	ref map<int, int> m_eAI_LastDoorInteractionTime = new map<int, int>;
	ref map<int, ref eAIDoorTargetInformation> m_eAI_DoorTargetInformation;
	bool m_eAI_PreventClimb;
	ref map<int, float> m_eAI_DoorAnimationTime;

	void BuildingBase()
	{
		if (!GetGame().IsServer())
			return;

		m_eAI_DynamicPatrolSpawner = new eAIDynamicPatrolSpawner<BuildingBase>(this);
	}

	override void DeferredInit()
	{
		super.DeferredInit();

		if (!GetGame().IsServer())
			return;

		//! Do not init patrols on CrashBase unless created by CE (EEOnCECreate/AfterStoreLoad will take care of init in that case)
		if (!IsInherited(CrashBase))
		{
			m_eAI_DynamicPatrolSpawner.Init();
		}

		if (!s_eAI_PreventClimb)
		{
			//! Hardcoded defaults (AI will vault those buildings' edges otherwise or climb on ledges unnecessarily)
			s_eAI_PreventClimb = {
				"land_busstop",
				"land_camp_house",
				"land_castle_bergfrit",
				//"land_cementworks",
				"land_city_firestation",
				"land_city_stand",
				"land_garage",
				"land_guardhouse",
				"land_house",
				"land_misc_polytunnel",
				"land_misc_greenhouse",
				"land_misc_trailroof",
				"land_mil_airfield_hq",
				"land_mil_atc",
				"land_mil_barracks",
				"land_mil_camonet_roof",
				"land_mil_firestation",
				"land_mil_guardbox",
				"land_mil_guardhouse",
				"land_mil_guardshed",
				"land_mil_tent",
				"land_office1",
				"land_office2",
				"land_rail_station",
				"land_shed",
				"land_tenement",
				"land_train_wagon_box",  //! Pathfinding won't find a path out of the wagon, that's the only reason it's excluded
				"land_village_healthcare",
				"land_village_store",
				"land_water_station",
				"land_workshop",
				"sign"
			};

			s_eAI_PreventClimb.InsertAll(GetExpansionSettings().GetAI().PreventClimb);
		}

		string type = GetType();
		type.ToLower();
		foreach (string preventClimb: s_eAI_PreventClimb)
		{
			if (type.IndexOf(preventClimb) >= 0)
			{
				m_eAI_PreventClimb = true;
				break;
			}
		}
	}

	override void EEOnCECreate()
	{
		super.EEOnCECreate();

		m_eAI_DynamicPatrolSpawner.Init();
	}

	override void AfterStoreLoad()
	{
		super.AfterStoreLoad();

		m_eAI_DynamicPatrolSpawner.Init();
	}

#ifdef DIAG_DEVELOPER
	override void EEHitBy(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		EXTrace.PrintHit(EXTrace.AI, this, "EEHitBy", damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);

		super.EEHitBy(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);
	}
#endif

	eAIDoorTargetInformation eAI_GetDoorTargetInformation(int doorIndex, vector position)
	{
		if (!m_eAI_DoorTargetInformation)
			m_eAI_DoorTargetInformation = new map<int, ref eAIDoorTargetInformation>;

		eAIDoorTargetInformation info;
		if (!m_eAI_DoorTargetInformation.Find(doorIndex, info))
		{
			info = new eAIDoorTargetInformation(this, doorIndex, position);
			m_eAI_DoorTargetInformation[doorIndex] = info;
		}

		return info;
	}

	float eAI_GetDoorAnimationTime(int doorIndex)
	{
		float time;

		if (!m_eAI_DoorAnimationTime)
			m_eAI_DoorAnimationTime = new map<int, float>;

		if (!m_eAI_DoorAnimationTime.Find(doorIndex, time))
		{
			string path = CFG_VEHICLESPATH + " " + GetType() + " Doors";
			int count = GetGame().ConfigGetChildrenCount(path);

			if (doorIndex < count)
			{
				string name;
				GetGame().ConfigGetChildName(path, doorIndex, name);
				time = GetGame().ConfigGetFloat(path + " " + name + " animPeriod");
			#ifdef DIAG_DEVELOPER
				EXTrace.Print(EXTrace.AI, this, "eAI_GetDoorAnimationTime(" + doorIndex + ") " + GetType() + " Doors " + name + " animPeriod " + time);
			#endif
			}

			time = Math.Max(time, 1.0);

			m_eAI_DoorAnimationTime[doorIndex] = time;
		}

		return time;
	}

	/**
	 * @brief If this is a building, return whether it can be entered or not
	 */
	bool Expansion_IsEnterable()
	{
		//! If it has no doors, assume not enterable
		if (GetDoorCount() == 0)
			return false;

		//! Ugly to have to hardcode this, but I fail to see a better way
		string type = GetType();
		type.ToLower();
		if (type.IndexOf("land_wreck_") == 0)
		{
			if (type.IndexOf("land_wreck_caravan") == 0)
				return true;

			if (type.IndexOf("land_wreck_v3s") == 0)
				return true;

			return false;
		}

		return true;
	}

	bool Expansion_IsWreck()
	{
		string type = GetType();
		type.ToLower();
		if (type.IndexOf("land_wreck_") == 0)
			return true;

		return false;
	}
};

//! Bit annoying to have to do this, it's just for error checking of AI patrol settings which uses IsKindOf
//! and BuildingBase is not an rVConfig type...
[eAIRegisterDynamicPatrolSpawner(House)]
modded class House
{
};
