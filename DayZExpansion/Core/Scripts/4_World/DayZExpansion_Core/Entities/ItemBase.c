/**
 * ItemBase.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

modded class ItemBase
{
	protected int m_ExpansionSaveVersion;

	//! Skinning
	protected ExpansionSkinModule m_SkinModule;
	protected ExpansionSkin m_CurrentSkin;

	protected string m_CurrentSkinName;
	protected int m_CurrentSkinIndex;
	protected int m_CurrentSkinSynchRemote;

	protected bool m_CanBeSkinned;
	protected autoptr array< ExpansionSkin > m_Skins;
	protected bool m_Expansion_SZCleanup;
	protected bool m_Expansion_IsStoreLoaded;
	protected bool m_Expansion_IsStoreSaved;

	protected ref map<string, float> m_Expansion_HealthBeforeHit;
	protected float m_Expansion_DamageMultiplier = 1.0;

	bool m_Expansion_AcceptingAttachment;
	bool m_Expansion_CanPlayerAttach;

	bool m_Expansion_IsAdminTool;
	bool m_Expansion_IsMeleeWeapon;

	bool m_Expansion_IsWorking;
	Man m_Expansion_PreviousOwner;
	Man m_Expansion_CurrentOwner;

	protected int m_Expansion_QueuedActions;
	protected bool m_Expansion_IsLootable = true;

	ref ExpansionNetsyncData m_Expansion_NetsyncData;
	ref ExpansionRPCManager m_Expansion_RPCManager;

	ref ExpansionSoundSet m_Expansion_DestroySound_SoundSet;

	ref ExpansionGlobalID m_Expansion_GlobalID;

	void ItemBase()
	{
		m_Expansion_IsAdminTool = ConfigGetBool("expansionIsAdminTool");

		Expansion_SetIsMeleeWeapon();

		ExpansionSetupSkins();

		RegisterNetSyncVariableInt( "m_CurrentSkinSynchRemote", 0, m_Skins.Count() );

		//! TODO: Once vanilla makes up its mind that untakeable items should not show in vicinity (they have code for it in VicinityItemManager,
		//! it just doesn't seem to work), could get rid of the netsync (but not the variable itself)
		RegisterNetSyncVariableBool("m_Expansion_IsLootable");

		if (IsMissionHost())
		{
			m_Expansion_HealthBeforeHit = new map<string, float>;
		}

		string destroySound = GetDestroySound();
		if (destroySound)
			m_Expansion_DestroySound_SoundSet = ExpansionSoundSet.Register(destroySound);
	}
	
	override string GetDisplayName()
	{
		string displayName = super.GetDisplayName();

		if (displayName.IndexOf("$UNT$") == 0)
			displayName.Replace("$UNT$", "");

		return displayName;
	}

	ExpansionGlobalID Expansion_GetGlobalID()
	{
		return m_Expansion_GlobalID;
	}
	
	//============================================
	// GetExpansionSaveVersion
	//============================================
	int GetExpansionSaveVersion()
	{
		return m_ExpansionSaveVersion;
	}
		
	//============================================
	// OnStoreLoad
	//============================================
	override bool OnStoreLoad( ParamsReadContext ctx, int version )
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.GENERAL_ITEMS, this, "OnStoreLoad");
#endif
		
		m_Expansion_IsStoreLoaded = true;

		return super.OnStoreLoad( ctx, version );
	}

	#ifdef EXPANSION_MODSTORAGE
	override void CF_OnStoreSave(CF_ModStorageMap storage)
	{
		super.CF_OnStoreSave(storage);

		//! Queue world untakeable items for next server start
		if (!m_Expansion_IsLootable && !m_Expansion_IsStoreSaved && !GetHierarchyParent() && GetLifetime())
			Expansion_QueueEntityActions(ExpansionItemBaseModule.SETUNLOOTABLE);

		m_Expansion_IsStoreSaved = true;

		auto ctx = storage[DZ_Expansion_Core];
		if (!ctx) return;

		ctx.Write(m_CurrentSkinName);

		if (m_Expansion_GlobalID && m_Expansion_GlobalID.m_IsSet)
		{
			ctx.Write(true);
			m_Expansion_GlobalID.OnStoreSave(ctx);
		}
		else
		{
			ctx.Write(false);
		}
	}
	
	override bool CF_OnStoreLoad(CF_ModStorageMap storage)
	{
		if (!super.CF_OnStoreLoad(storage))
			return false;

		auto ctx = storage[DZ_Expansion_Core];
		if (!ctx) return true;

		//! @note needed so we can sucessfully load player inventory when they haven't connected in the transition period
		//! as well as menu character inventory (can cause client CTD otherwise)
		m_ExpansionSaveVersion = ctx.GetVersion();

		if (!ctx.Read(m_CurrentSkinName))
			return false;

		if (ctx.GetVersion() < 52)
			return true;

		bool hasGlobalID;
		if (!ctx.Read(hasGlobalID))
			return false;

		if (hasGlobalID)
		{
			if (!m_Expansion_GlobalID)
				m_Expansion_GlobalID = new ExpansionGlobalID();

			if (!m_Expansion_GlobalID.OnStoreLoad(ctx))
				return false;
		}

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

	bool ExpansionIsLocked()
	{
		return false;
	}

	static void ExpansionPhaseObject(EntityAI other)
	{
#ifdef EXPANSIONMODVEHICLE
		ExpansionVehicleBase evb;
		CarScript car;

		if (Class.CastTo(car, other))
		{
			SetVelocity(other, car.m_State.m_LinearVelocity);
			dBodySetAngularVelocity(other, car.m_State.m_AngularVelocity);
		}
		else if (Class.CastTo(evb, other))
		{
			SetVelocity(other, evb.m_State.m_LinearVelocity);
			dBodySetAngularVelocity(other, evb.m_State.m_AngularVelocity);
		}
		else
#endif
		{
		}
	}

	void ExpansionCreateDynamicFromContact(EntityAI other, vector position, Contact data)
	{
		CreateDynamicPhysics(PhxInteractionLayers.DYNAMICITEM);
		EnableDynamicCCD(false);

		ExpansionHandleCollision(other, position);
	}

	void ExpansionHandleCollision(EntityAI other, vector position)
	{
		vector transform[4];
		GetTransform(transform);

		vector otherTransform[4];
		other.GetTransform(otherTransform);

		vector hitVelocity;
		float massA = dBodyGetMass(this);
		float massB = dBodyGetMass(other);
		float massNorm = 0.0;
		if (massA < massB)
			massNorm = (0.5 * massA / massB);
		else
			massNorm = 1.0 - (0.5 * massB / massA);

#ifdef EXPANSIONMODVEHICLE
		CarScript car;
		ExpansionVehicleBase evb;
		if (Class.CastTo(car, other))
		{
			hitVelocity = car.m_State.GetWorldVelocityAt(position.InvMultiply4(otherTransform)).Multiply3(transform);
		}
		else if (Class.CastTo(evb, other))
		{
			hitVelocity = evb.m_State.GetWorldVelocityAt(position.InvMultiply4(otherTransform)).Multiply3(transform);
		}
		else
#endif
		{
		}

		vector linearVelocity;
		vector angularVelocity;

		linearVelocity = -hitVelocity * (1.0 - massNorm);
		angularVelocity = position.InvMultiply4(otherTransform) * linearVelocity;

		SetVelocity(other, linearVelocity.InvMultiply3(otherTransform));
		dBodySetAngularVelocity(other, angularVelocity.InvMultiply3(otherTransform));

		linearVelocity = hitVelocity * massNorm;
		angularVelocity = position.InvMultiply4(transform) * linearVelocity;

		SetVelocity(this, linearVelocity.InvMultiply3(transform));
		dBodySetAngularVelocity(this, angularVelocity.InvMultiply3(transform));
	}
	
	//! Return true if the item contains liquid
	bool ExpansionIsLiquidItem()
	{
		return IsLiquidContainer() || IsKindOf("SodaCan_ColorBase");
	}

	void Expansion_SetIsMeleeWeapon()
	{
		if (IsWeapon())
			return;

		if (IsMeleeWeapon())
		{
			//! basically any item has isMeleeWeapon=1 in config, but we only want actual viable melee options
			//! (axes, knifes, sickles...)
			TStringArray inventorySlot();
			ConfigGetTextArray("inventorySlot", inventorySlot);
			if (inventorySlot.Find("Shoulder") == -1 && inventorySlot.Find("Melee") == -1 && inventorySlot.Find("Knife") == -1)
			{
				TStringArray itemInfo();
				ConfigGetTextArray("itemInfo", itemInfo);
				if (itemInfo.Find("Knife") == -1 && itemInfo.Find("Axe") == -1)
				{
					string suicideAnim = ConfigGetString("suicideAnim");
					if (!suicideAnim || suicideAnim != "woodaxe")
						return;
				}
			}

			m_Expansion_IsMeleeWeapon = true;
		}
	}

	bool Expansion_IsMeleeWeapon()
	{
		return m_Expansion_IsMeleeWeapon;
	}

	bool Expansion_IsMechanicalTrap()
	{
		return false;
	}

	bool Expansion_IsDanger()
	{
		return false;
	}

	bool Expansion_IsExplosive()
	{
		return IsExplosive();
	}

	bool Expansion_IsLiveExplosive()
	{
		ExplosivesBase explosive;
		if (Class.CastTo(explosive, this) && explosive.Expansion_IsLive())
			return true;

		return false;
	}

	TStringArray Expansion_GetAmmoTypes()
	{
		return {ConfigGetString("ammoType")};
	}

	/*! ExpansionActionOn<...>

		Override these in inherited items and call them from within an action
		if you want the item to react to different phases of the action.
	*/

	void ExpansionActionOnStart( ActionData action_data )
	{
	}

	void ExpansionActionOnAnimationEvent( ActionData action_data )
	{
	}

	void ExpansionActionOnStartAnimationLoop( ActionData action_data )
	{
	}

	void ExpansionActionOnUpdate( ActionData action_data )
	{
	}

	void ExpansionActionOnFinishProgress( ActionData action_data )
	{
	}

	void ExpansionActionOnEndRequest( ActionData action_data )
	{
	}

	void ExpansionActionOnEndInput( ActionData action_data )
	{
	}

	void ExpansionActionOnEndAnimationLoop( ActionData action_data )
	{
	}

	void ExpansionActionOnEnd( ActionData action_data )
	{
	}
	
	void ExpansionActionOnStageStart( SequentialActionData action_data )
	{
	}

	void ExpansionActionOnStageEnd( SequentialActionData action_data )
	{
	}

	// This is just here so it can be overriden in derived classes, but should return a useful default.
	bool CanBeDamaged()
	{
		return GetAllowDamage();
	}

	protected string GetDestroySound()
	{
		return "";
	}

	override void OnVariablesSynchronized()
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.GENERAL_ITEMS, this, "OnVariablesSynchronized");
#endif

		super.OnVariablesSynchronized();

		if ( m_CanBeSkinned && m_CurrentSkinSynchRemote != m_CurrentSkinIndex )
		{
			m_CurrentSkinIndex = m_CurrentSkinSynchRemote;
			if ( m_CurrentSkinIndex >= 0 && m_CurrentSkinIndex < m_Skins.Count() )
			{
				m_CurrentSkinName = m_SkinModule.GetSkinName( GetType(), m_CurrentSkinIndex );
				m_CurrentSkin = m_Skins[ m_CurrentSkinIndex ];
			} else
			{
				m_CurrentSkinName = "";
				m_CurrentSkin = NULL;	
			}

			ExpansionOnSkinUpdate();
		}

		if (m_Expansion_NetsyncData && !m_Expansion_NetsyncData.m_WasDataRequested)
			m_Expansion_NetsyncData.Request();
	}

	//============================================
	// ExpansionSetupSkins
	//============================================
	protected void ExpansionSetupSkins()
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.SKIN, this, "ExpansionSetupSkins");
#endif

		m_Skins = new array< ExpansionSkin >;

		if ( CF_Modules<ExpansionSkinModule>.Get(m_SkinModule) )
		{
			m_SkinModule.RetrieveSkins( GetType(), m_Skins, m_CurrentSkinName );
		}

		if (m_Skins.Count() != 0)
			m_CanBeSkinned = true;

		if ( m_CanBeSkinned )
		{
			if ( m_CurrentSkinName != "" )
			{
				m_CurrentSkinIndex = m_SkinModule.GetSkinIndex( GetType(), m_CurrentSkinName );
			} else
			{
				m_CurrentSkinIndex = 0;
				
				m_CurrentSkinName = m_SkinModule.GetSkinName( GetType(), m_CurrentSkinIndex );
			}
			
			m_CurrentSkinSynchRemote = m_CurrentSkinIndex;
			m_CurrentSkin = m_Skins[ m_CurrentSkinIndex ];

			ExpansionOnSkinUpdate();
		}
	}

	array< ExpansionSkin > ExpansionGetSkins()
	{
		return m_Skins;
	}

	bool ExpansionHasSkin( int skinIndex )
	{
		return skinIndex > -1 && skinIndex < m_Skins.Count();
	}

	//============================================
	// EEHealthLevelChanged
	//============================================
	override void EEHealthLevelChanged( int oldLevel, int newLevel, string zone )
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.GENERAL_ITEMS, this, "EEHealthLevelChanged");
#endif

		super.EEHealthLevelChanged( oldLevel, newLevel, zone );
	
		if ( m_CanBeSkinned && m_CurrentSkin )
		{
			string sZone = zone;
			sZone.ToLower();

			for ( int i = 0; i < m_CurrentSkin.DamageZones.Count(); i++ )
			{
				string cZone = m_CurrentSkin.DamageZones[i].Zone;
				cZone.ToLower();

				if ( cZone == sZone )
				{
					ExpansionOnSkinDamageZoneUpdate( m_CurrentSkin.DamageZones[i], newLevel );
				}
			}
		}
	}

	//============================================
	// ExpansionSetSkin
	//============================================
	void ExpansionSetSkin( int skinIndex )
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.SKIN, this, "ExpansionSetSkin");
#endif

		#ifdef EXPANSION_SKIN_LOGGING
		Print( m_CanBeSkinned );
		Print( skinIndex );
		#endif

		if ( !m_CanBeSkinned )
		{
			m_CurrentSkinName = "";

			return;
		}

		m_CurrentSkinIndex = skinIndex;

		if ( m_CurrentSkinIndex < 0 )
		{
			m_CurrentSkinIndex = 0;
		}

		if ( m_CurrentSkinIndex >= m_Skins.Count() )
		{
			m_CurrentSkinIndex = 0;
		}

		#ifdef EXPANSION_SKIN_LOGGING
		Print( m_CurrentSkinIndex );
		#endif

		m_CurrentSkinName = m_SkinModule.GetSkinName( GetType(), m_CurrentSkinIndex );
		m_CurrentSkinSynchRemote = m_CurrentSkinIndex;
		m_CurrentSkin = m_Skins[ m_CurrentSkinIndex ];

		#ifdef EXPANSION_SKIN_LOGGING
		Print( m_CurrentSkinName );
		Print( m_CurrentSkinSynchRemote );
		Print( m_CurrentSkin );
		#endif

		ExpansionOnSkinUpdate();

		SetSynchDirty();
	}

	//============================================
	// ExpansionOnSkinDamageZoneUpdate
	//============================================
	void ExpansionOnSkinDamageZoneUpdate( ExpansionSkinDamageZone zone, int level )
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.SKIN, this, "ExpansionOnSkinDamageZoneUpdate");
#endif

		#ifdef EXPANSION_SKIN_LOGGING
		Print( zone );
		Print( level );
		#endif

		for ( int i = 0; i < zone.HiddenSelections.Count(); i++ )
		{
			int selectionIndex = GetHiddenSelectionIndex( zone.HiddenSelections[i] );

			#ifdef EXPANSION_SKIN_LOGGING
			Print( "HiddenSelection: " + zone.HiddenSelections[i] );
			Print( "SelectionIndex: " + selectionIndex );
			#endif

			if ( level >= 0 && level < zone.HealthLevels.Count() )
			{
				ExpansionSkinHealthLevel healthLevel = zone.HealthLevels[level];

				#ifdef EXPANSION_SKIN_LOGGING
				Print( "RVTexture: " + healthLevel.RVTexture );
				Print( "RVMaterial: " + healthLevel.RVMaterial );
				#endif

				SetObjectTexture( selectionIndex, healthLevel.RVTexture );
				SetObjectMaterial( selectionIndex, healthLevel.RVMaterial );
			}
		}
	}

	//============================================
	// ExpansionOnSkinUpdate
	//============================================
	void ExpansionOnSkinUpdate()
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.SKIN, this, "ExpansionOnSkinUpdate");
#endif

		if ( !m_CurrentSkin )
		{
			return;
		}
		
		for ( int i = 0; i < m_CurrentSkin.HiddenSelections.Count(); i++ )
		{
			ExpansionSkinHiddenSelection selection = m_CurrentSkin.HiddenSelections[ i ];

			int selectionIndex = GetHiddenSelectionIndex( selection.HiddenSelection );

			#ifdef EXPANSION_SKIN_LOGGING
			Print( "HiddenSelection: " + selection.HiddenSelection );
			Print( "SelectionIndex: " + selectionIndex );
			Print( "RVTexture: " + selection.RVTexture );
			Print( "RVMaterial: " + selection.RVMaterial );
			#endif

			SetObjectTexture( selectionIndex, selection.RVTexture );
			SetObjectMaterial( selectionIndex, selection.RVMaterial );
		}
		
		for ( i = 0; i < m_CurrentSkin.DamageZones.Count(); i++ )
		{
			ExpansionOnSkinDamageZoneUpdate( m_CurrentSkin.DamageZones[i], GetHealthLevel( m_CurrentSkin.DamageZones[i].Zone ) );
		}
	}
	
	ExpansionSkin ExpansionGetCurrentSkin()
	{
		return m_CurrentSkin;
	}

	string ExpansionGetCurrentSkinName()
	{
		return m_CurrentSkinName;
	}
	
	int ExpansionGetCurrentSkinIndex()
	{
		return m_CurrentSkinIndex;
	}

	override void Explode(int damageType, string ammoType = "")
	{
		ExpansionDamageSystem.OnBeforeExplode(this, damageType, ammoType);

		super.Explode(damageType, ammoType);
	}

	override void EEHitBy(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		super.EEHitBy(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);

#ifdef DIAG_DEVELOPER
		EXTrace.PrintHit(EXTrace.GENERAL_ITEMS, this, "EEHitBy", damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);
#endif

		string damageZone = dmgZone;
		if (damageZone == "")
			damageZone = "GlobalHealth";

		float health = m_Expansion_HealthBeforeHit[dmgZone];
		float dmg = damageResult.GetDamage(damageZone, "Health");

		bool applyDamageCorrection;
		if ((damageType == DT_EXPLOSION || damageType == DT_FIRE_ARM) && m_Expansion_DamageMultiplier != 1.0)
			applyDamageCorrection = true;

		if (damageType == DT_EXPLOSION && ExpansionDamageSystem.IsEnabledForExplosionTarget(this))
		{
			ExpansionDamageSystem.OnExplosionHit(source, this, ammo);

			//! Use our own damage system for consistent explosion damage
			//! Note that this only works as intended if damage source root is not a player,
			//! else won't be able to get actual source's position in relation to target

			if (!GetHierarchyParent() && source && !source.GetHierarchyRootPlayer())
			{
				float baseDmg = ExpansionDamageSystem.GetExplosionDamage(source, this, ammo);
				if (baseDmg > dmg)
				{
					ExpansionDamageSystem.Log("Overriding " + source.ToString() + " damage dealt to " + ToString() + " at " + GetPosition() + " " + dmg.ToString() + " -> " + baseDmg.ToString());
					dmg = baseDmg;
					applyDamageCorrection = true;
				}
			}
		}

		if (applyDamageCorrection)
		{
			//! damageMultiplier > 1 applies bonus damage
			//! damageMultiplier < 1 negates damage (partly if multiplier > 0 or fully if 0)
			//! damageMultiplier == 1 effectively does nothing
			if (health > 0)
				SetHealth(damageZone, "Health", Math.Max(health - (dmg * m_Expansion_DamageMultiplier), 0));
		}

#ifdef EXPANSIONMODBASEBUILDING
		if (IsInherited(BaseBuildingBase) || IsInherited(ExpansionSafeBase))
			RaidLog(source, damageZone, ammo, health, dmg, m_Expansion_DamageMultiplier);
#endif
	}

	override bool EEOnDamageCalculated(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		if (!super.EEOnDamageCalculated( damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef))
			return false;

		EntityAI root = GetHierarchyRoot();

		if (root != this)
		{
			DayZPlayerImplement player;
			if (Class.CastTo(player, root))
			{
				if (!player.Expansion_CanBeDamaged(ammo))
					return false;
			}
			else
			{
				ExpansionVehicle vehicle;
				if (ExpansionVehicle.Get(vehicle, root))
				{
					if (!vehicle.CanBeDamaged())
						return false;
				}
			}
		}

		if (damageType == DT_EXPLOSION)
		{
			if (!ExpansionDamageSystem.OnExplosionDamageCalculated(damageResult, source, this, component, dmgZone, ammo, modelPos, speedCoef, false))
				return false;
		}

		m_Expansion_HealthBeforeHit[dmgZone] = GetHealth(dmgZone, "Health");

		return true;
	}

#ifdef EXPANSIONMODBASEBUILDING
	void RaidLog( EntityAI source, string damageZone, float health, float dmg, float damageMultiplier )
	{
		RaidLog( source, damageZone, "", health, dmg, damageMultiplier );
	}

	void RaidLog( EntityAI source, string damageZone, string ammo, float health, float dmg, float damageMultiplier )
	{
		if ( !GetExpansionSettings().GetLog().BaseBuildingRaiding )
			return;
			
		if ( ( dmg * damageMultiplier ) == 0 )
			return;

		PlayerBase player;
		string playerDesc;

		if ( source && Class.CastTo( player, source.GetHierarchyRootPlayer() ) )
		{
			playerDesc = "Player \"" + player.GetIdentityName() + "\" (ID " + player.GetIdentityUID() + ") at " + ExpansionStatic.VectorToString(player.GetPosition());
		} else
		{
			playerDesc = "A player";
		}

		string sourceDesc;

		if (source)
			sourceDesc = source.GetType();
		else
			sourceDesc = "<unknown source>";

		if (ammo)
			sourceDesc = ammo + " from " + sourceDesc;

		GetExpansionSettings().GetLog().PrintLog( "[BaseBuildingRaiding] ------------------------- Expansion Base Raiding Damage Report -------------------------" );
		GetExpansionSettings().GetLog().PrintLog( "[BaseBuildingRaiding] " + playerDesc + " damaged " + GetType() + " at " + ExpansionStatic.VectorToString(GetPosition()) );
		GetExpansionSettings().GetLog().PrintLog( "[BaseBuildingRaiding] They dealt "  + dmg + " * " + damageMultiplier + " = " + ( dmg * damageMultiplier ) + " damage with " + sourceDesc );
		GetExpansionSettings().GetLog().PrintLog( "[BaseBuildingRaiding] " + damageZone + " hitpoints decreased from " + health + " to " + GetHealth( damageZone, "Health" ) );
		GetExpansionSettings().GetLog().PrintLog( "[BaseBuildingRaiding] ----------------------------------------------------------------------------------------" );
	}
#endif

	// ------------------------------------------------------------
	// EEKilled
	// ------------------------------------------------------------
	override void EEKilled( Object killer )
	{
		super.EEKilled( killer );

		if (m_Expansion_DestroySound_SoundSet)
			m_Expansion_DestroySound_SoundSet.Play(GetPosition());

		ExpansionOnDestroyed( killer );
	}

	void ExpansionOnDestroyed( Object killer )
	{
	}

	//! This deals with spawning magazines on weapons correctly and should be used as a replacement for vanilla CreateInInventory
	EntityAI ExpansionCreateInInventory(string className, bool attachOnly = false, int slotId = InventorySlots.INVALID)
	{
		if (!GetInventory())
			return NULL;

		//! NOTE: Both actual magazines and ammo inherit from Magazine_Base
		if (IsInherited(Weapon_Base) && GetGame().IsKindOf(className, "Magazine_Base") && !GetGame().IsKindOf(className, "Ammunition_Base"))
		{
			//! It's an actual magazine
			Weapon_Base weapon = Weapon_Base.Cast(this);

			InventoryLocation il = new InventoryLocation;
			if (slotId == InventorySlots.INVALID)
				slotId = InventorySlots.MAGAZINE;
			il.SetAttachment(weapon, NULL, slotId);
			
			EntityAI magazine = SpawnEntity(className, il, ECE_IN_INVENTORY, RF_DEFAULT);
			if (!magazine)
				return NULL;

			//! Important: Needs to be called BEFORE pushing bullet to chamber, otherwise save will occur in FSM transition
			int stateId = weapon.ExpansionGetMagAttachedFSMStateID();

			int muzzleIndex = weapon.GetCurrentMuzzle();
			if (!weapon.IsChamberFull(muzzleIndex) || weapon.IsChamberFiredOut(muzzleIndex))
			{
				#ifdef EXPANSIONEXPRINT
				EXPrint("ExpansionCreateInInventory - pushing to chamber " + weapon);
				#endif
				pushToChamberFromAttachedMagazine(weapon, muzzleIndex);
			}

			if (stateId > -1)
			{
				ScriptReadWriteContext ctx = new ScriptReadWriteContext;
				ctx.GetWriteContext().Write(stateId);
				weapon.LoadCurrentFSMState(ctx.GetReadContext(), GetGame().SaveVersion());
			}

		#ifdef SERVER
			weapon.Synchronize();
		#else
			weapon.ShowMagazine();
		#endif

			//! We are done
			return magazine;
		}

		//! Any other item
		if (attachOnly)
		{
			if (slotId == InventorySlots.INVALID)
				return GetInventory().CreateAttachment(className);
			else
				return GetInventory().CreateAttachmentEx(className, slotId);
		}

		return GetInventory().CreateInInventory(className);
	}

	EntityAI ExpansionCreateAttachment(string className, int slotId = InventorySlots.INVALID)
	{
		return ExpansionCreateInInventory(className, true, slotId);
	}

	void OnEnterZone(ExpansionZoneType type)
	{
	}

	void OnExitZone(ExpansionZoneType type)
	{
	}
	
	override void EEItemLocationChanged(notnull InventoryLocation oldLoc, notnull InventoryLocation newLoc)
	{
		DayZPlayerImplement old_owner_dpi;
		DayZPlayerImplement new_owner_dpi;

		bool shouldSuper = true;
		
		if (oldLoc.GetParent())
			old_owner_dpi = DayZPlayerImplement.Cast(oldLoc.GetParent().GetHierarchyRootPlayer());
		
		if (newLoc.GetParent())
			new_owner_dpi = DayZPlayerImplement.Cast(newLoc.GetParent().GetHierarchyRootPlayer());

		//! super EEItemLocationChanged wants PlayerBase class, NPCs are DayZPlayerImplement so this is to prevent the super method from being called.
		if (old_owner_dpi && !PlayerBase.Cast(old_owner_dpi))
			shouldSuper = false;
		else if (new_owner_dpi && !PlayerBase.Cast(new_owner_dpi))
			shouldSuper = false;
		
		if (shouldSuper)
			super.EEItemLocationChanged(oldLoc, newLoc);

		if (!GetGame().IsServer())
			return;

		if (newLoc.GetParent() != oldLoc.GetParent())
		{
			if (oldLoc.GetType() == InventoryLocationType.CARGO)
				Expansion_UpdateParentCargoCount(oldLoc, -1);

			if (newLoc.GetType() == InventoryLocationType.CARGO)
				Expansion_UpdateParentCargoCount(newLoc, 1);
		}

		if (!GetExpansionSettings().GetSafeZone().Enabled)
			return;

		if (!GetExpansionSettings().GetSafeZone().EnableForceSZCleanup)
			return;

		//! Ignore unknown old location (item loaded from storage or newly created by CE or script)
		if (oldLoc.GetType() == InventoryLocationType.UNKNOWN)
			return;

		#ifdef EXPANSION_SAFEZONE_DEBUG
		EXPrint("[CORE][Expansion_SZCleanup] " + ToString() + " " + GetPosition() + " location changed " + typename.EnumToString(InventoryLocationType, oldLoc.GetType()) + " -> " + typename.EnumToString(InventoryLocationType, newLoc.GetType()));
		#endif

		if (newLoc.GetType() == InventoryLocationType.GROUND)
		{
			ExpansionCreateCleanup();
		}
		else if (oldLoc.GetType() == InventoryLocationType.GROUND)
		{
			float lifetime;
			if (GetEconomyProfile())
				lifetime = GetEconomyProfile().GetLifetime();
			if (m_Expansion_SZCleanup || GetLifetimeMax() < lifetime)
			{
				m_Expansion_SZCleanup = false;

				//! Make sure to reset max lifetime to value from CE
				SetLifetimeMax(lifetime);

				#ifdef EXPANSION_SAFEZONE_DEBUG
				EXPrint("[CORE][Expansion_SZCleanup] " + ToString() + " " + GetPosition() + " unmarked for cleanup - lifetime " + GetLifetime());
				#endif
			}
		}
	}

	//! Workaround for vanilla not initializing vehicle inv on client if not close to player (cargo count is used by e.g. market to show unsellable items)
	void Expansion_UpdateParentCargoCount(InventoryLocation loc, int delta)
	{
		auto vehicle = ExpansionVehicle.Get(loc.GetParent());
		if (vehicle)
		{
			delta *= GetInventory().CountInventory();
			vehicle.UpdateCargoCount(delta);
			#ifdef DIAG_DEVELOPER
			EXTrace.Print(EXTrace.GENERAL_ITEMS, this, vehicle.GetEntity().ToString() + " cargo count " + delta + " " + vehicle.GetCargoCount());
			#endif
		}
	}

	override void OnInventoryEnter(Man player)
	{
		super.OnInventoryEnter(player);

		Expansion_OnInventoryEnter(player);
	}

	void Expansion_OnInventoryEnter(Man player)
	{
		m_Expansion_CurrentOwner = player;

		PlayerBase pb;
		if (Class.CastTo(pb, player))
			pb.Expansion_OnInventoryUpdate(this, true);

		if (GetCompEM())
			m_Expansion_IsWorking = GetCompEM().IsWorking();
	}

	override void OnInventoryExit(Man player)
	{
		super.OnInventoryExit(player);

		Expansion_OnInventoryExit(player);
	}

	void Expansion_OnInventoryExit(Man player)
	{
		m_Expansion_PreviousOwner = m_Expansion_CurrentOwner;
		m_Expansion_CurrentOwner = null;

		PlayerBase pb;
		if (Class.CastTo(pb, player))
			pb.Expansion_OnInventoryUpdate(this, false);
	}

	override void OnWorkStart()
	{
		super.OnWorkStart();

		Expansion_OnWorkStart();
	}

	void Expansion_OnWorkStart()
	{
		PlayerBase pb;
		if (!m_Expansion_IsWorking && Class.CastTo(pb, GetHierarchyRootPlayer()))
			pb.Expansion_OnInventoryUpdate(this, true, true);

		m_Expansion_IsWorking = true;
	}

	override void OnWorkStop()
	{
		super.OnWorkStop();

		Expansion_OnWorkStop();
	}

	void Expansion_OnWorkStop()
	{
		PlayerBase pb;
		if (m_Expansion_IsWorking && Class.CastTo(pb, GetHierarchyRootPlayer()))
			pb.Expansion_OnInventoryUpdate(this, true, true);

		m_Expansion_IsWorking = false;
	}

	/**
	 * @brief return the root item of the hierarchy
	 * 
	 * I.e. if this item is a scope attached to a gun on a player's shoulder, return the gun.
	 */
	ItemBase Expansion_GetHierarchyRootItem()
	{
		ItemBase rootItem = this;
		ItemBase parentItem;

		while (rootItem.GetHierarchyParent() && Class.CastTo(parentItem, rootItem.GetHierarchyParent()))
		{
			rootItem = parentItem;
		}

		return rootItem;
	}

	/**
	 * @brief return the root of the hierarchy that is not a player
	 * 
	 * I.e. if this item is a scope attached to a gun on a player's shoulder, return the gun,
	 * but if the item is a scope attached to a gun inside a vehicle, return the vehicle.
	 */
	EntityAI Expansion_GetHierarchyRootExcludingPlayer()
	{
		EntityAI root = this;
		EntityAI parent = GetHierarchyParent();

		while (parent && !parent.IsMan())
		{
			root = parent;
			parent = parent.GetHierarchyParent();
		}

		return root;
	}

	/**
	 * @brief checks whether this item is contained in other or any of it's children
	 */
	bool Expansion_IsContainedIn(EntityAI other)
	{
		EntityAI parent = GetHierarchyParent();

		while (parent)
		{
			if (parent == other)
				return true;

			parent = parent.GetHierarchyParent();
		}

		return false;
	}

	typename Expansion_GetFamilyType()
	{
		if (IsInherited(ItemCompass))
		{
			return ItemCompass;
		}
		else if (IsInherited(ItemMap))
		{
			return ItemMap;
		}
		else if (IsInherited(Pen_ColorBase))
		{
			return Pen_ColorBase;
		}
		
		return Type();
	}

	string Expansion_GetTypeLower()
	{
		string type = GetType();
		type.ToLower();
		return type;
	}

	/**
	 * @brief Get raw (untranslated) display name of item
	 * 
	 * This is useful (e.g.) to send over network so client can do translation in their locale
	 */
	string Expansion_GetDisplayNameRaw()
	{
		string displayName = ConfigGetStringRaw("displayName");
		GetGame().FormatRawConfigStringKeys(displayName);
		return displayName;
	}

	override void DeferredInit()
    {
		super.DeferredInit();

		//! Skin

		if ( GetGame().IsServer() && m_CanBeSkinned )
		{
			m_CurrentSkinIndex = m_SkinModule.GetSkinIndex( GetType(), m_CurrentSkinName );
			m_CurrentSkinSynchRemote = m_CurrentSkinIndex;
			m_CurrentSkin = m_Skins[ m_CurrentSkinIndex ];

			ExpansionOnSkinUpdate();
		}

		if (!m_Expansion_IsStoreLoaded)
			return;

		//! Ideally would do the following in AfterStoreLoad, but vanilla doesn't always call super >:(
	
		ExpansionDeferredCreateCleanup();

		int actions = ExpansionItemBaseModule.s_Instance.ProcessQueuedEntityActions(this);
		if ((actions & ExpansionItemBaseModule.SETUNLOOTABLE) == ExpansionItemBaseModule.SETUNLOOTABLE && GetLifetime())
			Expansion_QueueEntityActions(ExpansionItemBaseModule.SETUNLOOTABLE);
    }

	void ExpansionDeferredCreateCleanup()
	{
		if (!GetExpansionSettings().GetSafeZone().Enabled)
			return;

		if (!GetExpansionSettings().GetSafeZone().EnableForceSZCleanup)
			return;

		ExpansionCreateCleanup();
	}

	void Expansion_SetLootable(bool pState)
	{
		m_Expansion_IsLootable = pState;

		SetTakeable(pState);

		if (GetInventory())
		{
			//! 10134 = 2 | 4 | 16 | 128 | 256 | 512 | 1024 | 8192
			if (pState)
				ExpansionStatic.UnlockInventoryRecursive(this, 10134);
			else
				ExpansionStatic.LockInventoryRecursive(this, 10134);
		}

		if (!m_Expansion_QueuedActions || GetHierarchyParent())
			return;

		if (pState)
		{
			//! Deferred removal of setuntakeable entity action from queue
			Expansion_DequeueEntityActions(ExpansionItemBaseModule.SETUNLOOTABLE);
		}
	}

	bool Expansion_IsLootable()
	{
		return m_Expansion_IsLootable;
	}

	override bool IsInventoryVisible()
	{
		if (!super.IsInventoryVisible())
			return false;

		return m_Expansion_IsLootable;
	}

	//! Prevent pickup if it's set to unlootable
	override bool CanPutAsAttachment( EntityAI parent )
	{
		if (!super.CanPutAsAttachment(parent))
			return false;

		if (parent)
		{
			DayZPlayerImplement player;
			if (Class.CastTo(player, parent.GetHierarchyRoot()) && player.Expansion_IsAI())
				return true;  //! AI can always take
		}

		return m_Expansion_IsLootable;
	}

	//! Prevent pickup if it's set to unlootable
	override bool CanPutInCargo(EntityAI parent)
	{
		if (!super.CanPutInCargo(parent))
			return false;

		if (parent)
		{
			DayZPlayerImplement player;
			if (Class.CastTo(player, parent.GetHierarchyRoot()) && player.Expansion_IsAI())
				return true;  //! AI can always take
		}

		return m_Expansion_IsLootable;
	}

	//! Prevent pickup if it's set to unlootable
	override bool CanPutIntoHands(EntityAI parent)
	{
		if (!super.CanPutIntoHands(parent))
			return false;

		if (parent)
		{
			DayZPlayerImplement player;
			if (Class.CastTo(player, parent.GetHierarchyRoot()) && player.Expansion_IsAI())
				return true;  //! AI can always take
		}

		return m_Expansion_IsLootable;
	}

	override void EEDelete(EntityAI parent)
	{
		super.EEDelete(parent);

		if (!m_Expansion_QueuedActions || GetHierarchyParent())
			return;

		//! Deferred removal of all entity actions from queue
		ExpansionItemBaseModule.s_Instance.QueueEntityActions(this, -int.MAX);
		m_Expansion_QueuedActions = 0;
	}

	override void OnCEUpdate()
	{
		super.OnCEUpdate();

		if (m_Expansion_SZCleanup && GetLifetime() < 0)
		{
			if (!GetHierarchyParent())
			{
				GetGame().ObjectDelete(this);

				#ifdef EXPANSION_SAFEZONE_DEBUG
				EXPrint("[CORE][Expansion_SZCleanup] " + ToString() + " " + GetPosition() + " deleted");
				#endif
			}
			else
			{
				//! Failsafe - this should not be able to happen
				m_Expansion_SZCleanup = false;

				EXPrint("[CORE][Expansion_SZCleanup] " + ToString() + " " + GetPosition() + " unmarked for cleanup - object has parent");
			}
		}
	}

	void ExpansionCreateCleanup()
	{
		if (IsInherited(ExpansionTemporaryOwnedContainer))
			return;

		if (m_Expansion_SZCleanup)
			return;

		if (GetHierarchyParent())
			return;

		if (!ExpansionZoneModule.IsInsideSafeZone(GetPosition()))
			return;

		if (ExpansionStatic.IsAnyOf(this, GetExpansionSettings().GetSafeZone().ForceSZCleanup_ExcludedItems, true))
			return;

		m_Expansion_SZCleanup = true;

		//! Using SetLifetimeMax here prevents the lifetime resetting to the value from CE
		//! if this item can receive attachments/cargo and something is attached/put in cargo
		//! while the item is on ground
		SetLifetimeMax(GetExpansionSettings().GetSafeZone().ItemLifetimeInSafeZone);

		#ifdef EXPANSION_SAFEZONE_DEBUG
		EXPrint("[CORE][Expansion_SZCleanup] " + ToString() + " " + GetPosition() + " marked for cleanup - lifetime " + GetLifetime());
		#endif
	}

	bool Expansion_CanObjectAttach(Object obj)
	{
		if (obj.IsMan())
			return Expansion_CanPlayerAttach();

		return false;
	}

	bool Expansion_CanPlayerAttach()
	{
#ifdef EXPANSION_PLAYER_ATTACHMENT_CANATTACH_OVERRIDE
		m_Expansion_CanPlayerAttach = true;
#endif

		return m_Expansion_CanPlayerAttach;
	}

	bool Expansion_CarContactActivates()
	{
		if (IsInherited(CarWheel))
			return true;

		return false;
	}

	TStringArray Expansion_GetAttachmentSlots()
	{
		TStringArray attachments = {};
		ConfigGetTextArray("attachments", attachments);
		return attachments;
	}

	TStringArray Expansion_GetInventorySlots()
	{
		TStringArray inventorySlots = {};
		ConfigGetTextArray("inventorySlot", inventorySlots);
		return inventorySlots;
	}

	bool Expansion_HasAttachmentSlot(string slotName)
	{
		//! @note inventory is not available in constructor
		if (GetInventory())
		{
			int slotID = InventorySlots.GetSlotIdFromString(slotName);

			if (!InventorySlots.IsSlotIdValid(slotID))
				return false;

			if (!GetInventory().HasAttachmentSlot(slotID))
				return false;
		}
		else
		{
			TStringArray attachments = Expansion_GetAttachmentSlots();

			return ExpansionStatic.StringArrayContainsIgnoreCase(attachments, slotName);
		}

		return true;
	}

	bool Expansion_HasAttachmentSlot(TStringArray slotNames)
	{
		foreach (string slotName: slotNames)
		{
			if (Expansion_HasAttachmentSlot(slotName))
				return true;
		}

		return false;
	}

	bool Expansion_HasInventorySlot(string slotName)
	{
		//! @note inventory is not available in constructor
		if (GetInventory())
		{
			int slotID = InventorySlots.GetSlotIdFromString(slotName);

			if (!InventorySlots.IsSlotIdValid(slotID))
				return false;

			if (!GetInventory().HasInventorySlot(slotID))
				return false;
		}
		else
		{
			TStringArray inventorySlots = Expansion_GetInventorySlots();

			return ExpansionStatic.StringArrayContainsIgnoreCase(inventorySlots, slotName);
		}

		return true;
	}

	bool Expansion_HasInventorySlot(TStringArray slotNames)
	{
		foreach (string slotName: slotNames)
		{
			if (Expansion_HasInventorySlot(slotName))
				return true;
		}

		return false;
	}

	bool Expansion_IsAdminTool()
	{
		return m_Expansion_IsAdminTool;
	}
	
	bool Expansion_IsSelectionRuined(string selection)
	{
		string zone;
		DamageSystem.GetDamageZoneFromComponentName(this, selection, zone);
		return GetHealthLevel(zone) == GameConstants.STATE_RUINED;
	}

	int Expansion_GetItemSize()
	{
		vector itemSize = ConfigGetVector("itemSize");
		return itemSize[0] * itemSize[1];
	}

	bool Expansion_IsStackable()
	{
		return m_CanThisBeSplit && !m_HasQuantityBar;
	}

	//! @brief Attempt to set stack amount if item is stackable. Return true if operation was performed, false if not.
	//! Giving a negative amount is allowed (treated the same as an amount of zero) if deleteIfZero = true.
	//! @note Valid to call on non-stackable items only if amount = 1 (no-op) or amount <= 0 and deleteIfZero = true
	bool Expansion_SetStackAmount(int amount, bool deleteIfZero = false)
	{
		if (amount <= 0 && deleteIfZero)
		{
			Delete();
			return true;
		}

		if (!Expansion_IsStackable())
		{
			if (amount != 1)
				Error(ToString() + " is not a stackable item, cannot set stack amount to " + amount + "!");
			return false;
		}

		if (amount < 0)
		{
			Error(ToString() + " cannot set stack amount to negative!");
			return false;
		}

		if (IsAmmoPile())
		{
			auto mag = Magazine.Cast(this);
			int ammoMax = mag.GetAmmoMax();
			if (amount > ammoMax)
				amount = ammoMax;
			if (GetGame().IsServer())
				mag.ServerSetAmmoCount(amount);
			else
				mag.LocalSetAmmoCount(amount);
		}
		else
		{
			//! No max check needed since it's handled by SetQuantity
			SetQuantity(amount);
		}

		return true;
	}

	//! @brief Attempt to decrease stack by amount. Return true if operation was performed, false if not.
	//! @note Changes amountToDelete in-place.
	bool Expansion_DecreaseStackAmount(inout int amountToDelete, bool deleteIfZero = false)
	{
		int amount = Expansion_GetStackAmount();
		if (!Expansion_SetStackAmount(amount - amountToDelete, deleteIfZero))
			return false;
		if (amount >= amountToDelete)
			amountToDelete = 0;
		else
			amountToDelete -= amount;
		return true;
	}

	//! @brief Get stack amount of item.
	//! @note Valid to call on non-stackable items (will return 1)
	int Expansion_GetStackAmount()
	{
		if (IsAmmoPile())
		{
			auto mag = Magazine.Cast(this);
			return mag.GetAmmoCount();
		}
		else if (Expansion_IsStackable())
		{
			return GetQuantity();
		}

		return 1;
	}

	float Expansion_GetQuantity(inout int quantityType)
	{
		float item_quantity = GetQuantity();
		int max_quantity = GetQuantityMax();

		float quantity_ratio;
		float quantity_output;

		if (max_quantity > 0) // Some items, like books, have max_quantity set to 0 => division by ZERO error in quantity_ratio
		{
			quantity_ratio = Math.Round((item_quantity / max_quantity) * 100);
			if (ConfigGetString("stackedUnit") == "pc.")
			{
				quantity_output = item_quantity;
				quantityType = ExpansionItemQuantityType.PC;
			}
			else if (ConfigGetString("stackedUnit") == "percentage")
			{
				quantity_output = quantity_ratio;
				quantityType = ExpansionItemQuantityType.PERCENTAGE;
			}
			else if (ConfigGetString("stackedUnit") == "g")
			{
				quantity_output = quantity_ratio;
				quantityType = ExpansionItemQuantityType.GRAM;
			}
			else if (ConfigGetString("stackedUnit") == "ml")
			{
				quantity_output = quantity_ratio;
				quantityType = ExpansionItemQuantityType.MILLILITER;
			}
			else if (ConfigGetString("stackedUnit") == "w" || HasEnergyManager())
			{
				quantity_output = quantity_ratio;
				quantityType = ExpansionItemQuantityType.POWER;
			}
			else if (IsInherited(Magazine))
			{
				Magazine magazine_item;
				Class.CastTo(magazine_item, this);
				quantity_output = magazine_item.GetAmmoCount();
				quantityType = ExpansionItemQuantityType.MAGAZINE;
			}
			else
			{
				quantity_output = 0;
			}
		}
		else
		{
			if (IsInherited(ClothingBase))
			{
				float heatIsolation = MiscGameplayFunctions.GetCurrentItemHeatIsolation(this);
				quantity_output = heatIsolation;
				quantityType = ExpansionItemQuantityType.HEATISOLATION;
			}
		}

		return quantity_output;
	}
	
	//! @brief if item is 9V battery or has one attached, return battery energy in range [0, 100], else 0
	//! @note this uses quantity so works on client as well since Battery9V has convertEnergyToQuantity=1 (quantity, unlike energy, is netsynced by the game)
	int Expansion_GetBatteryEnergy()
	{
		Battery9V battery;
		if (Class.CastTo(battery, this) || Class.CastTo(battery, FindAttachmentBySlotName("BatteryD")))
			return Math.Ceil(battery.GetQuantity() / battery.GetQuantityMax() * 100);

		return 0;
	}

	float Expansion_GetDPS()
	{
		if (m_Expansion_IsMeleeWeapon)
		{
			string ammoType;
			if (GetGame().ConfigGetText(CFG_VEHICLESPATH + " " + GetType() + " MeleeModes Default ammo", ammoType))
				return GetGame().ConfigGetFloat(CFG_AMMO + " " + ammoType + " DamageApplied Health damage");
		}

		return 0.0;
	}

	/**
	 * @brief compare DPS (dmg per second) of weapons.
	 * 
	 * @return -1 if this weapon's DPS is lower than the other's, 1 if higher, 0 if equal
	 */
	int Expansion_CompareDPS(ItemBase weapon)
	{
		float dpsA = Expansion_GetDPS();
		float dpsB = weapon.Expansion_GetDPS();

		if (dpsA < dpsB)
			return -1;

		if (dpsA > dpsB)
			return 1;

		return 0;
	}

	void Expansion_DropAllCargo(float lifetime = -1)
	{
		array<EntityAI> items = MiscGameplayFunctions.Expansion_GetCargoItems(this);

		float angle = Math.Acos(Math.RandomFloat(-1.0, 1.0));	

		float cos = Math.Cos(angle);
		float sin = Math.Sin(angle);

		vector halfExtents = "0.5 0 0.5";
		foreach (EntityAI item: items)
		{
			GetInventory().DropEntityInBounds(InventoryMode.SERVER, this, item, halfExtents, angle, cos, sin);
			if (lifetime > -1)
				item.SetLifetime(lifetime);
		}
	}

	void Expansion_QueueEntityActions(int actions)
	{
		if ((m_Expansion_QueuedActions & actions) == actions)  //! Already queued
			return;

		ExpansionItemBaseModule.s_Instance.QueueEntityActions(this, actions);

		m_Expansion_QueuedActions |= actions;
	}

	void Expansion_DequeueEntityActions(int actions)
	{
		if ((m_Expansion_QueuedActions & actions) == 0)  //! Not queued
			return;

		ExpansionItemBaseModule.s_Instance.QueueEntityActions(this, -actions);

		m_Expansion_QueuedActions &= ~actions;
	}

	void Expansion_PlaceOnSurfaceProper(vector position)
	{
	#ifdef EXTRACE_DIAG
		auto trace = EXTrace.Start(EXTrace.GENERAL_ITEMS, this, position.ToString());
	#endif

		vector pos = GetPosition();
		vector ori = GetOrientation();

		//! -------------------------------------------------------

		//! See hologram.c, AlignProjectionOnTerrain

		vector yaw = Vector(ori[0], 0.0, 0.0);
		vector mat0[3];
		vector mat1[3];
		vector mat2[3];
		vector normal = GetGame().SurfaceGetNormal(position[0], position[2]);

		vector angles = normal.VectorToAngles();
		angles[1] = angles[1] + 270;

		angles[0] = Math.Clamp(angles[0], 0, 360); 
		angles[1] = Math.Clamp(angles[1], 0, 360);
		angles[2] = Math.Clamp(angles[2], 0, 360);

		yaw[0] = yaw[0] + (360 - angles[0]);

		Math3D.YawPitchRollMatrix(yaw, mat0);
		Math3D.YawPitchRollMatrix(angles, mat1);
		Math3D.MatrixMultiply3(mat1, mat0, mat2);

		//! -------------------------------------------------------

		vector m4[4];
		m4[0] = mat2[0];
		m4[1] = mat2[1];
		m4[2] = mat2[2];
		m4[3] = position;

		PlaceOnSurfaceRotated(m4, m4[3]);

		//! -------------------------------------------------------

	/*
		vector minMax[2];
		float offsetY;
		if (GetCollisionBox(minMax))
		{
			vector p1 = minMax[0].Multiply4(m4);
			vector p2 = minMax[1].Multiply4(m4);
			vector center = vector.Zero.Multiply4(m4);
			if (p2[1] < center[1])
				offsetY = center[1] - p2[1];
			else
				offsetY = center[1] - p1[1];
		}

	#ifdef DIAG_DEVELOPER
		EXTrace.Print(EXTrace.GENERAL_ITEMS, this, "" + pos + " " + ori + " -> " + m4[3] + " + <0 " + offsetY + " 0> " + Math3D.MatrixToAngles(m4));
	#endif

		m4[3][1] = m4[3][1] + offsetY;
	*/

	#ifdef DIAG_DEVELOPER
		EXTrace.Print(EXTrace.GENERAL_ITEMS, this, "" + pos + " " + ori + " -> " + m4[3] + " " + Math3D.MatrixToAngles(m4));
	#endif
		
		SetTransform(m4);
	}

	//! Process wetness/temperature/decay like vanilla does
	void Expansion_ProcessWTD(float elapsed)
	{
		bool hasParent = false, hasRootAsPlayer = false;
		ItemBase refParentIB;
		
		bool wwtu = g_Game.IsWorldWetTempUpdateEnabled();
		bool foodDecay = g_Game.IsFoodDecayEnabled();
		
		if ( wwtu || foodDecay )
		{
			bool processWetness;
			//! https://feedback.bistudio.com/T173348
			if (wwtu && CanHaveWetness())
				processWetness = true;
			bool processTemperature;
			if (wwtu && CanHaveTemperature())
				processTemperature = true;
			bool processDecay;
			if (foodDecay && CanDecay() && CanProcessDecay())
				processDecay = true;
			
			if ( processWetness || processTemperature || processDecay)
			{
				HierarchyCheck( hasParent, hasRootAsPlayer, refParentIB );
			
				if ( processWetness )
					ProcessItemWetness( elapsed, hasParent, hasRootAsPlayer, refParentIB );
				
				if ( processTemperature )
					ProcessItemTemperature( elapsed, hasParent, hasRootAsPlayer, refParentIB );
			
				if ( processDecay )
					ProcessDecay( elapsed, hasRootAsPlayer );	
			}
		}
	}

	string Expansion_GetEntityStorageFileName()
	{
		return string.Empty;
	}

	bool Expansion_CanBeUsedToBandage()
	{
		return IsInherited(Rag) || IsInherited(BandageDressing) || IsInherited(Bandana_ColorBase);
	}

	bool Expansion_CanUseVirtualStorage(bool restoreOverride = false)
	{
		return false;
	}

	bool Expansion_StoreContents();

	bool Expansion_RestoreContents(PlayerBase player = null);
	
	void Expansion_SwitchItemSelectionTextureEx(EItemManipulationContext context, Param par = null)
	{
	}
};
