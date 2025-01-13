/**
 * ExpansionActionPairKey.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

class ExpansionActionPairKey: ActionInteractBase
{
	//! DO NOT STORE VARIABLES FOR SERVER SIDE OPERATION

	private bool m_IsGlitched;

	void ExpansionActionPairKey()
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
		if ( m_IsGlitched )
			return "#STR_EXPANSION_PAIR_KEY [Fix Glitch]";

		return "#STR_EXPANSION_PAIR_KEY";
	}

	override bool ActionCondition( PlayerBase player, ActionTarget target, ItemBase item )
	{
		ExpansionVehicle vehicle;
		ExpansionCarKey key;

		if ( ExpansionVehicle.Get( vehicle, player ) ) //! don't pair if we are inside the vehicle
			return false;

		if ( !ExpansionVehicle.Get( vehicle, target.GetParentOrObject() ) )
			return false;

		if ( !Class.CastTo( key, player.GetItemInHands() ) )
			return false;
		
		if ( key.IsInherited( ExpansionCarAdminKey ) )
			return false;

		if ( key.IsPaired() && !vehicle.HasKey() ) //! key is paired to something, vehicle doesn't have a key
		{
			if ( !key.IsPairedTo( vehicle ) ) //! the key is not paired to the vehicle
				return false;

			//! the key is paired to the vehicle but the vehicle has no key, we are glitched.
			m_IsGlitched = true;
		} else
		{
			if ( vehicle.HasKey() ) //! vehicle has a key
				return false;

			if ( key.IsPaired() ) //! key is paired
				return false;
		}
		
		return true;
	}

	override void OnStartServer( ActionData action_data )
	{
		super.OnStartServer( action_data );

		auto vehicle = ExpansionVehicle.Get( action_data.m_Target.GetParentOrObject() );
		vehicle.PairKey( ExpansionCarKey.Cast( action_data.m_Player.GetItemInHands() ) );

		if ( GetExpansionSettings().GetLog().VehicleCarKey )
			GetExpansionSettings().GetLog().PrintLog("[VehicleCarKey] PPlayer \"{1:name}\" (id={1:id} pos={1:position}) paired {2:type} to {3:type} (id={3:persistent_id} pos={3:position})", action_data.m_Player, action_data.m_Player.GetItemInHands(), vehicle.GetEntity());
	}

	override bool CanBeUsedInRestrain()
	{
		return false;
	}
};