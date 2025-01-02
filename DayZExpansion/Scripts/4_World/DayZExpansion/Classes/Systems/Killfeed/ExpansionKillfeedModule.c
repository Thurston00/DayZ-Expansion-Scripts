/**
 * ExpansionKillFeedModule.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

[CF_RegisterModule(ExpansionKillFeedModule)]
class ExpansionKillFeedModule: CF_ModuleWorld
{
	private string m_PlayerName;
	private string m_PlayerSteamWebhook;

	private string m_PlayerName2;
	private string m_PlayerSteamWebhook2;

	private PlayerBase m_Player;
	private PlayerBase m_SourcePlayer;
	private Object m_Source;
	private string m_SourceType;
	private PlayerStat<float> m_StatWater;
	private PlayerStat<float> m_StatEnergy;
	private float m_Blood;

	private bool m_HitCheckDone;

#ifdef JM_COT
	protected JMWebhookModule m_Webhook;
#endif

	string m_Expansion_SurvivorDisplayName;

	override void OnInit()
	{
		super.OnInit();

		Expansion_EnableRPCManager();
		Expansion_RegisterClientRPC("RPC_SendMessage");

#ifdef JM_COT
		CF_Modules<JMWebhookModule>.Get(m_Webhook);
#endif

		m_Expansion_SurvivorDisplayName = GetGame().ConfigGetTextOut(CFG_VEHICLESPATH + " SurvivorBase displayName");
	}

#ifdef JM_COT
	string GetWebhookTitle()
	{
		return "Killfeed Module";
	}

	void GetWebhookTypes( out array< string > types )
	{
		types.Insert( "Killfeed" );
	}

	string GetModuleName()
	{
		return ClassName();
	}

	void SendWebhook( string type, string message )
	{
		if ( !m_Webhook || IsMissionOffline() )
			return;

		auto msg = m_Webhook.CreateDiscordMessage();

		msg.GetEmbed().AddField( GetWebhookTitle(), message, false );

		m_Webhook.Post( GetModuleName() + type, msg );
	}

	void SendWebhook( string type, JMPlayerInstance player, string message )
	{
		#ifdef JM_COT_WEBHOOK_DEBUG
		Print( "+ExpansionKillFeedModule::SendWebhook() - Admin" );
		#endif
		if ( !m_Webhook || !player || IsMissionOffline() )
			return;

		auto msg = m_Webhook.CreateDiscordMessage( player, "Admin Account: " );

		msg.GetEmbed().AddField( GetWebhookTitle(), message, false );

		#ifdef JM_COT_WEBHOOK_DEBUG
		Print( "name=" + GetModuleName() + type );
		Print( "message=" + message );
		#endif

		m_Webhook.Post( GetModuleName() + type, msg );

		#ifdef JM_COT_WEBHOOK_DEBUG
		Print( "-ExpansionKillFeedModule::SendWebhook() - Admin" );
		#endif
	}

	void GetSubCommands(inout array<ref JMCommand> commands)
	{
	}

	void AddSubCommand(inout array<ref JMCommand> commands, string command, string function, string permission)
	{
		commands.Insert(new JMSubCommand(this, command, function, permission));
	}

	array<string> GetCommandNames()
	{
		return new array<string>();
	}
#endif

	void ResetKillfeed(PlayerBase player, Object source = null)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.KILLFEED, this, "" + player, "" + source);
#endif
		
		m_Player = player;
		m_PlayerName = GetIdentityName( player );

		#ifdef JM_COT
		m_PlayerSteamWebhook = player.FormatSteamWebhook();
		#endif

		m_PlayerName2 = "";
		m_PlayerSteamWebhook2 = "";
		m_SourcePlayer = null;
		m_Source = source;

		if (m_Source)
		{
			m_SourceType = m_Source.GetType();

			if (m_SourceType == "Expansion_C4_Explosion")
				m_SourceType = "ExpansionSatchel";
			else if (m_SourceType == "Expansion_RPG_Explosion")
				m_SourceType = "ExpansionRPG7";
			else if (m_SourceType == "Expansion_LAW_Explosion")
				m_SourceType = "ExpansionLAW";
			else if (m_SourceType == "Expansion_M203_HE_Explosion")
				m_SourceType = "Expansion_M79";
		}
		else
		{
			m_SourceType = "";
		}

		ResetHitCheckDone();
	}

	//! @note Event executed from playerbase EEHitBy function
	void OnPlayerHitBy(int damageType, PlayerBase player, EntityAI source, string ammo)
	{
		if ( !player || player.IsAlive() )
			return;

#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.KILLFEED, this, "damage type: " + damageType, "" + player, "" + source, ammo);
#endif
		
		switch ( damageType )
		{
			case DT_CUSTOM:
				if ( ammo == DayZPlayerImplementFallDamage.FALL_DAMAGE_AMMO_HEALTH )	//! Fall damage
				{
					ResetKillfeed(player, source);
					DoKillfeed_SetHitCheck(ExpansionKillFeedMessageType.FALL, "Human Skull", m_SourceType);
				}
				else if ( ammo == "TransportHit" )									//! Vehicle Crash
				{
					OnTransportHit(player, source);
				}
				else if ( ammo == "FireDamage" )									//! Fire damage (example: Fireplace)
				{
					ResetKillfeed(player, source);
					DoKillfeed_SetHitCheck(ExpansionKillFeedMessageType.FIRE, "Fireplace", m_SourceType);
				}
				else if ( source.GetType() == "AreaDamageBase" )					//! Area Hit
				{
					if ( !OnTransportHit(player, source) )							//! Check if hit came from a vehicle
					{
						ResetKillfeed(player, source);
						DoKillfeed_SetHitCheck(ExpansionKillFeedMessageType.AREA, "Human Skull", m_SourceType);				//! TODO
					}
				}
				else if ( ammo == "BarbedWireDamage" )								//! Barbed wire damage
				{
					ResetKillfeed(player, source);
					DoKillfeed_SetHitCheck(ExpansionKillFeedMessageType.BARBEDWIRE, "Human Skull", m_SourceType);
				}
				break;
			case DT_EXPLOSION:
				if( ammo == "ExpansionC4_Ammo" || (source && !source.IsWeapon()) )	//! Expansion Satchel or other non-weapon explosion
				{
					ResetKillfeed(player, source);
					DoKillfeed_SetHitCheck(ExpansionKillFeedMessageType.WEAPON_EXPLOSION, "Human Skull", m_SourceType);
				}
				break;
		}
	}

	void OnPlayerKilled( PlayerBase player, Object source )
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(true, this, "" + player, "" + source);
#endif 
		
		if ( !WasHitCheckDone() )
		{
			if ( !source )
			{
				OnDiedUnknown( player );  //! unknown death source
			}
			else if ( player == source )
			{
				if ( !OnPlayerSuicide(player) )
				{
					if ( !OnKilledByVehicleCrash(player) )
					{
						if ( !OnKilledByCondition(player) )
							OnDiedUnknown( player );
					}
				}
			}
			else if ( source.IsWeapon() || source.IsMeleeWeapon() )  //! player with weapon
			{	
				OnKilledByWeapon( player, source );
			}
			else if ( player != source && source.IsInherited( PlayerBase ) ) //! player with no weapon
			{			
				OnKilledByPlayer( player, source );
			}
			else if ( source.IsInherited( ZombieBase ) )
			{
				OnKilledByZombie( player, source );
			}
			else if ( source.IsInherited( AnimalBase ) )
			{	
				OnKilledByAnimal( player, source );
			}
			else
			{
				OnKilledByUnknown( player, source );
			}
		}
		else
		{
			ResetHitCheckDone();
		}
	}

	private bool OnTransportHit(PlayerBase player, EntityAI source)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.KILLFEED, this, "" + player, "" + source);
#endif 
		
		if ( player && !player.IsAlive() && source )
		{
			ExpansionVehicle vehicle;
			if ( ExpansionVehicle.Get(vehicle, source) )	//! Vehicle Hit
			{
				ResetKillfeed(player, source);

				m_SourcePlayer = PlayerBase.Cast( vehicle.CrewMember(DayZPlayerConstants.VEHICLESEAT_DRIVER) );

				if (m_SourcePlayer)
				{
					m_PlayerName2 = GetIdentityName( m_SourcePlayer );

					#ifdef JM_COT
					if ( m_SourcePlayer.GetIdentity() )
						m_PlayerSteamWebhook2 = m_SourcePlayer.FormatSteamWebhook();
					#endif
				}

				ExpansionKillFeedMessageType msgType;
				string icon;

				if ( vehicle.IsHelicopter() )	//! Vehicle Hit - Helicopter
				{
					if ( m_SourcePlayer )
						msgType = ExpansionKillFeedMessageType.HELI_HIT_DRIVER;
					else
						msgType = ExpansionKillFeedMessageType.HELI_HIT_NODRIVER;
					icon = "Helicopter";
				}
				else if ( vehicle.IsBoat() )	//! Vehicle Hit - Boat
				{
					if ( m_SourcePlayer )
						msgType = ExpansionKillFeedMessageType.BOAT_HIT_DRIVER;
					else
						msgType = ExpansionKillFeedMessageType.BOAT_HIT_NODRIVER;
					icon = "Boat";
				}
				else if ( vehicle.IsPlane() )	//! Vehicle Hit - Plane
				{
					if ( m_SourcePlayer )
						msgType = ExpansionKillFeedMessageType.PLANE_HIT_DRIVER;
					else
						msgType = ExpansionKillFeedMessageType.PLANE_HIT_NODRIVER;
					icon = "Plane";
				}
				else if ( vehicle.IsBike() )	//! Vehicle Hit - Bike
				{
					if ( m_SourcePlayer )
						msgType = ExpansionKillFeedMessageType.BIKE_HIT_DRIVER;
					else
						msgType = ExpansionKillFeedMessageType.BIKE_HIT_NODRIVER;
					icon = "Bike";
				}

				if (!msgType)	//! Vehicle Hit - Car
				{
					if ( m_SourcePlayer )
						msgType = ExpansionKillFeedMessageType.CAR_HIT_DRIVER;
					else
						msgType = ExpansionKillFeedMessageType.CAR_HIT_NODRIVER;
					icon = "Vehicle Crash";
				}

				DoKillfeed_SetHitCheck(msgType, icon, m_SourceType, m_PlayerName2);

				return true;
			}
		}

		return false;
	}

	private bool DoKillfeed_SetHitCheck(ExpansionKillFeedMessageType msgType, string icon = "Human Skull", string param1 = "", string param2 = "", string param3 = "")
	{
		m_HitCheckDone = DoKillfeed(msgType, icon, param1, param2, param3);
		return m_HitCheckDone;
	}

	private bool DoKillfeed(ExpansionKillFeedMessageType msgType, string icon = "Human Skull", string param1 = "", string param2 = "", string param3 = "")
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.KILLFEED, this, typename.EnumToString(ExpansionKillFeedMessageType, msgType), icon, param1, param2, param3);
#endif 
		
		if( !KillFeedCheckServerSettings(msgType) )
			return false;

		KillFeedMessage( msgType, icon, m_PlayerName, param1, param2, param3 );

		if (m_PlayerSteamWebhook2)
			DiscordMessage( msgType, m_PlayerSteamWebhook, m_PlayerSteamWebhook2, param1, param2);
		else
			DiscordMessage( msgType, m_PlayerSteamWebhook, param1, param2, param3);

		return true;
	}

	bool WasHitCheckDone()
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.KILLFEED, this, "" + m_HitCheckDone);
#endif 
		
		return m_HitCheckDone;
	}

	void ResetHitCheckDone()
	{
		m_HitCheckDone = false;

#ifdef EXTRACE
		EXTrace.Start(EXTrace.KILLFEED, this, "" + m_HitCheckDone);
#endif 
	}

	protected int CalcBlood(PlayerBase player)
	{
		return CalcPercent(player.GetHealth("", "Blood") - 2500,  2500);
	}

	protected int CalcPercent(float value, float max)
	{
		return (Math.Round(value) * 100) / max;
	}

	bool OnKilledByCondition(PlayerBase player)
	{
		m_StatWater = player.GetStatWater();
		m_StatEnergy = player.GetStatEnergy();
		m_Blood = CalcBlood(player);

#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.KILLFEED, this, "" + player, " water: " + m_StatWater.Get(), " food: " + m_StatEnergy.Get(), " blood: " + m_Blood, " health: " + player.GetHealth());
#endif 
		
		ResetKillfeed(player);

		//! Checks performed in order of potency/priority
		//! Note that cholera, food poisoning and salmonella cannot directly kill you,
		//! but the side effects (dehydration/starvation) can contribute if you don't drink/eat

		if (player.GetSingleAgentCount(eAgents.CHEMICAL_POISON) >= ContaminationStage2Mdfr.AGENT_THRESHOLD_ACTIVATE)
		{
			if (DoKillfeed(ExpansionKillFeedMessageType.STATUSEFFECT, "Gas Mask", "STR_EXPANSION_CHEMICAL_POISON"))
				return true;
		}

		if (player.GetSingleAgentCount(eAgents.WOUND_AGENT) >= WoundInfectStage2Mdfr.AGENT_THRESHOLD_ACTIVATE)
		{
			if (DoKillfeed(ExpansionKillFeedMessageType.STATUSEFFECT, "Drip", "STR_EXPANSION_WOUND_INFECTION"))
				return true;
		}

		if (m_Blood < 0.9)
		{
			if (player.GetModifiersManager().IsModifierActive(eModifiers.MDF_HEMOLYTIC_REACTION))
			{
				if (DoKillfeed(ExpansionKillFeedMessageType.STATUSEFFECT, "Drip", "STR_EXPANSION_HEMOLYTIC_REACTION"))
					return true;
			}

			if (DoKillfeed(ExpansionKillFeedMessageType.BLEEDING, "Drip"))
				return true;
		}

		if (player.GetSingleAgentCountNormalized(eAgents.CHOLERA) >= 0.7)
		{
			if (DoKillfeed(ExpansionKillFeedMessageType.STATUSEFFECT, "Human Skull", "STR_EXPANSION_CHOLERA"))
				return true;
		}

		if (player.GetSingleAgentCountNormalized(eAgents.FOOD_POISON) >= 0.7)
		{
			if (DoKillfeed(ExpansionKillFeedMessageType.STATUSEFFECT, "Human Skull", "STR_EXPANSION_FOOD_POISON"))
				return true;
		}

		if (player.GetSingleAgentCountNormalized(eAgents.SALMONELLA) >= 0.7)
		{
			if (DoKillfeed(ExpansionKillFeedMessageType.STATUSEFFECT, "Human Skull", "STR_EXPANSION_SALMONELLA"))
				return true;
		}

		if (m_StatWater.Get() < 0.9)
		{
			if (DoKillfeed(ExpansionKillFeedMessageType.DEHYDRATION, "Bottle"))
				return true;
		}

		if (m_StatEnergy.Get() < 0.9)
		{
			if (DoKillfeed(ExpansionKillFeedMessageType.STARVATION, "Apple Core"))
				return true;
		}

		return false;
	}

	bool OnKilledByVehicleCrash(PlayerBase player)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.KILLFEED, this, "" + player);
#endif 
		
		ExpansionVehicle vehicle;

		array<string> passenger_names;
		IEntity child;
		string formatted_names;
		ExpansionKillFeedMessageType msgType;
		string icon;

		//! Check if player died as driver in vehicle
		if ( ExpansionVehicle.Get( vehicle, player ) && GetGame().GetTickTime() - vehicle.GetCrewKilledTimestamp() < 0.1 )
		{
			Human driver = vehicle.CrewMember(DayZPlayerConstants.VEHICLESEAT_DRIVER);
			if (driver && driver != player)
				return true;  //! Only driver should trigger killfeed message (will include passengers)

			ResetKillfeed(player);

			m_Source = vehicle.GetEntity();
			m_SourceType = vehicle.GetType();

			passenger_names = new array<string>;

			//! Seated players
			Human crew;
			for (int i = 0; i < vehicle.CrewSize(); i++)
			{
				crew = vehicle.CrewMember(i);
				if (!crew || !crew.GetIdentity())
					continue;

				if (crew.GetIdentity().GetId() != player.GetIdentity().GetId())
					passenger_names.Insert( GetIdentityName( crew ) );
			}

			//! Attached players
			child = vehicle.GetEntity().GetChildren();
			while (child)
			{
				crew = Human.Cast(child);

				child = child.GetSibling();

				if (!crew || !crew.GetIdentity())
					continue;

				if (crew.GetIdentity().GetId() != player.GetIdentity().GetId())
				{
					string name = GetIdentityName( crew );
					if ( passenger_names.Find( name ) == -1 )
						passenger_names.Insert( name );
				}
			}

			if (passenger_names.Count() > 0)
			{
				foreach (string current_name: passenger_names)
				{
					if (formatted_names)
						formatted_names += ", ";
					formatted_names = current_name;
				}

				if ( vehicle.IsHelicopter() )
					msgType = ExpansionKillFeedMessageType.HELI_CRASH_CREW;
				else if ( vehicle.IsBoat() )
					msgType = ExpansionKillFeedMessageType.BOAT_CRASH_CREW;
				else
					msgType = ExpansionKillFeedMessageType.CAR_CRASH_CREW;
			}
			else
			{
				if ( vehicle.IsHelicopter() )
					msgType = ExpansionKillFeedMessageType.HELI_CRASH;
				else if ( vehicle.IsBoat() )
					msgType = ExpansionKillFeedMessageType.BOAT_CRASH;
				else
					msgType = ExpansionKillFeedMessageType.CAR_CRASH;
			}

			if ( vehicle.IsHelicopter() )
				icon = "Helicopter";
			else if ( vehicle.IsBoat() )
				icon = "Boat";
			else
				icon = "Vehicle Crash";

			DoKillfeed(msgType, icon, m_SourceType, formatted_names);

			return true;
		}

		return false;
	}

	bool OnPlayerSuicide(PlayerBase player)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.KILLFEED, this, "" + player);
#endif 
		
		//! Check if player suicided via emote
		if (!player.CommitedSuicide())
			return false;

		ResetKillfeed(player);

		//! Check if player had something in hands
		ItemBase item = player.m_Expansion_SuicideItem;
		if ( item )
		{
			string icon;
			if ( item.IsMeleeWeapon() )
				icon = "Knife";
			else if ( item.IsWeapon() )
				icon = "Gun";
			else
				icon = "Human Skull";

			DoKillfeed(ExpansionKillFeedMessageType.SUICIDE, icon, item.GetType());

			player.m_Expansion_SuicideItem = null;
		}
		else
		{
			//! Hmm
			DoKillfeed(ExpansionKillFeedMessageType.SUICIDE_NOITEM);
		}

		return true;
	}

	void OnKilledByWeapon(PlayerBase player, Object source)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.KILLFEED, this, "" + player, "" + source);
#endif 
		
		ResetKillfeed(player, source);

		m_SourcePlayer = PlayerBase.Cast( EntityAI.Cast( source ).GetHierarchyParent() );

		if (m_SourcePlayer)
		{
			m_PlayerName2 = GetIdentityName( m_SourcePlayer );
	
			#ifdef JM_COT
			if ( m_SourcePlayer.GetIdentity() )
				m_PlayerSteamWebhook2 = m_SourcePlayer.FormatSteamWebhook();
			#endif

			if( source.IsMeleeWeapon() )
			{
				DoKillfeed(ExpansionKillFeedMessageType.MELEEWEAPON, "Knife", m_PlayerName2, m_SourceType);
			}
			else
			{
				float distance = vector.Distance( player.GetPosition(), m_SourcePlayer.GetPosition() );
				DoKillfeed(ExpansionKillFeedMessageType.WEAPON, "Gun", m_PlayerName2, m_SourceType, Math.Round(distance).ToString());
			}
		}
		else if (source.IsInherited(ItemBase))
		{
			ItemBase item = ItemBase.Cast(source);
			if (item)
			{
				if( item.IsInherited(Grenade_Base) )
				{
					DoKillfeed(ExpansionKillFeedMessageType.WEAPON_EXPLOSION, "Grenade", m_SourceType);
				}
			}
		}
	}

	void OnKilledByPlayer(PlayerBase player, Object source)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.KILLFEED, this, "" + player, "" + source);
#endif 
		
		ResetKillfeed(player, source);

		m_SourcePlayer = PlayerBase.Cast(EntityAI.Cast(source));

		if (m_SourcePlayer)
		{
			m_PlayerName2 = GetIdentityName( m_SourcePlayer );

			#ifdef JM_COT
			if ( m_SourcePlayer.GetIdentity() )
				m_PlayerSteamWebhook2 = m_SourcePlayer.FormatSteamWebhook();
			#endif

			if (m_PlayerName2 != "" || m_PlayerSteamWebhook2 != "") //! Got player name
			{
				DoKillfeed(ExpansionKillFeedMessageType.BAREHANDS, "Grab", m_PlayerName2);
			}
		}
	}

	void OnKilledByZombie(PlayerBase player, Object source)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.KILLFEED, this, "" + player, "" + source);
#endif 
		
		ZombieBase zombie = ZombieBase.Cast(EntityAI.Cast(source));
		if (zombie)
		{
			ResetKillfeed(player, source);

			DoKillfeed(ExpansionKillFeedMessageType.INFECTED, "Infected 2");
		}
	}

	void OnKilledByAnimal(PlayerBase player, Object source)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.KILLFEED, this, "" + player, "" + source);
#endif 
		
		AnimalBase animal = AnimalBase.Cast(EntityAI.Cast(source));
		if (animal)
		{
			ResetKillfeed(player, source);

			string icon;
			if ( animal.IsKindOf( "Animal_UrsusArctos" ) )
				icon = "Bear";
			else
				icon = "Claw";

			DoKillfeed(ExpansionKillFeedMessageType.ANIMAL, icon, m_SourceType);
		}
	}

	void OnKilledByUnknown(PlayerBase player, Object source)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.KILLFEED, this, "" + player, "" + source);
#endif 
		
		ResetKillfeed(player, source);

		DoKillfeed(ExpansionKillFeedMessageType.KILLED_UNKNOWN, "Human Skull", m_SourceType);
	}

	void OnDiedUnknown(PlayerBase player)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.KILLFEED, this, "" + player);
#endif
		
		ResetKillfeed(player);

		DoKillfeed(ExpansionKillFeedMessageType.DIED_UNKNOWN, "Human Skull");
	}

	//! @note Called on Server
	private void KillFeedMessage( ExpansionKillFeedMessageType type, string icon, string param1 = "", string param2 = "", string param3 = "", string param4 = "")
	{
		if ( GetGame().IsServer() )
		{
			ExpansionKillFeedMessageMetaData kill_data = new ExpansionKillFeedMessageMetaData(type, icon, param1, param2, param3, param4);

			auto message_rpc = Expansion_CreateRPC("RPC_SendMessage");
			message_rpc.Write( kill_data );
			message_rpc.Expansion_Send(true);

			ExpansionLogKillfeed(kill_data);
		}
	}

	private bool KillFeedCheckServerSettings( ExpansionKillFeedMessageType type )
	{
		switch ( type )
		{
			case ExpansionKillFeedMessageType.FALL:
				return GetExpansionSettings().GetNotification().KillFeedFall;
			case ExpansionKillFeedMessageType.CAR_HIT_DRIVER:
				return GetExpansionSettings().GetNotification().KillFeedCarHitDriver;
			case ExpansionKillFeedMessageType.CAR_HIT_NODRIVER:
				return GetExpansionSettings().GetNotification().KillFeedCarHitNoDriver;
			case ExpansionKillFeedMessageType.CAR_CRASH:
				return GetExpansionSettings().GetNotification().KillFeedCarCrash;
			case ExpansionKillFeedMessageType.CAR_CRASH_CREW:
				return GetExpansionSettings().GetNotification().KillFeedCarCrashCrew;
			case ExpansionKillFeedMessageType.HELI_HIT_DRIVER:
				return GetExpansionSettings().GetNotification().KillFeedHeliHitDriver;
			case ExpansionKillFeedMessageType.HELI_HIT_NODRIVER:
				return GetExpansionSettings().GetNotification().KillFeedHeliHitNoDriver;
			case ExpansionKillFeedMessageType.HELI_CRASH:
				return GetExpansionSettings().GetNotification().KillFeedHeliCrash;
			case ExpansionKillFeedMessageType.HELI_CRASH_CREW:
				return GetExpansionSettings().GetNotification().KillFeedHeliCrashCrew;
			case ExpansionKillFeedMessageType.BOAT_HIT_DRIVER:
				return GetExpansionSettings().GetNotification().KillFeedBoatHitDriver;
			case ExpansionKillFeedMessageType.BOAT_HIT_NODRIVER:
				return GetExpansionSettings().GetNotification().KillFeedBoatHitNoDriver;
			case ExpansionKillFeedMessageType.BOAT_CRASH:
				return GetExpansionSettings().GetNotification().KillFeedBoatCrash;
			case ExpansionKillFeedMessageType.BOAT_CRASH_CREW:
				return GetExpansionSettings().GetNotification().KillFeedBoatCrashCrew;
			/*case ExpansionKillFeedMessageType.PLANE_HIT_DRIVER:
				return GetExpansionSettings().GetNotification().KillFeedPlaneHitDriver;
			case ExpansionKillFeedMessageType.PLANE_HIT_NODRIVER:
				return GetExpansionSettings().GetNotification().KillFeedPlaneHitNoDriver;
			case ExpansionKillFeedMessageType.BIKE_HIT_DRIVER:
				return GetExpansionSettings().GetNotification().KillFeedBikeHitDriver;
			case ExpansionKillFeedMessageType.BIKE_HIT_NODRIVER:
				return GetExpansionSettings().GetNotification().KillFeedBikeHitNoDriver;*/
			case ExpansionKillFeedMessageType.BARBEDWIRE:
				return GetExpansionSettings().GetNotification().KillFeedBarbedWire;
			case ExpansionKillFeedMessageType.FIRE:
				return GetExpansionSettings().GetNotification().KillFeedFire;
			/*case ExpansionKillFeedMessageType.SPECIAL_EXPLOSION:
				return GetExpansionSettings().GetNotification().KillFeedSpecialExplosion;*/
			case ExpansionKillFeedMessageType.WEAPON_EXPLOSION:
				return GetExpansionSettings().GetNotification().KillFeedWeaponExplosion;
			case ExpansionKillFeedMessageType.DEHYDRATION:
				return GetExpansionSettings().GetNotification().KillFeedDehydration;
			case ExpansionKillFeedMessageType.STARVATION:
				return GetExpansionSettings().GetNotification().KillFeedStarvation;
			case ExpansionKillFeedMessageType.BLEEDING:
				return GetExpansionSettings().GetNotification().KillFeedBleeding;
			case ExpansionKillFeedMessageType.STATUSEFFECT:
				return GetExpansionSettings().GetNotification().KillFeedStatusEffects;
			case ExpansionKillFeedMessageType.SUICIDE:
			case ExpansionKillFeedMessageType.SUICIDE_NOITEM:
				return GetExpansionSettings().GetNotification().KillFeedSuicide;
			case ExpansionKillFeedMessageType.WEAPON:
				return GetExpansionSettings().GetNotification().KillFeedWeapon;
			case ExpansionKillFeedMessageType.MELEEWEAPON:
				return GetExpansionSettings().GetNotification().KillFeedMeleeWeapon;
			case ExpansionKillFeedMessageType.BAREHANDS:
				return GetExpansionSettings().GetNotification().KillFeedBarehands;
			case ExpansionKillFeedMessageType.INFECTED:
				return GetExpansionSettings().GetNotification().KillFeedInfected;
			case ExpansionKillFeedMessageType.ANIMAL:
				return GetExpansionSettings().GetNotification().KillFeedAnimal;
			case ExpansionKillFeedMessageType.KILLED_UNKNOWN:
				return GetExpansionSettings().GetNotification().KillFeedKilledUnknown;
			case ExpansionKillFeedMessageType.DIED_UNKNOWN:
				return GetExpansionSettings().GetNotification().KillFeedDiedUnknown;
		}

		return false;
	}

	private void DiscordMessage(ExpansionKillFeedMessageType type, string param1 = "", string param2 = "", string param3 = "", string param4 = "")
	{
#ifdef JM_COT
		if ( GetExpansionSettings().GetNotification().EnableKillFeedDiscordMsg )
		{
			JMWebhookDiscordMessage discord_message = m_Webhook.CreateDiscordMessage();
	   		JMWebhookDiscordEmbed discord_embed = discord_message.GetEmbed();

			string message = GetKillFeedMessage(type);

			ExpansionKillFeedMessageMetaData kill_data = new ExpansionKillFeedMessageMetaData(type, "", param1, param2, param3, param4);
			StringLocaliser loc = GetLocaliser(kill_data);

			discord_embed.SetColor( 16711680 ); // 0xFF0000
			discord_embed.SetAuthor( "DayZ Expansion", "https://steamcommunity.com/sharedfiles/filedetails/?id=2116151222", "https://cdn.cloudflare.steamstatic.com/steamcommunity/public/images/avatars/98/980723d8611aa9b8b71eca92e5f911167489a785_full.jpg" );
			discord_embed.AddField( "Kill-Feed", loc.Format() );

			m_Webhook.Post( "Killfeed", discord_message );
		}
#endif
	}

	static string GetKillFeedMessage(ExpansionKillFeedMessageType type)
	{
		string message;
		switch ( type )
		{
			case ExpansionKillFeedMessageType.FALL:
				message = "STR_EXPANSION_KILLFEED_PLAYER_DIED_FALLING";
				break;
			case ExpansionKillFeedMessageType.CAR_HIT_DRIVER:
				message = "STR_EXPANSION_KILLFEED_PLAYER_KILLED_CRASH_CAR";
				break;
			case ExpansionKillFeedMessageType.CAR_HIT_NODRIVER:
				message = "STR_EXPANSION_KILLFEED_PLAYER_KILLED_CRASH_CAR_NODRIVER";
				break;
			case ExpansionKillFeedMessageType.CAR_CRASH:
				message = "STR_EXPANSION_KILLFEED_PLAYER_CRASHED_CAR";
				break;
			case ExpansionKillFeedMessageType.CAR_CRASH_CREW:
				message = "STR_EXPANSION_KILLFEED_PLAYER_CRASHED_CAR_CREW";
				break;
			case ExpansionKillFeedMessageType.HELI_HIT_DRIVER:
				message = "STR_EXPANSION_KILLFEED_PLAYER_KILLED_CRASH_HELI";
				break;
			case ExpansionKillFeedMessageType.HELI_HIT_NODRIVER:
				message = "STR_EXPANSION_KILLFEED_PLAYER_KILLED_CRASH_HELI_NODRIVER";
				break;
			case ExpansionKillFeedMessageType.HELI_CRASH:
				message = "STR_EXPANSION_KILLFEED_PLAYER_CRASHED_HELI";
				break;
			case ExpansionKillFeedMessageType.HELI_CRASH_CREW:
				message = "STR_EXPANSION_KILLFEED_PLAYER_CRASHED_HELI_CREW";
				break;
			case ExpansionKillFeedMessageType.BOAT_HIT_DRIVER:
				message = "STR_EXPANSION_KILLFEED_PLAYER_KILLED_CRASH_CAR";
				break;
			case ExpansionKillFeedMessageType.BOAT_HIT_NODRIVER:
				message = "STR_EXPANSION_KILLFEED_PLAYER_KILLED_CRASH_CAR_NODRIVER";
				break;
			case ExpansionKillFeedMessageType.BOAT_CRASH:
				message = "STR_EXPANSION_KILLFEED_PLAYER_CRASHED_CAR";
				break;
			case ExpansionKillFeedMessageType.BOAT_CRASH_CREW:
				message = "STR_EXPANSION_KILLFEED_PLAYER_CRASHED_CAR_CREW";
				break;
			case ExpansionKillFeedMessageType.PLANE_HIT_DRIVER:
				message = "STR_EXPANSION_KILLFEED_PLAYER_KILLED_CRASH_PLANE";
				break;
			case ExpansionKillFeedMessageType.PLANE_HIT_NODRIVER:
				message = "STR_EXPANSION_KILLFEED_PLAYER_CRASHED_PLANE";
				break;
			case ExpansionKillFeedMessageType.BIKE_HIT_DRIVER:
				message = "STR_EXPANSION_KILLFEED_PLAYER_KILLED_CRASH_BIKE";
				break;
			case ExpansionKillFeedMessageType.BIKE_HIT_NODRIVER:
				message = "STR_EXPANSION_KILLFEED_PLAYER_CRASHED_BIKE";
				break;
			case ExpansionKillFeedMessageType.BARBEDWIRE:
				message = "STR_EXPANSION_KILLFEED_PLAYER_BARBED_WIRE";
				break;
			case ExpansionKillFeedMessageType.FIRE:
				message = "STR_EXPANSION_KILLFEED_PLAYER_DIED_FIRE";
				break;
			case ExpansionKillFeedMessageType.SPECIAL_EXPLOSION:
				message = "STR_EXPANSION_KILLFEED_PLAYER_DIED_EXPLOSION_WEAPON";
				break;
			case ExpansionKillFeedMessageType.WEAPON_EXPLOSION:
				message = "STR_EXPANSION_KILLFEED_PLAYER_DIED_EXPLOSION";
				break;
			case ExpansionKillFeedMessageType.DEHYDRATION:
				message = "STR_EXPANSION_KILLFEED_PLAYER_DIED_DEHYDRATION";
				break;
			case ExpansionKillFeedMessageType.STARVATION:
				message = "STR_EXPANSION_KILLFEED_PLAYER_DIED_STARVATION";
				break;
			case ExpansionKillFeedMessageType.BLEEDING:
				message = "STR_EXPANSION_KILLFEED_PLAYER_DIED_BLEEDING";
				break;
			case ExpansionKillFeedMessageType.STATUSEFFECT:
				message = "STR_EXPANSION_KILLFEED_PLAYER_DIED_STATUSEFFECT";
				break;
			case ExpansionKillFeedMessageType.SUICIDE:
				message = "STR_EXPANSION_KILLFEED_PLAYER_DIED_SUICIDE";
				break;
			case ExpansionKillFeedMessageType.SUICIDE_NOITEM:
				message = "STR_EXPANSION_KILLFEED_PLAYER_DIED_SUICIDE_NOITEM";
				break;
			case ExpansionKillFeedMessageType.WEAPON:
				message = "STR_EXPANSION_KILLFEED_PLAYER_KILLED_PLAYER_WEAPON";
				break;
			case ExpansionKillFeedMessageType.WEAPON_NODISTANCE:
				message = "STR_EXPANSION_KILLFEED_PLAYER_KILLED_PLAYER_WEAPON_NODISTANCE";
				break;
			case ExpansionKillFeedMessageType.MELEEWEAPON:
				message = "STR_EXPANSION_KILLFEED_PLAYER_KILLED_PLAYER_MELEE";
				break;
			case ExpansionKillFeedMessageType.BAREHANDS:
				message = "STR_EXPANSION_KILLFEED_PLAYER_KILLED_PLAYE_BAREHANDS";
				break;
			case ExpansionKillFeedMessageType.INFECTED:
				message = "STR_EXPANSION_KILLFEED_PLAYER_KILLED_ZOMBIE";
				break;
			case ExpansionKillFeedMessageType.ANIMAL:
				message = "STR_EXPANSION_KILLFEED_PLAYER_KILLED_ANIMAL";
				break;
			case ExpansionKillFeedMessageType.KILLED_UNKNOWN:
				message = "STR_EXPANSION_KILLFEED_PLAYER_KILLED_UNKNOWN";
				break;
			case ExpansionKillFeedMessageType.DIED_UNKNOWN:
				message = "STR_EXPANSION_KILLFEED_PLAYER_DIED_UNKNOWN";
				break;
		}

		return message;
	}

	string GetIdentityName( Man player )
	{
		PlayerIdentity identity = player.GetIdentity();

		if ( identity == NULL )
		{
			string displayName = player.GetDisplayName();
			string name = displayName;
		#ifdef EXPANSIONMODAI
			eAIBase ai;
			if (Class.CastTo(ai, player) && (name == m_Expansion_SurvivorDisplayName || name == string.Empty))
			{
				name = player.GetType();
				int index = ExpansionString.LastIndexOf(name, "_");
				if (index > -1)
					displayName = name.Substring(index + 1, name.Length() - index - 1);

				name = ai.GetGroup().GetName();
				if (name == string.Empty)
					name = string.Format("AI %1 (%2)", displayName, ai.GetGroup().GetFaction().GetDisplayName());
				else
					name = string.Format("AI %1 (%2)", displayName, name);
			}
		#endif
			return name;
		}
	#ifdef EXPANSIONMODGROUPS
		if (GetExpansionSettings().GetParty().DisplayPartyTag)
		{
			PlayerBase pb;
			if (Class.CastTo(pb, player))
			{
				ExpansionPartyData partyData = pb.Expansion_GetParty();
				if (partyData)
				{
					if ( partyData.GetPartyTag() != string.Empty )
						return string.Format("%1%2", partyData.GetPartyTagFormatted(), identity.GetName());
				}
			}
		}
	#endif

		return identity.GetName();
	}

	//! @note Called on all Clients
	private void RPC_SendMessage(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
		ExpansionKillFeedMessageMetaData kill_data = new ExpansionKillFeedMessageMetaData( ExpansionKillFeedMessageType.UNKNOWN, "" );
		ctx.Read(kill_data);

		if (kill_data)
		{
#ifdef EXTRACE
			auto trace = EXTrace.Start(EXTrace.KILLFEED, this, kill_data.Message, kill_data.Icon, kill_data.FeedParam1, kill_data.FeedParam2, kill_data.FeedParam3, kill_data.FeedParam4);
#endif 
			
			StringLocaliser loc = GetLocaliser(kill_data);
			if ( GetExpansionSettings().GetNotification(false).KillFeedMessageType == ExpansionAnnouncementType.NOTIFICATION )
			{
				ExpansionNotification("STR_EXPANSION_KILLFEED_TITLE", loc, ExpansionIcons.GetPath(kill_data.Icon), COLOR_EXPANSION_NOTIFICATION_EXPANSION, 5, ExpansionNotificationType.KILLFEED).Create();
			}
			else if ( GetExpansionSettings().GetNotification(false).KillFeedMessageType == ExpansionAnnouncementType.CHAT )
			{
				GetGame().GetMission().OnEvent( ChatMessageEventTypeID, new ChatMessageEventParams( ExpansionChatChannels.CCSystem, "", "#STR_EXPANSION_KILLFEED_TITLE" + " - " + loc.Format(), "" ) );
			}
		}
	}

	private StringLocaliser GetLocaliser(ExpansionKillFeedMessageMetaData kill_data)
	{
		if (!kill_data)
			return NULL;

		string displayName1 = ExpansionStatic.GetItemDisplayNameWithType(kill_data.FeedParam1);
		string displayName2 = ExpansionStatic.GetItemDisplayNameWithType(kill_data.FeedParam2);
		string displayName3 = ExpansionStatic.GetItemDisplayNameWithType(kill_data.FeedParam3);
		string displayName4 = ExpansionStatic.GetItemDisplayNameWithType(kill_data.FeedParam4);

		auto loc = new StringLocaliser(kill_data.Message);
		loc.Set(0, displayName1);
		loc.Set(1, displayName2);
		loc.Set(2, displayName3);
		loc.Set(3, displayName4);

		return loc;
	}

	private void ExpansionLogKillfeed(ExpansionKillFeedMessageMetaData kill_data)
	{
		if (!kill_data)
			return;

		if (GetExpansionSettings().GetLog().Killfeed)
		{
			StringLocaliser loc = GetLocaliser(kill_data);
			string param;
			for (int i = 0; i < 4; i++)
			{
				param = loc.Get(i);
				param.ToUpper();
				if (param.Contains("#STR_"))
					loc.SetTranslates(i, true);
			}
			GetExpansionSettings().GetLog().PrintLog("[Killfeed] " + loc.Format());
		}
	}
}
