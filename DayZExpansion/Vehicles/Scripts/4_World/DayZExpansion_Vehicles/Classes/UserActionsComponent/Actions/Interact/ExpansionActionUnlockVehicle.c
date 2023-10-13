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

	override string GetText()
	{
		return "#unlock";
	}

	override bool ActionCondition( PlayerBase player, ActionTarget target, ItemBase item )
	{		
		CarScript car;
		ExpansionCarKey key;
		
		//! The intention is for vehicles to only be unlockable from outside if player has its key (or admin key) in hand,
		//! and to be unlockable from inside if they can't get out otherwise (so player can't accidentally lock themself in)

		if ( player.GetCommand_Vehicle() )
		{
			if ( !Class.CastTo( car, player.GetCommand_Vehicle().GetTransport() ) )
				return false;

			if ( !car.HasKey() )
				return false;

			int seat = car.CrewMemberIndex( player );
			//! If it's an even number, also check n+1
			//! If it's an uneven number, also check n-1
			//! So if you sit on driver seat and driver door is closed, but codriver door is open or missing, you can't unlock from inside.
			//! Similarly, if you sit on passenger seat and door on your side is locked, but door on other side is open or missing, you can't unlock from inside.
			int opposite_seat;
			if ( seat % 2 )
				opposite_seat = seat - 1;
			else
				opposite_seat = seat + 1;
			if ( seat >= 0 && ( car.CrewCanGetThrough( seat ) || car.CrewCanGetThrough( opposite_seat ) ) )
				return false;
		} else
		{
			if ( !Class.CastTo( car, target.GetParentOrObject() ) )
				return false;

			if ( !Class.CastTo( key, player.GetItemInHands() ) )
				return false;

			if ( !car.IsCarKeys( key ) && !key.IsInherited( ExpansionCarAdminKey ) )
				return false;
		}

		//! @note we explicitly check for LOCKED state instead of IsLocked() as we don't want to be able to unlock if forced locked
		if ( car.GetLockedState() != ExpansionVehicleLockState.LOCKED )
			return false;
		
		return true;
	}
	
	override void OnStartServer( ActionData action_data )
	{
		super.OnStartServer( action_data );
		
		CarScript car;
		ExpansionCarKey key;

		if ( action_data.m_Player.GetCommand_Vehicle() )
		{
			car = CarScript.Cast( action_data.m_Player.GetCommand_Vehicle().GetTransport() );
		} else
		{
			car = CarScript.Cast( action_data.m_Target.GetParentOrObject() );
			key = ExpansionCarKey.Cast( action_data.m_Player.GetItemInHands() );
		}
		
		if ( car )
		{
			car.UnlockCar( key );
	
			if ( GetExpansionSettings().GetLog().AdminTools && key && key.IsInherited( ExpansionCarAdminKey ) )
				GetExpansionSettings().GetLog().PrintLog("[AdminTools] Player \"" + action_data.m_Player.GetIdentity().GetName() + "\" (id=" + action_data.m_Player.GetIdentity().GetId() + " pos=" + action_data.m_Player.GetPosition() + ")" + " used the  "+ key.GetType() +" to unlock " + car.GetType() + " (id=" + car.GetVehiclePersistentIDString() + " pos=" + car.GetPosition() + ")");
			else if ( GetExpansionSettings().GetLog().VehicleCarKey && key )
				GetExpansionSettings().GetLog().PrintLog("[VehicleCarKey] Player \"" + action_data.m_Player.GetIdentity().GetName() + "\" (id=" + action_data.m_Player.GetIdentity().GetId() + " pos=" + action_data.m_Player.GetPosition() + ")" + " used the  "+ key.GetType() +" to unlock " + car.GetType() + " (id=" + car.GetVehiclePersistentIDString() + " pos=" + car.GetPosition() + ")");
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