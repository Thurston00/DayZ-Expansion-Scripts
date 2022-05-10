/**
 * ActionCheckPulse.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

#ifdef EXPANSIONMODMARKET
#ifdef ENFUSION_AI_PROJECT
modded class ActionCheckPulse
{
	override bool ActionCondition ( PlayerBase player, ActionTarget target, ItemBase item )
	{
		if (player && target.GetObject().IsInherited(ExpansionTraderAIBase))
			return false;

		return super.ActionCondition(player, target, item);
	}
};
#endif
#endif