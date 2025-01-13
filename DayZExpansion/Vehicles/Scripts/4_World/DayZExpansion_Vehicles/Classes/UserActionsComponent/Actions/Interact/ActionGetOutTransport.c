/**
 * ActionGetOutTransport.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

modded class ActionGetOutTransport
{
#ifdef DAYZ_1_25
	override void ProcessGetOutActionData(Car car, GetOutTransportActionData got_action_data)
	{
		super.ProcessGetOutActionData(car, got_action_data);

		auto vehicle = ExpansionVehicle.Get(car);
#else
	//! 1.26+
	override void ProcessGetOutTransportActionData(Transport veh, GetOutTransportActionData got_action_data)
	{
		super.ProcessGetOutTransportActionData(veh, got_action_data);

		auto vehicle = ExpansionVehicle.Get(veh);
#endif

		HumanCommandVehicle vehCommand = got_action_data.m_Player.GetCommand_Vehicle();

		if (vehicle)
		{
			if ( vehicle.CanObjectAttach(got_action_data.m_Player) && vehicle.LeavingSeatDoesAttachment(vehCommand.GetVehicleSeat()) )
			{
				got_action_data.m_WasJumpingOut = false;
			}

			// Should prevent a few issues related to towing and server crashes
			if (vehicle.CrewMemberIndex(got_action_data.m_Player) == DayZPlayerConstants.VEHICLESEAT_DRIVER && vehicle.IsTowing())
			{
				vehicle.DestroyTow();
			}
		}
	}

	override void OnStart(ActionData action_data)
	{
		super.OnStart(action_data);

		if (IsMissionClient())
		{
			GetUApi().GetInputByName("UACarLeft").ForceDisable(false);
			GetUApi().GetInputByName("UACarRight").ForceDisable(false);
			GetUApi().GetInputByName("UACarForward").ForceDisable(false);
			GetUApi().GetInputByName("UACarBack").ForceDisable(false);

			GetUApi().GetInputByName("UACarShiftGearUp").ForceDisable(false);
			GetUApi().GetInputByName("UACarShiftGearDown").ForceDisable(false);
		}
	}

	override void OnEnd(ActionData action_data)
	{
		auto got_action_data = GetOutTransportActionData.Cast(action_data);

		CarScript cs;
	#ifdef DAYZ_1_25
		Class.CastTo(cs, got_action_data.m_Car);
	#else
		//! 1.26+
		Class.CastTo(cs, got_action_data.m_Vehicle);
	#endif
		if (cs && !cs.Expansion_IsCar() && !cs.Expansion_IsDuck())
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(action_data.m_Player.GetInventory().UnlockInventory, 1500, false, LOCK_FROM_SCRIPT); //! Unlock after delay to fix hand desync bug
		else
			super.OnEnd(action_data);
	}

	override void OnEndServer(ActionData action_data)
	{
		GetOutTransportActionData got_action_data = GetOutTransportActionData.Cast(action_data);

	#ifdef DAYZ_1_25
		auto vehicle = ExpansionVehicle.Get(got_action_data.m_Car);
	#else
		//! 1.26+
		auto vehicle = ExpansionVehicle.Get(got_action_data.m_Vehicle);
	#endif

		if (vehicle)
		{
			if (vehicle.IsHelicopter())
				vehicle.SetHasPilot(false);  //! So we are able to detect if pilot got disconnected or got out on own accord
		}

		super.OnEndServer(action_data);

		if (vehicle && action_data.m_Player && action_data.m_Player.GetIdentity() && GetExpansionSettings().GetLog().VehicleLeave)
		{
			GetExpansionSettings().GetLog().PrintLog("[VehicleLeave] Player \"{1:name}\" (id={1:id}) left vehicle {2:name} (id={2:persistent_id} pos={2:position})", action_data.m_Player, vehicle.GetEntity());
		}
	}
};