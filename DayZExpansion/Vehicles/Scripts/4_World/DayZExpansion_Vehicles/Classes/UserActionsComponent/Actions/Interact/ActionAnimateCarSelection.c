/**
 * ActionAnimateCarSelection.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

modded class ActionAnimateCarSelection
{
	override bool ActionCondition( PlayerBase player, ActionTarget target, ItemBase item )
	{
		auto vehicle = ExpansionVehicle.Get(target.GetObject());
		if ( vehicle )
		{
			if ( vehicle.IsLocked() )
				return false;
		}

		return super.ActionCondition(player, target, item);
	}
}
