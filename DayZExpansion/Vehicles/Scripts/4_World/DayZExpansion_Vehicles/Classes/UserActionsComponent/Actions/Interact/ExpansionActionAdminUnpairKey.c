/**
 * ExpansionActionAdminUnpairKey.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

class ExpansionActionAdminUnpairKey: ActionInteractBase
{
	void ExpansionActionAdminUnpairKey()
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
		return "#STR_EXPANSION_UNPAIR_KEY";
	}

	override bool ActionCondition( PlayerBase player, ActionTarget target, ItemBase item )
	{
		if ( !target || !player )
			return false;

		auto vehicle = ExpansionVehicle.Get(target.GetParentOrObject());
		if ( !vehicle )
			return false;

		ExpansionCarAdminKey key;
		if ( !Class.CastTo( key, player.GetItemInHands() ) )
			return false;

		if ( !vehicle.HasKey() )
			return false;
		
		return true;
	}

	override void OnStartServer( ActionData action_data )
	{
		super.OnStartServer( action_data );

		auto vehicle = ExpansionVehicle.Get( action_data.m_Target.GetParentOrObject() );

		array< ExpansionCarKey > keys = new array< ExpansionCarKey >;
		ExpansionCarKey.GetKeysForVehicle( vehicle, keys );

		for ( int i = 0; i < keys.Count(); ++i )
			keys[i].Unpair( true );

		if ( GetExpansionSettings().GetLog().AdminTools )
			GetExpansionSettings().GetLog().PrintLog("[AdminTools] Player \"{1:name}\" (id={1:id} pos={1:position}) used the Admin CarKey to unpair {2:type} (id={2:persistent_id} pos={2:position})", action_data.m_Player, vehicle.GetEntity());

		vehicle.ResetKeyPairing();
	}

	override bool CanBeUsedInRestrain()
	{
		return false;
	}
}
