/**
 * ActionStopEngineBoat.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2024 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

#ifndef DAYZ_1_25
modded class ActionStopEngineBoat
{
	override void OnExecuteClient(ActionData action_data)
	{
		super.OnExecuteClient(action_data);

		auto vehicle = ExpansionVehicle.Get(action_data.m_Player);

		if (vehicle)
		{
			if (vehicle.EngineIsOn())  //! Super may already have stopped the engine
				vehicle.EngineStop();
		}
	}

	override void OnExecuteServer(ActionData action_data)
	{
		super.OnExecuteServer(action_data);

		auto vehicle = ExpansionVehicle.Get(action_data.m_Player);

		if (vehicle)
		{
			if (vehicle.EngineIsOn())  //! Super may already have stopped the engine
				vehicle.EngineStop();

			if (action_data.m_Player.GetIdentity() && GetExpansionSettings().GetLog().VehicleEngine)
				GetExpansionSettings().GetLog().PrintLog("[VehicleEngine] Player " + action_data.m_Player.GetIdentity().GetName() + " [uid=" + action_data.m_Player.GetIdentity().GetId() + "] stopped vehicle " + vehicle.GetDisplayName() + " (id=" + vehicle.GetPersistentIDString() + " pos=" + vehicle.GetPosition() + ")");
		}
	}

	override bool Expansion_CheckSuccess(ActionData action_data)
	{
		ExpansionVehicle vehicle;
		if (ExpansionVehicle.Get(vehicle, action_data.m_Player))
		{
			return !vehicle.EngineIsOn();
		}

		return false;
	}
};
#endif
