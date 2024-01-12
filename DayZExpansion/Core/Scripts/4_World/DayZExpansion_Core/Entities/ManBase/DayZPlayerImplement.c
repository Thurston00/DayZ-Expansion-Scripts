/**
 * DayZPlayerImplement.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

modded class DayZPlayerImplement
{
	ref ExpansionHumanST m_ExpansionST;

	protected bool m_Expansion_CanBeLooted = true;

	protected autoptr ExpansionZoneActor m_Expansion_SafeZoneInstance = new ExpansionZoneEntity<DayZPlayerImplement>(this);

	protected bool m_Expansion_IsInSafeZone;
	protected bool m_Expansion_IsInSafeZone_DeprecationWarning;
	protected bool m_Expansion_IsInSafeZoneSynchRemote;

	ref ExpansionNetsyncData m_Expansion_NetsyncData;
	ref ExpansionRPCManager m_Expansion_RPCManager;

	ref map<string, bool> m_Expansion_DisabledAmmoDamage = new map<string, bool>;
	bool m_Expansion_AllowDamage = true;

	vector m_Expansion_HeadBonePositionMS;
	float m_Expansion_HeadBoneIdleTime;
	vector m_Expansion_LHandBonePositionMS;
	float m_Expansion_LHandBoneIdleTime;
	vector m_Expansion_RHandBonePositionMS;
	float m_Expansion_RHandBoneIdleTime;
	vector m_Expansion_LFootBonePositionMS;
	float m_Expansion_LFootBoneIdleTime;
	vector m_Expansion_RFootBonePositionMS;
	float m_Expansion_RFootBoneIdleTime;

	bool m_Expansion_EnableBonePositionUpdate;

	int m_Expansion_CurrentPhxInteractionLayer;

	ref map<int, Object> m_Expansion_DebugObjects = new map<int, Object>();

	void DayZPlayerImplement()
	{
		Expansion_Init();
	}

	//! For NPCs that don't inherit from PlayerBase
	override bool IsInventoryVisible()
	{
		if (!super.IsInventoryVisible())
			return false;

		return m_Expansion_CanBeLooted;
	}

	void Expansion_Init()
	{
		RegisterNetSyncVariableBool("m_Expansion_IsInSafeZoneSynchRemote");
	}

	bool Expansion_HasAdminToolGodMode()
	{
		return false;
	}

	bool Expansion_HasAdminToolInvisibility()
	{
		return false;
	}

	void OnEnterZone(ExpansionZoneType type)
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.ZONES, this, "OnEnterZone");
#endif

		if (type == ExpansionZoneType.SAFE)
		{
			m_Expansion_IsInSafeZone = true;

		#ifdef SERVER
			m_Expansion_IsInSafeZoneSynchRemote = true;
			SetSynchDirty();
		#endif
		}
	}

	void OnExitZone(ExpansionZoneType type)
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.ZONES, this, "OnExitZone");
#endif

		if (type == ExpansionZoneType.SAFE)
		{
			m_Expansion_IsInSafeZone = false;

		#ifdef SERVER
			m_Expansion_IsInSafeZoneSynchRemote = false;
			SetSynchDirty();
		#endif
		}
	}
	
	// ------------------------------------------------------------
	// Expansion OnVariablesSynchronized
	// ------------------------------------------------------------
	override void OnVariablesSynchronized()
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.PLAYER, this, "OnVariablesSynchronized");
#endif

		super.OnVariablesSynchronized();
		
		if (!GetGame().IsClient())
			return;

		if ( m_Expansion_IsInSafeZoneSynchRemote && !m_Expansion_IsInSafeZone )
		{
			OnEnterZone(ExpansionZoneType.SAFE);
		} 
		else if ( !m_Expansion_IsInSafeZoneSynchRemote && m_Expansion_IsInSafeZone )
		{
			OnExitZone(ExpansionZoneType.SAFE);
		}

		if (m_Expansion_NetsyncData && !m_Expansion_NetsyncData.m_WasDataRequested)
			m_Expansion_NetsyncData.Request();
	}

	bool IsInSafeZone()
	{
		Expansion_Error("DEPRECATED: Please use Expansion_IsInSafeZone", m_Expansion_IsInSafeZone_DeprecationWarning);
		return Expansion_IsInSafeZone();
	}
	
	bool Expansion_IsInSafeZone()
	{
		return m_Expansion_IsInSafeZone;
	}

	//! Netsynced version of SetAllowDamage
	void Expansion_SetAllowDamage(bool state)
	{
		if (GetGame().IsServer())
			SetAllowDamage(state);
		else
			m_Expansion_AllowDamage = state;
	}

	void Expansion_SetAllowDamageEx(string ammoType, bool state)
	{
		//! @note State is inverted so we can easily check m_Expansion_DisabledAmmoDamage[ammoType] to see whether it's disabled
		m_Expansion_DisabledAmmoDamage[ammoType] = !state;
	}

	bool Expansion_CanBeDamaged(string ammo = string.Empty)
	{
		bool canBeDamaged;
		if (GetGame().IsServer())
			canBeDamaged = GetAllowDamage();
		else
			canBeDamaged = m_Expansion_AllowDamage;
		return canBeDamaged && !m_Expansion_IsInSafeZone && !m_Expansion_DisabledAmmoDamage[ammo];
	}

	void Expansion_SetCanBeLooted(bool canBeLooted)
	{
		m_Expansion_CanBeLooted = canBeLooted;
	}

	bool Expansion_CanBeLooted()
	{
		return m_Expansion_CanBeLooted;
	}

	override bool EEOnDamageCalculated(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		if (!super.EEOnDamageCalculated(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef))
			return false;

		return Expansion_CanBeDamaged(ammo);
	}

	override void EEHitBy(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		super.EEHitBy(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);

		if (damageType == DT_EXPLOSION && ExpansionDamageSystem.IsEnabledForExplosionTarget(this))
			ExpansionDamageSystem.OnExplosionHit(source, this, ammo, true, damageResult);
	}

	override void EEKilled(Object killer)
	{
		super.EEKilled(killer);

		if (!m_Expansion_CanBeLooted)
			//! 10134 = 2 | 4 | 16 | 128 | 256 | 512 | 1024 | 8192
			ExpansionStatic.LockInventoryRecursive(this, 10134);

		Expansion_DeleteDebugObjects();
	}

	override void EEDelete(EntityAI parent)
	{
		super.EEDelete(parent);

		Expansion_DeleteDebugObjects();
	}

	override bool CanBeSkinned()
	{
		if (!m_Expansion_CanBeLooted)
			return false;

		return super.CanBeSkinned();
	}

	override bool NameOverride(out string output)
	{
		if (m_Expansion_NetsyncData && m_Expansion_NetsyncData.Get(0, output))
			return true;
		else
			return super.NameOverride(output);
	}

	override void CommandHandler(float pDt, int pCurrentCommandID, bool pCurrentCommandFinished)
	{
#ifdef DIAG
		int interactionLayer = dBodyGetInteractionLayer(this);
#endif

		super.CommandHandler(pDt, pCurrentCommandID, pCurrentCommandFinished);

		if (m_Expansion_EnableBonePositionUpdate)
			Expansion_UpdateBonePositionTimes(pDt);

#ifdef DIAG
		if (interactionLayer != m_Expansion_CurrentPhxInteractionLayer)
		{
			m_Expansion_CurrentPhxInteractionLayer = interactionLayer;
			EXTrace.Print(EXTrace.PLAYER, this, "::CommandHandler current interaction layer " + ExpansionStatic.BitmaskEnumToString(PhxInteractionLayers, interactionLayer));
		}
#endif
	}

	vector Expansion_GetHeadingVector()
	{
		return Vector(-GetInputController().GetHeadingAngle() * Math.RAD2DEG, 0, 0).AnglesToVector();
	}

	float Expansion_GetMovementSpeed()
	{
		HumanCommandMove hcm = GetCommand_Move();
		if (hcm)
			return hcm.GetCurrentMovementSpeed();

		return 0.0;
	}

	float Expansion_GetMovementAngle()
	{
		HumanCommandMove hcm = GetCommand_Move();
		if (hcm)
			return hcm.GetCurrentMovementAngle();

		return 0.0;
	}

	void Expansion_DebugObject_Deferred(int i, vector position, string type = "ExpansionDebugBox", vector direction = vector.Zero, vector origin = vector.Zero, float lifetime = 300.0, int flags = 0)
	{
#ifdef DIAG
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(Expansion_DebugObject, 1, false, i, position, type, direction, origin, lifetime, flags);
#endif
	}

	void Expansion_DebugObject(int i, vector position, string type = "ExpansionDebugBox", vector direction = vector.Zero, vector origin = vector.Zero, float lifetime = 300.0, int flags = 0)
	{
#ifdef DIAG
		if (!m_Expansion_DebugObjects[i])
		{
			int iFlags;
			#ifdef SERVER
			iFlags = ECE_NOLIFETIME;
			#else
			iFlags = ECE_LOCAL;
			#endif
			Object obj = GetGame().CreateObjectEx(type, position, iFlags);
			if (!obj)
				return;
			m_Expansion_DebugObjects[i] = obj;
			#ifdef SERVER
			EntityAI ent;
			if (Class.CastTo(ent, obj))
				ent.SetLifetime(lifetime);
			#endif
		}
		else
		{
			m_Expansion_DebugObjects[i].SetPosition(position);
		}

		if (direction != vector.Zero)
		{
			m_Expansion_DebugObjects[i].SetDirection(direction);
		}

		if (origin != vector.Zero)
		{
			ExpansionDebugObject dbgObj;
			if (Class.CastTo(dbgObj, m_Expansion_DebugObjects[i]))
			{
				dbgObj.Expansion_DrawDebugLine(origin);
				dbgObj.Expansion_SetDebugLineFlags(flags);
			}
		}
#endif
	}

	void Expansion_DeleteDebugObject(int i)
	{
		Object obj = m_Expansion_DebugObjects[i];
		if (obj)
			GetGame().ObjectDelete(obj);

		m_Expansion_DebugObjects.Remove(i);
	}

	void Expansion_DeleteDebugObjects()
	{
		foreach (Object obj: m_Expansion_DebugObjects)
		{
			GetGame().ObjectDelete(obj);
		}

		m_Expansion_DebugObjects.Clear();
	}

	//! Uses head/barrel to determine direction, can be used on server
	//! @note on server, aim direction is always an approximation
	vector Expansion_GetAimDirection()
	{
		vector headTransform[4];
		GetBoneTransformWS(GetBoneIndexByName("head"), headTransform);

		auto hands = GetHumanInventory().GetEntityInHands();

		vector dir;

		if (hands && IsRaised() && hands.IsWeapon())
		{
#ifdef SERVER
			vector ori = GetOrientation();
			vector headOri = headTransform[1].VectorToAngles();
			ori[0] = headOri[0] + 5;
			if (ori[0] > 360)
				ori[0] = ori[0] - 360;
			ori[2] = headOri[2];
			//! @note pitch will only be accurate if looking straight ahead while ADS since it can't follow the barrel,
			//! and would need different adjustment if not ADS
			ori[1] = headOri[1] + 12.5;
			if (ori[1] > 360)
				ori[1] = ori[1] - 360;
			dir = ori.AnglesToVector();
#else
			//! Can only follow barrel on client, selection positions on server are completely off base
			vector weaponTransform[4];
			hands.GetTransform(weaponTransform);
			vector barrel_start = hands.GetSelectionPositionLS("konec hlavne").Multiply4(weaponTransform);
			vector barrel_end = hands.GetSelectionPositionLS("usti hlavne").Multiply4(weaponTransform);
			dir = vector.Direction(barrel_start, barrel_end).Normalized();
#endif
		}
		else
		{
			dir = headTransform[1];
		}

#ifdef SERVER
#ifdef DIAG
		vector neckPosition = GetBonePositionWS(GetBoneIndexByName("neck"));
		Expansion_DebugObject(0, neckPosition, "ExpansionDebugBox_Red", dir);
		Expansion_DebugObject(1, neckPosition + dir, "ExpansionDebugBox", dir, neckPosition);
#endif
#endif

		return dir;
	}

	void Expansion_UpdateBonePositionTimes(float pDt)
	{
		vector headPosition = GetBonePositionMS(GetBoneIndexByName("Head"));
		if (vector.DistanceSq(headPosition, m_Expansion_HeadBonePositionMS) < 0.0004)
		{
			m_Expansion_HeadBoneIdleTime += pDt;
		}
		else
		{
			m_Expansion_HeadBonePositionMS = headPosition;
			m_Expansion_HeadBoneIdleTime = 0;
		}
		vector lHandPosition = GetBonePositionMS(GetBoneIndexByName("LeftHand"));
		if (vector.DistanceSq(lHandPosition, m_Expansion_LHandBonePositionMS) < 0.01)
		{
			m_Expansion_LHandBoneIdleTime += pDt;
		}
		else
		{
			m_Expansion_LHandBonePositionMS = lHandPosition;
			m_Expansion_LHandBoneIdleTime = 0;
		}
		vector rHandPosition = GetBonePositionMS(GetBoneIndexByName("RightHand"));
		if (vector.DistanceSq(rHandPosition, m_Expansion_RHandBonePositionMS) < 0.01)
		{
			m_Expansion_RHandBoneIdleTime += pDt;
		}
		else
		{
			m_Expansion_RHandBonePositionMS = rHandPosition;
			m_Expansion_RHandBoneIdleTime = 0;
		}
		vector lFootPosition = GetBonePositionMS(GetBoneIndexByName("LeftFoot"));
		if (vector.DistanceSq(lFootPosition, m_Expansion_LFootBonePositionMS) < 0.0004)
		{
			m_Expansion_LFootBoneIdleTime += pDt;
		}
		else
		{
			m_Expansion_LFootBonePositionMS = lFootPosition;
			m_Expansion_LFootBoneIdleTime = 0;
		}
		vector rFootPosition = GetBonePositionMS(GetBoneIndexByName("RightFoot"));
		if (vector.DistanceSq(rFootPosition, m_Expansion_RFootBonePositionMS) < 0.0004)
		{
			m_Expansion_RFootBoneIdleTime += pDt;
		}
		else
		{
			m_Expansion_RFootBonePositionMS = rFootPosition;
			m_Expansion_RFootBoneIdleTime = 0;
		}
	}

	bool Expansion_IsAnimationIdle()
	{
		if (m_Expansion_HeadBoneIdleTime < 0.25)
			return false;
		if (m_Expansion_LHandBoneIdleTime < 0.25)
			return false;
		if (m_Expansion_RHandBoneIdleTime < 0.25)
			return false;
		if (m_Expansion_LFootBoneIdleTime < 0.25)
			return false;
		if (m_Expansion_RFootBoneIdleTime < 0.25)
			return false;
		return true;
	}

	int Expansion_GetEntityStorageAdditionalDataVersion()
	{
		return 0;
	}

	void Expansion_OnEntityStorageAdditionalDataSave(ParamsWriteContext ctx)
	{
	}

	bool Expansion_OnEntityStorageAdditionalDataLoad(ParamsWriteContext ctx, int version)
	{
		return true;
	}
};

modded class DayZPlayerCommandDeathCallback
{
	override void OnSimulationEnd()
	{
		EntityAI itemInHands;
		if (GetGame().IsServer())
			itemInHands = m_pPlayer.GetHumanInventory().GetEntityInHands();

		super.OnSimulationEnd();

		if (itemInHands && !m_pPlayer.Expansion_CanBeLooted())
		{
			ExpansionItemBaseModule.SetLootable(itemInHands, false);
			itemInHands.SetLifetimeMax(120);  //! Make sure it despawns quickly when left alone
		}
	}
};
