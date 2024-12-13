/**
 * ActionStartEngine.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

modded class ActionStartEngine
{
	string m_Expansion_EngineName;

	override string GetText()
	{
		return "#STR_EXPANSION_VEHICLE_ENGINE_START" + " " + m_Expansion_EngineName + " " + "#STR_EXPANSION_VEHICLE_ENGINE";
	}

	//! Can NOT call super here
	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		ExpansionVehicle vehicle;
		if (Expansion_ActionConditionStatic(player, target, item, vehicle))
		{
		#ifndef DAYZ_1_25
			//! 1.26
			if (vehicle.GetBoat())
				return false;  //! Vanilla boat has own start engine action
		#endif

			m_CommandUID = vehicle.GetEngineStartAnimation();

		#ifndef SERVER
			m_Expansion_EngineName = vehicle.EngineGetName();
		#endif

			return true;
		}

		return false;
	}

#ifdef WORKBENCH
	override static bool Expansion_ActionConditionStatic(PlayerBase player, ActionTarget target, ItemBase item, out ExpansionVehicle vehicle)
#else
	static bool Expansion_ActionConditionStatic(PlayerBase player, ActionTarget target, ItemBase item, out ExpansionVehicle vehicle)
#endif
	{
		if (!ExpansionVehicle.Get(vehicle, player))
			return false;

		if (!vehicle.EngineIsOn())
		{
			if (vehicle.GetHealthLevel("Engine") >= GameConstants.STATE_RUINED)
				return false;

			if (vehicle.CrewMemberIndex(player) == DayZPlayerConstants.VEHICLESEAT_DRIVER)
			{
				switch (vehicle.GetLockState())
				{
					case ExpansionVehicleLockState.NOLOCK:
					case ExpansionVehicleLockState.FORCEDUNLOCKED:
						return true;
					case ExpansionVehicleLockState.FORCEDLOCKED:
						return false;
				}

				if (!GetExpansionSettings().GetVehicle(false).IsLoaded())
					return false;

				ExpansionCarKey key;
				if (GetExpansionSettings().GetVehicle().VehicleRequireKeyToStart == 1)
				{
					return player.HasKeyForCar(vehicle);
				}
				else if (GetExpansionSettings().GetVehicle().VehicleRequireKeyToStart == 2)
				{
					if (!Class.CastTo(key, player.GetItemInHands()))
						return false;

					if (!vehicle.IsPairedTo(key))
						return false;

					return true;
				}

				//! GetExpansionSettings().GetVehicle().VehicleRequireKeyToStart == 0, or invalid number but this still saves checking
				return true;
			}
		}

		return false;
	}

	override void OnFinishProgress(ActionData action_data)
	{
		ExpansionVehicle vehicle;
		if (ExpansionVehicle.Get(vehicle, action_data.m_Player))
		{
			if (vehicle.GetExpansionVehicleBase())
			{
				//! START vanilla ActionContinuousBase
				if (LogManager.IsActionLogEnable())
					Debug.ActionLog("Time stamp: " + action_data.m_Player.GetSimulationTimeStamp(), this.ToString() , "n/a", "OnFinishProgress", action_data.m_Player.ToString());
				if (GetGame().IsServer())
				{
					OnFinishProgressServer(action_data);
			
					if (m_AdminLog)
						m_AdminLog.OnContinouousAction( action_data );
				}
				else
				{
					OnFinishProgressClient(action_data);
				}
				action_data.m_WasExecuted = false;
				//! END vanilla ActionContinuousBase

				return;
			}
		}

		super.OnFinishProgress(action_data);
	}

	override void OnFinishProgressClient(ActionData action_data)
	{
		ExpansionVehicle vehicle;
		if (ExpansionVehicle.Get(vehicle, action_data.m_Player))
		{
			if (vehicle.GetExpansionVehicleBase())
			{
				vehicle.EngineStart();
				return;
			}
		}

		super.OnFinishProgressClient(action_data);
	}

	//! Can NOT call super here
	override void OnFinishProgressServer(ActionData action_data)
	{
		Expansion_OnFinishProgressServerStatic(action_data);
	}

#ifdef WORKBENCH
	override static void Expansion_OnFinishProgressServerStatic(ActionData action_data)
#else
	static void Expansion_OnFinishProgressServerStatic(ActionData action_data)
#endif
	{
		ExpansionVehicle vehicle;
		if (ExpansionVehicle.Get(vehicle, action_data.m_Player))
		{
			float engineHealth = vehicle.GetEntity().GetHealth01("Engine", "");
			//! @note chance when engine is damaged (health level 0.5)
			float chance = GetExpansionSettings().GetVehicle().DamagedEngineStartupChancePercent / 100.0;
			//! @note calculated chance follows a power curve (linear if chance == 0.5)
			float chanceMin = 0.0025;
			bool clamp = chance == 0;
			if (chance < chanceMin)
				chance = chanceMin;
			chance = Math.Pow(engineHealth, -ExpansionMath.Log2(chance));
			if (clamp)
				chance = ExpansionMath.LinearConversion(chanceMin, 1.0, chance, 0.0, 1.0);
			if (chance >= Math.RandomFloatInclusive(0.0, 1.0))
			{
				vehicle.EngineStart();
				
				if (action_data.m_Player.GetIdentity() && GetExpansionSettings().GetLog().VehicleEngine)
					GetExpansionSettings().GetLog().PrintLog("[VehicleEngine] Player \"" + action_data.m_Player.GetIdentity().GetName() + "\" [uid=" + action_data.m_Player.GetIdentity().GetId() + "] started vehicle " + vehicle.GetDisplayName() + " (id=" + vehicle.GetPersistentIDString() + " pos=" + vehicle.GetPosition() + ")");
			}
		}
	}

	override bool Expansion_CheckSuccess(ActionData action_data)
	{
		ExpansionVehicle vehicle;
		if (ExpansionVehicle.Get(vehicle, action_data.m_Player))
		{
			return vehicle.EngineIsOn();
		}

		return false;
	}
};
