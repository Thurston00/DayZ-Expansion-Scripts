/**
 * ExpansionPlayerBase.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

class ExpansionInventoryItemType
{
	ref array<ItemBase> Items = new array<ItemBase>;
	int WorkingCount;

	int GetStackAmount()
	{
		int amount;

		foreach (ItemBase item: Items)
		{
			amount += item.Expansion_GetStackAmount();
		}

		return amount;
	}
}

modded class PlayerBase
{
	static ref ScriptInvoker s_Expansion_SI_OnPlayerConnected = new ScriptInvoker;

	protected string m_PlayerUID;
	protected string m_PlayerSteam;
	protected string m_PlayerName;

	private static autoptr map< string, PlayerBase > s_Expansion_AllPlayersUID = new map< string, PlayerBase >;
	private static autoptr map< string, string > s_Expansion_AllPlayersUID2PlainID = new map< string, string >;

	static ref CF_DoublyLinkedNodes_WeakRef<PlayerBase> s_Expansion_AllPlayers = new CF_DoublyLinkedNodes_WeakRef<PlayerBase>();

	ref CF_DoublyLinkedNode_WeakRef<PlayerBase> m_Expansion_Node;
	
	PlayerBase m_Expansion_NextPlayer;
	PlayerBase m_Expansion_PrevPlayer;

	protected bool m_Expansion_LeavingSafeZone;

	static ref map<string, bool> s_Expansion_RegisteredInventoryItemTypes = new map<string, bool>;
	static ref set<typename> s_Expansion_RegisteredInventoryItemTypenames = new set<typename>;
	ref map<string, ref ExpansionInventoryItemType> m_Expansion_InventoryItemTypes = new map<string, ref ExpansionInventoryItemType>;

	ExpansionTemporaryOwnedContainer m_Expansion_TemporaryOwnedContainer;

	ref ExpansionEntity m_Expansion_Entity;

	EntityAI m_Expansion_EntityToBeTakenToHands;

	void PlayerBase()
	{
		m_Expansion_Node = s_Expansion_AllPlayers.Add(this);

		if ( IsMissionClient() && GetGame() && GetGame().GetCallQueue( CALL_CATEGORY_SYSTEM ) ) 
			GetGame().GetCallQueue( CALL_CATEGORY_SYSTEM ).CallLater( DeferredClientInit, 100, false );
	}
	
	void ~PlayerBase()
	{
		if (!GetGame())
			return;
	
		if ( m_PlayerUID && s_Expansion_AllPlayersUID.Get( m_PlayerUID ) == this )
		{
			RemovePlayer( m_PlayerUID );
		}

		if (s_Expansion_AllPlayers)
			s_Expansion_AllPlayers.Remove(m_Expansion_Node);
	}

	static void Expansion_SendNear(ExpansionScriptRPC rpc, vector position, float distance, Object target = null, bool guaranteed = false)
	{
		float distanceSq = distance * distance;
		foreach (string uid, PlayerBase player: s_Expansion_AllPlayersUID)
		{
			if (player.GetIdentity() && vector.DistanceSq(player.GetPosition(), position) <= distanceSq)
			{
				rpc.Expansion_Send(target, guaranteed, player.GetIdentity());
			}
		}
	}

	/**
	 * @brief Send RPC to players in near network range
	 * 
	 * @param rpc ExpansionScriptRPC instance
	 * @param position Center position to determine radius
	 * @param target Target object (or null)
	 * @param guaranteed
	 */
	static void Expansion_SendNear(ExpansionScriptRPC rpc, vector position, Object target = null, bool guaranteed = false)
	{
		float distance = GetGame().ServerConfigGetInt("networkRangeNear");
		if (distance == 0.0)
			distance = 150;  //! default as per https://community.bistudio.com/wiki/DayZ:Server_Configuration
		//! @note verified that items have to be in network range + 10% for them to exist in client bubble
		Expansion_SendNear(rpc, position, distance * 1.10, target, guaranteed);
	}

	/**
	 * @brief Send RPC to players in far network range
	 * 
	 * @param rpc ExpansionScriptRPC instance
	 * @param position Center position to determine radius
	 * @param target Target object (or null)
	 * @param guaranteed
	 */
	static void Expansion_SendFar(ExpansionScriptRPC rpc, vector position, Object target = null, bool guaranteed = false)
	{
		float distance = GetGame().ServerConfigGetInt("networkRangeFar");
		if (distance == 0.0)
			distance = 1000;  //! default as per https://community.bistudio.com/wiki/DayZ:Server_Configuration
		//! @note verified that items have to be in network range + 10% for them to exist in client bubble
		Expansion_SendNear(rpc, position, distance * 1.10, target, guaranteed);
	}

	/**
	 * @brief Send RPC to players in network range of target object
	 * 
	 * @param rpc ExpansionScriptRPC instance
	 * @param target Target object (non-null)
	 * @param guaranteed
	 * 
	 * Appropriate network range is automatically chosen based on target hierarchy root.
	 */
	static void Expansion_Send(ExpansionScriptRPC rpc, notnull Object target, bool guaranteed = false)
	{
		Object root = target;

		EntityAI targetEntity;
		if (Class.CastTo(targetEntity, target))
			root = targetEntity.GetHierarchyRoot();

		//! Target is man or is in inventory of man, or target is transport, or root has forceFarBubble
		if (root.IsInherited(Man) || target.IsInherited(Transport) || root.ConfigGetString("forceFarBubble") == "true")
			Expansion_SendFar(rpc, target.GetPosition(), target, guaranteed);
		 //! Target is in inventory of root (might be transport or non-man object)
		else if (root != target)
			Expansion_SendNear(rpc, target.GetPosition(), 20.0, target, guaranteed);
		//! Target has no parent
		else
			Expansion_SendNear(rpc, target.GetPosition(), target, guaranteed);
	}

	void Expansion_SetTemporaryOwnedContainer(ExpansionTemporaryOwnedContainer container)
	{
		container.ExpansionSetContainerOwner(this);
		m_Expansion_TemporaryOwnedContainer = container;
	}

	/**
	 * @brief Get temporary owned container previously set, or null.
	 * 
	 * @param vicinity If true (default), previously set container must be in vicinity (within 2 m of player), else null is returned.
	 */
	ExpansionTemporaryOwnedContainer Expansion_GetTemporaryOwnedContainer(bool vicinity = true)
	{
		if (m_Expansion_TemporaryOwnedContainer)
		{
			if (!vicinity || vector.DistanceSq(GetPosition(), m_Expansion_TemporaryOwnedContainer.GetPosition()) <= 4.0)
				return m_Expansion_TemporaryOwnedContainer;
		}

		return null;
	}

	ItemBase Expansion_GetNVItem()
	{
		NVGoggles goggles;
		ItemBase nvItem;

		if (IsInOptics())
		{
			//! In hands check - priority since when in optics, NVG in hands or NV optics take precedence over NVG attached to headgear/eyewear
			EntityAI entityInHands = GetHumanInventory().GetEntityInHands();
			if (entityInHands)
			{
				Weapon weapon;
				ItemOptics optic;

				if (Class.CastTo(goggles, entityInHands))
				{
					nvItem = goggles;
				}
				else if (Class.CastTo(optic, entityInHands) || (Class.CastTo(weapon, entityInHands) && Class.CastTo(optic, weapon.GetAttachedOptics())))
				{
					if (optic.GetCurrentNVType() != NVTypes.NONE)
					{
						nvItem = optic;
					}
				}
			}
		}
		else if (!nvItem)
		{
			//! Headgear/eyewear check - eyewear (e.g. NVG headstrap) takes precedence over headgear (e.g. helmet)
			EntityAI headgear = FindAttachmentBySlotName("Eyewear");
			if (headgear && Class.CastTo(goggles, headgear.FindAttachmentBySlotName("NVG")))
			{
				nvItem = goggles;
			}
			else
			{
				headgear = FindAttachmentBySlotName("Headgear");
				if (headgear && Class.CastTo(goggles, headgear.FindAttachmentBySlotName("NVG")))
				{
					nvItem = goggles;
				}
			}
		}

		return nvItem;
	}

	// ------------------------------------------------------------
	// PlayerBase GetIdentitySteam
	// ------------------------------------------------------------
	string GetIdentitySteam()
	{
		if ( IsMissionClient() )
		{
			if ( GetIdentity() )
				return GetIdentity().Expansion_GetPlainId();
			else if ( IsMissionOffline() )
				return "OFFLINE";
		}

		return m_PlayerSteam;
	}

	// ------------------------------------------------------------
	// PlayerBase GetIdentityName
	// ------------------------------------------------------------
	string GetIdentityName()
	{
		if ( IsMissionClient() )
		{
			if ( GetIdentity() )
				return GetIdentity().GetName();
			else if ( IsMissionOffline() )
				return "OFFLINE";
		}

		return m_PlayerName;
	}

	string Expansion_GetSurvivorName()
	{
		string name = GetType();
		int index = ExpansionString.LastIndexOf(name, "_");
		if (index > -1)
			name = name.Substring(index + 1, name.Length() - index - 1);

		return name;
	}
	
	static set< PlayerBase > GetAll()
	{
		Error("DEPRECATED - please use linked list s_Expansion_AllPlayers");
		set<PlayerBase> allPlayers = new set<PlayerBase>;
		auto node = s_Expansion_AllPlayers.m_Head;
		while (node)
		{
			allPlayers.Insert(node.m_Value);
			node = node.m_Next;
		}
		return allPlayers;
	}

	static set<PlayerBase> Expansion_GetInSphere(vector position, int radius)
	{
		int radiusSq = radius * radius;
		set<PlayerBase> players();

		if ( IsMissionHost() )
		{
			foreach (PlayerBase pb: s_Expansion_AllPlayersUID)
			{
				if (vector.DistanceSq(position, pb.GetPosition()) > radiusSq)
					continue;

				players.Insert(pb);
			}
		}
		else
		{
			PlayerBase pbc;
			//! @note ClientData.m_PlayerBaseList only contains players in network bubble
			foreach (Man player: ClientData.m_PlayerBaseList)
			{
				if (!Class.CastTo( pbc, player ))
					continue;

				if (vector.DistanceSq(position, pbc.GetPosition()) > radiusSq)
					continue;

				players.Insert(pbc);
			}
		}
		
		return players;
	}

	// ------------------------------------------------------------
	// PlayerBase GetPlayerByUID
	// string id = PlayerIdentity::GetId()
	// Can be used on server/client side (note that on client, will only see players in network bubble)
	// ------------------------------------------------------------
	static PlayerBase GetPlayerByUID( string id )
	{
		if ( IsMissionHost() )
		{
			return s_Expansion_AllPlayersUID.Get( id );
		}
		else
		{
			PlayerBase pbc;
			//! @note ClientData.m_PlayerBaseList only contains players in network bubble
			foreach (Man player: ClientData.m_PlayerBaseList)
			{
				if ( !Class.CastTo( pbc, player ) || !pbc.GetIdentity() )
					continue;
				
				if ( pbc.GetIdentity().GetId() == id )
					return pbc;
			}
		}
		
		return NULL;
	}

	// ------------------------------------------------------------
	// PlayerBase Expansion_GetByPlainID
	// string id = PlayerIdentity::GetPlainId()
	// Can be used on server/client side (note that on client, will only see players in network bubble)
	static PlayerBase Expansion_GetByPlainID(string id)
	{
		if ( IsMissionHost() )
		{
			foreach (PlayerBase pb: s_Expansion_AllPlayersUID)
			{
				if (!pb.GetIdentity())
					continue;
				
				if ( pb.GetIdentity().GetPlainId() == id )
					return pb;
			}
		}
		else if (ClientData.m_PlayerList && ClientData.m_PlayerList.m_PlayerList)
		{
			//! @note on client, PlayerIdentity::GetPlainId() will return empty string,
			//! so we have to find corresponding SyncPlayer and can then match with associated player using normal (hashed) ID.
			//! While ClientData.m_PlayerList.m_PlayerList contains ALL players, ClientData.m_PlayerBaseList only contains players in network bubble
			foreach (SyncPlayer syncPlayer: ClientData.m_PlayerList.m_PlayerList)
			{
				if (!syncPlayer)
					continue;

				if (syncPlayer.m_UID == id)
					return GetPlayerByUID(syncPlayer.m_RUID);
			}
		}

		return NULL;
	}

	static string Expansion_GetPlainIDByUID(string id)
	{
		return s_Expansion_AllPlayersUID2PlainID[id];
	}

	static PlayerBase ExpansionGetPlayerByIdentity( PlayerIdentity identity )
	{
		if ( !GetGame().IsMultiplayer() )
			return PlayerBase.Cast( GetGame().GetPlayer() );

		return PlayerBase.Cast( identity.GetPlayer() );
	}

	static bool Expansion_IsOnline(string uid)
	{
		if ( IsMissionHost() )
		{
			return s_Expansion_AllPlayersUID.Contains(uid);
		}
		else if (ClientData.m_PlayerList && ClientData.m_PlayerList.m_PlayerList)
		{
			foreach (SyncPlayer syncPlayer: ClientData.m_PlayerList.m_PlayerList)
			{
				if (!syncPlayer)
					continue;

				if (syncPlayer.m_RUID == uid)
					return true;
			}
		}
		
		return false;
	}

	static int Expansion_GetOnlinePlayersCount()
	{
		return s_Expansion_AllPlayersUID.Count();
	}

	/**
	 * @brief Check whether any player is in sphere specified by outer and inner radius and centered at given position
	 * 
	 * @note sphere is hollow if innerRadius > 0.0 and solid if innerRadius = 0.0 (default)
	 */
	static bool Expansion_IsPlayerInSphere(vector center, float outerRadius, float innerRadius = 0.0)
	{
		float distanceSq;
		float outerRadiusSq = outerRadius * outerRadius;
		float innerRadiusSq = innerRadius * innerRadius;
		
		foreach (string uid, PlayerBase player: s_Expansion_AllPlayersUID)
		{
			if (!player.IsAlive() || !player.GetIdentity())
				continue;

			distanceSq = vector.DistanceSq(center, player.GetPosition());

			if (distanceSq <= outerRadiusSq && distanceSq >= innerRadiusSq)
				return true;
		}
		
		return false;
	}

	// ------------------------------------------------------------
	// PlayerBase GetIdentityUID
	// ------------------------------------------------------------
	string GetIdentityUID()
	{
		if ( IsMissionClient() )
		{
			if ( GetIdentity() )
				return GetIdentity().GetId();
			else if ( IsMissionOffline() )
				return "OFFLINE";
		}

		return m_PlayerUID;
	}
	
	// ------------------------------------------------------------
	// PlayerBase Expansion_AddPlayer
	// Only called server side, to get only alive players
	// ------------------------------------------------------------
	static void Expansion_AddPlayer( PlayerBase player, PlayerIdentity identity )
	{
		if ( !player )
			return;

		if ( identity )
		{
			player.m_PlayerUID = identity.GetId();
			player.m_PlayerSteam = identity.GetPlainId();
			player.m_PlayerName = identity.GetName();
		}

		if ( player.m_PlayerUID != "" )
		{
			s_Expansion_AllPlayersUID.Set( player.m_PlayerUID, player );
			s_Expansion_AllPlayersUID2PlainID.Set( player.m_PlayerUID, player.m_PlayerSteam );
		}
	}
	
	// ------------------------------------------------------------
	// PlayerBase RemovePlayer
	// Only called server side, to get only alive players
	// ------------------------------------------------------------
	static void RemovePlayer( string id )
	{
		s_Expansion_AllPlayersUID.Remove( id );
	}	
		
	static void Expansion_RemovePlayerPlainID(string id)
	{
		s_Expansion_AllPlayersUID2PlainID.Remove( id );
	}

	// ------------------------------------------------------------
	// PlayerBase DeferredClientInit
	// ------------------------------------------------------------
	void DeferredClientInit()
	{
	}

	override void SetActions( out TInputActionMap InputActionMap )
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.PLAYER, this, "SetActions");
#endif

		super.SetActions( InputActionMap );

		AddAction( ExpansionActionPaint, InputActionMap );

		AddAction(ExpansionActionDebugStoreEntity, InputActionMap);

		AddAction(ExpansionActionDebugLobotomize, InputActionMap);
	}
	
	override bool DropItem(ItemBase item)
	{
		if (super.DropItem(item))
		{
			if (!m_Expansion_CanBeLooted)
			{
				item.Expansion_SetLootable(false);
				item.SetLifetimeMax(120);  //! Make sure it despawns quickly when left alone
			}

			return true;
		}

		return false;
	}

	// ------------------------------------------------------------
	// Expansion OnUnconsciousStart
	// ------------------------------------------------------------
	override void OnUnconsciousStart()
	{
	#ifdef EXPANSIONUI
		if (IsMissionClient())
			GetDayZGame().GetExpansionGame().GetExpansionUIManager().CloseAll();
	#endif

		super.OnUnconsciousStart();

		if (!m_Expansion_CanBeLooted)
			//! 10134 = 2 | 4 | 16 | 128 | 256 | 512 | 1024 | 8192
			ExpansionStatic.LockInventoryRecursive(this, 10134);
	}

	override void OnUnconsciousStop(int pCurrentCommandID)
	{
		super.OnUnconsciousStop(pCurrentCommandID);

		if (IsAlive() && !m_Expansion_CanBeLooted)
			//! 10134 = 2 | 4 | 16 | 128 | 256 | 512 | 1024 | 8192
			ExpansionStatic.UnlockInventoryRecursive(this, 10134);
	}

	override void OnRestrainStart()
	{
	#ifdef EXPANSIONUI
		if (IsMissionClient())
			GetDayZGame().GetExpansionGame().GetExpansionUIManager().CloseAll();
	#endif

		super.OnRestrainStart();
	}

	//! Need to override vanilla PlayerBase::IsInventoryVisible
	override bool IsInventoryVisible()
	{
		if (!super.IsInventoryVisible())
			return false;

		return m_Expansion_CanBeLooted;
	}

	override void OnConnect()
	{
		super.OnConnect();

		//! Make sure we check straight away if player connects in a safezone
		if (!Expansion_IsInSafeZone() && GetExpansionSettings().GetSafeZone().Enabled)
			m_Expansion_SafeZoneInstance.Update();
	}

	override void OnReconnect()
	{
		super.OnReconnect();

		//! Make sure we check straight away if player reconnects in a safezone
		if (!Expansion_IsInSafeZone() && GetExpansionSettings().GetSafeZone().Enabled)
			m_Expansion_SafeZoneInstance.Update();
	}

	// ------------------------------------------------------------
	// PlayerBase OnEnterZone, server + client
	// ------------------------------------------------------------
	override void OnEnterZone(ExpansionZoneType type)
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.PLAYER, this, "OnEnterZone");
#endif

		if (type == ExpansionZoneType.SAFE)
		{
			OnEnterSafeZone();
		}
	}

	#ifdef ENFUSION_AI_PROJECT
	protected eAIGroup ExpansionGetEAIGroup(bool createIfNoneExists = true)
	{
		#ifdef EXPANSIONMODAI
		eAIGroup group = eAIGroup.GetGroupByLeader(this, false);
		if (group)
			return group;
		#else
		for (int i = 0; i < eAIGroup.GROUPS.Count(); i++) {
			if (!eAIGroup.GROUPS[i]) continue;
			eAIBase GrpLeader = eAIGroup.GROUPS[i].GetLeader();
			if (GrpLeader && GrpLeader == this)
				return eAIGroup.GROUPS[i];
		}
		#endif
		
		if (!createIfNoneExists) return null;
		
		#ifdef EXPANSIONMODAI
		eAIGroup newGroup = eAIGroup.CreateGroup();
		SetGroup(newGroup);
		#else
		eAIGroup newGroup = new eAIGroup();
		newGroup.SetLeader(this);
		eAIBase eAI_PB = eAIBase.Cast(this);
		eAI_PB.SetGroup(newGroup);
		#endif
		return newGroup;
	}
	#endif

	// ------------------------------------------------------------
	// PlayerBase OnExitZone, only server side
	// ------------------------------------------------------------
	override void OnExitZone(ExpansionZoneType type)
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.PLAYER, this, "OnExitZone");
#endif

		if (type == ExpansionZoneType.SAFE)
		{
			if (GetGame().IsClient())
			{
				// OnLeftSafeZone needs to be called on client
				OnLeftSafeZone();
				return;
			}

			m_Expansion_LeavingSafeZone = true;

			if ( GetIdentity() )
				ExpansionNotification("STR_EXPANSION_SAFEZONE_TITLE", "STR_EXPANSION_SAFEZONE_LEAVING", EXPANSION_NOTIFICATION_ICON_INFO, COLOR_EXPANSION_NOTIFICATION_AMETHYST).Create(GetIdentity());
				
			//TODO: expose to settings

			//! Remove any previous call to OnLeftSafeZone from the queue
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(OnLeftSafeZone);

			//! Delay actually leaving the safezone by 10 seconds
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(OnLeftSafeZone, 10000, false);
		}
	}

	override bool Expansion_HasAdminToolGodMode()
	{
		bool godMode;
#ifdef JM_COT
		godMode |= COTHasGodMode();
#endif
#ifdef VPPADMINTOOLS
		godMode |= GodModeStatus();
#endif
		return godMode;
	}

	override bool Expansion_HasAdminToolInvisibility()
	{
		bool invisibility;
#ifdef JM_COT_INVISIBILITY
		invisibility |= COTIsInvisible();
#endif
#ifdef VPPADMINTOOLS
		invisibility |= InvisibilityStatus();
#endif
		return invisibility;
	}

	/**
	 * @brief Check if other player is considered a helper.
	 */
	bool Expansion_IsHelper(PlayerBase other, bool checkIfWeAreHelper = false)
	{
		return false;
	}

	/**
	 * @brief Check if other player is considered friendly.
	 */
	bool Expansion_IsFriendly(PlayerBase other)
	{
		if (Expansion_IsHelper(other, true))
			return true;

		return false;
	}

	// ------------------------------------------------------------
	// PlayerBase OnEnterSafeZone, server + client
	// ------------------------------------------------------------
	void OnEnterSafeZone()
	{
		//! Avoid exploit where you are just outside safezone, get shot uncon, fall backwards into safezone,
		//! then disconnect and reconnect to dupe your character
		//! (your other unconscious body will still be on the ground inside safezone due to having gained godmode from it)
		if (IsUnconscious())
			return;

		EXPrint(ToString() + "::OnEnterSafeZone");
		Print(m_Expansion_LeavingSafeZone);
		Print(m_Expansion_IsInSafeZone);
		Print(m_Expansion_IsInSafeZoneSynchRemote);

		m_Expansion_IsInSafeZone = true;
		m_Expansion_LeavingSafeZone = false;

		#ifdef SERVER
		m_Expansion_IsInSafeZoneSynchRemote = true;
		SetSynchDirty();
		#endif

		if (!GetIdentity())
		{
			//! If this is AI, we still want it to be able to raise hands in safezones so it can reload its weapon
			return;
		}

		SetCanRaise(false);

		if (GetGame().IsClient())
			return;

		if (GetExpansionSettings().GetSafeZone().DisablePlayerCollision)
			PhysicsSetSolid(false);

		if (IsRestrained())
		{
			SetRestrained(false);

			EntityAI item_in_hands = GetItemInHands();
			if (item_in_hands)
				MiscGameplayFunctions.TransformRestrainItem(item_in_hands, null, null, this);
		}
		
		if ( GetIdentity() )
		{
			ExpansionNotification("STR_EXPANSION_SAFEZONE_TITLE", "STR_EXPANSION_SAFEZONE_ENTER", EXPANSION_NOTIFICATION_ICON_INFO, COLOR_EXPANSION_NOTIFICATION_AMETHYST).Create(GetIdentity());
			
			if ( GetExpansionSettings().GetLog().Safezone )
				GetExpansionSettings().GetLog().PrintLog("[Safezone] Player \"{1:name}\" (id={1:id} pos={1:position}) entered the safezone", this);
		}
	}

	// ------------------------------------------------------------
	// PlayerBase OnLeftSafeZone, server + client
	// ------------------------------------------------------------
	void OnLeftSafeZone()
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.PLAYER, this, "OnLeftSafeZone");
#endif

		if (GetGame().IsClient())
		{
			m_Expansion_IsInSafeZone = false;
			SetCanRaise(true);
			return;
		}

		if (!m_Expansion_LeavingSafeZone)
			return;
		
		m_Expansion_LeavingSafeZone = false;

		m_Expansion_IsInSafeZone = false;

		#ifdef SERVER
		m_Expansion_IsInSafeZoneSynchRemote = false;
		SetSynchDirty();
		#endif

		if (!GetIdentity())
		{
			return;
		}

		SetCanRaise(true);

		if (GetExpansionSettings().GetSafeZone().DisablePlayerCollision)
			PhysicsSetSolid(true);
	
		if ( GetIdentity() )
		{
			ExpansionNotification("STR_EXPANSION_SAFEZONE_TITLE", "STR_EXPANSION_SAFEZONE_LEFT", EXPANSION_NOTIFICATION_ICON_INFO, COLOR_EXPANSION_NOTIFICATION_AMETHYST).Create(GetIdentity());
	
			if ( GetExpansionSettings().GetLog().Safezone )
				GetExpansionSettings().GetLog().PrintLog("[Safezone] Player \"{1:name}\" (id={1:id} pos={1:position}) left the safezone" );
		}
	}

	// ------------------------------------------------------------
	// PlayerBase SetCanRaise
	// ------------------------------------------------------------	
	void SetCanRaise(bool mode)
	{		
		HumanInputController hic = GetInputController();
		if (hic)
		{
			hic.OverrideRaise(!mode, false);
		}
	}
	
	// ------------------------------------------------------------
	// PlayerBase EEKilled
	// ------------------------------------------------------------
	override void EEKilled( Object killer )
	{
	#ifdef EXPANSIONMONITORMODULE
		ExpansionMonitorModule monitorModule = ExpansionMonitorModule.Cast(CF_ModuleCoreManager.Get(ExpansionMonitorModule));
		if (monitorModule && GetIdentity() != NULL)
		{
			string playerUID = GetIdentity().GetId();
			vector pos = GetPosition();
			monitorModule.AddLastPlayerDeathPos(playerUID, pos);
			if (GetGame().IsServer())
				monitorModule.SyncStatsToClient(this);
		}
	#endif
		super.EEKilled(killer);
	}

#ifdef DAYZ_1_25
	override bool CanReceiveItemIntoHands(EntityAI item_to_hands)
	{
		if ( Expansion_IsAttached() )
		{
			Object attachmentObject = Expansion_GetAttachmentObject();
			bool isVehicle;
			isVehicle |= attachmentObject.IsInherited(CarScript);
		#ifdef EXPANSIONMODVEHICLE
			isVehicle |= attachmentObject.IsInherited(ExpansionVehicleBase);
		#endif
			if (isVehicle)
				return CanPickupHeavyItem(item_to_hands);
		}
		
		return super.CanReceiveItemIntoHands(item_to_hands);
	}
#endif
	
	override void OnVehicleSeatDriverEnter()
	{
		super.OnVehicleSeatDriverEnter();

		auto vehicle = ExpansionVehicle.Get(this);
		if (vehicle)
		{
			vehicle.SetLastDriverUID(this);
			if (IsMissionHost())
				GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(vehicle.ResetLastDriverUIDSynch, 1000, false);
		}
	}

	static void Expansion_RegisterInventoryItemType(typename type)
	{
		EXTrace.Print(EXTrace.GENERAL_ITEMS, null, "PlayerBase::Expansion_RegisterInventoryItemType " + type.ToString());
		s_Expansion_RegisteredInventoryItemTypenames.Insert(type);
	}

	static void Expansion_RegisterInventoryItemType(string type)
	{
		typename typeName = type.ToType();
		if (typeName)
		{
			EXTrace.Print(EXTrace.GENERAL_ITEMS, null, "PlayerBase::Expansion_RegisterInventoryItemType " + typeName);
			s_Expansion_RegisteredInventoryItemTypenames.Insert(typeName);
		}
		else
		{
			type.ToLower();
			EXTrace.Print(EXTrace.GENERAL_ITEMS, null, "PlayerBase::Expansion_RegisterInventoryItemType " + type);
			s_Expansion_RegisteredInventoryItemTypes[type] = true;
		}
	}

	/**
	 * @brief Check if item class or any parent class is registered.
	 * 
	 * @param item The item to be checked if its type name matches or inherits from any registered item type
	 * @param registeredType Will be set to all lowercase item type name if registered
	 * 
	 * @return bool True if registered, false if not
	 */
	static bool Expansion_IsInventoryItemTypeRegistered(ItemBase item, out string registeredType)
	{
		//if (Expansion_IsInventoryItemTypeRegistered(item.Type(), registeredType))
			//return true;

		if (Expansion_IsInventoryItemTypeRegistered(item.GetType(), registeredType))
			return true;

		foreach (typename typeName: s_Expansion_RegisteredInventoryItemTypenames)
		{
			if (item.IsInherited(typeName))
			{
				registeredType = typeName.ToString();
				registeredType.ToLower();
				return true;
			}
		}

		return false;
	}

	//protected static bool Expansion_IsInventoryItemTypeRegistered(typename type, out string registeredType)
	//{
		//if (s_Expansion_RegisteredInventoryItemTypenames.Find(type) > -1)
		//{
			//registeredType = type.ToString();
			//return true;
		//}

		//return false;
	//}

	/**
	 * @brief Check if type name is registered.
	 * 
	 * @param type Type name (case insensitive)
	 * @param registeredType Will be set to all lowercase item type name if registered
	 * 
	 * @return bool True if registered, false if not
	 */
	static bool Expansion_IsInventoryItemTypeRegistered(string type, out string registeredType)
	{
		type.ToLower();
		if (s_Expansion_RegisteredInventoryItemTypes.Contains(type))
		{
			registeredType = type;
			return true;
		}

		return false;
	}

	void Expansion_OnInventoryUpdate(ItemBase item, bool inInventory = true, bool checkOnlyIfWorking = false)
	{
#ifdef EXTRACE_DIAG
		auto trace = EXTrace.Start0(ExpansionTracing.GENERAL_ITEMS, this, "Expansion_OnInventoryUpdate");
		EXTrace.Add(trace, item.ToString());
		EXTrace.Add(trace, inInventory);
		EXTrace.Add(trace, checkOnlyIfWorking);
#endif

		string registeredType;
		ExpansionInventoryItemType itemType;

		if (Expansion_IsInventoryItemTypeRegistered(item, registeredType))
			itemType = Expansion_OnInventoryUpdateEx(registeredType, item, inInventory, checkOnlyIfWorking);

#ifdef EXTRACE_DIAG
		if (itemType)
		{
			EXTrace.Add(trace, itemType.Items.Count());
			EXTrace.Add(trace, itemType.WorkingCount);
		}
#endif
	}

	/**
	 * @brief Update inventory item type name to inventory item type instance mapping and return new or existing instance.
	 * 
	 * @param registeredType Type name. Needs to be lowercase!
	 * @param item Item the item to add or remove
	 * @param inInventory Whether the item is in inventory
	 * @param checkOnlyIfWorking If in inventory, only update whether the item's energy manager is currently working
	 */
	protected ExpansionInventoryItemType Expansion_OnInventoryUpdateEx(string registeredType, ItemBase item, bool inInventory = true, bool checkOnlyIfWorking = false)
	{
		ExpansionInventoryItemType itemType;

		bool found = m_Expansion_InventoryItemTypes.Find(registeredType, itemType);

		if (inInventory)
		{
			if (!found)
			{
				itemType = new ExpansionInventoryItemType;
				m_Expansion_InventoryItemTypes[registeredType] = itemType;
			}

			if (!checkOnlyIfWorking || !found)
			{
				//! Item entered inventory
				itemType.Items.Insert(item);
			}

			if (item.GetCompEM())
			{
				if (item.GetCompEM().IsWorking())
					itemType.WorkingCount++;
				else if (found && checkOnlyIfWorking && itemType.WorkingCount > 0)
					//! Item already in inventory
					itemType.WorkingCount--;
			}
		}
		else if (found)
		{
			//! Item exited inventory
			if (itemType.Items.Count() > 1)
			{
				itemType.Items.RemoveItem(item);

				if (item.GetCompEM() && item.GetCompEM().IsWorking() && itemType.WorkingCount > 0)
					itemType.WorkingCount--;
			}
			else
			{
				m_Expansion_InventoryItemTypes.Remove(registeredType);
				return null;
			}
		}

		return itemType;
	}

	int Expansion_GetInventoryCount(typename type, bool working = false)
	{
		return Expansion_GetInventoryCount(Expansion_GetInventoryItemType(type), working);
	}

	int Expansion_GetInventoryCount(string type, bool working = false)
	{
		return Expansion_GetInventoryCount(Expansion_GetInventoryItemType(type), working);
	}

	int Expansion_GetInventoryCount(ExpansionInventoryItemType itemType, bool working = false)
	{
		if (itemType)
		{
			if (working)
				return itemType.WorkingCount;
			else
				return itemType.Items.Count();
		}

		return 0;
	}

	ExpansionInventoryItemType Expansion_GetInventoryItemType(typename type)
	{
		return Expansion_GetInventoryItemType(type.ToString());
	}

	ExpansionInventoryItemType Expansion_GetInventoryItemType(string type)
	{
		type.ToLower();
		return m_Expansion_InventoryItemTypes[type];
	}

	bool Expansion_PrepareTakeEntityToHands(EntityAI entity)
	{
		if (!GetHumanInventory().CanAddEntityInHands(entity))
			return false;

		if (!m_Expansion_EntityToBeTakenToHands)
			m_Expansion_EntityToBeTakenToHands = entity;

		if (m_Expansion_EntityToBeTakenToHands == entity)
			return true;

		return false;
	}

	override void EEItemIntoHands(EntityAI item)
	{
		super.EEItemIntoHands(item);

		if (m_Expansion_EntityToBeTakenToHands == item)
			m_Expansion_EntityToBeTakenToHands = null;
	}
	
#ifdef EXPANSION_MODSTORAGE
	override void CF_OnStoreSave(CF_ModStorageMap storage)
	{
		super.CF_OnStoreSave(storage);

		auto ctx = storage[DZ_Expansion_Core];
		if (!ctx) return;

		if (ExpansionEntityStorageModule.IsCurrentRootEntity(this))
			Expansion_OnEntityStorageSave(ctx);
	}

	override bool CF_OnStoreLoad(CF_ModStorageMap storage)
	{
		if (!super.CF_OnStoreLoad(storage))
		{
			EXPrint(this, "---- PlayerBase::CF_OnStoreLoad super failed  ----");
			return false;
		}

		auto ctx = storage[DZ_Expansion_Core];
		if (!ctx) return true;

		if (ExpansionEntityStorageModule.IsCurrentRootEntity(this) && !Expansion_OnEntityStorageLoad(ctx))
			return false;

		return true;
	}

	void Expansion_OnEntityStorageSave(CF_ModStorage ctx)
	{
	#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.PLAYER, this);
	#endif

		if (!m_Expansion_Entity)
			m_Expansion_Entity = new ExpansionEntity(this);

		m_Expansion_Entity.CF_OnStoreSave(ctx);

		//! HumanInventory is not moddable, but its OnStoreSave is a no-op in vanilla anyway (writes no data)
		//GetHumanInventory().OnStoreSave(ctx);
		CF_OnStoreSaveLifespan(ctx);

	#ifdef SERVER
		GetPlayerStats().CF_SaveStats(ctx);
		m_ModifiersManager.CF_OnStoreSave(ctx);
		m_AgentPool.CF_OnStoreSave(ctx);
		GetSymptomManager().CF_OnStoreSave(ctx);
		GetBleedingManagerServer().CF_OnStoreSave(ctx);
		m_PlayerStomach.CF_OnStoreSave(ctx);
		ctx.Write(GetBrokenLegs());
		CF_SaveAreaPersistenceFlag(ctx);

		HumanCommandLadder ladder = GetCommand_Ladder();
		if (ladder)
		{
			ctx.Write(true);
			ctx.Write(ladder.GetLogoutPosition());
		}
		else
		{
			ctx.Write(false);
		}
		
		ArrowManagerPlayer arrowManager = ArrowManagerPlayer.Cast(GetArrowManager());
		arrowManager.CF_Save(ctx);
	#endif
	}

	bool Expansion_OnEntityStorageLoad(CF_ModStorage ctx)
	{
		if (!m_Expansion_Entity)
			m_Expansion_Entity = new ExpansionEntity(this);

		if (!m_Expansion_Entity.CF_OnStoreLoad(ctx, m_Expansion_StoreLoadedVersion))
		{
			EXPrint(this, "---- PlayerBase::Expansion_OnEntityStorageLoad failed to load entity state  ----");
			return false;
		}

		//! HumanInventory is not moddable, but its OnStoreLoad is a no-op in vanilla anyway (reads no data)
		//if (!GetHumanInventory().OnStoreLoad(ctx, m_Expansion_StoreLoadedVersion))
		//{
			//EXPrint(this, "---- PlayerBase::Expansion_OnEntityStorageLoad failed to load inventory state  ----");
			//return false;
		//}

		if (!CF_OnStoreLoadLifespan(ctx, m_Expansion_StoreLoadedVersion))
		{
			EXPrint(this, "---- PlayerBase::Expansion_OnEntityStorageLoad failed to load lifespan state  ----");
			return false;
		}

	#ifdef SERVER
		if (!GetPlayerStats().CF_LoadStats(ctx, m_Expansion_StoreLoadedVersion)) // load stats
		{
			EXPrint(this, "---- PlayerBase::Expansion_OnEntityStorageLoad failed to load PlayerStats  ----");
			return false;
		}
		
		if (!m_ModifiersManager.CF_OnStoreLoad(ctx, m_Expansion_StoreLoadedVersion))
		{
			EXPrint(this, "---- PlayerBase::Expansion_OnEntityStorageLoad failed to load ModifiersManager, read fail  ----");
			return false;
		}
		
		if (!m_AgentPool.CF_OnStoreLoad(ctx, m_Expansion_StoreLoadedVersion))
		{
			EXPrint(this, "---- PlayerBase::Expansion_OnEntityStorageLoad failed to load AgentPool, read fail  ----");
			return false;
		}
		
		
		if (!GetSymptomManager().CF_OnStoreLoad(ctx, m_Expansion_StoreLoadedVersion))
		{
			EXPrint(this, "---- PlayerBase::Expansion_OnEntityStorageLoad failed to load SymptomManager, read fail  ----");
			return false;
		}
		
		if (!GetBleedingManagerServer().CF_OnStoreLoad(ctx, m_Expansion_StoreLoadedVersion))
		{
			EXPrint(this, "---- PlayerBase::Expansion_OnEntityStorageLoad failed to load BleedingManagerServer, read fail  ----");
			return false;
		}
		
		if (!m_PlayerStomach.CF_OnStoreLoad(ctx, m_Expansion_StoreLoadedVersion))
		{
			EXPrint(this, "---- PlayerBase::Expansion_OnEntityStorageLoad failed to load PlayerStomach, read fail  ----");
			return false;
		}
		
		if (!ctx.Read(m_BrokenLegState))
		{
			EXPrint(this, "---- PlayerBase::Expansion_OnEntityStorageLoad failed to load broken legs state  ----");
			return false;
		}

		if (!ctx.Read(m_PersistentFlags))
		{
			EXPrint(this, "---- PlayerBase::Expansion_OnEntityStorageLoad failed to load Persistent Flags, read fail  ----");
			return false;
		}

		bool onLadder;
		if (!ctx.Read(onLadder))
		{
			EXPrint(this, "---- PlayerBase::Expansion_OnEntityStorageLoad failed to load ladder state  ----");
			return false;
		}

		if (onLadder)
		{
			vector position;
			if (!ctx.Read(position))
			{
				EXPrint(this, "---- PlayerBase::Expansion_OnEntityStorageLoad failed to load position on ladder  ----");
				return false;
			}

			Hive hive = GetHive();
			if (!hive || !hive.CharacterIsLoginPositionChanged(this))
				SetPosition(position);
		}
	
		ArrowManagerPlayer arrowManager = ArrowManagerPlayer.Cast(GetArrowManager());
		if (!arrowManager.CF_Load(ctx))
		{
			EXPrint(this, "---- PlayerBase::Expansion_OnEntityStorageLoad failed to load arrow manager state  ----");
			return false;
		}
	#endif

		EXPrint(this, "---- PlayerBase::Expansion_OnEntityStorageLoad SUCCESS ----");

		return true;
	}

	void CF_OnStoreSaveLifespan(CF_ModStorage ctx)
	{
		ctx.Write(m_LifeSpanState);	
		ctx.Write(m_LastShavedSeconds);	
		ctx.Write(m_HasBloodyHandsVisible);
		ctx.Write(m_HasBloodTypeVisible);
		ctx.Write(m_BloodType);
	}

	bool CF_OnStoreLoadLifespan(CF_ModStorage ctx, int version)
	{
		int lifespan_state = 0;
		if (!ctx.Read(lifespan_state))
			return false;
		m_LifeSpanState = lifespan_state;
		
		int last_shaved = 0;
		if (!ctx.Read(last_shaved))
			return false;
		m_LastShavedSeconds = last_shaved;

		int bloody_hands = 0;
		if (!ctx.Read(bloody_hands))
			return false;
		m_HasBloodyHandsVisible = bloody_hands;

		bool blood_visible = false;
		if (!ctx.Read(blood_visible))
			return false;
		m_HasBloodTypeVisible = blood_visible;

		int blood_type = 0;
		if (!ctx.Read(blood_type))
			return false;
		m_BloodType = blood_type;

		return true;
	}

	void CF_SaveAreaPersistenceFlag(CF_ModStorage ctx)
	{
		if (GetModifiersManager())
			SetPersistentFlag(PersistentFlag.AREA_PRESENCE, GetModifiersManager().IsModifierActive(eModifiers.MDF_AREAEXPOSURE));//set the flag for player's presence in contaminated area
		ctx.Write(m_PersistentFlags);
	}
#endif
}
