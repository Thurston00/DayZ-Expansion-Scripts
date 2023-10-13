/**
 * ActionForceConsume.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2023 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

modded class ActionForceConsume
{
	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		if (item.Expansion_IsQuestItem() && item.Expansion_IsDeliveryItem())
			return false;

		if (target.GetObject().IsInherited(ExpansionQuestNPCBase))
			return false;

	#ifdef EXPANSIONMODAI
		if (target.GetObject().IsInherited(ExpansionQuestNPCAIBase))
			return false;
	#endif

		return super.ActionCondition(player, target, item);
	}
};