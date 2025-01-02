modded class ItemBase
{
	private ref eAIItemTargetInformation m_TargetInformation;
	ref eAIDamageHandler m_eAI_DamageHandler = new eAIDamageHandler(this, m_TargetInformation);

	bool m_Expansion_IsOwnerPlayer;
	float m_eAI_LastRecordedImpactTime;

	void ItemBase()
	{
		m_TargetInformation = CreateTargetInformation();
	}

	protected eAIItemTargetInformation CreateTargetInformation()
	{
		return new eAIItemTargetInformation(this);
	}

	eAIItemTargetInformation GetTargetInformation()
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_0(this, "GetTargetInformation");
#endif

		return m_TargetInformation;
	}

	override void EEKilled(Object killer)
	{
		m_TargetInformation.OnDeath(killer);

		super.EEKilled(killer);

		eAIBase ai;
		if (Class.CastTo(ai, GetHierarchyRootPlayer()))
		{
			EXTrace.Print(EXTrace.AI, this, "::EEKilled - " + ai);
			ai.eAI_OnItemDestroyed(this);
		}
	}

	override bool EEOnDamageCalculated(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		if (!super.EEOnDamageCalculated(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef))
			return false;

		if (!m_eAI_DamageHandler.OnDamageCalculated(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef))
			return false;

		return true;
	}

	override void EEHitBy(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
	#ifdef DIAG_DEVELOPER
		EXTrace.PrintHit(EXTrace.AI, this, "EEHitBy[" + m_eAI_DamageHandler.m_HitCounter + "]", damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);

	#ifdef EXPANSION_AI_DMGDEBUG_CHATTY
		ExpansionStatic.MessageNearPlayers(GetPosition(), 100.0, "[" + ExpansionStatic.FormatFloat(GetGame().GetTickTime(), 3, false) + "] hit " + ToString() + " " + dmgZone);
	#endif
	#endif

		m_TargetInformation.OnHit(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);

		super.EEHitBy(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);
	}

	override void EEHealthLevelChanged(int oldLevel, int newLevel, string zone)
	{
		m_TargetInformation.OnHealthLevelChanged(oldLevel, newLevel, zone);

		super.EEHealthLevelChanged(oldLevel, newLevel, zone);

		if (!GetGame().IsServer())
			return;

		eAIBase ai;
		if (Class.CastTo(ai, GetHierarchyRootPlayer()) && GetInventory().IsAttachment() && m_Initialized)
			ai.eAI_UpdateProtectionLevels();
	}

	override void EEItemAttached(EntityAI item, string slot_name)
	{
		super.EEItemAttached(item, slot_name);

		if (!GetGame().IsServer())
			return;

		eAIBase ai;
		if (Class.CastTo(ai, GetHierarchyParent()))
			ai.eAI_UpdateProtectionLevels(this, 1);
	}

	override void EEItemDetached(EntityAI item, string slot_name)
	{
		super.EEItemDetached(item, slot_name);

		if (!GetGame().IsServer())
			return;

		eAIBase ai;
		if (Class.CastTo(ai, GetHierarchyParent()))
			ai.eAI_UpdateProtectionLevels(this, -1);
	}

	override void OnInventoryEnter(Man player)
	{
		super.OnInventoryEnter(player);

		m_TargetInformation.RemoveFromAll();

		eAIBase ai;
		if (!Class.CastTo(ai, player))
		{
			m_Expansion_IsOwnerPlayer = true;
			return;
		}

		//! TODO: Latest 1.26 patch on 2024-11-19 added OnChildItemReceived which would be a better place for the following code
		if (!GetGame().IsServer())
			return;

		EXTrace.Print(EXTrace.AI, this, "::OnInventoryEnter - " + player);

		ai.eAI_OnInventoryEnter(this);
	}

	override void OnInventoryExit(Man player)
	{
		super.OnInventoryExit(player);

		//! TODO: Latest 1.26 patch on 2024-11-19 added OnChildItemRemoved which would be a better place for the following code
		if (!GetGame().IsServer())
			return;

		eAIBase ai;
		if (!Class.CastTo(ai, player))
			return;

		EXTrace.Print(EXTrace.AI, this, "::OnInventoryExit - " + player);

		ai.eAI_OnInventoryExit(this);
	}

	override void OnWasAttached(EntityAI parent, int slot_id)
	{
		super.OnWasAttached(parent, slot_id);

		if (!GetGame().IsServer())
			return;

		eAIBase ai;
		if (!Class.CastTo(ai, parent.GetHierarchyRootPlayer()))
			return;

		//! If mag was attached to item that's in AI possession, remove from tracked list
		Magazine mag;
		if (Class.CastTo(mag, this))
		{
			EXTrace.Print(EXTrace.AI, this, "::OnWasAttached - " + ai + " " + parent + " " + InventorySlots.GetSlotName(slot_id));
			ai.eAI_OnMagAttached(parent, slot_id, mag);
		}
	}

	override void OnWasDetached(EntityAI parent, int slot_id)
	{
		super.OnWasDetached(parent, slot_id);

		if (!GetGame().IsServer())
			return;

		eAIBase ai;
		if (!Class.CastTo(ai, parent.GetHierarchyRootPlayer()))
			return;

		//! If mag was detached from item that's in AI possession, add to tracked list
		Magazine mag;
		if (Class.CastTo(mag, this))
		{
			EXTrace.Print(EXTrace.AI, this, "::OnWasDetached - " + ai + " " + parent + " " + InventorySlots.GetSlotName(slot_id));
			ai.eAI_OnMagDetached(parent, slot_id, mag);
		}
	}

	override void EOnContact(IEntity other, Contact extra)
	{
		if (m_CanPlayImpactSound && GetGame().IsServer())
		{
			float time = GetGame().GetTickTime();
			if (time - m_eAI_LastRecordedImpactTime > 1.0)
			{
				//! Because impacts may happen rapidly, we only update this once every second
				m_eAI_LastRecordedImpactTime = time;

			/*
				//! This WOULD work if all items had NoiseImpact...
				string path;

				if (IsWeapon())
					path = CFG_WEAPONSPATH;
				else if (IsMagazine())
					path = CFG_MAGAZINESPATH;
				else
					path = CFG_VEHICLESPATH;
	
				eAINoiseSystem.AddNoise(this, path + " " + GetType() + " NoiseImpact", m_ImpactSpeed, eAINoiseType.SOUND);
			*/

				eAINoiseSystem.AddNoise(this, CFG_VEHICLESPATH + " Bone NoiseImpact", 0.1, eAINoiseType.SOUND);
			}
		}
	}

	override bool AreChildrenAccessible()
	{
		eAIBase ai;
		if (Class.CastTo(ai, GetHierarchyRootPlayer()))
			return true;

		return super.AreChildrenAccessible();
	}

	bool Expansion_TryTurningOn()
	{
		if ( HasEnergyManager() )
		{
			if ( GetCompEM().CanWork() )
			{
				GetCompEM().SwitchOn();
				return true;
			}
		}
		return false;
	}

	bool Expansion_TryTurningOff()
	{
		if ( HasEnergyManager() )
		{
			if ( GetCompEM().IsSwitchedOn() )
			{
				GetCompEM().SwitchOff();
				return true;
			}
		}
		return false;
	}

	bool Expansion_TryTurningOnAnyLightsOrNVG(out float nightVisibility, PlayerBase player, bool skipNonNVG = false, bool skipNVG = false)
	{
		if ( !skipNVG && !player.IsUnconscious() && !player.IsRestrained() )
		{
			NVGoggles goggles = NVGoggles.Cast(FindAttachmentBySlotName("NVG"));
			if (goggles)
			{
				goggles.RotateGoggles(false);
				nightVisibility = 0.35;
				EXTrace.Print(EXTrace.AI, player, "switched on " + goggles.ToString());
				return true;
			}
		}
		
		ActionManagerBase actionManager = player.GetActionManager();
		ActionTarget atrg = new ActionTarget(this, null, -1, vector.Zero, -1);

		if ( !skipNonNVG )
		{
			if ( actionManager.GetAction(ActionTurnOnHelmetFlashlight).Can(player, atrg, this) )
			{
				ItemBase itemChild = ItemBase.Cast(FindAttachmentBySlotName("helmetFlashlight"));
				if ( itemChild.Expansion_TryTurningOn() )
				{
					nightVisibility = 0.15;
					EXTrace.Print(EXTrace.AI, player, "switched on " + itemChild.ToString());
					return true;
				}
			}

			if ( IsInherited(Headtorch_ColorBase) && actionManager.GetAction(ActionTurnOnHeadtorch).Can(player, atrg, this) )
			{
				if ( Expansion_TryTurningOn() )
				{
					nightVisibility = 0.15;
					EXTrace.Print(EXTrace.AI, player, "switched on head torch");
					return true;
				}
			}
			else if ( actionManager.GetAction(ActionTurnOnWhileInHands).Can(player, atrg, this) )
			{
				if ( Expansion_TryTurningOn() )
				{
					nightVisibility = 0.15;
					EXTrace.Print(EXTrace.AI, player, "switched on " + ToString());
					return true;
				}
			}
		}

		return false;
	}

	bool Expansion_TryTurningOffAnyLightsOrNVG(PlayerBase player, bool skipNVG = false)
	{
		if ( !skipNVG && !player.IsUnconscious() && !player.IsRestrained() )
		{
			NVGoggles goggles = NVGoggles.Cast(FindAttachmentBySlotName("NVG"));
			if (goggles)
			{
				goggles.RotateGoggles(true);
				EXTrace.Print(EXTrace.AI, player, "switched off " + goggles.ToString());
				return true;
			}
		}

		ActionManagerBase actionManager = player.GetActionManager();
		ActionTarget atrg = new ActionTarget(this, null, -1, vector.Zero, -1);

		if ( actionManager.GetAction(ActionTurnOffHelmetFlashlight).Can(player, atrg, this) )
		{
			ItemBase itemChild = ItemBase.Cast(FindAttachmentBySlotName("helmetFlashlight"));
			if ( itemChild.Expansion_TryTurningOff() )
			{
				EXTrace.Print(EXTrace.AI, player, "switched off " + itemChild.ToString());
				return true;
			}
		}

		if ( IsInherited(Headtorch_ColorBase) && actionManager.GetAction(ActionTurnOffHeadtorch).Can(player, atrg, this) )
		{
			if ( Expansion_TryTurningOff() )
			{
				EXTrace.Print(EXTrace.AI, player, "switched off head torch");
				return true;
			}
		}
		else if ( actionManager.GetAction(ActionTurnOffWhileInHands).Can(player, atrg, this) )
		{
			if ( Expansion_TryTurningOff() )
			{
				EXTrace.Print(EXTrace.AI, player, "switched off " + ToString());
				return true;
			}
		}

		return false;
	}

	Man Expansion_GetRootPlayerAliveExcluding(Man player)
	{
		Man owner = GetHierarchyRootPlayer();
		if (owner && owner != player && owner.IsAlive())
			return owner;

		return null;
	}
};
