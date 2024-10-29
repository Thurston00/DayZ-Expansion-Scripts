/*
 * CarScript.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

/**@class		CarScript
 * @brief
 **/
modded class CarScript
{
	static ref CF_DoublyLinkedNodes_WeakRef<CarScript> s_Expansion_AllVehicles = new CF_DoublyLinkedNodes_WeakRef<CarScript>();

	ref CF_DoublyLinkedNode_WeakRef<CarScript> m_Expansion_Node;

	protected autoptr ExpansionZoneActor m_Expansion_SafeZoneInstance = new ExpansionZoneEntity<CarScript>(this);

	protected string m_CurrentSkinName;

	protected bool m_Expansion_IsStoreLoaded;
	protected bool m_Expansion_IsStoreSaved;

#ifdef DAYZ_1_25
	bool m_Expansion_AcceptingAttachment;
#endif

	ref ExpansionGlobalID m_Expansion_GlobalID = new ExpansionGlobalID();

	ref ExpansionRPCManager m_Expansion_RPCManager;

	ref ExpansionVehicle m_ExpansionVehicle = new ExpansionVehicleT<CarScript>(this);

	// ------------------------------------------------------------
	// Constructor
	// ------------------------------------------------------------
	void CarScript()
	{
		m_Expansion_Node = s_Expansion_AllVehicles.Add(this);

		m_Expansion_RPCManager = new ExpansionRPCManager(this, CarScript);

		m_ExpansionVehicle.Init();
	}

	// ------------------------------------------------------------
	// Destructor
	// ------------------------------------------------------------
	void ~CarScript()
	{
		if (!GetGame())
			return;

		if (s_Expansion_AllVehicles)
			s_Expansion_AllVehicles.Remove(m_Expansion_Node);
	}

	override void SetActions()
	{
		super.SetActions();

		m_ExpansionVehicle.SetActions();
	}

	void Expansion_AddAction(typename actionName)
	{
		AddAction(actionName);
	}

	ExpansionVehicle GetExpansionVehicle()
	{
		return m_ExpansionVehicle;
	}

	bool Expansion_IsBike()
	{
		return false;
	}

	bool Expansion_IsBoat()
	{
		return false;
	}

	bool Expansion_IsCar()
	{
		return true;
	}

	bool Expansion_IsHelicopter()
	{
		return false;
	}

	bool Expansion_IsPlane()
	{
		return false;
	}

	bool Expansion_IsDuck()
	{
		return false;
	}

	override void DeferredInit()
	{
		super.DeferredInit();

		m_ExpansionVehicle.DeferredInit();
	}

	override bool OnStoreLoad( ParamsReadContext ctx, int version )
	{
#ifdef EXPANSIONTRACE
		auto trace = EXTrace.Start(ExpansionTracing.VEHICLES, this);
#endif

		m_Expansion_IsStoreLoaded = true;

		return super.OnStoreLoad( ctx, version );
	}

	static set< CarScript > GetAll()
	{
		Error("DEPRECATED - please use linked list s_Expansion_AllVehicles");
		set<CarScript> allVehicles = new set<CarScript>;
		auto node = s_Expansion_AllVehicles.m_Head;
		while (node)
		{
			allVehicles.Insert(node.m_Value);
			node = node.m_Next;
		}
		return allVehicles;
	}

	array< ExpansionSkin > ExpansionGetSkins()
	{
		return NULL;
	}

	bool ExpansionHasSkin( int skinIndex )
	{
		return false;
	}

	void ExpansionSetSkin( int skinIndex )
	{
	}

	string ExpansionGetCurrentSkinName()
	{
		return "";
	}
	
	int ExpansionGetCurrentSkinIndex()
	{
		return 0;
	}

	bool IsInSafeZone()
	{
		EXError.Error(this, "DEPRECATED, use GetExpansionVehicle().IsInSafeZone");
		return m_ExpansionVehicle.IsInSafeZone();
	}

	// ------------------------------------------------------------
	bool Expansion_IsInSafeZone()
	{
		EXError.Error(this, "DEPRECATED, use GetExpansionVehicle().IsInSafeZone");
		return m_ExpansionVehicle.IsInSafeZone();
	}

	// ------------------------------------------------------------
	bool CanBeDamaged()
	{
		return m_ExpansionVehicle.CanBeDamaged();
	}

	// ------------------------------------------------------------
	// Called only server side
	// ------------------------------------------------------------
	void OnEnterZone(ExpansionZoneType type)
	{
		m_ExpansionVehicle.OnEnterZone(type);
	}

	override void OnCEUpdate()
	{
		super.OnCEUpdate();

		m_ExpansionVehicle.SafeZoneCleanup(m_ElapsedSinceLastUpdate);
	}

	void ExpansionCreateCleanup()
	{
		EXError.Error(this, "DEPRECATED");
	}

	// ------------------------------------------------------------
	// Called only server side
	// ------------------------------------------------------------
	void OnExitZone(ExpansionZoneType type)
	{
		m_ExpansionVehicle.OnExitZone(type);
	}

	void Expansion_ResetSZParkingTime()
	{
		EXError.Error(this, "DEPRECATED");
	}

	void SetLockedState(ExpansionVehicleLockState newLockState)
	{
		EXError.Error(this, "DEPRECATED, use GetExpansionVehicle().SetLockState");
		m_ExpansionVehicle.SetLockState(newLockState);
	}

	ExpansionVehicleLockState GetLockedState()
	{
		EXError.Error(this, "DEPRECATED, use GetExpansionVehicle().GetLockState");
		return m_ExpansionVehicle.GetLockState();
	}

	void Expansion_SetAllDoorsAnimationPhase(float phase)
	{
		EXError.Error(this, "DEPRECATED, use GetExpansionVehicle().SetAllDoorsAnimationPhase");
		m_ExpansionVehicle.SetAllDoorsAnimationPhase(phase);
	}

	void Expansion_CloseAllDoors()
	{
		ForceUpdateLightsStart();
		m_ExpansionVehicle.SetAllDoorsAnimationPhase(0.0);
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(ForceUpdateLightsEnd, 500);
	}

	override bool EEOnDamageCalculated(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		if (!super.EEOnDamageCalculated(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef))
			return false;

		return m_ExpansionVehicle.CanBeDamaged();
	}

	bool Expansion_CanGetInSeat(PlayerBase player, int seatIdx)
	{
		return true;
	}

	bool Expansion_CanObjectAttach(Object obj)
	{
		if (obj.IsMan())
			return Expansion_CanPlayerAttach();

		return false;
	}

	bool Expansion_CanPlayerAttach()
	{
		return ConfigGetFloat("animPhysDetachSpeed") > 0;
	}

	bool Expansion_IsVehicleFunctional(bool checkOptionalParts = false, set<typename> missingComponents = null)
	{
		if (IsDamageDestroyed())
			return false;

		if (GetFluidFraction(CarFluid.FUEL) <= 0)
		{
			if (missingComponents)
				missingComponents.Insert(CarFluid);
			return false;
		}

		EntityAI item;

		if (IsVitalCarBattery() || IsVitalTruckBattery())
		{
			item = GetBattery();
			if (!item || item.IsRuined() || item.GetCompEM().GetEnergy() < m_BatteryEnergyStartMin)
			{
				if (missingComponents)
					missingComponents.Insert(VehicleBattery);
				return false;
			}
		}

		TStringArray plugSlotsToCheck = {};

		if (IsVitalSparkPlug())
			plugSlotsToCheck.Insert("SparkPlug");

		if (IsVitalGlowPlug())
			plugSlotsToCheck.Insert("GlowPlug");

		foreach (string plugSlotName: plugSlotsToCheck)
		{
			item = FindAttachmentBySlotName(plugSlotName);
			if (!item || item.IsRuined())
			{
				if (missingComponents)
					missingComponents.Insert(SparkPlug);
				return false;
			}
		}

		if (checkOptionalParts)
		{
			if (IsVitalRadiator())
			{
				item = GetRadiator();
				if (!item || item.IsRuined())
				{
					if (missingComponents)
						missingComponents.Insert(CarRadiator);
					return false;
				}
			}
		}

		return true;
	}
	
	string Expansion_GetAnimSourceFromSelection(string selection)
	{
		return GetAnimSourceFromSelection(selection);
	}
	
	string Expansion_GetDoorInvSlotNameFromSeatPos(int posIdx)
	{
		return GetDoorInvSlotNameFromSeatPos(posIdx);
	}
	
	string Expansion_GetDoorSelectionNameFromSeatPos(int posIdx)
	{
		return GetDoorSelectionNameFromSeatPos(posIdx);
	}

	int Expansion_GetCarDoorsState(string slotType)
	{
		return GetCarDoorsState(slotType);
	}

	override void EEInit()
	{
		super.EEInit();

		if (IsMissionHost() && GetExpansionSettings().GetSafeZone().Enabled)
			m_Expansion_SafeZoneInstance.Update();
	}

	override void EEDelete(EntityAI parent)
	{
		super.EEDelete(parent);

		m_ExpansionVehicle.OnDelete();
	}

	override void DamageCrew(float dmg)
	{
		if (m_ExpansionVehicle.IsInSafeZone())
			return;

		super.DamageCrew(dmg);
	}

	#ifdef EXPANSION_MODSTORAGE
	override void CF_OnStoreSave(CF_ModStorageMap storage)
	{
		super.CF_OnStoreSave(storage);

		m_Expansion_IsStoreSaved = true;

		auto ctx = storage[DZ_Expansion_Core];
		if (!ctx) return;

		ctx.Write(m_CurrentSkinName);

		#ifdef SERVER
		if (!m_Expansion_GlobalID.m_IsSet)
			m_Expansion_GlobalID.Acquire();
		#endif

		m_Expansion_GlobalID.OnStoreSave(ctx);
	}

	override bool CF_OnStoreLoad(CF_ModStorageMap storage)
	{
		if (!super.CF_OnStoreLoad(storage))
			return false;

		auto ctx = storage[DZ_Expansion_Core];
		if (!ctx) return true;

		if (!ctx.Read(m_CurrentSkinName))
			return false;

		if (ctx.GetVersion() < 41)
			return true;

		if (!m_Expansion_GlobalID.OnStoreLoad(ctx))
			return false;

		return true;
	}
	#endif

	bool Expansion_IsStoreLoaded()
	{
		return m_Expansion_IsStoreLoaded;
	}

	bool Expansion_IsStoreSaved()
	{
		return m_Expansion_IsStoreSaved;
	}

	override void OnVariablesSynchronized()
	{
		super.OnVariablesSynchronized();

		m_ExpansionVehicle.OnVariablesSynchronized();
	}

	void ExpansionSetLastDriverUID(PlayerBase player)
	{
		EXError.Error(this, "DEPRECATED, use GetExpansionVehicle().SetLastDriverUID");
		m_ExpansionVehicle.SetLastDriverUID(player);
	}

	void ExpansionResetLastDriverUIDSynch()
	{
		EXError.Error(this, "DEPRECATED, use GetExpansionVehicle().ResetLastDriverUIDSynch");
		m_ExpansionVehicle.ResetLastDriverUIDSynch();
	}

	string ExpansionGetLastDriverUID()
	{
		EXError.Error(this, "DEPRECATED, use GetExpansionVehicle().GetLastDriverUID");
		return m_ExpansionVehicle.GetLastDriverUID();
	}

	set<Human> Expansion_GetVehicleCrew(bool playersOnly = true, bool includeAttached = true)
	{
		EXError.Error(this, "DEPRECATED, use GetExpansionVehicle().GetCrew");
		return m_ExpansionVehicle.GetCrew(playersOnly, includeAttached);
	}

	void Expansion_ForceCrewGetOut()
	{
		EXError.Error(this, "DEPRECATED, use GetExpansionVehicle().ForceCrewGetOut");
		m_ExpansionVehicle.ForceCrewGetOut();
	}

	float Expansion_GetFuelAmmount()
	{
		return m_FuelAmmount;
	}

	void Expansion_EstimateTransform(float pDt, inout vector mat[4])
	{
		vector transform[4];
		GetTransform(transform);

		vector velocity = GetVelocity(this);
		vector angularVelocity = dBodyGetAngularVelocity(this);

		vector futureAngularVelocity = (angularVelocity * pDt);

		mat[0][0] = 0.0;
		mat[1][1] = 0.0;
		mat[2][2] = 0.0;

		mat[0][1] = -futureAngularVelocity[2];
		mat[1][0] = futureAngularVelocity[2];
		mat[2][0] = -futureAngularVelocity[1];
		mat[0][2] = futureAngularVelocity[1];
		mat[1][2] = -futureAngularVelocity[0];
		mat[2][1] = futureAngularVelocity[0];

		Math3D.MatrixInvMultiply3(mat, transform, mat);

		mat[0] = transform[0] + mat[0];
		mat[1] = transform[1] + mat[1];
		mat[2] = transform[2] + mat[2];

		mat[0].Normalize();
		mat[1].Normalize();
		mat[2].Normalize();

		mat[3] = transform[3] + (velocity * pDt);
	}

	override void EEHitBy(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		super.EEHitBy(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);

		if (damageType == DT_EXPLOSION && ExpansionDamageSystem.IsEnabledForExplosionTarget(this))
			ExpansionDamageSystem.OnExplosionHit(source, this, ammo);
	}
};