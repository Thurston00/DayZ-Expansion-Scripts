modded class DayZPlayerImplement
{
#ifdef DIAG_DEVELOPER
	static int DEBUG_EXPANSION_AI_CLIMB;
	static bool DEBUG_EXPANSION_AI_VEHICLE;
#endif

	static bool s_eAI_DebugDamage;
	
	private ref eAIPlayerTargetInformation m_TargetInformation = new eAIPlayerTargetInformation(this);
	ref eAIDamageHandler m_eAI_DamageHandler = new eAIDamageHandler(this, m_TargetInformation);

	private eAIGroup m_eAI_Group;
	private eAIGroup m_Expansion_FormerGroup;
	protected typename m_eAI_FactionType;
	private int m_eAI_GroupID;
	int m_eAI_GroupMemberID = -1;  //! Only unique within current group
	private int m_eAI_FactionTypeID;
	private int m_eAI_FactionTypeIDSynch;
	private int m_eAI_GroupMemberIndex;
	private int m_eAI_GroupMemberIndexSynch;

	private bool m_eAI_IsPassive;

	float m_eAI_LastAggressionTime;
	float m_eAI_LastAggressionTimeout;

	float m_eAI_LastHitTime;
	float m_eAI_LastNoiseTime;

	float m_eAI_DamageReceivedMultiplier = 1.0;

#ifdef DIAG_DEVELOPER
#ifndef SERVER
	autoptr array<Shape> m_Expansion_DebugShapes = new array<Shape>();
#endif
#endif

	void ~DayZPlayerImplement()
	{
		if (!GetGame())
			return;

	#ifdef DIAG_DEVELOPER
		EXTrace.Print(EXTrace.AI, this, "~DayZPlayerImplement");
	#endif

		eAI_Cleanup(true);
	}

	override void Expansion_Init()
	{
#ifdef EXTRACE_DIAG
		auto trace = EXTrace.Start(EXTrace.AI, this);
#endif

		super.Expansion_Init();

		RegisterNetSyncVariableInt("m_eAI_GroupID", -1, int.MAX);
		RegisterNetSyncVariableInt("m_eAI_FactionTypeIDSynch");
		RegisterNetSyncVariableInt("m_eAI_GroupMemberIndexSynch", 0, 0xffff);

		m_eAI_GroupID = -1;
		m_eAI_FactionTypeID = -1;

		if (GetGame().IsServer() && m_eAI_FactionType)
		{
			//! @note w/o the cast to eAIFaction, the compiler warns about unsafe downcasting.
			//! Of course the compiler is wrong, because we're casting up, not down, so this cast here is just there to satisfy compiler shortcomings.
			//! Yes I wrote this comment for the sole reason that I'm annoyed by this.
			SetGroup(eAIGroup.CreateGroup(eAIFaction.Cast(m_eAI_FactionType.Spawn())));
		}
	}

	//! Vanilla, can this AI be targeted by Zs/Animals?
	override bool CanBeTargetedByAI(EntityAI ai)
	{
		if (!super.CanBeTargetedByAI(ai))
			return false;

		if (GetGroup())
			return !GetGroup().GetFaction().IsFriendlyEntity(ai, this);

		return true;
	}

	eAIPlayerTargetInformation GetTargetInformation()
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_0(this, "GetTargetInformation");
#endif

		return m_TargetInformation;
	}

	bool IsAI()
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_0(this, "IsAI");
#endif

		return false;
	}

	override bool Expansion_IsAI()
	{
		return IsAI();
	}

	override int Expansion_GetEntityStorageAdditionalDataVersion()
	{
		return 1;
	}

	override void Expansion_OnEntityStorageAdditionalDataSave(ParamsWriteContext ctx)
	{
		super.Expansion_OnEntityStorageAdditionalDataSave(ctx);

		ctx.Write(m_eAI_FactionTypeID);
	}

	override bool Expansion_OnEntityStorageAdditionalDataLoad(ParamsWriteContext ctx, int version)
	{
		if (!super.Expansion_OnEntityStorageAdditionalDataLoad(ctx, version))
			return false;

		if (!ctx.Read(m_eAI_FactionTypeID))
			return false;

		if (!m_eAI_Group || m_eAI_Group.GetFaction().GetTypeID() != m_eAI_FactionTypeID)
		{
			eAIFaction faction = eAIFaction.CreateByID(m_eAI_FactionTypeID);

			if (faction)
			{
				if (m_eAI_Group)
					m_eAI_Group.SetFaction(faction);
				else
					eAIGroup.GetGroupByLeader(this, true, faction);
			}
		}

		return true;
	}

	void SetGroup(eAIGroup group, bool autoDeleteFormerGroupIfEmpty = true)
	{
#ifdef EXTRACE_DIAG
		auto trace = EXTrace.Start(EXTrace.AI, this, "" + group, "" + autoDeleteFormerGroupIfEmpty);
#endif

		if (m_eAI_Group == group)
			return;

		if (m_eAI_Group)
		{
			m_eAI_Group.RemoveMember(this, autoDeleteFormerGroupIfEmpty);

			if (IsAI() && m_eAI_Group.m_Persist && m_eAI_Group.m_BaseName)
				eAI_DeletePersistentFiles();

			m_eAI_GroupID = -1;
			m_eAI_GroupMemberID = -1;

			EXTrace.Print(EXTrace.AI, this, "Current AI group: " + m_eAI_Group);
			if (!autoDeleteFormerGroupIfEmpty)
				m_Expansion_FormerGroup = m_eAI_Group;
		}

		EXTrace.Print(EXTrace.AI, this, "Setting AI group: " + group);
		m_eAI_Group = group;

		if (m_eAI_Group)
		{
			m_eAI_GroupID = m_eAI_Group.GetID();
			m_eAI_GroupMemberID = m_eAI_Group.m_NextGroupMemberID++;
			int factionTypeID = m_eAI_Group.GetFaction().GetTypeID();
			if (factionTypeID != m_eAI_FactionTypeID)
				eAI_SetFactionTypeID(factionTypeID);

			SetGroupMemberIndex(m_eAI_Group.AddMember(this));
			EXTrace.Print(EXTrace.AI, this, "Group ID: " + m_eAI_GroupID);
		}
		else if (m_eAI_FactionTypeID != -1)
		{
			eAI_SetFactionTypeID(-1);
		}

		if (GetGame().IsDedicatedServer())
			SetSynchDirty();
	}

	void eAI_DeletePersistentFiles()
	{
		string path = m_eAI_Group.GetStorageDirectory() + m_eAI_GroupMemberID.ToString();

		string aiDir = path + "\\";
		ExpansionStatic.DeleteDirectoryStructureRecursive(aiDir, ".bin");

		string aiPath = path + ".bin";
		if (FileExist(aiPath))
			DeleteFile(aiPath);
	}

	eAIGroup GetGroup()
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_0(this, "GetGroup");
#endif

		return m_eAI_Group;
	}

	void Expansion_SetFormerGroup(eAIGroup group)
	{
		m_Expansion_FormerGroup = group;
	}

	eAIGroup Expansion_GetFormerGroup()
	{
		return m_Expansion_FormerGroup;
	}

	int GetGroupID()
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_0(this, "GetGroupID");
#endif

		return m_eAI_GroupID;
	}

	void SetGroupMemberIndex(int index)
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_1(this, "SetGroupMemberIndex").Add(index);
#endif

		m_eAI_GroupMemberIndex = index;
		m_eAI_GroupMemberIndexSynch = index;

		if (GetGame().IsDedicatedServer())
			SetSynchDirty();
	}

	void eAI_SetFactionTypeID(int id)
	{
#ifdef EXTRACE_DIAG
		auto trace = EXTrace.Start(EXTrace.AI, this, "" + eAIFaction.GetTypeByID(id), "" + id);
#endif

		int oldFactionTypeID = m_eAI_FactionTypeID;
		m_eAI_FactionTypeID = id;
		m_eAI_FactionTypeIDSynch = id;

		eAI_OnFactionChange(oldFactionTypeID, id);

		if (GetGame().IsDedicatedServer())
			SetSynchDirty();
	}

	void eAI_OnFactionChange(int oldFactionTypeID, int newFactionTypeID)
	{
	}

	int eAI_GetFactionTypeID()
	{
		return m_eAI_FactionTypeID;
	}

	override void OnVariablesSynchronized()
	{
		super.OnVariablesSynchronized();

		if (m_eAI_GroupID == -1 && m_eAI_Group)
		{
			//! @note this is only ever to be used for players, not AI! AI may NEVER not have a group!

			EXTrace.Print(EXTrace.AI, this, "left group ID " + m_eAI_Group.GetID());

			m_eAI_Group.RemoveMember(this);

			m_eAI_Group = null;

			m_eAI_FactionTypeID = -1;  //! Make sure faction is updated if necessary
		}
		else if (m_eAI_Group && m_eAI_Group.GetID() != m_eAI_GroupID)
		{
			EXTrace.Print(EXTrace.AI, this, "moved from group ID " + m_eAI_Group.GetID() + " -> " + m_eAI_GroupID);

			m_eAI_Group.RemoveMember(this);

			m_eAI_Group = eAIGroup.GetGroupByID(m_eAI_GroupID, true);

			m_eAI_Group.Client_SetMemberIndex(this, m_eAI_GroupMemberIndexSynch);

			m_eAI_FactionTypeID = -1;  //! Make sure faction is updated if necessary
		}
		else if (m_eAI_GroupID != -1 && !m_eAI_Group)
		{
			EXTrace.Print(EXTrace.AI, this, "joined group ID " + m_eAI_GroupID);

			m_eAI_Group = eAIGroup.GetGroupByID(m_eAI_GroupID, true);

			m_eAI_Group.Client_SetMemberIndex(this, m_eAI_GroupMemberIndexSynch);

			m_eAI_FactionTypeID = -1;  //! Make sure faction is updated if necessary
		}
		else if (m_eAI_Group && m_eAI_GroupMemberIndexSynch != m_eAI_GroupMemberIndex)
		{
			EXTrace.Print(EXTrace.AI, this, "moved within group, member index " + m_eAI_GroupMemberIndex + " -> " + m_eAI_GroupMemberIndexSynch);

			// @note: this has to be the last check as when moving/joining groups
			// the index is out of synch and will be handled in the above checks

			m_eAI_Group.Client_SetMemberIndex(this, m_eAI_GroupMemberIndexSynch);
		}

		if (m_eAI_Group && m_eAI_GroupMemberIndexSynch != m_eAI_GroupMemberIndex)
			m_eAI_GroupMemberIndex = m_eAI_GroupMemberIndexSynch;

		if (m_eAI_Group && m_eAI_FactionTypeID != m_eAI_FactionTypeIDSynch)
		{
			EXTrace.Print(EXTrace.AI, this, "changing faction ID from " + m_eAI_FactionTypeID + " -> " + m_eAI_FactionTypeIDSynch);

			m_eAI_FactionTypeID = m_eAI_FactionTypeIDSynch;
	
			typename factionType = eAIFaction.GetTypeByID(m_eAI_FactionTypeIDSynch);

			if (factionType && m_eAI_Group.GetFaction().Type() != factionType)
			{
				EXTrace.Print(EXTrace.AI, this, "changing faction from " + m_eAI_Group.GetFaction().Type() + " -> " + factionType);

				auto faction = eAIFaction.Cast(factionType.Spawn());
				if (faction)
					m_eAI_Group.SetFaction(faction);
			}
		}
	}

	void eAI_SetPassive(bool state = true)
	{
		m_eAI_IsPassive = state;
	}

	bool eAI_IsPassive()
	{
		if (m_eAI_IsPassive)
			return true;
		if (m_eAI_Group)
			return m_eAI_Group.GetFaction().IsPassive();
		return false;
	}

	bool eAI_IsSideSteppingObstacles()
	{
		return false;
	}

	override bool Expansion_CanBeDamaged(string ammo = string.Empty)
	{
		if (!super.Expansion_CanBeDamaged(ammo))
			return false;

		if (m_eAI_Group)
			return !m_eAI_Group.GetFaction().IsInvincible();
			
		return true;
	}

	override bool EEOnDamageCalculated(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		if (!super.EEOnDamageCalculated(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef))
			return false;

		if (!m_eAI_DamageHandler.OnDamageCalculated(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef))
			return false;

		return true;
	}

	//! Suppress "couldn't kill player" in server logs when AI gets killed
	Hive GetHive()
	{
		#ifdef EXTRACE_DIAG
		auto trace = EXTrace.Start(EXTrace.AI, this);
		#endif

		if (IsAI())
			return null;

		return Expansion_GlobalGetHive();
	}

	override void EEKilled(Object killer)
	{
		m_TargetInformation.OnDeath(killer);

		super.EEKilled(killer);

		//! Since we're going to remove this player from their group, do it in next frame so other mods can still access group in EEKilled
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Call(eAI_Cleanup, false);
	}

	/**
	 * @brief Do cleanup after AI is killed or destroyed.
	 * 
	 * @param autoDeleteGroup Will be false when player is killed (called after EEKilled), true when player is being destroyed (called from DTOR)
	 * 
	 * @note If player is killed before being destroyed, then GetGroup().RemoveMember will return false on 2nd invocation of eAI_Cleanup
	 * from DTOR, and if there are still other alive group members, then the killed player is removed from deceased group members.
	 * If there are no other alive group members, then group is just destroyed (in the next frame).
	 */
	void eAI_Cleanup(bool autoDeleteGroup = false)
	{
		if (GetGroup() && !GetGroup().RemoveMember(this, autoDeleteGroup) && autoDeleteGroup)
		{
			if (GetGroup().Count())
				GetGroup().RemoveDeceased(this);
			else
				GetGroup().Delete();
		}
	}

	override void EEHitBy(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
	#ifdef DIAG_DEVELOPER
		EXTrace.PrintHit(EXTrace.AI, this, "EEHitBy[" + m_eAI_DamageHandler.m_HitCounter + "]", damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);

	#ifdef EXPANSION_AI_DMGDEBUG_CHATTY
		ExpansionStatic.MessageNearPlayers(GetPosition(), 100.0, "[" + ExpansionStatic.FormatFloat(GetGame().GetTickTime(), 3, false) + "] hit " + ToString() + " " + dmgZone);
	#endif
	#endif

		m_eAI_LastHitTime = GetGame().GetTickTime();

		m_TargetInformation.OnHit(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);

		super.EEHitBy(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);

		if (IsDamageDestroyed())
			return;

		if (!GetGroup())
			return;

		ZombieBase zmb;
		if (Class.CastTo(zmb, source))
		{
			zmb.GetTargetInformation().AddFriendlyAI(this);

			return;
		}

		PlayerBase player;
		if (Class.CastTo(player, source.GetHierarchyRootPlayer()) && player != this)
		{
			//! If attacker is not AI, or we are their current target (else it was accidental friendly fire),
			//! target attacker for up to 2 minutes
			eAIBase ai;
			if (!Class.CastTo(ai, player) || (ai.GetTarget() && ai.GetTarget().GetEntity() == this))
			{
				if (!ai || IsAI())
					player.GetTargetInformation().AddFriendlyAI(this, 120000, true, 0.21);  //! Attacking player will be attacked by friendly AI
				else
					GetGroup().AddTarget(this, player.GetTargetInformation(), 120000, true, 0.21);   //! Attacking friendly AI will be attacked by group members of player
			}

			return;
		}

		AnimalBase animal;
		if (Class.CastTo(animal, source))
		{
			animal.GetTargetInformation().AddFriendlyAI(this);

			return;
		}

		//! If we loose 5.55555% of current damage zone health or more by vehicle hit, add vehicle as target
		CarScript vehicle;
		if (Class.CastTo(vehicle, source) && damageResult.GetDamage(dmgZone, "Health") >= GetHealth(dmgZone, "Health") * 0.055555)
		{
			vehicle.GetTargetInformation().AddFriendlyAI(this);
		}
	}

	override void EEHealthLevelChanged(int oldLevel, int newLevel, string zone)
	{
	#ifdef DIAG_DEVELOPER
		EXTrace.Print(EXTrace.AI, this, "EEHealthLevelChanged " + oldLevel + " -> " + newLevel + " " + zone);
	#endif

		m_TargetInformation.OnHealthLevelChanged(oldLevel, newLevel, zone);

		super.EEHealthLevelChanged(oldLevel, newLevel, zone);
	}

	bool eAI_UpdateAgressionTimeout(float timeThreshold)
	{
		if (!m_eAI_LastAggressionTime)
			return false;

		float time = ExpansionStatic.GetTime(true);
		float timeout = timeThreshold - (time - m_eAI_LastAggressionTime);
		bool active;
		if (timeout > 0)
			active = true;

		if (active && time + timeout > m_eAI_LastAggressionTimeout)
		{
			m_eAI_LastAggressionTimeout = time + timeout;
			SetSynchDirty();
		}

		return active;
	}

	float eAI_GetLastAggressionCooldown()
	{
		float cooldown = m_eAI_LastAggressionTimeout - ExpansionStatic.GetTime(true);
		if (cooldown > 0)
			return cooldown;

		return 0;
	}

	override void AddNoise(NoiseParams noisePar, float noiseMultiplier = 1.0)
	{
		super.AddNoise(noisePar, noiseMultiplier);

		//! Because noises may fire rapidly, we only update this once every second
		float time = GetGame().GetTickTime();
		if (time - m_eAI_LastNoiseTime < 1.0)
			return;

		m_eAI_LastNoiseTime = time;

		DayZPlayerType type = GetDayZPlayerType();
		string cfgPath = "CfgVehicles SurvivorBase ";
		switch (noisePar)
		{
			case type.GetNoiseParamsStand():
				eAINoiseSystem.AddNoise(this, cfgPath + "NoiseStepStand", noiseMultiplier);
				break;
			case type.GetNoiseParamsCrouch():
				eAINoiseSystem.AddNoise(this, cfgPath + "NoiseStepCrouch", noiseMultiplier);
				break;
			case type.GetNoiseParamsProne():
				eAINoiseSystem.AddNoise(this, cfgPath + "NoiseStepProne", noiseMultiplier);
				break;
			case type.GetNoiseParamsLandLight():
				eAINoiseSystem.AddNoise(this, cfgPath + "NoiseLandLight", noiseMultiplier);
				break;
			case type.GetNoiseParamsLandHeavy():
				eAINoiseSystem.AddNoise(this, cfgPath + "NoiseLandHeavy", noiseMultiplier);
				break;
			case type.GetNoiseParamsWhisper():
				eAINoiseSystem.AddNoise(this, cfgPath + "NoiseWhisper", noiseMultiplier);
				break;
			case type.GetNoiseParamsTalk():
				eAINoiseSystem.AddNoise(this, cfgPath + "NoiseTalk", noiseMultiplier);
				break;
			case type.GetNoiseParamsShout():
				eAINoiseSystem.AddNoise(this, cfgPath + "NoiseShout", noiseMultiplier);
				break;
		#ifdef DIAG_DEVELOPER
			default:
				if (noisePar)
					EXTrace.Print(EXTrace.AI, this, "::AddNoise " + noisePar + " " + noiseMultiplier);
				break;
		#endif
		}
	}

#ifdef DIAG_DEVELOPER
#ifndef SERVER
	void AddShape(Shape shape)
	{
		m_Expansion_DebugShapes.Insert(shape);
	}
#endif

	override void CommandHandler(float pDt, int pCurrentCommandID, bool pCurrentCommandFinished)
	{
#ifndef SERVER
		for (int i = m_Expansion_DebugShapes.Count() - 1; i >= 0; i--)
			m_Expansion_DebugShapes[i].Destroy();
		m_Expansion_DebugShapes.Clear();
#endif

		if (DEBUG_EXPANSION_AI_CLIMB != 0)
		{
			PlayerBase playerPB = PlayerBase.Cast(this);
	
			SHumanCommandClimbResult result();
			
			if (DEBUG_EXPANSION_AI_CLIMB & 0x01 != 0)
			{
				HumanCommandClimb.DoClimbTest(playerPB, result, 0);
				ExpansionClimb.DebugClimb(playerPB, result, 0xAAFFFF00, 0xAA00FFFF);
			}
			
			if (DEBUG_EXPANSION_AI_CLIMB & 0x10 != 0)
			{
				ExpansionClimb.DoClimbTest(playerPB, result);
				ExpansionClimb.DebugClimb(playerPB, result, 0xAAFF0000, 0xAA0000FF);
			}
		}

		super.CommandHandler(pDt, pCurrentCommandID, pCurrentCommandFinished);
	}
#endif
};
