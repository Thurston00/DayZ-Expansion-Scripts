/**
 * ExpansionGeneralSettings.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

/**@class		ExpansionGeneralSettings
 * @brief		General settings class
 **/
class ExpansionGeneralSettings: ExpansionSettingBase
{
	static const int VERSION = 13;

	bool DisableShootToUnlock;
	bool EnableGravecross;
	bool GravecrossDeleteBody;
	float GravecrossTimeThreshold;
	float GravecrossSpawnTimeDelay;
	ref ExpansionMapping Mapping;
	LampModeEnum EnableLamps;										//! 0 - Disable Street-Lights | 1 - Will use and spawn Generators for Street-Lights | 2 - Street-Lights always on
	bool EnableGenerators;
	bool EnableLighthouses;
	bool EnableHUDNightvisionOverlay;
	bool DisableMagicCrosshair;
	bool EnableAutoRun;
	bool UseDeathScreen;
	bool UseDeathScreenStatistics;
	bool UseExpansionMainMenuLogo;
	bool UseExpansionMainMenuIcons;
	bool UseExpansionMainMenuIntroScene;
	bool UseNewsFeedInGameMenu;

	ref ExpansionHudIndicatorColors HUDColors;
	
	bool EnableEarPlugs;
	string InGameMenuLogoPath;
	
	[NonSerialized()]
	private bool m_IsLoaded;

	// ------------------------------------------------------------
	void ExpansionGeneralSettings()
	{
		HUDColors = new ExpansionHudIndicatorColors;
		Mapping = new ExpansionMapping;
	}
	
	// ------------------------------------------------------------
	override bool OnRecieve( ParamsReadContext ctx )
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.SETTINGS, this, "OnRecieve");
#endif

		ctx.Read(DisableShootToUnlock);
		ctx.Read(EnableGravecross);
		ctx.Read(GravecrossDeleteBody);
		ctx.Read(GravecrossTimeThreshold);
		ctx.Read(GravecrossSpawnTimeDelay);		

		GetDayZGame().GetExpansionGame().ReadRemovedWorldObjects(ctx);

		int interiorCount;
		ctx.Read(interiorCount);
		Mapping.BuildingInteriors = interiorCount > 0;
		while (interiorCount)
		{
			string interior;
			ctx.Read(interior);
			Mapping.Interiors.Insert(interior);
			interiorCount--;
		}

		ctx.Read(Mapping.BuildingIvys);

		ctx.Read(EnableLamps);
		ctx.Read(EnableGenerators);
		ctx.Read(EnableLighthouses);
		ctx.Read(EnableHUDNightvisionOverlay);
		ctx.Read(DisableMagicCrosshair);
		ctx.Read(EnableAutoRun);
		ctx.Read(UseDeathScreen);
		ctx.Read(UseDeathScreenStatistics);
		ctx.Read(UseExpansionMainMenuLogo);
		ctx.Read(UseExpansionMainMenuIcons);
		ctx.Read(UseExpansionMainMenuIntroScene);
		ctx.Read(UseNewsFeedInGameMenu);

		HUDColors.OnReceive(ctx);
		HUDColors.Update();
		
		ctx.Read(EnableEarPlugs);
		ctx.Read(InGameMenuLogoPath);

		m_IsLoaded = true;

		ExpansionSettings.SI_General.Invoke();
		
		return true;
	}
	
	override void OnSend( ParamsWriteContext ctx )
	{
		ctx.Write(DisableShootToUnlock);
		ctx.Write(EnableGravecross);
		ctx.Write(GravecrossDeleteBody);
		ctx.Write(GravecrossTimeThreshold);
		ctx.Write(GravecrossSpawnTimeDelay);		

		GetDayZGame().GetExpansionGame().WriteRemovedWorldObjects(ctx);

		//! Do not send mapping, but do send interiors if enabled as interiors w/o collision are created only clientside
		if (Mapping.BuildingInteriors)
		{
			ctx.Write(Mapping.Interiors.Count());
			foreach (string interior: Mapping.Interiors)
			{
				ctx.Write(interior);
			}
		}
		else
		{
			ctx.Write(0);
		}

		//! Ivys are created only clientside
		ctx.Write(Mapping.BuildingIvys);

		ctx.Write(EnableLamps);
		ctx.Write(EnableGenerators);
		ctx.Write(EnableLighthouses);
		ctx.Write(EnableHUDNightvisionOverlay);
		ctx.Write(DisableMagicCrosshair);
		ctx.Write(EnableAutoRun);
		ctx.Write(UseDeathScreen);
		ctx.Write(UseDeathScreenStatistics);
		ctx.Write(UseExpansionMainMenuLogo);
		ctx.Write(UseExpansionMainMenuIcons);
		ctx.Write(UseExpansionMainMenuIntroScene);
		ctx.Write(UseNewsFeedInGameMenu);

		HUDColors.OnSend(ctx);
		
		ctx.Write(EnableEarPlugs);
		ctx.Write(InGameMenuLogoPath);
	}

	// ------------------------------------------------------------
	override int Send( PlayerIdentity identity )
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_1(ExpansionTracing.SETTINGS, this, "Send").Add(identity);
#endif

		if ( !IsMissionHost() )
		{
			return 0;
		}
		
		auto rpc = CreateRPC();
		OnSend( rpc );
		rpc.Expansion_Send(true, identity);
		
		return 0;
	}

	// ------------------------------------------------------------
	override bool Copy( ExpansionSettingBase setting )
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_1(ExpansionTracing.SETTINGS, this, "Copy").Add(setting);
#endif

		ExpansionGeneralSettings settings = ExpansionGeneralSettings.Cast( setting );
		if ( !settings )
			return false;

		CopyInternal( settings );
		
		return true;
	}

	// ------------------------------------------------------------
	private void CopyInternal(  ExpansionGeneralSettings s )
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_1(ExpansionTracing.SETTINGS, this, "CopyInternal").Add(s);
#endif
		
		EnableGravecross = s.EnableGravecross;
		DisableShootToUnlock = s.DisableShootToUnlock;
		GravecrossDeleteBody = s.GravecrossDeleteBody;
		GravecrossTimeThreshold = s.GravecrossTimeThreshold;
		GravecrossSpawnTimeDelay = s.GravecrossSpawnTimeDelay;
		Mapping.Copy( s.Mapping );
		EnableLamps = s.EnableLamps;
		EnableGenerators = s.EnableGenerators;
		EnableLighthouses = s.EnableLighthouses;
		EnableHUDNightvisionOverlay = s.EnableHUDNightvisionOverlay;
		DisableMagicCrosshair = s.DisableMagicCrosshair;
		EnableAutoRun = s.EnableAutoRun;
		UseDeathScreen = s.UseDeathScreen;
		UseDeathScreenStatistics = s.UseDeathScreenStatistics;
		UseNewsFeedInGameMenu = s.UseNewsFeedInGameMenu;
		HUDColors = s.HUDColors;
		EnableEarPlugs = s.EnableEarPlugs;
		InGameMenuLogoPath = s.InGameMenuLogoPath;
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

		bool generalSettingsExist = FileExist(EXPANSION_GENERAL_SETTINGS);

		if (generalSettingsExist)
		{
			EXPrint("[ExpansionGeneralSettings] Load existing setting file:" + EXPANSION_GENERAL_SETTINGS);
			
			ExpansionGeneralSettings settingsDefault = new ExpansionGeneralSettings;
			settingsDefault.Defaults();

			JsonFileLoader<ExpansionGeneralSettings>.JsonLoadFile(EXPANSION_GENERAL_SETTINGS, this);

			if (m_Version < VERSION)
			{
				EXPrint("[ExpansionGeneralSettings] Load - Converting v" + m_Version + " \"" + EXPANSION_GENERAL_SETTINGS + "\" to v" + VERSION);

				if (m_Version < 7)
				{
					//! Remove buildings without custom interiors from existing settings
					Mapping.Interiors.RemoveItem( "Land_City_School" );
					Mapping.Interiors.RemoveItem( "Land_Village_Pub" );
					Mapping.Interiors.RemoveItem( "Land_House_1B01_Pub" );
					Mapping.Interiors.RemoveItem( "Land_House_1W03" );
					Mapping.Interiors.RemoveItem( "Land_House_1W04" );
					Mapping.Interiors.RemoveItem( "Land_House_1W05" );
					Mapping.Interiors.RemoveItem( "Land_House_1W08" );
					Mapping.Interiors.RemoveItem( "Land_House_1W09" );
					Mapping.Interiors.RemoveItem( "Land_House_1W10" );
					Mapping.Interiors.RemoveItem( "Land_House_1W12" );
					Mapping.Interiors.RemoveItem( "Land_House_1W03_Brown" );
					Mapping.Interiors.RemoveItem( "Land_House_1W04_Yellow" );
					Mapping.Interiors.RemoveItem( "Land_House_1W05_Yellow" );
					Mapping.Interiors.RemoveItem( "Land_House_1W08_Brown" );
					Mapping.Interiors.RemoveItem( "Land_House_1W09_Yellow" );
					Mapping.Interiors.RemoveItem( "Land_House_1W10_Brown" );
					Mapping.Interiors.RemoveItem( "Land_House_1W12_Brown" );
					Mapping.Interiors.RemoveItem( "Land_House_2W04" );
					Mapping.Interiors.RemoveItem( "Land_House_2W03_Brown" );
					Mapping.Interiors.RemoveItem( "Land_House_2W04_Yellow" );
					Mapping.Interiors.RemoveItem( "Land_Lighthouse" );
					Mapping.Interiors.RemoveItem( "Land_Power_Station" );
				}

				if (m_Version < 8)
				{
					HUDColors = settingsDefault.HUDColors;
				}
				
				if (m_Version < 9)
				{
					EnableEarPlugs = settingsDefault.EnableEarPlugs;
					InGameMenuLogoPath = settingsDefault.InGameMenuLogoPath;
				}

				if (m_Version < 10)
				{
					GravecrossSpawnTimeDelay = settingsDefault.GravecrossSpawnTimeDelay;
				}

				if (m_Version < 11)
				{
					UseExpansionMainMenuLogo = settingsDefault.UseExpansionMainMenuLogo;
					UseExpansionMainMenuIcons = settingsDefault.UseExpansionMainMenuIcons;
					UseExpansionMainMenuIntroScene = settingsDefault.UseExpansionMainMenuIntroScene;
				}

				m_Version = VERSION;
				save = true;
			}
			else
			{
				JsonFileLoader<ExpansionGeneralSettings>.JsonLoadFile(EXPANSION_GENERAL_SETTINGS, this);
			}

			HUDColors.Update();
		}
		else
		{
			EXPrint("[ExpansionGeneralSettings] No existing setting file:" + EXPANSION_GENERAL_SETTINGS + ". Creating defaults!");
			Defaults();
			save = true;
		}

		if (save)
			Save();
		
		return generalSettingsExist;
	}
	
	// ------------------------------------------------------------
	override bool OnSave()
	{
		Print("[ExpansionGeneralSettings] Saving settings");

		JsonFileLoader<ExpansionGeneralSettings>.JsonSaveFile( EXPANSION_GENERAL_SETTINGS, this );

		return true;
	}
	
	// ------------------------------------------------------------
	override void Update( ExpansionSettingBase setting )
	{
		super.Update( setting );

		ExpansionSettings.SI_General.Invoke();
	}

	// ------------------------------------------------------------
	override void Defaults()
	{
		m_Version = VERSION;
		
		DisableShootToUnlock = false;
		EnableGravecross = false;
		GravecrossDeleteBody = true;
		GravecrossTimeThreshold = 1200;
		GravecrossSpawnTimeDelay = 180;
		
		Mapping.Defaults();

		EnableLamps = LampModeEnum.AlwaysOnEverywhere;
		EnableGenerators = false;
		EnableLighthouses = true;
		
		EnableHUDNightvisionOverlay = true;

		DisableMagicCrosshair = true;
		
		EnableAutoRun = true;
		
		UseDeathScreen = true;
		UseDeathScreenStatistics = true;
		
		UseExpansionMainMenuLogo = true;
		UseExpansionMainMenuIcons = true;
		UseExpansionMainMenuIntroScene = true;
		UseNewsFeedInGameMenu = true;

		HUDColors.Update();
		
		EnableEarPlugs = true;
		InGameMenuLogoPath = "set:expansion_iconset image:logo_expansion_white";
	}
	
	// ------------------------------------------------------------
	override string SettingName()
	{
		return "General Settings";
	}
};
