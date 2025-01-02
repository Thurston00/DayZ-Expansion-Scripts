/**
 * ExpansionAIPatrolSettings.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

class ExpansionAIPatrolSettingsBase: ExpansionSettingBase
{
    bool Enabled;
	float DespawnTime;				    // If all players outside despawn radius, ticks up time. When despawn time reached, patrol is deleted.
	float RespawnTime;				    // Time in seconds before the dead patrol will respawn. If set to -1, they won't respawn
	float MinDistRadius;			    // If the player is closer than MinDistRadius from the spawn point, the patrol won't spawn
	float MaxDistRadius;			    // Same but if the player is further away than MaxDistRadius, the bots won't spawn

	float DespawnRadius;

	float AccuracyMin;
	float AccuracyMax;

	float ThreatDistanceLimit;
	float NoiseInvestigationDistanceLimit;
	float DamageMultiplier;
	float DamageReceivedMultiplier;
}

class ExpansionAIPatrolSettingsV4
{
	ref array< ref ExpansionAICrashPatrol > EventCrashPatrol;
	ref array< ref ExpansionAIPatrol > Patrol;
}

class ExpansionAIPatrolSettingsV19
{
	ref array< ref ExpansionAIObjectPatrol_V19 > ObjectPatrols;
	ref array< ref ExpansionAIPatrol_V19 > Patrols;
}

/**@class		ExpansionAIPatrolSettings
 * @brief		Spawn settings class
 **/
class ExpansionAIPatrolSettings: ExpansionAIPatrolSettingsBase
{
	static const int VERSION = 22;

	ref array< ref ExpansionAIObjectPatrol > ObjectPatrols;
	ref array< ref ExpansionAIPatrol > Patrols;

	[NonSerialized()]
	private ref TStringArray m_UniquePersistentPatrolNames;
	
	[NonSerialized()]
	private bool m_IsLoaded;

    void ExpansionAIPatrolSettings()
    {
        ObjectPatrols = new array< ref ExpansionAIObjectPatrol >;
        Patrols = new array< ref ExpansionAIPatrol >;
    }
	
	// ------------------------------------------------------------
	override bool OnRecieve( ParamsReadContext ctx )
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_1(ExpansionTracing.SETTINGS, this, "OnRecieve").Add(ctx);
#endif

		return true;
	}
	
	// ------------------------------------------------------------
	override void OnSend( ParamsWriteContext ctx )
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_1(ExpansionTracing.SETTINGS, this, "OnSend").Add(ctx);
#endif
	}

	// ------------------------------------------------------------
	override int Send( PlayerIdentity identity )
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_1(ExpansionTracing.SETTINGS, this, "Send").Add(identity);
#endif
		
		return 0;
	}

	// ------------------------------------------------------------
	override bool Copy( ExpansionSettingBase setting )
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_1(ExpansionTracing.SETTINGS, this, "Copy").Add(setting);
#endif

		ExpansionAIPatrolSettings s;
		if ( !Class.CastTo( s, setting ) )
			return false;

		CopyInternal( s );
		
		return true;
	}
	
	// ------------------------------------------------------------
	private void CopyInternal(  ExpansionAIPatrolSettings s )
	{
		ExpansionArray<ExpansionAIObjectPatrol>.RefCopy(s.ObjectPatrols, ObjectPatrols);
		ExpansionArray<ExpansionAIPatrol>.RefCopy(s.Patrols, Patrols);
		
		ExpansionAIPatrolSettingsBase sb = s;
		CopyInternal( sb );
	}

	private void CopyInternal(  ExpansionAIPatrolSettingsBase s )
	{
		Enabled = s.Enabled;
		RespawnTime = s.RespawnTime;
		DespawnTime = s.DespawnTime;
		MinDistRadius = s.MinDistRadius;
		MaxDistRadius = s.MaxDistRadius;
		DespawnRadius = s.DespawnRadius;
		AccuracyMin = s.AccuracyMin;
		AccuracyMax = s.AccuracyMax;
		ThreatDistanceLimit = s.ThreatDistanceLimit;
		NoiseInvestigationDistanceLimit = s.NoiseInvestigationDistanceLimit;
		DamageMultiplier = s.DamageMultiplier;
		DamageReceivedMultiplier = s.DamageReceivedMultiplier;
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
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.SETTINGS, this, "OnLoad");
#endif

		m_IsLoaded = true;

		bool save;

		bool settingsExist = FileExist(EXPANSION_AIPATROL_SETTINGS);

		if (settingsExist)
		{
			EXPrint("[ExpansionAIPatrolSettings] Load existing setting file:" + EXPANSION_AIPATROL_SETTINGS);

			int version;

			//! Try loading settings base
			ExpansionAIPatrolSettingsBase settingsBase;
			if (!ExpansionJsonFileParser<ExpansionAIPatrolSettingsBase>.Load(EXPANSION_AIPATROL_SETTINGS, settingsBase))
			{
				//! Use defaults, but DON'T save them
				Defaults();

				version = VERSION;
			}
			else if (settingsBase.m_Version < 20)
			{
				CopyInternal(settingsBase);

				version = settingsBase.m_Version;
			}
			//! Try loading full setttings
			else if (!ExpansionJsonFileParser<ExpansionAIPatrolSettings>.Load(EXPANSION_AIPATROL_SETTINGS, this))
			{
				//! Use defaults, but DON'T save them
				Defaults();

				version = VERSION;
			}
			else
			{
				version = m_Version;
			}

			if (version < VERSION)
			{
				EXPrint("[ExpansionAIPatrolSettings] Load - Converting v" + version + " \"" + EXPANSION_AIPATROL_SETTINGS + "\" to v" + VERSION);

				ExpansionAIPatrolSettings settingsDefault = new ExpansionAIPatrolSettings;
				settingsDefault.Defaults();

				if (version < 5)
				{
					ExpansionAIPatrolSettingsV4 settingsV4 = new ExpansionAIPatrolSettingsV4;
					ExpansionJsonFileParser<ExpansionAIPatrolSettingsV4>.Load(EXPANSION_AIPATROL_SETTINGS, settingsV4);
					foreach (ExpansionAICrashPatrol crashPatrol: settingsV4.EventCrashPatrol)
					{
						crashPatrol.ClassName = crashPatrol.EventName;
						ObjectPatrols.Insert(crashPatrol);
					}
					Patrols = settingsV4.Patrol;
				}
				else if (version < 20)
				{
					ExpansionAIPatrolSettingsV19 settingsV19;
					if (ExpansionJsonFileParser<ExpansionAIPatrolSettingsV19>.Load(EXPANSION_AIPATROL_SETTINGS, settingsV19))
					{
						foreach (ExpansionAIObjectPatrol_V19 objPatrolV19: settingsV19.ObjectPatrols)
						{
							if (!objPatrolV19.Loadout)
								objPatrolV19.Loadout = objPatrolV19.LoadoutFile;
							ObjectPatrols.Insert(objPatrolV19);
						}

						foreach (ExpansionAIPatrol_V19 patrolV19: settingsV19.Patrols)
						{
							if (!patrolV19.Loadout)
								patrolV19.Loadout = patrolV19.LoadoutFile;
							Patrols.Insert(patrolV19);
						}
					}
				}

				if (version < 6)
					DespawnRadius = MaxDistRadius * 1.1;

				if (version < 8)
				{
					DespawnTime = settingsDefault.DespawnTime;
					AccuracyMin = settingsDefault.AccuracyMin;
					AccuracyMax = settingsDefault.AccuracyMax;
				}

				if (version < 16)
					NoiseInvestigationDistanceLimit = settingsDefault.NoiseInvestigationDistanceLimit;

				if (version < 17 && !DamageReceivedMultiplier)
					DamageReceivedMultiplier = settingsDefault.DamageReceivedMultiplier;

				m_Version = VERSION;
				save = true;
			}

			m_UniquePersistentPatrolNames = {};

			foreach (ExpansionAIPatrol patrol: Patrols)
			{
				if (version < 2)
				{
					patrol.MinSpreadRadius = 1;
					patrol.MaxSpreadRadius = 100;
				}

				if (version < 4)
					patrol.UpdateSettings();

				if (version < 6)
				{
					if (patrol.MaxDistRadius <= 0)
						patrol.DespawnRadius = -2;
					else
						patrol.DespawnRadius = patrol.MaxDistRadius * 1.1;
				}

				if (version < 7)
					patrol.Formation = "RANDOM";

				if (version < 8)
				{
					patrol.DespawnTime = -1;
					patrol.AccuracyMin = -1;
					patrol.AccuracyMax = -1;
				}

				if (version < 11)
				{
					patrol.ThreatDistanceLimit = -1.0;
					patrol.DamageMultiplier = -1.0;
				}

				if (version < 16)
					patrol.NoiseInvestigationDistanceLimit = -1.0;

				if (version < 17 && !patrol.DamageReceivedMultiplier)
					patrol.DamageReceivedMultiplier = -1.0;

				if (version < 22 && !patrol.LootingBehaviour)
					patrol.SetDefaultLootingBehaviour();

				if (patrol.Persist)
				{
					if (patrol.Name && m_UniquePersistentPatrolNames.Find(patrol.Name) == -1)
					{
						m_UniquePersistentPatrolNames.Insert(patrol.Name);
						patrol.GenerateBaseName();
					}
					else
					{
						EXError.Error(this, "Not an unique persistent patrol name: '" + patrol.Name + "'", {});
					}
				}
			}
		
			ExpansionAIObjectPatrol policeCrashPatrol;

			foreach (ExpansionAIObjectPatrol objectPatrol: ObjectPatrols)
			{
				if (version < 2)
				{
					objectPatrol.DefaultSpread();

					switch (objectPatrol.ClassName)
					{
						case "Wreck_Mi8":
							objectPatrol.ClassName = "Wreck_Mi8_Crashed";
							break;
						case "Wreck_PoliceCar":
							objectPatrol.ClassName = "Land_Wreck_sed01_aban1_police";
							policeCrashPatrol = objectPatrol;
							break;
					}
				}

				if (version < 4)
					objectPatrol.UpdateSettings();

				if (version < 6)
				{
					if (objectPatrol.MaxDistRadius <= 0)
						objectPatrol.DespawnRadius = -2;
					else
						objectPatrol.DespawnRadius = objectPatrol.MaxDistRadius * 1.1;
				}

				if (version < 7)
					objectPatrol.Formation = "RANDOM";

				if (version < 8)
				{
					objectPatrol.DespawnTime = -1;
					objectPatrol.AccuracyMin = -1;
					objectPatrol.AccuracyMax = -1;
				}

				if (version < 9)
				{
					objectPatrol.RespawnTime = -2;
				}

				if (version < 11)
				{
					objectPatrol.ThreatDistanceLimit = -1.0;
					objectPatrol.DamageMultiplier = -1.0;
				}

				if (version < 16)
					objectPatrol.NoiseInvestigationDistanceLimit = -1.0;

				if (version < 17 && !objectPatrol.DamageReceivedMultiplier)
					objectPatrol.DamageReceivedMultiplier = -1.0;

				if (version < 22 && !objectPatrol.LootingBehaviour)
					objectPatrol.SetDefaultLootingBehaviour();

				if (!objectPatrol.ClassName)
				{
					EXError.Warn(this, "Ignoring empty object patrol classname", {});
				}
				else if (!ExpansionStatic.IsAnyOf(objectPatrol.ClassName, eAIRegisterDynamicPatrolSpawner.s_RegisteredTypes))
				{
					EXError.Warn(this, "Ignoring object patrol classname '" + objectPatrol.ClassName + "' because it is not registered for dynamic patrol spawning", {});
				}

				if (objectPatrol.Persist)
				{
					EXError.ErrorOnce(this, "Persistence is not supported for object patrols", {});
					objectPatrol.Persist = false;
				}
			}

			if (policeCrashPatrol)
			{
				TStringArray policeWrecks = {"Land_Wreck_sed01_aban2_police", "Land_Wreck_hb01_aban1_police", "Land_Wreck_hb01_aban2_police"};
				foreach (string policeWreck: policeWrecks)
				{
					ObjectPatrols.Insert(new ExpansionAIObjectPatrol(policeCrashPatrol.NumberOfAI, policeCrashPatrol.Speed, policeCrashPatrol.UnderThreatSpeed, policeCrashPatrol.Behaviour, policeCrashPatrol.Faction, policeCrashPatrol.Loadout, policeCrashPatrol.CanBeLooted, policeCrashPatrol.UnlimitedReload, policeCrashPatrol.Chance, policeCrashPatrol.MinDistRadius, policeCrashPatrol.MaxDistRadius, policeWreck));
				}
			}
		}
		else
		{
			EXPrint("[ExpansionAIPatrolSettings] No existing setting file:" + EXPANSION_AIPATROL_SETTINGS + ". Creating defaults!");
			Defaults();
			save = true;
		}

		if (save)
			Save();
		
		return settingsExist;
	}

	// ------------------------------------------------------------
	override bool OnSave()
	{
		Print("[ExpansionAIPatrolSettings] Saving settings");
		
		JsonFileLoader<ExpansionAIPatrolSettings>.JsonSaveFile( EXPANSION_AIPATROL_SETTINGS, this );
		return true;
	}
	
	// ------------------------------------------------------------
	override void Update( ExpansionSettingBase setting )
	{
		super.Update( setting );

		ExpansionSettings.SI_AI.Invoke();
	}

	// ------------------------------------------------------------
	override void Defaults()
	{
		m_Version = VERSION;

        Enabled = true;
        RespawnTime = -1;
        DespawnTime = 600;
        #ifdef DIAG_DEVELOPER
        MinDistRadius = 1;
        #else
        MinDistRadius = 400;
        #endif
        MaxDistRadius = 1000;
        DespawnRadius = 1100;
		AccuracyMin = -1;
		AccuracyMax = -1;

		ThreatDistanceLimit = -1;
		NoiseInvestigationDistanceLimit = -1;
		DamageMultiplier = -1;
		DamageReceivedMultiplier = -1;
    
        string worldName = ExpansionStatic.GetCanonicalWorldName();

        DefaultObjectPatrols(worldName);
        DefaultPatrols(worldName);
	}
    void DefaultObjectPatrols(string worldName)
    {
        switch (worldName)
        {
            case "chernarusplus":
                ObjectPatrols.Insert( new ExpansionAIObjectPatrol(-3, "WALK", "SPRINT", "HALT_OR_ALTERNATE", "West", "", true, false, 1.0, -1, -1, "Wreck_UH1Y"));
                ObjectPatrols.Insert( new ExpansionAIObjectPatrol(-3, "WALK", "SPRINT", "HALT_OR_ALTERNATE", "East", "", true, false, 1.0, -1, -1, "Wreck_Mi8_Crashed"));
                ObjectPatrols.Insert( new ExpansionAIObjectPatrol(-3, "WALK", "SPRINT", "HALT_OR_ALTERNATE", "East", "PoliceLoadout", true, false, 1.0, -1, -1, "Land_Wreck_sed01_aban1_police"));
                ObjectPatrols.Insert( new ExpansionAIObjectPatrol(-3, "WALK", "SPRINT", "HALT_OR_ALTERNATE", "East", "PoliceLoadout", true, false, 1.0, -1, -1, "Land_Wreck_sed01_aban2_police"));
                ObjectPatrols.Insert( new ExpansionAIObjectPatrol(-3, "JOG", "SPRINT", "HALT_OR_ALTERNATE", "West", "NBCLoadout", true, false, 1.0, -1, -1, "ContaminatedArea_Static"));
                ObjectPatrols.Insert( new ExpansionAIObjectPatrol(-3, "JOG", "SPRINT", "HALT_OR_ALTERNATE", "West", "NBCLoadout", true, false, 1.0, -1, -1, "ContaminatedArea_Dynamic"));
            break;
            case "enoch":
                ObjectPatrols.Insert( new ExpansionAIObjectPatrol(-3, "WALK", "SPRINT", "HALT_OR_ALTERNATE", "West", "", true, false, 1.0, -1, -1, "Wreck_UH1Y"));
                ObjectPatrols.Insert( new ExpansionAIObjectPatrol(-3, "WALK", "SPRINT", "HALT_OR_ALTERNATE", "East", "", true, false, 1.0, -1, -1, "Wreck_Mi8_Crashed"));
                ObjectPatrols.Insert( new ExpansionAIObjectPatrol(-3, "WALK", "SPRINT", "HALT_OR_ALTERNATE", "East", "PoliceLoadout", true, false, 1.0, -1, -1, "Land_Wreck_hb01_aban1_police"));
                ObjectPatrols.Insert( new ExpansionAIObjectPatrol(-3, "WALK", "SPRINT", "HALT_OR_ALTERNATE", "East", "PoliceLoadout", true, false, 1.0, -1, -1, "Land_Wreck_hb01_aban2_police"));
                ObjectPatrols.Insert( new ExpansionAIObjectPatrol(-3, "JOG", "SPRINT", "HALT_OR_ALTERNATE", "West", "NBCLoadout", true, false, 1.0, -1, -1, "ContaminatedArea_Static"));
                ObjectPatrols.Insert( new ExpansionAIObjectPatrol(-3, "JOG", "SPRINT", "HALT_OR_ALTERNATE", "West", "NBCLoadout", true, false, 1.0, -1, -1, "ContaminatedArea_Dynamic"));
            break;
			case "deerisle":
				ObjectPatrols.Insert( new ExpansionAIObjectPatrol(-3, "WALK", "SPRINT", "HALT_OR_ALTERNATE", "West", "", true, false, 1.0, -1, -1, "Wreck_UH1Y"));
				ObjectPatrols.Insert( new ExpansionAIObjectPatrol(-3, "WALK", "SPRINT", "HALT_OR_ALTERNATE", "East", "", true, false, 1.0, -1, -1, "Wreck_Mi8_Crashed"));
				ObjectPatrols.Insert( new ExpansionAIObjectPatrol(-3, "JOG", "SPRINT", "HALT_OR_ALTERNATE", "West", "NBCLoadout", true, false, 1.0, -1, -1, "ContaminatedArea_Static"));
            break;
			case "sakhal":
                ObjectPatrols.Insert( new ExpansionAIObjectPatrol(-3, "WALK", "SPRINT", "HALT_OR_ALTERNATE", "West", "", true, false, 1.0, -1, -1, "Wreck_UH1Y"));
				ObjectPatrols.Insert( new ExpansionAIObjectPatrol(-3, "WALK", "SPRINT", "HALT_OR_ALTERNATE", "East", "", true, false, 1.0, -1, -1, "Wreck_Mi8_Crashed"));

                ObjectPatrols.Insert( new ExpansionAIObjectPatrol(-3, "WALK", "SPRINT", "HALT_OR_ALTERNATE", "East", "PoliceLoadout", true, false, 1.0, -1, -1, "Land_Wreck_sed02_aban1_Police_DE"));
                ObjectPatrols.Insert( new ExpansionAIObjectPatrol(-3, "WALK", "SPRINT", "HALT_OR_ALTERNATE", "East", "PoliceLoadout", true, false, 1.0, -1, -1, "Land_Wreck_sed02_aban2_Police_DE"));
				
                ObjectPatrols.Insert( new ExpansionAIObjectPatrol(-3, "JOG", "SPRINT", "HALT_OR_ALTERNATE", "East", "NBCLoadout", true, false, 1.0, -1, -1, "ContaminatedArea_Static"));
                ObjectPatrols.Insert( new ExpansionAIObjectPatrol(-3, "JOG", "SPRINT", "HALT_OR_ALTERNATE", "East", "NBCLoadout", true, false, 1.0, -1, -1, "ContaminatedArea_Dynamic"));
            break;
            default:
                if ( GetExpansionSettings().GetLog().AIObjectPatrol )
                    GetExpansionSettings().GetLog().PrintLog("[AI Object Patrol] WARNING: The map \""+worldName+"\" doesn't have a default config");
                    GetExpansionSettings().GetLog().PrintLog("[AI Object Patrol] Generating an example config...");

                ObjectPatrols.Insert( new ExpansionAIObjectPatrol(-3, "WALK", "SPRINT", "ALTERNATE", "West", "", true, false, 1.0, -1, -1, "Wreck_UH1Y"));
                ObjectPatrols.Insert( new ExpansionAIObjectPatrol(-3, "WALK", "SPRINT", "ALTERNATE", "East", "", true, false, 1.0, -1, -1, "Wreck_Mi8_Crashed"));
                ObjectPatrols.Insert( new ExpansionAIObjectPatrol(-3, "WALK", "SPRINT", "HALT", "East", "PoliceLoadout", true, false, 1.0, -1, -1, "Land_Wreck_sed01_aban1_police"));
                ObjectPatrols.Insert( new ExpansionAIObjectPatrol(-3, "WALK", "SPRINT", "HALT", "East", "PoliceLoadout", true, false, 1.0, -1, -1, "Land_Wreck_sed01_aban2_police"));
                ObjectPatrols.Insert( new ExpansionAIObjectPatrol(-3, "WALK", "SPRINT", "HALT", "East", "PoliceLoadout", true, false, 1.0, -1, -1, "Land_Wreck_hb01_aban1_police"));
                ObjectPatrols.Insert( new ExpansionAIObjectPatrol(-3, "WALK", "SPRINT", "HALT", "East", "PoliceLoadout", true, false, 1.0, -1, -1, "Land_Wreck_hb01_aban2_police"));
                return;
            break;
	    }

        if ( GetExpansionSettings().GetLog().AIObjectPatrol )
            GetExpansionSettings().GetLog().PrintLog("[AI Object Patrol] Generating default Object Patrol config for \""+worldName+"\"");        
	}

    void DefaultPatrols(string worldName)
    {
        switch (worldName)
        {
            case "banov":
                Patrols.Insert( new ExpansionAIPatrol(-3, 	"WALK", "SPRINT",     "ALTERNATE",     "West",     	"", true, false, 1.0, -1, -1, -2, -100,	{"976.139 263.979 12683.9", "1094.76 269.075 12877.5"}));
                Patrols.Insert( new ExpansionAIPatrol(-3, 	"WALK", "SPRINT",     "ALTERNATE",     "West",     	"", true, false, 1.0, -1, -1, -2, -100,  	{"671.794 245.434 11457.6", "831.525 245.301 11211.7"}));
                Patrols.Insert( new ExpansionAIPatrol(-3, 	"WALK", "SPRINT",     "ALTERNATE",     "West",     	"", true, false, 1.0, -1, -1, -2, -100,  	{"1743.85 268.621 11658.2", "1829.22 261.877 12054.7"}));
                Patrols.Insert( new ExpansionAIPatrol(-3, 	"WALK", "SPRINT",     "ALTERNATE",     "West",     	"", true, false, 1.0, -1, -1, -2, -100,  	{"1520.8 229.459 9879.21", "1742.91 229.232 9592.4"}));
                Patrols.Insert( new ExpansionAIPatrol(-3, 	"WALK", "SPRINT",     "ALTERNATE",     "West",     	"", true, false, 1.0, -1, -1, -2, -100,  	{"1880.51 224.479 8392.91", "1921.05 221.434 8027.86"}));
                Patrols.Insert( new ExpansionAIPatrol(-3, 	"WALK", "SPRINT",     "ALTERNATE",     "West",     	"", true, false, 1.0, -1, -1, -2, -100,  	{"1884.05 227.035 6599.31", "2117.14 222.043 6418.1"}));
                Patrols.Insert( new ExpansionAIPatrol(-3, 	"WALK", "SPRINT",     "ALTERNATE",     "West",     	"", true, false, 1.0, -1, -1, -2, -100,  	{"1226.16 234.056 4521.42", "978.184 229.308 4361.1"}));
                Patrols.Insert( new ExpansionAIPatrol(-3, 	"WALK", "SPRINT",     "ALTERNATE",     "West",     	"", true, false, 1.0, -1, -1, -2, -100,  	{"640.75 250.928 3287.15", "479.013 252.843 3354.61"}));
                Patrols.Insert( new ExpansionAIPatrol(-3, 	"WALK", "SPRINT",     "ALTERNATE",     "West",     	"", true, false, 1.0, -1, -1, -2, -100,  	{"2576.35 237.181 1903.83", "2440.35 233.075 2074.31"}));
                Patrols.Insert( new ExpansionAIPatrol(-3, 	"WALK", "SPRINT",     "ALTERNATE",     "West",     	"", true, false, 1.0, -1, -1, -2, -100,  	{"3623.74 214.088 3556.15", "3760.02 214.088 3307.33"}));
                Patrols.Insert( new ExpansionAIPatrol(-3, 	"WALK", "SPRINT",     "ALTERNATE",     "West",     	"", true, false, 1.0, -1, -1, -2, -100,  	{"5200.22 201.686 4526.44", "5063.23 205.131 5062.29"}));
                Patrols.Insert( new ExpansionAIPatrol(-3, 	"WALK", "SPRINT",     "ALTERNATE",     "West",     	"", true, false, 1.0, -1, -1, -2, -100,  	{"4554.5 211.798 7601.49", "4658.85 217.283 7921.67"}));
            break;
            case "chernarusplus":
                Patrols.Insert( new ExpansionAIPatrol(-3, 	"WALK", "SPRINT",     "ALTERNATE",		"West",     	"", true, false, 1.0, -1, -1, -2, -100,  	{"4211.222656 109.023384 6382.064453", "4151.662598 105.450653 6080.294434", "4160.971680 105.412598 6035.103027", "4160.699219 106.251480 5906.830078", "4107.862793 108.930527 5898.482910", "4057.258057 114.174736 5584.595703", "4084.560059 113.232422 5494.540039", "4079.308105 113.801163 5435.953125", "4081.735840 113.402504 5385.576172", "4067.266113 109.788383 4904.508789", "4126.504883 107.371178 4647.128906"}));
                Patrols.Insert( new ExpansionAIPatrol(-3, 	"WALK", "SPRINT",     "ALTERNATE",    	"West",     	"", true, false, 1.0, -1, -1, -2, -100,  	{"4508.860352 109.347275 6230.751465", "4448.490723 109.377792 6025.892578", "4546.902832 112.057549 6126.590332", "4613.975586 110.163887 6112.869629", "4563.825684 110.696106 5797.450195", "4551.805664 110.555084 5652.876953", "4312.203613 110.752151 5366.941895"}));
                Patrols.Insert( new ExpansionAIPatrol(1,	"WALK", "SPRINT",     "ALTERNATE",	"Raiders",	"", true, false, 1.0, -1, -1, -2, -100,  		{"10545.687500 38.037155 10384.205078", "10502.615234 40.377762 10464.754883", "10700.634766 34.346542 10461.470703", "10645.350586 36.451836 10377.769531"}));
                Patrols.Insert( new ExpansionAIPatrol(1,	"WALK", "SPRINT",     "ALTERNATE",     "Raiders",    "", true, false, 1.0, -1, -1, -2, -100, 		{"8588.209961 103.304726 13439.002930", "8578.585938 106.485222 13465.281250", "8605.717773 112.455276 13521.625977", "8640.509766 120.118225 13590.360352", "8641.976563 127.327774 13644.069336", "8643.523438 123.581627 13609.848633", "8724.077148 121.008499 13532.352539", "8792.484375 119.344810 13479.867188", "8843.358398 122.085854 13469.464844", "8881.161133 121.293594 13413.066406", "8837.004883 121.072372 13470.006836", "8742.039063 121.246811 13516.587891", "8704.238281 119.762787 13546.516602", "8609.144531 113.184517 13527.601563", "8573.016602 112.247055 13530.662109", "8563.083984 118.173904 13576.212891", "8500.525391 135.590210 13653.769531", "8456.375000 139.365845 13677.127930", "8546.610352 126.129440 13615.544922", "8566.747070 116.355377 13563.688477", "8573.123047 112.562714 13532.739258", "8563.700195 107.298088 13476.465820", "8586.047852 103.664093 13442.43557"}));
                Patrols.Insert( new ExpansionAIPatrol(-3,	"WALK", "SPRINT",     "ALTERNATE",  "Civilian",     "", true, false, 1.0, -1, -1, -2, -100,  	{"9963.346680 55.640099 10900.844727", "9965.398438 54.729034 10969.536133", "9924.380859 57.232151 10901.967773"}));
            break;
            case "deerisle":
                Patrols.Insert(new ExpansionAIPatrol(-3, "WALK", "SPRINT", "ALTERNATE", "West", "", true, false, 1.0, -1, -1, -2, -100, {"2614.28 35.1122 3482.59", "2637.89 34.7324 3502.45", "2649.33 33.0476 3525.77", "2705.26 22.8186 3574.8", "2707.79 22.4635 3585.09", "2752.8 22.4625 3629.68", "2823.14 22.4625 3699.33", "2918.73 22.7593 3797.69", "2923.53 22.4625 3810.79", "2973.94 22.4625 3883.61", "3003.54 22.4625 3948.92", "3007.75 22.7279 3989.39", "3025.54 22.7257 4031.3", "3053.0 22.4625 4058.19", "3126.36 22.7551 4233.04", "3112.06 22.723 4235.07"}));
                Patrols.Insert(new ExpansionAIPatrol(-3, "WALK", "SPRINT", "ALTERNATE", "West", "", true, false, 1.0, -1, -1, -2, -100, {"13605.3 12.922 9866.76", "13493.5 12.922 9873.86", "13489.6 12.922 9884.22", "13498.3 12.922 9931.7", "13482.2 12.8804 9946.66", "13426 12.95 9973.23", "13402.3 12.95 9978.1", "13399.0 12.95 9919.61", "13394.2 12.92 9857.08", "13397.2 12.922 9842.91", "13401.9 13.13 9843.68"}));
                Patrols.Insert(new ExpansionAIPatrol(-3, "WALK", "SPRINT", "ALTERNATE", "West", "", true, false, 1.0, -1, -1, -2, -100, {"13469.2 12.9525 9603.42", "13432.9 12.9525 9608.83", "13438.3 12.9172 9657.9", "13489.3 12.9525 9651.27", "13488.5 12.9525 9637.62", "13542.0 12.9525 9630.26"}));
                Patrols.Insert(new ExpansionAIPatrol(-3, "WALK", "SPRINT", "ALTERNATE", "West", "", true, false, 1.0, -1, -1, -2, -100, {"6137.18 8.1828 14667.3", "6127.65 8.1598 14655.7", "6127.19 6.34518 14647.1", "6148.16 6.02 14647.3", "6148.73 6.02 14702.1", "6168.66 6.00306 14728.6", "6169.38 6.014 14824.4", "6165.39 6.014 14827.5", "6053.98 6.031 14827.0", "6037.31 6.019 14826.6", "6037.33 6.02 14830.5", "5950.27 6.02 14830.1", "5937.34 6.0199 14822.4", "5933.13 6.01682 14809.5", "5933.28 6.01995 14585.2", "5961.84 6.01966 14568.8", "6145.52 6.02156 14568.7", "6145.21 6.01999 14584.8", "6148.13 6.01999 14584.8", "6147.86 6.02 14638.8", "6132.03 6.34517 14638.9", "6135.35 8.18288 14612"}));
                Patrols.Insert(new ExpansionAIPatrol(-3, "WALK", "SPRINT", "ALTERNATE", "West", "", true, false, 1.0, -1, -1, -2, -100, {"7173.408691 9.388966 8463.492188","7160.309082 11.887913 8478.935547","7140.454590 11.644825 8480.054688","7113.766113 10.662418 8474.416016","7072.175293 9.221134 8458.308594","7026.322754 9.179523 8437.571289","6980.965820 10.233067 8412.732422","6955.680664 10.120960 8385.440430","6905.847168 8.989536 8330.739258","6863.664063 9.215154 8331.541016","6836.514160 9.638411 8321.602539","6797.618164 9.495352 8287.283203","6740.117188 9.814436 8248.661133","6718.867188 9.970747 8250.128906","6691.622070 8.951742 8219.959961","6675.319336 7.644308 8202.658203","6649.652832 7.907194 8204.944336","6631.635254 8.704412 8206.903320","6622.793945 7.470053 8185.242676","6551.247070 7.014589 8149.377930","6536.215332 7.431044 8156.504883","6501.638672 11.898161 8223.717773","6492.099609 13.591047 8241.760742","6435.308105 21.961739 8311.371094","6439.540527 23.613239 8328.807617","6467.306641 25.377892 8359.319336","6551.270508 21.832504 8434.381836","6582.410645 21.633068 8462.824219","6607.481445 22.558878 8501.813477","6643.264648 22.621046 8565.387695","6667.231934 22.503086 8600.999023","6724.488770 22.513264 8600.029297","6771.289551 22.521908 8601.418945","6793.190918 22.503086 8590.014648","6809.542480 23.100607 8605.534180","6828.111328 23.235811 8597.363281","6851.508301 23.267950 8579.098633","6873.802734 23.380533 8573.422852","6884.249512 23.221556 8583.534180","6889.550781 22.448168 8612.526367","6899.882324 21.569456 8629.641602","6975.955566 14.889189 8643.426758","6988.708984 14.723112 8634.900391","6991.130859 14.723120 8611.249023","6990.776855 14.735922 8585.320313","6999.100098 14.724508 8595.410156","7012.806152 14.819036 8601.688477","7156.463379 7.533155 8668.093750","7188.237793 7.533087 8635.582031","7225.431152 7.533075 8577.524414","7232.838867 7.533079 8556.956055","7225.312012 7.533087 8501.372070","7201.724609 7.533102 8450.014648"}));
                Patrols.Insert(new ExpansionAIPatrol(-3, "WALK", "SPRINT", "ALTERNATE", "East", "", true, false, 1.0, -1, -1, -2, -100, {"5105.292969  74.883148  3407.413574","4937.243164  74.821365  3224.987549","4966.652832  74.883087  3180.755127","4997.585938  74.883217  3182.841797","5026.474121  74.882973  3186.964844","5177.353027  74.883057  3076.269287","5240.547363  74.883102  3116.733154","5278.296875  74.883057  3148.326904","5316.493164  74.883087  3160.389893","5364.889160  74.883087  3168.323975","5407.135254  74.883057  3164.797119","5469.905273  74.883179  3131.442627","5520.993164  74.883087  3122.511230","5588.959961  74.883080  3113.893555","5688.474121  74.883072  3087.173340","5702.459961  74.883179  3091.440918","5740.863281  74.883118  3137.692139","5766.643555  74.883118  3226.102051","5794.341797  74.883057  3316.956787","5835.802734  74.883118  3410.853516","5885.872559  74.883209  3507.269775","5927.307617  74.883362  3533.771240","5986.234863  74.882935  3568.775146","6025.899414  74.882935  3600.790771","6044.972168  74.883148  3630.977051","6047.193359  74.882751  3671.805664","6028.593262  73.353088  3852.787109","6008.884277  74.246895  3863.177979","5941.529297  74.883141  3868.817627","5846.303711  74.882988  3859.514893","5756.658691  74.883194  3844.458496","5666.505371  74.883118  3847.945801","5503.789551  74.883041  3840.066162","5282.430176  74.882889  3924.535400","5185.626465  74.883057  3928.957275","5163.816406  74.883087  3919.254395","5030.320801  74.882843  3596.252686","5041.362305  74.883011  3572.342285"}));
                Patrols.Insert(new ExpansionAIPatrol(-3, "JOG", "SPRINT", "ALTERNATE", "West", "", true, false, 1.0, -1, -1, -2, -100, {"5023.589355 9.128414 7229.875977","4992.779297 10.270885 7210.366211","4967.647461 12.362774 7191.795898","4973.936035 14.241009 7158.613281","4994.817383 19.061993 7121.469727","5026.020020 20.684582 7103.006348","5035.761719 20.788662 7066.528320","5044.874512 21.313137 7010.070313","5031.183594 21.781475 6981.863770","5006.580078 21.752743 6971.242676","4970.947754 21.547436 6955.353516","4940.404297 21.457897 6924.586426","4929.826172 24.395214 6858.939453","4902.103027 26.175083 6819.469727","4864.590332 26.559422 6796.793945","4826.681152 26.563356 6789.344727","4800.619629 26.563362 6803.583008","4745.721680 21.879513 6834.797363","4692.837891 21.313381 6849.098145","4647.015137 21.313465 6894.228027","4611.984863 21.313328 6942.446777","4572.544922 21.313389 7036.241211","4553.119629 21.313377 7087.086426","4512.923828 21.313328 7157.360352","4479.827637 21.313366 7224.496582","4436.747070 21.313374 7279.045410","4487.253906 21.313351 7309.398926","4512.975098 21.313351 7308.438477","4547.350098 21.313366 7329.120605","4581.737305 21.313389 7369.389648","4755.085449 16.931751 7473.766113","4789.499023 15.890812 7465.653320","4919.640625 15.516087 7465.255859","4941.862305 14.042789 7434.039063","4992.607910 12.955448 7388.423828","5022.069336 11.650700 7332.769043","5046.530762 9.257355 7293.361328","5045.930664 9.534286 7250.209961"}));
                Patrols.Insert(new ExpansionAIPatrol(-3, "JOG", "SPRINT", "ALTERNATE", "West", "", true, false, 1.0, -1, -1, -2, -100, {"6897.104492 44.021137 4012.673828","6842.273438 46.737545 4013.390869","6771.734375 48.194508 4031.554688","6671.769043 39.125134 4024.631104","6596.561035 40.753994 3993.569580","8179.428711 2.924011 2849.741455","8149.074219 2.988998 2834.369629","8122.857910 3.702179 2825.269043","8095.122559 3.738388 2814.752686","8076.083008 3.987488 2793.997559","8060.978027 5.015549 2777.531006","8035.273438 8.749878 2776.475342","8004.330078 13.752808 2777.164063","7962.450195 18.345261 2786.500977","7931.662109 21.232788 2779.054199","7903.535645 23.544861 2767.040527","7867.930664 24.424133 2765.795898","7831.402344 26.427658 2751.152588","7815.127930 26.329788 2758.852783","7810.836914 24.722824 2772.945313","7811.668457 23.530670 2792.183838","7819.034668 22.802322 2804.089111","7836.331055 21.124908 2810.410400","7857.161621 18.554581 2817.930176","7864.521484 17.217606 2835.835693","7863.428711 16.966125 2849.535156","7856.153320 17.411224 2864.746094","7828.728516 20.659576 2889.870117","7816.283203 20.529007 2927.052002","7784.317383 23.594574 2966.616943","7737.316895 27.908234 3016.287109","7691.141113 30.633850 3070.248291","7623.613281 34.496063 3141.477295","7602.263184 35.670204 3165.946777","7556.743652 36.234535 3164.327148","7439.909180 31.439651 3119.395264","7434.365723 29.946365 3103.269531","7458.239258 26.129852 2970.760498","7417.231445 32.008514 2870.650879","7384.517578 33.522335 2840.519287","7354.351074 32.686783 2857.552246","7311.598633 29.586929 2866.230713","7159.538086 25.032822 2865.060791","7134.467773 25.033081 2872.776855","7044.474121 25.055946 2874.498535","7008.395508 26.092699 2855.772949","6856.687500 28.936148 2855.697998","6841.005859 28.010029 2858.415527","6806.844238 25.831034 2878.357422","6779.008789 25.100523 2902.508301","6753.899414 23.753138 2945.811279","6731.682129 24.075098 3007.853027","6728.552734 24.843439 3080.551025","6733.214844 27.284525 3106.632568","6734.564453 28.858675 3123.229492","6717.104492 32.490810 3150.805176","6664.845215 31.099644 3113.001465","6631.446777 30.471411 3092.233887","6582.086914 37.175491 3089.964600","6560.549805 38.931984 3086.347656","6520.246094 37.138817 3066.472168","6496.960449 33.297905 3046.066406","6451.425293 29.467440 2979.942383","6432.821777 31.996532 2963.232178","6431.282227 33.946495 2919.659912","6454.630371 33.380405 2877.786865","6473.607910 31.127491 2842.326172","6529.279785 33.853764 2838.841309","6572.902344 35.040798 2851.526367","6596.276367 34.975422 2851.935303","6644.467773 32.872581 2865.423340","6699.875977 32.360275 2849.088623","6736.550781 33.390095 2814.649414","6766.872559 32.523785 2821.925781","6774.668945 30.047482 2836.899658","6797.131348 27.175587 2837.732178","6823.747559 26.363750 2869.113525","6799.473145 25.469666 2882.020264","6774.371094 24.788200 2910.954590","6756.423828 23.753059 2941.262207","6737.038574 23.753052 2984.018555","6729.011719 23.672117 3028.337402","6728.158203 25.182100 3086.891357","6734.647949 28.289200 3117.988281","6718.448730 32.362370 3148.242920","6746.092773 31.732847 3165.163574","6836.893066 26.366173 3186.455811","6910.361816 29.219650 3216.559570","6954.007813 32.317978 3222.030273","6982.460938 44.249649 3255.333252","6966.448242 57.931980 3323.297852","6944.679688 69.751419 3396.014648","7023.480957 61.865868 3431.227051","7064.491699 55.182533 3454.273682","7130.115723 43.350189 3486.160889","7112.151855 49.310394 3561.590576","7094.652344 54.748322 3617.674072","7097.737305 60.560059 3681.124512","7075.779297 61.940491 3728.962891","7009.927734 57.159912 3782.783936","6967.057129 51.619274 3858.282471","6972.096191 47.892155 3943.286133","6981.169434 46.187870 3984.227783"}));
                Patrols.Insert(new ExpansionAIPatrol(-3, "WALK", "SPRINT", "ALTERNATE", "East", "", true, false, 1.0, -1, -1, -2, -100, {"5004.802246 18.821041 1260.274048","5035.195801 18.209957 1240.634277","5055.747559 17.985332 1218.028198","5048.761230 17.726864 1204.968994","5016.326172 17.882870 1231.750977","4979.917480 16.690670 1217.797974","4938.226563 13.830379 1197.873901","4890.552246 10.440151 1188.925049","4824.031738 6.762966 1213.569092","4801.750488 9.389462 1296.074219","4778.162598 9.393063 1311.304688","4748.961426 9.703396 1312.309204","4716.996094 11.005672 1364.977295","4716.264160 10.004604 1405.306274","4661.491211 12.722897 1538.352783","4577.537598 17.060879 1568.928711","4507.254883 16.488125 1684.533813","4514.718750 18.998135 1744.351685","4480.170898 27.429501 2015.915039","4492.197266 17.206020 2127.564941","4597.801758 14.997982 2144.635742","4635.490723 14.844448 2142.144531","4731.012695 14.441647 2077.525146","4756.801270 12.836346 2055.916260","4811.145508 8.682232 2049.876221","4837.827148 7.947586 2036.968506","4850.624023 9.207855 2004.873047","4867.536621 10.099442 1993.682007","4897.504883 10.289421 2006.348633","4922.653320 10.302731 2017.452271","4960.833496 10.231007 2020.802612","4996.945801 9.540453 2042.454102","5026.220703 9.576416 2044.817139","5051.073730 10.010212 2045.343140","5078.269043 11.379700 2062.395020","5140.619629 16.521576 2048.908447","5180.918457 18.699017 2052.528564","5225.732910 21.215664 2062.529053","5264.776855 23.527645 2071.632080","5363.172363 31.896786 2142.549072","5418.923828 31.002621 2113.879150","5498.963379 30.763317 2050.544922","5585.296875 34.817150 2024.716797","5684.998047 35.971291 1970.189331","5809.072266 37.279922 1855.469604","5871.693848 39.239044 1822.434448","5862.568848 39.181557 1793.611206","5819.487793 42.873787 1755.275391","5788.761719 48.449821 1737.903442","5763.625488 52.473915 1718.924805","5692.450684 56.128624 1674.482666","5613.318359 56.183281 1635.194214","5539.051270 56.183212 1600.366699","5452.719727 56.183319 1581.690430"}));
                Patrols.Insert(new ExpansionAIPatrol(-3, "WALK", "SPRINT", "ALTERNATE", "West", "", true, false, 1.0, -1, -1, -2, -100, {"2579.932129 23.333069 4235.567871","2593.196777 23.333071 4219.643555","2656.975586 23.333076 4182.639160","2863.843018 23.333073 4059.833984","2899.538818 23.333069 4120.433594","2853.204834 23.333061 4159.608398","2605.180908 23.357174 4300.535156","2645.617676 21.772598 4374.602539","2988.077637 21.783073 4204.449707","2938.694336 23.333069 4093.587891","2902.603271 23.333069 4120.771484","2819.662598 23.333069 3980.012451","2842.596191 23.333061 3956.686035","2800.976563 23.333061 3877.970215","2503.984131 23.333071 4053.990723","2537.265869 23.333059 4108.304688","2549.204834 23.333080 4121.586426","2549.275391 23.333075 4145.065918","2513.447754 23.513081 4179.836914","2547.269287 23.513081 4240.889160"}));
                Patrols.Insert(new ExpansionAIPatrol(-3, "JOG", "SPRINT", "ALTERNATE", "West", "", true, false, 1.0, -1, -1, -2, -100, {"3786.805908 12.333086 9733.815430","3758.332275 12.333063 9722.347656","3736.635010 12.333002 9722.320313","3719.119141 12.306665 9711.250977","3711.237793 12.333086 9684.793945","3700.300781 12.355684 9667.724609","3672.041992 12.314631 9643.527344","3654.281738 12.350168 9617.707031","3658.359619 12.333109 9584.325195","3648.146973 12.333361 9550.807617","3647.339600 12.331461 9521.150391","3663.618896 12.333078 9484.071289","3640.170166 12.333078 9466.806641","3603.309814 14.863077 9467.035156","3584.229980 12.903780 9471.476563","3569.193115 12.333094 9450.920898","3570.201660 12.350084 9425.680664","3592.129150 14.005274 9396.376953","3605.931396 14.863016 9367.699219","3630.786133 12.418398 9345.233398","3631.999023 12.352945 9313.415039","3598.363281 14.842585 9247.262695","3568.584717 12.333078 9222.100586","3547.712646 12.333124 9216.068359","3536.873779 12.333124 9192.736328","3545.318848 12.333025 9169.984375","3561.759033 12.333048 9139.125977","3553.612061 12.333094 9116.165039","3535.462891 12.333109 9107.804688","3509.122314 12.324732 9095.628906","3478.594482 12.333109 9073.530273","3448.274658 12.333071 9039.604492","3431.082520 12.333094 9036.455078","3409.281250 12.333101 9058.358398","3387.759521 12.336718 9051.425781","3365.641113 12.333109 9012.490234","3346.658691 12.332193 8976.356445","3350.416016 12.333055 8936.791016","3335.197998 12.333086 8924.931641","3311.619385 12.354143 8920.767578","3302.078369 12.328264 8910.823242","3296.809570 12.331057 8891.041992","3272.611572 12.333094 8856.617188","3271.885986 12.333063 8839.529297","3285.544189 12.324991 8821.890625","3306.633789 12.333094 8810.247070","3328.892822 12.333105 8784.223633","3348.830322 12.318827 8743.290039","3375.992676 12.333071 8748.544922","3401.464355 12.333086 8742.940430","3402.901367 12.333086 8717.410156","3403.462646 12.333162 8697.310547","3416.426758 12.333063 8668.534180","3433.639160 12.333071 8643.429688","3448.937012 12.333078 8641.264648","3464.744873 12.327776 8646.118164","3479.317871 12.333117 8630.125977","3491.797363 12.332994 8612.158203","3511.294434 12.333086 8604.883789","3536.136230 12.281733 8621.510742","3560.478760 12.332090 8634.785156","3593.125732 14.309748 8639.667969","3628.485352 12.333082 8673.004883","3650.887695 12.333091 8671.662109","3669.029053 11.964583 8680.644531","3681.000488 12.199207 8686.785156","3711.575684 12.213129 8687.706055","3742.802734 12.332726 8700.209961","3742.393311 12.600687 8716.777344","3747.585205 12.592478 8730.720703","3774.217529 12.353716 8730.978516","3776.315674 14.573619 8818.950195","3776.729980 15.447012 8871.737305","3777.903564 16.883074 8939.324219","3778.117188 16.883051 9029.975586","3778.040771 16.883081 9057.962891"}));
                Patrols.Insert(new ExpansionAIPatrol(-3, "WALK", "SPRINT", "ALTERNATE", "East", "", true, false, 1.0, -1, -1, -2, -100, {"10248.541992 7.769688 4006.076172","10188.319336 5.519455 4004.424316","10125.097656 5.528504 4004.649902","10074.928711 9.633690 3991.812500","10008.732422 13.007263 3988.743896","9925.311523 17.588524 4007.845947","9865.559570 19.401848 4010.413330","9801.088867 20.170387 4039.091309","9754.723633 18.890755 4085.807129","9696.554688 15.939964 4165.580078","9645.530273 22.081924 4142.987305","9650.413086 24.423183 4115.669434","9657.914063 26.652901 4092.913330","9647.224609 32.876186 4063.172607","9615.327148 47.798241 4007.450684","9607.370117 50.911316 3986.755371","9617.142578 52.514336 3955.597900","9600.973633 53.062370 3951.022949","9577.680664 53.165916 3954.803223","9508.858398 57.597313 3948.985352","9486.725586 59.937141 3945.851807","9468.635742 60.429298 3947.414551","9445.693359 61.248924 3941.057861","9424.458984 61.532570 3921.611572","9385.827148 61.283897 3903.966553","9346.924805 63.174126 3913.922852","9327.238281 64.339684 3933.384277","9274.254883 65.982872 3951.609619","9195.087891 53.859047 3976.080811","9158.679688 50.126732 3991.563232","9150.239258 49.329140 4001.597656","9113.832031 46.349838 4009.520996","9097.755859 44.491844 4023.793701","9087.282227 42.876030 4044.645020","9070.584961 41.297493 4064.223145","9050.478516 40.560204 4080.165771","9033.879883 39.349876 4095.475586","9021.335938 35.225075 4122.157715","9018.450195 32.103951 4140.063965","9018.059570 29.107567 4160.020508","9012.372070 28.813042 4167.108887","8938.162109 28.567360 4183.574219","8895.636719 24.763252 4187.396484","8851.059570 19.607323 4176.767090","8798.993164 13.634010 4172.526367","8771.652344 11.686577 4175.770020","8768.846680 10.984276 4129.094727","8762.291016 9.779076 4080.722412","8743.181641 6.689003 4030.406982","8711.707031 6.476220 3991.117188","8691.881836 5.576279 3971.292725","8638.368164 4.783066 3943.628906","8591.295898 4.784836 3919.444580","8567.605469 5.964898 3888.136230","8560.098633 6.642510 3847.295410","8561.667969 6.622109 3785.136719","8562.445313 6.653069 3733.442871","8573.798828 6.613053 3705.441406","8597.836914 6.653145 3652.540039","8624.749023 6.653061 3600.360107","8608.791992 6.653031 3582.737549","8613.640625 6.653054 3563.313721","8597.356445 6.933562 3547.088135","8653.596680 6.527781 3493.386963","8676.112305 6.653090 3510.047607","8692.521484 6.653071 3521.793213","8741.209961 6.681731 3473.288330","8767.605469 6.618741 3436.840576","8795.853516 5.146226 3416.399170","8834.324219 3.897534 3407.109375","8852.898438 4.083982 3421.691895","8883.180664 5.893029 3417.282959","8916.852539 6.999246 3393.503906","8930.104492 5.765046 3377.290771","8958.083008 6.363282 3376.937012","8999.769531 6.709023 3379.410400","9048.626953 8.828233 3343.027588","9066.011719 10.491853 3323.599609","9088.827148 11.515054 3321.328125","9102.313477 13.376496 3342.465576","9126.626953 15.321641 3356.397949","9160.794922 14.958711 3353.843018","9186.342773 14.707620 3362.515381","9202.639648 18.834747 3385.048340","9212.712891 29.320480 3440.402344","9225.765625 26.732159 3419.581543","9257.206055 23.710781 3380.809082","9272.066406 20.468517 3367.987793","9281.643555 17.370243 3339.685059","9320.713867 19.210238 3320.362061","9340.695313 19.597979 3296.574463","9370.236328 20.964132 3286.156494","9402.354492 21.395861 3261.492188","9438.041992 7.439215 3214.109619","9458.627930 4.739218 3192.897949","9506.948242 5.494055 3183.573486","9584.356445 10.347834 3170.478516","9622.754883 14.359442 3204.779297","9642.785156 18.883429 3236.333252","9640.224609 19.626120 3276.525391","9667.747070 25.737738 3321.172363","9729.589844 30.514462 3374.979736","9762.726563 32.956879 3414.352051","9784.910156 45.840729 3459.639648","9796.734375 57.087585 3502.388184","9794.308594 57.747520 3540.257568","9827.741211 56.616608 3532.566406","9849.041016 55.851387 3529.862305","9879.608398 54.236763 3546.859863","9907.598633 50.589169 3568.269775","9931.843750 44.288937 3591.917480","9974.830078 38.879383 3605.683838","10005.426758 37.031338 3637.030029","10034.870117 29.225174 3628.653320","10049.310547 25.680244 3650.831787","10053.655273 22.568758 3705.808105","10078.684570 16.885641 3772.148926","10081.750977 17.355642 3788.521729","10057.663086 21.595007 3801.720947","10043.509766 25.497499 3818.090820","10027.912109 27.177629 3854.565674","10012.454102 26.388800 3890.626709","10019.016602 21.129225 3920.223633","10033.092773 11.904242 3982.553955","10070.111328 10.020567 3989.810547","10111.440430 6.829031 4000.438721","10177.646484 5.270506 4004.403564","10242.304688 7.526014 4006.807617","10301.955078 6.954718 3988.060303","10337.705078 5.041197 3978.397217","10360.471680 4.318647 3996.477295"}));
                Patrols.Insert(new ExpansionAIPatrol(-3, "JOG", "SPRINT", "ALTERNATE", "West", "", true, false, 1.0, -1, -1, -2, -100, {"6202.138672 345.321625 12460.618164","6229.141602 324.048096 12345.915039","6243.221191 322.199188 12330.677734","6270.552246 321.703430 12319.665039","6328.999023 314.390503 12250.198242","6410.031738 308.038239 12158.772461","6431.396973 302.304260 12122.565430","6442.512695 292.479950 12018.642578","6463.594727 293.548492 11934.055664","6465.384277 296.752930 11823.707031","6453.285645 298.011658 11775.234375","6456.870605 300.292572 11729.112305","6471.326172 304.844543 11662.198242","6467.652832 300.664642 11626.468750","6442.675293 291.597717 11602.035156","6413.555176 281.592712 11573.769531","6410.750000 279.323822 11540.553711","6423.329590 279.653320 11504.419922","6448.427246 280.170074 11464.835938","6456.966309 278.921082 11429.042969","6472.223145 267.874329 11374.311523","6499.748047 259.875610 11319.764648","6512.123535 247.558929 11252.789063","6578.355469 238.905746 11170.990234","6606.053711 235.148514 11139.529297","6608.106934 229.014847 11104.818359","6599.475586 217.951920 11057.047852","6614.799316 210.816696 10975.557617","6632.123047 213.180832 10939.381836","6679.582031 223.034622 10921.174805","6712.383301 226.413910 10889.478516","6753.969238 221.894363 10870.875977","6796.062012 210.065323 10846.957031","6830.915527 198.424850 10825.327148","6878.863770 188.806351 10815.914063","6906.447266 187.208725 10842.923828","6920.634766 189.553574 10877.647461","6950.529785 190.788773 10885.879883","7004.051270 185.972092 10870.401367","7106.845703 186.488434 10838.797852","7106.845703 186.488327 10838.797852","7180.180664 184.444702 10788.323242","7208.601074 179.340225 10748.356445","7234.298340 171.834045 10696.221680","7274.218262 167.651443 10665.591797","7287.732422 164.585114 10644.990234","7285.643555 161.600769 10625.599609","7282.501953 158.098755 10602.527344","7295.586426 152.095001 10547.192383","7313.128418 151.273346 10504.957031","7326.812988 151.080078 10492.423828","7374.333008 149.995148 10487.060547","7410.066895 149.512665 10487.383789","7440.154785 150.985474 10504.989258","7466.247559 151.360413 10528.826172","7522.505371 150.131790 10549.182617","7560.093750 146.333908 10578.802734","7581.125000 148.227631 10609.394531","7602.015137 151.364273 10634.862305","7634.915039 153.057404 10650.686523","7669.272461 157.551178 10673.138672","7741.967773 164.167252 10679.934570","7777.560059 168.087082 10694.134766","7812.870117 167.605484 10685.429688","7840.303223 166.593826 10666.457031","7869.845703 166.382370 10668.013672","8026.387207 171.759354 10757.857422","8060.728516 169.945709 10763.327148","8071.536133 169.091324 10744.208008","8128.921875 169.051651 10705.557617","8175.679688 166.027542 10702.098633","8220.624023 165.371323 10673.463867","8301.456055 166.800491 10646.778320","8332.442383 160.706085 10663.897461","8355.316406 157.286987 10678.330078","8391.648438 160.890060 10626.557617","8426.968750 163.912552 10579.764648","8426.573242 170.825546 10547.739258","8403.417969 175.129471 10534.621094","8379.342773 181.323303 10515.273438","8375.875000 185.578552 10493.719727","8388.520508 188.747009 10473.718750","8398.215820 193.130661 10454.993164","8391.427734 197.760025 10436.585938","8375.029297 198.373383 10426.208984","8352.362305 198.413330 10421.707031","8328.804688 203.191223 10411.528320","8309.601563 212.042465 10384.977539","8311.858398 215.951538 10353.143555","8322.240234 215.848419 10304.883789","8337.522461 214.381653 10282.982422","8361.531250 215.751312 10251.543945","8356.890625 217.400070 10220.630859","8343.730469 214.510727 10190.852539","8321.241211 214.284760 10175.021484","8299.141602 215.740891 10160.070313","8284.049805 215.741898 10130.567383","8285.288086 208.997406 10094.094727","8322.694336 201.470154 10063.564453","8365.375977 193.846115 10039.197266","8412.299805 189.562653 10013.914063","8465.420898 187.729782 10004.166992","8497.033203 190.664673 9985.451172","8524.536133 196.210159 9938.729492","8542.442383 196.790512 9942.490234","8581.516602 181.000259 10001.314453","8618.755859 167.499161 10032.166016","8655.578125 160.442459 10076.380859","8668.973633 152.187119 10113.398438","8691.911133 148.567535 10126.292969","8715.178711 139.778397 10080.195313","8706.933594 129.682114 10030.741211","8685.197266 124.256424 10001.259766","8639.260742 113.499496 9947.524414","8619.245117 109.230354 9904.308594","8620.499023 103.639626 9860.453125","8643.027344 99.579987 9806.521484","8636.366211 92.402901 9759.613281","8619.595703 90.178818 9726.648438","8587.968750 90.169662 9729.569336","8550.750000 89.257507 9757.329102","8518.315430 89.347610 9761.166016","8499.183594 89.443863 9757.776367","8466.921875 89.286118 9737.913086","8434.358398 90.421776 9751.141602","8402.984375 91.663414 9774.312500","8383.630859 93.472664 9816.482422","8322.985352 92.791931 9883.106445","8191.746094 83.484909 9953.945313","8161.888672 79.489967 9968.033203","8121.907227 75.682961 9972.027344","8079.500488 72.002968 9970.732422","8034.942383 69.877647 9977.638672","7983.012207 68.337029 9964.451172","7932.417969 67.161781 9964.633789","7849.304199 58.296795 9946.168945","7809.474609 44.065323 9912.628906","7737.568848 15.064072 9799.327148","7727.638672 19.457527 9743.398438","7744.047852 30.447029 9691.614258","7773.502441 39.623726 9632.633789","7799.892578 35.816147 9582.921875","7795.454102 32.362160 9556.981445","7770.863281 30.346939 9541.246094","7727.119141 24.859253 9535.963867","7671.690430 12.477303 9518.131836","7630.869141 5.973797 9496.998047","7585.926270 5.772697 9454.497070","7529.269043 6.496822 9404.428711","7453.726563 13.237339 9329.285156","7424.702637 14.469864 9304.002930","7390.439453 12.436226 9296.210938","7337.773438 12.267272 9269.393555","7303.327637 14.818325 9194.083984","7267.643555 17.591663 9177.576172","7221.020508 24.133026 9132.542969","7178.157715 24.157520 9122.238281","7118.756836 14.397516 9090.702148","7086.331055 6.133612 9037.391602","7094.543457 5.152830 8976.301758","7108.181641 6.143047 8926.677734","7109.153320 6.143047 8853.937500","7115.529785 6.143082 8819.778320","7124.489258 6.129326 8806.489258","7295.987793 602.453064 11727.133789","7235.632813 602.461365 11729.445313","7220.677246 604.933044 11780.683594","7206.975586 604.839905 11802.013672","7182.456055 604.551453 11814.440430","7157.024902 604.713257 11845.578125","7156.565918 604.713135 11890.947266","7155.970215 604.713013 11917.059570","7141.772461 604.270447 11939.000977","7111.823730 599.848633 11949.428711","7070.614746 597.524841 11942.410156","7029.594238 591.112122 11969.536133","6983.071289 586.995544 11986.441406","6945.396484 581.700500 11993.677734","6896.416016 576.235962 12045.791992","6898.613281 573.272095 12082.774414","6924.391602 566.322021 12126.538086","6926.935547 564.454956 12149.121094","6913.149414 558.889954 12196.500977","6915.467285 552.885620 12230.815430","6936.224609 547.172363 12283.648438","6940.373047 545.822144 12310.492188","6989.173828 545.579834 12353.256836","6992.595215 545.033386 12369.198242","6981.243164 544.043640 12397.243164","6939.568848 544.551575 12434.422852","6934.239258 543.649658 12462.373047","6936.302246 541.681274 12490.804688","6919.399414 539.945557 12507.755859","6886.835449 536.096191 12512.227539","6739.784180 503.700226 12468.579102","6687.835449 491.556305 12440.481445","6659.958984 485.316132 12435.128906","6614.412598 478.013062 12450.490234","6554.704590 466.606140 12493.153320","6523.694824 456.086243 12560.924805","6502.363770 452.592834 12594.432617","6465.446777 444.003632 12622.412109","6393.275391 428.332947 12661.645508","6330.916016 416.516113 12700.784180","6292.637207 413.054443 12726.906250","6251.597168 407.305023 12718.796875","6220.489258 396.778870 12676.241211","6205.812500 383.552185 12630.454102"}));
                Patrols.Insert(new ExpansionAIPatrol(-3, "WALK", "SPRINT", "ALTERNATE", "East", "", true, false, 1.0, -1, -1, -2, -100, {"9130.47 55.8203 3942.87","9133.83 56.4846 3883.3","9096.23 51.9295 3807.21","9069.48 46.538 3716.73","9133.84 45.6782 3728.55","9162.2 46.251 3741.11","9196.48 47.2028 3691.01","9208.14 45.8609 3638.59","9225.12 39.9417 3596.74","9280.07 37.6949 3561.63","9330.12 46.8958 3566.72","9408.73 74.487 3611.83","9411.87 81.0733 3659.95","9383.19 77.4689 3659.53"}));
                Patrols.Insert(new ExpansionAIPatrol(-3, "WALK", "SPRINT", "ALTERNATE", "East", "", true, false, 1.0, -1, -1, -2, -100, {"12825.344727 4.894055 9906.302734","12815.158203 5.225951 9921.689453","12376.908203 7.238228 9932.627930","12326.162109 1.535592 9923.113281","12298.700195 1.795208 9927.007813","12276.838867 6.007958 9935.373047","12251.041992 7.321588 9936.276367","11746.091797 7.370661 9951.780273"}));
                Patrols.Insert(new ExpansionAIPatrol(-3, "WALK", "SPRINT", "ALTERNATE", "West", "", true, false, 1.0, -1, -1, -2, -100, {"2026.18 9.2025 8893.73","2021.73 9.2025 8827.45","1971.06 9.2025 8823.38","1926.41 9.2025 8824.34","1852.09 9.2025 8825.84","1834.99 9.20246 8856.11","1833.4 9.20245 8903.46","1880.59 9.2025 8903.19","1922.44 9.2025 8903.75","1945.39 9.20249 8935.69","1983.64 9.2025 8976.72","2037.48 9.2025 9030.04","1979.83 9.2025 8970.17","1924.56 9.2025 8904.36","1984.49 9.2025 8901.59","2024.51 9.20249 8903.54","2026.1 9.2025 8937.6","2008.6 9.20247 8938.48"}));
                Patrols.Insert(new ExpansionAIPatrol(-3, "JOG", "SPRINT", "ALTERNATE", "West", "", true, false, 1.0, -1, -1, -2, -100, {"2646.98 40.9853 13899.0","2673.88 40.2328 13899.8","2706.45 35.935 13930.6","2750.63 29.9544 13932.3","2764.93 27.4871 13906.1","2710.65 34.2131 13883.6","2690.22 33.142 13842.6","2708.58 27.2688 13811.3","2628.55 26.278 13781.1","2551.11 25.4771 13803.2","2518.63 24.1589 13836.7","2519.46 24.1838 13876.4","2510.15 24.0416 13865.0","2452.17 10.0884 13830.5","2432.55 6.40275 13776.1","2461.84 4.09093 13698.1","2520.72 7.97647 13651.5","2575.93 6.82799 13604.3","2621.36 14.8866 13609.6","2642.75 18.279 13719.5"}));
                Patrols.Insert(new ExpansionAIPatrol(-3, "WALK", "SPRINT", "ALTERNATE", "East", "", true, false, 1.0, -1, -1, -2, -100, {"657.714 30.4275 772.406","650.889 37.7634 864.345","592.248 57.6684 903.387","607.048 31.6412 921.698","575.976 30.3831 972.724","535.294 29.591 996.144","460.183 16.6023 956.739"}));
            break;
            case "enoch":
                Patrols.Insert( new ExpansionAIPatrol(1, 	"WALK", "SPRINT",    	"ALTERNATE",	"Raiders",    "", true, false, 1.0, -1, -1, -2, -100,  	{"6453.31 174.932 10981.6", "6534.43 174.578 11023.00", "6615.63 173.571 11067.5", "6687.14 174.054 11103.7"}));
                Patrols.Insert( new ExpansionAIPatrol(-3, 	"WALK", "SPRINT",    	"ALTERNATE", 	"Civilian",     "", true, false, 1.0, -1, -1, -2, -100, {"6303.47 381.472 3911.79", "6317.71 379.912 3870.99", "6330.94 380.986 3840.62", "6337.35 381.841 3819.01", "6344.2 382.049 3788.23", "6397.86 382.435 3715.53", "6413.77 381.767 3698.73", "6447.35 381.685 3674.81", "6474.48 381.403 3664.66", "6513.5 382.929 3663.82", "6584.13 379.569 3557.25", "6637.13 366.449 3490.86", "6652.16 357.021 3435.77", "6681.99 346.724 3395.99", "6712.61 328.543 3335.31", "6727.83 322.986 3324.47", "6744.66 319.581 3326.5", "6767.47 315.498 3338.96", "6792.34 311.333 3342.37", "6826.68 303.954 3331.26", "6867.14 299.576 3324.07", "6907.82 299.158 3298.6", "6938.85 298.278 3285.71", "6976.62 300.97 3247.94", "6987.2 300.871 3203.35", "7008.57 299.646 3181.55", "7095.26 296.592 3144.57", "7187.00 295.347 3085.46"}));
                Patrols.Insert( new ExpansionAIPatrol(-3, 	"WALK", "SPRINT",	 	"ALTERNATE", 	"East",     	"", true, false, 1.0, -1, -1, -2, -100, {"9272.91 199.589 10826.2", "9266.68 199.532 10876.1", "9270.63 199.459 10937.9", "9250.82 198.64 10983.2", "9164.87 198.142 11063.4"}));
                Patrols.Insert( new ExpansionAIPatrol(-3, 	"WALK", "SPRINT",    	"ALTERNATE", 	"East",			"", true, false, 1.0, -1, -1, -2, -100,    	{"9486.09 242.261 10360.6", "9435.31 242.442 10421.8", "9394.87 238.669 10507.00", "9382.44 232.412 10558.3", "9371.1 217.304 10683.0"}));
            break;
            case "namalsk":
                Patrols.Insert( new ExpansionAIPatrol(-3, 	"WALK", "SPRINT",     "ALTERNATE",	"West",    		"", true, false, 1.0, -1, -1, -2, -100,  	{"6307.7 14.4 11810.6", "6519.9 17.3 11901.6", "6967.4 6.5 11884.6", "7216.6 10.147 11843.1", "7405.78 23.18 11655.8"}));
                Patrols.Insert( new ExpansionAIPatrol(-3, 	"WALK", "SPRINT",     "ALTERNATE", 	"West",    		"", true, false, 1.0, -1, -1, -2, -100,  	{"5028.91 7.42 11359.5", "4971.8 9.96 11305.5", "4852.96 13.88 11257.2", "4727.41 14.78 11099.7", "4690.78 21.86 10948.9", "4513.78 22.17 10819", "4333.31 15.53 11054.3", "4269.85 9.43 11046"}));
                Patrols.Insert( new ExpansionAIPatrol(-3, 	"WALK", "SPRINT",     "ALTERNATE", 	"West",    		"", true, false, 1.0, -1, -1, -2, -100,  	{"4492.01 3.43 10429.7", "4533.44 5.01 10371.7", "4624.5 13.35 10400.1", "4865.77 10.34 10446.7", "4964.16 2.78 10533.5", "5129.82 9.57 10510.6", "5315.19 15.77 10591", "5493.19 5.28 10593", "5524.4 14.20 10447.9", "5483.9 21.08 10348.9", "5389.42 4.64 10169.5", "5547.42 5.48 10047", "5637.03 1.09 9960", "5650.19 2.91 9899.06", "5574.11 8.27 9843.13", "5343.65 10.77 9940.64", "5154.59 14.7 9905.64", "4920.51 11.26 9685.87", "4749.81 7.86 9834.98", "4795.58 6.04 10146.9", "4580.36 6.88 10154", "4398.7 5.98 10033.1"}));
                Patrols.Insert( new ExpansionAIPatrol(-3, 	"WALK", "SPRINT",     "ALTERNATE", 	"West",    		"", true, false, 1.0, -1, -1, -2, -100,  	{"6805.86 15.01 11660.4", "6867.16 15.13 11474", "6826.18 15.12 11427.5", "6765.64 15.01 11304.2", "6713.69 15 11202.7", "6605.51 15.2 10994", "6625.24 15.94 10943.1", "6634.26 16.12 10924.8", "6583.29 20.79 10864.8"}));
                Patrols.Insert( new ExpansionAIPatrol(-3, 	"WALK", "SPRINT",     "ALTERNATE",  "West",    		"", true, false, 1.0, -1, -1, -2, -100,  	{"7052.16 6.39 10051.4", "6997.08 4.0 10063.2", "6955.39 4.45 9904.53", "6881.63 5.3 9743.56", "6760.12 9.25 9758.2", "6686.01 11.36 9858.15", "6658.22 9.26 9958.59", "6762.43 7.05 10087"}));
                Patrols.Insert( new ExpansionAIPatrol(-3, 	"WALK", "SPRINT",     "ALTERNATE",  "West",    		"", true, false, 1.0, -1, -1, -2, -100,  	{"4756.11 153.16 7399.41", "5243.13 209.35 7437.1", "5272.81 271.206 7190.86", "5270.04 256.98 7017.9", "5456.37 196.30 6618.96", "5677.01 107.66 6280.14"}));
                Patrols.Insert( new ExpansionAIPatrol(-3, 	"WALK", "SPRINT",     "ALTERNATE",  "West",    		"", true, false, 1.0, -1, -1, -2, -100,  	{"7674.4 75.96 8643.82", "7864.4 41.8 8563.62", "7965.19 30 8887.57", "7696.5 47.78 9120.63", "7315.1 26.6 8972.65"}));
                Patrols.Insert( new ExpansionAIPatrol(-3, 	"WALK", "SPRINT",     "ALTERNATE",  "West",    		"", true, false, 1.0, -1, -1, -2, -100,  	{"7977.68 3.93 7430.08", "7731.84 54.31 7202.23", "7475.49 72.2477 7486.18", "7667.53 33.30 7918.47", "7888.6 3.20 7997.02"}));
                Patrols.Insert( new ExpansionAIPatrol(-2, 	"WALK", "SPRINT",     "ALTERNATE",  "West",    		"", true, false, 1.0, -1, -1, -2, -100,  	{"5301.82 58.87 9602.39", "5548.07 43.18 9543.59", "5725.36 19.7 9450.14", "5476.45 29.84 9738.32"}));
                Patrols.Insert( new ExpansionAIPatrol(-2, 	"WALK", "SPRINT",     "ALTERNATE",  "West",    		"", true, false, 1.0, -1, -1, -2, -100,  	{"1198.73 2 11635.4", "1353.84 2 11604", "1351.76 2 11960.4", "1112.03 2 12009.2", "1112.48 8.92 11898.3"}));
                Patrols.Insert( new ExpansionAIPatrol(-3, 	"WALK", "SPRINT",     "ALTERNATE",  "West",    		"", true, false, 1.0, -1, -1, -2, -100, 	{"5407.11 65.41 6072.06", "5246 40.67 6230.45", "4805 88.73 6460.97", "4785.95 96.61 6655.96"}));
                Patrols.Insert( new ExpansionAIPatrol(-2, 	"WALK", "SPRINT",     "ALTERNATE",  "West",    		"", true, false, 1.0, -1, -1, -2, -100,  	{"2914.75 4.57 11032.5", "2442.75 2 11160.6", "1997.79 2 10807", "2762.99 2 10394.4"}));
                Patrols.Insert( new ExpansionAIPatrol(-2, 	"WALK", "SPRINT",     "ALTERNATE",  "West",    		"", true, false, 1.0, -1, -1, -2, -100,  	{"5304.6 40.85 10803.6", "5612.07 47.78 10952.4", "5713.27 58.28 11090.8", "6148.22 7.7 11587.3"}));
                Patrols.Insert( new ExpansionAIPatrol(-2, 	"WALK", "SPRINT",     "ALTERNATE",  "West",    		"", true, false, 1.0, -1, -1, -2, -100,  	{"4717.85 111.52 7672.94", "4311.86 129.89 7290.57", "3957.02 106.42 7239.99", "3498.03 139.88 6967.42", "3241.25 104.45 7299.5", "3663.37 90.21 7627.72", "3894.5 78.88 7776.37"}));
            break;
            case "takistanplus":
                Patrols.Insert( new ExpansionAIPatrol(1, 	"WALK", "SPRINT",     "ALTERNATE",  "Raiders",    "", true, false, 1.0, -1, -1, -2, -100,  {"1916.59 208.159 4010.34","1895.84 209.13 3960.54", "1877.69 209.431 3888.43", "1858.52 211.273 3855.79", "1805.82 211.543 3797.75", "1600.85 212.617 3613.8"}));
                Patrols.Insert( new ExpansionAIPatrol(1, 	"WALK", "SPRINT",     "ALTERNATE",  "Raiders",    "", true, false, 1.0, -1, -1, -2, -100, 	{"8030.88 158.835 7929.06", "7963.1 157.233 7962.07", "7814.7 153.932 8005.98", "7701.14 150.676 8071.15", "7600.04 147.619 8144.57", "7518.89 144.197 822.29", "7461.86 143.722 8296.28", "7390.85 141.246 8414.14"}));
                Patrols.Insert( new ExpansionAIPatrol(1,	"WALK", "SPRINT",     "ALTERNATE",  "Raiders",    "", true, false, 1.0, -1, -1, -2, -100, 	{"7555.1 288.249 1897.49", "7586.85 288.08 1907.81", "7695.77 289.716 1982.37", "7868.56 297.397 2086.09", "8066.59 296.357 2194.71"}));
            break;
            case "sakhal":
                Patrols.Insert( new ExpansionAIPatrol(-3, 	"WALK", "SPRINT",     "ALTERNATE",  "East",    "", true, false, 0.5, -1, -1, -2, -100,  {"8288.998 214.898 12107.159", "7591.821 144.462 11994.015", "7194.002 153.701 11617.268", "7781.733 152.445 11257.704", "8204.424 97.679 10884.202", "8453.134 28.790 10419.384", "8009.339 23.783 9666.985"}));
                Patrols.Insert( new ExpansionAIPatrol(-3, 	"WALK", "SPRINT",     "ALTERNATE",  "East",    "", true, false, 0.5, -1, -1, -2, -100,  {"10200.659 255.569 8671.933", "10558.350 194.310 8343.568", "11213.360 91.887 7881.493", "10531.717 26.432 7083.338", "9452.423 41.868 7515.399", "8581.497 49.547 7691.303", "8117.741 34.458 8017.024", "8214.167 70.111 8699.115", "8620.819 98.713 9060.422", "8285.052 20.001 9695.093", "8746.909 22.127 10125.139", "9311.235 51.971 9833.982", "9987.700 131.807 9327.429", "10031.570 210.613 8962.563", "9906.058 239.776 8792.247"}));
                Patrols.Insert( new ExpansionAIPatrol(-3, 	"WALK", "SPRINT",     "ALTERNATE",  "East",    "", true, false, 0.5, -1, -1, -2, -100,  {"4787.609 28.300 4245.399", "5014.234 28.141 3898.959", "5442.419 2.579 4121.542", "5669.906 3.021 4564.939", "5123.237 2.728 4902.908", "4751.950 3.085 4899.631"}));
                Patrols.Insert( new ExpansionAIPatrol(-3, 	"WALK", "SPRINT",     "ALTERNATE",  "East",    "", true, false, 0.5, -1, -1, -2, -100,  {"3501.150 32.635 6567.945", "3381.435 3.170 6797.855", "3248.667 3.170 7064.388", "3023.729 3.170 7220.322", "2799.487 5.791 7303.858", "2543.604 10.244 7389.098", "2067.675 37.134 7401.283", "2132.727 29.908 6962.922", "1989.028 32.602 6976.982", "1657.325 6.066 6890.798", "1830.458 2.716 6548.650", "1969.076 2.620 6119.752", "2417.135 7.772 5891.624", "2949.798 19.185 6009.204", "3012.716 43.775 6567.360", "2918.819 36.214 6914.654"}));
				
                Patrols.Insert( new ExpansionAIPatrol(-2, 	"WALK", "SPRINT",     "ALTERNATE",  "Raiders",    "", true, false, 0.5, -1, -1, -2, -100,  {"6762.087 2.909 7135.547", "6925.275 2.460 7130.842", "6961.268 2.660 7321.813", "7021.689 2.660 7486.428", "6949.924 2.475 7549.416", "7005.497 2.660 7667.544", "7185.033 2.630 7835.801", "7400.235 2.630 7772.836", "7480.426 4.728 7298.641", "7341.819 2.704 7371.494", "7299.538 2.877 7159.987", "7041.316 3.792 7058.205"}));
                Patrols.Insert( new ExpansionAIPatrol(-2, 	"WALK", "SPRINT",     "ALTERNATE",  "Raiders",    "", true, false, 0.5, -1, -1, -2, -100,  {"8208.648 30.675 10260.268", "8288.153 30.790 10311.540", "8339.283 30.769 10384.679", "8452.334 30.790 10307.793", "8434.165 30.790 10253.832", "8353.144 30.790 10267.011", "8380.400 30.790 10200.639"}));
                Patrols.Insert( new ExpansionAIPatrol(-2, 	"WALK", "SPRINT",     "ALTERNATE",  "Raiders",    "", true, false, 0.5, -1, -1, -2, -100,  {"7751.106 2.610 7940.767", "7806.555 2.044 7863.530", "7936.524 6.189 7796.568", "7943.852 7.066 7716.451", "7819.790 6.466 7711.988", "7732.433 2.743 7790.704", "7605.634 2.600 7887.587", "7671.484 4.056 7705.067", "7517.055 2.630 7784.107", "7464.175 2.630 7845.798", "7400.849 2.630 7897.020", "7412.900 2.630 7752.343", "7389.504 2.750 7990.607", "7379.863 2.750 8058.280", "7319.713 2.830 8028.648", "7211.844 6.530 8060.472", "7155.630 6.530 8056.667", "7220.532 2.649 7872.838", "7252.780 2.630 7838.619", "7479.202 2.630 7705.203", "7768.701 6.517 7712.756", "7939.187 6.490 7892.674", "7886.628 6.933 7976.605"}));

				// Disabled by default -- Roaming AI
                Patrols.Insert( new ExpansionAIPatrol(1, 	"WALK", "SPRINT",     "ROAMING",  "Raiders",    "", true, false, 0.0, -1, -1, -2, -100,  {"6654.610 5.285 13392.426", "7229.590 5.617 13474.621", "7571.210 9.701 13599.684", "7927.490 3.583 13581.710", "8278.823 5.686 13684.717",}));
                Patrols.Insert( new ExpansionAIPatrol(1, 	"WALK", "SPRINT",     "ROAMING",  "Raiders",    "", true, false, 0.0, -1, -1, -2, -100,  {"8406.119 2.200 13767.616", "8601.439 2.961 13829.995", "8821.770 1.761 13787.972", "8957.258 2.035 13805.658", "9769.307 1.611 13607.958", "9984.609 1.905 13577.867", "10141.311 2.204 13554.617"}));
                Patrols.Insert( new ExpansionAIPatrol(1, 	"WALK", "SPRINT",     "ROAMING",  "Raiders",    "", true, false, 0.0, -1, -1, -2, -100,  {"10564.874 1.976 13486.112", "10833.917 2.183 13445.404", "11051.165 2.631 13314.489", "11262.787 2.101 13158.023", "11409.177 2.358 13238.533", "11589.211 1.480 13200.708", "11783.768 2.292 13085.467"}));
                Patrols.Insert( new ExpansionAIPatrol(1, 	"WALK", "SPRINT",     "ROAMING",  "Raiders",    "", true, false, 0.0, -1, -1, -2, -100,  {"12065.160 2.509 13038.068", "12256.508 2.337 12905.627", "12393.394 2.432 12785.816", "12548.587 2.669 12796.528", "12623.583 1.445 12699.051", "12757.104 1.999 12573.312", "12950.847 2.009 12544.023"}));
                Patrols.Insert( new ExpansionAIPatrol(1, 	"WALK", "SPRINT",     "ROAMING",  "Raiders",    "", true, false, 0.0, -1, -1, -2, -100,  {"13123.501 2.550 12463.987", "13233.468 1.569 12245.814", "13381.938 2.878 12313.685", "13541.592 2.506 12321.443", "14181.143 4.154 11931.988", "14286.049 2.296 11675.615", "14516.447 1.637 11455.939"}));
                Patrols.Insert( new ExpansionAIPatrol(1, 	"WALK", "SPRINT",     "ROAMING",  "Raiders",    "", true, false, 0.0, -1, -1, -2, -100,  {"14568.992 2.549 11191.166", "14629.071 2.185 11052.888", "14707.979 2.336 10866.926", "14826.339 2.140 10647.543", "14790.296 1.346 10402.294", "14715.979 1.535 10102.405", "14673.604 2.118 9534.150"}));
                Patrols.Insert( new ExpansionAIPatrol(1, 	"WALK", "SPRINT",     "ROAMING",  "Raiders",    "", true, false, 0.0, -1, -1, -2, -100,  {"14654.053 1.658 9211.176", "14428.208 1.734 8677.497", "14355.728 5.904 8499.604", "14135.915 1.340 8400.694", "14024.399 2.168 8073.576", "14045.272 5.774 7887.219", "13732.162 2.992 7798.370"}));
                Patrols.Insert( new ExpansionAIPatrol(1, 	"WALK", "SPRINT",     "ROAMING",  "Raiders",    "", true, false, 0.0, -1, -1, -2, -100,  {"13661.135 14.905 7654.860", "13713.676 4.684 7437.048", "13530.539 3.168 7344.184", "13287.767 1.435 7470.301", "13078.437 2.526 7459.525", "12838.097 2.333 7126.293", "12804.625 3.170 6936.801"}));
                Patrols.Insert( new ExpansionAIPatrol(1, 	"WALK", "SPRINT",     "ROAMING",  "Raiders",    "", true, false, 0.0, -1, -1, -2, -100,  {"12810.851 16.385 6725.023", "12425.978 13.015 6639.098", "12048.669 2.631 6525.326", "11592.789 5.820 6341.671", "11257.915 5.949 5846.341", "10962.493 2.040 6052.329", "10708.177 2.171 6317.607"}));
                Patrols.Insert( new ExpansionAIPatrol(1, 	"WALK", "SPRINT",     "ROAMING",  "Raiders",    "", true, false, 0.0, -1, -1, -2, -100,  {"10393.715 2.478 6187.195", "9953.999 1.557 6290.409", "9591.376 1.708 6449.003", "9459.974 1.599 6307.580", "8982.282 2.135 6393.391", "8469.981 2.931 6503.731", "8007.057 1.912 6760.490"}));
                Patrols.Insert( new ExpansionAIPatrol(1, 	"WALK", "SPRINT",     "ROAMING",  "Raiders",    "", true, false, 0.0, -1, -1, -2, -100,  {"7628.999 4.402 6656.849", "7432.771 2.335 6985.986", "6770.729 3.064 6894.994", "6444.311 2.632 7009.784", "6193.438 2.546 7162.027", "5931.584 2.243 7291.747", "5628.142 1.968 7489.789"}));
                Patrols.Insert( new ExpansionAIPatrol(1, 	"WALK", "SPRINT",     "ROAMING",  "Raiders",    "", true, false, 0.0, -1, -1, -2, -100,  {"5364.644 1.978 7650.386", "5272.702 1.796 8261.495", "4905.257 2.705 8524.025", "4686.263 2.252 8453.321", "4549.524 2.192 8217.537", "4333.692 2.569 8092.424", "4026.226 1.661 8275.648"}));
                Patrols.Insert( new ExpansionAIPatrol(1, 	"WALK", "SPRINT",     "ROAMING",  "Raiders",    "", true, false, 0.0, -1, -1, -2, -100,  {"4327.793 2.674 8790.745", "4769.151 1.415 9713.441", "4842.771 2.802 10198.465", "4992.027 2.830 10244.188", "5118.587 2.049 10271.950", "4717.816 1.971 11341.220", "4999.015 1.473 11515.101"}));
                Patrols.Insert( new ExpansionAIPatrol(1, 	"WALK", "SPRINT",     "ROAMING",  "Raiders",    "", true, false, 0.0, -1, -1, -2, -100,  {"5130.658 2.198 11705.246", "5412.902 2.065 11960.760", "5498.660 2.005 12089.734", "5473.788 2.003 12215.852", "5499.650 3.337 12478.439", "5850.876 2.695 12777.014", "5958.480 3.254 12828.792"}));
			break;
            default:
                if ( GetExpansionSettings().GetLog().AIPatrol )
                    GetExpansionSettings().GetLog().PrintLog("[AI Patrol] WARNING: The map \""+worldName+"\" doesn't have a default config");
                    GetExpansionSettings().GetLog().PrintLog("[AI Patrol] Generating an example config...");

				//! Default patrols are deliberately designed to be opposing factions and run into each other eventually :-)
                Patrols.Insert( new ExpansionAIPatrol(-3, 	"WALK", "SPRINT",     "LOOP",     "West",     	"", true, false, 1.0, -1, -1, -2, -100, {"6000 0 6000", "6000 0 6500", "6500 0 6500", "6500 0 6000"}));
                Patrols.Insert( new ExpansionAIPatrol(-3, 	"WALK", "SPRINT",     "LOOP",     "East",     	"", true, false, 1.0, -1, -1, -2, -100, {"6500 0 6500", "6000 0 6500", "6000 0 6000", "6500 0 6000"}));
                Patrols.Insert( new ExpansionAIPatrol(1, 	"SPRINT", "SPRINT",   "ALTERNATE",     "Raiders",	"", true, false, 1.0, -1, -1, -2, -100, {"6000 0 6000", "6250 0 6250", "6500 0 6500"}));
                return;
            break;
        }
        if ( GetExpansionSettings().GetLog().AIPatrol )
            GetExpansionSettings().GetLog().PrintLog("[AI Patrol] Generating default Patrol config for \""+worldName+"\"");
        
    }
	
	// ------------------------------------------------------------
	override string SettingName()
	{
		return "AI Patrol Settings";
	}
};
