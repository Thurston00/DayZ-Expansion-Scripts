/**
 * ActionStopEngine.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

modded class ActionStopEngine
{
	CarScript m_Car;

	override void CreateConditionComponents()
	{
		m_ConditionItem = new CCINone;
		m_ConditionTarget = new CCTNone;
	}

	override string GetText()
	{
		if (m_Car)
			return "#STR_EXPANSION_VEHICLE_ENGINE_STOP" + " " + m_Car.Expansion_EngineGetName() + " " + "#STR_EXPANSION_VEHICLE_ENGINE";

		return m_Text;
	}

	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		HumanCommandVehicle vehCmd = player.GetCommand_Vehicle();

		if (vehCmd && vehCmd.GetVehicleSeat() == DayZPlayerConstants.VEHICLESEAT_DRIVER)
		{
			Transport trans = vehCmd.GetTransport();
			if (trans)
			{
				if (Class.CastTo(m_Car, trans) && m_Car.Expansion_EngineIsOn())
				{
					m_CommandUID = m_Car.Expansion_EngineStopAnimation();

					//if ( m_Car.CrewMemberIndex( player ) == DayZPlayerConstants.VEHICLESEAT_DRIVER )
					if (Math.AbsFloat(m_Car.GetSpeedometer()) <= 8)
						return true;
				}
			}
		}
		return false;
	}

	override void OnExecuteServer(ActionData action_data)
	{
		HumanCommandVehicle vehCmd = action_data.m_Player.GetCommand_Vehicle();

		if (vehCmd && vehCmd.GetVehicleSeat() == DayZPlayerConstants.VEHICLESEAT_DRIVER)
		{
			Transport trans = vehCmd.GetTransport();
			if (trans)
			{
				if (Class.CastTo(m_Car, trans) && m_Car.Expansion_EngineIsOn())
				{
					m_Car.Expansion_EngineStop();
					
					if (!GetGame().IsMultiplayer())
					{
						EffectSound sound = SEffectManager.PlaySound(m_Car.m_EngineStopFuel, m_Car.GetPosition());
						sound.SetSoundAutodestroy(true);
					}

					if (action_data.m_Player.GetIdentity() && GetExpansionSettings().GetLog().VehicleEngine)
						GetExpansionSettings().GetLog().PrintLog("[VehicleEngine] Player " + action_data.m_Player.GetIdentity().GetName() + " [uid=" + action_data.m_Player.GetIdentity().GetId() + "] stopped vehicle " + m_Car.GetDisplayName() + " (id=" + m_Car.GetVehiclePersistentIDString() + " pos=" + m_Car.GetPosition() + ")");
				}
			}
		}
	}

	override void OnExecuteClient(ActionData action_data)
	{
		HumanCommandVehicle vehCmd = action_data.m_Player.GetCommand_Vehicle();

		if (vehCmd && vehCmd.GetVehicleSeat() == DayZPlayerConstants.VEHICLESEAT_DRIVER)
		{
			Transport trans = vehCmd.GetTransport();
			if (trans)
			{
				if (Class.CastTo(m_Car, trans) && m_Car.Expansion_EngineIsOn())
				{
					m_Car.Expansion_EngineStop();
					EffectSound sound = SEffectManager.PlaySound(m_Car.m_EngineStopFuel, m_Car.GetPosition());
					sound.SetSoundAutodestroy(true);
				}
			}
		}
	}
};
