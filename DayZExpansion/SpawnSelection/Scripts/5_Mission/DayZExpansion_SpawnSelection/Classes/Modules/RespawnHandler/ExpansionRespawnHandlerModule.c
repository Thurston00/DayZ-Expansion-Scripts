/**
 * ExpansionRespawnHandlerModule.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2024 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

class ExpansionLastPlayerSpawnLocation
{
	string Key;
	bool IsTerritory;
	
	void ExpansionLastPlayerSpawnLocation(string key, bool isTerritory = false)
	{
		Key = key;
		IsTerritory = isTerritory;
	}

	void WriteTo(ParamsWriteContext ctx)
	{
		ctx.Write(Key);
		ctx.Write(IsTerritory);
	}

	bool ReadFrom(ParamsReadContext ctx)
	{
		if (!ctx.Read(Key))
			return false;

		if (!ctx.Read(IsTerritory))
			return false;

		return true;
	}
}

[CF_RegisterModule(ExpansionRespawnHandlerModule)]
class ExpansionRespawnHandlerModule: CF_ModuleWorld
{
	protected string s_Folder;
	protected string s_CooldownsFolder;
	protected string s_FileName;
	ref map<string, ref ExpansionPlayerState> m_PlayerStartStates;
	bool m_SpawnSelected;
	ref map<string, ref ExpansionRespawnDelayTimers> m_PlayerRespawnDelays;
	ref map<string, ref ExpansionLastPlayerSpawnLocation> m_PlayerLastSpawnLocation;

	void ExpansionRespawnHandlerModule()
	{
		m_PlayerStartStates = new map<string, ref ExpansionPlayerState>;
		m_PlayerRespawnDelays = new map<string, ref ExpansionRespawnDelayTimers>;
		m_PlayerLastSpawnLocation = new map<string, ref ExpansionLastPlayerSpawnLocation>;
	}
	
	override void OnInit()
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(ExpansionTracing.RESPAWN, this);
#endif

		super.OnInit();
		
		EnableClientNew();
		EnableClientReady();
		EnableInvokeConnect();
		EnableMissionFinish();
		EnableMissionStart();
		Expansion_EnableRPCManager();

		Expansion_RegisterClientRPC("RPC_ShowSpawnMenu");
		Expansion_RegisterServerRPC("RPC_SelectSpawn");
		Expansion_RegisterClientRPC("RPC_CloseSpawnMenu");
		Expansion_RegisterClientRPC("RPC_CheckPlayerCooldowns");
		Expansion_RegisterClientRPC("RPC_Callback");
	}
	
	// ------------------------------------------------------------
	// Called on server
	// ------------------------------------------------------------
	void StartSpawnSelection(PlayerBase player, PlayerIdentity identity)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(ExpansionTracing.RESPAWN, this);
#endif
	
		if (!IsMissionHost())
			return;
		
		if (!player)
		{
			Error(ToString() + "::StartSpawnSelection - player is NULL!");
			return;
		}
		
		if (!identity)
		{
			Error(ToString() + "::StartSpawnSelection - player identity is NULL!");
			return;
		}

		string uid = identity.GetId();
		array<ref ExpansionSpawnLocation> territoryspawnlist = new array<ref ExpansionSpawnLocation>;
		if (GetExpansionSettings().GetSpawn().SpawnOnTerritory)
			territoryspawnlist = GetTerritoryList(identity.GetId());
		
		//! This is NOT an error. When only territory spawn is enabled, the player may simply not have a territory.
		if (!GetExpansionSettings().GetSpawn().SpawnLocations.Count() && (!territoryspawnlist || !territoryspawnlist.Count()))
			return;

		player.m_Expansion_SpawnSelect = true;

		//! Store state (to be restored once spawn selection ends)
		if (!m_PlayerStartStates.Contains(uid))
		{
			m_PlayerStartStates.Insert(uid, new ExpansionPlayerState(player));
			Save();
		}

		auto rpc = Expansion_CreateRPC("RPC_ShowSpawnMenu");

		auto spawnlist = GetExpansionSettings().GetSpawn().SpawnLocations;

		rpc.Write(spawnlist.Count());
		foreach (ExpansionSpawnLocation normalSpawn: spawnlist)
		{
			normalSpawn.OnSend(rpc);
		}
		
		rpc.Write(territoryspawnlist.Count());

		//! @note: We send the territory list with a ParamsWriteContext method as we send unserialised data also that would be skiped otherwise by the RPC.
		foreach (ExpansionSpawnLocation territorySpawn: territoryspawnlist)
		{
			territorySpawn.OnSend(rpc);
		}

		rpc.Expansion_Send(true, identity);
	}

	void CheckResumeSpawnSelection(PlayerBase player, PlayerIdentity identity)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(ExpansionTracing.RESPAWN, this);
#endif

		if (!player)
			return;
		
		string uid = identity.GetId();

		//! If we have a player state, it means an earlier spawn select wasn't completed by the player
		ExpansionPlayerState state = m_PlayerStartStates.Get(uid);
		if (state)
		{
			StartSpawnSelection(player, identity);
		}
		else
		{
			//! Related: 0_Preload\3_Game\Analytics\AnalyticsManagerServer.c
			player.StatRegister(AnalyticsManagerServer.STAT_DISTANCE);
			player.StatRegister(AnalyticsManagerServer.STAT_PLAYTIME);
			//player.StatSyncToClient();
		}
	}

	// ------------------------------------------------------------
	// Called on client
	// ------------------------------------------------------------
	private void RPC_ShowSpawnMenu(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(ExpansionTracing.RESPAWN, this);
#endif

		int normalCount;
		if (!ctx.Read(normalCount))
			Error(ToString() + "::RPC_ShowSpawnMenu - Could not read territoryCount");

		ExpansionSpawnLocation loc;

		array<ref ExpansionSpawnLocation> spawnlist = {};
		while (normalCount--)
		{
			loc = new ExpansionSpawnLocation;
			loc.OnReceive(ctx);
			spawnlist.Insert(loc);
		}

		int territoryCount;
		if (!ctx.Read(territoryCount))
			Error(ToString() + "::RPC_ShowSpawnMenu - Could not read territoryCount");

		//! @note: We get the territory list with a ParamsReadContext method as we recive unserialised data also that would be skiped otherwise by the RPC.
		array<ref ExpansionSpawnLocation> territoryspawnlist = {};
		while (territoryCount--)
		{
			loc = new ExpansionSpawnLocation;
			loc.OnReceive(ctx);
			territoryspawnlist.Insert(loc);
		}
		
		//! Abort if both lists not set/empty
		if ((!spawnlist || !spawnlist.Count()) && (!territoryspawnlist || !territoryspawnlist.Count()))
		{
			Error(ToString() + "::RPC_ShowSpawnMenu - No spawn locations!");
			return;
		}
		
		//! Call this periodically via call queue (will check if game is ready to show menu, then clean itself up)
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(Exec_ShowSpawnMenu, 1, true, spawnlist, territoryspawnlist);
	}
	
	// ------------------------------------------------------------
	// Called on client
	// ------------------------------------------------------------
	private void Exec_ShowSpawnMenu(array<ref ExpansionSpawnLocation> spawnlist, array<ref ExpansionSpawnLocation> territoryspawnlist)
	{
		//! Return if game is not yet ready to show spawn menu
		if (GetDayZGame().IsLoading())
			return;

		if (GetGame().GetUIManager().GetMenu())
			return;

		if (GetDayZExpansion().GetExpansionUIManager().GetMenu())
			return;

		if (GetDayZGame().GetMissionState() != DayZGame.MISSION_STATE_GAME)
			return;

		if (!GetGame().GetMission().GetHud())
			return;

	#ifdef EXPANSIONMONITORMODULE
		if (!GetExpansionSettings().GetSpawn(false).IsLoaded())
			return;
	#endif

		//! Game is ready to show menu
#ifdef EXTRACE
		auto trace = EXTrace.Start(ExpansionTracing.RESPAWN, this);
#endif

		ExpansionSpawnSelectionMenu spawnSelectionMenu = ExpansionSpawnSelectionMenu.Cast(GetDayZExpansion().GetExpansionUIManager().CreateSVMenu("ExpansionSpawnSelectionMenu"));
		//! In case spawn select menu could not be created, player will stay at original position and spawn select won't show
		if (!spawnSelectionMenu || !spawnSelectionMenu.IsVisible())
		{
			Error(ToString() + "::Exec_ShowSpawnMenu - Spawn selection menu could not be created or shown!");
			return;
		}
		
		if (spawnlist)
			spawnSelectionMenu.FillList(spawnlist, ExpansionSpawnSelectionLocationType.NORMAL);

		if (territoryspawnlist)
			spawnSelectionMenu.FillList(territoryspawnlist, ExpansionSpawnSelectionLocationType.TERRITORY);
		
		RequestPlacePlayerAtTempSafePosition();
	}

	// ------------------------------------------------------------
	// Called on server
	// ------------------------------------------------------------	
	array<ref ExpansionSpawnLocation> GetTerritoryList(string playerUID)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(ExpansionTracing.RESPAWN, this);
#endif

		array<ref ExpansionSpawnLocation> spawnLocations = new array<ref ExpansionSpawnLocation>;
		if (!IsMissionHost())
			return spawnLocations;
			
	#ifdef EXPANSIONMODBASEBUILDING
		ExpansionTerritoryModule territories_module = ExpansionTerritoryModule.Cast(CF_ModuleCoreManager.Get(ExpansionTerritoryModule));
		if (!territories_module)
			return spawnLocations;

		array<vector> positions = new array<vector>;
		ExpansionSpawnLocation location;
	
		map<int, TerritoryFlag> territoryFlags = territories_module.GetAllTerritoryFlags();
		foreach (int id, TerritoryFlag currentFlag: territoryFlags)
		{
			ExpansionTerritory territory = currentFlag.GetTerritory();
			if (!territory)
				continue;

			if (!territory.IsMember(playerUID))
				continue;
			
			vector pos = territory.GetPosition();
			// Offset player slighly horizontally and vertically so we don't spawn them on top of the flag pole
			pos = Vector(pos[0] + 1.0, pos[1], pos[2] + 1.0);

			positions.Insert(pos);
			location = new ExpansionSpawnLocation;
			location.SetLocation(territory.GetTerritoryName(), positions, id);
			if (GetExpansionSettings().GetSpawn().EnableRespawnCooldowns)
				location.SetUseCooldown(true);
			
			spawnLocations.Insert(location);
			positions.Clear();
		}
	#endif
		
		return spawnLocations;
	}
	
	// ------------------------------------------------------------
	// Called from Client
	// ------------------------------------------------------------	
	void RequestPlacePlayerAtTempSafePosition()
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(ExpansionTracing.RESPAWN, this);
#endif

		//! FIXME (or not): Moving the player causes desync. Just skip it for now.
		//! We have the teleport sync juncture, maybe we can use that here?!
		return;

		auto rpc = Expansion_CreateRPC("RPC_RequestPlacePlayerAtTempSafePosition");
		rpc.Expansion_Send(true);
	}
	
	// ------------------------------------------------------------
	// Called from client
	// ------------------------------------------------------------
	void SelectSpawn(int index, int spawnPointIndex, string locKey, bool isTerritory, bool useCooldown)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(ExpansionTracing.RESPAWN, this, index.ToString(), spawnPointIndex.ToString(), locKey, isTerritory.ToString(), useCooldown.ToString());
#endif

		m_SpawnSelected = true;
		if (ProcessCooldown(GetGame().GetPlayer().GetIdentity(), locKey, isTerritory, useCooldown, false))
			return;

		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(Exec_ShowSpawnMenu);

		auto rpc = Expansion_CreateRPC("RPC_SelectSpawn");
		rpc.Write(index);
		rpc.Write(spawnPointIndex);
		rpc.Write(isTerritory);
		rpc.Expansion_Send(true);
	}
	
	//! Check existing cooldowns and add if none present.
	//! Return true if point index already had a cooldown (that hasn't expired), false otherwise.
	bool ProcessCooldown(PlayerIdentity sender, string locKey, bool isTerritory, bool useCooldown, bool addCooldown = true)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(ExpansionTracing.RESPAWN, this, locKey, isTerritory.ToString(), useCooldown.ToString(), addCooldown.ToString());
#endif

		bool hasCooldown = false;
		if (GetExpansionSettings().GetSpawn().EnableRespawnCooldowns && useCooldown)
		{
			string cooldownIndex;
			int respawnCooldown = GetExpansionSettings().GetSpawn().GetCooldown(isTerritory);
			string playerUID = sender.GetId();
			ExpansionRespawnDelayTimers playerCooldowns = m_PlayerRespawnDelays[playerUID];
			if (playerCooldowns)
			{					
				ExpansionRespawnDelayTimer timer = playerCooldowns[locKey];
				if (timer)
				{
					int cooldownTime = timer.GetTimeDiff();
					int cooldown = respawnCooldown;
					if (GetExpansionSettings().GetSpawn().PunishMultispawn)
						cooldown += timer.GetPunishment();
					if (cooldownTime < cooldown)
						hasCooldown = true;
				}
			}

			if (!hasCooldown)
			{
				if (addCooldown)
					AddCooldown(playerUID, locKey, isTerritory);
			}
			else
			{
				ExpansionNotification(new StringLocaliser("STR_EXPANSION_SPAWNSELECTION_POINT_LOCKED"), new StringLocaliser("STR_EXPANSION_SPAWNSELECTION_POINT_LOCKED_DESC"), EXPANSION_NOTIFICATION_ICON_INFO, COLOR_EXPANSION_NOTIFICATION_ERROR).Create(sender);
			}
		}

		return hasCooldown;
	}

	// ------------------------------------------------------------
	// Called on server
	// ------------------------------------------------------------
	private void RPC_SelectSpawn(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(ExpansionTracing.RESPAWN, this);
#endif

		int pointIndex;
		if (!ctx.Read(pointIndex))
			Error(ToString() + "::RPC_SelectSpawn - ERROR: Could not read spawn point index!");
		
		int spawnPointIndex;
		if (!ctx.Read(spawnPointIndex)) //! @note failing to read spawn point should still call exec so proper cleanup is done
			Error(ToString() + "::RPC_SelectSpawn - ERROR: Could not read spawn point position!");

		bool isTerritory;
		if (!ctx.Read(isTerritory))
			Error(ToString() + "::RPC_SelectSpawn - ERROR: Could not read spawn point isTerritory bolean!");

		Exec_SelectSpawn(sender, pointIndex, spawnPointIndex, isTerritory);
	}
	
	// ------------------------------------------------------------
	// Called on server
	// ------------------------------------------------------------
	private void Exec_SelectSpawn(PlayerIdentity sender, int pointIndex, int spawnPointIndex, bool isTerritory)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(ExpansionTracing.RESPAWN, this, pointIndex.ToString(), spawnPointIndex.ToString());
#endif

		string playerUID = sender.GetId();
		PlayerBase player = PlayerBase.GetPlayerByUID(playerUID);
		if (!player)
		{
			Error(ToString() + "::Exec_SelectSpawn - ERROR: player with UID " + playerUID + " dos not exist");
			return;
		}
		
		ExpansionPlayerState state = m_PlayerStartStates.Get(playerUID);
		if (!state)
		{
			Error(ToString() + "::Exec_SelectSpawn - ERROR: Could not get player state for player with UID " + playerUID);
			return;
		}

		array<ref ExpansionSpawnLocation> locations = GetExpansionSettings().GetSpawn().SpawnLocations;
		array<ref ExpansionSpawnLocation> territories = GetTerritoryList(playerUID);

		if (pointIndex < 0 || pointIndex >= locations.Count() + territories.Count())
		{
			Error(ToString() + "::Exec_SelectSpawn - ERROR: Invalid location index");
			return;  //! Invalid location index
		}

		ExpansionSpawnLocation loc;
		if (!isTerritory)
			loc = locations[pointIndex];
		else
			loc = territories[pointIndex - locations.Count()];

		if (spawnPointIndex < 0 || spawnPointIndex >= loc.Positions.Count())
		{
			Error(ToString() + "::Exec_SelectSpawn - ERROR: Location doesn't have spawn points");
			return;
		}

		string locationKey = loc.GetKey();
		
		if (ProcessCooldown(sender, locationKey, isTerritory, loc.UseCooldown))
		{
			Callback();
			return;
		}

		vector spawnPoint = loc.Positions[spawnPointIndex];

		//! If spawn point is still zero vector, use original spawn position
		//! (this is not necessarily an error, it is valid to include the zero vector among spawn positions in the configuration)
		if (spawnPoint == vector.Zero)
			spawnPoint = state.m_Position;

		if (spawnPoint[1] == 0)  //! If Y is zero, use surface Y instead
			spawnPoint[1] = GetGame().SurfaceY(spawnPoint[0], spawnPoint[2]);

		DayZPlayerSyncJunctures.ExpansionTeleport(player, spawnPoint);
	}
	
	// ------------------------------------------------------------
	// Called on server
	// ------------------------------------------------------------
	private void Callback()
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(ExpansionTracing.RESPAWN, this);
#endif
		
		auto rpc = Expansion_CreateRPC("RPC_Callback");
		rpc.Expansion_Send(true);
	}
	
	// ------------------------------------------------------------
	// Called on client
	// ------------------------------------------------------------
	private void RPC_Callback(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(ExpansionTracing.RESPAWN, this);
#endif
		
		ExpansionSpawnSelectionMenu spawnSelectionMenu = ExpansionSpawnSelectionMenu.Cast(GetDayZExpansion().GetExpansionUIManager().GetMenu());
		if (!spawnSelectionMenu || !spawnSelectionMenu.IsVisible())
			return;
		
		spawnSelectionMenu.ClearSpawnPoint();
	}
	
	// ------------------------------------------------------------
	// Called on server
	// ------------------------------------------------------------
	void EndSpawnSelection(PlayerBase player)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(ExpansionTracing.RESPAWN, this);
#endif

		if (!IsMissionHost())
			return;

		PlayerIdentity identity = player.GetIdentity();

		if (!identity)
			return; // can be NULL if the player rage quit

		string uid = identity.GetId();
		auto state = m_PlayerStartStates.Get(uid);

		//! Make sure clothes and contained items are dry after using spawn select in case we were in the water
		ResetItemWetness(player);

		//! Restore state
		state.ApplyTo(player);
		m_PlayerStartStates.Remove(uid);

		Save();

		//! Remove any sickness player may have gained while in the cold deep
		player.RemoveAllAgents();

		player.m_PlayerOldPos = player.GetPosition();
		player.m_Expansion_SpawnSelect = false;

		//! Related: 0_Preload\3_Game\Analytics\AnalyticsManagerServer.c
		player.StatRegister(AnalyticsManagerServer.STAT_DISTANCE);
		player.StatRegister(AnalyticsManagerServer.STAT_PLAYTIME);
		//player.StatSyncToClient();
		
		auto rpc = Expansion_CreateRPC("RPC_CloseSpawnMenu");
		rpc.Expansion_Send(true, identity);

		if (GetExpansionSettings().GetLog().SpawnSelection)
			GetExpansionSettings().GetLog().PrintLog("[SpawnSelection] Player \"" + identity.GetName() + "\" (id=" + uid + ")" + " spawned at " + player.GetPosition());

		auto container = player.Expansion_GetTemporaryOwnedContainer(false);
		if (container)
			container.Expansion_PlaceOnSurfaceProper(player.GetPosition());
	}
	
	void ResetItemWetness(EntityAI parent)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(ExpansionTracing.RESPAWN, this);
#endif

		array<EntityAI> items = new array<EntityAI>;
		items.Reserve(parent.GetInventory().CountInventory());

		parent.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);
		ItemBase item;
		foreach (EntityAI entity: items)
		{
			if (Class.CastTo(item, entity))
				item.SetWet(0);
		}
	}

	// ------------------------------------------------------------
	// Called on client
	// ------------------------------------------------------------
	private void RPC_CloseSpawnMenu(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(ExpansionTracing.RESPAWN, this);
#endif
		
		//! Use CallLater to make menu disappear *after* player position has updated on client
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(Exec_CloseSpawnMenu, 1000, false);
	}
	
	// ------------------------------------------------------------
	// Called on client
	// ------------------------------------------------------------
	private void Exec_CloseSpawnMenu()
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(ExpansionTracing.RESPAWN, this);
#endif

		if (!IsMissionClient())
			return;
		
		ExpansionSpawnSelectionMenu spawnSelectionMenu = ExpansionSpawnSelectionMenu.Cast(GetDayZExpansion().GetExpansionUIManager().GetMenu());
		if (spawnSelectionMenu)
		{
			GetDayZExpansion().GetExpansionUIManager().CloseMenu();
		}
	}

	void SetExpansionStartingGear(PlayerBase player)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(ExpansionTracing.RESPAWN, this);
#endif

		if (!IsMissionHost())
			return;
				
		if (GetExpansionSettings().GetSpawn().StartingGear.EnableStartingGear)
		{
			EntityAI parent = player;  //! This changes dynamically when items don't fit
			ExpansionStartingGear gear;
			
			if (Class.CastTo(gear, GetExpansionSettings().GetSpawn().StartingGear))
			{
				//! Add items to clothing (if present) or player
				//! If items don't fit into inventory, they'll be placed in an ExpansionTemporaryOwnedContainer 

				//! Add items to top/shirt/jacket
				AddGear(player, gear.UpperGear, parent, "Body");
				
				//! Add items to pants
				AddGear(player, gear.PantsGear, parent, "Legs");
				
				//! Add items to backpack
				AddGear(player, gear.BackpackGear, parent, "Back");
				
				//! Add items to vest
				AddGear(player, gear.VestGear, parent, "Vest");
				
				EntityAI parentPlayer = player;  //! Always use player as parent for shoulder/melee slots

				//! Add primary weapon and its attachments
				if (gear.PrimaryWeapon)
					AddItem(player, gear.PrimaryWeapon, parentPlayer);
				
				//! Add secondary weapon and its attachments
				if (gear.SecondaryWeapon)
					AddItem(player, gear.SecondaryWeapon, parentPlayer);
			}
		}
	}

	private void AddGear(PlayerBase player, array< ref ExpansionStartingGearItem > gearItems, inout EntityAI parent, string slotName)
	{
		EntityAI item = player.FindAttachmentBySlotName(slotName);
		foreach (ExpansionStartingGearItem gearItem: gearItems)
		{
			if (item)
			{
				AddItem(player, gearItem, item);
				//! When gear doesn't fit into item, a ExpansionTemporaryOwnedContainer will be created
				//! and item reference will be changed in-place to this container. We need to update `out parent` in this case,
				//! So that other gear that may be added will be placed into that container as well if they don't fit into their
				//! respective player attachment slot item.
				if (item.IsInherited(ExpansionTemporaryOwnedContainer))
					parent = item;
			} else {
				AddItem(player, gearItem, parent);
			}
		}
	}

	private void AddItem(PlayerBase player, ExpansionStartingGearItem gearItem, inout EntityAI parent)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.RESPAWN, this, "" + player, gearItem.ClassName, "" + parent);
#endif

		if (!gearItem.ClassName)
			return;

		int remainingAmount = gearItem.Quantity;  //! SpawnOnParent will deduct spawned amount!
		EntityAI item = EntityAI.Cast(ExpansionItemSpawnHelper.SpawnOnParent(gearItem.ClassName, player, parent, remainingAmount, gearItem.Quantity, gearItem.Attachments, -1, true));

#ifdef EXTRACE
		EXTrace.Add(trace, "" + item);
#endif

		if (!item)
			return;

		if (GetExpansionSettings().GetSpawn().StartingGear.SetRandomHealth)
			SetRandomHealth(item);

		if (!GetExpansionSettings().GetSpawn().StartingGear.ApplyEnergySources)
			return;

		bool hasEnergySourceSlot;
		//hasEnergySourceSlot = item.GetInventory().HasInventorySlot(InventorySlots.GetSlotIdFromString("BatteryD"));  //! WHY THE FUCK does this not work?!?

		TStringArray attachmentSlotNames = new TStringArray;
		if (GetGame().ConfigIsExisting("CfgVehicles " + gearItem.ClassName + " attachments"))
			GetGame().ConfigGetTextArray("CfgVehicles " + gearItem.ClassName + " attachments", attachmentSlotNames);
		else if (GetGame().ConfigIsExisting("CfgWeapons " + gearItem.ClassName + " attachments"))
			GetGame().ConfigGetTextArray("CfgWeapons " + gearItem.ClassName + " attachments", attachmentSlotNames);

		foreach (string slotName: attachmentSlotNames)
		{
			slotName.ToLower();
			if (slotName == "batteryd")
			{
				hasEnergySourceSlot = true;
				break;
			}
		}

		if (hasEnergySourceSlot)
			item.GetInventory().CreateAttachment("Battery9V");  //! TODO: Should probably be able to deal with arbitrary power sources?
	}

	string GetRandomLoadout(ref array<ref ExpansionSpawnGearLoadouts> loadouts)
	{
		bool IsZero = false;
		while (!IsZero)
		{
			IsZero = true;
			foreach (ExpansionSpawnGearLoadouts selectedLoadout : loadouts)
			{
				if (selectedLoadout.Chance == -1)
					return loadouts.GetRandomElement().Loadout;
					
				if (selectedLoadout.Chance > 1)
				{
					if (GetExpansionSettings().GetLog().SpawnSelection)
						GetExpansionSettings().GetLog().PrintLog("[SpawnSelection] !!! ERROR !!! loadout ("+selectedLoadout.Loadout+") has a chance of "+selectedLoadout.Chance+" and shouldn't be greater than 1.0");

					continue;
				}

				if (selectedLoadout.Chance > Math.RandomFloat(0.0, 1.0))
					return selectedLoadout.Loadout;

				IsZero = (selectedLoadout.Chance <= 0 );
			}
		}
		
		if (GetExpansionSettings().GetLog().SpawnSelection)
			GetExpansionSettings().GetLog().PrintLog("[SpawnSelection] !!! ERROR !!! Couldn't fine a loadout with a chance greater than 0.0");

		return "";
	}

	void ExpansionEquipCharacter(PlayerBase player)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(ExpansionTracing.RESPAWN, this);
#endif

		if ( !IsMissionHost() )
			return;

		if ( GetExpansionSettings().GetSpawn().UseLoadouts )
		{
			string loadout;
			if ( player.IsMale() )
			{
				loadout = GetRandomLoadout(GetExpansionSettings().GetSpawn().MaleLoadouts);
			} else {
				loadout = GetRandomLoadout(GetExpansionSettings().GetSpawn().FemaleLoadouts);
			}

			ExpansionHumanLoadout.Apply(player, loadout, false);
			
			SetExpansionStartingGear(player); // allow players to still use the starting gear system if they want to
		} else {
			ExpansionStartingClothing startingClothing;
			if (Class.CastTo(startingClothing, GetExpansionSettings().GetSpawn().StartingClothing))
			{
				AddClothing(player, startingClothing.Headgear, InventorySlots.HEADGEAR);
				AddClothing(player, startingClothing.Glasses, InventorySlots.EYEWEAR);
				AddClothing(player, startingClothing.Masks, InventorySlots.MASK);
				AddClothing(player, startingClothing.Tops, InventorySlots.BODY);
				AddClothing(player, startingClothing.Vests, InventorySlots.VEST);
				AddClothing(player, startingClothing.Gloves, InventorySlots.GLOVES);
				AddClothing(player, startingClothing.Pants, InventorySlots.LEGS);
				AddClothing(player, startingClothing.Belts, InventorySlots.HIPS);
				AddClothing(player, startingClothing.Shoes, InventorySlots.FEET);
				AddClothing(player, startingClothing.Armbands, InventorySlots.ARMBAND);
				AddClothing(player, startingClothing.Backpacks, InventorySlots.BACK);
				
				SetExpansionStartingGear(player);
			}
		}		
	}
	
	private void AddClothing(PlayerBase player, TStringArray clothingItems, int slotId)
	{
		EntityAI clothing = player.GetInventory().CreateAttachmentEx(clothingItems.GetRandomElement(), slotId);
		if (clothing && GetExpansionSettings().GetSpawn().StartingClothing.SetRandomHealth)
			SetRandomHealth(clothing);
	}

	void SetRandomHealth(EntityAI itemEnt)
	{
		if (itemEnt)
		{
			int rndHlt = Math.RandomInt(55,100);
			itemEnt.SetHealth("","",rndHlt);
		}
	}

	override void OnMissionStart(Class sender, CF_EventArgs args)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(ExpansionTracing.RESPAWN, this);
#endif

		super.OnMissionStart(sender, args);

		if (!GetGame().IsDedicatedServer())
			return;

		string filename_old = EXPANSION_FOLDER + "playerstates.bin";

		int instance_id = GetGame().ServerConfigGetInt( "instanceId" );
		string folder = "$mission:storage_" + instance_id + "\\expansion\\";
		s_Folder = folder;
		s_FileName = folder + "spawnselect.bin";

		if (!FileExist(folder))
			MakeDirectory(folder);

		if (!FileExist(s_FileName) && FileExist(filename_old))
		{
			if (CopyFile(filename_old, s_FileName))
				DeleteFile(filename_old);
		}

		s_CooldownsFolder = folder + "cooldowns\\";

		if (!FileExist(s_CooldownsFolder))
			MakeDirectory(s_CooldownsFolder);

		//! Load all states of players that haven't finished spawn select
		Load();
	}

	override void OnMissionFinish(Class sender, CF_EventArgs args)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(ExpansionTracing.RESPAWN, this);
#endif

		super.OnMissionFinish(sender, args);

		if (!GetGame().IsDedicatedServer())
			return;

		//! Save all states of players that haven't finished spawn select
		Save();
	}

	void Save()
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(ExpansionTracing.RESPAWN, this);
#endif

		FileSerializer file = new FileSerializer;
		if (file.Open(s_FileName, FileMode.WRITE))
		{
			//! Can't do it the sane, elegant way, causes server crash when accessing m_PlayerStartStates. Have to do it the awkward, BS way
			//file.Write(m_PlayerStartStates);

			file.Write(m_PlayerStartStates.Count());

			foreach (string uid, ExpansionPlayerState state: m_PlayerStartStates)
			{
				file.Write(uid);
				file.Write(state);
			}

			file.Close();
		}
	}

	void SaveCooldowns(string playerUID)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(ExpansionTracing.RESPAWN, this);
#endif

		string cooldownsFile = s_CooldownsFolder + playerUID + ".bin";

		FileSerializer file = new FileSerializer;
		if (file.Open(cooldownsFile, FileMode.WRITE))
		{
			ExpansionRespawnDelayTimers playerCooldowns = m_PlayerRespawnDelays[playerUID];
			if (playerCooldowns)
			{
				playerCooldowns.RemoveExpiredCooldowns();

				file.Write(playerCooldowns.Count());
				foreach (string key, ExpansionRespawnDelayTimer playerTimer: playerCooldowns)
				{
					playerTimer.WriteTo(file);
				}
			}
			else
			{
				file.Write(0);
			}

			ExpansionLastPlayerSpawnLocation last = m_PlayerLastSpawnLocation[playerUID];
			if (last)
			{
				file.Write(true);
				last.WriteTo(file);
			}
			else
			{
				file.Write(false);
			}

			file.Close();
		}
	}

	void Load()
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(ExpansionTracing.RESPAWN, this);
#endif

		if (!FileExist(s_FileName))
			return;

		FileSerializer file = new FileSerializer;
		if (file.Open(s_FileName, FileMode.READ))
		{
			//! Can't do it the sane, elegant way, causes server crash when accessing m_PlayerStartStates. Have to do it the awkward, BS way
			//file.Read(m_PlayerStartStates);

			int count;
			file.Read(count);

			string uid;
			ExpansionPlayerState state;
			for (int i = 0; i < count; i++)
			{
				file.Read(uid);
				file.Read(state);
				if (uid && state)
					m_PlayerStartStates.Insert(uid, state);
			}

			file.Close();
		}
	}

	void AddCooldown(string playerUID, string key, bool isTerritory)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(ExpansionTracing.RESPAWN, this);
#endif
		
		bool hasCooldownEntry = false;
		//! Check if player has a exiting cooldown entry for this spawn point key
		ExpansionRespawnDelayTimers playerCooldowns = m_PlayerRespawnDelays[playerUID];
		if (playerCooldowns)
		{
			ExpansionRespawnDelayTimer timer = playerCooldowns[key];
			if (timer)
			{
				if (GetExpansionSettings().GetSpawn().PunishMultispawn)
				{
					//! Check if this is the same spawnpoint the player used before
					ExpansionLastPlayerSpawnLocation lastLocation = m_PlayerLastSpawnLocation.Get(playerUID);
					if (lastLocation && lastLocation.Key == key)
					{
						//! Check if the player used this spawn point in the timeframe set in PunishTimeframe spawn settings
						if (timer.GetTimeDiff() < GetExpansionSettings().GetSpawn().PunishTimeframe)
						{
							timer.AddPunishment(GetExpansionSettings().GetSpawn().PunishCooldown);
						}
						else
						{
							if (timer.GetPunishment() > 0)
								timer.SetPunishment(0);
						}
					}
				}	
				
				hasCooldownEntry = true;
				timer.SetTime(); //! Update timestamp for this hasCooldownEntry
			}
		}
		
		//! Add new cooldown entry
		if (!hasCooldownEntry)
		{
			if (!playerCooldowns)
			{
				playerCooldowns = new ExpansionRespawnDelayTimers;
				m_PlayerRespawnDelays.Insert(playerUID, playerCooldowns);
			}
			playerCooldowns.Insert(key, new ExpansionRespawnDelayTimer(key, isTerritory));
		}
		
		//! Save last used spawnpoint key
		ExpansionLastPlayerSpawnLocation last;
		if (m_PlayerLastSpawnLocation.Find(playerUID, last))
		{	
			last.Key = key;
			last.IsTerritory = isTerritory;
		}
		else
		{
			last = new ExpansionLastPlayerSpawnLocation(key, isTerritory);
			m_PlayerLastSpawnLocation.Insert(playerUID, last);
		}

		SaveCooldowns(playerUID);
	}
	
	// ------------------------------------------------------------
	// Called on server
	// ------------------------------------------------------------
	void RespawnCountdownCheck(string playerUID)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(ExpansionTracing.RESPAWN, this);
#endif

		if (!GetGame().IsServer() && !GetGame().IsMultiplayer())
			return;

		ExpansionRespawnDelayTimers playerCooldowns = m_PlayerRespawnDelays[playerUID];
		if (!playerCooldowns)
		{
			playerCooldowns = new ExpansionRespawnDelayTimers;
			m_PlayerRespawnDelays.Insert(playerUID, playerCooldowns);
		}

		ExpansionLastPlayerSpawnLocation last;

		if (!playerCooldowns.Count())
		{
			string cooldownsFile = s_CooldownsFolder + playerUID + ".bin";
			if (FileExist(cooldownsFile))
			{
				FileSerializer file = new FileSerializer;
				if (file.Open(cooldownsFile, FileMode.READ))
				{
					int count;
					file.Read(count);

					ExpansionRespawnDelayTimer playerTimer;
					for (int i = 0; i < count; i++)
					{
						playerTimer = new ExpansionRespawnDelayTimer("");
						if (playerTimer.ReadFrom(file))
						{
							if (!playerTimer.Key)
								EXError.Error(this, "Player " + playerUID + " cooldown has an empty key!");
							else if (playerTimer.HasCooldown())
								playerCooldowns.Insert(playerTimer.Key, playerTimer);
							else
								EXTrace.Print(EXTrace.SPAWNSELECTION, this, "Discarded expired cooldown " + playerTimer.Key);
						}
						else
						{
							Error("Couldn't read spawn delay at index " + i);
							file.Close();
							return;
						}
					}

					bool hasLastSpawnLoc;
					if (!file.Read(hasLastSpawnLoc))
					{
						Error("Couldn't read if last player spawn location is present");
						file.Close();
						return;
					}

					if (hasLastSpawnLoc)
					{
						last = new ExpansionLastPlayerSpawnLocation("");
						if (last.ReadFrom(file))
						{
							m_PlayerLastSpawnLocation[playerUID] = last;
						}
						else
						{
							Error("Couldn't read last player spawn location");
							file.Close();
							return;
						}
					}

					file.Close();
				}
			}
		}
		
		PlayerBase player = PlayerBase.GetPlayerByUID(playerUID);
		if (!player)
			return;
		
		auto rpc = Expansion_CreateRPC("RPC_CheckPlayerCooldowns");

		rpc.Write(playerCooldowns.Count());
		foreach (string key, ExpansionRespawnDelayTimer timer: playerCooldowns)
		{
			timer.Now = CF_Date.Now(true).GetTimestamp();
			timer.WriteTo(rpc);
		}

		last = m_PlayerLastSpawnLocation[playerUID];
		if (last)
		{
			rpc.Write(true);
			m_PlayerLastSpawnLocation[playerUID].WriteTo(rpc);
		}
		else
		{
			rpc.Write(false);
		}

		rpc.Expansion_Send(true, player.GetIdentity());
	}
	
	// ------------------------------------------------------------
	// Called on client
	// ------------------------------------------------------------
	private void RPC_CheckPlayerCooldowns(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(ExpansionTracing.RESPAWN, this);
#endif

		int cooldownsCount;
		if (!ctx.Read(cooldownsCount))
		{
			Error(ToString() + "::RPC_CheckPlayerCooldowns - Could not read player cooldowns count");
			return;
		}

		m_PlayerRespawnDelays.Clear();
		ExpansionRespawnDelayTimers timers = new ExpansionRespawnDelayTimers;
		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
		string playerUID = player.GetIdentityUID();
		m_PlayerRespawnDelays.Insert(playerUID, timers);
		int now = CF_Date.Now(true).GetTimestamp();

		while (cooldownsCount--)
		{
			auto timer = new ExpansionRespawnDelayTimer("");
			if (!timer.ReadFrom(ctx))
			{
				Error(ToString() + "::RPC_CheckPlayerCooldowns - Could not read player cooldown");
				return;
			}

			//! Correct for difference between client and server clock
			timer.Timestamp += now - timer.Now;

			if (!timer.Key)
				EXError.Error(this, "Player cooldown has an empty key!");
			else if (timers.Contains(timer.Key))
				EXError.Error(this, "Player cooldown with key \"" + timer.Key + "\" is already present!");
			else
				timers.Insert(timer.Key, timer);
		}

		bool hasLastSpawnLoc;
		if (!ctx.Read(hasLastSpawnLoc))
		{
			Error(ToString() + "::RPC_CheckPlayerCooldowns - Could not read if player last spawn loc is present");
			return;
		}
		
		if (hasLastSpawnLoc)
		{
			auto last = new ExpansionLastPlayerSpawnLocation("");
			if (!last.ReadFrom(ctx))
			{
				Error(ToString() + "::RPC_CheckPlayerCooldowns - Could not read player last spawn loc");
				return;
			}
			
			m_PlayerLastSpawnLocation[playerUID] = last;
		}
		else
		{
			m_PlayerLastSpawnLocation.Remove(playerUID);
		}
	}

	override void OnInvokeConnect(Class sender, CF_EventArgs args)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(ExpansionTracing.RESPAWN, this);
#endif

		super.OnInvokeConnect(sender, args);

		auto cArgs = CF_EventPlayerArgs.Cast(args);

		if (!cArgs.Player)
			return;
		
		if (!cArgs.Identity)
			return;
		
		if (GetExpansionSettings().GetSpawn().EnableRespawnCooldowns)
			RespawnCountdownCheck(cArgs.Identity.GetId());
	}

	override void OnClientNew(Class sender, CF_EventArgs args)
	{
		auto cArgs = CF_EventNewPlayerArgs.Cast(args);
		
		float spawnHealth = GetExpansionSettings().GetSpawn().SpawnHealthValue;
		if (spawnHealth > 0 && spawnHealth <= cArgs.Player.GetMaxHealth("GlobalHealth", "Health"))
			cArgs.Player.SetHealth("", "", spawnHealth);
		
		if (GetExpansionSettings().GetSpawn().SpawnEnergyValue <= cArgs.Player.GetStatEnergy().GetMax())
			cArgs.Player.GetStatEnergy().Set(GetExpansionSettings().GetSpawn().SpawnEnergyValue);
		
		if (GetExpansionSettings().GetSpawn().SpawnWaterValue <= cArgs.Player.GetStatWater().GetMax())
			cArgs.Player.GetStatWater().Set(GetExpansionSettings().GetSpawn().SpawnWaterValue);
		
		if (GetExpansionSettings().GetSpawn().EnableSpawnSelection)
			StartSpawnSelection(cArgs.Player, cArgs.Identity);
	}

	override void OnClientReady(Class sender, CF_EventArgs args)
	{
		auto cArgs = CF_EventPlayerArgs.Cast(args);

		if (GetExpansionSettings().GetSpawn().EnableSpawnSelection)
			CheckResumeSpawnSelection(cArgs.Player, cArgs.Identity);
	}
}
