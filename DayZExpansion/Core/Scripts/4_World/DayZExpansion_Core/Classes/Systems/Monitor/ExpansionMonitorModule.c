/**
 * ExpansionMonitorModule.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

/**@class		ExpansionMonitorModule
 * @brief		This class handle expansion monitor module
 * @note		Player identity plain (steam64) ID is used for stats and states to reduce data payload,
 * 				normal (hashed) ID is used for player death position
 **/

#ifdef EXPANSIONMONITORMODULE
[CF_RegisterModule(ExpansionMonitorModule)]
class ExpansionMonitorModule: CF_ModuleWorld
{
	private const float UPDATE_TICK_TIME = 1.0; // refreshes 100 players every ten seconds
	private const int UPDATE_PLAYERS_PER_TICK = 10;

	//Server only
	private ref map<string, ref ExpansionSyncedPlayerStats> m_Stats;
	private ref map<string, ref ExpansionSyncedPlayerStates> m_States;
	
	private float m_UpdateQueueTimer;
	private int m_CurrentPlayerTick;
	
	//Client only
	static ref ExpansionSyncedPlayerStats m_ClientStats;
	static ref ExpansionSyncedPlayerStates m_ClientStates;
	
	static ref ScriptInvoker m_StatsInvoker = new ScriptInvoker();
	static ref ScriptInvoker m_StatesInvoker = new ScriptInvoker();

	vector m_LastDeathPos;
	
	// Server & Client
	private ref map<string, vector> m_PlayerDeathPositions;
	
	// ------------------------------------------------------------
	// ExpansionMonitorModule OnInit
	// ------------------------------------------------------------
	override void OnInit()
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.PLAYER_MONITOR, this, "OnInit");
#endif

		super.OnInit();

		EnableClientDisconnect();
		EnableClientNew();
		EnableClientReady();
		EnableInvokeConnect();
		Expansion_EnableRPCManager();

		//EXPrint(ToString() + "::OnInit - GetGame() " + GetGame());  // OK
		//EXPrint(ToString() + "::OnInit - GetGame().IsServer() " + GetGame().IsServer());  // true on client during init
		//EXPrint(ToString() + "::OnInit - GetGame().IsMultiplayer() " + GetGame().IsMultiplayer());  // false on client during init

		m_Stats = new map<string, ref ExpansionSyncedPlayerStats>;
		m_States = new map<string, ref ExpansionSyncedPlayerStates>;

		m_ClientStats = new ExpansionSyncedPlayerStats;
		m_ClientStates = new ExpansionSyncedPlayerStates;
		
		m_PlayerDeathPositions = new map<string, vector>;

		Expansion_RegisterServerRPC("RPC_RequestPlayerStates");
		Expansion_RegisterServerRPC("RPC_RequestPlayerStats");
		Expansion_RegisterServerRPC("RPC_RequestPlayerStatsAndStates");
		Expansion_RegisterClientRPC("RPC_SyncStats");
		Expansion_RegisterClientRPC("RPC_SyncStates");
		Expansion_RegisterClientRPC("RPC_SendMessage");
		Expansion_RegisterClientRPC("RPC_SendPlayerStates");
		Expansion_RegisterClientRPC("RPC_SendPlayerStats");
		Expansion_RegisterClientRPC("RPC_SendPlayerStatsAndStates");
		Expansion_RegisterClientRPC("RPC_SyncLastDeathPos");
	}

	void RPC_SyncStats(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
		if (!m_ClientStats)
			return;

		m_ClientStats.OnRecieve(ctx, true, false);
		m_StatsInvoker.Invoke(m_ClientStats);
	}

	void RPC_SyncStates(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
		if (!m_ClientStates)
			return;
		
		m_ClientStates.OnRecieve(ctx);
		m_StatesInvoker.Invoke(m_ClientStates);
	}

	void RPC_RequestPlayerStats(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
		RPC_RequestPlayerStatsAndStates(ctx, sender, true, false);
	}

	void RPC_SendPlayerStats(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
		RPC_SendPlayerStatsAndStates(ctx, true, false);
	}

	void RPC_RequestPlayerStates(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
		RPC_RequestPlayerStatsAndStates(ctx, sender, false, true);
	}

	void RPC_SendPlayerStates(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
		RPC_SendPlayerStatsAndStates(ctx, false, true);
	}

	void RPC_RequestPlayerStatsAndStates(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
		RPC_RequestPlayerStatsAndStates(ctx, sender, true, true);
	}

	void RPC_SendPlayerStatsAndStates(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
		RPC_SendPlayerStatsAndStates(ctx, true, true);
	}

	override void OnClientNew(Class sender, CF_EventArgs args)
	{
		super.OnClientNew(sender, args);

		auto cArgs = CF_EventPlayerArgs.Cast(args);

		bool isClientRespawn = SyncEvents.s_Expansion_RespawningUIDs[cArgs.Identity.GetId()];
		EXTrace.Print(EXTrace.PLAYER_MONITOR, this, "::OnClientNew - is client respawn? " + isClientRespawn);

		if (!isClientRespawn)
			PlayerJoinedNotification(cArgs.Identity);
	}

	override void OnClientReady(Class sender, CF_EventArgs args)
	{
		super.OnClientReady(sender, args);

		auto cArgs = CF_EventPlayerArgs.Cast(args);

		PlayerJoinedNotification(cArgs.Identity);
	}

	void PlayerJoinedNotification(PlayerIdentity identity)
	{
		if (GetExpansionSettings().GetNotification().ShowPlayerJoinServer && identity)
		{			
			if (GetExpansionSettings().GetNotification().JoinMessageType == ExpansionAnnouncementType.NOTIFICATION) 
			{
				ExpansionNotification("STR_EXPANSION_PLAYER_JOINED_TITLE", new StringLocaliser("STR_EXPANSION_PLAYER_JOINED_TEXT", identity.GetName()), EXPANSION_NOTIFICATION_ICON_PERSONA).Info();
			} 
			else if (GetExpansionSettings().GetNotification().JoinMessageType == ExpansionAnnouncementType.CHAT)
			{
				ServerChatMessage(new StringLocaliser("STR_EXPANSION_PLAYER_JOINED_TITLE"), new StringLocaliser("STR_EXPANSION_PLAYER_JOINED_TEXT", identity.GetName()));
			}
		}
	}
	
	// ------------------------------------------------------------
	// ExpansionMonitorModule OnInvokeConnect
	// ------------------------------------------------------------
	override void OnInvokeConnect(Class sender, CF_EventArgs args)
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.PLAYER_MONITOR, this, "OnInvokeConnect");
#endif

		super.OnInvokeConnect(sender, args);

		auto cArgs = CF_EventPlayerArgs.Cast(args);

		if (!cArgs.Identity)
			return;

		//! If this is a respawn, need to do nothing of the below
		if (SyncEvents.s_Expansion_RespawningUIDs[cArgs.Identity.GetId()])
			return;

		string playerID = cArgs.Identity.GetPlainId();
		AddPlayerStats(playerID);
		AddPlayerStates(playerID);
		
		SyncLastDeathPos(cArgs.Identity);
	}
	
	// ------------------------------------------------------------
	// ExpansionMonitorModule OnClientDisconnect
	// Called on server
	// ------------------------------------------------------------
	override void OnClientDisconnect(Class sender, CF_EventArgs args)
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.PLAYER_MONITOR, this, "OnClientDisconnect");
#endif

		super.OnClientDisconnect(sender, args);

		auto cArgs = CF_EventPlayerDisconnectedArgs.Cast(args);
		
		string playerID = PlayerBase.Expansion_GetPlainIDByUID(cArgs.UID);
		RemovePlayerStats(playerID);
		RemovePlayerStates(playerID);

		PlayerLeftNotification(cArgs.Identity);
	}
	
	void PlayerLeftNotification(PlayerIdentity identity)
	{
		if (GetExpansionSettings().GetNotification().ShowPlayerLeftServer && identity) 
		{			
			if (GetExpansionSettings().GetNotification().LeftMessageType == ExpansionAnnouncementType.NOTIFICATION) 
			{
				ExpansionNotification("STR_EXPANSION_PLAYER_LEFT_TITLE", new StringLocaliser("STR_EXPANSION_PLAYER_LEFT_TEXT", identity.GetName()), EXPANSION_NOTIFICATION_ICON_PERSONA).Info();
			} 
			else if (GetExpansionSettings().GetNotification().LeftMessageType == ExpansionAnnouncementType.CHAT)
			{
				ServerChatMessage(new StringLocaliser("STR_EXPANSION_PLAYER_LEFT_TITLE"), new StringLocaliser("STR_EXPANSION_PLAYER_LEFT_TEXT", identity.GetName()));
			}
		}
	}

	// ------------------------------------------------------------
	// ExpansionMonitorModule UpdateStats
	// ------------------------------------------------------------	
	private void UpdateStats(string playerID)
	{
		if (!GetGame().IsServer() && !GetGame().IsMultiplayer())
			return;
		
		ExpansionSyncedPlayerStats stats = m_Stats.Get(playerID);
		if (!stats)
		{
			Error("ExpansionMonitorModule::UpdateStats - Could not get player stats for player with id: " + playerID);
			return;
		}
		
		PlayerBase player = PlayerBase.Expansion_GetByPlainID(playerID);
		if (!player)
		{
			//! Don't error here, player will be NULL if dead
			return;
		}
			
		UpdateStats(stats, player, false, false);
	}

	private void UpdateStats(ExpansionSyncedPlayerStats stats, PlayerBase player, bool send = false, bool includeRegisteredStats = false, bool includeBaseStats = true)
	{
		if (includeBaseStats)
		{
			stats.m_Health = CalcHealth(player);
			stats.m_Blood = CalcBlood(player);
			stats.m_Water = CalcWater(player);
			stats.m_Energy = CalcEnergy(player);
			stats.m_Position = player.GetPosition();
		}

		if (includeRegisteredStats)
		{
			//stats.m_Stamina = CalcStamina(player);
			stats.m_Distance = player.StatGet(AnalyticsManagerServer.STAT_DISTANCE);
			stats.m_Playtime = player.StatGet(AnalyticsManagerServer.STAT_PLAYTIME);
			stats.m_PlayersKilled = player.StatGet(AnalyticsManagerServer.STAT_PLAYERS_KILLED);
		#ifdef ENFUSION_AI_PROJECT
			stats.m_AIKilled = player.StatGet(AnalyticsManagerServer.EXP_STAT_AI_PLAYERS_KILLED);
		#endif
			stats.m_InfectedKilled = player.StatGet(AnalyticsManagerServer.STAT_INFECTED_KILLED);
			stats.m_AnimalsKilled = player.StatGet(AnalyticsManagerServer.EXP_STAT_ANIMALS_KILLED);
			stats.m_LongestShot = player.StatGet(AnalyticsManagerServer.STAT_LONGEST_SURVIVOR_HIT);
		}
				
		if (!send)
			return;

		auto rpc = Expansion_CreateRPC("RPC_SyncStats");
		stats.OnSend(rpc, includeRegisteredStats, includeBaseStats);
		//! @note guaranteed = false is intentional here (performance)
		rpc.Expansion_Send(false, player.GetIdentity());
	}

	// ------------------------------------------------------------
	// ExpansionMonitorModule UpdateStates
	// ------------------------------------------------------------		
	private void UpdateStates(string playerID)
	{
		if (!GetGame().IsServer() && !GetGame().IsMultiplayer())
			return;
		
		ExpansionSyncedPlayerStates states = m_States.Get(playerID);
		if (!states)
		{
			Error("ExpansionMonitorModule::UpdateStates - Could not get player states for player with id: " + playerID);
			return;
		}
		
		PlayerBase player = PlayerBase.Expansion_GetByPlainID(playerID);
		if (!player)
		{
			//! Don't error here, player will be NULL if dead
			return;
		}
			
		UpdateStates(states, player, false);
	}

	private void UpdateStates(ExpansionSyncedPlayerStates states, PlayerBase player, bool send = false)
	{
		states.m_Bones = BonesState(player);
		states.m_Sick = IsBrainSick(player);
		states.m_Cholera = HasCholera(player);
		states.m_Influenza = HasInfluenza(player);
		states.m_Salmonella = HasSalmonella(player);
		states.m_Poison = HasFoodPoison(player);
		states.m_Infection= HasWoundInfection(player);
		states.m_Cuts = HasCuts(player);
		states.m_Stance = GetPlayerStance(player);
		
		if (!send)
			return;

		auto rpc = Expansion_CreateRPC("RPC_SyncStates");
		states.OnSend(rpc);
		//! @note guaranteed = false is intentional here (performance)
		rpc.Expansion_Send(false, player.GetIdentity());
	}
	
	// ------------------------------------------------------------
	// ExpansionMonitorModule AddPlayerStats
	// Called on server
	// ------------------------------------------------------------
	void AddPlayerStats(string playerID)
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.PLAYER_MONITOR, this, "AddPlayerStats");
#endif

		if (!GetGame().IsServer() && !GetGame().IsMultiplayer())
			return;
				
		if (!m_Stats.Contains(playerID)) //! RemovePlayerStats would never be called if AddPlayerStats was called twice (logout cancelled or reconnected in time)
		{
			auto stats = new ExpansionSyncedPlayerStats;
			m_Stats.Insert(playerID, stats);
		}
	}
	
	// ------------------------------------------------------------
	// ExpansionMonitorModule AddPlayerStates
	// Called on server
	// ------------------------------------------------------------
	void AddPlayerStates(string playerID)
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.PLAYER_MONITOR, this, "AddPlayerStates");
#endif

		if (!GetGame().IsServer() && !GetGame().IsMultiplayer())
			return;
		
		if (!m_States.Contains(playerID)) //! RemovePlayerStates would never be called if AddPlayerStats was called twice (logout cancelled or reconnected in time)
		{
			auto states = new ExpansionSyncedPlayerStates;
			m_States.Insert(playerID, states);
		}
	}
	
	// ------------------------------------------------------------
	// ExpansionMonitorModule RemovePlayerStats
	// Called on server
	// ------------------------------------------------------------
	void RemovePlayerStats(string playerID)
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.PLAYER_MONITOR, this, "RemovePlayerStats");
#endif

		if (!GetGame().IsServer() && !GetGame().IsMultiplayer())
			return;
		
		if (m_Stats.Contains(playerID))
			m_Stats.Remove(playerID);
	}
	
	// ------------------------------------------------------------
	// ExpansionMonitorModule RemovePlayerStates
	// Called on server
	// ------------------------------------------------------------
	void RemovePlayerStates(string playerID)
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.PLAYER_MONITOR, this, "RemovePlayerStates");
#endif

		if (!GetGame().IsServer() && !GetGame().IsMultiplayer())
			return;
		
		if (m_States.Contains(playerID))
			m_States.Remove(playerID);
	}
	
	// ------------------------------------------------------------
	// ExpansionMonitorModule ServerChatMessage
	// Called on server
	// ------------------------------------------------------------
	void ServerChatMessage(StringLocaliser tag, StringLocaliser message)
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.PLAYER_MONITOR, this, "ServerChatMessage");
#endif

		auto message_rpc = Expansion_CreateRPC("RPC_SendMessage");
		message_rpc.Write(tag);
		message_rpc.Write(message);
		message_rpc.Expansion_Send(true);
	}
	
	// ------------------------------------------------------------
	// ExpansionMonitorModule RPC_SendMessage
	// Called on all Clients
	// ------------------------------------------------------------
	private void RPC_SendMessage(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.PLAYER_MONITOR, this, "RPC_SendMessage");
#endif

		StringLocaliser tag;
		if (!ctx.Read(tag))
			return;
		
		StringLocaliser message;
		if (!ctx.Read(message))
			return;
		
		GetGame().GetMission().OnEvent(ChatMessageEventTypeID, new ChatMessageEventParams(ExpansionChatChannels.CCSystem, "", tag.Format() + " - " + message.Format(), ""));
	}
	
	// ------------------------------------------------------------
	// ExpansionMonitorModule CalcHealth
	// Called on server
	// ------------------------------------------------------------
	static int CalcHealth(PlayerBase player)
	{
		return Calc(player.GetHealth("","Health"), player.GetMaxHealth("GlobalHealth", "Health"));
	}
	
	// ------------------------------------------------------------
	// ExpansionMonitorModule CalcBlood
	// Called on server
	// ------------------------------------------------------------
	static int CalcBlood(PlayerBase player)
	{
		return Calc(player.GetHealth("", "Blood") - 2500,  2500);
	}

	// ------------------------------------------------------------
	// ExpansionMonitorModule CalcWater
	// Called on server
	// ------------------------------------------------------------
	static int CalcWater(PlayerBase player)
	{
		return CalcStat(player.GetStatWater());
	}

	// ------------------------------------------------------------
	// ExpansionMonitorModule CalcEnergy
	// Called on server
	// ------------------------------------------------------------
	static int CalcEnergy(PlayerBase player)
	{
		return CalcStat(player.GetStatEnergy());
	}
	
	// ------------------------------------------------------------
	// ExpansionMonitorModule CalcStamina
	// Called on server
	// ------------------------------------------------------------
	static int CalcStamina(PlayerBase player)
	{
		return CalcStat(player.GetStatStamina());
	}
	
	// ------------------------------------------------------------
	// ExpansionMonitorModule Calc
	// Called on server
	// ------------------------------------------------------------
	static int Calc(float value, float max)
	{
		return (Math.Round(value) * 100) / max;
	}
	
	// ------------------------------------------------------------
	// ExpansionMonitorModule CalcStat
	// Called on server
	// ------------------------------------------------------------
	static int CalcStat(PlayerStat<float> stat)
	{
		return Calc(stat.Get(), stat.GetMax());
	}
	
	// ------------------------------------------------------------
	// ExpansionMonitorModule BonesState
	// Called on server
	// ------------------------------------------------------------
	static int BonesState(PlayerBase player)
	{
		int bonesState;
		eBrokenLegs state = player.GetBrokenLegs();
		if (state == eBrokenLegs.NO_BROKEN_LEGS)
		{
			bonesState = 0;
		}
		else if (state == eBrokenLegs.BROKEN_LEGS)
		{
			bonesState = 1;
		}
		else if (state == eBrokenLegs.BROKEN_LEGS_SPLINT)
		{
			bonesState = 2;
		}
		
		return bonesState;
	}
	
	// ------------------------------------------------------------
	// ExpansionMonitorModule IsBrainSick
	// Called on server
	// ------------------------------------------------------------
	static bool IsBrainSick(PlayerBase player)
	{
		bool brainSick = false;
		
		if (player.GetSingleAgentCount(eAgents.BRAIN) >= BrainDiseaseMdfr.AGENT_THRESHOLD_ACTIVATE)
			return true;
		
		return false;
	}
	
	// ------------------------------------------------------------
	// ExpansionMonitorModule HasCholera
	// Called on server
	// ------------------------------------------------------------
	static bool HasCholera(PlayerBase player)
	{
		if (player.GetSingleAgentCount(eAgents.CHOLERA) >= CholeraMdfr.AGENT_THRESHOLD_ACTIVATE)
			return true;
		
		return false;
	}
	
	// ------------------------------------------------------------
	// ExpansionMonitorModule HasInfluenza
	// Called on server
	// ------------------------------------------------------------
	static bool HasInfluenza(PlayerBase player)
	{
		if (player.GetSingleAgentCount(eAgents.INFLUENZA) >= InfluenzaMdfr.AGENT_THRESHOLD_ACTIVATE)
			return true;
		
		return false;
	}
	
	// ------------------------------------------------------------
	// ExpansionMonitorModule HasSalmonella
	// Called on server
	// ------------------------------------------------------------
	static bool HasSalmonella(PlayerBase player)
	{
		if (player.GetSingleAgentCount(eAgents.SALMONELLA) >= SalmonellaMdfr.AGENT_THRESHOLD_ACTIVATE)
			return true;
		
		return false;
	}
	
	// ------------------------------------------------------------
	// ExpansionMonitorModule HasFoodPoison
	// Called on server
	// ------------------------------------------------------------
	static bool HasFoodPoison(PlayerBase player)
	{
		if (player.GetSingleAgentCount(eAgents.FOOD_POISON) >= PoisoningMdfr.AGENT_THRESHOLD_ACTIVATE)
			return true;
		
		return false;
	}
	
	// ------------------------------------------------------------
	// ExpansionMonitorModule HasWoundInfection
	// Called on server
	// ------------------------------------------------------------
	static bool HasWoundInfection(PlayerBase player)
	{
		if (player.GetSingleAgentCount(eAgents.WOUND_AGENT) >= WoundInfectionMdfr.AGENT_THRESHOLD_ACTIVATE)
			return true;
		
		return false;
	}
	
	// ------------------------------------------------------------
	// ExpansionMonitorModule HasCuts
	// Called on server
	// ------------------------------------------------------------
	static int HasCuts(PlayerBase player)
	{
		int cuts;
		cuts = player.GetBleedingSourceCount();
		if (player.IsBleeding() && cuts > 0)
			return cuts;
		
		return 0;
	}
	
	// ------------------------------------------------------------
	// ExpansionMonitorModule GetPlayerStance
	// Called on server
	// ------------------------------------------------------------	
	static ExpansionPlayerStanceStatus GetPlayerStance(PlayerBase player)
	{
		ExpansionPlayerStanceStatus stance = ExpansionPlayerStanceStatus.UNKNOWN;
		ExpansionVehicle vehicle;
		
		if (!ExpansionVehicle.Get(vehicle, player))
		{
			if (!player.IsAlive())
			{
				stance = ExpansionPlayerStanceStatus.DEAD;
			}
			else if (player.IsUnconscious())
			{
				stance = ExpansionPlayerStanceStatus.UNCONSCIOUS;
			}
			else if (player.IsPlayerInStance(DayZPlayerConstants.STANCEMASK_RAISEDERECT) || player.IsPlayerInStance(DayZPlayerConstants.STANCEMASK_ERECT))
			{
				stance = ExpansionPlayerStanceStatus.STAND;
			}
			else if (player.IsPlayerInStance(DayZPlayerConstants.STANCEMASK_CROUCH) || player.IsPlayerInStance(DayZPlayerConstants.STANCEMASK_RAISEDCROUCH))
			{
				stance = ExpansionPlayerStanceStatus.CROUCH;
			}
			else if (player.IsPlayerInStance(DayZPlayerConstants.STANCEMASK_PRONE) || player.IsPlayerInStance(DayZPlayerConstants.STANCEMASK_RAISEDPRONE))
			{
				stance = ExpansionPlayerStanceStatus.PRONE;
			}
		}
		else
		{
			if (vehicle.IsHelicopter())
			{
				stance = ExpansionPlayerStanceStatus.HELICOPTER;
			}
			else if (vehicle.IsBoat())
			{
				stance = ExpansionPlayerStanceStatus.BOAT;
			}
			else
			{
				stance = ExpansionPlayerStanceStatus.CAR;
			}
		}
		
		return stance;
	}
	
	// ------------------------------------------------------------
	// ExpansionMonitorModule GetClientStats
	// Called on client
	// ------------------------------------------------------------ 
	ExpansionSyncedPlayerStats GetClientStats()
	{
		return m_ClientStats;
	}
	
	// ------------------------------------------------------------
	// ExpansionMonitorModule GetClientStates
	// Called on client
	// ------------------------------------------------------------ 
	ExpansionSyncedPlayerStates GetClientStates()
	{
		return m_ClientStates;
	}
	
	// ------------------------------------------------------------
	// ExpansionMonitorModule RequestPlayerStats
	// Called from client - if empty string given as playerID, use sender ID
	// ------------------------------------------------------------
	void RequestPlayerStats(string playerID = "", bool includeRegisteredStats = false, bool includeBaseStats = true)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(ExpansionTracing.PLAYER_MONITOR, this, playerID, "" + includeRegisteredStats, "" + includeBaseStats);
#endif
		
		auto rpc = Expansion_CreateRPC("RPC_RequestPlayerStats");
		rpc.Write(playerID);
		rpc.Write(includeRegisteredStats);
		rpc.Write(includeBaseStats);
		//! @note guaranteed = false is intentional here (performance)
		rpc.Expansion_Send(false);
	}
	
	// ------------------------------------------------------------
	// ExpansionMonitorModule RPC_RequestPlayerStatsAndStates
	// Called on server
	// ------------------------------------------------------------
	private void RPC_RequestPlayerStatsAndStates(ParamsReadContext ctx, PlayerIdentity sender, bool includeStats = true, bool includeStates = true)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(ExpansionTracing.PLAYER_MONITOR, this, "" + includeStats, "" + includeStates);
#endif
		
		string playerID;
		if (!ctx.Read(playerID))
			return;

#ifdef EXTRACE
		EXTrace.Add(trace, playerID);
#endif
		
		string originalPlayerID = playerID;

		bool includeRegisteredStats;
		if (includeStats && !ctx.Read(includeRegisteredStats))
			return;

		bool includeBaseStats;
		if (includeStats && !ctx.Read(includeBaseStats))
			return;

		if (!sender)
			return;

		PlayerBase player;
		if (playerID)
		{
			player = PlayerBase.Expansion_GetByPlainID(playerID);

			if (!player)
			{
				//! Player instance was probably removed (e.g. gravecross deleted body)
				//EXPrint("[ExpansionMonitorModule] WARNING: Player with plain ID " + playerID + " was not found");
				return;
			}
		}
		else
		{
			playerID = sender.GetPlainId();
			player = PlayerBase.GetPlayerByUID(sender.GetId());

			if (!player)
			{
				//! Player instance was probably removed (e.g. gravecross deleted body)
				//EXPrint("[ExpansionMonitorModule] WARNING: Player with ID " + sender.GetId() + " (plain ID " + playerID + ") was not found");
				return;
			}
		}

		if (!playerID)
		{
			EXPrint("[ExpansionMonitorModule] WARNING: Player plain ID is empty string");
			return;
		}

		ExpansionSyncedPlayerStats playerStats;
		if (includeStats)
		{
			playerStats = GetPlayerStatsByID(playerID);
			if (!playerStats)
			{
				return;
			}
			UpdateStats(playerStats, player, false, includeRegisteredStats, includeBaseStats);
		}
		
		ExpansionSyncedPlayerStates playerStates;
		if (includeStates)
		{
			playerStates = GetPlayerStatesByID(playerID);
			if (!playerStates)
			{
				return;
			}
			UpdateStates(playerStates, player, false);
		}
		
		SendPlayerStatsAndStates(playerStats, playerStates, originalPlayerID, sender, includeRegisteredStats, includeBaseStats);
	}
	
	// ------------------------------------------------------------
	// ExpansionMonitorModule GetPlayerStatsByID
	// Called on server
	// ------------------------------------------------------------
	ExpansionSyncedPlayerStats GetPlayerStatsByID(string playerID)
	{
		ExpansionSyncedPlayerStats playerStats;
		m_Stats.Find(playerID, playerStats);
		return playerStats;
	}
	
	// ------------------------------------------------------------
	// ExpansionMonitorModule SendPlayerStatsAndStates
	// Called on server
	// ------------------------------------------------------------
	void SendPlayerStatsAndStates(ExpansionSyncedPlayerStats playerStats, ExpansionSyncedPlayerStates playerStates, string playerID, PlayerIdentity ident, bool includeRegisteredStats = false, bool includeBaseStats = true)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(ExpansionTracing.PLAYER_MONITOR, this, "" + playerStats, "" + playerStates, playerID, "" + includeRegisteredStats, "" + includeBaseStats);
#endif
		
		string fn;
		if (playerStats && playerStates)
			fn = "RPC_SendPlayerStatsAndStates";
		else if (playerStats)
			fn = "RPC_SendPlayerStats";
		else
			fn = "RPC_SendPlayerStates";

		auto rpc = Expansion_CreateRPC(fn);

		rpc.Write(playerID);

		if (playerStats)
		{
			rpc.Write(includeRegisteredStats);
			rpc.Write(includeBaseStats);
			playerStats.OnSend(rpc, includeRegisteredStats, includeBaseStats);
		}

		if (playerStates)
			playerStates.OnSend(rpc);

		//! @note guaranteed = false is intentional here (performance)
		rpc.Expansion_Send(false, ident);
	}
	
	// ------------------------------------------------------------
	// ExpansionMonitorModule RPC_SendPlayerStatsAndStates
	// Called on client
	// ------------------------------------------------------------
	private void RPC_SendPlayerStatsAndStates(ParamsReadContext ctx, bool includeStats = true, bool includeStates = true)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(ExpansionTracing.PLAYER_MONITOR, this, "" + includeStats, "" + includeStates);
#endif
		
		string playerID;
		if (!ctx.Read(playerID))
			return;

#ifdef EXTRACE
		EXTrace.Add(trace, playerID);
#endif
			
		bool includeRegisteredStats;
		if (includeStats && !ctx.Read(includeRegisteredStats))
			return;
		
		bool includeBaseStats;
		if (includeStats && !ctx.Read(includeBaseStats))
			return;
		
		if (includeStats)
		{
			ExpansionSyncedPlayerStats playerStats();
			if (!playerStats.OnRecieve(ctx, includeRegisteredStats, includeBaseStats))
				return;

			playerStats.m_PlainID = playerID;

			m_StatsInvoker.Invoke(playerStats);
			m_ClientStats = playerStats;
		}
		
		if (includeStates)
		{
			ExpansionSyncedPlayerStates playerStates();
			if (!playerStates.OnRecieve(ctx))
				return;

			playerStates.m_PlainID = playerID;
			
			m_StatesInvoker.Invoke(playerStates);
			m_ClientStates = playerStates;
		}
	}
	
	// ------------------------------------------------------------
	// ExpansionMonitorModule RequestPlayerStates
	// Called from client
	// ------------------------------------------------------------
	void RequestPlayerStates(string playerID)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(ExpansionTracing.PLAYER_MONITOR, this, playerID);
#endif
		
		auto rpc = Expansion_CreateRPC("RPC_RequestPlayerStates");
		rpc.Write(playerID);
		//! @note guaranteed = false is intentional here (performance)
		rpc.Expansion_Send(false);
	}
	
	// ------------------------------------------------------------
	// ExpansionMonitorModule GetPlayerStatesByID
	// Called on server
	// ------------------------------------------------------------
	ExpansionSyncedPlayerStates GetPlayerStatesByID(string playerID)
	{
		ExpansionSyncedPlayerStates playerStates;
		m_States.Find(playerID, playerStates);
		return playerStates;
	}
	
	// ------------------------------------------------------------
	// ExpansionMonitorModule RequestPlayerStatsAndStates
	// Called from client
	// ------------------------------------------------------------
	void RequestPlayerStatsAndStates(string playerID, bool includeRegisteredStats = false, bool includeBaseStats = true)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(ExpansionTracing.PLAYER_MONITOR, this, playerID, "" + includeRegisteredStats, "" + includeBaseStats);
#endif
		
		auto rpc = Expansion_CreateRPC("RPC_RequestPlayerStatsAndStates");
		rpc.Write(playerID);
		rpc.Write(includeRegisteredStats);
		rpc.Write(includeBaseStats);
		//! @note guaranteed = false is intentional here (performance)
		rpc.Expansion_Send(false);
	}

	// ------------------------------------------------------------
	// ExpansionMonitorModule AddLastPlayerDeathPos
	// Called on server
	// ------------------------------------------------------------
	void AddLastPlayerDeathPos(string playerUID, vector pos)
	{
		if (m_PlayerDeathPositions.Get(playerUID))
			m_PlayerDeathPositions.Set(playerUID, pos);
		else
			m_PlayerDeathPositions.Insert(playerUID, pos);
	}
	
	// ------------------------------------------------------------
	// ExpansionMonitorModule GetLastDeathPosServer
	// Called on server
	// ------------------------------------------------------------
	vector GetLastDeathPosServer(string playerUID)
	{
		return m_PlayerDeathPositions.Get(playerUID);
	}
	
	// ------------------------------------------------------------
	// ExpansionMonitorModule SyncLastDeathPos
	// Called on server
	// ------------------------------------------------------------
	void SyncLastDeathPos(PlayerIdentity identity)
	{
		if (!GetGame().IsServer() && !GetGame().IsMultiplayer())
			return;

		if (!identity)
			return;
		
		vector pos = GetLastDeathPosServer(identity.GetId());
		if (pos == vector.Zero)
			return;
		
		auto rpc = Expansion_CreateRPC("RPC_SyncLastDeathPos");
		rpc.Write(pos);
		rpc.Expansion_Send(true, identity);
	}
	
	void SyncStatsToClient(PlayerBase player)
	{
		string playerID = player.GetIdentity().GetPlainId();

		auto playerStats = GetPlayerStatsByID(playerID);
		if (!playerStats)
			return;

		UpdateStats(playerStats, player, true, true, false);
	}

	// ------------------------------------------------------------
	// ExpansionMonitorModule RPC_SyncLastDeathPos
	// Called on client
	// ------------------------------------------------------------
	private void RPC_SyncLastDeathPos(PlayerIdentity ident, Object target, ParamsReadContext ctx)
	{
		if (!ctx.Read(m_LastDeathPos))
		{
			Error(ToString() + "::RPC_SyncLastDeathPos - Could not read last death position!");
		}
	}
	
	// ------------------------------------------------------------
	// ExpansionMonitorModule GetLastDeathPosClient
	// Called on client
	// ------------------------------------------------------------
	vector GetLastDeathPosClient()
	{
		return m_LastDeathPos;
	}
};
#endif
