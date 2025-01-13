/**
 * ActionSwitchSeats.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

modded class ActionSwitchSeats
{
	override void Start(ActionData action_data)
	{
		super.Start(action_data);

		if (GetGame().IsServer() && action_data.m_Player && action_data.m_Player.GetIdentity() && GetExpansionSettings().GetLog().VehicleEnter)
		{
			auto vehicle = ExpansionVehicle.Get(action_data.m_Target.GetObject());
			if (vehicle)
			{
				int componentIndex = action_data.m_Target.GetComponentIndex();
				int crew_index = vehicle.CrewPositionIndex(componentIndex);
				string seat;
				if (crew_index == DayZPlayerConstants.VEHICLESEAT_DRIVER)
				{
					if (vehicle.IsHelicopter() || vehicle.IsPlane())
						seat = "pilot";
					else
						seat = "driver";
				}
				else
				{
					seat = "passenger";
				}

				GetExpansionSettings().GetLog().PrintLog("[VehicleEnter] Player \"{1:name}\" (id={1:id}) switched seats in vehicle {2:name} (id={2:persistent_id} pos={2:position}) to " + seat, action_data.m_Player, vehicle.GetEntity());
			}
		}
	}
}

class ExpansionActionSwitchSeats: ActionSwitchSeats
{
	static ref TTypenameArray s_Expansion_VehicleTypes = {ExpansionBus, Vehicle_ExpansionBus};

	override void CreateConditionComponents()  
	{
		m_ConditionItem = new CCINone;

		//! Hack fix switch seats not showing up in bus if set to CCTObject
		//! Having this here makes the action show up, but switching seats in bus does nothing :-(
		m_ConditionTarget = new CCTNone;
	}

	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		IEntity parent = player.GetParent();

		if (!parent)
			return false;

		if (!ExpansionStatic.IsAnyOf(parent, s_Expansion_VehicleTypes))
			return false;

		return super.ActionCondition(player, target, item);
	}
}
