/**
 * ExpansionActionDisconnectTow.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

class ExpansionActionDisconnectTow : ActionInteractBase
{
	bool m_IsWinch;

	void ExpansionActionDisconnectTow()
	{
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_OPENDOORFW;
		m_StanceMask = DayZPlayerConstants.STANCEMASK_ALL;
		m_HUDCursorIcon = CursorIcons.OpenDoors;
	}

	override void CreateConditionComponents()
	{
		m_ConditionItem = new CCINone;
		m_ConditionTarget = new CCTNone;
	}

	override string GetText()
	{
		if (m_IsWinch)
			return "#STR_EXPANSION_DISCONNECT_WINCH";

		return "#STR_EXPANSION_DISCONNECT_TOW";
	}

	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
	#ifndef SERVER
		if ( !player.IsCameraInsideVehicle() || CarDoor.Cast( target.GetObject() ) )
			return false;
	#endif

		auto vehicle = ExpansionVehicle.Get(player);
		if (vehicle && vehicle.CrewMemberIndex(player) == DayZPlayerConstants.VEHICLESEAT_DRIVER)
		{
			m_IsWinch = vehicle.IsHelicopter();
			return vehicle.IsTowing();
		}
		return false;
	}

	override void OnStartServer(ActionData action_data)
	{
		super.OnStartServer(action_data);

		auto vehicle = ExpansionVehicle.Get(action_data.m_Player);
		if (vehicle && vehicle.CrewMemberIndex(action_data.m_Player) == DayZPlayerConstants.VEHICLESEAT_DRIVER)
		{
			vehicle.DestroyTow();
			if (GetExpansionSettings().GetLog().VehicleTowing)
			{
				EntityAI towedEntity = vehicle.GetTowedEntity();
				if (!towedEntity)
					return;

				GetExpansionSettings().GetLog().PrintLog("[VehicleTowing] Player \"{1:name}\" (id={1:id} pos={1:position}) untowed {2:type} (id={2:persistent_id} pos={2:position}) with {3:type} (id={3:persistent_id} pos={3:position})", action_data.m_Player, towedEntity, vehicle.GetEntity());
			}
		}
	}

	override bool CanBeUsedInVehicle()
	{
		return true;
	}
};
