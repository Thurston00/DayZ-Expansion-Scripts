/**
 * ExpansionActionCrackSafe.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

class ExpansionActionCrackSafe : ExpansionActionDestroyBase
{
	override bool SetupCondition()
	{
		return GetExpansionSettings().GetRaid(false).IsLoaded();
	}

	override void Setup( PlayerBase player, ActionTarget target, ItemBase item )
	{
		m_Time = GetExpansionSettings().GetRaid().SafeRaidToolTimeSeconds;
		m_Cycles = GetExpansionSettings().GetRaid().SafeRaidToolCycles;
		m_MinHealth01 = 0.01;  //! 1% health
		m_ToolDamagePercent = GetExpansionSettings().GetRaid().SafeRaidToolDamagePercent;
	}

	override string GetText()
	{
		return "#STR_EXPANSION_UA_CRACK_SAFE";
	}

	override bool DestroyCondition( PlayerBase player, ActionTarget target, ItemBase item, bool camera_check )
	{
		if ( !super.DestroyCondition( player, target, item, camera_check ) )
			return false;

		ExpansionSafeBase safe = ExpansionSafeBase.Cast( target.GetObject() );

		if ( !safe || !safe.ExpansionIsLocked() )
			return false;

		if ( !ExpansionStatic.IsAnyOf(item, GetExpansionSettings().GetRaid().SafeRaidTools) )
			return false;

		return true;
	}

	override bool CanBeDestroyed( Object targetObject )
	{
		auto settings = GetExpansionSettings().GetRaid();
		return settings.CanRaidSafes && (!settings.SafeRaidUseSchedule || settings.IsRaidableNow());
	}

	override bool Expansion_CheckSuccess(ActionData action_data)
	{
		ExpansionSafeBase safe;
		if (Class.CastTo(safe, action_data.m_Target.GetObject()))
			return !safe.ExpansionIsLocked();

		return false;
	}
}
