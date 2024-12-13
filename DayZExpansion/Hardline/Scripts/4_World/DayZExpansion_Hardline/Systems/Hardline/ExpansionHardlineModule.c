/**
 * ExpansionHardlineModule.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2023 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

[CF_RegisterModule(ExpansionHardlineModule)]
class ExpansionHardlineModule: CF_ModuleWorld
{
	protected static ExpansionHardlineModule s_Instance;
	static int s_ReceiveFactionReputation_RPCID;
	
	protected ref map<string, ref ExpansionHardlinePlayerData> m_HardlinePlayerData = new map<string, ref ExpansionHardlinePlayerData>; //! Server
	
	void ExpansionHardlineModule()
	{
		s_Instance = this;
	}
	
	override void OnInit()
	{
		super.OnInit();

/**
 * Order of player-related mission events
 * 
 * On connect (new character):
 * 1) OnClientPrepare
 * 3) OnClientNew (calls CreateCharacter/EquipCharacter in vanilla)
 * 3) OnInvokeConnect
 * 
 * On connect (existing character):
 * 1) OnClientPrepare
 * 2) OnClientReady
 * 3) OnInvokeConnect
 * 
 * On respawn:
 * 1) OnClientRespawn
 * 2) OnClientPrepare
 * 3) OnClientNew (calls CreateCharacter/EquipCharacter in vanilla)
 * 4) OnInvokeConnect
 * 
 * On logout:
 * 1) OnClientLogout
 * 2) OnClientLogoutCancelled (if player cancels logout countdown)
 * 
 * On disconnect (if logout countdown runs out or player early disconnects):
 * 1) OnClientDisconnect
 * 2) OnInvokeDisconnect (only if player character not yet deleted)
 */

		EnableClientPrepare();
		EnableInvokeConnect();
		EnableClientDisconnect();
		Expansion_EnableRPCManager();
		s_ReceiveFactionReputation_RPCID = Expansion_RegisterClientRPC("RPC_ReceiveFactionReputation");
	}
	
	//! Client
	protected void RPC_ReceiveFactionReputation(PlayerIdentity identity, Object target, ParamsReadContext ctx)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.HARDLINE, this);
#endif
		
		PlayerBase clientPB = PlayerBase.Cast(GetGame().GetPlayer());
		if (!clientPB)
		{
			Error(ToString() + "::RPC_ReceiveFactionReputation - Could not get player!");
			return;
		}

		if (!clientPB.m_Expansion_HardlineData)
			clientPB.m_Expansion_HardlineData = new ExpansionHardlinePlayerData;
		else
			clientPB.m_Expansion_HardlineData.FactionReputation.Clear();

		if (!clientPB.m_Expansion_HardlineData.ReadFactionReputation(ctx))
		{
			Error(ToString() + "::RPC_ReceiveFactionReputation - Could not read faction reputation data!");
			return;
		}
	}
	
	override void OnClientPrepare(Class sender, CF_EventArgs args)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.HARDLINE, this);
#endif

		auto cArgs = CF_EventPlayerPrepareArgs.Cast(args);

		if (GetExpansionSettings().GetHardline().UseReputation)
			LoadPlayerData(cArgs.Identity);
	}

	override void OnInvokeConnect(Class sender, CF_EventArgs args)
	{
	#ifdef EXTRACE_DIAG
		auto trace = EXTrace.Start(ExpansionTracing.HARDLINE, this);
	#endif

		super.OnInvokeConnect(sender, args);

		auto cArgs = CF_EventPlayerArgs.Cast(args);
		
	#ifdef EXTRACE_DIAG
		EXTrace.Add(trace, cArgs.Player);
		EXTrace.Add(trace, cArgs.Identity);
	#endif

		if (GetExpansionSettings().GetHardline().UseReputation)
			SetupClientData(cArgs.Player, cArgs.Identity);
	}

	override void OnClientDisconnect(Class sender, CF_EventArgs args)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.HARDLINE, this);
#endif

		auto cArgs = CF_EventPlayerDisconnectedArgs.Cast(args);

		//! Just to conserve some memory, not needed otherwise
		m_HardlinePlayerData.Remove(cArgs.UID);
	}

	//! Load player data early so it's available in StartingEquipSetup
	protected void LoadPlayerData(PlayerIdentity identity)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.HARDLINE, this);
#endif 
		
		//! Check if hardline player data file exists and load it
		string playerUID = identity.GetId();
		auto data = m_HardlinePlayerData[playerUID];

		if (!data)
		{
			data = new ExpansionHardlinePlayerData;

			if (data.Load(playerUID))
			{
				EXPrint(ToString() + " ::LoadPlayerData - Loaded player hardline data for player " + identity.GetName() + "[" + playerUID + "]");
			#ifdef EXPANSIONMODAI
				if (data.FactionID != -1 && !GetExpansionSettings().GetHardline().EnableFactionPersistence)
				{
					data.FactionID = -1;
					data.Save(playerUID);
				}
			#endif
			}
	
			m_HardlinePlayerData[playerUID] = data;
		}
	}

	//! For use in init.c StartingEquipSetup (e.g. for reputation or faction specific starting gear) which runs before OnInvokeConnect
	ExpansionHardlinePlayerData GetPlayerData(PlayerBase player)
	{
		PlayerIdentity identity = player.GetIdentity();
		if (!identity)
			return null;

		string playerUID = identity.GetId();
		return m_HardlinePlayerData[playerUID];
	}

	protected void SetupClientData(PlayerBase player, PlayerIdentity identity)
	{
	#ifdef EXTRACE_DIAG
		auto trace = EXTrace.Start(ExpansionTracing.HARDLINE, this);
		EXTrace.Add(trace, player);
		EXTrace.Add(trace, identity);
	#endif

		string playerUID = identity.GetId();
		player.Expansion_SetHardlineData(m_HardlinePlayerData[playerUID]);
	}
	
	void OnEntityKilled(EntityAI victim, Object killer)
	{
	#ifdef EXTRACE_DIAG
		auto trace = EXTrace.Start(ExpansionTracing.HARDLINE, this);
		EXTrace.Add(trace, victim);
		EXTrace.Add(trace, killer);
	#endif
		
		EntityAI killerEntity;
		if (!Class.CastTo(killerEntity, killer))
			return;

		PlayerBase killerPlayer;
		PlayerBase victimPlayer;
		
		bool killerIsPlayer = false;
		bool killerIsAI = false;
		bool victimIsPlayer = false;
		bool victimIsAI = false;
		
		//! Set killer entity
		if (Class.CastTo(killerPlayer, killerEntity.GetHierarchyRootPlayer()))
		{
		#ifdef EXPANSIONMODAI
			if (killerPlayer.IsAI())
				killerIsAI = true;
			else
		#endif
			killerIsPlayer = true;
		}

		//! Set victim entity
		if (Class.CastTo(victimPlayer, victim.GetHierarchyRootPlayer()))
		{
		#ifdef EXPANSIONMODAI
			if (victimPlayer.IsAI())
				victimIsAI = true;
			else
		#endif
			victimIsPlayer = true;
		}
		
		if (!victimPlayer && !killerPlayer)
			return;

		//! oof, this could use some optimization to get rid of the 2nd Is() check...
		string match;
		int reputation;
		foreach (string cls, int rep: GetExpansionSettings().GetHardline().EntityReputation)
		{
		    if (ExpansionStatic.Is(victim, cls) && (!match || ExpansionStatic.Is(cls, match)))
		    {
				match = cls;
		        reputation = rep;
		    }
		}

		if (reputation == 0)
			return;

		//! Killer was a player (not AI) and not a suicide
		if (killerIsPlayer && killerPlayer != victimPlayer)
		{
			if (victimIsPlayer || victimIsAI)
			{
				HandlePlayerKilledPlayer(killerPlayer, victimPlayer, reputation, victimIsAI);
			}
			else
			{
				HandlePlayerKilledEntity(killerPlayer, victim, reputation);
			}
		}
	#ifdef EXPANSIONMODAI
		//! Killer was AI and victim was a player
		else if (victimIsPlayer && killerIsAI) 
		{
			HandlePlayerKilledByAI(victimPlayer, killerPlayer, reputation);
		}
	#endif
		//! Victim was a player that committed suicide or other indirectly inflicted death (e.g. fall damage, explosion...)
		else if (victimIsPlayer)
		{
			HandlePlayerDeath(victimPlayer);
		}
	}
	
	protected void HandlePlayerKilledEntity(PlayerBase killer, EntityAI victim, int reputation)
	{		
#ifdef EXTRACE
		auto trace = EXTrace.Start(ExpansionTracing.HARDLINE, this);
#endif 
		
		bool isFriendly;
	#ifdef EXPANSIONMODAI
		  //! If the entity was friendly to the killer the killer will lose reputation
        if (killer.GetGroup() && killer.GetGroup().GetFaction().IsFriendlyEntity(victim))
			isFriendly = true;
	#endif
		if (isFriendly)
			reputation = -reputation;

		killer.Expansion_AddReputation(reputation);
		MissionBaseWorld missionWorld = MissionBaseWorld.Cast(GetGame().GetMission());
		if (missionWorld)
		{
			missionWorld.Expansion_OnPlayerKilledEntity(killer, victim, reputation);
		}
	}
	
	protected void HandlePlayerKilledPlayer(PlayerBase killer, PlayerBase victim, int reputation, bool victimIsAI = false)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(ExpansionTracing.HARDLINE, this);
#endif 
			
		bool isFriendly = false;
		
	#ifdef EXPANSIONMODGROUPS
		//! Check if killer and victim are in the same party/group.
		if (killer.GetIdentity() && victim.GetIdentity() && killer.Expansion_GetPartyID() == victim.Expansion_GetPartyID())
			isFriendly = true;
	#endif

	#ifdef EXPANSIONMODAI
		//! Check if killer player or AI are in a faction.
		if (victim.GetGroup() && !isFriendly)
		{
			if (victim.GetGroup() == killer.GetGroup())
			{
				isFriendly = true;
			}
			else if (killer.GetGroup())
			{
				isFriendly = victim.GetGroup().GetFaction().IsFriendly(killer.GetGroup().GetFaction());
			}
			else
			{
				isFriendly = victim.GetGroup().GetFaction().IsFriendlyEntity(killer);
			}
		}
	#endif
		
	#ifdef DIAG_DEVELOPER
		ModulePrint(ToString() + "::HandlePlayerKilledPlayer - Are killer and victim friendly to each other? " + isFriendly);
	#endif
		
		//! If player killed a friendly player because they share the same faction or same party then we remove instead of add the reputation.
		if (isFriendly)
			reputation = -reputation;
		
		killer.Expansion_AddReputation(reputation);
		MissionBaseWorld missionWorld = MissionBaseWorld.Cast(GetGame().GetMission());
		if (missionWorld)
		{
			missionWorld.Expansion_OnPlayerKilledPlayer(killer, victim, reputation, victimIsAI);
		}
	}
		
	protected void HandlePlayerDeath(PlayerBase victim)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(ExpansionTracing.HARDLINE, this);
#endif 
		
		int repToRemove = GetExpansionSettings().GetHardline().ReputationLossOnDeath;
		
		if (!repToRemove)
			return;
		
		victim.Expansion_DecreaseReputation(repToRemove);
		MissionBaseWorld missionWorld = MissionBaseWorld.Cast(GetGame().GetMission());
		if (missionWorld)
		{
			missionWorld.Expansion_OnPlayerDeath(victim, repToRemove);
		}
	}
	
#ifdef EXPANSIONMODAI
	protected void HandlePlayerKilledByAI(PlayerBase victim, PlayerBase killer, int reputation)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(ExpansionTracing.HARDLINE, this);
#endif 
		
		HandlePlayerDeath(victim);
		MissionBaseWorld missionWorld = MissionBaseWorld.Cast(GetGame().GetMission());
		if (missionWorld)
		{
			missionWorld.Expansion_OnPlayerKilledAI(killer, victim, reputation);
		}
	}
#endif

	void OnPlayerAction(PlayerBase player, int reputation)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(ExpansionTracing.HARDLINE, this);
#endif 
		
		if (reputation == 0)
			return;
		
	#ifdef EXPANSIONMODAI
		if (player.IsAI())
			return;
	#endif

		player.Expansion_AddReputation(reputation);
	}
	
	static ExpansionHardlineModule GetModuleInstance()
	{
		return s_Instance;
	}
	
	static void ModulePrint(string text)
	{
	#ifdef DIAG_DEVELOPER
		EXTrace.Print(EXTrace.HARDLINE, s_Instance, text);
	#endif
	}
 };