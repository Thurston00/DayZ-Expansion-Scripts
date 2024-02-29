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

	protected bool m_Expansion_IsInSafeZone;
	protected bool m_Expansion_IsInSafeZone_DeprecationWarning;
	protected bool m_Expansion_SZCleanup;
	protected float m_Expansion_SZParkingTime;

	protected string m_CurrentSkinName;

	protected bool m_Expansion_IsStoreLoaded;
	protected bool m_Expansion_IsStoreSaved;

	protected string m_Expansion_LastDriverUID;
	protected bool m_Expansion_SynchLastDriverUID;
	protected bool m_Expansion_LastDriverUIDSynched;

	bool m_Expansion_AcceptingAttachment;

	int m_Expansion_CargoCount;

	ref ExpansionGlobalID m_Expansion_GlobalID = new ExpansionGlobalID();

	ref ExpansionRPCManager m_Expansion_RPCManager;

	// ------------------------------------------------------------
	// Constructor
	// ------------------------------------------------------------
	void CarScript()
	{
		m_Expansion_Node = s_Expansion_AllVehicles.Add(this);
		RegisterNetSyncVariableBool("m_Expansion_SynchLastDriverUID");
		RegisterNetSyncVariableInt("m_Expansion_CargoCount");

		m_Expansion_RPCManager = new ExpansionRPCManager(this, CarScript);
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

	override void DeferredInit()
	{
		super.DeferredInit();

		if (m_Expansion_IsStoreLoaded && !IsSetForDeletion() && ExpansionEntityStoragePlaceholder.Expansion_HasPlaceholder(this))
		{
			EXPrint("Deleting " + this + " " + GetPosition() + " global ID " + m_Expansion_GlobalID.IDToString());
			Delete();
		}
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

	bool IsInSafeZone()
	{
		Expansion_Error("DEPRECATED: Please use Expansion_IsInSafeZone", m_Expansion_IsInSafeZone_DeprecationWarning);
		return Expansion_IsInSafeZone();
	}

	// ------------------------------------------------------------
	bool Expansion_IsInSafeZone()
	{
		return m_Expansion_IsInSafeZone;
	}

	// ------------------------------------------------------------
	bool CanBeDamaged()
	{
		if ( GetExpansionSettings().GetSafeZone().Enabled && Expansion_IsInSafeZone() )
		{
			return !GetExpansionSettings().GetSafeZone().DisableVehicleDamageInSafeZone;
		}

		return GetAllowDamage();
	}

	// ------------------------------------------------------------
	// Called only server side
	// ------------------------------------------------------------
	void OnEnterZone(ExpansionZoneType type)
	{
		if (type != ExpansionZoneType.SAFE) return;

		EXTrace.Print(EXTrace.VEHICLES, this, "::OnEnterZone " + GetPosition());

		m_Expansion_IsInSafeZone = true;

		if (GetExpansionSettings().GetSafeZone().EnableForceSZCleanupVehicles)
			ExpansionCreateCleanup();
	}

	override void OnCEUpdate()
	{
		super.OnCEUpdate();

		Expansion_UpdateSafeZoneCleanup();
	}

	void Expansion_UpdateSafeZoneCleanup()
	{
		//! If vehicle is not in a safezone, bail
		if (!m_Expansion_IsInSafeZone)
			return;

		//! If vehicle vehicle safezone cleanup is not enabled, bail
		if (!m_Expansion_SZCleanup)
			return;

	#ifdef EXPANSIONMODVEHICLE
		//! Heli needs to be landed to count as parked
		ExpansionHelicopterScript heli;
		if (Class.CastTo(heli, this) && heli.Expansion_EngineIsSpinning() && !heli.IsLanded())
			return;
	#endif

		//! If vehicle isn't moving, consider it parked and increase parking time
		if (!IsMoving())
			m_Expansion_SZParkingTime += m_ElapsedSinceLastUpdate;

		//! If parking time is zero, bail
		if (!m_Expansion_SZParkingTime)
			return;

		//! If parking time exceeds max allowed vehicle lifetime in SZ, delete it
		float lifetime = GetExpansionSettings().GetSafeZone().VehicleLifetimeInSafeZone;
		if (m_Expansion_SZParkingTime > lifetime)
		{
			Expansion_ForceCrewGetOut();

			//! If we have last driver UID, notify player that their vehicle is being deleted
			if (m_Expansion_LastDriverUID)
			{
				PlayerBase player = PlayerBase.GetPlayerByUID(m_Expansion_LastDriverUID);
				if (player)
					ExpansionNotification("STR_EXPANSION_SAFEZONE_TITLE", string.Format("%1 at %2 was deleted after exceeding the maximum allowed safezone parking time of %3.", GetDisplayName(), ExpansionStatic.VectorToString(GetPosition(), ExpansionVectorToString.Labels), ExpansionStatic.GetTimeString(lifetime, true))).Error(player.GetIdentity());
			}

			Delete();
		}
	}

	void ExpansionCreateCleanup()
	{
		if (m_Expansion_SZCleanup)
			return;

		if (ExpansionStatic.IsAnyOf(this, GetExpansionSettings().GetSafeZone().ForceSZCleanup_ExcludedItems, true))
			return;

		m_Expansion_SZCleanup = true;

		#ifdef EXPANSION_SAFEZONE_DEBUG
		EXPrint("[CORE][Expansion_SZCleanup] " + ToString() + " " + GetPosition() + " marked for cleanup - lifetime " + GetLifetime());
		#endif
	}

	// ------------------------------------------------------------
	// Called only server side
	// ------------------------------------------------------------
	void OnExitZone(ExpansionZoneType type)
	{
		if (type != ExpansionZoneType.SAFE) return;

		EXTrace.Print(EXTrace.VEHICLES, this, "::OnExitZone " + GetPosition());

		m_Expansion_IsInSafeZone = false;
		m_Expansion_SZCleanup = false;
		Expansion_ResetSZParkingTime();
	}

	void Expansion_ResetSZParkingTime()
	{
		m_Expansion_SZParkingTime = 0;
	}

	void SetLockedState(ExpansionVehicleLockState newLockState)
	{
		Error("NOT IMPLEMENTED");
	}

	ExpansionVehicleLockState GetLockedState()
	{
		Error("NOT IMPLEMENTED");
		return ExpansionVehicleLockState.NOLOCK;
	}

	void Expansion_SetAllDoorsAnimationPhase(float phase)
	{
		CarDoor carDoor;

		for (int i = 0; i < GetInventory().AttachmentCount(); i++)
		{
			if (Class.CastTo(carDoor, GetInventory().GetAttachmentFromIndex(i)))
			{
				TStringArray selectionNames = {};

				carDoor.GetActionComponentNameList(0, selectionNames);

				if (!selectionNames.Count())
					continue;

				TStringArray animSources = {};
				string animSource;

				foreach (string selectionName: selectionNames)
				{
					animSource = GetAnimSourceFromSelection(selectionName);
					if (animSource)
						animSources.Insert(animSource);
				}

				 //! Turns out correct selection is always last one when doing it this way
				int lastIdx = animSources.Count() - 1;
				if (lastIdx > -1)
				{
					animSource = animSources[lastIdx];
					if (GetAnimationPhase(animSource) > 0.0)
						SetAnimationPhase(animSource, 0.0);
				}
			}
		}
	}

	void Expansion_CloseAllDoors()
	{
		ForceUpdateLightsStart();
		Expansion_SetAllDoorsAnimationPhase(0.0);
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(ForceUpdateLightsEnd, 500);
	}

	override bool EEOnDamageCalculated(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		if (!super.EEOnDamageCalculated(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef))
			return false;

		return CanBeDamaged();
	}

	bool Expansion_CanObjectAttach(Object obj)
	{
		return false;
	}

	bool Expansion_IsVehicleFunctional(bool checkOptionalParts = false)
	{
		if (IsDamageDestroyed())
			return false;

		if (GetFluidFraction(CarFluid.FUEL) <= 0)
			return false;

		EntityAI item;

		if (IsVitalCarBattery() || IsVitalTruckBattery())
		{
			item = GetBattery();
			if (!item || item.IsRuined() || item.GetCompEM().GetEnergy() < m_BatteryEnergyStartMin)
				return false;
		}

		if (IsVitalSparkPlug())
		{
			item = FindAttachmentBySlotName("SparkPlug");
			if (!item || item.IsRuined())
				return false;
		}

		if (IsVitalGlowPlug())
		{
			item = FindAttachmentBySlotName("GlowPlug");
			if (!item || item.IsRuined())
				return false;
		}

		if (checkOptionalParts)
		{
			if (IsVitalRadiator())
			{
				item = GetRadiator();
				if (!item || item.IsRuined())
					return false;
			}
		}

		return true;
	}

	override void EEInit()
	{
		super.EEInit();

		if (IsMissionHost() && GetExpansionSettings().GetSafeZone().Enabled)
			m_Expansion_SafeZoneInstance.Update();
	}

	override void DamageCrew(float dmg)
	{
		if (Expansion_IsInSafeZone())
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

	override void OnVariablesSynchronized()
	{
		super.OnVariablesSynchronized();

		if (m_Expansion_SynchLastDriverUID != m_Expansion_LastDriverUIDSynched)
		{
			m_Expansion_LastDriverUIDSynched = m_Expansion_SynchLastDriverUID;

			if (!m_Expansion_SynchLastDriverUID)
				return;

			//! Reset m_Expansion_LastDriverUID client-side if vehicle has driver and it is not the player
			Human driver = CrewMember(DayZPlayerConstants.VEHICLESEAT_DRIVER);
			Man player = GetGame().GetPlayer();
			if (driver && player && driver != player)
			{
				m_Expansion_LastDriverUID = "";
			}
		}
	}

	void ExpansionSetLastDriverUID(PlayerBase player)
	{
		m_Expansion_LastDriverUID = player.GetIdentityUID();

		EXPrint(ToString() + "::ExpansionSetLastDriverUID - " + m_Expansion_LastDriverUID);

		if (!IsMissionHost())
			return;

		m_Expansion_SynchLastDriverUID = true;

		SetSynchDirty();
	}

	void ExpansionResetLastDriverUIDSynch()
	{
		EXPrint(ToString() + "::ExpansionResetLastDriverUIDSynch");

		m_Expansion_SynchLastDriverUID = false;

		SetSynchDirty();
	}

	string ExpansionGetLastDriverUID()
	{
		return m_Expansion_LastDriverUID;
	}

	set<Human> Expansion_GetVehicleCrew(bool playersOnly = true, bool includeAttached = true)
	{
		set<Human> players = new set<Human>;
		Human crew;

		//! Seated players
		for (int i = 0; i < CrewSize(); i++)
		{
			crew = CrewMember(i);
			if (!crew)
				continue;

			if (!playersOnly || crew.GetIdentity())
				players.Insert(crew);
		}

		if (!includeAttached)
			return players;

		//! Attached players
		IEntity child = GetChildren();
		while (child)
		{
			crew = Human.Cast(child);

			child = child.GetSibling();

			if (!crew)
				continue;

			if (!playersOnly || crew.GetIdentity())
				players.Insert(crew);
		}

		return players;
	}

	void Expansion_ForceCrewGetOut()
	{
		auto crew = Expansion_GetVehicleCrew(false, false);
		foreach (auto member: crew)
		{
			//! Open the door so player can get out
			int crewIdx = CrewMemberIndex(member);
			string selection = GetDoorSelectionNameFromSeatPos(crewIdx);
			if (selection)
				SetAnimationPhase(GetAnimSourceFromSelection(selection), 1.0);

			//! Push them out
			HumanCommandVehicle vehCommand = member.GetCommand_Vehicle();
			if (vehCommand)
				vehCommand.GetOutVehicle();
		}
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