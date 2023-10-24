/**
 * ExpansionQuestItem_Logs.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

#ifdef NAMALSK_SURVIVAL
/*#ifdef EXPANSIONMODQUESTS
modded class dzn_athena_planning
{
	override void DeferredInit()
	{
		super.DeferredInit();

		Expansion_SetQuestID(800);
		Expansion_SetIsQuestGiver(true);
	}
};

modded class dzn_athena_action1
{

};

modded class dzn_athena_action2
{

};

modded class dzn_netanyas_log
{

};

modded class dzn_athena3_august
{

};

modded class dzn_athena3_exp1
{

};

modded class dzn_athena3_exp2
{

};

modded class dzn_athena3_october
{

};

modded class dzn_phoenix_log_1
{

};

modded class dzn_phoenix_log_2
{

};

modded class dzn_phoenix_log_3
{

};

modded class dzn_phoenix_log_4
{

};

modded class dzn_phoenix_log_5
{

};

modded class dzn_lantia_december
{

};

modded class dzn_lantia_vega7
{

};
#endif*/

class NA_Documents_Sanctuary1 extends ItemBase
{
	override void SetActions()
	{
		super.SetActions();
		AddAction(ActionReadBook);
	}
};

class NA_Documents_Sanctuary2 extends ItemBase
{
	override void SetActions()
	{
		super.SetActions();
		AddAction(ActionReadBook);
	}
};

class NA_Documents_SanctuarySecrets extends ItemBase
{
	override void SetActions()
	{
		super.SetActions();
		AddAction(ActionReadBook);
	}
};

class NA_Documents_ProjectSanctuary extends ItemBase
{
	override void SetActions()
	{
		super.SetActions();
		AddAction(ActionReadBook);
	}
};
#endif
