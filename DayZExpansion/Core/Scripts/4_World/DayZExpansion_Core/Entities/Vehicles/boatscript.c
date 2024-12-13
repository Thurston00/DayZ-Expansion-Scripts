#ifndef DAYZ_1_25
modded class BoatScript
{
	static ref CF_DoublyLinkedNodes_WeakRef<BoatScript> s_Expansion_AllVehicles = new CF_DoublyLinkedNodes_WeakRef<BoatScript>();

	ref CF_DoublyLinkedNode_WeakRef<BoatScript> m_Expansion_Node;

	protected autoptr ExpansionZoneActor m_Expansion_SafeZoneInstance = new ExpansionZoneEntity<BoatScript>(this);

	protected string m_Expansion_CurrentSkinName;

	protected bool m_Expansion_IsStoreLoaded;
	protected bool m_Expansion_IsStoreSaved;

	ref ExpansionGlobalID m_Expansion_GlobalID = new ExpansionGlobalID();

	ref ExpansionRPCManager m_Expansion_RPCManager;

	ref ExpansionVehicle m_ExpansionVehicle = new ExpansionVehicleT<BoatScript>(this);

	void BoatScript()
	{
		m_Expansion_Node = s_Expansion_AllVehicles.Add(this);

		m_Expansion_RPCManager = new ExpansionRPCManager(this, BoatScript);

		m_ExpansionVehicle.Init();
	}

	void ~BoatScript()
	{
		if (!GetGame())
			return;

		if (s_Expansion_AllVehicles)
			s_Expansion_AllVehicles.Remove(m_Expansion_Node);
	}

	override void EEInit()
	{
		super.EEInit();

		if (GetGame().IsServer() && GetExpansionSettings().GetSafeZone().Enabled)
			m_Expansion_SafeZoneInstance.Update();
	}

	override void DeferredInit()
	{
		super.DeferredInit();

		m_ExpansionVehicle.DeferredInit();
	}

	override void EEDelete(EntityAI parent)
	{
		super.EEDelete(parent);

		m_ExpansionVehicle.OnDelete();
	}

	override void EEKilled( Object killer )
	{
		super.EEKilled( killer );

		m_ExpansionVehicle.OnDestroyed(killer);
	}

	override bool OnStoreLoad( ParamsReadContext ctx, int version )
	{
		m_Expansion_IsStoreLoaded = true;

		return super.OnStoreLoad( ctx, version );
	}

#ifdef EXPANSION_MODSTORAGE
	override void CF_OnStoreSave(CF_ModStorageMap storage)
	{
		super.CF_OnStoreSave(storage);

		m_Expansion_IsStoreSaved = true;

		auto ctx = storage[DZ_Expansion_Core];
		if (!ctx) return;

		ctx.Write(m_Expansion_CurrentSkinName);

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

		if (!ctx.Read(m_Expansion_CurrentSkinName))
			return false;

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

	override bool EEOnDamageCalculated(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		if (!super.EEOnDamageCalculated(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef))
			return false;

		if (!m_ExpansionVehicle.CanBeDamaged())
			return false;

		if (damageType == DT_EXPLOSION)
		{
			if (!ExpansionDamageSystem.OnExplosionDamageCalculated(damageResult, source, this, component, dmgZone, ammo, modelPos, speedCoef, true))
				return false;
		}

		return true;
	}

	override void EEHitBy(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		super.EEHitBy(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);

		if (damageType == DT_EXPLOSION && ExpansionDamageSystem.IsEnabledForExplosionTarget(this))
			ExpansionDamageSystem.OnExplosionHit(source, this, ammo);
	}

	override void OnCEUpdate()
	{
		super.OnCEUpdate();

		m_ExpansionVehicle.SafeZoneCleanup(m_ElapsedSinceLastUpdate);
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
		return true;
	}

	bool Expansion_IsCar()
	{
		return false;
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

	bool Expansion_EngineIsOn()
	{
		return EngineIsOn();
	}

	void Expansion_EngineStop()
	{
		EngineStop();
	}

	bool Expansion_IsVehicleFunctional(bool checkOptionalParts = false, set<typename> missingComponents = null)
	{
		if (IsDamageDestroyed())
			return false;

		if (GetFluidFraction(BoatFluid.FUEL) <= 0)
		{
			if (missingComponents)
				missingComponents.Insert(BoatFluid);
			return false;
		}

		EntityAI item;

		if (IsVitalSparkPlug())
		{
			item = FindAttachmentBySlotName("SparkPlug");
			if (!item || item.IsRuined())
			{
				if (missingComponents)
					missingComponents.Insert(SparkPlug);
				return false;
			}
		}

		return true;
	}
	
	//! @note if GetAnimSourceFromSelection is ever added to BoatScript in vanilla, this should call it and return the result
	string Expansion_GetAnimSourceFromSelection(string selection)
	{
		return "";
	}
	
	//! @note if GetDoorInvSlotNameFromSeatPos is ever added to BoatScript in vanilla, this should call it and return the result
	string Expansion_GetDoorInvSlotNameFromSeatPos(int posIdx)
	{
		return "";
	}
	
	//! @note if GetDoorSelectionNameFromSeatPos is ever added to BoatScript in vanilla, this should call it and return the result
	string Expansion_GetDoorSelectionNameFromSeatPos(int posIdx)
	{
		return "";
	}

	//! @note if GetCarDoorsState is ever added to BoatScript in vanilla, this should call it and return the result
	int Expansion_GetCarDoorsState(string slotType)
	{
		return 0;
	}

	void OnEnterZone(ExpansionZoneType type)
	{
		m_ExpansionVehicle.OnEnterZone(type);
	}

	void OnExitZone(ExpansionZoneType type)
	{
		m_ExpansionVehicle.OnExitZone(type);
	}

	void Expansion_CloseAllDoors()
	{
		m_ExpansionVehicle.SetAllDoorsAnimationPhase(0.0);
	}

	void ExpansionSetSkin(int skinIndex)
	{
	}

	bool ExpansionHasSkin(int skinIndex)
	{
		return false;
	}

	string ExpansionGetCurrentSkinName()
	{
		return m_Expansion_CurrentSkinName;
	}
	
	int ExpansionGetCurrentSkinIndex()
	{
		return 0;
	}
}
#endif
