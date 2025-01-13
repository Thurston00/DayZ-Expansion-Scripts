/**
 * ExpansionLogSettings.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

/**@class		ExpansionLogSettings
 * @brief		Log settings class
 **/
class ExpansionLogSettings: ExpansionSettingBase
{
	static const int VERSION = 8;

	bool Safezone;				//! If enabled, generate logs when the player leave or enter a safezone
	bool AdminTools;			//! If enabled, generate logs of the adminhammer and expansionadmincarkey when used
	bool ExplosionDamageSystem;			//! If enabled, generate logs of the Expansion explosion damage system when used

	bool VehicleCarKey;			//! If enabled, generate logs about pairing, unpairing, locking, unlocking vehicles with car keys
	bool VehicleTowing;			//! If enabled, generate logs about towing and untowing vehicles
	bool VehicleLockPicking;	//! If enabled, generate logs about lockpicking a vehicle
	bool VehicleDestroyed; 		//! Print a log when a vehicle is destroyed
	bool VehicleAttachments;
	bool VehicleEnter;
	bool VehicleLeave;
	bool VehicleDeleted;
	bool VehicleEngine;

	bool BaseBuildingRaiding;	//! If enabled, generate logs about raiding expansion basebuilding and safes
	bool CodeLockRaiding;		//! If enabled, generate logs about codelock raiding (wrong core)
	bool Territory;				//! If enabled, generate logs about creating, deleting territories and inviting players

	bool Killfeed;				//! If enabled, generate logs based on the killfeed module

	bool SpawnSelection;		//! If enabled, generate logs when the player spawn

	bool Party;					//! If enabled, generate logs when the player create a party or invite someone

	bool MissionAirdrop;		//! If enabled, generate logs of spawned airdrops from missions or player called
	//bool MissionHorde;		//! Will log Horde missions

	bool Chat;					//! If enabled, generate logs of the chat

	bool Market;				//! If enabled, generate logs for market system actions by all players
	bool ATM;					//! If enabled, generate logs for ATM system actions by all players

	bool AIGeneral; 			// If enabled, generate logs about AI. This logs will be non specific
	bool AIPatrol; 				// If enabled, generate logs about AI Patrols
	bool AIObjectPatrol; 		// If enabled, generate logs about AI Crash Patrols

	bool LogToScripts; 			// Should the prints be logged in the scripts logs ?
	bool LogToADM; 				// Should the prints be logged in the ADM logs ?

	bool Hardline;			//! If enabled, generate logs for market system actions by all players
	
	bool Garage;				//! If enabled, generate logs for garage module events.
	bool VehicleCover;				//! If enabled, generate logs for vehicle-cover actions.

	bool EntityStorage;			//! If enabled, generate logs for entity-storage actions.
	
	bool Quests;

	[NonSerialized()]
	private string m_FileName;

	[NonSerialized()]
	private FileHandle m_FileLog;

	[NonSerialized()]
	private bool m_IsLoaded;

	// ------------------------------------------------------------
	void ExpansionLogSettings()
	{
	}

	// ------------------------------------------------------------
	override bool OnRecieve( ParamsReadContext ctx )
	{
		//Not sent to client under normal operation
		m_IsLoaded = true;

		ExpansionLogSettings setting;
		if ( !ctx.Read( setting ) )
		{
			Error("ExpansionLogSettings::OnRecieve setting");
			return false;
		}

		CopyInternal( setting );

		m_IsLoaded = true;

		ExpansionSettings.SI_Log.Invoke();

		return true;
	}

	override void OnSend( ParamsWriteContext ctx )
	{
		ExpansionLogSettings thisSetting = this;

		ctx.Write( thisSetting );
	}

	// ------------------------------------------------------------
	override int Send( PlayerIdentity identity )
	{
		//Not sent to client
	}

	// ------------------------------------------------------------
	override bool Copy( ExpansionSettingBase setting )
	{
		ExpansionLogSettings s;
		if ( !Class.CastTo( s, setting ) )
			return false;

		CopyInternal( s );
		return true;
	}

	// ------------------------------------------------------------
	private void CopyInternal( ExpansionLogSettings s )
	{
		Safezone = s.Safezone;
		ExplosionDamageSystem = s.ExplosionDamageSystem;

		VehicleCarKey = s.VehicleCarKey;
		VehicleTowing = s.VehicleTowing;
		VehicleLockPicking = s.VehicleLockPicking;
		VehicleDestroyed = s.VehicleDestroyed;
		VehicleAttachments = s.VehicleAttachments;
		VehicleEnter = s.VehicleEnter;
		VehicleLeave = s.VehicleLeave;
		VehicleDeleted = s.VehicleDeleted;
		VehicleEngine = s.VehicleEngine;

		BaseBuildingRaiding = s.BaseBuildingRaiding;
		CodeLockRaiding = s.CodeLockRaiding;
		Territory = s.Territory;

		Killfeed = s.Killfeed;

		SpawnSelection = s.SpawnSelection;

		MissionAirdrop = s.MissionAirdrop;
		//MissionHorde = s.MissionHorde;

		Party = s.Party;

		Chat = s.Chat;

		Market = s.Market;
		ATM = s.ATM;

		Hardline = s.Hardline;

		AIGeneral = s.AIGeneral;
		AIPatrol = s.AIPatrol;
		AIObjectPatrol = s.AIObjectPatrol;
		
		AdminTools = s.AdminTools;

		LogToScripts = s.LogToScripts;
		LogToADM = s.LogToADM;
		
		Garage = s.Garage;
		VehicleCover = s.VehicleCover;

		EntityStorage = s.EntityStorage;
	}

	// ------------------------------------------------------------
	override bool IsLoaded()
	{
		return m_IsLoaded;
	}

	// ------------------------------------------------------------
	override void Unload()
	{
		m_IsLoaded = false;
	}

	// ------------------------------------------------------------
	override bool OnLoad()
	{
		m_IsLoaded = true;

		m_FileName = EXPANSION_LOG_FOLDER + "\\" + "ExpLog_" + ExpansionStatic.GetISODateTime(false, "_", "-") + ".log";

		bool save;

		bool  logSettingsExist = FileExist(EXPANSION_LOG_SETTINGS);

		if (logSettingsExist)
		{
			EXPrint("[ExpansionLogSettings] Load existing setting file:" + EXPANSION_LOG_SETTINGS);

			ExpansionLogSettings settingsDefault = new ExpansionLogSettings;
			settingsDefault.Defaults();

			JsonFileLoader<ExpansionLogSettings>.JsonLoadFile(EXPANSION_LOG_SETTINGS, this);

			if (m_Version < VERSION)
			{
				EXPrint("[ExpansionLogSettings] Load - Converting v" + m_Version + " \"" + EXPANSION_LOG_SETTINGS + "\" to v" + VERSION);

				if (m_Version < 2)
				{
					//! New with v2
					LogToScripts = settingsDefault.LogToScripts;
					LogToADM = settingsDefault.LogToADM;

					VehicleDestroyed = settingsDefault.VehicleDestroyed;
				}

				if (m_Version < 3)
				{
					AIGeneral = settingsDefault.AIGeneral;
					AIPatrol = settingsDefault.AIPatrol;
				}

				if (m_Version < 4)
				{
					AIObjectPatrol = settingsDefault.AIObjectPatrol;
				}

				if (m_Version < 5)
				{
					ExplosionDamageSystem = settingsDefault.ExplosionDamageSystem;
				}

				if (m_Version < 6)
				{
					VehicleAttachments = settingsDefault.VehicleAttachments;
					VehicleEnter = settingsDefault.VehicleEnter;
					VehicleLeave = settingsDefault.VehicleLeave;
					VehicleDeleted = settingsDefault.VehicleDeleted;
					VehicleEngine = settingsDefault.VehicleEngine;
				}
				
				if (m_Version < 7)
				{
					Garage = settingsDefault.Garage;
					VehicleCover = settingsDefault.VehicleCover;
				}
				
				if (m_Version < 7)
				{
					EntityStorage = settingsDefault.EntityStorage;
				}
				
				if (m_Version < 8)
				{
					Quests = settingsDefault.Quests;
				}

				m_Version = VERSION;
				save = true;
			}
		}
		else
		{
			EXPrint("[ExpansionLogSettings] No existing setting file:" + EXPANSION_LOG_SETTINGS + ". Creating defaults!");
			Defaults();
			save = true;
		}

		if (save)
			Save();

		return logSettingsExist;
	}

	// ------------------------------------------------------------
	override bool OnSave()
	{
		Print("[ExpansionLogSettings] Saving settings");

		JsonFileLoader<ExpansionLogSettings>.JsonSaveFile( EXPANSION_LOG_SETTINGS, this );

		return true;
	}

	// ------------------------------------------------------------
	override void Update( ExpansionSettingBase setting )
	{
		super.Update( setting );

		ExpansionSettings.SI_Log.Invoke();
	}

	// ------------------------------------------------------------
	override void Defaults()
	{
		m_Version = VERSION;

		Safezone = true;
		AdminTools = true;
		ExplosionDamageSystem = true;

		LogToScripts = false;
		LogToADM = false;

		VehicleDestroyed = true;
		VehicleCarKey = true;
		VehicleTowing = true;
		VehicleLockPicking = true;
		VehicleAttachments = true;
		VehicleEnter = true;
		VehicleLeave = true;
		VehicleDeleted = true;
		VehicleEngine = true;

		BaseBuildingRaiding = true;
		CodeLockRaiding = true;
		Territory = true;

		Killfeed = true;

		SpawnSelection = true;

		MissionAirdrop = true;
		//MissionHorde = true;

		Party = true;

		Chat = true;

		AIGeneral = true;
		AIPatrol = true;
		AIObjectPatrol = true;

		Market = true;
		ATM = true;

		Hardline = true;
		
		Garage = true;
		VehicleCover = true;

		EntityStorage = true;
		
		Quests = true;
	}

	override string SettingName()
	{
		return "Log Settings";
	}
	
	void PrintLog(string text, string param1 = string.Empty, string param2 = string.Empty, string param3 = string.Empty, string param4 = string.Empty, string param5 = string.Empty, string param6 = string.Empty, string param7 = string.Empty, string param8 = string.Empty, string param9 = string.Empty)
	{
		if ( !FileExist( EXPANSION_LOG_FOLDER ) )
		{
			ExpansionStatic.MakeDirectoryRecursive( EXPANSION_LOG_FOLDER );
		}

		string output = ExpansionStatic.GetISOTime() + " " + string.Format(text, param1, param2, param3, param4, param5, param6, param7, param8, param9);

		if (LogToScripts || LogToADM)
		{
			if (LogToScripts)
			{
				Print(output);
			}
			if (LogToADM)
			{
				GetGame().AdminLog(output);
			}
		} else {
			if (!FileExist( m_FileName ))
				m_FileLog = OpenFile(m_FileName, FileMode.WRITE);
			else
				m_FileLog = OpenFile(m_FileName, FileMode.APPEND);
			FPrintln(m_FileLog, output);
			CloseFile(m_FileLog);
		}
	}

	void PrintLog(string fmt, Class p1, Class p2 = null, Class p3 = null, Class p4 = null, Class p5 = null, Class p6 = null, Class p7 = null, Class p8 = null, Class p9 = null)
	{
		PrintLog(ExpansionStatic.FormatString(fmt, p1, p2, p3, p4, p5, p6, p7, p8, p9));
	}
};