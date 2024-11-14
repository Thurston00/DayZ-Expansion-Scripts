/**
 * eAIBase.c
 * 
 * Partly based on Enfusion AI Project Copyright 2021 William Bowers
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

enum eAIStance
{
	UNKNOWN = -1,
	ERECT,
	CROUCH,
	PRONE
}

class eAIBase: PlayerBase
{
	const int EAI_COMMANDID_MOVE = 1;

	static bool AI_HANDLEDOORS = true;
	static bool AI_HANDLEVAULTING = true;

	static bool s_eAI_UnlimitedReload;

	private static autoptr array<eAIBase> s_AllAI = new array<eAIBase>();
	static float s_eAI_LastCEUpdateTime;
	static ref set<Object> s_eAI_TakenCoverObjects = new set<Object>;

	private ref eAICallbacks m_eAI_Callbacks = new eAICallbacks(this);

	protected autoptr eAIFSM m_FSM;
	bool m_eAI_IsFightingFSM;
	bool m_eAI_ShouldTakeCover;
	bool m_eAI_UpdatePotentialCoverObjects;

	// Targeting data
	private autoptr array<ref eAITarget> m_eAI_Targets;
	ref eAITarget m_eAI_ItemTargetHistory[4];
	typename m_eAI_LastEngagedTargetType;
	int m_eAI_AcuteDangerTargetCount;
#ifdef DIAG_DEVELOPER
	bool m_eAI_PrintCurrentTarget;
#endif
	private autoptr map<eAITargetInformation, ref eAITargetInformationState> m_eAI_TargetInformationStates;
	float m_ThreatClearedTimeout;
	float m_eAI_CurrentThreatToSelf;
	float m_eAI_PreviousThreatToSelf;
	float m_eAI_CurrentThreatToSelfActive;
	float m_eAI_PreviousThreatToSelfActive;
	float m_eAI_AccuracyMin;
	float m_eAI_AccuracyMax;
	float m_eAI_ThreatDistanceLimit;
	float m_eAI_NoiseInvestigationDistanceLimit;
	float m_eAI_DamageMultiplier;
	bool m_eAI_SyncCurrentTarget;
	int m_eAI_CurrentTarget_NetIDLow;
	int m_eAI_CurrentTarget_NetIDHigh;
	int m_eAI_CurrentTarget_NetIDLowSync;
	int m_eAI_CurrentTarget_NetIDHighSync;
	ref eAINoiseTargetInformation m_eAI_NoiseTargetInfo = new eAINoiseTargetInformation();
	int m_eAI_NoiseTarget;
	private bool m_eAI_HasLOS;
	ref array<ref eAIShot> m_eAI_FiredShots = {};
	float m_eAI_PurgeFiredShotsTick;
	Object m_eAI_HitObject;
	int m_eAI_DiscardedShot_DbgIdx = -8;

	// Command handling
	private ExpansionHumanCommandScript m_eAI_Command;
	private int m_eAI_CurrentCommandID;
	private float m_eAI_CommandTime;
	bool m_eAI_IsSwimming;

	private bool m_eAI_UnconsciousVehicle;

	private Transport m_eAI_Transport;
	private int m_eAI_Transport_SeatIndex;

	private bool m_eAI_Melee;
	private bool m_eAI_MeleeDidHit;
	private int m_eAI_MeleeTime;

	private ref eAIAimingProfile m_AimingProfile;

	private ref eAIActionManager m_eActionManager;
	private ref eAIMeleeCombat m_eMeleeCombat;

	private vector m_eAI_LastMovementCheckTargetPosition;
	private vector m_eAI_LastMovementCheckUnitPosition;
	private float m_eAI_MovementCheckTimeout;

	private bool m_eAI_IsChangingStance;
	private bool m_eAI_ShouldGetUp = true;
	int m_eAI_StancePreference = -1;

	// Position for aiming/looking in the world
	private vector m_eAI_LookPosition_WorldSpace;
	private vector m_eAI_AimPosition_WorldSpace;

	// A direction vector (not YPR) in Model Space, not World Space
	private vector m_eAI_LookRelAngles;
	private vector m_eAI_LookDirectionTarget_ModelSpace;
	private bool m_eAI_LookDirection_Recalculate;

	private vector m_eAI_AimRelAngles;
	protected float m_eAI_AimRelAngleLR;
	protected float m_eAI_AimRelAngleUD;
	private vector m_eAI_AimDirectionTarget_ModelSpace;
	private bool m_eAI_AimDirection_Recalculate;
	private vector m_eAI_AimDirectionPrev;

	private bool m_MovementSpeedActive;
	private int m_MovementSpeed;
	private int m_MovementSpeedLimit = 3;
	private int m_MovementSpeedLimitUnderThreat = 3;
	int m_eAI_RoamingMovementSpeedLimit;
	private bool m_MovementDirectionActive;
	protected bool m_eAI_ResetMovementDirectionActive;
	private float m_MovementDirection;
	private float m_SideStepAngle;
	private bool m_eAI_TurnTargetActive;
	private float m_eAI_TurnTarget;
	bool m_eAI_Halt;

	float m_eAI_FormationPositionUpdateTime;
	vector m_eAI_FormationPosition;
	vector m_eAI_FormationDirection;

	float m_eAI_FormationDirectionUpdateTime;
	float m_eAI_FormationDirectionNextUpdateTime;
	float m_eAI_MovementSpeedPrev;

	bool m_eAI_HasProjectileWeaponInHands;
	private bool m_WeaponRaised;
	private bool m_WeaponRaisedPrev;

	private float m_WeaponRaisedTimer;
	private float m_WeaponLowerTimeout;

	ref array<Weapon_Base> m_eAI_Firearms = {};
	ref array<Weapon_Base> m_eAI_Handguns = {};
	ref array<Weapon_Base> m_eAI_Launchers = {};
	ref array<ItemBase> m_eAI_MeleeWeapons = {};
	ref array<ItemBase> m_eAI_Bandages = {};
	ref array<Magazine> m_eAI_Magazines = {};
	ItemBase m_eAI_BandageToUse;

	ref map<typename, Magazine> m_eAI_EvaluatedFirearmTypes = new map<typename, Magazine>;

	Object m_eAI_SideStepObject;
	private float m_eAI_SideStepTimeout;
	private bool m_eAI_SideStepCancelOnLOS;
	private float m_eAI_PositionOverrideTimeout;

	private int m_eAI_UnlimitedReload;
	bool m_eAI_IsUnlimitedReloadAll;
	private float m_eAI_SniperProneDistanceThreshold;
	int m_eAI_LootingBehavior = eAILootingBehavior.DEFAULT;

	// Path Finding
#ifndef EAI_USE_LEGACY_PATHFINDING
	/*private*/ ref ExpansionPathHandler m_PathFinding;
#else
	/*private*/ ref eAIPathFinding m_PathFinding;
#endif
	bool m_eAI_TargetPositionIsFinal = true;
	bool m_eAI_PositionIsFinal = true;
	int m_eAI_PrevHCCState;
	float m_eAI_SurfaceY;
	ref array<EffectArea> m_eAI_ContaminatedAreas;
	float m_eAI_ContaminatedAreaAvoidanceDirection;
	ref map<int, float> m_eAI_ProtectionLevels;

	private Apple m_DebugTargetApple;
	private vector m_DebugTargetOrientation;

	float m_eAI_UpdateTargetsTick;
	autoptr array<EntityAI> m_eAI_PotentialTargetEntities = new array<EntityAI>();
	ref array<EntityAI> m_eAI_TrackedBodies = {};
	int m_eAI_CurrentPotentialTargetIndex;
	CF_DoublyLinkedNode_WeakRef<PlayerBase> m_eAI_PotentialTargetPlayer;
	float m_eAI_UpdateNearTargetsTime;
	int m_eAI_UpdateNearTargetsCount;
	ref set<Object> m_eAI_PotentialCoverObjects = new set<Object>;
	Object m_eAI_CurrentCoverObject;
	vector m_eAI_CurrentCoverPosition;
	bool m_eAI_IsInCover;
	float m_eAI_FlankAngle;
	float m_eAI_FlankTime;
	float m_eAI_FlankTimeMax;

	int m_Expansion_EmoteID;
	bool m_Expansion_EmoteAutoCancel;
	int m_Expansion_EmoteAutoCancelDelay; //! ms

	int m_eAI_MinTimeTillNextFire;
	int m_eAI_QueuedShots;

	private float m_Expansion_UpdateTime;
	private float m_Expansion_UpdateTimeThreshold = 5.0;

	float m_Expansion_DaylightVisibility = -1;
	float m_Expansion_Visibility = 0.1;
	float m_Expansion_VisibilityDistThreshold = 90.0;
	ref array<ItemBase> m_Expansion_ActiveVisibilityEnhancers = {};
	bool m_Expansion_TriedTurningOnVisibilityEnhancers;

	ref set<Man> m_eAI_InteractingPlayers = new set<Man>;

	bool m_eAI_DespawnOnLoosingAggro;

	//ref IsObjectObstructedCache m_eAI_IsItemObstructedCache = new IsObjectObstructedCache(vector.Zero, 1);
	ref map<EntityAI, bool> m_eAI_ThreatOverride = new map<EntityAI, bool>;
	ItemBase m_eAI_LastDroppedItem;
	ref map<string, float> m_eAI_RecentlyDroppedItems = new map<string, float>;

	ref Timer m_eAI_ClientUpdateTimer;

	static ref ExpansionSoundSet s_eAI_LoveSound01_SoundSet;
	static ref ExpansionSoundSet s_eAI_LoveSound02_SoundSet;

	int m_eAI_Meme;
	int m_eAI_MemeLevel;
	float m_eAI_Lean;
	float m_eAI_LeanTarget;
	
#ifndef DAYZ_1_25	
	vector m_ExTransformPlayer[4];
#endif
	
	ref set<PlayerBase> m_eAI_Spectators = new set<PlayerBase>;

	void eAIBase()
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_0(this, "eAIBase");
#endif

#ifdef CF_DEBUG
		CF_Debug.Create(this);
#endif

		s_AllAI.Insert(this);

		if (GetGame().IsServer())
		{
			ExpansionAISettings settings = GetExpansionSettings().GetAI();
			AI_HANDLEVAULTING = settings.Vaulting;
			m_eAI_MemeLevel = settings.MemeLevel;
			eAI_RandomizeContaminatedAreaAvoidanceDirection();
		}

		SetEventMask(EntityEvent.INIT);

		if (GetGame().IsClient())
		{
			m_eAI_ClientUpdateTimer = new Timer(CALL_CATEGORY_SYSTEM);
			m_eAI_ClientUpdateTimer.Run(1.0 / 30.0, this, "eAI_ClientUpdate", NULL, true);
		}
	}

	static eAIBase Get(int index)
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_1("eAIBase", "Get").Add(index);
#endif
		
		if (index >= s_AllAI.Count())
		{
			return null;
		}

		return s_AllAI[index];
	}

	static array<eAIBase> eAI_GetAll()
	{
		return s_AllAI;
	}

	//! @note Init is called from vanilla PlayerBase ctor, so runs before eAIBase ctor!
	override void Init()
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_0(this, "Init");
#endif

		super.Init();

		m_eAI_Targets = {};
		m_eAI_TargetInformationStates = new map<eAITargetInformation, ref eAITargetInformationState>();

		m_eAI_ContaminatedAreas = {};
		m_eAI_ProtectionLevels = new map<int, float>;

		m_AimingProfile = new eAIAimingProfile(this);

		m_eMeleeCombat = new eAIMeleeCombat(this);

		m_MeleeCombat = m_eMeleeCombat;
		m_MeleeFightLogic = new DayZPlayerMeleeFightLogic_LightHeavy(this);

		m_WeaponManager = new eAIWeaponManager(this);
		m_ShockHandler = new eAIShockHandler(this);

		if (GetGame().IsServer())
		{
			eAI_SetAccuracy(-1, -1);
			eAI_SetThreatDistanceLimit(-1);
			eAI_SetNoiseInvestigationDistanceLimit(-1);
			eAI_SetDamageMultiplier(-1);
			eAI_SetDamageReceivedMultiplier(-1);
			m_eAI_SniperProneDistanceThreshold = GetExpansionSettings().GetAI().SniperProneDistanceThreshold;

#ifndef EAI_USE_LEGACY_PATHFINDING
			m_PathFinding = new ExpansionPathHandler(this);
#else
			m_PathFinding = new eAIPathFinding(this);
#endif

			LoadFSM();

			s_Expansion_AllPlayers.m_OnRemove.Insert(eAI_OnRemovePlayer);

			GetStatWater().Set(GetStatWater().GetMax());
			GetStatEnergy().Set(GetStatEnergy().GetMax());
		}

		LookAtDirection("0 0 1");
		AimAtDirection("0 0 1");

		//! Vehicles mod will set this in PlayerBase::Init if loaded
		if (!m_ExpansionST)
			m_ExpansionST = new ExpansionHumanST(this);

		eAINoiseSystem.SI_OnNoiseAdded.Insert(eAI_OnNoiseEvent);
	}

	override void Expansion_Init()
	{
		if (GetGame().IsServer() && !m_eAI_FactionType)
		{
			m_eAI_FactionType = eAIFactionRaiders;
		}

		super.Expansion_Init();

		RegisterNetSyncVariableBool("m_Expansion_CanBeLooted");
		RegisterNetSyncVariableFloat("m_eAI_AccuracyMin");
		RegisterNetSyncVariableFloat("m_eAI_AccuracyMax");
		//RegisterNetSyncVariableInt("m_eAI_CurrentTarget_NetIDLow");
		//RegisterNetSyncVariableInt("m_eAI_CurrentTarget_NetIDHigh");

		m_Expansion_NetsyncData = new ExpansionNetsyncData(this);

		if (!s_eAI_LoveSound01_SoundSet)
			s_eAI_LoveSound01_SoundSet = ExpansionSoundSet.Register("Expansion_AI_The_Sound_Of_Love_01_SoundSet");
		if (!s_eAI_LoveSound02_SoundSet)
			s_eAI_LoveSound02_SoundSet = ExpansionSoundSet.Register("Expansion_AI_The_Sound_Of_Love_02_SoundSet");
	}

	static void ReloadAllFSM()
	{
		array<eAIBase> ai();
		foreach (auto ai0 : s_AllAI)
		{
			if (!ai0.GetFSM())
			{
				continue;
			}

			ai0.m_FSM = null;

			ai.Insert(ai0);
		}

		ExpansionFSMType.UnloadAll();

		foreach (auto ai1 : ai)
		{
			ai1.LoadFSM();
		}
	}

	void LoadFSM()
	{
		ExpansionFSMType type = ExpansionFSMType.LoadXML("DayZExpansion/AI/scripts/FSM", "Master");
		ExpansionFSMOwnerType owner = this;
		if (type && Class.CastTo(m_FSM, type.Spawn(owner, null)))
		{
			m_FSM.StartDefault();
		}
		else
		{
			CF_Log.Error("Invalid FSM");
		}
	}

	void ~eAIBase()
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_0(this, "~eAIBase");
#endif

		if (!GetGame())
			return;

#ifdef CF_DEBUG
		CF_Debug.Destroy(this);
#endif

		s_AllAI.RemoveItem(this);
	}

	protected override void EOnInit(IEntity other, int extra)
	{
		EXTrace.Print(EXTrace.AI, this, "EOnInit");

		OnSelectPlayer();
	}

	override void OnSelectPlayer()
	{
		EXTrace.Print(EXTrace.AI, this, "OnSelectPlayer");

		m_QuickBarBase.updateSlotsCount();

		m_PlayerSelected = true;

		m_WeaponManager.SortMagazineAfterLoad();

		if (GetGame().IsServer())
		{
			//! add callbacks for ai target system
			SetAITargetCallbacks(new AITargetCallbacksPlayer(this));

			GetSoftSkillsManager().InitSpecialty(GetStatSpecialty().Get());
			GetModifiersManager().SetModifiers(true);

			SetSynchDirty();
		}

		CheckForGag();

		m_eActionManager = new eAIActionManager(this);
		m_ActionManager = m_eActionManager;
	}

	override void OnCEUpdate()
	{
		super.OnCEUpdate();

		int persistentCount = eAIGroup.s_PersistentGroups.Count();

		if (persistentCount > 0)
		{
			int time = GetGame().GetTickTime();

			if (time - s_eAI_LastCEUpdateTime > 3.0)
			{
				int lastPersistIndex = persistentCount - 1;
	
				if (eAIGroup.s_eAI_CurrentPersistIndex > lastPersistIndex)
					eAIGroup.s_eAI_CurrentPersistIndex = lastPersistIndex;
				else if (eAIGroup.s_eAI_CurrentPersistIndex == lastPersistIndex)
					eAIGroup.s_eAI_CurrentPersistIndex = 0;
				else
					eAIGroup.s_eAI_CurrentPersistIndex += 1;

				s_eAI_LastCEUpdateTime = time;
			}
		}

		if (!IsAlive())
			return;

		eAIGroup group = GetGroup();
		if (group && group.m_Persist && group.m_BaseName && group.GetLeader() == this)
		{
			//! Persist one AI group per CE update interval (except if no players online, always persist)

			int idx = eAIGroup.s_PersistentGroups.Find(group);

#ifdef EXTRACE
			auto trace = EXTrace.Start(EXTrace.AI, this, "persistent group '" + group.GetName() + "' index " + idx + " current " + eAIGroup.s_eAI_CurrentPersistIndex);
#endif 
				
			if (idx != eAIGroup.s_eAI_CurrentPersistIndex && PlayerBase.Expansion_GetOnlinePlayersCount() > 0)
				return;

#ifdef EXTRACE
			EXTrace.Add(trace, "PERSIST");
#endif 
			
			group.Save(true);

		//#ifdef DIAG_DEVELOPER
			//ExpansionNotification("EXPANSION AI", "Saved group " + group.GetName()).Info();
		//#endif

		/*
			if (IsAlive())
			{
				string basePath = group.GetStorageDirectory();
				if (FileExist(basePath) || ExpansionStatic.MakeDirectoryRecursive(basePath))
				{
					string path = basePath + m_eAI_GroupMemberID.ToString() + ".bin";
					ExpansionEntityStorageModule.SaveToFile(this, path);
				}
			}
		*/
		}
	}

#ifdef DIAG_DEVELOPER
	ref map<EntityAI, bool> m_eAI_PlayerIsEnemyStatus = new map<EntityAI, bool>;
	void eAI_UpdatePlayerIsEnemyStatus(EntityAI player, bool state, string reason)
	{
		bool currentState;
		if (!m_eAI_PlayerIsEnemyStatus.Find(player, currentState) || state != currentState)
		{
			EXPrint(ToString() + " ::eAI_UpdatePlayerIsEnemyStatus " + GetGroup().GetFaction() + " | " + reason + " | " + player.ToString() + " | is enemy? " + state);
			m_eAI_PlayerIsEnemyStatus[player] = state;
		}
	}
#endif

	/**
	 * @brief check if other player is currently considered an enemy
	 * 
	 * @param other Other player entity
	 * @param track Track movement (will return true also for friendly targets)
	 * @param[out] isPlayerMoving Is other player moving?
	 * @param[out] friendly Are we normally friendly towards other player?
	 * @param[out] targeted Are we currently targeting/targeted by other player? (our threat to them or theirs to us above 0.2)
	 */
	bool PlayerIsEnemy(EntityAI other, bool track = false, out bool isPlayerMoving = false, out bool friendly = false, out bool targeted = false)
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_1(this, "PlayerIsEnemy").Add(other);
#endif

		PlayerBase player;
		if (!Class.CastTo(player, other))
		{
#ifdef DIAG_DEVELOPER
			eAI_UpdatePlayerIsEnemyStatus(player, true, "target not a player");
#endif
			return true;
		}

		if (player.Expansion_GetMovementSpeed() > 0 || player.IsClimbing() || player.IsFalling() || player.IsFighting() || player.IsLeaning())
			isPlayerMoving = true;

		if (GetGroup().GetFaction().IsObserver())
		{
			friendly = true;

			//! Interacting players are always "enemies" to observers (will be looked at)
			if (m_eAI_InteractingPlayers.Find(player) > -1)
				return true;

			//! Look at others if they move
			return isPlayerMoving;
		}

		//! Are we targeting them and aggro?
		if (eAI_GetTargetThreat(player.GetTargetInformation()) > 0.2)
			targeted = true;

		if (player.GetGroup())
		{
			if (player.GetGroup() == GetGroup())
			{
#ifdef DIAG_DEVELOPER
				eAI_UpdatePlayerIsEnemyStatus(player, false, "target has same group");
#endif
				friendly = true;

				if (track && GetExpansionSettings().GetAI().MemeLevel > 0 && isPlayerMoving)
					return true;

				return false;
			}

			//! If we are not targeting them, are they targeting us?
			eAIBase ai;
			float ourThreatToThem;
			if (Class.CastTo(ai, player))
				ourThreatToThem = ai.eAI_GetTargetThreat(GetTargetInformation());
			if (!targeted && !player.GetGroup().GetFaction().IsObserver() && ourThreatToThem > 0.2)
				targeted = true;

			//! Other faction friendly to our faction or this specific AI?
			if (player.GetGroup().GetFaction().IsFriendly(GetGroup().GetFaction()) || player.GetGroup().GetFaction().IsFriendlyEntity(this, player))
			{
#ifdef DIAG_DEVELOPER
				if (eAI_IsPassive())
					eAI_UpdatePlayerIsEnemyStatus(player, targeted, "passive | target is friendly " + player.GetGroup().GetFaction());
				else
					eAI_UpdatePlayerIsEnemyStatus(player, targeted, "target is friendly " + player.GetGroup().GetFaction());
#endif
				friendly = true;

				if (track && GetExpansionSettings().GetAI().MemeLevel > 0 && isPlayerMoving)
					return true;

				return targeted;
			}

			//! If the other player is a guard and we are not, consider them an enemy if they raised their weapon and
			//! either aren't AI or consider us a threat
			if (player.GetGroup().GetFaction().IsGuard())
			{
				bool hostile;
				//! https://feedback.bistudio.com/T173348
				if ((player.IsRaised() || player.IsFighting()) && (!ai || ourThreatToThem >= 0.4))
					hostile = true;
#ifdef DIAG_DEVELOPER
				eAI_UpdatePlayerIsEnemyStatus(player, hostile, "target is guard");
#endif
				return hostile;
			}
		}

		//! Our faction fiendly to specific player?
		if (GetGroup().GetFaction().IsFriendlyEntity(other, this))
		{
#ifdef DIAG_DEVELOPER
			if (eAI_IsPassive())
				eAI_UpdatePlayerIsEnemyStatus(player, targeted, "passive | friendly | target has no group");
			else
				eAI_UpdatePlayerIsEnemyStatus(player, targeted, "friendly");
#endif
			friendly = true;

			if (track && GetExpansionSettings().GetAI().MemeLevel > 0 && isPlayerMoving)
				return true;

			return targeted;
		}

#ifdef EXPANSIONMODGROUPS
		if (GetGame().IsServer())
		{
			PlayerBase leader;
			PlayerBase otherLeader;
			if (player.GetGroup())
				Class.CastTo(otherLeader, player.GetGroup().GetLeader());
			else
				otherLeader = player;
			if (otherLeader && otherLeader.Expansion_GetParty() && Class.CastTo(leader, GetGroup().GetLeader()) && leader.Expansion_GetParty())
			{
				//! If leaders of both AI groups are actual players or other player has no AI group,
				//! check if they are in the same Expansion party, and if so,
				//! AI will not be hostile to the other group/player
				if (leader.Expansion_GetParty() == otherLeader.Expansion_GetParty())
				{
#ifdef DIAG_DEVELOPER
					eAI_UpdatePlayerIsEnemyStatus(player, targeted, "target party is leader party");
#endif
					friendly = true;

					if (track && GetExpansionSettings().GetAI().MemeLevel > 0 && isPlayerMoving)
						return true;

					return targeted;
				}
			}
		}
#endif

		// at this point we know both we and they have groups, and the groups aren't friendly towards each other
#ifdef DIAG_DEVELOPER
		if (player.Expansion_HasAdminToolInvisibility())
			eAI_UpdatePlayerIsEnemyStatus(player, false, "target invisible");
		else
			eAI_UpdatePlayerIsEnemyStatus(player, true, "not friendly");
#endif
		return true;
	}

	void TryFireWeapon()
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_0(this, "TryFireWeapon");
#endif

		if (GetGame().GetTime() < m_eAI_MinTimeTillNextFire)
			return;

		Weapon_Base weapon = Weapon_Base.Cast(GetHumanInventory().GetEntityInHands());
		if (!weapon)
			return;

		if (!eAI_CanFire(weapon))
			return;

		if (!eAI_HasLOS())
			return;

		if (m_eAI_QueuedShots > 0)
		{
			//! Continue firing in current mode until all queued shots fired
		}
		else if (Math.RandomFloat(0.0, 1.0) > 0.40)
		{
			//! Determine firing mode

			float distSq = GetTarget().GetDistanceSq(this, true);

			bool burst;
			bool fullAuto;

			if (distSq < 200)
			{
				if (Math.RandomFloat(0.0, 1.0) > 0.60 || !weapon.Expansion_SetFireMode(ExpansionFireMode.FullAuto))
					burst = weapon.Expansion_SetFireMode(ExpansionFireMode.Burst);
				else
					fullAuto = true;
			}
			else if (distSq < 1500)
			{
				burst = weapon.Expansion_SetFireMode(ExpansionFireMode.Burst);
			}

			if (burst)
			{
				weapon.ResetBurstCount();
				m_eAI_QueuedShots = weapon.GetCurrentModeBurstSize(weapon.GetCurrentMuzzle());
			}
			else if (fullAuto)
			{
				Magazine mag;
				if (Class.CastTo(mag, weapon.GetMagazine(weapon.GetCurrentMuzzle())))
					m_eAI_QueuedShots = Math.Ceil(((200 - distSq) / 200) * Math.Min(mag.GetAmmoMax() / 4, mag.GetAmmoCount()));
			}
			else
			{
				weapon.Expansion_SetFireMode(ExpansionFireMode.SemiAuto);
			}
		}
		else
		{
			weapon.Expansion_SetFireMode(ExpansionFireMode.SemiAuto);
		}

	#ifdef DIAG_DEVELOPER
		EXTrace.Print(EXTrace.AI, this, "Firing, queued shots: " + m_eAI_QueuedShots);
	#endif

		GetWeaponManager().Fire(weapon);

		if (m_eAI_QueuedShots > 0)
			m_eAI_QueuedShots--;

		if (m_eAI_QueuedShots > 0)
			m_eAI_MinTimeTillNextFire = GetGame().GetTime() + weapon.GetReloadTime(weapon.GetCurrentMuzzle()) * 1000;
		else
			m_eAI_MinTimeTillNextFire = GetGame().GetTime() + Math.RandomIntInclusive(200, 300);
	}

	bool eAI_CanFire(Weapon_Base weapon)
	{
		if (IsClimbing() || IsFalling() || IsFighting() || IsSwimming())
			return false;

		if (GetDayZPlayerInventory().IsProcessing())
			return false;

		if (!IsRaised())
			return false;

		if (!weapon.CanFire())
			return false;

		if (GetWeaponManager().IsRunning())
			return false;

		int muzzleIndex = weapon.GetCurrentMuzzle();
		if (!weapon.CanFire(muzzleIndex))
			return false;

		return true;
	}

	bool eAI_ShouldBandage()
	{
		if (IsBleeding() && GetGame().GetTickTime() - m_eAI_LastHitTime > 10 && (m_ContaminatedAreaCount == 0 || m_eAI_ProtectionLevels[DEF_CHEMICAL] >= 5.0) && (m_eAI_CurrentThreatToSelfActive < 0.4 || GetHealth01("", "Blood") < 0.7 || !m_eAI_Targets.Count() || GetTarget().info.IsInherited(eAIItemTargetInformation) || GetTarget().info.IsInherited(eAINoiseTargetInformation)))
			return true;

		return false;
	}

	override void EEHitBy(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		super.EEHitBy(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);

		m_eAI_FlankTime = 0.0;
	}

	override void EEKilled(Object killer)
	{
	#ifdef EXTRACE_DIAG
		auto trace = EXTrace.Start(EXTrace.AI, this, "" + killer);
	#endif

		super.EEKilled(killer);

		if (GetGame().IsServer())
		{
			if (!m_Expansion_EmoteAutoCancel)
				GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(Expansion_PlayEmote);

			eAIGroup group = GetGroup();
			if (group && group.m_Persist && group.m_BaseName)
				eAI_DeletePersistentFiles();
		}
	}

	override void eAI_Cleanup(bool autoDeleteGroup = false)
	{
	#ifdef EXTRACE_DIAG
		auto trace = EXTrace.Start(EXTrace.AI, this);
	#endif

		array<ref eAITargetInformation> targets = {};
		foreach (eAITargetInformation info, eAITargetInformationState state: m_eAI_TargetInformationStates)
		{
			if (info)
				targets.Insert(info);
		}

		foreach (eAITargetInformation target: targets)
		{
			target.RemoveAI(this);
		}

		if (m_eAI_TargetInformationStates.Count())
		{
			eAI_DebugTargetInformationStates();

			m_eAI_TargetInformationStates.Clear();
		}

		super.eAI_Cleanup(autoDeleteGroup);

		if (GetGame().IsServer() && !IsDamageDestroyed())
			s_Expansion_AllPlayers.m_OnRemove.Remove(eAI_OnRemovePlayer);

		if (m_eAI_ClientUpdateTimer && m_eAI_ClientUpdateTimer.IsRunning())
			m_eAI_ClientUpdateTimer.Stop();

		eAINoiseSystem.SI_OnNoiseAdded.Remove(eAI_OnNoiseEvent);

		if (m_eAI_CurrentCoverObject)
			s_eAI_TakenCoverObjects.RemoveItem(m_eAI_CurrentCoverObject);
	}

	void eAI_DebugTargetInformationStates()
	{
		foreach (eAITargetInformation info, eAITargetInformationState state: m_eAI_TargetInformationStates)
		{
			EXPrint(this, "Debug: Not removed during target cleanup " + info.GetDebugName() + " " + state);
		}
	}

	override bool IsAI()
	{
		return true;
	}

#ifndef EAI_USE_LEGACY_PATHFINDING
	ExpansionPathHandler GetPathFinding()
#else
	eAIPathFinding GetPathFinding()
#endif
	{
		return m_PathFinding;
	}

	eAIFSM GetFSM()
	{
		return m_FSM;
	}

	ItemBase GetBandageToUse()
	{
		if (m_eAI_BandageToUse)
		{
			if (m_eAI_BandageToUse.IsDamageDestroyed())
				m_eAI_BandageToUse = null;
			else
				return m_eAI_BandageToUse;
		}

		foreach (ItemBase bandage: m_eAI_Bandages)
		{
			if (bandage && !bandage.IsDamageDestroyed())
			{
				m_eAI_BandageToUse = bandage;
				return bandage;
			}
		}

		return null;
	}

	Weapon_Base eAI_GetAnyWeaponToUse(bool requireAmmo = false, bool preferExplosiveAmmo = false)
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_1(EXTrace.AI, this).Add(requireAmmo);
#endif

		Weapon_Base weapon = eAI_GetWeaponToUse(requireAmmo, preferExplosiveAmmo);
		if (!weapon)
			weapon = eAI_GetWeaponToUse(requireAmmo, !preferExplosiveAmmo);

		return weapon;
	}

	Weapon_Base eAI_GetWeaponToUse(bool requireAmmo = false, bool preferExplosiveAmmo = false)
	{
		if (preferExplosiveAmmo)
			return eAI_GetWeaponToUse(m_eAI_Launchers, requireAmmo);

		Weapon_Base weapon = eAI_GetWeaponToUse(m_eAI_Firearms, requireAmmo);
		if (weapon)
			return weapon;

		return eAI_GetWeaponToUse(m_eAI_Handguns, requireAmmo);
	}

	Weapon_Base eAI_GetWeaponToUse(array<Weapon_Base> weapons, bool requireAmmo = false)
	{
		Magazine mag;

		foreach (Weapon_Base weapon: weapons)
		{
			if (weapon && !weapon.IsDamageDestroyed())
			{
				if (!requireAmmo)
					return weapon;

				if (eAI_HasAmmoForFirearm(weapon, mag, false))
					return weapon;
			}
		}

		return null;
	}

	ItemBase GetMeleeWeaponToUse()
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_0(this, "GetMeleeWeaponToUse");
#endif

		// very messy :)
		foreach (ItemBase melee: m_eAI_MeleeWeapons)
		{
			if (melee && !melee.IsDamageDestroyed())
			{
				return melee;
			}
		}

		return null;
	}

	//! Unlike GetMagazineToReload, this can be used to check if there is a mag/ammo for a weapon that is not in hands
	Magazine eAI_GetMagazineToReload(Weapon_Base weapon)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI, eAIBase, weapon.ToString());
#endif
		
		eAIWeaponManager weapon_manager = eAIWeaponManager.Cast(GetWeaponManager());

		Magazine ammo_magazine; // magazine
		int last_ammo_magazine_count;
		int ammo_magazine_count;

		Magazine ammo_pile; // ammo pile
		int last_ammo_pile_count;
		int ammo_pile_count;

		bool unlimitedReload = eAI_IsTargetUnlimitedReload();

		// Get all magazines in (player) inventory
		foreach (Magazine magazine: m_eAI_Magazines)
		{
			if (!magazine)
				continue;

			ammo_pile_count = magazine.GetAmmoCount();

			// magazines (get magazine with max ammo count)
			if (weapon_manager.CanAttachMagazine_NoHandsCheck(weapon, magazine) || weapon_manager.CanSwapMagazine_NoHandsCheck(weapon, magazine))
			{
				if (ammo_pile_count > 0 || unlimitedReload)
				{
					if (last_ammo_magazine_count == 0)
					{
						ammo_magazine = magazine;
						last_ammo_magazine_count = ammo_pile_count;
					}
					else
					{
						if (last_ammo_magazine_count < ammo_pile_count)
						{
							ammo_magazine = magazine;
							last_ammo_magazine_count = ammo_pile_count;
						}
					}
				}
			}
			// bullets (get ammo pile with min ammo count)
			else if (weapon_manager.CanLoadBullet_NoHandsCheck(weapon, magazine))
			{
				if (ammo_pile_count > 0 || unlimitedReload)
				{
					if (last_ammo_pile_count == 0)
					{
						ammo_pile = magazine;
						last_ammo_pile_count = ammo_pile_count;
					}
					else
					{
						if (last_ammo_pile_count > ammo_pile_count)
						{
							ammo_pile = magazine;
							last_ammo_pile_count = ammo_pile_count;
						}
					}
				}
			}
		}

#ifdef DIAG_DEVELOPER
		if (ammo_magazine)
			EXTrace.Print(EXTrace.AI, this, "eAI_GetMagazineToReload " + ammo_magazine + " ammo count " + last_ammo_magazine_count);
		else
			EXTrace.Print(EXTrace.AI, this, "eAI_GetMagazineToReload " + ammo_pile + " ammo count " + last_ammo_pile_count);
#endif

		// prioritize magazine
		Magazine chosen;
		if (ammo_magazine)
			chosen = ammo_magazine;
		else
			chosen = ammo_pile;

		if (chosen && unlimitedReload)
		{
			chosen.ServerSetAmmoMax();
			chosen.SetSynchDirty();
		}

		return chosen;
	}

	void eAI_SetUnlimitedReload(int unlimitedReload)
	{
		m_eAI_UnlimitedReload = unlimitedReload;

		if (unlimitedReload == eAITargetType.ALL || unlimitedReload == eAITargetType.ALL_OR)
			m_eAI_IsUnlimitedReloadAll = true;
		else
			m_eAI_IsUnlimitedReloadAll = false;
	}

	bool eAI_IsTargetUnlimitedReload()
	{
		if (s_eAI_UnlimitedReload)
			return true;

		if (m_eAI_IsUnlimitedReloadAll)
			return true;

		if (!m_eAI_LastEngagedTargetType)
			return false;

		switch (m_eAI_LastEngagedTargetType)
		{
			case eAICreatureTargetInformation:
				if (m_eAI_UnlimitedReload & eAITargetType.ANIMAL)
					return true;
				break;

			case eAIZombieTargetInformation:
				if (m_eAI_UnlimitedReload & eAITargetType.INFECTED)
					return true;
				break;

			case eAIPlayerTargetInformation:
				if (m_eAI_UnlimitedReload & eAITargetType.PLAYER)
					return true;
				break;

			case eAIVehicleTargetInformation:
				if (m_eAI_UnlimitedReload & eAITargetType.VEHICLE)
					return true;
				break;
		}

		return false;
	}

	void eAI_SetAccuracy(float accuracyMin, float accuracyMax)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI, eAIBase);
		EXTrace.Add(trace, accuracyMin);
		EXTrace.Add(trace, accuracyMax);
#endif 
		
		if (accuracyMin <= 0)
			accuracyMin = GetExpansionSettings().GetAI().AccuracyMin;

		m_eAI_AccuracyMin = accuracyMin;

		if (accuracyMax <= 0)
			accuracyMax = GetExpansionSettings().GetAI().AccuracyMax;

		m_eAI_AccuracyMax = accuracyMax;

#ifdef EXTRACE
		EXTrace.Add(trace, m_eAI_AccuracyMin);
		EXTrace.Add(trace, m_eAI_AccuracyMax);
#endif 

		SetSynchDirty();
	}

	void eAI_SetThreatDistanceLimit(float distance)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI, eAIBase);
		EXTrace.Add(trace, distance);
#endif 

		if (distance <= 0)
			distance = GetExpansionSettings().GetAI().ThreatDistanceLimit;

		m_eAI_ThreatDistanceLimit = distance;

#ifdef EXTRACE
		EXTrace.Add(trace, distance);
#endif 
	}

	void eAI_SetNoiseInvestigationDistanceLimit(float distance)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI, eAIBase);
		EXTrace.Add(trace, distance);
#endif 

		if (distance <= 0)
			distance = GetExpansionSettings().GetAI().NoiseInvestigationDistanceLimit;

		m_eAI_NoiseInvestigationDistanceLimit = distance;

#ifdef EXTRACE
		EXTrace.Add(trace, distance);
#endif 
	}

	void eAI_SetDamageMultiplier(float multiplier)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI, eAIBase);
		EXTrace.Add(trace, multiplier);
#endif 

		if (multiplier <= 0)
			multiplier = GetExpansionSettings().GetAI().DamageMultiplier;

		m_eAI_DamageMultiplier = multiplier;

#ifdef EXTRACE
		EXTrace.Add(trace, multiplier);
#endif 
	}

	void eAI_SetDamageReceivedMultiplier(float multiplier)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI, eAIBase);
		EXTrace.Add(trace, multiplier);
#endif 

		if (multiplier <= 0)
			multiplier = GetExpansionSettings().GetAI().DamageReceivedMultiplier;

		m_eAI_DamageReceivedMultiplier = multiplier;

#ifdef EXTRACE
		EXTrace.Add(trace, multiplier);
#endif 
	}

	void eAI_SetLootingBehavior(int bitmask)
	{
		m_eAI_LootingBehavior = bitmask;
	}

	override bool IsSwimming()
	{
		if (super.IsSwimming())
			return true;

		return m_eAI_IsSwimming;
	}

	override bool IsFighting()
	{
		return m_IsFighting || (m_eAI_MeleeTime > 0 && GetGame().GetTime() - m_eAI_MeleeTime < 1500);
	}

	eAIAimingProfile GetAimingProfile()
	{
		return m_AimingProfile;
	}

	array<ref eAITarget> GetTargets()
	{
		return m_eAI_Targets;
	}

	int TargetCount()
	{
		return m_eAI_Targets.Count();
	}

	eAITarget GetTarget(int index = 0)
	{
		return m_eAI_Targets[index];
	}

	void OnAddTarget(eAITarget target)
	{
		m_eAI_Targets.Insert(target);
		if (m_eAI_Targets.Count() == 1)
			m_eAI_SyncCurrentTarget = true;
#ifdef DIAG_DEVELOPER
		m_eAI_PrintCurrentTarget = true;
		EXTrace.Print(EXTrace.AI, this, "OnAddTarget " + target.GetDebugName() + " - found at time " + target.found_at_time + " - max time " + target.max_time + " - target count " + m_eAI_Targets.Count());
#endif
	}

	void OnRemoveTarget(eAITarget target)
	{
		int removeIndex = m_eAI_Targets.Find(target);
		if (removeIndex >= 0)
		{
			m_eAI_Targets.RemoveOrdered(removeIndex);
			if (removeIndex == 0)
				m_eAI_SyncCurrentTarget = true;
			if (removeIndex < m_eAI_NoiseTarget)
				m_eAI_NoiseTarget--;
		}
		eAI_RemoveTargetInfoState(target.info);
		if (target.info.IsInherited(eAINoiseTargetInformation))
		{
			if (m_eAI_CurrentThreatToSelfActive < 0.4 && !Math.RandomInt(0, 3))
				Expansion_SetEmote(EmoteConstants.ID_EMOTE_SHRUG, true);
			m_eAI_NoiseTargetInfo.SetNoiseParams(null, vector.Zero, 0.0, 0.0, 0.0);
		}
#ifdef DIAG_DEVELOPER
		m_eAI_PrintCurrentTarget = true;
		EXTrace.Print(EXTrace.AI, this, "OnRemoveTarget " + target.GetDebugName() + " - time remaining " + (target.found_at_time + target.max_time - GetGame().GetTime()) + " - target count " + m_eAI_Targets.Count());
#endif
	}

	void eAI_RemoveTargetInfoState(eAITargetInformation info)
	{
#ifdef DIAG_DEVELOPER
		EXTrace.Print(EXTrace.AI, this, "eAI_RemoveTargetInfoState " + info.GetDebugName());
#endif
		m_eAI_TargetInformationStates.Remove(info);
		ItemBase item;
		if (Class.CastTo(item, info.GetEntity()))
		{
			eAI_ThreatOverride(item, false);
		}
	}

	void eAI_UpdateAcuteDangerTargetCount(eAITarget target, int delta)
	{
		switch (target.info.Type())
		{
			case eAICreatureTargetInformation:
			case eAIZombieTargetInformation:
				m_eAI_AcuteDangerTargetCount += delta;
				break;
		}
	}

	bool eAI_IsLowVitals()
	{
		if (GetHealth01("", "") <= 0.5 || GetHealth01("", "Blood") < 0.7)
			return true;

		return false;
	}

	float GetThreatToSelf(bool ignoreLOS = false)
	{
		if (ignoreLOS)
			return m_eAI_CurrentThreatToSelf;

		return m_eAI_CurrentThreatToSelfActive;
	}

	//! @note all targets except item targets (no state)
	float eAI_GetTargetThreat(eAITargetInformation info, bool ignoreLOS = false)
	{
		eAITargetInformationState state;
		if (m_eAI_TargetInformationStates.Find(info, state))
		{
			if (ignoreLOS)
				return state.m_ThreatLevel;

			return state.m_ThreatLevelActive;
		}

		return 0.0;
	}

	void DetermineThreatToSelf(float pDt)
	{
		m_eAI_PreviousThreatToSelf = m_eAI_CurrentThreatToSelf;
		m_eAI_PreviousThreatToSelfActive = m_eAI_CurrentThreatToSelfActive;

		if (m_eAI_Targets.Count() > 0)
		{
			auto target = GetTarget();

			eAITargetInformationState state;
			m_eAI_CurrentThreatToSelf = target.GetThreat(this, state);

			if (!state)
			{
				m_eAI_CurrentThreatToSelfActive = m_eAI_CurrentThreatToSelf;
			}
			else
			{
				m_eAI_CurrentThreatToSelfActive = state.m_ThreatLevelActive;
			}
		}
		else
		{
			m_eAI_CurrentThreatToSelf = 0.0;
			m_eAI_CurrentThreatToSelfActive = 0.0;
		}

		if (m_eAI_PreviousThreatToSelf != m_eAI_CurrentThreatToSelf)
			m_eAI_Callbacks.OnThreatLevelChanged(m_eAI_PreviousThreatToSelf, m_eAI_CurrentThreatToSelf);

		if (m_eAI_PreviousThreatToSelfActive != m_eAI_CurrentThreatToSelfActive)
			m_eAI_Callbacks.OnActiveThreatLevelChanged(m_eAI_PreviousThreatToSelfActive, m_eAI_CurrentThreatToSelfActive);
	}

	/*!
	 * @brief Get state for given target info.
	 * New state will be created if none exists.
	 * 
	 * @param[out] created Will be set to true if new state was created.
	 * 
	 * @return eAITargetInformationState
	 */
	eAITargetInformationState eAI_GetTargetInformationState(eAITargetInformation info, bool initialUpdate = true, out bool created = false)
	{
		eAITargetInformationState state;
		if (!m_eAI_TargetInformationStates.Find(info, state))
		{
#ifdef DIAG_DEVELOPER
			EXTrace.Print(EXTrace.AI, this, "Adding new target info state for " + info.GetDebugName());
#endif
			state = new eAITargetInformationState(this, info, initialUpdate);
			m_eAI_TargetInformationStates[info] = state;
			created = true;
		}

		return state;
	}

	void eAI_SetDespawnOnLoosingAggro(bool state)
	{
		m_eAI_DespawnOnLoosingAggro = state;
	}

	void eAI_Despawn()
	{
		Delete();
	}

	TStringArray eAI_DumpState(PlayerIdentity identity, bool includeGroup = true, string indent = string.Empty)
	{
		bool isAdmin;
		if (GetExpansionSettings().GetAI().IsAdmin(identity))
			isAdmin = true;

		TStringArray report = {};

		report.Insert(indent + string.Format("%1", Debug.GetDebugName(this)));

		if (indent)
		{
			indent.Replace("\\", ".");
			indent.Replace("-", " ");
		}

		report.Insert(indent + string.Format("|- Speed limit normal/combat %1/%2", typename.EnumToString(eAIMovementSpeed, m_MovementSpeedLimit), typename.EnumToString(eAIMovementSpeed, m_MovementSpeedLimitUnderThreat)));
		report.Insert(indent + string.Format("|- Current velocity %1 m/s", ExpansionStatic.FloatToString(Expansion_GetActualVelocity().Length())));

		if (includeGroup)
			report.InsertAll(GetGroup().DumpState(identity, false, indent + "|- "));

		bool isGroupLeader;
		if (GetGroup().GetLeader() == this)
			isGroupLeader = true;
		report.Insert(indent + string.Format("|- Is group leader %1", isGroupLeader.ToString()));

		bool isFormLeader;
		if (GetGroup().GetFormationLeader() == this)
			isFormLeader = true;
		report.Insert(indent + string.Format("|- Is formation leader %1", isFormLeader.ToString()));

		auto fsmState = GetFSM().GetState();
		if (fsmState)
		{
			auto subFSM = fsmState.GetSubFSM();
			ExpansionState subFSMState;
			if (subFSM)
				subFSMState = subFSM.GetState();
			if (subFSMState)
				report.Insert(indent + string.Format("|- FSM state %1->%2", fsmState.GetName(), subFSMState.GetName()));
			else
				report.Insert(indent + string.Format("|- FSM state %1", fsmState.GetName()));
		}

		int pointIdx = m_PathFinding.m_PointIdx;
		if (m_PathFinding.m_Count >= 2)
			pointIdx += 2;

		report.Insert(indent + string.Format("|- Path progression %1/%2", pointIdx, m_PathFinding.m_Count));

		auto hcm = GetCommand_MoveAI();
		if (hcm)
		{
			report.Insert(indent + string.Format("|  |- Current point %1",
												 ExpansionStatic.VectorToString(hcm.GetWaypoint(),
																				ExpansionVectorToString.Plain)));
			report.Insert(indent + string.Format("|  |  \\- 2D distance to AI %1 m", hcm.GetWaypointDistance2D()));
		}

		report.Insert(indent + string.Format("|  |- Endpoint %1",
											 ExpansionStatic.VectorToString(m_PathFinding.GetEnd(),
																			ExpansionVectorToString.Plain)));
		report.Insert(indent + string.Format("|  |  \\- 2D distance to AI %1 m",
											 ExpansionMath.Distance2D(GetPosition(), m_PathFinding.GetEnd())));
		report.Insert(indent + string.Format("|  |- Target position %1 (%2)",
											 ExpansionStatic.VectorToString(m_PathFinding.GetTarget(),
																			ExpansionVectorToString.Plain),
											 ExpansionStatic.VectorToString(m_PathFinding.m_TargetPosition,
																			ExpansionVectorToString.Plain)));
		report.Insert(indent + string.Format("|  |  |- 2D distance to AI %1 m",
											 ExpansionMath.Distance2D(GetPosition(), m_PathFinding.GetTarget())));

		bool reachable;
		if (!m_PathFinding.m_IsUnreachable)
			reachable = true;

		bool targetReachable;
		if (!m_PathFinding.m_IsTargetUnreachable)
			targetReachable = true;

		report.Insert(indent + string.Format("|  |  |- Is reachable %1 (%2)", reachable.ToString(), targetReachable.ToString()));
		report.Insert(indent + string.Format("|  |  \\- Is final %1", m_eAI_TargetPositionIsFinal.ToString()));

		report.Insert(indent + string.Format("|  \\- Is AI position final %1", m_eAI_PositionIsFinal.ToString()));

		if (m_eAI_CurrentCoverObject)
		{
			report.Insert(indent + string.Format("|- Current cover object %1", Debug.GetDebugName(m_eAI_CurrentCoverObject)));
			report.Insert(indent + string.Format("|  |- Position %1",
												 ExpansionStatic.VectorToString(m_eAI_CurrentCoverObject.GetPosition(),
																				ExpansionVectorToString.Plain)));
			report.Insert(indent + string.Format("|  \\- 2D distance to AI %1 m",
												 ExpansionMath.Distance2D(GetPosition(), m_eAI_CurrentCoverObject.GetPosition())));
		}

		if (m_eAI_CurrentCoverPosition != vector.Zero)
		{
			report.Insert(indent + string.Format("|- Current cover position %1",
												 ExpansionStatic.VectorToString(m_eAI_CurrentCoverPosition,
																				ExpansionVectorToString.Plain)));
			report.Insert(indent + string.Format("|  |- 2D distance to AI %1 m", ExpansionMath.Distance2D(GetPosition(), m_eAI_CurrentCoverPosition)));
			report.Insert(indent + string.Format("|  \\- Is AI in cover %1", m_eAI_IsInCover.ToString()));
		}


		vector lookAnglesTarget = m_eAI_LookDirectionTarget_ModelSpace.VectorToAngles();
		string look = string.Format("%1° %2°", ExpansionMath.RelAngle(lookAnglesTarget[0]), ExpansionMath.RelAngle(lookAnglesTarget[1]));
		if (m_eAI_LookDirection_Recalculate)
			look += " (tracking)";
		else
			look += " (static)";

		report.Insert(indent + string.Format("|- Target look H V %1", look));

		vector lookAngles = GetLookDirection().VectorToAngles();
		report.Insert(indent + string.Format("|- Current look H V %1° %2°", ExpansionMath.RelAngle(lookAngles[0]), ExpansionMath.RelAngle(lookAngles[1])));

		vector aimAnglesTarget = m_eAI_AimDirectionTarget_ModelSpace.VectorToAngles();
		string aim = string.Format("%1° %2°", ExpansionMath.RelAngle(aimAnglesTarget[0]), ExpansionMath.RelAngle(aimAnglesTarget[1]));
		if (m_eAI_AimDirection_Recalculate)
			aim += " (tracking)";
		else
			aim += " (static)";

		report.Insert(indent + string.Format("|- Target aim H V %1", aim));

		vector aimAngles = GetAimDirection().VectorToAngles();
		report.Insert(indent + string.Format("|- Current aim H V %1° %2°", ExpansionMath.RelAngle(aimAngles[0]), ExpansionMath.RelAngle(aimAngles[1])));

		report.Insert(indent + string.Format("|- Current absolute threat to AI %1", GetThreatToSelf(true)));
		report.Insert(indent + string.Format("|- Current active threat to AI %1", GetThreatToSelf()));

		report.Insert(indent + string.Format("|- Tracked/stateful targets %1/%2", m_eAI_Targets.Count(), m_eAI_TargetInformationStates.Count()));

		array<ref eAITarget> targets = {};
		ExpansionArray<eAITarget>.RefCopy(m_eAI_Targets, targets);

		foreach (eAITargetInformation info, eAITargetInformationState state: m_eAI_TargetInformationStates)
		{
			bool tracked = false;

			foreach (eAITarget trackedTarget: targets)
			{
				if (trackedTarget.info == info)
				{
					tracked = true;
					break;
				}
			}

			if (!tracked)
				targets.Insert(new eAITarget(GetGroup(), -1, -1, info));
		}

		foreach (int i, eAITarget target: targets)
		{
			if (!target)
				continue;

			DayZPlayerImplement player = null;
			ItemBase item = null;

			string targetInfo = target.info.ClassName().Substring(3, target.info.ClassName().Length() - 20);
			if (target.GetEntity())
			{
				if (Class.CastTo(player, target.GetEntity()) && player.GetIdentity())
					targetInfo += " " + player.GetIdentity().GetName();
				else
					targetInfo += " " + Debug.GetDebugName(target.GetEntity());

				if (target.found_at_time < 0)
					targetInfo += " (untracked)";

				Class.CastTo(item, target.GetEntity());
			}

			report.Insert(indent + string.Format("|- Target %1 %2", i, targetInfo));

			if (player)
			{
				if (player.GetGroup())
					//report.InsertAll(player.GetGroup().DumpState(identity, false, indent + "|  |- "));
					report.Insert(indent + "|  |- " + string.Format("Group ID %1 %2", player.GetGroup().GetID(), player.GetGroup().GetName()));

				bool moving = false;
				bool friendly = false;
				bool targeted = false;
				bool enemy = PlayerIsEnemy(player, false, moving, friendly, targeted);
				string standing;
				if (!enemy || (friendly && !targeted))
					standing = "neutral";
				else
					standing = "enemy";
				report.Insert(indent + string.Format("|  |- Is seen as %1", standing));
				if (isAdmin)
					report.Insert(indent + string.Format("|  |- Is moving %1", moving.ToString()));
				report.Insert(indent + string.Format("|  |- Can be friendly %1", friendly.ToString()));
				report.Insert(indent + string.Format("|  |- Is targeted %1", targeted.ToString()));
			}
			else if (item)
			{
				bool hasFreeInvLoc = eAI_FindFreeInventoryLocationFor(item);
				report.Insert(indent + string.Format("|  |- Fits in AI inventory %1", hasFreeInvLoc.ToString()));
				bool isIgnoredItem = eAI_GetThreatOverride(item);
				report.Insert(indent + string.Format("|  |- Is ignored item %1", isIgnoredItem.ToString()));
			}

			eAITargetInformationState targetInfoState = null;
			report.Insert(indent + string.Format("|  |- Absolute threat to AI %1", target.GetThreat(this, targetInfoState)));

			if (targetInfoState)
			{
				report.Insert(indent + string.Format("|  |- Active threat to AI %1", targetInfoState.m_ThreatLevelActive));

				string losState = targetInfoState.m_LOS.ToString();
				if (i > 0)
					losState += " (stale)";

				report.Insert(indent + string.Format("|  |- LOS state %1", losState));
			}

			if (isAdmin)
			{
			#ifdef DIAG_DEVELOPER
				if (targetInfoState)
				{
					report.Insert(indent + string.Format("|  |- LOS ray hit position %1", ExpansionStatic.VectorToString(targetInfoState.m_LOSRaycastHitPosition, ExpansionVectorToString.Plain)));
					report.Insert(indent + string.Format("|  |- LOS ray hit object %1", Debug.GetDebugName(targetInfoState.m_LOSRaycastHitObject)));
				}
			#endif

				report.Insert(indent + string.Format("|  |- Actual position %1", ExpansionStatic.VectorToString(target.GetPosition(this, true), ExpansionVectorToString.Plain)));
				report.Insert(indent + string.Format("|  |- Actual distance to AI %1 m", target.GetDistance(this, true)));

				if (targetInfoState)
				{
					report.Insert(indent + string.Format("|  |- Search position %1", ExpansionStatic.VectorToString(target.GetPosition(this), ExpansionVectorToString.Plain)));
					report.Insert(indent + string.Format("|  |- Search distance to AI %1 m", target.GetDistance(this)));
				}
			}

			if (target.found_at_time > -1)
			{
				string foundAtTime = ExpansionStatic.FormatTime(GetDayZGame().ExpansionGetStartTimestamp(true) + target.found_at_time * 0.001, true, true, true);
				report.Insert(indent + string.Format("|  |- Found at time %1 UTC", foundAtTime));
				report.Insert(indent + string.Format("|  \\- Max time %1s", target.max_time * 0.001));
			}
			else
			{
				eAI_FixupLastReportEntry(report);
			}
		}

		report.Insert(indent + string.Format("|- Water %1%% Energy %2%% Heat %3%%", GetStatWater().Get() / GetStatWater().GetMax() * 100, GetStatEnergy().Get() / GetStatEnergy().GetMax() * 100, GetStatHeatComfort().Get() / GetStatHeatComfort().GetMax() * 100));
		report.Insert(indent + string.Format("|- Health %1%% Blood %2%% Shock %3%%", GetHealth01() * 100, GetHealth01("", "Blood") * 100, GetHealth01("", "Shock") * 100));
		report.Insert(indent + string.Format("|- Bleeding %1", GetBleedingSourceCount()));

		bool hasBrokenLegs;
		if (GetBrokenLegs() == eBrokenLegs.BROKEN_LEGS)
			hasBrokenLegs = true;

		report.Insert(indent + string.Format("|- Broken legs %1", hasBrokenLegs.ToString()));

		auto entityInHands = GetHumanInventory().GetEntityInHands();
		string itemName;
		if (entityInHands)
			itemName = Debug.GetDebugName(entityInHands);
		else
			itemName = "NONE";
		report.Insert(indent + string.Format("|- Item in hands %1", itemName));

		eAI_WeaponReport(m_eAI_Firearms, report, indent);
		eAI_WeaponReport(m_eAI_Handguns, report, indent);
		eAI_WeaponReport(m_eAI_Launchers, report, indent);

		report.Insert(indent + string.Format("|- Firearm types and mags to reload %1", m_eAI_EvaluatedFirearmTypes.Count()));
		foreach (typename weaponType, Magazine magToReload: m_eAI_EvaluatedFirearmTypes)
		{
			report.Insert(indent + string.Format("|  |- %1 -> %2", weaponType, magToReload));
		}
		eAI_FixupLastReportEntry(report);

		array<EntityAI> cargoItems = MiscGameplayFunctions.Expansion_GetCargoItems(this);
		report.Insert(indent + string.Format("\\- Items in cargo %1", cargoItems.Count()));

		Magazine mag;
		ItemBase cargoItemBase;
		foreach (EntityAI cargoItem: cargoItems)
		{
			if (Class.CastTo(mag, cargoItem))
				report.Insert(indent + string.Format(".  |- %1 (%2/%3)", cargoItem.GetType(), mag.GetAmmoCount(), mag.GetAmmoMax()));
			else if (Class.CastTo(cargoItemBase, cargoItem) && cargoItemBase.HasQuantity())
				report.Insert(indent + string.Format(".  |- %1 (%2/%3)", cargoItem.GetType(), cargoItem.GetQuantity(), cargoItem.GetQuantityMax()));
			else
				report.Insert(indent + string.Format(".  |- %1", cargoItem.GetType()));
		}
		eAI_FixupLastReportEntry(report);

		return report;
	}

	void eAI_WeaponReport(array<Weapon_Base> weapons, TStringArray report, string indent = string.Empty)
	{
		foreach (Weapon_Base weapon: weapons)
		{
			report.Insert(indent + string.Format("|- Weapon %1", Debug.GetDebugName(weapon)));

			int mi = weapon.GetCurrentMuzzle();

			report.Insert(indent + string.Format("|  |- Is chambered %1", weapon.Expansion_IsChambered(mi).ToString()));

			Magazine attachedMag = null;
			int ammoCount = weapon.Expansion_GetMagazineAmmoCount(mi, attachedMag);

			if (attachedMag)
				report.Insert(indent + string.Format("|  |- Has attached mag %1 (%2/%3)", attachedMag.GetType(), ammoCount, attachedMag.GetAmmoMax()));
			else if (weapon.GetMagazineTypeCount(mi) > 0)
				report.Insert(indent + "|  |- Has no attached mag");
			else if (weapon.HasInternalMagazine(mi))
				report.Insert(indent + string.Format("|  |- Has internal mag (%1/%2)", ammoCount, weapon.GetInternalMagazineMaxCartridgeCount(mi)));

			Magazine mag = null;
			if (m_eAI_EvaluatedFirearmTypes.Find(weapon.Type(), mag) && mag && mag != attachedMag && mag.GetAmmoCount() > 0)
				report.Insert(indent + string.Format("|  \\- Has ammo/mag to reload %1 (%2/%3)", mag.GetType(), mag.GetAmmoCount(), mag.GetAmmoMax()));
			else
				report.Insert(indent + "|  \\- Has no ammo/mag to reload");
		}
	}

	void eAI_FixupLastReportEntry(TStringArray report)
	{
		int index = report.Count() - 1;
		string last = report[index];
		last.Replace("|-", "\\-");
		report[index] = last;
	}

	void ReactToThreatChange(float pDt, EntityAI entityInHands = null)
	{
		if (m_eAI_CurrentThreatToSelf >= 0.2)
		{
			if (m_eAI_PreviousThreatToSelf < 0.2)
				EXTrace.Print(EXTrace.AI, this, "current threat to self >= 0.2 (active " + m_eAI_CurrentThreatToSelfActive + ")");

			if (!m_Expansion_DaylightVisibility && m_eAI_CurrentThreatToSelfActive >= 0.4 && m_eAI_PreviousThreatToSelfActive < 0.4 && !m_Expansion_ActiveVisibilityEnhancers.Count())
			{
				float nightVisibility;
				Expansion_TryTurningOnAnyLightsOrNVG(nightVisibility, false, true);  //! Switch on lights at night (skip NVG)
			}
		}
		else
		{
			if (m_eAI_PreviousThreatToSelf >= 0.2)
			{
				EXTrace.Print(EXTrace.AI, this, "current threat to self < 0.2 (active " + m_eAI_CurrentThreatToSelfActive + ")");

				if (!m_Expansion_DaylightVisibility && m_Expansion_ActiveVisibilityEnhancers.Count())
					Expansion_TryTurningOffAnyLightsOrNVG(true);  //! Switch off any lights (skip NVG)
			}
		}

		if (m_eAI_CurrentThreatToSelfActive >= 0.4)
		{
			if (m_eAI_PreviousThreatToSelfActive < 0.4)
				EXTrace.Print(EXTrace.AI, this, "current active threat to self >= 0.4 (" + m_eAI_CurrentThreatToSelfActive + ")");

			if (m_ThreatClearedTimeout <= 0)
				m_ThreatClearedTimeout = Math.RandomFloat(1, 3);
		}
		else
		{
			if (m_eAI_PreviousThreatToSelfActive >= 0.4)
				EXTrace.Print(EXTrace.AI, this, "current active threat to self < 0.4 (" + m_eAI_CurrentThreatToSelfActive + ")");

			if (!m_eAI_Meme && m_eAI_CurrentThreatToSelfActive > 0.15 && m_eAI_PreviousThreatToSelfActive < 0.15 && !Math.RandomInt(0, 3))
				m_eAI_Meme = Math.RandomInt(2, 4);

			if (m_ThreatClearedTimeout > 0)
			{
				m_ThreatClearedTimeout -= pDt;
				if (m_ThreatClearedTimeout <= 0)
				{
					if (m_eAI_DespawnOnLoosingAggro)
					{
						eAI_Despawn();
					}
					else if (m_eAI_CurrentThreatToSelf <= 0.2 && !GetExpansionSettings().GetAI().Manners)
					{
						int maxEmote;

						if (Expansion_GetMovementSpeed() == 0.0 && Math.RandomInt(0, 101) <= 10)  //! 10% chance to dance
							maxEmote = 5;
						else
							maxEmote = 4;

						int emoteId;
						switch (Math.RandomIntInclusive(1, maxEmote))
						{
							case 0:
								emoteId = EmoteConstants.ID_EMOTE_TAUNT;
								break;
							case 1:
								emoteId = EmoteConstants.ID_EMOTE_TAUNTELBOW;
								break;
							case 2:
								emoteId = EmoteConstants.ID_EMOTE_THROAT;
								break;
							case 3:
								emoteId = EmoteConstants.ID_EMOTE_DABBING;
								break;
							case 4:
								emoteId = EmoteConstants.ID_EMOTE_CLAP;
								break;
							case 5:
								emoteId = EmoteConstants.ID_EMOTE_DANCE;
								break;
						}
						//EXPrint(ToString() + " can play emote? " + m_EmoteManager.CanPlayEmote(emoteId));
						Expansion_SetEmote(emoteId, true);
					}
				}
			}
			else if (m_eAI_PreviousThreatToSelf < 0.15 && m_eAI_CurrentThreatToSelf >= 0.15 && m_eAI_CurrentThreatToSelf < 0.2 && !m_Expansion_EmoteID && Math.RandomFloat(0.0, 1.0) < 0.05)
			{
				eAI_RandomGreeting(true);
			}
		}

		if (IsRaised() && !GetWeaponManager().IsRunning())
		{
			//! CanRaiseWeapon will return false if getting out of way of vehicle so we are able to lower weapon instantly to move faster
			if (!CanRaiseWeapon())
			{
				RaiseWeapon(false);
			}
			else if (m_eAI_CurrentThreatToSelfActive < 0.2 && !m_eAI_IsInCover)
			{
				if (!entityInHands || !entityInHands.IsWeapon())
				{
					m_WeaponLowerTimeout = 0.0;
					RaiseWeapon(false);
				}
				else
				{
					if (m_eAI_PreviousThreatToSelfActive >= 0.2)
						m_WeaponLowerTimeout = Math.RandomFloat(15.0, 30.0);

					m_WeaponLowerTimeout -= pDt;
			
					if (m_WeaponLowerTimeout <= 0)
						RaiseWeapon(false);
				}
			}
		}
	}

	void UpdateTargets(float pDt, EntityAI entityInHands = null)
	{
#ifdef EXTRACE_DIAG
		auto trace = EXTrace.Profile(EXTrace.AI, eAIBase);
#endif

#ifdef EAI_TRACE
		int ticks;
		int elapsed;
#endif
		// TODO: use particle system instead
		// XXX: I have no clue what the above comment means -lava76

		eAIGroup group = GetGroup();
		if (!group)  //! This should never happen
			return;

		if (eAI_IsPassive())
			return;

		int i;

		eAIFaction faction = group.GetFaction();

		vector center = GetPosition();

		m_eAI_UpdateTargetsTick += pDt;
		if (m_eAI_CurrentPotentialTargetIndex >= m_eAI_PotentialTargetEntities.Count() && m_eAI_UpdateTargetsTick > Math.RandomFloat(0.1, 0.2))
		{
			m_eAI_UpdateTargetsTick = 0;
			m_eAI_CurrentPotentialTargetIndex = 0;

			//! Get objects in near range (30 m)

#ifdef EAI_TRACE
			ticks = TickCount(0);
#endif

			vector min = Vector(center[0] - 30, center[1] - 30, center[2] - 30);
			vector max = Vector(center[0] + 30, center[1] + 30, center[2] + 30);

			bool updateCoverObjects;

			if (m_eAI_ShouldTakeCover)
				updateCoverObjects = true;
			else if (group.GetWaypointBehaviour() == eAIWaypointBehavior.ROAMING && group.GetFormationLeader() == this)
				updateCoverObjects = true;

			int flags = QueryFlags.DYNAMIC;
			if (updateCoverObjects && (m_eAI_UpdateNearTargetsCount % 33 == 0 || m_eAI_UpdatePotentialCoverObjects))
			{
				m_eAI_PotentialCoverObjects.Clear();
				//flags |= QueryFlags.STATIC;
				DayZPlayerUtils.PhysicsGetEntitiesInBox(min, max, m_eAI_PotentialTargetEntities);
				m_eAI_UpdatePotentialCoverObjects = false;
				for (i = m_eAI_PotentialTargetEntities.Count() - 1; i >= 0; i--)
				{
					EntityAI potentialCoverObj = m_eAI_PotentialTargetEntities[i];

					bool isPotentialCoverObject = false;

					if (potentialCoverObj.IsBuilding())
					{
						string type = potentialCoverObj.GetType();
						if ((type.IndexOf("Land_") == 0 || type.IndexOf("Wreck_") == 0) && type.IndexOf("Gate") == -1)
						{
						#ifdef DIAG_DEVELOPER
							EXTrace.Print(EXTrace.AI, this, "Potential cover object (building): " + type +  " " + potentialCoverObj);
						#endif
							isPotentialCoverObject = true;
						}
					}
					else if (!m_eAI_ShouldTakeCover)
					{
						//! Ignore all non-building objects if not seeking cover
						continue;
					}
					else if (potentialCoverObj.IsRock())
					{
						//! @note rocks are not picked up by SceneGetEntitiesInBox even with QueryFlags.STATIC
					#ifdef DIAG_DEVELOPER
						EXTrace.Print(EXTrace.AI, this, "Potential cover object (rock): " + potentialCoverObj);
					#endif
						isPotentialCoverObject = true;
					}
					else if (potentialCoverObj.IsTree())
					{
						//! @note trees are not picked up by SceneGetEntitiesInBox even with QueryFlags.STATIC
					#ifdef DIAG_DEVELOPER
						EXTrace.Print(EXTrace.AI, this, "Potential cover object (tree): " + potentialCoverObj);
					#endif
						isPotentialCoverObject = true;
					}
					else if (potentialCoverObj.IsTransport())
					{
					#ifdef DIAG_DEVELOPER
						EXTrace.Print(EXTrace.AI, this, "Potential cover object (transport): " + potentialCoverObj);
					#endif
						isPotentialCoverObject = true;
					}

					if (isPotentialCoverObject)
					{
						m_eAI_PotentialCoverObjects.Insert(potentialCoverObj);
						//! Remove from potential target entities
						m_eAI_PotentialTargetEntities.Remove(i);
					}
				}
			}
			else
			{
				DayZPlayerUtils.SceneGetEntitiesInBox(min, max, m_eAI_PotentialTargetEntities, flags);
			}

			m_eAI_UpdateNearTargetsCount++;

#ifdef EAI_TRACE
			elapsed = TickCount(ticks);
			float time = elapsed / 10000.0;
			m_eAI_UpdateNearTargetsTime += time;
			float timeAvg = m_eAI_UpdateNearTargetsTime / m_eAI_UpdateNearTargetsCount;

			EXTrace.Add(trace, "objects in near range " + m_eAI_PotentialTargetEntities.Count() + " time (ms) " + time + " timeAvg (ms) " + timeAvg);
#endif
		}

		//! Get other players/AI in extended range (1000 m radius) - check one player per tick (30 players per second)

#ifdef EAI_TRACE
		ticks = TickCount(0);
#endif

		if (!faction.IsObserver())  //! Observers only react to near players
		{
			if (!m_eAI_PotentialTargetPlayer)
				m_eAI_PotentialTargetPlayer = s_Expansion_AllPlayers.m_Head;

			PlayerBase player = m_eAI_PotentialTargetPlayer.m_Value;

			EntityAI playerEntity = player;
			if (player && player != this && m_eAI_PotentialTargetEntities.Find(playerEntity) == -1 && Math.IsPointInCircle(center, 1000, player.GetPosition()))
			{
				m_eAI_PotentialTargetEntities.Insert(playerEntity);

#ifdef EAI_TRACE
				elapsed = TickCount(ticks);

				EXTrace.Add(trace, "player/AI in extended range " + player.GetType() + " time (ms) " + (elapsed / 10000.0).ToString());
#endif
			}

			m_eAI_PotentialTargetPlayer = m_eAI_PotentialTargetPlayer.m_Next;
		}

		PlayerBase playerThreat;
		eAIBase ai;
		ItemBase targetItem;
		Weapon_Base targetWeapon;
		Magazine mag;

		Weapon_Base fireArm;
		ItemBase itemInHands;
		Class.CastTo(itemInHands, entityInHands);
		bool hasFirearmWithAmmo;
		bool hasMeleeWeapon;

		if (!Class.CastTo(fireArm, entityInHands))
			fireArm = eAI_GetAnyWeaponToUse(false);

		if (fireArm)
		{
			if (fireArm.Expansion_HasAmmo())
				hasFirearmWithAmmo = true;
		}
		else if ((itemInHands && itemInHands.Expansion_IsMeleeWeapon()) || GetMeleeWeaponToUse())
		{
			hasMeleeWeapon = true;
		}

		ItemBase bandage;
		if (IsBleeding())
			bandage = GetBandageToUse();

		float group_count = group.Count();
		bool playerIsEnemy;

		while (m_eAI_CurrentPotentialTargetIndex < m_eAI_PotentialTargetEntities.Count())
		{
			EntityAI entity = m_eAI_PotentialTargetEntities[m_eAI_CurrentPotentialTargetIndex++];
			if (!entity)
				continue;

			if (Class.CastTo(playerThreat, entity))
			{
				if (playerThreat == this)
					continue;
				if (playerThreat.eAI_IsPassive())
					continue;
				if (!playerThreat.IsPlayerLoaded())
					continue;
				playerIsEnemy = PlayerIsEnemy(playerThreat, true);
				if (!playerThreat.IsAlive() || (playerThreat.IsUnconscious() && playerThreat.IsAI() && playerIsEnemy))
				{
					if (m_eAI_TrackedBodies.Find(entity) > -1)
						continue;

					m_eAI_TrackedBodies.Insert(entity);

					if (Class.CastTo(ai, playerThreat))
					{
						//! Allow to loot anything from dead/uncon AI

						ExpansionArrayTools<Weapon_Base, EntityAI>.InsertAll_UpCast(ai.m_eAI_Firearms, m_eAI_PotentialTargetEntities);
						ExpansionArrayTools<Weapon_Base, EntityAI>.InsertAll_UpCast(ai.m_eAI_Handguns, m_eAI_PotentialTargetEntities);
						ExpansionArrayTools<Weapon_Base, EntityAI>.InsertAll_UpCast(ai.m_eAI_Launchers, m_eAI_PotentialTargetEntities);
						ExpansionArrayTools<ItemBase, EntityAI>.InsertAll_UpCast(ai.m_eAI_MeleeWeapons, m_eAI_PotentialTargetEntities);
						ExpansionArrayTools<ItemBase, EntityAI>.InsertAll_UpCast(ai.m_eAI_Bandages, m_eAI_PotentialTargetEntities);
						ExpansionArrayTools<Magazine, EntityAI>.InsertAll_UpCast(ai.m_eAI_Magazines, m_eAI_PotentialTargetEntities);

						for (i = 0; i < ai.GetInventory().AttachmentCount(); i++)
						{
							EntityAI attachment = ai.GetInventory().GetAttachmentFromIndex(i);
							if (attachment && attachment.IsClothing())
								m_eAI_PotentialTargetEntities.Insert(attachment);
						}
					}
					else if (group.m_Persist && group.m_BaseName)
					{
						//! Allow to loot shoulder/melee slots from dead players if this AI is in a persistent group

						EntityAI shoulder = playerThreat.FindAttachmentBySlotName("Shoulder");
						if (shoulder)
							m_eAI_PotentialTargetEntities.Insert(shoulder);

						EntityAI melee = playerThreat.FindAttachmentBySlotName("Melee");
						if (melee)
							m_eAI_PotentialTargetEntities.Insert(melee);
					}

					continue;
				}
				if (!playerIsEnemy)
					continue;
			}
			else if (entity.IsInherited(DayZPlayerImplement))
				continue;  //! Ignore non PlayerBase NPCs
			else if (Class.CastTo(targetItem, entity))
			{
				//! If the object is an item, ignore it if any of the following conditions are met.
				//! Note these should be roughly in sync with what's in eAIItemTargetInformation::CalculateThreat,
				//! but the latter can be more specific

				if (targetItem.IsDamageDestroyed() || targetItem.IsSetForDeletion() || eAI_GetThreatOverride(targetItem))
					continue;

				if (targetItem.Expansion_IsDanger())
				{
					if (!Expansion_CanBeDamaged())
						continue;
				}
				else
				{
					//! Check if we're interested in the item

					bool shouldPickupBandage = false;
					bool isWeaponOrNonEmptyMag = false;
					bool isInterestingClothing = false;

					if (targetItem.IsWeapon())
					{
						if (faction.IsWeaponPickupEnabled())
						{
							if (targetItem.ShootsExplosiveAmmo())
							{
								if ((m_eAI_LootingBehavior & eAILootingBehavior.WEAPONS_LAUNCHERS) == 0)
									continue;
								
							}

							if ((m_eAI_LootingBehavior & eAILootingBehavior.WEAPONS_FIREARMS) == 0)
							{
								continue;
							}

							if (eAI_WasItemRecentlyDropped(targetItem))
							{
								EXPrint(this, "Ignoring weapon because it was recently dropped: " + targetItem + " (type=" + targetItem.GetType() + " health=" + targetItem.GetHealth() + ")");
								eAI_ThreatOverride(targetItem, true);  //! Ignore by overriding threat so we don't have to do above checks again
								continue;
							}

							if (!hasFirearmWithAmmo || (itemInHands && itemInHands.IsWeapon() && (m_eAI_LootingBehavior & eAILootingBehavior.UPGRADE) && eAI_WeaponSelection(itemInHands, targetItem)))
								isWeaponOrNonEmptyMag = true;
						}
					}
					else if (targetItem.Expansion_IsMeleeWeapon())
					{
						if (faction.IsWeaponPickupEnabled() && (m_eAI_LootingBehavior & eAILootingBehavior.WEAPONS_MELEE))
						{
							if (eAI_WasItemRecentlyDropped(targetItem))
							{
								EXPrint(this, "Ignoring melee weapon because it was recently dropped: " + targetItem + " (type=" + targetItem.GetType() + " health=" + targetItem.GetHealth() + ")");
								eAI_ThreatOverride(targetItem, true);  //! Ignore by overriding threat so we don't have to do above checks again
								continue;
							}

							if (!hasFirearmWithAmmo && (!hasMeleeWeapon || (itemInHands && itemInHands.Expansion_IsMeleeWeapon() && (m_eAI_LootingBehavior & eAILootingBehavior.UPGRADE) && itemInHands.Expansion_CompareDPS(targetItem) < 0)))
								isWeaponOrNonEmptyMag = true;
						}
					}
					else if (targetItem.IsMagazine())
					{
						if (faction.IsWeaponPickupEnabled())
						{
							if ((m_eAI_LootingBehavior & eAILootingBehavior.WEAPONS_FIREARMS) == 0)
							{
								if ((m_eAI_LootingBehavior & eAILootingBehavior.WEAPONS_LAUNCHERS) == 0)
									continue;
							}

							if (fireArm && Class.CastTo(mag, targetItem) && mag.GetAmmoCount())
								isWeaponOrNonEmptyMag = true;
						}
					}
					else if (targetItem.Expansion_CanBeUsedToBandage())
					{
						if ((IsBleeding() && !bandage) || (!IsBleeding() && (m_eAI_LootingBehavior & eAILootingBehavior.BANDAGES) && m_eAI_Bandages.Count() < 3))
							shouldPickupBandage = true;
					}
					else if ((m_eAI_LootingBehavior & eAILootingBehavior.CLOTHING) && targetItem.IsClothing())
					{
						//! Ignore hockey mask since it blocks certain helmets from being worn and due to its armor value
						//! will only be swapped out for a different mask when it's ruined
						//! (hockey mask is the only vanilla mask with armor)
						if (targetItem.IsInherited(HockeyMask))
							continue;

						TStringArray inventorySlots = targetItem.Expansion_GetInventorySlots();
						bool canWear;
						ItemBase currentlyWornGear;

						foreach (string slot: inventorySlots)
						{
							if (GetInventory().HasAttachmentSlot(InventorySlots.GetSlotIdFromString(slot)))
							{
								canWear = true;

								if (!Class.CastTo(currentlyWornGear, FindAttachmentBySlotName(slot)))
								{
									//! Found empty slot
									currentlyWornGear = null;  //! null any gear that was found in another slot
									break;
								}
							}
						}

						if (canWear && eAI_ClothingSelection(currentlyWornGear, targetItem))
							isInterestingClothing = true;

						if (!isInterestingClothing)
							eAI_ThreatOverride(targetItem, true);  //! Ignore by overriding threat so we don't have to do above checks again
					}

					if (!isWeaponOrNonEmptyMag && !shouldPickupBandage && !isInterestingClothing)
						continue;
				}
			}
			else if (entity.IsBuilding())
				continue;
			else if (entity.IsInherited(Transport))
				continue;
			else if (faction.IsFriendlyEntity(entity, this))
				continue;

			if (!eAI_ProcessTarget(entity, group))
				continue;

			if (m_eAI_Targets.Count() * 2 > group_count)
				break;
		}

		if (m_eAI_CurrentPotentialTargetIndex >= m_eAI_PotentialTargetEntities.Count())
		{
			m_eAI_PotentialTargetEntities.Clear();
			m_eAI_TrackedBodies.Clear();
		}

		//! Since item target info states are only removed if the item is deleted or picked up,
		//! we periodically purge states to avoid accumulating a lot of stale states over time

		float timestamp = GetGame().GetTickTime();

		i = 0;

		foreach (eAITargetInformation info, eAITargetInformationState state: m_eAI_TargetInformationStates)
		{
			if (!info)
			{
				//! This shouldn't be possible to happen, but let's see...
				EXPrint(this, "Removing target info state without info: #" + i + " " + state);
				m_eAI_TargetInformationStates.RemoveElement(i);
				break;
			}
			//! Purge if out of range
			else if (info.IsInherited(eAIItemTargetInformation) && !Math.IsPointInCircle(center, 500.0, info.GetPosition(this)))
			////! Purge if last threat level update was longer than 120 s ago
			//else if (info.IsInherited(eAIItemTargetInformation) && timestamp - state.m_ThreatLevelUpdateTimestamp > 120.0)
			{
				eAI_RemoveTargetInfoState(info);
				break;
			}

			i++;
			if (i == 50)
				break;
		}
	}

	bool eAI_ProcessTarget(EntityAI entity, eAIGroup group, out eAITargetInformation info = null)
	{
		info = eAITargetInformation.GetTargetInformation(entity);
		if (!info)
			return false;

		if (!info.IsActive())
		{
			if (entity.IsZombie())
			{
				if (m_eAI_TrackedBodies.Find(entity) == -1)
				{
					m_eAI_TrackedBodies.Insert(entity);

					array<EntityAI> cargoItems = MiscGameplayFunctions.Expansion_GetCargoItems(entity, true);
					foreach (EntityAI cargoItem: cargoItems)
					{
						m_eAI_PotentialTargetEntities.Insert(cargoItem);
					}
				}
			}

			return false;
		}

		if (entity.IsItemBase() || entity.IsZombie())
		{
			if (info.ShouldRemove(this))
				return false;
		}
		else if (info.ShouldRemove())
		{
			return false;
		}

		int num_ai_in_group_targetting = 0;
		if (m_eAI_Targets.Count() > 0 && m_eAI_CurrentThreatToSelf >= 0.4 && info.IsTargetted(group, num_ai_in_group_targetting))
		{
			int group_count = group.Count();
			float num_ai_in_group_not_targeting = group_count - num_ai_in_group_targetting;
			if (!num_ai_in_group_not_targeting)
				return false;
			float threatLevel = info.GetThreat(this);
			float frac = num_ai_in_group_not_targeting / group_count;
			if ((frac * threatLevel) < (1.0 / group_count))
				return false;
		}

		info.AddAI(this);

		return true;
	}

	void eAI_OnRemovePlayer(CF_DoublyLinkedNode_WeakRef<PlayerBase> node)
	{
		if (node == m_eAI_PotentialTargetPlayer)
			m_eAI_PotentialTargetPlayer = node.m_Next;
	}

	/**
	 * @brief select between current and compared weapon.
	 * 
	 * @return true if compared weapon was selected
	 */
	bool eAI_WeaponSelection(ItemBase currentItem, ItemBase compareItem)
	{
		Weapon_Base currentWeapon;
		Weapon_Base compareWeapon;

		if (!Class.CastTo(currentWeapon, currentItem) || !Class.CastTo(compareWeapon, compareItem))
			return false;

		ExpansionWeaponType currentWeaponType = currentWeapon.Expansion_GetWeaponType();
		ExpansionWeaponType compareWeaponType = compareWeapon.Expansion_GetWeaponType();

		if (compareWeaponType == ExpansionWeaponType.Handgun && currentWeaponType != ExpansionWeaponType.Handgun)
			return false;  //! Don't replace non-handgun with handgun

		if (currentWeaponType == ExpansionWeaponType.Marksman && currentWeapon.GetAttachedOptics())
		{
			if (compareWeaponType == ExpansionWeaponType.Marksman)
			{
				if (!compareWeapon.GetAttachedOptics())
					return false; //! Don't replace marksman rifle w/ optics with marksman rifle w/o optics
			}
			else if (compareWeapon.m_Expansion_WeaponInfo.m_AvgDmg <= currentWeapon.m_Expansion_WeaponInfo.m_AvgDmg)
			{
				return false; //! Don't replace marksman rifle w/ optics with non-marksman weapon unless other weapon avg dmg per shot is higher
			}
		}
		else if (compareWeapon.IsInherited(RifleSingleShot_Base) && !currentWeapon.IsInherited(RifleSingleShot_Base))
		{
			return false;  //! Don't replace non-single-shot rifle with single-shot-rifle
		}

		float currentDPS = currentWeapon.Expansion_GetDPS();
		float compareDPS = compareWeapon.Expansion_GetDPS();

		if (currentDPS > 0.0 && compareDPS > 0.0 && currentWeapon.GetInventory().AttachmentCount() < compareWeapon.GetInventory().AttachmentCount())
		{
			if (compareDPS / currentDPS > 0.75)
				return true;
		}
		else if (compareDPS > currentDPS)
		{
			return true;
		}

		return false;
	}

	bool eAI_ClothingSelection(ItemBase currentlyWornGear, ItemBase targetItem)
	{
		if (!currentlyWornGear)
		{
			if ((m_eAI_LootingBehavior & eAILootingBehavior.UPGRADE) == 0)
				return false;

			if (!eAI_WasItemRecentlyDropped(targetItem))
			{
				return true;
			}
			else
			{
				EXPrint(this, "Ignoring item because it was recently dropped: " + targetItem + " (type=" + targetItem.GetType() + " health=" + targetItem.GetHealth() + ")");
				return false;
			}
		}

		if (targetItem.IsInherited(NVGHeadstrap) && targetItem.GetInventory().AttachmentCount() == 0)
			return false;

		if (!currentlyWornGear.IsInherited(HelmetBase) || targetItem.IsInherited(HelmetBase))
		{
			if (currentlyWornGear.IsDamageDestroyed())
			{
				if ((m_eAI_LootingBehavior & eAILootingBehavior.UPGRADE) || targetItem.GetType() == currentlyWornGear.GetType())
					return true;
			}
			else if (m_eAI_LootingBehavior & eAILootingBehavior.UPGRADE)
			{
				//! Is target item better than what we currently have in some capacity?

				CargoBase curCargo = currentlyWornGear.GetInventory().GetCargo();
				CargoBase tgtCargo = targetItem.GetInventory().GetCargo();

				if (curCargo && tgtCargo)
				{
					//! Always prefer larger cargo space which usually also reflects the other attributes of clothing
					//! (military clothing usually has the largest cargo capacity and best protection)
					if (tgtCargo.GetWidth() < curCargo.GetWidth() || tgtCargo.GetHeight() < curCargo.GetHeight())
						return false;
				}

				string curGearProjDmgPath = "CfgVehicles " + currentlyWornGear.GetType() + " DamageSystem GlobalArmor Projectile Health damage";
				string tgtGearProjDmgPath = "CfgVehicles " + targetItem.GetType() + " DamageSystem GlobalArmor Projectile Health damage";

				float curGearProjDmg = 1.0;
				float tgtGearProjDmg = 1.0;

				if (GetGame().ConfigIsExisting(curGearProjDmgPath))
				{
					curGearProjDmg = GetGame().ConfigGetFloat(curGearProjDmgPath);
					EXTrace.Print(EXTrace.AI, this, "Current gear " + currentlyWornGear.GetType() + " armor dmg " + curGearProjDmg);
				}

				if (GetGame().ConfigIsExisting(tgtGearProjDmgPath))
				{
					tgtGearProjDmg = GetGame().ConfigGetFloat(tgtGearProjDmgPath);
					EXTrace.Print(EXTrace.AI, this, "Target gear " + targetItem.GetType() + " armor dmg " + tgtGearProjDmg);
				}

				if (tgtGearProjDmg < curGearProjDmg || (tgtGearProjDmg == curGearProjDmg && targetItem.GetHealth() > currentlyWornGear.GetHealth()))
				{
					if (!eAI_WasItemRecentlyDropped(targetItem))
						return true;
					else
						EXPrint(this, "Ignoring item for swap because it was recently dropped: " + targetItem + " (type=" + targetItem.GetType() + " health=" + targetItem.GetHealth() + ")");
				}
			}
		}

		return false;
	}

	/**
	 * @brief forget recently dropped item
	 * 
	 * @return true if item was previously remembered
	 * 
	 * @note items are remembered by their type name and position (fractional digits rounded to two decimals)
	 */
	bool eAI_ForgetRecentlyDroppedItem(ItemBase item)
	{
		string key = string.Format("%1@%2", item.GetType(), ExpansionStatic.VectorToString(item.GetPosition()));
		if (m_eAI_RecentlyDroppedItems.Contains(key))
		{
			m_eAI_RecentlyDroppedItems.Remove(key);
			return true;
		}

		return false;
	}

	/**
	 * @brief forget items dropped more than 5 minutes ago
	 * 
	 * @return number of stale items removed from memory
	 */
	int eAI_ForgetStaleRecentlyDroppedItems()
	{
		float time = GetGame().GetTickTime();

		TStringArray staleRecentlyDroppedItems = {};

		foreach (string key, float timeDropped: m_eAI_RecentlyDroppedItems)
		{
			if (time - timeDropped > 300.0)
				staleRecentlyDroppedItems.Insert(key);
		}

		foreach (string toRemove: staleRecentlyDroppedItems)
		{
			m_eAI_RecentlyDroppedItems.Remove(toRemove);
		}

		return staleRecentlyDroppedItems.Count();
	}

	/**
	 * @brief remember recently dropped item
	 * 
	 * @note items are remembered by their type name and position (fractional digits rounded to two decimals)
	 */
	void eAI_RememberRecentlyDroppedItem(ItemBase item)
	{
		eAI_ForgetStaleRecentlyDroppedItems();

		string key = string.Format("%1@%2", item.GetType(), ExpansionStatic.VectorToString(item.GetPosition()));
		m_eAI_RecentlyDroppedItems[key] = GetGame().GetTickTime();

		EXTrace.Print(EXTrace.AI, this, "Recently dropped items: " + m_eAI_RecentlyDroppedItems.Count());
	}

	/**
	 * @brief check if item was recently dropped
	 * 
	 * @return true if item was dropped 5 minutes ago or less
	 * 
	 * @note items are remembered by their type name and position (fractional digits rounded to two decimals)
	 */
	bool eAI_WasItemRecentlyDropped(ItemBase item)
	{
		string key = string.Format("%1@%2", item.GetType(), ExpansionStatic.VectorToString(item.GetPosition()));
		float timeDropped;
		if (m_eAI_RecentlyDroppedItems.Find(key, timeDropped) && GetGame().GetTickTime() - timeDropped <= 300.0)
			return true;

		return false;
	}

	override void SetGroup(eAIGroup group, bool autoDeleteFormerGroupIfEmpty = true)
	{
		if (!group)
		{
			Error("Group cannot be NULL");
			return;
		}

		if (GetGroup() == group)
			return;

		//! Since targets are per-group, need to clear if group changed.
		//! m_eAI_Targets can be NULL at this point because it's set in Init which is called from vanilla PlayerBase ctor,
		//! but group is assigned in DayZPlayerImplement ctor
		if (m_eAI_Targets)
		{
			for (int i = m_eAI_Targets.Count() - 1; i >= 0; i--)
			{
				eAITarget target = m_eAI_Targets[i];
				target.RemoveAI(this);
			}
		}

		super.SetGroup(group, autoDeleteFormerGroupIfEmpty);
	}

	bool eAI_RemoveTargets()
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_0(this, "eAI_RemoveTargets");
#endif

#ifdef EXTRACE_DIAG
		auto hitch = new EXHitch(ToString() + "::eAI_RemoveTargets ", 20000);
#endif

		int count = m_eAI_Targets.Count();

		for (int i = count - 1; i >= 0; i--)
		{
			eAITarget target = m_eAI_Targets[i];
			if (target.ShouldRemove(this))
			{
#ifdef DIAG_DEVELOPER
				EXTrace.Print(EXTrace.AI, this, "eAI_RemoveTargets - removing target " + target.GetDebugName());
#endif
				target.RemoveAI(this);
			}
		}

#ifdef DIAG_DEVELOPER
		if (count > 0 && m_eAI_Targets.Count() == 0)
			EXTrace.Print(EXTrace.AI, this, "eAI_RemoveTargets - no more targets");
#endif

		return count != m_eAI_Targets.Count();
	}

	void eAI_PrioritizeTargets()
	{
#ifdef EXTRACE_DIAG
		auto trace = EXTrace.Profile(EXTrace.AI, eAIBase);
#endif

		//! find the target with the highest threat level, no sorting
		//! (sorting performance would be abysmal if there are many targets due to processing n*(n/2)-n/2 targets when comparing threat levels)

		float threat;
		float max_threat = 0.099999;  //! 0.1 is the 'look at' threshold
		float noise_max_threat;
		int max_threat_idx;

		m_eAI_NoiseTarget = 0;
		m_eAI_AcuteDangerTargetCount = 0;

		foreach (int i, eAITarget target: m_eAI_Targets)
		{
			threat = target.GetThreat(this);
			if (threat > max_threat)
			{
				max_threat_idx = i;
				max_threat = threat;
			}
			else if (target.info.IsInherited(eAINoiseTargetInformation) && !IsUnconscious())
			{
				m_eAI_NoiseTarget = i;
			}

			if (threat > 0.152)
				eAI_UpdateAcuteDangerTargetCount(target, 1);

		#ifdef DIAG_DEVELOPER
			if (m_eAI_PrintCurrentTarget)
				EXTrace.Print(EXTrace.AI, this, "eAI_PrioritizeTargets - target " + i + " " + target.GetDebugName() + " threat lvl " + threat);
		#endif
		}

		if (max_threat_idx > 0)
		{
			eAITarget selectedTarget = m_eAI_Targets[max_threat_idx];

			if (selectedTarget.info.IsInherited(eAIItemTargetInformation) && selectedTarget != m_eAI_ItemTargetHistory[0])
			{
				m_eAI_ItemTargetHistory[3] = m_eAI_ItemTargetHistory[2];
				m_eAI_ItemTargetHistory[2] = m_eAI_ItemTargetHistory[1];
				m_eAI_ItemTargetHistory[1] = m_eAI_ItemTargetHistory[0];
				m_eAI_ItemTargetHistory[0] = selectedTarget;

				//! Avoid switching between same two item targets in alternating fashion as it can cause AI
				//! to move between these two items endlessly when it doesn't get close enough for pickup
				if (selectedTarget == m_eAI_ItemTargetHistory[2] && selectedTarget != m_eAI_ItemTargetHistory[1] && m_eAI_ItemTargetHistory[1] == m_eAI_ItemTargetHistory[3])
					return;
			}

			eAITarget previousTarget = m_eAI_Targets[0];
			m_eAI_Targets[0] = selectedTarget;
			if (m_eAI_NoiseTarget == max_threat_idx)
				m_eAI_NoiseTarget = 0;
			else if (previousTarget.info.IsInherited(eAINoiseTargetInformation) && !IsUnconscious())
				m_eAI_NoiseTarget = max_threat_idx;
			m_eAI_Targets[max_threat_idx] = previousTarget;
			m_eAI_SyncCurrentTarget = true;
#ifdef DIAG_DEVELOPER
			m_eAI_PrintCurrentTarget = true;
		}

		if (m_eAI_PrintCurrentTarget && m_eAI_Targets[0])
		{
			m_eAI_PrintCurrentTarget = false;
			EXTrace.Print(EXTrace.AI, this, "eAI_PrioritizeTargets - prioritizing target " + max_threat_idx + " " + m_eAI_Targets[0].GetDebugName() + " threat lvl " + max_threat);
#endif
		}
	}

	void eAI_SyncCurrentTarget()
	{
		m_eAI_SyncCurrentTarget = false;

	#ifdef SERVER
		eAITarget target = GetTarget();

		EntityAI targetEntity;
		if (target)
			targetEntity = target.GetEntity();

		if (targetEntity && !targetEntity.IsItemBase())
		{
			targetEntity.GetNetworkID(m_eAI_CurrentTarget_NetIDLow, m_eAI_CurrentTarget_NetIDHigh);
		}
		else
		{
			m_eAI_CurrentTarget_NetIDLow = 0;
			m_eAI_CurrentTarget_NetIDHigh = 0;
		}

		SetSynchDirty();
	#endif
	}

	void eAI_UpdateCurrentTarget_Client()
	{
		eAITarget target = GetTarget();
		if (target)
			target.RemoveAI(this);
		if (m_eAI_CurrentTarget_NetIDLow == 0 && m_eAI_CurrentTarget_NetIDHigh == 0)
			return;
		Object entity = GetGame().GetObjectByNetworkId(m_eAI_CurrentTarget_NetIDLow, m_eAI_CurrentTarget_NetIDHigh);
		if (!entity)
			return;
		eAITargetInformation info = eAITargetInformation.GetTargetInformation(entity);
		if (!info)
			return;
		target = info.AddAI(this);
		
		EXTrace.Print(EXTrace.AI, this, "Prioritizing target " + info.GetDebugName());
	}

	void eAI_OnNoiseEvent(EntityAI source, vector position, float lifetime, eAINoiseParams params, float strengthMultiplier)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Profile(EXTrace.AI, this);
#endif

		float strength = params.m_Strength * strengthMultiplier;
		if (strength <= 0 || m_eAI_NoiseInvestigationDistanceLimit <= 0 || IsUnconscious())
			return;

		EntityAI root;
		if (source)
		{
			root = source.GetHierarchyRoot();

			if (root == this)
				return;

			if (m_eAI_CurrentThreatToSelfActive >= 0.4)
			{
				eAITarget target = GetTarget();
				if (target && target.GetEntity() == root)
					return;  //! Already targeting the source entity
			}
		}

		//! Friendly checks. Not using PlayerIsEnemy here is intentional.
		//! We ignore noises made by friendlies.
		DayZPlayerImplement player;
		eAIGroup ourGroup = GetGroup();
		eAIFaction faction = ourGroup.GetFaction();
		if (Class.CastTo(player, root))
		{
			eAIGroup theirGroup = player.GetGroup();
			if (theirGroup)
			{
				if (theirGroup == ourGroup || theirGroup.GetFaction().IsFriendly(faction) || theirGroup.GetFaction().IsFriendlyEntity(this))
					return;
			}
			
			//! We ignore the noise if noise source is a player that we have line of sight to
			if (eAI_HasLOS(player.GetTargetInformation()))
				return;
		}

		if (source && position == vector.Zero)
			position = source.GetPosition();

		float strengthSq = strength * strength;
		float distSq = vector.DistanceSq(GetPosition(), position);
		if (distSq > strengthSq)
			return;

		float distance = Math.Sqrt(distSq);

		//! Now, convert strength (range) to threat.
		//! @note noise threat is capped at 0.4 so entity targets can take precedence.
		float threatLevel;
		if (eAI_IsPassive() || faction.IsObserver() || faction.IsGuard())
		{
			threatLevel = ExpansionMath.PowerConversion(0.5, Math.Max(strength, 30.0), distance, 0.152, 0.1, 0.1);
		}
		else
		{
			/*
			if (root && !root.IsItemBase())
			{
				eAITargetInformation info;
				eAI_ProcessTarget(root, ourGroup, info);
				eAITargetInformationState state = eAI_GetTargetInformationState(info);
				if (state.m_ThreatLevel >= 0.4)
				{
					state.m_ThreatLevelActive = state.m_ThreatLevel;
					state.m_SearchPosition = ExpansionMath.GetRandomPointInCircle(position, distance * 0.05);
					state.m_SearchDirection = vector.Direction(GetPosition(), state.m_SearchPosition);
					return;
				}
			}
			*/

			threatLevel = ExpansionMath.LinearConversion(Math.Min(strength, m_eAI_NoiseInvestigationDistanceLimit), strength * 1.1, distance, 0.4, 0.1024);
		}

		//! Update noise target info
		if (threatLevel >= m_eAI_NoiseTargetInfo.GetThreat())
		{
			position = ExpansionMath.GetRandomPointInCircle(position, distance * 0.05);

			if (threatLevel >= 0.4)
				lifetime = ExpansionMath.LinearConversion(0.0, 500.0, distance, 3.0, 240.0);  //! Leave enough time to run there and check it out
			else
				lifetime = Math.RandomFloat(2.0, 4.0);  //! Just look briefly

			m_eAI_NoiseTargetInfo.SetNoiseParams(source, position, strength, lifetime, threatLevel);

			auto state = eAI_GetTargetInformationState(m_eAI_NoiseTargetInfo);
			state.UpdatePosition(true);

			int max_time = lifetime * 1000;
			if (!m_eAI_NoiseTargetInfo.IsTargettedBy(this))
				m_eAI_NoiseTargetInfo.AddAI(this, max_time);
			else
				m_eAI_NoiseTargetInfo.Update(ourGroup, max_time);
		}

	#ifdef DIAG_DEVELOPER
		EXTrace.Print(EXTrace.AI, this, string.Format("::eAI_OnNoiseEvent %1 %2 %3 %4 %5 %6 %7", source, position.ToString(), lifetime, params.m_Strength, strengthMultiplier, params.m_Path, typename.EnumToString(eAINoiseType, params.m_Type)));
	#endif
	}

	override void OnVariablesSynchronized()
	{
		super.OnVariablesSynchronized();

		if (m_eAI_CurrentTarget_NetIDLowSync != m_eAI_CurrentTarget_NetIDLow || m_eAI_CurrentTarget_NetIDHighSync != m_eAI_CurrentTarget_NetIDHigh)
		{
			m_eAI_CurrentTarget_NetIDLowSync = m_eAI_CurrentTarget_NetIDLow;
			m_eAI_CurrentTarget_NetIDHighSync = m_eAI_CurrentTarget_NetIDHigh;
			eAI_UpdateCurrentTarget_Client();
		}
	}

	override void OnInputForRemote(ParamsReadContext ctx)
	{
		int userDataType = 0;
		if (!ctx.Read(userDataType))
		{
			Error("DayZPlayer: OnInputForRemote - cannot read input type");
			return;
		}
		
		switch (userDataType)
		{
			case INPUT_UDT_WEAPON_REMOTE_EVENT:
				if (GetHumanInventory().GetEntityInHands())
					GetDayZPlayerInventory().OnEventForRemoteWeapon(ctx);
				break;
			case INPUT_UDT_HAND_REMOTE_EVENT:
				GetDayZPlayerInventory().OnHandEventForRemote(ctx);
				break;
			case INPUT_UDT_INVENTORY:
				GetDayZPlayerInventory().OnInputUserDataForRemote(ctx);
				break;
			default:
				Error("OnInputForRemote - unknown userDataType=" + userDataType);
				break;
		}
	}

	eAICommandMove GetCommand_MoveAI()
	{
		return eAICommandMove.Cast(GetCommand_Script());
	}

	eAICommandMove StartCommand_MoveAI()
	{
		int stance;
		if (GetCommand_Move() || GetCommand_Action() || GetCommand_Damage())
		{
			stance = m_MovementState.m_iStanceIdx;
			if (m_MovementState.IsRaised())
				stance -= DayZPlayerConstants.STANCEIDX_RAISED;
		}
		EXTrace.Print(EXTrace.AI, this, "StartCommand_MoveAI " + typename.EnumToString(eAIStance, stance) + " (" + stance + ")");
		// WARNING: memory leak
		eAICommandMove cmd = new eAICommandMove(this, m_ExpansionST, stance);
		StartCommand_Script(cmd);
		m_eAI_Command = cmd;
		if (m_WeaponRaised)
			AnimSetBool(m_ExpansionST.m_VAR_Raised, m_WeaponRaised);
		return cmd;
	}

/*
	eAICommandVehicle GetCommand_VehicleAI()
	{
		return eAICommandVehicle.Cast(GetCommand_Script());
	}

	eAICommandVehicle StartCommand_VehicleAI(Transport vehicle, int seatIdx, int seat_anim, bool fromUnconscious = false)
	{
		// WARNING: memory leak
		eAICommandVehicle cmd = new eAICommandVehicle(this, m_ExpansionST, vehicle, seatIdx, seat_anim, fromUnconscious);
		StartCommand_Script(cmd);
		m_eAI_Command = cmd;
		return cmd;
	}
*/

	HumanCommandVehicle GetCommand_VehicleAI()
	{
		return GetCommand_Vehicle();
	}
	HumanCommandVehicle StartCommand_VehicleAI(Transport vehicle, int seatIdx, int seat_anim, bool fromUnconscious = false)
	{
		return StartCommand_Vehicle(vehicle, seatIdx, seat_anim, fromUnconscious);
	}

	void Notify_Transport(Transport vehicle, int seatIndex)
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_2(this, "Notify_Transport").Add(vehicle).Add(seatIndex);
#endif

		m_eAI_Transport = vehicle;
		m_eAI_Transport_SeatIndex = seatIndex;
	}

	Transport eAI_GetTransport()
	{
		return m_eAI_Transport;
	}

	void Notify_Melee()
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_0(this, "Notify_Melee");
#endif

		Expansion_GetUp();

		m_eAI_Melee = true;

		if (!IsFighting() && GetExpansionSettings().GetAI().MemeLevel > 9000)
			s_eAI_LoveSound02_SoundSet.Play(this);
	}

	override void OnCommandMelee2Start()
	{
		super.OnCommandMelee2Start();
		EXTrace.Print(EXTrace.AI, this, "OnCommandMelee2Start");
		m_eAI_MeleeTime = GetGame().GetTime();
	}

	override void EndFighting()
	{
		super.EndFighting();
		EXTrace.Print(EXTrace.AI, this, "EndFighting - melee time " + (GetGame().GetTime() - m_eAI_MeleeTime) + " ms");
	}

	override void OnCommandSwimStart()
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI, this);
#endif 
		
		ItemBase itemInHands = GetItemInHands();
		if (itemInHands)
		{
			if (itemInHands.IsHeavyBehaviour())
			{
				TryHideItemInHands(false);
				eAI_DropItemInHandsImpl(true);
			}
			else
			{
				TryHideItemInHands(true);
			}
			itemInHands.OnItemInHandsPlayerSwimStart(this);
		}
		m_AnimCommandStarting = HumanMoveCommandID.CommandSwim;
		
		AbortWeaponEvent();
		GetWeaponManager().DelayedRefreshAnimationState(10);
		RequestHandAnimationStateRefresh();

		if (!GetGame().IsServer())
			return;

		if (GetGroup().GetFormationLeader() == this)
			GetGroup().EnableSwimming(true);  //! Enable swimming for the whole group so they can follow the leader
		else
			m_PathFinding.EnableSwimming(true);
	}

	override void OnCommandSwimFinish()
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI, this);
#endif 
		
		TryHideItemInHands(false, true);
		
		GetWeaponManager().RefreshAnimationState();

		//auto leader = GetGroup().GetFormationLeader();
		//if (leader == this)
			//GetGroup().EnableSwimming(false);  //! Disable swimming for the whole group (unless they are currently swimming)
		//else if (!leader.IsSwimming())
			//m_PathFinding.EnableSwimming(false);
	}

	override void EEItemAttached(EntityAI item, string slot_name)
	{
		super.EEItemAttached(item, slot_name);

		eAI_UpdateProtectionLevel(DEF_BIOLOGICAL, item, 1);
		eAI_UpdateProtectionLevel(DEF_CHEMICAL, item, 1);
	}

	override void EEItemDetached(EntityAI item, string slot_name)
	{
		super.EEItemDetached(item, slot_name);

		eAI_UpdateProtectionLevel(DEF_BIOLOGICAL, item, -1);
		eAI_UpdateProtectionLevel(DEF_CHEMICAL, item, -1);
	}

	void eAI_UpdateProtectionLevel(int type, EntityAI entity, int value)
	{
		ItemBase item;
		if (Class.CastTo(item, entity))
		{
			int level = m_eAI_ProtectionLevels[type] + item.GetProtectionLevel(type, true) * value;
			m_eAI_ProtectionLevels[type] = level;
		#ifdef DIAG_DEVELOPER
			if (type == DEF_CHEMICAL)
				EXTrace.Print(EXTrace.AI, this, "Chemical protection level: " + level);
			else
				EXTrace.Print(EXTrace.AI, this, "Biological protection level: " + level);
		#endif
		}
	}

	override void Expansion_OnContaminatedAreaEnterServer(EffectArea area, EffectTrigger trigger)
	{
		EXTrace.Print(EXTrace.AI, this, "Expansion_OnContaminatedAreaEnterServer " + area + " " + area.m_Position + " " + area.m_Radius);

		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(eAI_ForgetContaminatedArea);

		if (m_eAI_ContaminatedAreas.Find(area) == -1)
			m_eAI_ContaminatedAreas.Insert(area);
	}

	override void Expansion_OnContaminatedAreaExitServer(EffectArea area, EffectTrigger trigger)
	{
		EXTrace.Print(EXTrace.AI, this, "Expansion_OnContaminatedAreaExitServer " + area + " " + area.m_Position + " " + area.m_Radius);

		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(eAI_ForgetContaminatedArea, 5 * 60 * 1000, false, area);
	}

	void eAI_ForgetContaminatedArea(EffectArea area)
	{
		if (area)
		{
			EXTrace.Print(EXTrace.AI, this, "eAI_ForgetContaminatedArea " + area + " " + area.m_Position + " " + area.m_Radius);

			m_eAI_ContaminatedAreas.RemoveItem(area);
		}
		else
		{
			for (int i = m_eAI_ContaminatedAreas.Count() - 1; i >= 0; i--)
			{
				if (!m_eAI_ContaminatedAreas[i])
					m_eAI_ContaminatedAreas.Remove(i);
			}
		}

		if (m_eAI_ContaminatedAreas.Count() == 0)
			eAI_RandomizeContaminatedAreaAvoidanceDirection();
	}

	void eAI_RandomizeContaminatedAreaAvoidanceDirection()
	{
		if (Math.RandomInt(0, 2))
			m_eAI_ContaminatedAreaAvoidanceDirection = 15.0;
		else
			m_eAI_ContaminatedAreaAvoidanceDirection = -15.0;
	}

	/**
	 * @brief Overrides the desired position to generate a new path
	 *
	 * @param pPosition the target position for path finding
	 */
	void OverrideTargetPosition(vector pPosition, bool isFinal = true, float maxDistance = 1.0, bool allowJumpClimb = true)
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_1(this, "OverrideTargetPosition").Add(pPosition);
#endif

		if (m_eAI_ContaminatedAreas.Count() && Expansion_CanBeDamaged() && m_eAI_ProtectionLevels[DEF_CHEMICAL] < 5.0)
		{
			EffectArea closestArea;
			vector position = GetPosition();
			vector oPos = pPosition;

			if (ContaminatedArea_Base.s_Expansion_MergedClusters.FindClosestPointOutsideAnyCluster(position, pPosition, m_eAI_ContaminatedAreaAvoidanceDirection, closestArea, this))
			{
				pPosition = ExpansionStatic.GetSurfaceRoadPosition(pPosition, RoadSurfaceDetection.CLOSEST);
				isFinal = true;
				Expansion_DebugObject(3142, pPosition, "ExpansionDebugNoticeMe_Cyan");
				if (closestArea.Expansion_IsPointInside(oPos) && !closestArea.Expansion_IsPointInside(position))
				{
					m_PathFinding.m_IsTargetUnreachable = true;
					m_PathFinding.m_IsUnreachable = true;
				}
			}
		}

#ifndef EAI_USE_LEGACY_PATHFINDING
		m_PathFinding.SetTarget(pPosition, maxDistance, allowJumpClimb);
#else
		m_PathFinding.OverridePosition(pPosition);
#endif

		m_eAI_TargetPositionIsFinal = isFinal;
	}

	void eAI_CheckIsInCover()
	{
		if (m_eAI_CurrentCoverObject && Math.IsPointInCircle(GetPosition(), 0.55, m_eAI_CurrentCoverPosition))
		{
			if (!m_eAI_IsInCover)
				m_eAI_Callbacks.OnEnterCover(m_eAI_CurrentCoverObject);
			m_eAI_IsInCover = true;
		}
		else
		{
			if (m_eAI_IsInCover)
				m_eAI_Callbacks.OnLeaveCover(m_eAI_CurrentCoverObject);
			m_eAI_IsInCover = false;
		}
	}

	void OverrideTargetPosition(eAITarget target, bool isFinal = true)
	{
		vector pos;
		vector dir = target.GetDirection(this);

		float dist = dir.Length();
		float minDist = target.GetMinDistance(this, dist);
		bool allowJumpClimb = true;
		bool keepMinDistToTarget;
		bool cannotMelee;
		bool flank;

		if (minDist)
		{
			//! Return early if we are already overriding position and not yet near endpoint
			if (!eAI_ShouldUpdatePosition())
				return;

			keepMinDistToTarget = true;
			//EXTrace.Print(EXTrace.AI, this, "Positioning - min dist " + minDist);
		}
		else
		{
			if (!IsRaised() && target.IsEntity() && !target.CanMeleeIfClose(this))
				cannotMelee = true;

			//! While weapon is raised and we have LOS or action/weapon manager is running (e.g. reload/unjam), reposition/seek cover
			if ((IsRaised() && eAI_HasLOS(target)) || GetActionManager().GetRunningAction() || GetWeaponManager().IsRunning() || cannotMelee)
			{
				allowJumpClimb = false;
				//EXTrace.Print(EXTrace.AI, this, "Positioning - flank time reset " + m_eAI_FlankTime);
				m_eAI_FlankTime = 0.0;
				m_eAI_FlankTimeMax = 0.0;
			}
			else if (eAI_IsInFlankRange(dist) && !eAI_IsDangerousAltitude())
			{
				if (m_eAI_FlankTime <= 0.0)
				{
					if (Math.RandomInt(0, 2))
						m_eAI_FlankAngle = Math.RandomFloat(56.25, 67.5);
					else
						m_eAI_FlankAngle = -Math.RandomFloat(56.25, 67.5);

					m_eAI_FlankTimeMax = Math.RandomFloat(dist * 0.3, dist * 0.6);
					//EXTrace.Print(EXTrace.AI, this, "Positioning - flank angle " + m_eAI_FlankAngle + ", time max " + m_eAI_FlankTimeMax);
				}

				m_eAI_FlankTime += m_dT;

				//! Return early if we are already overriding position and not yet near endpoint and not in cover
				if (m_eAI_FlankTimeMax > 0.0 && !eAI_ShouldUpdatePosition() && !m_eAI_IsInCover)
				{
					//EXTrace.Print(EXTrace.AI, this, "Positioning - flanking, not updating pos tmax=" + m_eAI_FlankTime + " timeout=" + m_eAI_PositionOverrideTimeout + " inCover=" + m_eAI_IsInCover);
					return;
				}

				//! Flank time is based on a random speed estimate.
				//! Sprint = 6.66 m/s avg or factor 0.3, half speed = 3.33 m/s avg or factor 0.6 (lower speed = more time)
				if (m_eAI_FlankTime >= m_eAI_FlankTimeMax)
				{
					//EXTrace.Print(EXTrace.AI, this, "Positioning - flank time exceeded " + m_eAI_FlankTime);

					if ((m_eAI_FlankTime >= m_eAI_FlankTimeMax * 2 && Math.RandomInt(0, 3000) < 1) || m_PathFinding.m_IsUnreachable)
					{
						m_eAI_FlankTime = 0.0;
						Expansion_GetUp();
						return;
					}

					if (m_eAI_IsInCover)
					{
						if (m_eAI_CurrentCoverObject && m_eAI_CurrentCoverObject.IsRock())
							OverrideStance(DayZPlayerConstants.STANCEIDX_PRONE);
						else if (eAI_GetStance() == eAIStance.ERECT)
							OverrideStance(DayZPlayerConstants.STANCEIDX_CROUCH);

						EntityAI hands = GetHumanInventory().GetEntityInHands();
						if (hands && hands.IsWeapon())
							RaiseWeapon(true);

						m_eAI_TargetPositionIsFinal = true;

						return;
					}
				}

				flank = true;
				isFinal = false;
				Expansion_GetUp();
				//EXTrace.Print(EXTrace.AI, this, "Positioning - flanking " + m_eAI_FlankTime);
			}
			else
			{
				//EXTrace.Print(EXTrace.AI, this, "Positioning - dist " + dist);
				m_eAI_FlankTime = 0.0;  //! Allow switch to search if flanking previously
			}
		}

		Weapon_Base weapon;
		if (Class.CastTo(weapon, GetHumanInventory().GetEntityInHands()))
		{
			if (weapon.ShootsExplosiveAmmo())
			{
				minDist = Math.Max(weapon.Expansion_GetMinSafeFiringDistance(), minDist);
				keepMinDistToTarget = true;
			}
		}

		if (!keepMinDistToTarget && !allowJumpClimb)
		{
			if (dist < 3.0 && !cannotMelee)
			{
				//! If the distance to target is already less than 3 m, don't take cover, just try to keep the distance
				minDist = 3.0;  //! Potentially backpedal
			}
			else
			{
				//! Return early if we are already overriding position and not yet near endpoint
				if (!eAI_ShouldUpdatePosition())
					return;

				if (weapon)
				{
					//! Determine min distance range based on zeroing range.
					//! Final min distance will be chosen to be lower or equal to this range.

					ItemOptics optics = weapon.GetAttachedOptics();

					if (optics)
						minDist = optics.GetZeroingDistanceZoomMax();
					else
						minDist = weapon.GetZeroingDistanceZoomMax(weapon.GetCurrentMuzzle()) * 0.3;

					if (minDist > 1000)
						minDist = 1000;
				}

				//! Stay at current distance if lower than min distance range
				//! (roughly, may take cover or reposition to random point if no cover available)
				if (dist <= minDist)
					minDist = dist;
				else if (cannotMelee)
					minDist = 30.0;
			}

			//EXTrace.Print(EXTrace.AI, this, "Positioning - no jump/climb, min dist " + minDist);
		}

		Object currentCoverObj = m_eAI_CurrentCoverObject;

		if (minDist || flank)
		{
			pos = GetPosition();

			if (flank)
			{
				vector ori = dir.VectorToAngles();

				ori[0] = ExpansionMath.RelAngle(ori[0] + m_eAI_FlankAngle);

				dir = ori.AnglesToVector();
			}

			if (m_eAI_CurrentCoverObject)
			{
				s_eAI_TakenCoverObjects.RemoveItem(m_eAI_CurrentCoverObject);
				m_eAI_CurrentCoverObject = null;
			}

			if (m_eAI_PotentialCoverObjects.Count() && (!target.IsMechanicalTrap() || target.IsExplosive()))
			{
				vector aimDirectionTarget = GetAimDirectionTarget();
				vector forwardPos = GetPosition();
				
				vector toObjDir;
				vector toObjDirNorm;
				float dot;
				int coverObjKey;
				float distSq = dist * dist;
				float objDistSq;

				vector targetPos;
				float minDistSq;
				float objToTargetDistSq;

				if (keepMinDistToTarget)
				{
					targetPos = target.GetPosition(this, true);
					minDistSq = minDist * minDist;
				}

				TIntArray distances = {};
				map<int, Object> potentialCoverObjs = new map<int, Object>;

				foreach (Object obj: m_eAI_PotentialCoverObjects)
				{
					if (!obj)
						continue;

					//! Don't try to find cover behind objects with active physics
					if (dBodyIsActive(obj))
						continue;

					if (flank)
					{
						if (obj == currentCoverObj)
							continue;
					}

					if (s_eAI_TakenCoverObjects.Find(obj) > -1)
						continue;

					toObjDir = vector.Direction(forwardPos, obj.GetPosition());
					toObjDirNorm = toObjDir.Normalized();

					objDistSq = toObjDir.LengthSq();

					if (flank)
					{
						dot = vector.Dot(dir, toObjDirNorm);

						//! Select object if in direction of flank angle
						if (dot < 0.75)
							continue;
					}
					else
					{
						dot = vector.Dot(aimDirectionTarget, toObjDirNorm);

						//! Select object if closer than target or if opposite of AI aim direction target
						if ((keepMinDistToTarget || distSq <= objDistSq) && dot >= 0.0)
							continue;
					}

					if (keepMinDistToTarget)
						objToTargetDistSq = vector.DistanceSq(obj.GetPosition(), targetPos);

					if (!keepMinDistToTarget || objToTargetDistSq > minDistSq)
					{
						coverObjKey = Math.Round(objDistSq);
						distances.Insert(coverObjKey);
						potentialCoverObjs[coverObjKey] = obj;
					}
				}

				if (distances.Count())
				{
					//! Select closest potential cover
					distances.Sort();

					coverObjKey = distances[0];

					m_eAI_CurrentCoverObject = potentialCoverObjs[coverObjKey];
					//m_eAI_PotentialCoverObjects.Clear();
					//m_eAI_PotentialCoverObjects.Insert(m_eAI_CurrentCoverObject);
					s_eAI_TakenCoverObjects.Insert(m_eAI_CurrentCoverObject);

				#ifdef DIAG_DEVELOPER
					EXTrace.Print(EXTrace.AI, this, "New cover object selected: " + m_eAI_CurrentCoverObject + " " + m_eAI_CurrentCoverObject.GetPosition());
				#endif

					BuildingBase building;
					bool houseWithDoors;
					float extension;
					vector minMax[2];
					if (m_eAI_CurrentCoverObject.IsTree())
					{
						extension = 2.0;
					}
					else if (Class.CastTo(building, m_eAI_CurrentCoverObject) && building.Expansion_IsEnterable())
					{
						houseWithDoors = true;
					}
					else if (m_eAI_CurrentCoverObject.GetCollisionBox(minMax))
					{
						minMax[0][1] = 0.0;
						minMax[1][1] = 0.0;
						extension = vector.Distance(minMax[0], minMax[1]) * 0.5 + 1.0;
					}
					else
					{
						extension = m_eAI_CurrentCoverObject.ClippingInfo(minMax);
					}

					vector objPos = m_eAI_CurrentCoverObject.GetPosition();

					if (houseWithDoors)
					{
						pos = objPos;
					}
					else
					{
						objPos[1] = m_eAI_AimPosition_WorldSpace[1];
						vector objToTargetDir = vector.Direction(objPos, m_eAI_AimPosition_WorldSpace);
						vector objToTargetDirNorm = objToTargetDir.Normalized();
						pos = objPos - objToTargetDirNorm * extension;
					}

					if (flank)
					{
						m_eAI_FlankTimeMax = Math.RandomFloat(dist * 0.3, dist * 0.6);
						m_eAI_PositionOverrideTimeout = m_eAI_FlankTimeMax;
					}
					else
					{
						m_eAI_PositionOverrideTimeout = 3.0;
					}
				}
			}

			if (!m_eAI_CurrentCoverObject)
			{
				//EXTrace.Print(EXTrace.AI, this, "Positioning - no cover obj");

				if (dist != minDist)
				{
					//EXTrace.Print(EXTrace.AI, this, "Positioning - dist != min dist, AI pos=" + pos);

					pos = pos + dir.Normalized() * (dist - minDist * Math.RandomFloat(1.0, 2.0));
				}

				if (flank)
				{
					m_eAI_FlankTimeMax = Math.RandomFloat(dist * 0.3, dist * 0.6);
					m_eAI_PositionOverrideTimeout = m_eAI_FlankTimeMax;
				}
				else
				{
					if (keepMinDistToTarget)
					{
						m_eAI_PositionOverrideTimeout = 3.0;
					}
					else
					{
						if (minDist > 3.0)
							pos = ExpansionMath.GetRandomPointInCircle(pos, minDist);

						m_eAI_PositionOverrideTimeout = Math.RandomFloat(5.0, 10.0);
					}
				}
			}
		}
		else
		{
			pos = target.GetPosition(this);
		}

		//EXTrace.Print(EXTrace.AI, this, "Positioning - pos " + pos + " dist " + dist + " minDist " + minDist + " cover obj " + m_eAI_CurrentCoverObject);
		OverrideTargetPosition(pos, isFinal, 1.0, allowJumpClimb);

		if ((minDist || flank) && m_eAI_CurrentCoverObject && m_eAI_CurrentCoverObject != currentCoverObj)
		{
			m_PathFinding.OnUpdate(m_dT, -1.0);
			m_eAI_CurrentCoverPosition = m_PathFinding.GetEnd();
		#ifdef DIAG_DEVELOPER
			//EXTrace.Print(EXTrace.AI, this, "Positioning - cover obj " + m_eAI_CurrentCoverObject + " pos " + m_eAI_CurrentCoverObject.GetPosition() + " cover pos " + m_eAI_CurrentCoverPosition + " dist to cover pos " + vector.Distance(m_eAI_CurrentCoverObject.GetPosition(), m_eAI_CurrentCoverPosition));
			Expansion_DebugObject_Deferred(20, m_eAI_CurrentCoverPosition, "ExpansionDebugNoticeMe", GetDirection());
		}
		else if (!m_eAI_CurrentCoverObject)
		{
			Expansion_DebugObject_Deferred(20, "0 0 0", "ExpansionDebugNoticeMe");
		#endif
		}
	}

	bool eAI_ShouldUpdatePosition()
	{
		if (m_eAI_PositionOverrideTimeout > 0 && (!Math.IsPointInCircle(GetPosition(), 0.55, m_PathFinding.GetEnd()) || m_eAI_IsInCover))
			return false;

		return true;
	}

	bool eAI_IsInFlankRange(eAITarget target)
	{
		return eAI_IsInFlankRange(target.GetDistance(this));
	}

	bool eAI_IsInFlankRange(float dist)
	{
		if (dist > 5.0 && dist <= 200.0)
			return true;

		return false;
	}

	void OverrideTurnTarget(bool pActive, float angle)
	{
		m_eAI_TurnTargetActive = pActive;
		m_eAI_TurnTarget = angle;
	}

	/**
	 * @brief Overrides speed target at which the ai wants to move at
	 *
	 * @param pActive is the override active
	 * @param pPosition 0 = IDLE, 1 = WALK, 2 = RUN, 3 = SPRINT
	 */
	void OverrideMovementSpeed(bool pActive, int pSpeed)
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_2(this, "OverrideMovementSpeed").Add(pActive).Add(pSpeed);
#endif

		if (EXTrace.AI && (pActive != m_MovementSpeedActive || pSpeed != m_MovementSpeed))
			EXTrace.Print(true, this, "OverrideMovementSpeed " + m_MovementSpeedActive + " -> " + pActive + " " + m_MovementSpeed + " -> " + pSpeed);

		m_MovementSpeedActive = pActive;
		m_MovementSpeed = pSpeed;
	}

	/**
	 * @brief Overrides movement direction (forwards, backwards or strafing)
	 *
	 * @param pActive is the override active
	 * @param pDirection Relative angle on current model orientation from [-180, 180] where 0 is forward.
	 */
	void OverrideMovementDirection(bool pActive, float pDirection)
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_2(this, "OverrideMovementDirection").Add(pActive).Add(pDirection);
#endif

		if (!pActive && m_SideStepAngle)
			return;

		if (EXTrace.AI && (pActive != m_MovementDirectionActive || pDirection != m_MovementDirection))
			EXTrace.Print(true, this, "OverrideMovementDirection " + m_MovementDirectionActive + " -> " + pActive + " " + m_MovementDirection + " -> " + pDirection);

		m_MovementDirectionActive = pActive;
		m_MovementDirection = pDirection;
	}

	//! @note returns whether stance was changed or not
	bool OverrideStance(int pStanceIdx, bool force = false)
	{
		if (IsClimbing() || IsFalling() || IsFighting() || IsSwimming() || IsUnconscious())
			return false;

		eAICommandMove move = GetCommand_MoveAI();

		if (!move && force)
			move = StartCommand_MoveAI();

		if (move)
		{
			int stanceIdx = move.GetStance();
			if (move.OverrideStance(pStanceIdx, force))
			{
				if (pStanceIdx == DayZPlayerConstants.STANCEIDX_ERECT)
					m_eAI_ShouldGetUp = true;
				else
					m_eAI_ShouldGetUp = false;

				if (EXTrace.AI)
					EXTrace.Print(true, this, "OverrideStance " + typename.EnumToString(eAIStance, stanceIdx) + " (" + stanceIdx + ") -> " + typename.EnumToString(eAIStance, pStanceIdx) + " (" + pStanceIdx + ") " + force);

				return true;
			}
		}

		return false;
	}

#ifndef DAYZ_1_27
	//! This has been moved to native so it now has a script compile error
	//! Requested for 'HumanCommandScript.GetCurrentStance' to be exposed
	override bool IsPlayerInStance(int pStanceMask)
	{
		auto cmd = GetCommand_MoveAI();
		if (cmd)
			return ((1 << cmd.GetStance()) & pStanceMask) != 0;

		//! @note IMPORTANT need to call super if not in eAICommandMove else it breaks actions!
		return super.IsPlayerInStance(pStanceMask);
	}
#endif
	
	bool eAI_IsChangingStance()
	{
		auto cmd = GetCommand_MoveAI();
		return cmd && cmd.IsChangingStance();
	}

	bool eAI_ShouldGetUp()
	{
		return m_eAI_ShouldGetUp;
	}

	void eAI_SetSniperProneDistanceThreshold(float distance)
	{
		m_eAI_SniperProneDistanceThreshold = distance;
	}

	bool eAI_AdjustStance(Weapon_Base gun, float distToTargetSq)
	{
		//! Go prone if target is farther than threshold, we weren't hit in the last 10 s, we face the target, and we have a long range optic,
		//! otherwise stand back up

		float distThreshSq = m_eAI_SniperProneDistanceThreshold * m_eAI_SniperProneDistanceThreshold;
		if (distThreshSq > 0.0 && distToTargetSq > distThreshSq && GetGame().GetTickTime() - m_eAI_LastHitTime > 10.0)
		{
			vector targetDirection = vector.Direction(GetPosition(), m_eAI_AimPosition_WorldSpace).Normalized();
			ItemOptics optics;
			if (vector.Dot(GetDirection(), targetDirection) > 0.9 && Class.CastTo(optics, gun.GetAttachedOptics()) && optics.GetZeroingDistanceZoomMax() >= 800)
				return OverrideStance(DayZPlayerConstants.STANCEIDX_PRONE);
		}

		if (m_eAI_IsInCover)
			return false;

		return OverrideStance(DayZPlayerConstants.STANCEIDX_ERECT);
	}

	bool eAI_AdjustStance(int lastFireTime, int timeSinceLastFire, int timeBetweenFiringAndGettingUp)
	{
		bool getUp;

		if (lastFireTime > 0 && timeSinceLastFire > timeBetweenFiringAndGettingUp && !m_eAI_IsInCover)
		{
			getUp = Expansion_GetUp();
			if (getUp && EXTrace.AI)
				EXTrace.Print(true, this, "eAI_AdjustStance " + timeSinceLastFire + " > " + timeBetweenFiringAndGettingUp);
		}

		return getUp;
	}

	int eAI_GetStance()
	{
		auto cmd = GetCommand_MoveAI();
		if (cmd)
			return cmd.GetStance();

		return eAIStance.UNKNOWN;
	}

	void SetMovementSpeedLimit(int pSpeed, bool roaming = false)
	{
		m_MovementSpeedLimit = eAI_GetMovementSpeed(pSpeed);

		if (roaming)
			m_eAI_RoamingMovementSpeedLimit = m_MovementSpeedLimit;
	}

	void SetMovementSpeedLimits(int pSpeed, int pSpeedUnderThreat = -1)
	{
		SetMovementSpeedLimit(pSpeed);
		if (pSpeedUnderThreat == -1)
			pSpeedUnderThreat = pSpeed;
		m_MovementSpeedLimitUnderThreat = eAI_GetMovementSpeed(pSpeedUnderThreat);
	}

	/**
	 * @brief set best speed limit based on member state (broken legs etc)
	 */
	void eAI_SetBestSpeedLimit(int speedLimit)
	{
		if (speedLimit <= 1)
			return;

		eAIGroup group = GetGroup();
		eAIBase ai;

		if (group.GetWaypointBehaviour() == eAIWaypointBehavior.ROAMING && speedLimit < m_eAI_RoamingMovementSpeedLimit)
			speedLimit = m_eAI_RoamingMovementSpeedLimit;

		for (int i = 0; i < group.Count(); i++)
		{
			if (Class.CastTo(ai, group.GetMember(i)) && ai != this)
			{
				if (ai.GetBrokenLegs() == eBrokenLegs.BROKEN_LEGS || (!IsSwimming() && ai.IsSwimming()))
				{
					if (m_MovementSpeedLimit > 1)
						SetMovementSpeedLimit(1);

					return;
				}

				//! If any other AI in group is more than 30 m from leader, slow down enough for them to be able to catch up
				if (speedLimit > 2 && vector.DistanceSq(GetPosition(), ai.GetPosition()) > 900)
				{
					if (m_MovementSpeedLimit > 2)
						SetMovementSpeedLimit(2);

					return;
				}
			}
		}

		if (m_MovementSpeedLimit < speedLimit)
			SetMovementSpeedLimit(speedLimit);
	}

	int GetMovementSpeedLimit()
	{
		return m_MovementSpeedLimit;
	}

	int GetMovementSpeedLimitUnderThreat()
	{
		return m_MovementSpeedLimitUnderThreat;
	}

	float eAI_GetCurrentMovementSpeedLimit()
	{
		auto cmd = GetCommand_MoveAI();
		if (cmd)
			return cmd.GetSpeedLimit();

		return -1;
	}

	static int eAI_GetMovementSpeed(eAIMovementSpeed pSpeed)
	{
		switch (pSpeed)
		{
			case eAIMovementSpeed.RANDOM:
				return Math.RandomIntInclusive(0, 3);
			case eAIMovementSpeed.RANDOM_NONSTATIC:
				return Math.RandomIntInclusive(1, 3);
		}

		return pSpeed;
	}

	void CreateDebugApple()
	{
		Class.CastTo(m_DebugTargetApple, GetGame().CreateObject("Apple", GetPosition(), true));
	}

	void DestroyDebugApple()
	{
		if (m_DebugTargetApple)
			m_DebugTargetApple.Delete();
	}

	EntityAI Expansion_CloneItemToHands(EntityAI src)
	{
		InventoryLocation il_dst = new InventoryLocation();

		il_dst.SetHands(this, src);

		return Expansion_CloneItemToLocation(src, il_dst);
	}

	EntityAI Expansion_CloneItemToGround(EntityAI src, vector position = "0 0 0")
	{
		InventoryLocation ground();

		vector mat[4];
		Math3D.MatrixIdentity4(mat);
		mat[3] = position;

		ground.SetGround(NULL, mat);

		return Expansion_CloneItemToLocation(src, ground);
	}

	EntityAI Expansion_CloneItemToInventory(EntityAI src, FindInventoryLocationType flags = 0)
	{
		InventoryLocation location();

		/*!
		 * ATTACHMENT          = 4
		 * CARGO               = 8
		 * HANDS               = 16
		 * PROXYCARGO          = 32
		 * ANY_CARGO           = 40 (CARGO | PROXYCARGO)
		 * ANY                 = 60 (ATTACHMENT | ANY_CARGO | HANDS)
		 * NO_SLOT_AUTO_ASSIGN = 64
		 */
		if (!flags)
			flags = FindInventoryLocationType.ATTACHMENT | FindInventoryLocationType.CARGO;

		if (!GetInventory().FindFirstFreeLocationForNewEntity(src.GetType(), flags, location))
			return null;

		return Expansion_CloneItemToLocation(src, location);
	}

	EntityAI Expansion_CloneItemToLocation(EntityAI src, InventoryLocation location)
	{
		if (!src || src.IsSetForDeletion())
			return null;

		InventoryLocation il_src = new InventoryLocation();

		src.GetInventory().GetCurrentInventoryLocation(il_src);

		if (il_src.GetType() == InventoryLocationType.HANDS)
		{
			if (GetWeaponManager().IsRunning())
				GetWeaponManager().OnWeaponActionEnd();  //! Prevent getting stuck in running state
		}

		if (location.GetType() == InventoryLocationType.HANDS)
		{
			//! Forcing switch to HumanCommandMove before taking to hands,
			//! and hiding/showing item in hands after, unbreaks hand anim state
			if (!GetCommand_Move())
				StartCommand_Move();
			else
				m_eAI_CommandTime = 0.0;
		}

		//! Needs to be 'lootable' while in AI inventory, else AI won't be able to properly interact with item
		//! (e.g. won't be able to detach/attach mag to firearm)
		EntityAI dst = ExpansionItemSpawnHelper.Clone(src, true, location, false);
		if (dst)
		{
			ItemBase item;
			ItemBase dstItem;
			if (Class.CastTo(dstItem, dst) && Class.CastTo(item, src))
				dstItem.m_Expansion_IsOwnerPlayer = item.m_Expansion_IsOwnerPlayer;

		#ifdef JM_COT
			set<PlayerBase> spectators;
			if (src.GetHierarchyRootPlayer() != this && dstItem)
			{
				eAIGroup group = GetGroup();
				eAIBase ai;
				for (int i = 0; i < group.Count(); i++)
				{
					if (Class.CastTo(ai, group.GetMember(i)) && ai.m_eAI_Spectators.Count() > 0)
					{
						spectators = ai.m_eAI_Spectators;
						break;
					}
				}
			}
		#endif

			GetGame().ObjectDelete(src);

			if (location.GetType() == InventoryLocationType.HANDS)
			{
				//! Forcing switch to HumanCommandMove before taking to hands,
				//! and hiding/showing item in hands after, unbreaks hand anim state
				GetItemAccessor().HideItemInHands(true);
				GetItemAccessor().HideItemInHands(false);
			}
			else if (location.GetType() == InventoryLocationType.GROUND && !m_Expansion_CanBeLooted)
			{
				if (dstItem && !dstItem.m_Expansion_IsOwnerPlayer)
				{
					ExpansionItemBaseModule.SetLootable(dst, false);
					dst.SetLifetimeMax(120);  //! Make sure it despawns quickly when left alone
				}
			}

		#ifdef JM_COT
			if (spectators)
			{
				string itemName = dstItem.Expansion_GetDisplayNameRaw();

				foreach (PlayerBase spectator: spectators)
				{
					if (spectator)
						ExpansionNotification(GetCachedName(), itemName, "Open Hand", ARGB(255, 248, 228, 51), 1.0, ExpansionNotificationType.ACTIVITY).Create(spectator.GetIdentity());
				}
			}
		#endif
		}

		return dst;
	}	
	
	override string GetCachedName()
	{
		m_CachedPlayerName = GetDisplayName();

		if (m_AdminLog && m_CachedPlayerName == m_AdminLog.m_Expansion_SurvivorDisplayName)
			m_CachedPlayerName = Expansion_GetSurvivorName();

		return m_CachedPlayerName;
	}

	//! @note INTERNAL USE ONLY
	void eAI_OnInventoryEnter(ItemBase item)
	{
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Call(eAI_AddItem, item);  //! Add in next tick to make sure we're not instantly adding items created by ObjectCreate(Ex) and similar methods where item setup (quantity etc) is performed in script after creation
	}

	//! @note INTERNAL USE ONLY
	void eAI_OnInventoryExit(ItemBase item)
	{
		eAI_RemoveItem(item);
	}

	//! @note INTERNAL USE ONLY
	void eAI_OnItemDestroyed(ItemBase item)
	{
		eAI_RemoveItem(item);
	}

	//! @note INTERNAL USE ONLY
	private void eAI_AddItem(ItemBase item)
	{
		//! @note since call to eAI_AddItem is deferred, we need to do these checks here
		if (!item || item.GetHierarchyRootPlayer() != this || item.IsDamageDestroyed())
			return;

		Weapon_Base weapon;
		if (Class.CastTo(weapon, item))
		{
			EXTrace.Print(EXTrace.AI, this, "eAI_AddItem - weapon " + item);
			if (item.ShootsExplosiveAmmo())
			{
				m_eAI_Launchers.Insert(weapon);
			}
			else
			{
				if (item.IsKindOf("Pistol_Base"))
					m_eAI_Handguns.Insert(weapon);
				else
					m_eAI_Firearms.Insert(weapon);
			}
			return;
		}

		//! When item is cloned to a shoulder slot or similar, GetInventory().IsAttachment() is true, but GetHierarchyRootPlayer() is NULL
		//! GetHierarchyRoot() is the item itself in that case
		if (item.Expansion_IsMeleeWeapon() && (!item.GetInventory().IsAttachment() || !item.GetHierarchyRootPlayer() || item.GetHierarchyRootPlayer() == this))
		{
			EXTrace.Print(EXTrace.AI, this, "eAI_AddItem - melee weapon " + item);
			m_eAI_MeleeWeapons.Insert(item);
			return;
		}

		if (item.Expansion_CanBeUsedToBandage())
		{
			EXTrace.Print(EXTrace.AI, this, "eAI_AddItem - bandage/rag " + item);
			m_eAI_Bandages.Insert(item);
			return;
		}

		//! Ammo/magazines
		Magazine mag;
		if (Class.CastTo(mag, item) && !item.GetInventory().IsAttachment())
		{
			EXTrace.Print(EXTrace.AI, this, "eAI_AddItem - mag " + item);
			eAI_AddMag(mag, true);
		}
	}

	//! @note INTERNAL USE ONLY
	void eAI_OnMagAttached(EntityAI parent, int slot_id, Magazine mag)
	{
		//! If mag is attached, remove from available mags
		eAI_RemoveMag(mag);
	}

	//! @note INTERNAL USE ONLY
	void eAI_OnMagDetached(EntityAI parent, int slot_id, Magazine mag)
	{
		//! If mag is detached, add to available mags
		if (!mag.IsDamageDestroyed() && parent != this)
			eAI_AddMag(mag);
	}

	//! @note INTERNAL USE ONLY
	private void eAI_AddMag(Magazine mag, bool fillAnyCompatibleMag = false)
	{
		EXTrace.Print(EXTrace.AI, this, "eAI_AddMag - " + mag + " " + mag.GetType() + " ammo count " + mag.GetAmmoCount());

		if (mag.GetAmmoCount() == 0 && eAI_GetMagazineTypeCount(mag.GetType()) > 0)
		{
			EXTrace.Print(EXTrace.AI, this, "Deleting empty duplicate " +  mag);
			GetGame().ObjectDelete(mag);
			return;
		}

		//! If current weapon doesn't have internal mag, fill any compatible mag in inventory from mag entering inv
		//! (and delete afterwards if empty)
		Weapon_Base weapon;
		if (fillAnyCompatibleMag && Class.CastTo(weapon, GetHumanInventory().GetEntityInHands()) && !weapon.HasInternalMagazine(weapon.GetCurrentMuzzle()))
			eAI_FillAnyCompatibleMag(weapon, mag, true);

		if (mag && !mag.IsSetForDeletion())  //! eAI_FillMag may delete mag if empty
			m_eAI_Magazines.Insert(mag);

		//! Force re-evaluation of any gun (loot) targets/guns in inventory
		eAI_EvaluateFirearmTypes();
	}

	//! @note INTERNAL USE ONLY
	private void eAI_RemoveItem(ItemBase item)
	{
		Weapon_Base weapon;
		if (Class.CastTo(weapon, item))
		{
			EXTrace.Print(EXTrace.AI, this, "eAI_RemoveItem - gun " + item);
			if (item.ShootsExplosiveAmmo())
			{
				m_eAI_Launchers.RemoveItem(weapon);
			}
			else
			{
				if (item.IsKindOf("Pistol_Base"))
					m_eAI_Handguns.RemoveItem(weapon);
				else
					m_eAI_Firearms.RemoveItem(weapon);
			}
			m_eAI_EvaluatedFirearmTypes.Remove(item.Type());
			return;
		}

		if (item.Expansion_IsMeleeWeapon())
		{
			EXTrace.Print(EXTrace.AI, this, "eAI_RemoveItem - melee weapon " + item);
			m_eAI_MeleeWeapons.RemoveItem(item);
			return;
		}

		if (item.Expansion_CanBeUsedToBandage())
		{
			EXTrace.Print(EXTrace.AI, this, "eAI_RemoveItem - bandage/rag " + item);
			m_eAI_Bandages.RemoveItem(item);
			if (item == m_eAI_BandageToUse)
				m_eAI_BandageToUse = null;
			return;
		}

		//! Ammo/magazines
		Magazine mag;
		if (Class.CastTo(mag, item))
		{
			EXTrace.Print(EXTrace.AI, this, "eAI_RemoveItem - mag " + item);
			eAI_RemoveMag(mag, true);
		}
	}

	//! @note INTERNAL USE ONLY
	private void eAI_RemoveMag(Magazine mag, bool checkAttached = false)
	{
		int removeIndex = m_eAI_Magazines.Find(mag);
		if (removeIndex >= 0)
		{
			m_eAI_Magazines.RemoveOrdered(removeIndex);
			if (checkAttached && mag.GetInventory().IsAttachment())
				EXError.Warn(this, "Removed mag is attached but was tracked as available for reload " + ExpansionStatic.GetDebugInfo(mag) + ", parent " + ExpansionStatic.GetDebugInfo(mag.GetHierarchyParent()));
		}

		EXTrace.Print(EXTrace.AI, this, "eAI_RemoveMag - " + mag + " index " + removeIndex + " remaining " + m_eAI_Magazines.Count());

		for (int i = m_eAI_Magazines.Count() - 1; i >= 0; i--)
		{
			mag = m_eAI_Magazines[i];
			if (!mag)
			{
				EXError.Warn(this, "NULL entry found in magazines tracked for reload at index " + i);
				m_eAI_Magazines.Remove(i);
			}
		}

		//! Force re-evaluation of any gun (loot) targets/guns in inventory
		eAI_EvaluateFirearmTypes();
	}

	bool eAI_HasMagazineType(string type)
	{
		foreach (Magazine mag: m_eAI_Magazines)
		{
			if (!mag)
				continue;

			if (mag.GetType() == type)
				return true;
		}

		return false;
	}

	bool eAI_HasMagazineTypeWithAmmo(string type)
	{
		foreach (Magazine mag: m_eAI_Magazines)
		{
			if (!mag)
				continue;

			if (mag.GetType() == type && mag.GetAmmoCount() > 0)
				return true;
		}

		return false;
	}

	bool eAI_HasWeaponForMagazine(Magazine mag)
	{
		if (eAI_HasWeaponForMagazine(m_eAI_Firearms, mag))
			return true;

		if (eAI_HasWeaponForMagazine(m_eAI_Handguns, mag))
			return true;

		if (eAI_HasWeaponForMagazine(m_eAI_Launchers, mag))
			return true;

		return false;
	}

	bool eAI_HasWeaponForMagazine(array<Weapon_Base> weapons, Magazine mag)
	{
		foreach (Weapon_Base weapon: weapons)
		{
			if (weapon.Expansion_IsCompatibleMag(weapon.GetCurrentMuzzle(), mag))
				return true;
		}

		return false;
	}

	int eAI_GetMagazineTypeAmmoCount(string type)
	{
		int ammoCount;

		foreach (Magazine mag: m_eAI_Magazines)
		{
			if (!mag)
				continue;

			if (mag.GetType() == type)
				ammoCount += mag.GetAmmoCount();
		}

		return ammoCount;
	}

	int eAI_GetMagazineTypeCount(string type)
	{
		int count;

		foreach (Magazine mag: m_eAI_Magazines)
		{
			if (!mag)
				continue;

			if (mag.GetType() == type)
				count++;
		}

		return count;
	}

	int eAI_GetMagazineTypeWithAmmoCount(string type)
	{
		int count;

		foreach (Magazine mag: m_eAI_Magazines)
		{
			if (!mag)
				continue;

			if (mag.GetType() == type && mag.GetAmmoCount() > 0)
				count++;
		}

		return count;
	}

	bool eAI_HasWeaponInInventory(bool requireAmmo = false)
	{
		if (m_eAI_Firearms.Count() == 0 && m_eAI_Handguns.Count() == 0 && m_eAI_Launchers.Count() == 0)
			return false;

		if (requireAmmo && !eAI_GetAnyWeaponToUse(true))
			return false;

		return true;
	}

	bool eAI_HasFirearm(bool requireAmmo = false)
	{
		if (m_eAI_Launchers.Count() == 0)
			return false;

		if (requireAmmo && !eAI_GetWeaponToUse(true, false))
			return false;

		return true;
	}

	bool eAI_HasLauncher(bool requireAmmo = false)
	{
		if (m_eAI_Launchers.Count() == 0)
			return false;

		if (requireAmmo && !eAI_GetWeaponToUse(true, true))
			return false;

		return true;
	}

	bool eAI_HasAmmoForFirearm(Weapon_Base gun, out Magazine mag, bool checkMagsInInventory = true)
	{
		if (gun.Expansion_HasAmmo(mag))
			return true;

		bool found = m_eAI_EvaluatedFirearmTypes.Find(gun.Type(), mag);
		bool hasAmmo;

		if (found && mag && mag.GetAmmoCount())
			hasAmmo = true;

		if (!found || (checkMagsInInventory && !hasAmmo))
		{
			//! eAI_GetMagazineToReload will only return non-empty mags/ammo. EXPENSIVE, use with care.
			mag = eAI_GetMagazineToReload(gun);

			EXTrace.Print(EXTrace.AI, this, "eAI_HasAmmoForFirearm - inserting " + gun.Type() + " " + mag);
			m_eAI_EvaluatedFirearmTypes[gun.Type()] = mag;

			return mag != null;
		}

		return hasAmmo;
	}

	void eAI_EvaluateFirearmTypes()
	{
		TTypenameArray toRemove();

		foreach (typename type, Magazine mag: m_eAI_EvaluatedFirearmTypes)
		{
			if (!mag || mag.GetHierarchyRootPlayer() != this)
				toRemove.Insert(type);
		}

		foreach (typename removeType: toRemove)
		{
			EXTrace.Print(EXTrace.AI, this, "eAI_EvaluateFirearmTypes - removing " + removeType + " " + m_eAI_EvaluatedFirearmTypes[removeType]);
			m_eAI_EvaluatedFirearmTypes.Remove(removeType);
		}
	}

	bool eAI_ShouldPreferExplosiveAmmo()
	{
		eAITarget target = GetTarget();

		if (target)
		{
			DayZPlayerImplement player;
			if (Class.CastTo(player, target.GetEntity()) && player.Expansion_GetParent() && eAI_GetTargetThreat(target.info, true) > 0.2)
				return true;
		}

		return false;
	}

	string Expansion_CommandIDToString(int cmdID)
	{
		string cmd;
		switch (cmdID)
		{
			case DayZPlayerConstants.COMMANDID_NONE:
				cmd = "COMMANDID_NONE";
				break;
			case DayZPlayerConstants.COMMANDID_MOVE:
				cmd = "COMMANDID_MOVE";
				break;
			case DayZPlayerConstants.COMMANDID_ACTION:
				cmd = "COMMANDID_ACTION";
				break;
			case DayZPlayerConstants.COMMANDID_MELEE:
				cmd = "COMMANDID_MELEE";
				break;
			case DayZPlayerConstants.COMMANDID_MELEE2:
				cmd = "COMMANDID_MELEE2";
				break;
			case DayZPlayerConstants.COMMANDID_FALL:
				cmd = "COMMANDID_FALL";
				break;
			case DayZPlayerConstants.COMMANDID_DEATH:
				cmd = "COMMANDID_DEATH";
				break;
			case DayZPlayerConstants.COMMANDID_DAMAGE:
				cmd = "COMMANDID_DAMAGE";
				break;
			case DayZPlayerConstants.COMMANDID_LADDER:
				cmd = "COMMANDID_LADDER";
				break;
			case DayZPlayerConstants.COMMANDID_UNCONSCIOUS:
				cmd = "COMMANDID_UNCONSCIOUS";
				break;
			case DayZPlayerConstants.COMMANDID_SWIM:
				cmd = "COMMANDID_SWIM";
				break;
			case DayZPlayerConstants.COMMANDID_VEHICLE:
				cmd = "COMMANDID_VEHICLE";
				break;
			case DayZPlayerConstants.COMMANDID_CLIMB:
				cmd = "COMMANDID_CLIMB";
				break;
			case DayZPlayerConstants.COMMANDID_SCRIPT:
				cmd = "COMMANDID_SCRIPT";
				break;
		}

		cmd += "(" + cmdID.ToString() + ")";

		return cmd;
	}

	void eAI_ClientUpdate()
	{
		float pDt = 1.0 / 30.0;

		GetTransform(m_ExTransformPlayer);

		if (!eAI_HandleAiming(pDt))
			return;

		HumanInputController hic = GetInputController();
		EntityAI entityInHands = GetHumanInventory().GetEntityInHands();

		if (hic && entityInHands && entityInHands.IsInherited(Weapon_Base))
		{
			bool exitIronSights = false;
			HandleWeapons(pDt, entityInHands, hic, exitIronSights);
		}
	}

	bool eAI_HandleAiming(float pDt, bool hasLOS = false)
	{
		auto vehCmd = GetCommand_VehicleAI();
		if (vehCmd && (vehCmd.IsGettingIn() || vehCmd.IsGettingOut()))
			return false;

		eAITarget target = m_eAI_Targets[0];

		bool isServer = GetGame().IsServer();
		bool lookAim;
		bool lookDirectionRecalculate;
		bool aimDirectionRecalculate;

		//! Aim from, not to! Has to be "neck" because that's how player model holds gun, everything else will break aiming
		vector neck = GetBonePositionWS(GetBoneIndexByName("neck"));

		if (target)
		{
			//! If we have LOS, or active threat level is equal to or above 0.4 (meaning we had LOS before), or target is noise,
			//! look/aim in (last known) target direction
			if (hasLOS || (m_eAI_CurrentThreatToSelfActive >= 0.4 && !target.info.IsInherited(eAIItemTargetInformation)) || target.info.IsInherited(eAINoiseTargetInformation))
				lookAim = true;

			//! If we don't have LOS, are in cover and in a building with doors, then look in direction of previous waypoint,
			//! which is guaranteed to be the direction of a door
			BuildingBase building;
			bool houseWithDoors;
			vector prevWaypoint;
			if (!hasLOS && m_eAI_IsInCover && Class.CastTo(building, m_eAI_CurrentCoverObject) && building.Expansion_IsEnterable())
			{
				prevWaypoint = m_PathFinding.m_PrevPoint;
				if (ExpansionMath.Distance2DSq(GetPosition(), prevWaypoint) > 0.0001)
				{
					houseWithDoors = true;
					lookAim = true;
				}
			}

			vector aimPosition;

			if (houseWithDoors)
				aimPosition = prevWaypoint + (neck - GetPosition());
			else
				aimPosition = target.GetPosition(this, !isServer) + target.GetAimOffset(this);

			if ((!isServer || (IsRaised() && m_eAI_CurrentThreatToSelfActive > 0.152) || (!IsRaised() && m_eAI_CurrentThreatToSelfActive > 0.15) || houseWithDoors) && lookAim)
				aimDirectionRecalculate = true;

			if (isServer)
			{
				if (aimDirectionRecalculate || (!IsRaised() && (m_eAI_CurrentThreatToSelfActive > 0.1 || houseWithDoors) && lookAim))
					lookDirectionRecalculate = true;

				LookAtPosition(aimPosition, lookDirectionRecalculate);
			}

			AimAtPosition(aimPosition, aimDirectionRecalculate);
		}

		if (!target || (!lookDirectionRecalculate && !aimDirectionRecalculate))
		{
			if (!isServer || !eAI_UpdateLookDirectionPreference())
			{
				if (isServer && m_eAI_LookDirection_Recalculate)
					LookAtDirection("0 0 1");

				if (m_eAI_AimDirection_Recalculate)
					AimAtDirection("0 0 1");
			}
		}

		if (isServer && m_eAI_LookDirection_Recalculate)
		{
			m_eAI_LookDirectionTarget_ModelSpace = vector.Direction(neck, m_eAI_LookPosition_WorldSpace).Normalized().InvMultiply3(m_ExTransformPlayer);
		}

		if (m_eAI_AimDirection_Recalculate)
		{
			m_eAI_AimDirectionTarget_ModelSpace = vector.Direction(neck, m_eAI_AimPosition_WorldSpace).Normalized().InvMultiply3(m_ExTransformPlayer);
		}

		if (target)
			return true;

		return false;
	}

	override void CommandHandler(float pDt, int pCurrentCommandID, bool pCurrentCommandFinished)
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_3(this, "CommandHandler").Add(pDt).Add(pCurrentCommandID).Add(pCurrentCommandFinished);
#endif

		//! New in 1.22
		EvaluateDamageHit(pCurrentCommandID);

		//! Used for animated/continuous action progress
		m_dT = pDt;

		// CarScript car;
		// if (Class.CastTo(car, GetParent()))
		//{
		//	car.Control(pDt);
		// }

		int i;

#ifdef DIAG_DEVELOPER
#ifndef SERVER
		for (i = m_Expansion_DebugShapes.Count() - 1; i >= 0; i--)
			m_Expansion_DebugShapes[i].Destroy();
		m_Expansion_DebugShapes.Clear();
#endif
#endif
		if (pCurrentCommandID != m_eAI_CurrentCommandID)
		{
			if (EXTrace.AI)
				EXTrace.Print(true, this, "CommandHandler " + Expansion_CommandIDToString(m_eAI_CurrentCommandID) + " -> " + Expansion_CommandIDToString(pCurrentCommandID) + " finished " + pCurrentCommandFinished);
			m_eAI_CurrentCommandID = pCurrentCommandID;
			m_eAI_CommandTime = 0.0;
		}
		else if (pCurrentCommandFinished && EXTrace.AI)
		{
			EXTrace.Print(true, this, "CommandHandler " + Expansion_CommandIDToString(pCurrentCommandID) + " finished " + pCurrentCommandFinished);
		}

		m_eAI_CommandTime += pDt;

		Expansion_UpdateBonePositionTimes(pDt);
		Expansion_UpdateActualVelocity(pDt);

		GetMovementState(m_MovementState);

		//! handle death with high priority
		if (HandleDeath(pCurrentCommandID))
		{
			return;
		}

		if (!GetGame().IsServer())  //! @note ComandHandler will not run on MP client for AI anyway
			return;

		int simulationPrecision = 0;

		GetTransform(m_ExTransformPlayer);

		vector position = m_ExTransformPlayer[3];
		m_eAI_SurfaceY = GetGame().SurfaceY(position[0], position[2]);

		if (position[1] < m_eAI_SurfaceY - 0.5)
		{
			position[1] = m_eAI_SurfaceY;
			EXTrace.Print(EXTrace.AI, this, "below surface - teleporting from " + m_ExTransformPlayer[3] + " to " + position);
			SetPosition(position);
		}

		eAI_CheckIsInCover();

		EntityAI entityInHands = GetHumanInventory().GetEntityInHands();

		if (entityInHands && entityInHands.IsWeapon())
			m_eAI_HasProjectileWeaponInHands = true;
		else
			m_eAI_HasProjectileWeaponInHands = false;

		eAITarget previousTarget = m_eAI_Targets[0];
		UpdateTargets(pDt, entityInHands);
		if (eAI_RemoveTargets() || m_eAI_UpdateTargetsTick == 0)
			eAI_PrioritizeTargets();
		//if (m_eAI_SyncCurrentTarget)
			//eAI_SyncCurrentTarget();

		bool hadLOS = m_eAI_HasLOS;
		m_eAI_HasLOS = EnforceLOS();

	#ifdef DIAG_DEVELOPER
		if (!m_eAI_Targets.Count())
		{
			Expansion_DeleteDebugObject(18);
			Expansion_DeleteDebugObject(19);
		}
	#endif

		eAITarget currentTarget;
		if (!m_eAI_HasLOS && m_eAI_NoiseTarget > 0)
		{
			currentTarget = m_eAI_Targets[0];
			m_eAI_Targets[0] = m_eAI_Targets[m_eAI_NoiseTarget];
			m_eAI_Targets[m_eAI_NoiseTarget] = currentTarget;
#ifdef DIAG_DEVELOPER
			float noiseThreatLevelActive = eAI_GetTargetInformationState(m_eAI_Targets[0].info).m_ThreatLevelActive;
			EXTrace.Print(EXTrace.AI, this, "eAI_PrioritizeTargets - prioritizing noise target " + m_eAI_NoiseTarget + " " + m_eAI_Targets[0].GetDebugName() + " threat lvl " + m_eAI_CurrentThreatToSelfActive + " -> " + noiseThreatLevelActive);
#endif
			m_eAI_NoiseTarget = 0;
		}

		currentTarget = m_eAI_Targets[0];

		if (!currentTarget)
		{
			if (previousTarget)
				m_eAI_Callbacks.OnNoMoreTargets();
		}
		else if (currentTarget != previousTarget)
		{
			m_eAI_Callbacks.OnTargetSelected(currentTarget);
		}

		if (m_eAI_HasLOS != hadLOS)
		{
			if (m_eAI_HasLOS)
				m_eAI_Callbacks.OnLOS(currentTarget);
			else
				m_eAI_Callbacks.OnLOSLost(currentTarget);
		}

		DetermineThreatToSelf(pDt);
		ReactToThreatChange(pDt, entityInHands);

		eAI_HandleAiming(pDt, m_eAI_HasLOS);

		if (pCurrentCommandID != DayZPlayerConstants.COMMANDID_CLIMB)
			m_PathFinding.OnUpdate(pDt, simulationPrecision);
	#ifdef DIAG_DEVELOPER
		else
			m_PathFinding.DrawDebug();
	#endif

		if (m_eAI_ResetMovementDirectionActive)
		{
			m_MovementDirectionActive = false;
			m_eAI_ResetMovementDirectionActive = false;
		}

		if (m_eAI_SideStepTimeout > 0)
		{
			if (m_eAI_HasLOS && m_eAI_SideStepCancelOnLOS)
				m_eAI_SideStepTimeout = 0.0;
			else
				m_eAI_SideStepTimeout -= pDt;

			if (m_eAI_SideStepTimeout <= 0)
			{
				eAI_ResetSidestep();
				//StartCommand_MoveAI();
			}
		}

		if (m_eAI_PositionOverrideTimeout > 0)
			m_eAI_PositionOverrideTimeout -= pDt;

		HumanInputController hic = GetInputController();
		
		CheckZeroSoundEvent();
		CheckSendSoundEvent();
		
		ProcessADDModifier();

		if (hic && pCurrentCommandID == DayZPlayerConstants.COMMANDID_SCRIPT && !pCurrentCommandFinished && m_eAI_Command)
		{
			eAICommandVehicle vehicleCmd;
			if (!Class.CastTo(vehicleCmd, m_eAI_Command) || !vehicleCmd.IsGettingIn())
			{
				bool exitIronSights = false;
				//! @note HandleWeapons also deals with hands lowering/raising if no weapon in hands
				HandleWeapons(pDt, entityInHands, hic, exitIronSights);
			}
		}

		GetDayZPlayerInventory().HandleInventory(pDt);

		if (m_WeaponManager)
			m_WeaponManager.Update(pDt);
		if (m_EmoteManager && IsPlayerSelected())
			m_EmoteManager.Update(pDt);
		if (m_RGSManager)
			 m_RGSManager.Update();
		if (m_StaminaHandler)
			m_StaminaHandler.Update(pDt, pCurrentCommandID);
		if (m_InjuryHandler)
			m_InjuryHandler.Update(pDt);
		if (m_HCAnimHandler)
			m_HCAnimHandler.Update(pDt, m_MovementState);
		if (m_ShockHandler)
			m_ShockHandler.Update(pDt);

		GetPlayerSoundManagerServer().Update();
		ShockRefill(pDt);
		FreezeCheck();

		GetHumanInventory().Update(pDt);
		if (m_IsDrowning)
			ProcessDrowning(pDt);
		UpdateDelete();

		OnScheduledTick(pDt);

		m_eAI_PurgeFiredShotsTick += pDt;
		if (m_eAI_PurgeFiredShotsTick > 1.0)
		{
			m_eAI_PurgeFiredShotsTick = 0;

			float time = GetGame().GetTickTime();

		#ifdef DIAG_DEVELOPER
			bool dbgObjEnabled = s_Expansion_DebugObjects_Enabled;
			s_Expansion_DebugObjects_Enabled = s_eAI_DebugDamage;
		#endif

			for (i = m_eAI_FiredShots.Count() - 1; i >= 0; i--)
			{
				eAIShot shot = m_eAI_FiredShots[i];
				float elapsed = time - shot.m_Time;
				//! If shot has been processed or flight time exceeds 6 seconds (DayZ max), remove shot
				if ((shot.m_ProcessedTime && time - shot.m_ProcessedTime > 0.005) || elapsed > 6.0)
				{
				#ifdef DIAG_DEVELOPER
					if (!shot.m_ProcessedTime)
					{
						EXTrace.Print(EXTrace.AI, this, "Discarding unprocessed " + shot.GetInfo());
						vector dir = Vector(shot.m_Direction[0], 0, shot.m_Direction[2]);
						Expansion_DebugObject(m_eAI_DiscardedShot_DbgIdx--, shot.m_HitPosition - "0 1.5 0", "ExpansionDebugNoticeMe_Orange", dir, vector.Zero, 5);
						Expansion_DebugObject(m_eAI_DiscardedShot_DbgIdx--, shot.m_HitPosition, "ExpansionDebugBox_Orange", dir, vector.Zero, 5);

					#ifdef EXPANSION_AI_DMGDEBUG_CHATTY
						ExpansionStatic.MessageNearPlayers(shot.m_HitPosition, 100.0, "[" + ExpansionStatic.FormatFloat(shot.m_Time, 3, false) + "] timeout " + shot.m_HitObject + " travel " + ExpansionStatic.FormatFloat(shot.m_TravelTime, 4, false) + " elapsed " + ExpansionStatic.FormatFloat(elapsed, 4, false));
					#endif
					}
				#endif
					m_eAI_FiredShots.Remove(i);
				}
			}

		#ifdef DIAG_DEVELOPER
			s_Expansion_DebugObjects_Enabled = dbgObjEnabled;
		#endif
		}

		//! Do FSM update only after current command has been running for at least one command handler tick, else initial gun holding will look scuffed
		if (m_FSM && m_eAI_CommandTime > pDt)
			m_FSM.Update(pDt, simulationPrecision);

		bool skipScript;

		if (m_ActionManager)
		{
			m_ActionManager.Update(pCurrentCommandID);

			if (pCurrentCommandID == DayZPlayerConstants.COMMANDID_UNCONSCIOUS)
			{
				OnUnconsciousUpdate(pDt, m_LastCommandBeforeUnconscious);
				if (!m_IsUnconscious)
				{
					m_IsUnconscious = true;
					OnUnconsciousStart();
				}

				if (!m_ShouldBeUnconscious && m_UnconsciousTime > 2)
				{
					HumanCommandUnconscious hcu = GetCommand_Unconscious();
					if (hcu)
					{
						int wakeUpStance = DayZPlayerConstants.STANCEIDX_PRONE;

						//! Don't set the stance if we are swimming or in a vehicle, stance change animation could play
						if (m_Swimming.m_bWasSwimming || m_LastCommandBeforeUnconscious == DayZPlayerConstants.COMMANDID_VEHICLE)
							wakeUpStance = -1;

						hcu.WakeUp(wakeUpStance);
					}
				}
			}
			else
			{
				if (m_ShouldBeUnconscious)
				{
					m_LastCommandBeforeUnconscious = pCurrentCommandID;
					m_eAI_UnconsciousVehicle = false;

					auto vehCmd = GetCommand_VehicleAI();
					if (vehCmd)
					{
						m_eAI_UnconsciousVehicle = true;

						// not going to bother supporting knocking players out at this current moment
						m_TransportCache = vehCmd.GetTransport();

						vehCmd.KeepInVehicleSpaceAfterLeave(true);
					}

					SetOrientation(GetOrientation());
					StartCommand_Unconscious(0);

					skipScript = true;
				}

				if (m_IsUnconscious)
				{
					m_IsUnconscious = false;
					OnUnconsciousStop(pCurrentCommandID);

					skipScript = true;
				}
			}
		}

		OnCommandHandlerTick(pDt, pCurrentCommandID);

		m_eAI_Command = ExpansionHumanCommandScript.Cast(GetCommand_Script());

		bool returnEarly;

		if (pCurrentCommandFinished)
		{
			if (pCurrentCommandID == DayZPlayerConstants.COMMANDID_UNCONSCIOUS)
			{
				if (m_eAI_UnconsciousVehicle && (m_TransportCache != null))
				{
					int crew_index = m_TransportCache.CrewMemberIndex(this);
					int seat = m_TransportCache.GetSeatAnimationType(crew_index);
					StartCommand_VehicleAI(m_TransportCache, crew_index, seat, true);
					m_TransportCache = null;
					return;
				}
			}

			if (PhysicsIsFalling(true))
			{
				SetOrientation(GetOrientation());
				StartCommand_Fall(0);

				return;
			}

			if (pCurrentCommandID == DayZPlayerConstants.COMMANDID_UNCONSCIOUS)
				StartCommand_Move();
			else
				StartCommand_MoveAI();

			returnEarly = true;
		}
		else if (pCurrentCommandID == DayZPlayerConstants.COMMANDID_CLIMB)
		{
			HumanCommandClimb hcc = GetCommand_Climb();

			if (hcc)
			{
				int hccState = hcc.GetState();
				
				/**
				 * @note
				 * For vaulting: MOVE -> TAKEOFF -> ONTOP -> FALLING -> FINISH
				 * For climbing: MOVE -> TAKEOFF -> ONTOP
				 */

				if (hccState != m_eAI_PrevHCCState)
				{
				#ifdef DIAG_DEVELOPER
					EXTrace.Print(EXTrace.AI, this, "CommandClimb -> " + typename.EnumToString(ClimbStates, hccState));
				#endif
					m_eAI_PrevHCCState = hccState;
				}
			}
		}
		// taken from vanilla DayZPlayerImplement
		else if (pCurrentCommandID == DayZPlayerConstants.COMMANDID_FALL)
		{
			HumanCommandFall fall = GetCommand_Fall();

			vector wl;
			if (!IsSwimming() && DayZPlayerUtils.CheckWaterLevel(this, wl) == EWaterLevels.LEVEL_SWIM_START)
			{
				StartCommand_MoveAI();
				return;
			}

			if (fall && fall.PhysicsLanded())
			{
				DayZPlayerType type = GetDayZPlayerType();
				NoiseParams npar;

				FallDamageData fallDamageData = new FallDamageData();
				fallDamageData.m_Height = m_FallYDiff - GetPosition()[1];

				// land
				if (fallDamageData.m_Height < 0.5)
				{
					fallDamageData.m_LandType = HumanCommandFall.LANDTYPE_NONE; 
					fall.Land(fallDamageData.m_LandType);
					npar = type.GetNoiseParamsLandLight();
					AddNoise(npar);
				}
				else if (fallDamageData.m_Height < 3.0)
				{
					if (m_MovementState.IsInProne() || m_MovementState.IsInRaisedProne())
						fallDamageData.m_LandType = HumanCommandFall.LANDTYPE_NONE;
					else
						fallDamageData.m_LandType = HumanCommandFall.LANDTYPE_LIGHT;
					
					fall.Land(fallDamageData.m_LandType);
					npar = type.GetNoiseParamsLandLight();
					AddNoise(npar);
				}
				else if (fallDamageData.m_Height < 5.0)
				{
					fallDamageData.m_LandType = HumanCommandFall.LANDTYPE_MEDIUM;
					fall.Land(fallDamageData.m_LandType);
					npar = type.GetNoiseParamsLandHeavy();
					AddNoise(npar);
				}
				else
				{
					fallDamageData.m_LandType = HumanCommandFall.LANDTYPE_HEAVY;
					fall.Land(fallDamageData.m_LandType);
					npar = type.GetNoiseParamsLandHeavy();
					AddNoise(npar);
				}
				
				if (fallDamageData.m_Height >= DayZPlayerImplementFallDamage.HEALTH_HEIGHT_LOW && GetInstanceType() == DayZPlayerInstanceType.INSTANCETYPE_CLIENT)
				{
					OnPlayerRecievedHit();
				}

				m_FallDamage.HandleFallDamage(fallDamageData);
				m_JumpClimb.CheckAndFinishJump(fallDamageData.m_LandType);
			}

			// return;
		}
		else if (IsSwimming())
		{
			if (position[1] > m_eAI_SurfaceY)
			{
				//! Apply buoyancy
				vector neck = GetBonePositionWS(GetBoneIndexByName("neck"));
				position[1] = position[1] + GetGame().GetWaterDepth(neck);
				SetPosition(position);
			}
		}
		else if (PhysicsIsFalling(false))
		{
			SetOrientation(GetOrientation());
			StartCommand_Fall(0);

			m_FallYDiff = GetPosition()[1];
			// return;

			//! Prevent NULL pointers by skipping COMMANDID_SCRIPT handling
			skipScript = true;
		}

		if (HandleDamageHit(pCurrentCommandID))
		{
			// return;

			skipScript = true;
		}

		if (returnEarly)
			return;

		if (pCurrentCommandID == DayZPlayerConstants.COMMANDID_VEHICLE)
		{
			HumanCommandVehicle hcv = GetCommand_Vehicle();
			if (hcv.WasGearChange())
			{
				GearChangeActionCallback cb = GearChangeActionCallback.Cast(AddCommandModifier_Action(DayZPlayerConstants.CMD_ACTIONMOD_SHIFTGEAR, GearChangeActionCallback));
				cb.SetVehicleCommand(hcv);
			}

			return;
		}
		else if (m_eAI_Transport)
		{
			int seat_anim_type = m_eAI_Transport.GetSeatAnimationType(m_eAI_Transport_SeatIndex);
			auto vehCommand = StartCommand_VehicleAI(m_eAI_Transport, m_eAI_Transport_SeatIndex, seat_anim_type);
			vehCommand.SetVehicleType(m_eAI_Transport.GetAnimInstance());

			m_eAI_Transport = null;
			return;
		}

		if (pCurrentCommandID == DayZPlayerConstants.COMMANDID_MOVE)
		{
			//! COMMANDID_MOVE will be running if the AI was just spawned or if we woke up from uncon
			//! IMPORTANT: Start AI move only after delay, else hand anim state will be broken!
			if (m_eAI_CommandTime > pDt && !m_FSM.IsInState("Unconscious"))
				StartCommand_MoveAI();
			// return;
		}

		if (pCurrentCommandID == DayZPlayerConstants.COMMANDID_MELEE2)
		{
			HumanCommandMelee2 hcm2 = GetCommand_Melee2();
			if (hcm2)
			{
				if (hcm2.WasHit())
				{
					m_eMeleeCombat.OnHit();

					m_eAI_MeleeDidHit = true;
				}

				if (m_eAI_Melee && m_eAI_MeleeDidHit && hcm2.IsInComboRange())
				{
					m_eAI_Melee = false;
					m_eAI_MeleeTime = GetGame().GetTime();

					m_eMeleeCombat.Combo(hcm2);
				}
			}
		}
		else if (m_eAI_Melee)
		{
			m_eAI_Melee = false;

			m_eMeleeCombat.Start();

			//! Prevent NULL pointers by skipping COMMANDID_SCRIPT handling
			skipScript = true;
		}

		m_eAI_MeleeDidHit = false;

		if (eAI_IsChangingStance())
		{
			m_eAI_IsChangingStance = true;
		}
		else if (m_eAI_IsChangingStance)
		{
			m_eAI_IsChangingStance = false;

			//! HACK: Fix collision box
			StartCommand_Move();
			skipScript = true;
		}

		if (pCurrentCommandID == DayZPlayerConstants.COMMANDID_SCRIPT && m_eAI_Command && !skipScript)
		{
			vector lookTargetRelAngles = m_eAI_LookDirectionTarget_ModelSpace.VectorToAngles();
			vector aimTargetRelAngles = m_eAI_AimDirectionTarget_ModelSpace.VectorToAngles();

			if (IsRaised())
			{
				//! Need to adjust look direction when aiming
				lookTargetRelAngles[0] = lookTargetRelAngles[0] - aimTargetRelAngles[0];
				if (entityInHands && entityInHands.IsWeapon())
					lookTargetRelAngles[1] = lookTargetRelAngles[1] - aimTargetRelAngles[1];
			}

			//! We want to interpolate rel angles for looking! Otherwise, if the conversion to rel angles happens later,
			//! there will be a sudden jump in the unit's head rotation between 180 and -180 due to the way the head animation is set up
			lookTargetRelAngles[0] = ExpansionMath.RelAngle(lookTargetRelAngles[0]);
			lookTargetRelAngles[1] = ExpansionMath.RelAngle(lookTargetRelAngles[1]);

			lookTargetRelAngles[1] = Math.Clamp(lookTargetRelAngles[1], -85.0, 85.0);  //! Valid range is [-85, 85]

			//TODO: quaternion slerp instead for better, accurate results
			m_eAI_LookRelAngles = ExpansionMath.InterpolateAngles(m_eAI_LookRelAngles, lookTargetRelAngles, pDt, Math.RandomFloat(3.0, 5.0), Math.RandomFloat(1.0, 3.0));

			m_eAI_Command.SetLookAnglesRel(m_eAI_LookRelAngles[0], m_eAI_LookRelAngles[1]);

			eAICommandMove hcm;

			int performCommand;
			if (Class.CastTo(hcm, m_eAI_Command))
				performCommand = EAI_COMMANDID_MOVE;

			switch (performCommand)
			{
			case EAI_COMMANDID_MOVE:
				if (AI_HANDLEDOORS && HandleBuildingDoors(hcm, pDt))
				{
					break;
				}
				else if (AI_HANDLEVAULTING && HandleVaulting(hcm, pDt))
				{
					if (m_JumpClimb.Expansion_Climb())
						break;
					
					//! If we didn't vault or climb, find way around obstacle, but only if path is not blocked physically
					//! (path blocked physically is only set on pathfinding if navmesh isn't blocked,
					//! so in that case we won't find an alternative path)
					if (m_PathFinding.m_AllowJumpClimb && !m_PathFinding.m_IsBlockedPhysically)
						m_PathFinding.SetAllowJumpClimb(false, 15.0);
				}
				else if (m_PathFinding.m_IsJumpClimb && m_PathFinding.m_AllowJumpClimb && !m_PathFinding.m_IsBlockedPhysically && hcm.IsBlocked() && hcm.GetBlockedTime() > pDt)
				{
					//! If we are still blocked, find way around obstacle
					//! (need to be blocked for at least one commandhandler tick
					//! else it messes with climbing after door interaction)
					m_PathFinding.SetAllowJumpClimb(false, 15.0);
				}
				
				float speedLimit;
				if (m_eAI_Halt)
					speedLimit = 0.0;
				else if (eAI_IsSideSteppingVehicle())
					speedLimit = 3.0;
				else if (m_eAI_CurrentThreatToSelfActive >= 0.4)
					speedLimit = m_MovementSpeedLimitUnderThreat;
				else
					speedLimit = m_MovementSpeedLimit;

				if (m_WeaponRaised)
					speedLimit = Math.Min(speedLimit, 2);

				float turnTarget;
				bool setTurnTarget;

				if (m_eAI_TurnTargetActive)
				{
					turnTarget = m_eAI_TurnTarget;
					setTurnTarget = true;
				}
				else if (m_eAI_AimDirection_Recalculate || Math.AbsFloat(m_eAI_AimRelAngles[0]) > 90)
				{
					turnTarget = GetAimDirectionTarget().VectorToAngles()[0];
					setTurnTarget = true;

					if (!m_eAI_LookDirection_Recalculate)
					{
						lookTargetRelAngles[0] = 0.0;
						m_eAI_LookDirectionTarget_ModelSpace = lookTargetRelAngles.AnglesToVector();
					}

					if (!m_eAI_AimDirection_Recalculate)
					{
						aimTargetRelAngles[0] = 0.0;
						m_eAI_AimDirectionTarget_ModelSpace = aimTargetRelAngles.AnglesToVector();
					}
				}
				else if (IsInherited(eAINPCBase) && hcm.GetCurrentMovementSpeed() == 0.0)
				{
					turnTarget = GetOrientation()[0];
					setTurnTarget = true;
				}

				if (setTurnTarget)
					hcm.SetTurnTarget(turnTarget, m_eAI_TurnTargetActive);

				if (m_StaminaHandler && !CanConsumeStamina(EStaminaConsumers.SPRINT) || !CanSprint())
				{
					speedLimit = Math.Min(speedLimit, 2);
				}

				if (!m_MovementDirectionActive)
					m_MovementDirection = 0;

				hcm.SetSpeedOverrider(m_MovementSpeedActive);
				hcm.SetTargetSpeed(m_MovementSpeed);
				hcm.SetSpeedLimit(speedLimit);
				hcm.SetTargetDirection(m_MovementDirection, m_MovementDirectionActive);

				m_SprintFull = false;
				if (hcm.GetCurrentMovementSpeed() > 2.99)
				{
					m_SprintedTime += pDt;
				}
				else
				{
					m_SprintedTime = 0.0;
				}

				if (m_SprintedTime > 0.5)
					m_SprintFull = true;

				if (hcm.GetPositionTime() > 3.0 && Expansion_IsAnimationIdle())
					eAI_Unbug("move");

				break;
			default:
			#ifdef DIAG_DEVELOPER
				Expansion_DebugObject(22222, "0 0 0", "ExpansionDebugSphereSmall_White");
				Expansion_DebugObject(22223, "0 0 0", "ExpansionDebugSphereSmall_Black");
			#endif
				break;
			}
		}
	}

	bool eAI_UpdateLookDirectionPreference()
	{
		if (!GetGroup() || IsUnconscious())
			return false;
		
		float time = GetGame().GetTickTime();
		bool force;

		if (m_eAI_FormationDirectionUpdateTime == 0.0)
			force = true;

		float speed = Expansion_GetMovementSpeed();
		if (m_eAI_MovementSpeedPrev == 0.0 && speed > 0.0)
			force = true;

		if (!force && time - m_eAI_FormationDirectionUpdateTime < m_eAI_FormationDirectionNextUpdateTime)
			return true;

		int count = GetGroup().Count();
		
		m_eAI_MovementSpeedPrev = speed;
		float f = 4.0 - speed;
		if (speed > 0.0)
			m_eAI_FormationDirectionNextUpdateTime = Math.RandomFloat(0.5, 0.333333 * f);
		else
			m_eAI_FormationDirectionNextUpdateTime = Math.RandomFloat(2.0, 6.666666 * f);
		m_eAI_FormationDirectionUpdateTime = time;
		vector pos, ori, dir;
		float angle;
		bool isDir;
		bool isDirWS;

		auto hcv = GetCommand_VehicleAI();
		if (speed > 0 || hcv)
		{
			isDir = true;
			eAICommandMove hcm = GetCommand_MoveAI();
			if (hcm)
			{
				Object blockingObject = hcm.GetBlockingObject();

				if (blockingObject)
				{
					pos = blockingObject.GetPosition();
					pos[1] = GetPosition()[1];
				}
				else
				{
					pos = hcm.GetWaypoint() + GetDirection();
				}

				dir = vector.Direction(GetPosition(), pos);
				if (dir.LengthSq() > 0.01)
				{
					angle = ExpansionMath.AngleDiff2(GetOrientation()[0], dir.VectorToAngles()[0]);
					ori[0] = Math.Clamp(angle, -45.0, 45.0);

					//! We use the up/down angle of the path segment direction because direction from AI to waypoint
					//! may otherwise be steeper than intended as AI position is on surface while waypoint is on navmesh,
					//! which can lead to a significant height difference.
					angle = ExpansionMath.RelAngle(m_PathFinding.m_PathSegmentDirection.VectorToAngles()[1]);
					ori[1] = Math.Clamp(angle, -45.0, 45.0);

					if (speed >= 2)
						ori[1] = ori[1] + 8 * speed;  //! The faster we go, the more we lean forward, compensate this by looking up slightly

					//EXTrace.Print(EXTrace.AI, this, "look u/d to wp " + ori[1]);
				}
				else
				{
					return true;
				}
			}
			else
			{
				ori[0] = Math.RandomFloat(-22.0 * f, 22.0 * f);
			}
		}
		else
		{
			//! Roll dice for a random look at behaviour
			int diceRoll = Math.RandomIntInclusive(1, 100);

			//! 4% to look at another member and Emote if weapon not raised (we don't want to aim gun at friendly)
			if (diceRoll < 4 && count > 1 && !IsRaised())
			{
				pos = GetGroup().GetRandomMemberExcluding(this).GetPosition() + "0 1.5 0";
				ori = vector.Direction(GetPosition() + "0 1.5 0", pos).VectorToAngles();

				if (Math.AbsFloat(ori[1]) > 45.0)
					return true;  //! Don't look up or down at steep angles

				angle = ExpansionMath.AngleDiff2(GetOrientation()[0], ori[0]);

				if (!m_Expansion_EmoteID && angle > -30 && angle < 30)
				{
					isDir = true;
					ori[0] = angle;

					eAI_RandomGreeting();
				}
			}
			//! 12% to look at another member if weapon not raised (we don't want to aim gun at friendly)
			else if (diceRoll < 16 && count > 1 && !IsRaised())
			{
				pos = GetGroup().GetRandomMemberExcluding(this).GetPosition() + "0 1.5 0";
				ori = vector.Direction(GetPosition() + "0 1.5 0", pos).VectorToAngles();

				if (Math.AbsFloat(ori[1]) > 45.0)
					return true;  //! Don't look up or down at steep angles

				angle = ExpansionMath.AngleDiff2(GetOrientation()[0], ori[0]);

				if (angle < -80 || angle > 80)
				{
					isDir = true;
					ori[0] = angle;
				}
				else
				{
					pos[1] = pos[1] + Math.RandomFloat(0.1, 0.2);
				}
			}
			//! 84% to look in formation direction, ahead or around (100% if no other group members)
			else
			{
				isDir = true;

				//! 41% to look in formation direction (56% if no other group members)
				if (diceRoll < 57)
				{
					ori[0] = Math.RandomFloat(-54.0, 54.0);

					if (count > 1 && GetGroup().GetFormationState() == eAIGroupFormationState.IN)
					{
						isDirWS = true;

						vector formPos = GetGroup().GetFormation().ToWorld(m_eAI_FormationPosition);
						dir = vector.Direction(formPos, GetGroup().GetFormation().ToWorld(m_eAI_FormationPosition + m_eAI_FormationDirection));
						ori[0] = ori[0] + dir.VectorToAngles()[0];

					#ifdef DIAG_DEVELOPER
						Expansion_DebugObject(987654321, formPos + "0 1.5 0" + dir, "ExpansionDebugSphereSmall_Orange", dir, formPos + "0 1.5 0");
					#endif
					}

					if (!m_Expansion_EmoteID && Math.RandomFloat(0.0, 1.0) < 0.05)
						Expansion_SetEmote(EmoteConstants.ID_EMOTE_WATCHING, true);
				}
				//! 43% to look ahead if NPC or around if not NPC
				else if (IsInherited(eAINPCBase))
				{
					ori[0] = Math.RandomFloat(-72.0, 72.0);
				}
				else
				{
					ori[0] = Math.RandomFloat(-144.0, 144.0);
				}
			}
		}

		if (isDir)
		{
			if (IsRaised())
				ori[1] = Math.RandomFloat(-3.0, 0.0);
			else if (speed == 0 || hcv)
				ori[1] = Math.RandomFloat(-36.0, 18.0);

			dir = ori.AnglesToVector();

			if (isDirWS)
				dir = dir.InvMultiply3(m_ExTransformPlayer);

			LookAtDirection(dir);
			AimAtDirection(dir);
		}
		else
		{
			LookAtPosition(pos, true);
			AimAtPosition(pos, true);
		}

		return true;
	}

	void eAI_RandomGreeting(bool excludeWatching = false)
	{
		bool manners = GetExpansionSettings().GetAI().Manners;

		int emoteId;
		int min;
		int max;

		if (manners)
			min = 4;

		if (Expansion_GetMovementSpeed() > 0.0)
			max = 8;
		else
			max = 9;  //! Only include salute if not moving

		switch (Math.RandomIntInclusive(min, max))
		{
			case 0:
				emoteId = EmoteConstants.ID_EMOTE_TAUNT;
				break;
			case 1:
				emoteId = EmoteConstants.ID_EMOTE_TAUNTELBOW;
				break;
			case 2:
				emoteId = EmoteConstants.ID_EMOTE_THROAT;
				break;
			case 3:
				emoteId = EmoteConstants.ID_EMOTE_TAUNTTHINK;
				break;
			case 4:
				emoteId = EmoteConstants.ID_EMOTE_GREETING;
				break;
			case 5:
				emoteId = EmoteConstants.ID_EMOTE_TAUNTKISS;
				break;
			case 6:
				emoteId = EmoteConstants.ID_EMOTE_POINT;
				break;
			case 7:
				if (excludeWatching)
					return;
				emoteId = EmoteConstants.ID_EMOTE_WATCHING;
				break;
			case 8:
				emoteId = EmoteConstants.ID_EMOTE_HEART;
				break;
			case 9:
				emoteId = EmoteConstants.ID_EMOTE_SALUTE;
				break;
		}

		Expansion_SetEmote(emoteId, true);
	}

	bool EnforceLOS()
	{
		//! Prevent core dump on server shutdown
		if (!GetGame())
			return false;

		if (!m_eAI_Targets.Count())
		{
			return false;
		}

		auto target = GetTarget();

		EntityAI targetEntity = target.GetEntity();

		if (!targetEntity)
		{
			return false;
		}

		auto state = m_eAI_TargetInformationStates[target.info];
		if (!state)
			return false;

		PlayerBase targetPlayer;
		if (Class.CastTo(targetPlayer, targetEntity) && targetPlayer.Expansion_HasAdminToolInvisibility())
		{
#ifdef DIAG_DEVELOPER
			eAI_UpdatePlayerIsEnemyStatus(targetPlayer, false, "target invisible");
#endif
			state.m_LOS = false;
			return false;
		}

		if (IsUnconscious())
		{
			state.m_LOS = false;
			return false;
		}

		bool isItemTarget = targetEntity.IsItemBase();

		string boneName;
		if (IsRaised())
			boneName = "Neck";
		else
			boneName = "Head";

		vector begPos = GetBonePositionWS(GetBoneIndexByName(boneName));
		vector aimOffset = target.GetAimOffset(this);
		vector dir = vector.Direction(begPos, target.GetPosition(this, true) + aimOffset);
		//! Extend LOS ray by some amount because some targets like doors have inaccurate position and ray would not hit otherwise
		vector endPos = begPos + dir + dir.Normalized() * 0.5;

		vector contactPos;
		vector contactDir;
		int contactComponent;

		set< Object > results = new set< Object >;
		float radius;
	#ifdef EXPANSION_AI_ITEM_TARGET_REQUIRE_LOS
		if (isItemTarget)
			radius = 0.15;
		else
	#endif
			radius = 0.05;
		bool hadLOS = state.m_LOS;
		state.m_LOS = DayZPhysics.RaycastRV(begPos, endPos, contactPos, contactDir, contactComponent, results, null, this, false, false, ObjIntersectView, radius);
		if (!state.m_LOS)
		{
			if (hadLOS && m_eAI_IsFightingFSM)
				GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(eAI_UpdateLeanTarget, 1000, false);

		#ifdef DIAG_DEVELOPER
			if (hadLOS)
			{
				EXTrace.Print(EXTrace.AI, this, "lost line of sight to target " + targetEntity);
				EXTrace.Print(EXTrace.AI, this, "aim offset " + aimOffset + " end pos " + endPos);
			}

			state.m_LOSRaycastHitObject = null;
			state.m_LOSRaycastHitPosition = vector.Zero;

			Expansion_DebugObject_Deferred(18, "0 0 0", "ExpansionDebugSphereSmall");
			Expansion_DebugObject_Deferred(19, "0 0 0", "ExpansionDebugSphereSmall_Red");
		#endif
			return false;
		}

		//! Reset state for further checks
		state.m_LOS = false;

		Object parent;

		if (targetPlayer && (!Class.CastTo(parent, targetPlayer.Expansion_GetParent()) || !parent.IsTransport()))
		{
			//! If targeting a player and parent is NULL or not transport, check if we are facing target
			//! (horizontal aim/look direction, not movement direction)
			vector toTargetDirection = vector.Direction(GetPosition(), endPos);
			float toTargetAngle = toTargetDirection.VectorToAngles()[0];
			float aimAngle = GetAimDirection().VectorToAngles()[0];
			if (Math.AbsFloat(ExpansionMath.AngleDiff2(toTargetAngle, aimAngle)) > 135)  //! AI is facing away
				return false;
		}

		bool isCreatureTarget = targetEntity.IsDayZCreature();

		if (results.Count() && results[0].IsBuilding())
		{
			//! Check again in case view is "blocked" by a window/other see-through object
			//! TODO: If a player is behind an Expansion BaseBuilding wall, the AI will "see" the player if he is standing next to a window position
			//! (regardless if the wall actually has windows or not or if they are closed) due to the wall's firegeo and use of ObjIntersectFire
			results.Clear();
			//Expansion_DebugObject_Deferred(1819, contactPos, "ExpansionDebugSphereSmall_Red", contactDir);
			DayZPhysics.RaycastRV(contactPos, endPos, contactPos, contactDir, contactComponent, results, null, this, false, false, ObjIntersectFire, radius);
		}

		//float targetDistSq = vector.DistanceSq(begPos, endPos);
		float contactToTargetDistSq = vector.DistanceSq(contactPos, endPos);

		DayZPlayerImplement player;

		bool sideStep;

		Object hitObj;
		EntityAI hitEntity;
		EntityAI hitEntityRoot;
		bool isCreatureHit;
		foreach (Object obj: results)
		{
			hitObj = obj;

			if (Class.CastTo(hitEntity, obj))
			{
				hitEntityRoot = hitEntity.GetHierarchyRoot();
				isCreatureHit = hitEntityRoot.IsDayZCreature();
				obj = hitEntityRoot;
			}

			//! Tree with player more than 2 m away from contact pos
			if (obj.IsTree() && contactToTargetDistSq > 4 && !state.m_SearchPositionUpdateCount)
			{
				sideStep = state.m_ThreatLevelActive >= 0.4;
				break;
			}

			//if (obj.IsBuilding())
			//{
			//	state.m_LOS = false;
			//	break;
			//}

			if (obj == targetEntity)
			{
				//! Right on target
				state.m_LOS = true;
			}
			else if (obj == parent)
			{
				state.m_LOS = true;
			}
			else
			{
				if (Class.CastTo(player, obj))
				{
					if (isItemTarget && !player.IsAlive() && targetEntity.GetHierarchyRootPlayer() == player)
					{
						//! Target is item and item is in player inventory
						state.m_LOS = true;
					}
					else if (eAI_GetTargetThreat(player.GetTargetInformation()) < 0.2)
					{
						sideStep = m_eAI_IsFightingFSM;  //! Sidestep if we are in fighting FSM
					}
				}
				else if (isCreatureTarget && isCreatureHit)
				{
					//! If object is zombie or animal but not the target or its parent, we don't care if they get shot when they are in the way
					state.m_LOS = true;
				}
				else if (((obj.IsTree() || obj.IsBush()) && contactToTargetDistSq <= 4 && state.m_ThreatLevelActive >= 0.4) || contactToTargetDistSq <= 0.0625)
				{
					//! If object is tree/bush but not the target or its parent, we don't care if they get shot when they are in the way
					state.m_LOS = true;
				}
			}

			break;
		}

		if (state.m_LOS && !hadLOS)
		{
			m_eAI_PositionOverrideTimeout = 0.0;
			if (m_eAI_LeanTarget)
				GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(eAI_ResetLeanTarget, 1000, false);
		}
		else if (hadLOS && !state.m_LOS && m_eAI_IsFightingFSM)
		{
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(eAI_UpdateLeanTarget, 1000, false);
		}

	#ifdef DIAG_DEVELOPER
		if (state.m_LOS != hadLOS)
		{
			EXTrace.Print(EXTrace.AI, this, "has line of sight to target (" + targetEntity + ")? " + state.m_LOS);
			EXTrace.Print(EXTrace.AI, this, "aim offset " + aimOffset + " end pos " + endPos);
		}

		state.m_LOSRaycastHitObject = hitObj;
		state.m_LOSRaycastHitPosition = contactPos;

		if (state.m_LOS)
		{
			Expansion_DebugObject_Deferred(18, contactPos, "ExpansionDebugSphereSmall", dir, begPos);
			Expansion_DebugObject_Deferred(19, "0 0 0", "ExpansionDebugSphereSmall_Red");
		}
		else
		{
			Expansion_DebugObject_Deferred(18, "0 0 0", "ExpansionDebugSphereSmall");
			Expansion_DebugObject_Deferred(19, contactPos, "ExpansionDebugSphereSmall_Red", dir, begPos);
		}
	#endif

		if (sideStep)
		{
			if (m_eAI_SideStepTimeout > 0 || m_MovementDirectionActive)
				return state.m_LOS;

			//! First check if we are roughly moving in target direction
			//! @note vector.Direction(GetPosition(), GetTarget().GetPosition(this)).Normalized() returns zero vector,
			//! have to first assign target position to variable. Why is this language so INCREDIBLY fucked? :-(
			vector targetPos = state.m_SearchPosition;
			vector targetDirection = vector.Direction(GetPosition(), targetPos).Normalized();
			float dot = vector.Dot(GetDirection(), targetDirection);
			if (dot >= 0.75)
			{
				eAI_ForceSideStep(1.5, hitObj, 0.0, false, true);
			}
		}

		return state.m_LOS;
	}

	//! @note when leaning, aim animation direction needs to be adjusted for character rotation, see HandleWeapons
	void eAI_UpdateLeanTarget()
	{
		eAITarget target = GetTarget();
		if (!target)
			return;

		vector p1 = GetPosition();
		vector p2 = p1 + m_PathFinding.m_PathSegmentDirection;
		vector p = target.GetPosition(this);

		if (vector.DistanceSq(p1, p) < vector.DistanceSq(p2, p))
		{
			//! Moving away from target
			p1 = p2;
			p2 = GetPosition();
		}

		if (ExpansionMath.Side(p1, p2, p) < 0)
			m_eAI_LeanTarget = -1.0;  //! target is on the right, lean left
		else
			m_eAI_LeanTarget = 1.0;  //! target is on the left, lean right
	}

	void eAI_ResetLeanTarget()
	{
		m_eAI_LeanTarget = 0.0;
	}

	void eAI_ForceSideStep(float duration, Object obj = null, float angle = 0.0, bool allowBackPedaling = true, bool sideStepCancelOnLOS = false)
	{
		bool blockedLeft;
		bool blockedRight;

		if (angle == 0.0)
		{
			eAICommandMove cmd = GetCommand_MoveAI();
			if (cmd)
			{
				blockedLeft = cmd.CheckBlockedLeft();
				blockedRight = cmd.CheckBlockedRight();
			}

			vector transform[4];
			GetTransform(transform);

			if (!blockedLeft)
				blockedLeft = m_PathFinding.IsBlocked(transform[3], transform[3] + (-0.5 * transform[0]));

			if (!blockedRight)
				blockedRight = m_PathFinding.IsBlocked(transform[3], transform[3] + (0.5 * transform[0]));

		#ifdef DIAG_DEVELOPER
			if (blockedLeft)
				Expansion_DebugObject(9999, transform[3] + (-0.5 * transform[0]) + "0 1.5 0", "ExpansionDebugBox_Red", GetDirection());
			else
				Expansion_DebugObject(9999, "0 0 0", "ExpansionDebugBox_Red", GetDirection());

			if (blockedRight)
				Expansion_DebugObject(10000, transform[3] + (0.5 * transform[0]) + "0 1.5 0", "ExpansionDebugBox_Blue", GetDirection());
			else
				Expansion_DebugObject(10000, "0 0 0", "ExpansionDebugBox_Blue", GetDirection());
		#endif

			if (blockedLeft && blockedRight)
			{
				if (!allowBackPedaling)
					return;

				//! Backpedal
				m_SideStepAngle = -180;
			}
			else if (blockedLeft || (!blockedRight && Math.RandomIntInclusive(0, 1)))
			{
				//! Go right if only blocked left or 50% chance go right if neither blocked left/right
				m_SideStepAngle = 90;
			}
			else
			{
				//! Go left
				m_SideStepAngle = -90;
			}
		}
		else
		{
			m_SideStepAngle = angle;
		}

		//! Move, b*tch, get out the way :-)
		m_eAI_SideStepObject = obj;
		m_eAI_SideStepTimeout = duration;
		m_eAI_SideStepCancelOnLOS = sideStepCancelOnLOS;

		if (m_EmoteManager.IsEmotePlaying())
			m_EmoteManager.ServerRequestEmoteCancel();

		EXTrace.Print(EXTrace.AI, this, "sidestep " + m_SideStepAngle + " " + obj);
		OverrideMovementDirection(true, m_SideStepAngle);
	}

	void eAI_CancelSidestep()
	{
		m_eAI_SideStepTimeout = 0.0;
		eAI_ResetSidestep();
	}

	protected void eAI_ResetSidestep()
	{
		EXTrace.Print(EXTrace.AI, this, "sidestep 0");

		m_SideStepAngle = 0.0;
		m_eAI_SideStepCancelOnLOS = false;
		m_eAI_SideStepObject = null;

		OverrideMovementDirection(true, 0.0);
		m_eAI_ResetMovementDirectionActive = true;  //! Next call to cmdhandler

		m_PathFinding.StopOverride();
	}

	bool eAI_IsSideStepping()
	{
		return m_eAI_SideStepTimeout > 0.0;
	}

	float eAI_GetSideStepTimeout()
	{
		return m_eAI_SideStepTimeout;
	}

	override bool eAI_IsSideSteppingObstacles()
	{
		eAICommandMove cmd = GetCommand_MoveAI();
		return cmd && cmd.IsSideSteppingObstacles();
	}

	bool eAI_IsSideSteppingVehicle()
	{
		if (m_eAI_SideStepObject && m_eAI_SideStepObject.IsTransport())
			return true;

		return false;
	}

	bool eAI_IsUnreachable(float distanceSq, float minDistanceSq, vector position)
	{
		if (distanceSq > minDistanceSq && Math.IsPointInCircle(position, Math.Sqrt(minDistanceSq), GetPosition()))
			return true;

		return false;
	}

	void eAI_ThreatOverride(EntityAI entity, bool state)
	{
	#ifdef EXTRACE_DIAG
		//EXTrace.Print(EXTrace.AI, this, "eAI_ThreatOverride " + entity + " " + state);
		auto trace = EXTrace.StartStack(EXTrace.AI, this, entity.ToString() + " " + state);
	#endif

		if (state)
			m_eAI_ThreatOverride[entity] = true;
		else
			m_eAI_ThreatOverride.Remove(entity);
	}

	bool eAI_GetThreatOverride(EntityAI entity)
	{
		return m_eAI_ThreatOverride[entity];
	}

	bool eAI_HasLOS()
	{
		return eAI_HasLOS(GetTarget());
	}

	/**
	 * @brief Return whether or not we have LOS to this target
	 * 
	 * @note WARNING: When not passing in current target, returns cached (last calculated) LOS info which may be stale!
	 * Use with care!
	 */
	bool eAI_HasLOS(eAITarget target)
	{
		if (!target)
			return false;

		return eAI_HasLOS(target.info);
	}

	bool eAI_HasLOS(eAITargetInformation info)
	{
		if (!info.IsEntity())
			return false;

		auto state = m_eAI_TargetInformationStates[info];
		if (!state)
			return false;

		return state.m_LOS;
	}

	void eAI_SetIsFightingFSM(bool state)
	{
	#ifdef DIAG_DEVELOPER
		EXTrace.Print(EXTrace.AI, this, "SetIsFightingFSM " + state);
	#endif
		m_eAI_IsFightingFSM = state;
		GetGroup().SetIsInCombat(state);

		eAI_SetShouldTakeCover(state);

		if (m_eAI_IsFightingFSM && GetExpansionSettings().GetAI().MemeLevel > 9000)
			eAI_PlayRandomLoveSound();
	}

	void eAI_SetShouldTakeCover(bool state)
	{
		if (state)
		{
			m_eAI_ShouldTakeCover = true;
			m_eAI_UpdatePotentialCoverObjects = true;
		}
		else
		{
			m_eAI_ShouldTakeCover = false;
		}
	}

	void eAI_LeaveCover()
	{
		if (m_eAI_CurrentCoverObject)
		{
			s_eAI_TakenCoverObjects.RemoveItem(m_eAI_CurrentCoverObject);
			m_eAI_CurrentCoverObject = null;
		}
	}

	void eAI_PlayRandomLoveSound()
	{
		if (Math.RandomInt(0, 2))
			s_eAI_LoveSound01_SoundSet.Play(this);
		else
			s_eAI_LoveSound02_SoundSet.Play(this);
	}

	override void OnScheduledTick(float deltaTime)
	{
		if (!IsPlayerSelected() || !IsAlive())
			return;

		if (m_ModifiersManager)
			m_ModifiersManager.OnScheduledTick(deltaTime);
		//if (m_NotifiersManager)
			//m_NotifiersManager.OnScheduledTick();
		//! These send RPC to identity which is NULL for AI and thus affects all actual players HUDs (shows AI values instead of player)
		// if( m_TrasferValues )
			//m_TrasferValues.OnScheduledTick(deltaTime);
		// if( m_VirtualHud )
			//m_VirtualHud.OnScheduledTick();
		if (GetBleedingManagerServer())
			GetBleedingManagerServer().OnTick(deltaTime);
		if ( m_Environment )
			m_Environment.Update(deltaTime);

		// Check if electric device needs to be unplugged
		ItemBase heldItem = GetItemInHands();
		if (heldItem && heldItem.HasEnergyManager() && heldItem.GetCompEM().IsPlugged())
		{
			// Now we know we are working with an electric device which is plugged into a power source.
			EntityAI placed_entity = heldItem;

			// Unplug the device when the player is too far from the power source.
			placed_entity.GetCompEM().UpdatePlugState();
		}

		m_Expansion_UpdateTime += deltaTime;
		if ( m_Expansion_UpdateTime > m_Expansion_UpdateTimeThreshold )
		{
			m_Expansion_UpdateTime = 0;
			Expansion_OnAIUpdate();
		}

	}

	// We should integrate this into ReloadWeapon
	void ReloadWeaponAI(EntityAI weapon, EntityAI magazine)
	{
#ifdef EXTRACE
		EXTrace trace;
#endif

		m_eAI_PositionOverrideTimeout = 0;  //! Reset so we prioritize taking cover during reload

		// CF_Log.Debug(this.ToString() + "(DayZPlayerInstanceType." + GetInstanceType().ToString() + ") is trying to reload " + magazine.ToString() + " into " + weapon.ToString());
		eAIActionManager mngr_ai;
		CastTo(mngr_ai, GetActionManager());

		if (mngr_ai && FirearmActionLoadMultiBulletRadial.Cast(mngr_ai.GetRunningAction()))
		{
#ifdef EXTRACE
			trace = EXTrace.Start0(EXTrace.AI, this, "Interrupting current reload action for " + weapon);
#endif
			mngr_ai.Interrupt();
		}
		else if (GetHumanInventory().GetEntityInHands() != magazine)
		{
			Weapon_Base wpn;
			Magazine mag;
			Class.CastTo(wpn, weapon);
			Class.CastTo(mag, magazine);
			int mi = wpn.GetCurrentMuzzle();
			Magazine currentMag = wpn.GetMagazine(mi);
			EXTrace.Print(EXTrace.AI, this, "ReloadWeaponAI - wpn " + wpn + " attached mag " + currentMag + " internal mag cartridge count " + wpn.GetInternalMagazineCartridgeCount(mi) + " - reload from " + mag);
			if (GetWeaponManager().CanUnjam(wpn))
			{
#ifdef EXTRACE
				//! NEVER unjam while reloading, unjamming should ALWAYS be handled by unjamming state in FSM so that weapon manager timeout can be dealt with
				trace = EXTrace.Start0(EXTrace.AI, this, "Not reloading jammed " + wpn);
#endif
				return;
			}
			else if (wpn.IsChamberFiredOut(mi) && wpn.GetInternalMagazineCartridgeCount(mi) > 0 && GetWeaponManager().CanEjectBullet(wpn))
			{
#ifdef EXTRACE
				trace = EXTrace.Start0(EXTrace.AI, this, "Ejecting bullet and chambering from internal mag " + wpn);
#endif
				GetWeaponManager().EjectBullet();
				return;
			}
			else if (GetWeaponManager().CanAttachMagazine(wpn, mag))
			{
#ifdef EXTRACE
				trace = EXTrace.Start0(EXTrace.AI, this, "Attaching mag to " + wpn + " " + mag);
#endif
				GetWeaponManager().AttachMagazine(mag);
				return;
			}
			else if (GetWeaponManager().CanSwapMagazine(wpn, mag))
			{
#ifdef EXTRACE
				trace = EXTrace.Start0(EXTrace.AI, this, "Swapping mag " + wpn + " " + mag);
#endif
				GetWeaponManager().SwapMagazine(mag);
				return;
			}
			else if (mag && currentMag && currentMag.GetAmmoCount() == 0 && GetWeaponManager().CanDetachMagazine(wpn, currentMag))
			{
				InventoryLocation il = new InventoryLocation;

				if (mag.IsAmmoPile() && GetInventory().FindFreeLocationFor(currentMag, FindInventoryLocationType.CARGO, il))
				{
#ifdef EXTRACE
					trace = EXTrace.Start0(EXTrace.AI, this, "Detaching mag " + wpn + " " + currentMag);
#endif

					//! TODO: Proper fill up of current mag from ammo pile after detach (put gun away, take mag to hand, fill mag, reattach to wpn)
					eAI_FillMag(currentMag, mag, true);

					GetWeaponManager().DetachMagazine(il);

					//! Force re-evaluation of mag to use for reloading
					typename removeType = wpn.Type();
					EXTrace.Print(EXTrace.AI, this, "ReloadWeaponAI - removing " + removeType + " " + m_eAI_EvaluatedFirearmTypes[removeType]);
					m_eAI_EvaluatedFirearmTypes.Remove(removeType);
					return;
				}
				else if (!mag.IsAmmoPile())
				{
#ifdef EXTRACE
					trace = EXTrace.Start0(EXTrace.AI, this, "Attached mag doesn't fit in inventory, dropping " + wpn + " " + currentMag);
#endif

					//! Could also use PredictiveDropEntity, but let's be explicit for maximum control
					vector m4[4];
					Math3D.MatrixIdentity4(m4);
					
					//! We don't care if a valid transform couldn't be found, we just want to preferably use it instead of placing on the player
					GameInventory.PrepareDropEntityPos(this, currentMag, m4, false, GameConstants.INVENTORY_ENTITY_DROP_OVERLAP_DEPTH);
					il.SetGround(currentMag, m4);

					GetWeaponManager().DetachMagazine(il);
					return;
				}

				//! If mag to reload from is ammo pile but current mag doesn't fit in inventory,
				//! we fall through here to load single bullet or eject casing
			}

			if (GetWeaponManager().CanLoadBullet(wpn, mag))
			{
#ifdef EXTRACE
				trace = EXTrace.Start0(EXTrace.AI, this, "Loading bullet " + wpn + " " + mag);
#endif
				// GetWeaponManager().LoadMultiBullet(mag);

				if (mag.IsAmmoPile() && eAI_IsTargetUnlimitedReload() && mag.GetAmmoMax() == 1)
					GetInventory().CreateInInventory(mag.GetType());

				ActionTarget atrg = new ActionTarget(mag, this, -1, vector.Zero, -1.0);
				if (mngr_ai && !mngr_ai.GetRunningAction() && mngr_ai.GetAction(FirearmActionLoadMultiBulletRadial).Can(this, atrg, wpn))
					mngr_ai.PerformActionStart(mngr_ai.GetAction(FirearmActionLoadMultiBulletRadial), atrg, wpn);
			}
			else if ((wpn.IsChamberEmpty(mi) || wpn.IsChamberFiredOut(mi)) && GetWeaponManager().CanEjectBullet(wpn))
			{
#ifdef EXTRACE
				//! @note WeaponManager::EjectBullet also deals with loading bullet from attached or internal mag into chamber
				trace = EXTrace.Start0(EXTrace.AI, this, "Ejecting bullet/cycling action " + wpn + " chamber empty? " + wpn.IsChamberEmpty(mi) + " fired out? " + wpn.IsChamberFiredOut(mi));
#endif
				GetWeaponManager().EjectBullet();
			}
			else
			{
				if (mag)
					EXError.Error(this, "Can't reload " + wpn + " from " + mag + " (" + mag.GetType() + ") - not implemented");
				else
					EXError.Error(this, "Can't reload " + wpn + " - not implemented");
			}
		}
	}

	override void QuickReloadWeapon(EntityAI weapon)
	{
		ReloadWeaponAI(weapon, GetMagazineToReload(weapon));
	}

	/**
	 * @brief Fill mag from ammo source
	 * 
	 * @param magToFill
	 * @param ammoSource
	 * @param deleteSourceIfDepleted If ammo source is depleted during this operation, it is deleted
	 * 
	 * @return number of transferred cartridges
	 */
	int eAI_FillMag(Magazine magToFill, Magazine ammoSource, bool deleteSourceIfDepleted = false)
	{
		int ammoCount = ammoSource.GetAmmoCount();
		float totalDamage;
		int reloadAmmo = MiscGameplayFunctions.Expansion_TransferCartridges(ammoSource, magToFill, ammoCount, false, totalDamage);

	#ifdef DIAG_DEVELOPER
		EXTrace.Print(EXTrace.AI, this, "Filled mag " + magToFill + " with " + reloadAmmo + " cartridges from " + ammoSource);
	#endif

		ammoCount -= reloadAmmo;
		if (ammoCount == 0 && deleteSourceIfDepleted && ammoSource && !ammoSource.IsSetForDeletion())
			GetGame().ObjectDelete(ammoSource);

		return reloadAmmo;
	}

	/**
	 * @brief Fill any non-full mag in inventory which is compatible with weapon from ammo source
	 * 
	 * @param weapon
	 * @param ammoSource
	 * @param deleteSourceIfDepleted If ammo source is depleted during this operation, it is deleted
	 * 
	 * @note If ammo source is a mag and its health is higher and ammo max is higher or equal than mag in inv,
	 * the operation is swapped and ammo source is filled from mag
	 * 
	 * @return number of transferred cartridges
	 */
	int eAI_FillAnyCompatibleMag(Weapon_Base weapon, Magazine ammoSource, bool deleteSourceIfDepleted = false)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI, this);
#endif 
		
		int numberOfTransferredCartridges;

		foreach (Magazine magToFill: m_eAI_Magazines)
		{
			if (!magToFill)
				continue;

			if (magToFill.IsAmmoPile())
				continue;

			if (magToFill.IsFullQuantity())
				continue;

			if (!magToFill.IsCompatiableAmmo(ammoSource))
				continue;

			if (!ammoSource.IsAmmoPile() && ammoSource.GetHealthLevel() < magToFill.GetHealthLevel() && ammoSource.GetAmmoMax() >= magToFill.GetAmmoMax())
			{
				if (weapon.TestAttachMagazine(weapon.GetCurrentMuzzle(), ammoSource, false, true))
				{
					numberOfTransferredCartridges += eAI_FillMag(ammoSource, magToFill, deleteSourceIfDepleted);
					if (!magToFill || magToFill.IsSetForDeletion() || magToFill.GetAmmoCount() == 0)
						break;

					continue;
				}
			}

			if (weapon.TestAttachMagazine(weapon.GetCurrentMuzzle(), magToFill, false, true))
			{
				numberOfTransferredCartridges += eAI_FillMag(magToFill, ammoSource, deleteSourceIfDepleted);
				if (!ammoSource || ammoSource.IsSetForDeletion() || ammoSource.GetAmmoCount() == 0)
					break;
			}
		}

		return numberOfTransferredCartridges;
	}

	// Returns true only if there is a weapon in hands, and the weapon has no rounds ready,
	// and the inventory does have magazines to reload to.
	bool ShouldReload(out EntityAI magazine, out EntityAI weapon)
	{
		Weapon weapon_in_hands;
		if (!Class.CastTo(weapon_in_hands, GetItemInHands()))
			return false;

		int mi = weapon_in_hands.GetCurrentMuzzle();
		Magazine mag = weapon_in_hands.GetMagazine(mi);
		if (mag && mag.GetAmmoCount() > 0)
			return false;

		magazine = GetMagazineToReload(weapon_in_hands);
		weapon = weapon_in_hands;
		if (!magazine || !weapon)
			return false;

		return true;
	}

#ifdef CF_DEBUG
	override bool CF_OnDebugUpdate(CF_Debug instance, CF_DebugUI_Type type)
	{
		super.CF_OnDebugUpdate(instance, type);

		instance.Add("FSM", m_FSM);
		if (m_FSM)
		{
			instance.IncrementTab();
			instance.Add(m_FSM);
			instance.DecrementTab();
		}

		return true;
	}
#endif

	//! @note: do not call frequently
	void Expansion_OnAIUpdate()
	{
		Expansion_UpdateVisibility();
	}

	void Expansion_TryTurningOnAnyLightsOrNVG(out float nightVisibility, bool skipNonNVG = false, bool skipNVG = false)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI, eAIBase);
#endif 
		
		ItemBase itembs;
		
		itembs = GetItemOnHead();
		if ( itembs && itembs.Expansion_TryTurningOnAnyLightsOrNVG(nightVisibility, this, skipNonNVG, skipNVG) )
			m_Expansion_ActiveVisibilityEnhancers.Insert(itembs);
		
		itembs = GetItemOnSlot("Eyewear");
		if ( itembs && itembs.Expansion_TryTurningOnAnyLightsOrNVG(nightVisibility, this, skipNonNVG, skipNVG) )
			m_Expansion_ActiveVisibilityEnhancers.Insert(itembs);

		itembs = GetItemInHands();
		if ( itembs && itembs.Expansion_TryTurningOnAnyLightsOrNVG(nightVisibility, this, skipNonNVG, skipNVG) )
			m_Expansion_ActiveVisibilityEnhancers.Insert(itembs);

		m_Expansion_TriedTurningOnVisibilityEnhancers = true;
	}

	void Expansion_TryTurningOffAnyLightsOrNVG(bool skipNVG = false)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI, eAIBase);
#endif 
		
		ItemBase itembs;
		
		itembs = GetItemOnHead();
		if ( itembs && itembs.Expansion_TryTurningOffAnyLightsOrNVG(this, skipNVG) )
			m_Expansion_ActiveVisibilityEnhancers.RemoveItemUnOrdered(itembs);
		
		itembs = GetItemOnSlot("Eyewear");
		if ( itembs && itembs.Expansion_TryTurningOffAnyLightsOrNVG(this, skipNVG) )
			m_Expansion_ActiveVisibilityEnhancers.RemoveItemUnOrdered(itembs);

		itembs = GetItemInHands();
		if ( itembs && itembs.Expansion_TryTurningOffAnyLightsOrNVG(this, skipNVG) )
			m_Expansion_ActiveVisibilityEnhancers.RemoveItemUnOrdered(itembs);
	}

	void Expansion_UpdateVisibility(bool force = false)
	{
		float fogVisibility;
		float overcastVisibility;
		float rainVisibility;
		#ifndef DAYZ_1_25
		float snowVisibility;
		#endif
		float daylightVisibility = g_Game.GetWorld().GetSunOrMoon();  //! 0/1 Night/Day

		#ifdef DAYZ_1_25
		m_Environment.Expansion_GetWeatherVisibility(fogVisibility, overcastVisibility, rainVisibility);
		#else
		m_Environment.Expansion_GetWeatherVisibility(fogVisibility, overcastVisibility, rainVisibility, snowVisibility);
		#endif
		if (!fogVisibility)
			EXPrint(ToString() + " ERROR: Fog visibility is zero!");
		if (!overcastVisibility)
			EXPrint(ToString() + " ERROR: Overcast visibility is zero!");
		if (!rainVisibility)
			EXPrint(ToString() + " ERROR: Rain visibility is zero!");
		#ifndef DAYZ_1_25
		if (!snowVisibility)
			EXPrint(ToString() + " ERROR: Snow visibility is zero!");
		#endif

		if (force || daylightVisibility != m_Expansion_DaylightVisibility)
		{
			m_Expansion_DaylightVisibility = daylightVisibility;
			if (daylightVisibility)
				m_Expansion_Visibility = 1.0;
			else if (CfgGameplayHandler.GetLightingConfig())
				m_Expansion_Visibility = 0.01;  //! Assume dark night, visibility 10 m
			else
				m_Expansion_Visibility = 0.1;  //! Assume bright night, visibility 100 m

			m_Expansion_TriedTurningOnVisibilityEnhancers = false;
		}

		if (!daylightVisibility)
		{
			if (force || !m_Expansion_TriedTurningOnVisibilityEnhancers)
			{
				//! Try switching on any lights or NVG at night
				Expansion_TryTurningOnAnyLightsOrNVG(m_Expansion_Visibility);
				EXTrace.Print(EXTrace.AI, this, "nighttime base visibility " + m_Expansion_Visibility);
			}
		}
		else if (m_Expansion_ActiveVisibilityEnhancers.Count())
		{
			Expansion_TryTurningOffAnyLightsOrNVG();
		}

		if (!m_Expansion_Visibility)
			EXPrint(ToString() + " ERROR: Base visibility is zero!");

		float visibilityLimit = m_eAI_ThreatDistanceLimit * 0.001;
		if (!visibilityLimit)
			EXPrint(ToString() + " ERROR: Visibility limit is zero! Threat distance limit: " + m_eAI_ThreatDistanceLimit);
		if (visibilityLimit > 0 && m_Expansion_Visibility > visibilityLimit)
			m_Expansion_Visibility = visibilityLimit;

		//! Limit visibility in contaminated areas due to gas clouds
		if (m_Expansion_Visibility > 0.2 && GetModifiersManager().IsModifierActive(eModifiers.MDF_AREAEXPOSURE))
			m_Expansion_Visibility = 0.2;  //! 200 m

		//! Final visibility
		#ifdef DAYZ_1_25
		m_Expansion_Visibility = Math.Min(Math.Min(fogVisibility, Math.Min(overcastVisibility, rainVisibility)), m_Expansion_Visibility);
		#else
		m_Expansion_Visibility = Math.Min(Math.Min(fogVisibility, Math.Min(overcastVisibility, Math.Min(rainVisibility, snowVisibility))), m_Expansion_Visibility);
		#endif
		m_Expansion_VisibilityDistThreshold = 900 * m_Expansion_Visibility;
	}

	float Expansion_GetVisibility(float distance)
	{
		if (distance > m_Expansion_VisibilityDistThreshold)
			return ExpansionMath.PowerConversion(1100 * m_Expansion_Visibility, m_Expansion_VisibilityDistThreshold, distance, 0.0, 1.0, 2.0);

		return 1.0;
	}

	float Expansion_GetVisibilityDistThreshold()
	{
		return m_Expansion_VisibilityDistThreshold;
	}

	override void HandleWeapons(float pDt, Entity pInHands, HumanInputController pInputs, out bool pExitIronSights)
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_3(this, "HandleWeapons").Add(pDt).Add(pInHands).Add(pInputs);
#endif

		HumanCommandWeapons hcw = GetCommandModifier_Weapons();
		GetDayZPlayerInventory().HandleWeaponEvents(pDt, pExitIronSights);

		Weapon_Base weapon;
		Class.CastTo(weapon, pInHands);

		if (m_WeaponRaised != m_WeaponRaisedPrev)
		{
			m_WeaponRaisedPrev = m_WeaponRaised;
			m_WeaponRaisedTimer = 0.0;

			if (g_Game.IsServer() && m_eAI_Command.IsInherited(eAICommandMove))
			{
				AnimSetBool(m_ExpansionST.m_VAR_Raised, m_WeaponRaised);
				if (!m_WeaponRaised)
					AnimSetBool(m_ExpansionST.m_VAR_ADS, false);
			}
		}

		if (m_WeaponRaised || g_Game.IsClient())
		{
			if (g_Game.IsServer())
				m_WeaponRaisedTimer += pDt;

			vector aimTargetRelAngles = m_eAI_AimDirectionTarget_ModelSpace.VectorToAngles();

			bool instant;

			if (g_Game.IsServer() && m_eAI_AimDirectionPrev != vector.Zero)
			{
				eAICommandMove cmd = GetCommand_MoveAI();
				if (cmd)
				{
					//! Adjust aim when turning
					float diff = ExpansionMath.AngleDiff2(m_eAI_AimDirectionPrev.VectorToAngles()[0], GetAimDirectionTarget().VectorToAngles()[0]);
					float diffAbs = Math.AbsFloat(diff);
					if (diffAbs <= 10.0)
					{
						instant = true;
					}
				}
			}

			//! We want to interpolate rel angles for aiming! Otherwise, if the conversion to rel angles happens later,
			//! there will be a sudden jump in the unit's rotation between 180 and -180 due to the way the animation is set up
			aimTargetRelAngles[0] = ExpansionMath.RelAngle(aimTargetRelAngles[0]);
			aimTargetRelAngles[1] = ExpansionMath.RelAngle(aimTargetRelAngles[1]);

			aimTargetRelAngles[1] = Math.Clamp(aimTargetRelAngles[1], -85.0, 85.0);  //! Valid range is [-85, 85]

			//TODO: quaternion slerp instead for better, accurate results
			m_eAI_AimRelAngles = ExpansionMath.InterpolateAngles(m_eAI_AimRelAngles, aimTargetRelAngles, pDt, Math.RandomFloat(3.0, 5.0), Math.RandomFloat(1.0, 3.0));
			if (instant)
				m_eAI_AimRelAngles[0] = aimTargetRelAngles[0];

			GetAimingProfile().Update();
			vector direction = GetAimingProfile().GetAimDirection();
			vector orientation = direction.VectorToAngles();
			m_eAI_AimRelAngleLR = orientation[0];
			m_eAI_AimRelAngleUD = orientation[1];

		/*
		#ifndef SERVER
		#ifdef DIAG_DEVELOPER
			vector position = GetBonePositionWS(GetBoneIndexByName("neck"));

			vector points[2];
			points[0] = position;
			points[1] = position + (direction * 1000.0);
			m_Expansion_DebugShapes.Insert(Shape.CreateLines(COLOR_BLUE, ShapeFlags.VISIBLE, points, 2));
		#endif
		#endif
		*/

			if (g_Game.IsServer())
			{
				float dist = vector.Distance(GetBonePositionWS(GetBoneIndexByName("neck")), m_eAI_AimPosition_WorldSpace);
				float distClamped = Math.Clamp(dist, 1.0, 360.0);

				float aimX = ExpansionMath.RelAngle(ExpansionMath.AbsAngle(m_eAI_AimRelAngles[0]) + (-15.0 / distClamped));
				float aimY = m_eAI_AimRelAngles[1];

				//! When leaning, aim animation direction needs to be adjusted for character rotation
				if (m_eAI_Lean)
					aimY += Math.Clamp(aimX, -45.0, 45.0) * Math.Clamp(m_eAI_Lean, -1.0, 1.0);

				AnimSetFloat(m_ExpansionST.m_VAR_AimX, aimX);
				AnimSetFloat(m_ExpansionST.m_VAR_AimY, aimY);

				bool ads;
				if (weapon && !GetWeaponManager().IsRunning() && Math.AbsFloat(ExpansionMath.AngleDiff2(m_eAI_AimRelAngles[0], aimTargetRelAngles[0])) < 45.0)
				{
					ItemOptics optics = weapon.GetAttachedOptics();

					float zeroingDistanceZoomMin;

					if (optics)
						zeroingDistanceZoomMin = optics.GetZeroingDistanceZoomMin();
					else
						zeroingDistanceZoomMin = weapon.GetZeroingDistanceZoomMin(weapon.GetCurrentMuzzle());

					if (zeroingDistanceZoomMin * 0.3 < dist)
						ads = true;
				}

				AnimSetBool(m_ExpansionST.m_VAR_ADS, ads);
			}
		}
		else if (g_Game.IsServer())
		{
			//! Interpolate to look direction if not raised so the next time we raise it animates in the direction we are looking

			vector lookTargetRelAngles = m_eAI_LookDirectionTarget_ModelSpace.VectorToAngles();

			lookTargetRelAngles[0] = ExpansionMath.RelAngle(lookTargetRelAngles[0]);
			lookTargetRelAngles[1] = ExpansionMath.RelAngle(lookTargetRelAngles[1]);

			lookTargetRelAngles[1] = Math.Clamp(lookTargetRelAngles[1], -85.0, 85.0);  //! Valid range is [-85, 85]

			m_eAI_AimRelAngles = ExpansionMath.InterpolateAngles(m_eAI_AimRelAngles, lookTargetRelAngles, pDt, Math.RandomFloat(3.0, 5.0), Math.RandomFloat(1.0, 3.0));
		}

		m_eAI_AimDirectionPrev = GetAimDirection();
	}

	// As with many things we do, this is an almagomation of the client and server code
	override void CheckLiftWeapon()
	{
		if (!GetGame().IsServer())
			return;

		Weapon_Base weap;
		if (Weapon_Base.CastTo(weap, GetItemInHands()))
		{
			m_LiftWeapon_player = weap.LiftWeaponCheck(this);
		}

		return;
	}

	bool CanRaiseWeapon()
	{
		if (IsClimbing() || IsFalling() || IsSwimming())
			return false;

		if (eAI_IsSideSteppingVehicle())
			return false;

		return true;
	}

	// @param true to put weapon up, false to lower
	void RaiseWeapon(bool up = true)
	{
		m_WeaponRaised = up;
		if (!up)
			m_eAI_QueuedShots = 0;
	}

	override bool IsRaised()
	{
		return m_WeaponRaised;
	}

	override bool IsWeaponRaiseCompleted()
	{
		return m_WeaponRaisedTimer > 0.5;
	}

	void eAI_AddInteractingPlayer(Man player)
	{
		m_eAI_InteractingPlayers.Insert(player);
	}

	void eAI_RemoveInteractingPlayer(Man player)
	{
#ifdef DAYZ_1_20
		int remove_index = m_eAI_InteractingPlayers.Find(player);
		if (remove_index >= 0)
			m_eAI_InteractingPlayers.Remove(remove_index);
#else
		//! 1.21+
		m_eAI_InteractingPlayers.RemoveItem(player);
#endif
	}

	bool eAI_HasInteractingPlayers()
	{
		return m_eAI_InteractingPlayers.Count() > 0;
	}

	override void SetActions(out TInputActionMap InputActionMap)
	{
		super.SetActions(InputActionMap);

		AddAction(eAIActionTakeItemToHands, InputActionMap);
		AddAction(eAIActionTakeItem, InputActionMap);
		AddAction(eAIActionDropItem, InputActionMap);
	}

	ActionBase StartAction(typename actionType, ActionTarget target, ItemBase mainItem = null)
	{
		ActionBase action = m_eActionManager.GetAction(actionType);

		if (!mainItem)
			mainItem = GetItemInHands();

		m_eActionManager.PerformActionStart(action, target, mainItem);

		m_eAI_PositionOverrideTimeout = 0;  //! Reset so we prioritize taking cover during actions

		return action;
	}

	ActionBase StartActionObject(typename actionType, Object target, ItemBase mainItem = null)
	{
		return StartAction(actionType, new ActionTarget(target, null, -1, vector.Zero, -1.0), mainItem);
	}

	// @param LookWS a position in WorldSpace to look at
	void LookAtPosition(vector pPositionWS, bool recalculate = true)
	{
		m_eAI_LookPosition_WorldSpace = pPositionWS;
		m_eAI_LookDirection_Recalculate = recalculate;
	}

	// @param AimWS a position in WorldSpace to Aim at
	void AimAtPosition(vector pPositionWS, bool recalculate = true)
	{
		m_eAI_AimPosition_WorldSpace = pPositionWS;
		m_eAI_AimDirection_Recalculate = recalculate;
	}

	void LookAtDirection(vector pDirectionMS)
	{
		m_eAI_LookDirectionTarget_ModelSpace = pDirectionMS;
		m_eAI_LookDirection_Recalculate = false;
	}

	void AimAtDirection(vector pDirectionMS)
	{
		m_eAI_AimDirectionTarget_ModelSpace = pDirectionMS;
		m_eAI_AimDirection_Recalculate = false;
	}

	vector GetLookRelAngles()
	{
		return m_eAI_LookRelAngles;
	}

	vector GetLookDirection()
	{
		return m_eAI_LookRelAngles.AnglesToVector().Multiply3(m_ExTransformPlayer);
	}

	bool GetLookDirectionRecalculate()
	{
		return m_eAI_LookDirection_Recalculate;
	}

	//! Used to calculate aim in aiming profile
	vector GetAimRelAngles()
	{
		return m_eAI_AimRelAngles;
	}

	vector GetAimDirection()
	{
		return m_eAI_AimRelAngles.AnglesToVector().Multiply3(m_ExTransformPlayer);
	}

	vector GetAimDirectionTarget()
	{
		return m_eAI_AimDirectionTarget_ModelSpace.Multiply3(m_ExTransformPlayer);
	}

	//! Only valid while weapon raised
	vector GetWeaponAimDirection()
	{
		vector aimRelAngles = Vector(m_eAI_AimRelAngleLR, m_eAI_AimRelAngleUD, 0.0);
		return aimRelAngles.AnglesToVector();
	}

	override vector Expansion_GetAimDirection()
	{
		return GetAimDirection();
	}

	override vector GetAimPosition()
	{
		return m_eAI_AimPosition_WorldSpace;
	}

	bool GetAimDirectionRecalculate()
	{
		return m_eAI_AimDirection_Recalculate;
	}

	override bool AimingModel(float pDt, SDayZPlayerAimingModel pModel)
	{
		return false;
	}

	override bool HeadingModel(float pDt, SDayZPlayerHeadingModel pModel)
	{
		if (GetCommand_VehicleAI())
		{
			m_fLastHeadingDiff = 0;

			return false;
		}

		pModel.m_fHeadingAngle = GetOrientation()[0] * Math.DEG2RAD;
		pModel.m_fOrientationAngle = GetOrientation()[0] * Math.DEG2RAD;
		//Print(pModel.m_fHeadingAngle);
		//GetMovementState(m_MovementState);
		//return DayZPlayerImplementHeading.RotateOrient(pDt, pModel, m_fLastHeadingDiff);
		return true;
	}

	override void OnUnconsciousUpdate(float pDt, int last_command)
	{
		m_UnconsciousTime += pDt;

		if (GetGame().IsServer())
		{
			int shock_simplified = SimplifyShock();

			if (m_ShockSimplified != shock_simplified)
			{
				m_ShockSimplified = shock_simplified;
				SetSynchDirty();
			}

			if (m_UnconsciousTime > PlayerConstants.UNCONSCIOUS_IN_WATER_TIME_LIMIT_TO_DEATH && last_command == DayZPlayerConstants.COMMANDID_SWIM)
			{
				SetHealth("", "", -100);
			}
		}
	}

	//! Hack to make AI responsive again when weapon manager or action bugs out.
	//! Starts the damage hit animation (fullbody forward) w/o actually applying any damage, afterwards it should be fine again.
	//! @note (for documentation purposes) other tried ands tested workarounds that are less desirable:
	//! - Send AI uncon and wake it up again (more awkward and takes more time)
	//! - Let AI fall a short distance (unreliable in fixing the bug)
	void eAI_Unbug(string what)
	{

#ifdef DIAG_DEVELOPER
		string msg = "Action timed out for " + Debug.GetDebugName(this) + " while trying to " + what + ", item in hands " + Debug.GetDebugName(GetItemInHands());
		EXTrace.Print(true, this, msg);
		ExpansionNotification("ACTION TIMEOUT", msg).Error();
#endif

		EXTrace.Print(true, this, "Applying SMACK OF GOD (='-')-o )'~')");
		StartCommand_Damage(1, 180);
	}

	override void OnUnconsciousStart()
	{
		eAI_DropItemInHandsImpl(false, false, false);

		super.OnUnconsciousStart();
	}

	bool eAI_DropItemInHands(bool useAction = true)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI, this);
#endif 
		
		ItemBase itemInHands = GetItemInHands();
		if (itemInHands)
			return eAI_DropItem(itemInHands, useAction);

		return false;
	}

	bool eAI_DropItemInHandsImpl(bool overrideThreat = false, bool switchOff = true, bool remember = true)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI, this);
#endif 

		ItemBase itemInHands = GetItemInHands();
		if (itemInHands)
			return eAI_DropItemImpl(itemInHands, overrideThreat, switchOff, remember);

		return false;
	}

	bool eAI_DropItem(ItemBase item, bool useAction = true, bool switchOff = true, bool remember = true)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI, this, "" + item);
#endif 

		if (!useAction)
			return eAI_DropItemImpl(item, eAI_GetThreatOverride(item), switchOff, remember);
		else if (StartAction(eAIActionDropItem, null, item))
			return true;

		return false;
	}

	bool eAI_DropItemImpl(ItemBase item, bool overrideThreat = false, bool switchOff = true, bool remember = true)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI, this, "" + item);
#endif 

		InventoryLocation il_dst = new InventoryLocation();

		GameInventory.SetGroundPosByOwner(this, item, il_dst);

		if (switchOff)
			item.Expansion_TryTurningOffAnyLightsOrNVG(this);

		//! 1) Remove from active visibility enhancers if present
		m_Expansion_ActiveVisibilityEnhancers.RemoveItemUnOrdered(item);

		bool result;

		if (item != GetItemInHands())
		{
			if (eAI_TakeItemToLocation(item, il_dst))
			{
				result = true;

				if (Class.CastTo(m_eAI_LastDroppedItem, item))
					eAI_ThreatOverride(m_eAI_LastDroppedItem, overrideThreat);

				item.SetLifetime(300);
			}
		}
		else
		{
			EntityAI dst = Expansion_CloneItemToLocation(item, il_dst);
			if (dst)
			{
				result = true;

				if (Class.CastTo(m_eAI_LastDroppedItem, dst))
					eAI_ThreatOverride(m_eAI_LastDroppedItem, overrideThreat);

				dst.SetLifetime(300);
			}
		}

		//! 2) Update visibility regardless of result because of 1)
		Expansion_UpdateVisibility(true);

		if (result && remember)
			eAI_RememberRecentlyDroppedItem(m_eAI_LastDroppedItem);

		return result;
	}

	bool eAI_TakeItemToHands(ItemBase item, bool useAction = true)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI, this, "" + item);
#endif 

		if (item.GetHierarchyRootPlayer() == this || !useAction)
			return eAI_TakeItemToHandsImpl(item);
		else if (StartActionObject(eAIActionTakeItemToHands, item))
			return true;

		return false;
	}

	bool eAI_TakeItemToHandsImpl(ItemBase item)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI, this, "" + item);
#endif 

		eAI_UpdateVisitedBuildings();

		if (!item)
			return false;

		EntityAI hands = GetHumanInventory().GetEntityInHands();
		if (hands)
		{
		#ifdef DIAG_DEVELOPER
			EXError.Error(this, string.Format("Cannot take %1 to hands - current hands entity: %2", item, hands));
		#endif
			return false;
		}

		InventoryLocation il_dst = new InventoryLocation;
		il_dst.SetHands(this, item);

		if (il_dst.IsValid())
		{
			if (!item.IsTakeable() || item.Expansion_IsInventoryLocked())
				item.Expansion_SetLootable(true);

		#ifdef EXPANSIONMODAI_TAKETOLOCATION
			if (eAI_TakeItemToLocation(item, il_dst))
		#else
			if (Expansion_CloneItemToLocation(item, il_dst))
		#endif
			{
				if (m_eAI_Targets.Count() > 1 && GetExpansionSettings().GetAI().MemeLevel > 9000)
					eAI_PlayRandomLoveSound();

				m_eAI_PositionOverrideTimeout = 0;  //! Reset so we prioritize taking cover during actions

				Expansion_UpdateVisibility(true);

				return true;
			}
		}

	#ifdef DIAG_DEVELOPER
		InventoryLocation il_src = new InventoryLocation;
		item.GetInventory().GetCurrentInventoryLocation(il_src);
		EXError.Error(this, string.Format("Cannot take %1 to hands - src %2 dst %3", item, ExpansionStatic.DumpToString(il_src), ExpansionStatic.DumpToString(il_dst)));
	#endif

		//! If we couldn't take, make sure we don't try again
		if (item && !item.IsSetForDeletion())
			eAI_ThreatOverride(item, true);

		return false;
	}

	bool eAI_FindFreeInventoryLocationFor(ItemBase item, FindInventoryLocationType flags = 0, out InventoryLocation il_dst = null)
	{
		/*!
		 * ATTACHMENT          = 4
		 * CARGO               = 8
		 * HANDS               = 16
		 * PROXYCARGO          = 32
		 * ANY_CARGO           = 40 (CARGO | PROXYCARGO)
		 * ANY                 = 60 (ATTACHMENT | ANY_CARGO | HANDS)
		 * NO_SLOT_AUTO_ASSIGN = 64
		 */
		if (!flags)
			flags = FindInventoryLocationType.ATTACHMENT | FindInventoryLocationType.CARGO;

		il_dst = new InventoryLocation();

		if (GetInventory().FindFreeLocationFor(item, flags, il_dst))
		{
			if (GameInventory.LocationCanAddEntity(il_dst))
				return true;

			il_dst.Reset();  //! Invalidate location
		}

		return false;
	}

	bool eAI_TakeItemToInventory(ItemBase item, bool useAction = true)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI, this, "" + item);
#endif 

		if (!item)
			return false;

		if (item == GetItemInHands() || !useAction)
		{
			if (eAI_TakeItemToInventoryImpl(item))
				return true;
		}
		else if (StartActionObject(eAIActionTakeItem, item))
		{
			return true;
		}

		return false;
	}

	bool eAI_TakeItemToInventoryImpl(ItemBase item, FindInventoryLocationType flags = 0)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI, this, "" + item);
#endif 

		eAI_UpdateVisitedBuildings();

		if (!item)
			return false;

		InventoryLocation il_dst;

		ItemBase currentlyWornGear;
		bool transferCargo;

		if (item.IsClothing())
		{
			TStringArray inventorySlots = item.Expansion_GetInventorySlots();
			bool canWear;

			foreach (string slot: inventorySlots)
			{
				if (GetInventory().HasAttachmentSlot(InventorySlots.GetSlotIdFromString(slot)))
				{
					canWear = true;

					if (!Class.CastTo(currentlyWornGear, FindAttachmentBySlotName(slot)))
					{
						//! Found empty slot
						currentlyWornGear = null;  //! null any gear that was found in another slot
						break;
					}
				}
			}

			if (currentlyWornGear)
			{
				il_dst = new InventoryLocation();

				if (currentlyWornGear.GetInventory().GetCurrentInventoryLocation(il_dst) && eAI_DropItemImpl(currentlyWornGear, true) && CanReceiveAttachment(item, il_dst.GetSlot()))
				{
					//! Drop whole cargo of target item before taking
					item.Expansion_DropAllCargo();

					//il_dst.SetAttachment(this, item, il_dst.GetSlot());
					il_dst.SetItem(item);

					EXTrace.Print(EXTrace.AI, this, "Swapping " + currentlyWornGear + " for " + item);

					transferCargo = true;
				}
				else if (il_dst.IsValid())
				{
					il_dst.Reset();  //! Invalidate location
				}
			}
			else if (canWear)
			{
				flags = FindInventoryLocationType.ATTACHMENT;
			}
		}

		if (!currentlyWornGear)
			eAI_FindFreeInventoryLocationFor(item, flags, il_dst);

		bool result;

		if (il_dst.IsValid())
		{
			if (!item.IsTakeable() || item.Expansion_IsInventoryLocked())
				item.Expansion_SetLootable(true);

			item.Expansion_TryTurningOffAnyLightsOrNVG(this);

			//! 1) Remove from active visibility enhancers if present
			m_Expansion_ActiveVisibilityEnhancers.RemoveItemUnOrdered(item);

		#ifdef EXPANSIONMODAI_TAKETOLOCATION
			if (eAI_TakeItemToLocation(item, il_dst))
			{
				result = true;

				if (transferCargo && !MiscGameplayFunctions.Expansion_MoveCargo(currentlyWornGear, item))
					currentlyWornGear.Expansion_DropAllCargo(300);
			}
		#else
			EntityAI clone = Expansion_CloneItemToLocation(item, il_dst);
			if (clone)
			{
				result = true;

				if (transferCargo && !MiscGameplayFunctions.Expansion_MoveCargo(currentlyWornGear, clone))
					currentlyWornGear.Expansion_DropAllCargo(300);
			}
		#endif

			//! 2) Update visibility regardless of result because of 1)
			Expansion_UpdateVisibility(true);
		}

		if (!result)
		{
			//! If we couldn't take, make sure we don't try again
			if (!item.IsSetForDeletion())
				eAI_ThreatOverride(item, true);

			if (currentlyWornGear)
			{
				eAI_ThreatOverride(currentlyWornGear, false);  //! Enable taking what we dropped earlier
				eAI_ForgetRecentlyDroppedItem(currentlyWornGear);
			}
		}

		return result;
	}

	bool eAI_TakeItemToLocation(ItemBase item, InventoryLocation il_dst)
	{
		InventoryLocation il_src = new InventoryLocation();

		item.GetInventory().GetCurrentInventoryLocation(il_src);

		if (il_src.CompareLocationOnly(il_dst) && il_src.GetFlip() == il_dst.GetFlip())
			return false;

		if (il_dst.GetType() != InventoryLocationType.GROUND)
		{
			if (il_dst.GetType() == InventoryLocationType.HANDS || il_src.GetType() == InventoryLocationType.HANDS)
			{
				//! Forcing switch to HumanCommandMove before taking to/from hands,
				//! and hiding/showing item in hands after, unbreaks hand anim state
				if (!GetCommand_Move())
					StartCommand_Move();
				else
					m_eAI_CommandTime = 0.0;
			}

			if (il_src.GetType() == InventoryLocationType.HANDS)
			{
				if (m_WeaponManager.IsRunning())
					m_WeaponManager.OnWeaponActionEnd();  //! Prevent getting stuck in running state

				//! Forcing switch to HumanCommandMove before taking from hands,
				//! and hiding item in hands before, unbreaks hand anim state
				GetItemAccessor().HideItemInHands(true);
			}

			if (il_dst.GetType() == InventoryLocationType.HANDS)
			{
				//! Needs to be 'lootable' while in AI inventory, else AI won't be able to properly interact with item
				//! (e.g. won't be able to detach/attach mag to firearm)
				ExpansionItemBaseModule.SetLootable(item, true);
			}
		}

		if (GetGame().IsMultiplayer())
			GetGame().RemoteObjectTreeDelete(item);

		bool result = LocalTakeToDst(il_src, il_dst);

		if (il_src.GetType() == InventoryLocationType.HANDS && il_dst.GetType() != InventoryLocationType.GROUND)
		{
			//! Forcing switch to HumanCommandMove before taking from hands,
			//! and showing item in hands after, unbreaks hand anim state
			GetItemAccessor().HideItemInHands(false);
		}

		if (result)
		{
			if (il_dst.GetType() == InventoryLocationType.HANDS)
			{
				//! Forcing switch to HumanCommandMove before taking to hands,
				//! and hiding/showing item in hands after, unbreaks hand anim state
				GetItemAccessor().HideItemInHands(true);
				GetItemAccessor().HideItemInHands(false);
			}
			else if (il_src.GetType() != InventoryLocationType.GROUND && il_dst.GetType() == InventoryLocationType.GROUND)
			{
				ItemBase itemBs;
				if (Class.CastTo(itemBs, item))
				{
					if (!m_Expansion_CanBeLooted && !itemBs.m_Expansion_IsOwnerPlayer)
					{
						ExpansionItemBaseModule.SetLootable(item, false);
						item.SetLifetimeMax(120);  //! Make sure it despawns quickly when left alone
					}
				}
			}
		}

		if (GetGame().IsMultiplayer())
			GetGame().RemoteObjectTreeCreate(item);

		return result;
	}

	bool eAI_TakeItemToInventoryDropShoulderImpl(ItemBase item)
	{
		if (item.IsWeapon())
		{
			array<Weapon_Base> weapons;
			if (item.IsKindOf("Pistol_Base"))
				weapons = m_eAI_Handguns;
			else if (item.ShootsExplosiveAmmo())
				weapons = m_eAI_Launchers;
			else
				weapons = m_eAI_Firearms;

			foreach (Weapon_Base weapon: weapons)
			{
				if (weapon.IsDamageDestroyed())
				{
					eAI_DropItemImpl(weapon);
					break;
				}

				if (item != weapon && item.GetType() == weapon.GetType())
				{
					//! We already have a weapon of that type in inventory

					if (weapon.GetHealthLevel() > item.GetHealthLevel())
					{
						//! Existing weapon is in worse condition, drop it
						eAI_DropItemImpl(weapon, true);
						break;
					}

					//! Existing weapon is in equal or better condition, don't take item
					eAI_ThreatOverride(item, true);
					return false;
				}
			}
		}

		if (!eAI_TakeItemToInventoryImpl(item))
		{
			//! If we failed to take item to inv, item is weapon and both shoulder and melee slots are taken,
			//! drop one to make space for item and try again
			if (item.IsWeapon() && item.Expansion_HasInventorySlot((TStringArray){"Shoulder", "Melee"}))
			{
				ItemBase shoulder;
				ItemBase melee;

				if (Class.CastTo(shoulder, FindAttachmentBySlotName("Shoulder")) && Class.CastTo(melee, FindAttachmentBySlotName("Melee")))
				{
					ItemBase other;

					if (eAI_WeaponSelection(melee, shoulder))
						other = melee;
					else
						other = shoulder;

					if (eAI_DropIfLessDPS(item, other))
						return eAI_TakeItemToInventoryImpl(item);
				}
			}

			return false;
		}

		return true;
	}

	/**
	 * @brief drop other item if not a weapon or less DPS than item
	 */
	bool eAI_DropIfLessDPS(ItemBase item, ItemBase other)
	{
		if ((!other.IsWeapon() || !eAI_WeaponSelection(item, other)) && eAI_DropItemImpl(other))
		{
			eAI_ThreatOverride(other, false);
			return true;
		}

		return false;
	}

	bool eAI_IsItemObstructed(ItemBase item)
	{
		vector begPos;
		MiscGameplayFunctions.GetHeadBonePos(this, begPos);
		vector endPos = item.GetCenter();

		//! @note cannot use IsObjectObstructedFilterEx
		//! (uses vanilla ItemBase::CanObstruct which will always return true on server as it is only meant to be called on client because it relies on g_Game.GetPlayer)
		//m_eAI_IsItemObstructedCache.ClearCache();
		//m_eAI_IsItemObstructedCache.RaycastStart = begPos;
		//m_eAI_IsItemObstructedCache.ObjectCenterPos = endPos;

		//if (MiscGameplayFunctions.IsObjectObstructedFilterEx(item, m_eAI_IsItemObstructedCache, this))
			//return true;

		vector contactPos, contactDir;
		int contactComponent;
		set<Object> results = new set<Object>;

		if (DayZPhysics.RaycastRV(begPos, endPos, contactPos, contactDir, contactComponent, results, item, this, false, false, ObjIntersectFire, 0.0, CollisionFlags.ALLOBJECTS))
		{
			if (results.Count() == 0)
				return true;

			EntityAI entity;

			foreach (Object obj: results)
			{
				if (Class.CastTo(entity, obj))
					obj = entity.GetHierarchyRoot();

				if (ExpansionStatic.CanObstruct(obj))
					return true;
			}
		}

		return false;
	}

	bool Expansion_GetUp(bool force = false)
	{
		if (!force)
		{
			if (IsPlayerInStance(DayZPlayerConstants.STANCEMASK_ERECT | DayZPlayerConstants.STANCEMASK_RAISEDERECT))
				return false;
		}

		return OverrideStance(DayZPlayerConstants.STANCEIDX_ERECT, force);
	}

	//! @brief Set emote for playing
	//! Setting autoCancel to true will automatically cancel the emote if it is a static pose
	//! @note Emote will not play instantly! Will play when AI FSM can transition to PlayEmote state
	void Expansion_SetEmote(int emoteID, bool autoCancel = false, int autoCancelDelay = 0)
	{
		m_Expansion_EmoteID = emoteID;
		m_Expansion_EmoteAutoCancel = autoCancel;
		m_Expansion_EmoteAutoCancelDelay = autoCancelDelay;
	}

	void Expansion_PlayEmote()
	{
		if (!m_Expansion_EmoteID)
			return;

		if (!m_EmoteManager.IsEmotePlaying() && m_EmoteManager.CanPlayEmote(m_Expansion_EmoteID))
		{
			m_EmoteManager.PlayEmote(m_Expansion_EmoteID);
			if (m_Expansion_EmoteAutoCancel)
			{
				int delay;
				switch (m_Expansion_EmoteID)
				{
					//! These emotes are temporary poses and don't need to be canceled
					case EmoteConstants.ID_EMOTE_TAUNTKISS:
					case EmoteConstants.ID_EMOTE_TAUNTELBOW:
					case EmoteConstants.ID_EMOTE_THROAT:
					case EmoteConstants.ID_EMOTE_DANCE:
					case EmoteConstants.ID_EMOTE_DABBING:
					case EmoteConstants.ID_EMOTE_CLAP:
					case EmoteConstants.ID_EMOTE_TAUNTTHINK:
					case EmoteConstants.ID_EMOTE_MOVE:
					case EmoteConstants.ID_EMOTE_DOWN:
					case EmoteConstants.ID_EMOTE_COME:
					case EmoteConstants.ID_EMOTE_NOD:
					case EmoteConstants.ID_EMOTE_SHAKE:
					case EmoteConstants.ID_EMOTE_SHRUG:
					case EmoteConstants.ID_EMOTE_VOMIT:
						break;

					//! These emotes are static poses with looping animation
					case EmoteConstants.ID_EMOTE_SOS:
						if (m_Expansion_EmoteAutoCancelDelay)
							delay = m_Expansion_EmoteAutoCancelDelay;
						else
							delay = 4000;
						break;

					//! Assume static pose
					default:
						if (m_Expansion_EmoteAutoCancelDelay)
							delay = m_Expansion_EmoteAutoCancelDelay;
						else
							delay = 2000;
						break;
				}
				if (delay)
					GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(m_EmoteManager.ServerRequestEmoteCancel, delay);
			}
		}

		if (m_Expansion_EmoteAutoCancel)
			m_Expansion_EmoteID = 0;
	}

	override vector Expansion_GetHeadingVector()
	{
		return Vector(GetOrientation()[0], 0, 0).AnglesToVector();
	}

	override float Expansion_GetMovementSpeed()
	{
		eAICommandMove cmd = GetCommand_MoveAI();
		if (cmd)
		{
			float speed = cmd.GetCurrentMovementSpeed();
			if (speed > 2.0 && m_WeaponRaised)
				speed = 2.0;
			return speed;
		}

		return 0.0;
	}

	override float Expansion_GetMovementAngle()
	{
		eAICommandMove cmd = GetCommand_MoveAI();
		if (cmd)
			return cmd.GetCurrentMovementAngle();

		return 0.0;
	}

	float Expansion_GetTargetMovementAngle()
	{
		eAICommandMove cmd = GetCommand_MoveAI();
		if (cmd)
			return cmd.GetTargetDirection();

		return 0.0;
	}

	bool HandleVaulting(eAICommandMove hcm, float pDt)
	{
		//if (!m_PathFinding.IsVault())
		//{
		//	return false;
		//}

		if (!m_PathFinding.m_AllowJumpClimb && !m_PathFinding.m_IsBlockedPhysically)
			return false;

		if (Math.AbsFloat(ExpansionMath.AngleDiff2(GetOrientation()[0], m_PathFinding.m_PathSegmentDirection.VectorToAngles()[0])) > 45.0)
			return false;

		Object blockingObject = hcm.GetBlockingObject();

		bool isBlockingItem;
		bool climbFloatingItem;

		//! Superjanky but allows climbing floating items with collision, e.g. Expansion basebuilding floors,
		//! while preventing unwanted climbing on other items.
		//! Offset of 0.5 is because AI, like players, can just walk over small height differences
		if (blockingObject && blockingObject.IsInventoryItem() && ExpansionStatic.CanObstruct(blockingObject))
		{
			isBlockingItem = true;

			if (GetPosition()[1] + 0.5 < blockingObject.GetPosition()[1])
				climbFloatingItem = true;
		}

		if ((!m_PathFinding.m_IsJumpClimb || isBlockingItem) && !climbFloatingItem)
			return false;

		if (m_eAI_PositionIsFinal && Math.Round(Expansion_GetMovementSpeed()) == 0.0)
			return false;

		SHumanCommandClimbSettings hcls = GetDayZPlayerType().CommandClimbSettingsW();
		
		if ( m_MovementState.m_iMovement != DayZPlayerConstants.MOVEMENTIDX_IDLE )
			hcls.m_fFwMaxDistance = 2.5;
		else
			hcls.m_fFwMaxDistance = 1.2;

		SetOrientation(GetOrientation());
		HumanCommandClimb.DoClimbTest(this, m_ExClimbResult, 0);

		if (m_ExClimbResult.m_bIsClimb || m_ExClimbResult.m_bIsClimbOver)
		{
			//! AI, like players, can just walk over small height differences (avoids awkwardly climbing stairs)
			//! @note hcls.m_fFwMinHeight cannot be used for this since it is too high (0.7)
			//! @note this check is only needed for vanilla DoClimbTest, ExpansionClimb::DoClimbTest checks height internally
			if (m_ExClimbResult.m_fClimbHeight < 0.5)
				return false;

			return true;
		}

		//! As we are essentially using Zombie pathfinding, we may encounter situations where the path will go through a fence
		//! that Zs would be able to jump (e.g. wall_indfnc_9.p3d) but player AI would not due to HumanCommandClimb.DoClimbTest not letting us.
		//! Use ExpansionClimb.DoClimbTest with alwaysAllowClimb = true instead.
		if (m_PathFinding.m_DoClimbTestEx)
		{
			ExpansionClimb.DoClimbTest(this, m_ExClimbResult, true);
			if (m_ExClimbResult.m_bIsClimb || m_ExClimbResult.m_bIsClimbOver)
				return true;
		}

		return false;
	}

	override bool CanJump()
	{
		//! Following is equivalent to vanilla PlayerBase::CanJump

		if (GetBrokenLegs() != eBrokenLegs.NO_BROKEN_LEGS)
		{	
			return false;
		}
		
		if (!CanConsumeStamina(EStaminaConsumers.JUMP))
			return false;

		//! disables jump when player is significantly injured
		if (m_InjuryHandler && m_InjuryHandler.GetInjuryAnimValue() >= InjuryAnimValues.LVL3)
			return false;
		
		if (IsInFBEmoteState() || m_EmoteManager.m_MenuEmote)
		{
			return false;
		}

		//! Following is roughly equivalent to vanilla DayZPlayerImplement::CanJump

		if (IsFBSymptomPlaying() || IsRestrained() || IsUnconscious())
			return false;
		
		if (m_MovementState.m_iStanceIdx == DayZPlayerConstants.STANCEIDX_PRONE || m_MovementState.m_iStanceIdx == DayZPlayerConstants.STANCEIDX_RAISEDPRONE)
			return false;
		
		HumanItemBehaviorCfg hibcfg = GetItemAccessor().GetItemInHandsBehaviourCfg();
		if (!hibcfg.m_bJumpAllowed)
			return false;
		
		if (!DayZPlayerUtils.PlayerCanChangeStance(this, DayZPlayerConstants.STANCEIDX_ERECT) || !DayZPlayerUtils.PlayerCanChangeStance(this, DayZPlayerConstants.STANCEIDX_RAISEDERECT))
			return false;
		
		return true;
	}

	override bool CanClimb(int climbType, SHumanCommandClimbResult climbRes)
	{
		//! Following is equivalent to vanilla PlayerBase::CanClimb

		if (GetBrokenLegs() == eBrokenLegs.BROKEN_LEGS)
		{
			return false;
		}
			
		if (climbType == 1 && !CanConsumeStamina(EStaminaConsumers.VAULT))
			return false;
		
		if (climbType == 2 && (!CanConsumeStamina(EStaminaConsumers.CLIMB) || GetBrokenLegs() != eBrokenLegs.NO_BROKEN_LEGS))
			return false;

		if (climbType > 0 && m_InjuryHandler && m_InjuryHandler.GetInjuryAnimValue() >= InjuryAnimValues.LVL3)
			return false;

		//! Following is roughly equivalent to vanilla DayZPlayerImplement::CanClimb

		if (IsFBSymptomPlaying() || IsRestrained() || IsUnconscious() || IsInFBEmoteState())
			return false;
		
		if (m_MovementState.IsInProne() || m_MovementState.IsInRaisedProne())
			return false;

		HumanItemBehaviorCfg hibcfg = GetItemAccessor().GetItemInHandsBehaviourCfg();
		if (!hibcfg.m_bJumpAllowed)
			return false;

		if (climbRes)
		{
			if (!eAI_CanClimbOn(climbRes.m_GrabPointParent, climbRes))
				return false;
			if (!eAI_CanClimbOn(climbRes.m_ClimbStandPointParent, climbRes))
				return false;
			if (!eAI_CanClimbOn(climbRes.m_ClimbOverStandPointParent, climbRes))
				return false;

			IEntity standPointParent;
			vector standPoint;
			vector checkPosition;
			vector checkDirection;
			vector unitPosition = GetPosition();

			if (climbRes.m_ClimbOverStandPoint != vector.Zero)
			{
				standPointParent = climbRes.m_ClimbOverStandPointParent;
				standPoint = climbRes.m_ClimbOverStandPoint;
			}
			else if (climbRes.m_ClimbStandPoint != vector.Zero)
			{
				standPointParent = climbRes.m_ClimbStandPointParent;
				standPoint = climbRes.m_ClimbStandPoint;
			}
			else
			{
				checkDirection = GetDirection() * 1.5;
			}

			if (standPoint != vector.Zero)
			{
				Object standPointObject;
				if (Class.CastTo(standPointObject, standPointParent))
				{
					//! Standpoint parent is object, local coordinates unequal world coordinates
					vector transform[4];
					standPointParent.GetTransform(transform);
					checkPosition = standPoint.Multiply4(transform);
				}
				else
				{
					//! No standpoint parent or part of world, local coordinates equal world coordinates
					checkPosition = standPoint;
				}
				checkPosition[1] = unitPosition[1];
				checkDirection = vector.Direction(unitPosition, checkPosition);
			}

			vector surfacePosition = ExpansionStatic.GetSurfaceRoadPosition(unitPosition + checkDirection);
			//! Swim start water level = 1.5 m, see DayZPlayerUtils::CheckWaterLevel
			if (!GetGroup().IsFormationLeaderSwimming() && GetGame().GetWaterDepth(surfacePosition) > 1.5)
			{
			#ifdef DIAG_DEVELOPER
				Expansion_DebugObject(1344, surfacePosition, "ExpansionDebugRodBig_Blue", checkDirection);
			#endif
				return false;
			}
		#ifdef DIAG_DEVELOPER
			else
			{
				Expansion_DebugObject(1344, "0 0 0", "ExpansionDebugRodBig_Blue");
			}
		#endif

			if (!eAI_IsFallSafe(checkDirection))
				return false;
		}

		return true;
	}

	bool eAI_CanClimbOn(IEntity parent, SHumanCommandClimbResult climbRes)
	{
		EntityAI entity;
		if (Class.CastTo(entity, parent) && entity.IsHologram())
			return false;

		Object object;
		bool isPathEndPointCollidingObject;
		if (Class.CastTo(object, parent) && ExpansionStatic.IsColliding(object, m_PathFinding.GetEnd()))
			isPathEndPointCollidingObject = true;

		if ((m_WeaponRaised || m_eAI_IsFightingFSM) && !isPathEndPointCollidingObject && !climbRes.m_bIsClimbOver && !m_PathFinding.m_IsBlockedPhysically)
			return false;

		if (object)
		{ 
			if (object.IsTree() || object.IsBush() || object.IsMan())
				return false;

			BuildingBase building;
			if (Class.CastTo(building, object))
			{
				if (building.m_eAI_PreventClimb || building.IsInherited(ExpansionDebugObject))
					return false;

				//! Don't allow vaulting unenterable building unless path endpoint is within building or we are within building
				if (!building.Expansion_IsEnterable() && !isPathEndPointCollidingObject && !ExpansionStatic.IsColliding(object, GetPosition()) && climbRes.m_bIsClimbOver)
				{
					eAICommandMove move = GetCommand_MoveAI();
					if (!move || !move.IsBlocked())
					{
						if (EXTrace.AI)
							EXTrace.Print(true, this, "eAI_CanClimbOn false " + Debug.GetDebugName(parent) + " is scenery? " + object.IsScenery() + " is plain? " + object.IsPlainObject());

						return false;
					}
				}
			}
			else if (object.IsTransport() || object.IsInventoryItem())
			{
				if (!isPathEndPointCollidingObject && !ExpansionStatic.IsColliding(object, GetPosition()) && climbRes.m_bIsClimbOver)
					return false;
			}
			else if (!object.IsScenery())
			{
				string debugName = object.GetDebugName();
				debugName.ToLower();

				if ((debugName.Contains("container") || debugName.Contains("tank_small") || debugName.Contains("misc_haybale") || debugName.Contains("misc_woodreserve")) && climbRes.m_bIsClimbOver)
				{
					if (EXTrace.AI)
						EXTrace.Print(true, this, "eAI_CanClimbOn false " + Debug.GetDebugName(parent) + " is scenery? " + object.IsScenery() + " is plain? " + object.IsPlainObject());

					return false;
				}

				//! @note AI should NEVER climb rocks, it can make them "fall into" the rock and through the map
				if (debugName.Contains("pole") || debugName.Contains("sign") || debugName.Contains("busstop") || debugName.Contains("rock") || object.IsRock() || debugName.Contains("stairs") || debugName.Contains("farm_strawstack"))
				{
					if (EXTrace.AI)
						EXTrace.Print(true, this, "eAI_CanClimbOn false " + Debug.GetDebugName(parent) + " is scenery? " + object.IsScenery() + " is plain? " + object.IsPlainObject());

					return false;
				}
			}
		}

		if (EXTrace.AI && parent)
			EXTrace.Print(true, this, "eAI_CanClimbOn " + Debug.GetDebugName(parent) + " is scenery? " + object.IsScenery() + " is plain? " + object.IsPlainObject());

		return true;
	}

	bool eAI_IsFallSafe(vector checkDirection, int dbgIndex = 1337)
	{
		vector position = GetPosition();
		vector checkPosition = position + checkDirection;
		
		vector hitPosition;
		vector hitNormal;
		int contactComponent;
		set<Object> results = new set<Object>;

		//! Is there a physical object blocking check direction (e.g. a wall)?
		if (DayZPhysics.RaycastRV(checkPosition + "0 2 0", position + "0 2 0", hitPosition, hitNormal, contactComponent, results, null, this, false, false, ObjIntersectGeom, 0.2))
		{
			//vector fromHit = hitPosition - checkDirection.Normalized() * 0.2;

			////! Is there a physical object blocking up direction from hit pos (e.g. a ceiling)?
			//if (DayZPhysics.RaycastRV(fromHit, fromHit + "0 3.5 0", hitPosition, hitNormal, contactComponent, results, null, this, false, false, ObjIntersectGeom))
				return true;
		}

		//! @note the 1.5m vertical offset has been added for the top floor of Land_HouseBlock_2F3, else surface won't be detected properly
		//! (doesn't matter if via SurfaceRoad3D or raycast)
		float offsetY = 1.5;

/*
		vector begPos = Vector(checkPosition[0], checkPosition[1] + offsetY, checkPosition[2]);
*/

		checkPosition[1] = GetGame().SurfaceRoadY3D(checkPosition[0], checkPosition[1] + offsetY, checkPosition[2], RoadSurfaceDetection.UNDER);

		float fallHeight = position[1] - checkPosition[1];
/*
		if (fallHeight > DayZPlayerImplementFallDamage.HEALTH_HEIGHT_LOW)
		{
			PhxInteractionLayers collisionLayerMask;
			collisionLayerMask |= PhxInteractionLayers.ROADWAY;
			collisionLayerMask |= PhxInteractionLayers.BUILDING;
			collisionLayerMask |= PhxInteractionLayers.FENCE;
			collisionLayerMask |= PhxInteractionLayers.VEHICLE;
			collisionLayerMask |= PhxInteractionLayers.TERRAIN;

			Object hitObject;
		#ifdef DIAG_DEVELOPER
			string type;
		#endif

			if (DayZPhysics.RayCastBullet(begPos, checkPosition, collisionLayerMask, this, hitObject, hitPosition, null, null))
			{
				checkPosition = hitPosition;

				fallHeight = position[1] - checkPosition[1];

			#ifdef DIAG_DEVELOPER
				if (fallHeight > DayZPlayerImplementFallDamage.HEALTH_HEIGHT_LOW)
				{
					if (hitObject)
						type = hitObject.GetType();
					EXTrace.Print(EXTrace.AI, this, "eAI_IsFallSafe RayCastBullet hitObject " + hitObject + " " + type + " " + hitPosition);
				}
			#endif
			}

			if (fallHeight > DayZPlayerImplementFallDamage.HEALTH_HEIGHT_LOW)
			{
				results = new set<Object>;

				if (DayZPhysics.RaycastRV(begPos, checkPosition, hitPosition, hitNormal, contactComponent, results, null, this, false, false, ObjIntersectFire, 0.0))
				{
					checkPosition = hitPosition;

					fallHeight = position[1] - checkPosition[1];

				#ifdef DIAG_DEVELOPER
					if (fallHeight > DayZPlayerImplementFallDamage.HEALTH_HEIGHT_LOW)
					{
						hitObject = results[0];
						if (hitObject)
							type = hitObject.GetType();
						EXTrace.Print(EXTrace.AI, this, "eAI_IsFallSafe RaycastRV hitObject " + hitObject + " " + type + " " + hitPosition);
					}
				#endif
				}
			}
		}
*/
		//! https://feedback.bistudio.com/T173348
		bool isFallSafe;
		if (fallHeight <= DayZPlayerImplementFallDamage.HEALTH_HEIGHT_LOW || (GetHealth01() - Math.InverseLerp(DayZPlayerImplementFallDamage.HEALTH_HEIGHT_LOW, DayZPlayerImplementFallDamage.HEALTH_HEIGHT_HIGH, fallHeight) >= 0.7))
			isFallSafe = true;

	#ifdef DIAG_DEVELOPER
		//EXTrace.Print(EXTrace.AI, this, "eAI_IsFallSafe position " + ExpansionStatic.VectorToString(position, ExpansionVectorToString.Plain) + " checkDirection " + ExpansionStatic.VectorToString(checkDirection, ExpansionVectorToString.Plain) + " " + ExpansionStatic.VectorToString(checkDirection.VectorToAngles(), ExpansionVectorToString.Plain) + " checkPosition " + ExpansionStatic.VectorToString(checkPosition, ExpansionVectorToString.Plain) + " fallHeight " + fallHeight + " safe " + isFallSafe);
		vector arrowPos = checkPosition;
		arrowPos[1] = position[1] + 0.3;
		vector arrowOri = checkDirection.VectorToAngles();
		arrowOri[1] = -90.0;
		if (isFallSafe)
		{
			Expansion_DebugObject(dbgIndex, arrowPos, "ExpansionDebugArrow", arrowOri.AnglesToVector());
			Expansion_DebugObject(dbgIndex + 1, "0 0 0", "ExpansionDebugArrow_Red");
			Expansion_DebugObject(dbgIndex + 2, "0 0 0", "ExpansionDebugArrow_Red");
		}
		else
		{
			Expansion_DebugObject(dbgIndex, "0 0 0", "ExpansionDebugArrow");
			Expansion_DebugObject(dbgIndex + 1, arrowPos, "ExpansionDebugArrow_Red", arrowOri.AnglesToVector());
			arrowPos[1] = checkPosition[1] + 0.3;
			Expansion_DebugObject(dbgIndex + 2, arrowPos, "ExpansionDebugArrow_Red", arrowOri.AnglesToVector());
		}
	#endif

		return isFallSafe;
	}

	bool eAI_IsDangerousAltitude()
	{
		if (IsSwimming())
			return false;

		vector position = m_ExTransformPlayer[3];
		float fallHeight = position[1] - m_eAI_SurfaceY;
		vector surfacePosition = Vector(position[0], m_eAI_SurfaceY, position[2]);
		float waterDepth = Math.Max(GetGame().GetWaterDepth(surfacePosition), 0.0);
		if (fallHeight - waterDepth > DayZPlayerImplementFallDamage.HEALTH_HEIGHT_LOW)
			return true;

		return false;
	}

	void eAI_JumpOrClimb()
	{
		m_JumpClimb.JumpOrClimb();
	}

	bool HandleBuildingDoors(eAICommandMove hcm, float pDt)
	{
		//if (!m_PathFinding.IsDoor())
			//return;

		BuildingBase cover;
		if (!m_PathFinding.m_IsBlocked && !hcm.IsBlocked() && (!m_eAI_CurrentCoverObject || !Class.CastTo(cover, m_eAI_CurrentCoverObject) || cover.Expansion_IsEnterable()))
			return false;

		if (m_eAI_PositionIsFinal && Math.Round(hcm.GetCurrentMovementSpeed()) == 0.0)
			return false;

		vector position = m_ExTransformPlayer[3] + (m_ExTransformPlayer[1] * 1.1);
		vector direction = m_ExTransformPlayer[2];

		float fwdBwd = 1.0;
		if (Math.AbsFloat(Expansion_GetMovementAngle()) > 135.0)
			fwdBwd = -1.0;

		vector p0 = position;
		vector p1 = position + (direction * 1.5 * fwdBwd);

		if (GetWeaponManager().IsRunning())
		{
		#ifdef DIAG_DEVELOPER
			Expansion_DebugObject(22222, "0 0 0", "ExpansionDebugSphereSmall_White");
			Expansion_DebugObject(22223, p1, "ExpansionDebugSphereSmall_Black", direction, position);
		#endif
			return false;
		}

	#ifdef DIAG_DEVELOPER
		Expansion_DebugObject(22222, p1, "ExpansionDebugSphereSmall_White", direction, position);
		Expansion_DebugObject(22223, "0 0 0", "ExpansionDebugSphereSmall_Black");
	#endif

		RaycastRVParams params(p0, p1, this, 0.5);
		array<ref RaycastRVResult> results();
		array<Object> excluded();
		excluded.Insert(this);

		params.with = this;
		params.flags = CollisionFlags.ALLOBJECTS;
		params.type = ObjIntersectView;

		if (!DayZPhysics.RaycastRVProxy(params, results, excluded))
			return false;

		int time = GetGame().GetTime();

		foreach (auto result : results)
		{
			BuildingBase building;
			if (!Class.CastTo(building, result.obj))
				continue;

			int doorIndex = building.GetDoorIndex(result.component);
			if (doorIndex == -1)
				continue;

			bool isDoorOpen = building.IsDoorOpen(doorIndex);
			bool isEnterable = building.Expansion_IsEnterable();

			if (isDoorOpen)
			{
				if (isEnterable)
				{
					Object blockingObject = hcm.GetBlockingObject();
					if (blockingObject || !hcm.CheckBlocked())
					{
						//! If we're blocked less than 3 seconds, and there's enough free space to move fwd/left/right,
						//! and speed limit is zero or movement speed is zero or velocity is higher than 0.001 m/s, ignore opened door
						if (hcm.GetBlockedTime() < 3.0 && (CheckFreeSpace(vector.Forward, 0.3, false) || CheckFreeSpace(vector.Aside, 0.5, false) || CheckFreeSpace(-vector.Aside, 0.5, false)) && (m_MovementSpeedLimit == 0 || Math.Floor(hcm.GetCurrentMovementSpeed()) == 0.0 || hcm.GetPositionTime() < 1.5))
						{
							if (hcm.IsBlocked())
								m_PathFinding.ForceRecalculate(true);
							continue;
						}
					}
				}
			}
			else if (!isEnterable)
			{
				//! Do not open doors on structures that cannot be entered, e.g. wrecks (less chance of getting stuck on them when closed)
				continue;
			}
			else if (building.IsDoorLocked(doorIndex))
			{
				//auto info = building.eAI_GetDoorTargetInformation(doorIndex, result.pos);
				auto info = building.eAI_GetDoorTargetInformation(doorIndex, building.GetDoorSoundPos(doorIndex));
				info.AddAI(this);

				continue;
			}

			if (time - building.m_eAI_LastDoorInteractionTime[doorIndex] < 1500)
				continue;

			/**
			 * TODO: Use FSM events to notify that the door should be opened. 
			 * TODO: Add support for animation action logic to FSM states
			 * TODO: FSM file just for doors. Opening, picking, breaking, closing. Vehicles and Buildings.
			 * 
			 *	GetFSM().Start("OpenDoor")
			 * 
			 * Events are stored in the transitions
			 */

			//! Decrease chance of AI getting stuck between wall and opened door by temporarily stopping before opening
			if (!isDoorOpen && !m_eAI_Halt)
			{
				m_eAI_Halt = true;
				GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(eAI_SetHalt, 650, false, false);
			}

			if (isDoorOpen)
			{
				building.CloseDoor(doorIndex);
			}
			else
			{
				building.OpenDoor(doorIndex);
			}
			
			ActionInteractBaseCB.Cast(AddCommandModifier_Action(DayZPlayerConstants.CMD_ACTIONMOD_OPENDOORFW,ActionInteractBaseCB));

			building.m_eAI_LastDoorInteractionTime[doorIndex] = time;
			return true;
		}

		return false;
	}

	void eAI_SetHalt(bool halt)
	{
		m_eAI_Halt = halt;
	}

	void eAI_UpdateVisitedBuildings()
	{
		if (GetGroup().GetWaypointBehaviour() == eAIWaypointBehavior.ROAMING && GetGroup().GetFormationLeader() == this)
		{
			vector begPos = GetPosition() + "0 1.5 0";
			//! Raycast up instead of down because not all buildings have floors (e.g. sheds)
			//! Raycast height is chosen so that it can still hit ceiling of Land_Cementworks_Hall2 from floor
			vector endPos = GetPosition() + "0 11 0";
			vector hitPosition;
			vector hitNormal;
			int contactComponent;
			set<Object> results = new set<Object>;

			if (DayZPhysics.RaycastRV(begPos, endPos, hitPosition, hitNormal, contactComponent, results, null, this, false, false, ObjIntersectGeom, 0.0, CollisionFlags.ALLOBJECTS))
			{
				BuildingBase building;
				foreach (Object obj: results)
				{
					if (Class.CastTo(building, obj))
					{
						GetGroup().m_VisitedBuildings.Insert(building);
						break;
					}
				}
			}
		}
	}
};
