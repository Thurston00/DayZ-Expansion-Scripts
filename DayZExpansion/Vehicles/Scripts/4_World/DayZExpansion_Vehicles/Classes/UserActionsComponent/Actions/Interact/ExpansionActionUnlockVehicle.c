/**
 * ExpansionActionUnlockVehicle.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

class ExpansionActionUnlockVehicle: ActionInteractBase
{
	//! DO NOT STORE VARIABLES FOR SERVER SIDE OPERATION

	void ExpansionActionUnlockVehicle()
	{
		m_StanceMask = DayZPlayerConstants.STANCEMASK_CROUCH | DayZPlayerConstants.STANCEMASK_ERECT;
	}

	override void CreateConditionComponents()  
	{
		m_ConditionItem = new CCINone;
		m_ConditionTarget = new CCTNone;
	}

	override bool ActionCondition( PlayerBase player, ActionTarget target, ItemBase item )
	{		
		ExpansionVehicle vehicle;
		ExpansionCarKey key;
		
		//! The intention is for vehicles to only be unlockable from outside if player has its key (or admin key) in hand,
		//! and to be unlockable from inside if they can't get out otherwise (so player can't accidentally lock themself in)

		if ( ExpansionVehicle.Get( vehicle, player ) )
		{
			if ( !vehicle.HasKey() )
				return false;

			int seat = vehicle.CrewMemberIndex( player );
			//! If it's an even number, also check n+1
			//! If it's an uneven number, also check n-1
			//! So if you sit on driver seat and driver door is closed, but codriver door is open or missing, you can't unlock from inside.
			//! Similarly, if you sit on passenger seat and door on your side is locked, but door on other side is open or missing, you can't unlock from inside.
			int opposite_seat;
			if ( seat % 2 )
				opposite_seat = seat - 1;
			else
				opposite_seat = seat + 1;
			if ( seat >= 0 && ( vehicle.CrewCanGetThrough( seat ) || vehicle.CrewCanGetThrough( opposite_seat ) ) )
				return false;
		}
		else
		{
			if ( !ExpansionVehicle.Get( vehicle, target.GetParentOrObject() ) )
				return false;

			if ( !Class.CastTo( key, player.GetItemInHands() ) )
				return false;

			if ( !vehicle.IsPairedTo( key ) && !key.IsInherited( ExpansionCarAdminKey ) )
				return false;
		}

		//! @note we explicitly check for LOCKED state instead of IsLocked() as we don't want to be able to unlock if forced locked
		ExpansionVehicleLockState lockState = vehicle.GetLockState();
		switch (lockState)
		{
			case ExpansionVehicleLockState.LOCKED:
				m_Text = "#unlock";
				return true;

			case ExpansionVehicleLockState.READY_TO_LOCK:
				m_Text = "#STR_EXPANSION_VEHICLE_CANCEL_LOCK";
				return true;
		}

		return false;
		
	}
	
	override void OnStartServer( ActionData action_data )
	{
		super.OnStartServer( action_data );
		
		ExpansionVehicle vehicle;
		ExpansionCarKey key;

		if ( action_data.m_Player.GetCommand_Vehicle() )
		{
			vehicle = ExpansionVehicle.Get( action_data.m_Player.GetCommand_Vehicle().GetTransport() );
		}
		else
		{
			vehicle = ExpansionVehicle.Get( action_data.m_Target.GetParentOrObject() );
			key = ExpansionCarKey.Cast( action_data.m_Player.GetItemInHands() );
		}
		
		if ( vehicle )
		{
			vehicle.Unlock( key );
	
			if ( GetExpansionSettings().GetLog().AdminTools && key && key.IsInherited( ExpansionCarAdminKey ) )
				GetExpansionSettings().GetLog().PrintLog("[AdminTools] Player \"{1:name}\" (id={1:id} pos={1:position}) used {2:type} to unlock {3:type} (id={3:persistent_id} pos={3:position})", action_data.m_Player, key, vehicle.GetEntity());
			else if ( GetExpansionSettings().GetLog().VehicleCarKey && key )
				GetExpansionSettings().GetLog().PrintLog("[VehicleCarKey] Player \"{1:name}\" (id={1:id} pos={1:position}) used {2:type} to unlock {3:type} (id={3:persistent_id} pos={3:position})", action_data.m_Player, key, vehicle.GetEntity());
		}
	}

	override bool CanBeUsedInVehicle()
	{
		return true;
	}

	override bool CanBeUsedInRestrain()
	{
		return false;
	}
};