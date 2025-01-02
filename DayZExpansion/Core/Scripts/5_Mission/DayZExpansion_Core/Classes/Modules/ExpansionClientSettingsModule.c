/**
 * ExpansionClientSettingsModule.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

[CF_RegisterModule(ExpansionClientSettingsModule)]
class ExpansionClientSettingsModule : CF_ModuleWorld
{
	static ref ScriptInvoker m_SettingChanged = new ScriptInvoker;

	override void OnInit()
	{
		super.OnInit();

		EnableClientLogout();
		EnableInvokeConnect();
		EnableSettingsChanged();
	}

	override bool IsServer()
	{
		return false;
	}

	override bool IsClient()
	{
		return true;
	}

	override void OnInvokeConnect(Class sender, CF_EventArgs args)
	{
		super.OnInvokeConnect(sender, args);

		auto cArgs = CF_EventPlayerArgs.Cast(args);

		OnSettingsChanged(this, CF_EventArgs.Empty);
	}
	
#ifdef DAYZ_1_25
	override void OnSettingsChanged(Class sender, CF_EventArgs args)
	{
		super.OnSettingsChanged(sender, args);

		MissionBaseWorld missionWorld = MissionBaseWorld.Cast(GetGame().GetMission());
		if (missionWorld)
		{
			missionWorld.Expansion_UpdateWorldViewDistances();
		}
	}
#endif
};
