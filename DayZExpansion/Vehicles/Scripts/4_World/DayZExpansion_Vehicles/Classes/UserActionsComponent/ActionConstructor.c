/**
 * ActionConstructor.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

/**@class		ActionConstructor
 * @brief		
 **/
modded class ActionConstructor
{
	// ------------------------------------------------------------
	// RegisterActions
	// ------------------------------------------------------------
	override void RegisterActions( TTypenameArray actions )
	{
		super.RegisterActions( actions );
		
		#ifdef EXPANSION_VEHICLE_TOWING
		actions.Insert( ExpansionActionConnectTow );
		actions.Insert( ExpansionActionDisconnectTow );
		#endif
		
		actions.Insert( ExpansionActionSwitchGear );
		actions.Insert( ExpansionActionPairKey );
		actions.Insert( ExpansionActionAdminUnpairKey );
		actions.Insert( ExpansionActionLockVehicle );
		actions.Insert( ExpansionActionUnlockVehicle );
		actions.Insert( ExpansionActionOpenVehicleDoor );
		actions.Insert( ExpansionActionCloseVehicleDoor );

		actions.Insert( ExpansionActionSwitchLights );
		actions.Insert( ExpansionActionFillFuel );

		//! Tool-based actions
		actions.Insert( ExpansionActionPickVehicleLock );
		actions.Insert( ExpansionActionChangeVehicleLock );
		actions.Insert( ExpansionActionRepairVehicle );

		//! Boats
		actions.Insert( ExpansionActionNextEngine );
		actions.Insert( ExpansionActionNextEngineInput );		
	#ifdef DAYZ_1_25
		actions.Insert( ExpansionActionBoardBuoyantObject );
	#endif
		
		//! Helicopters
		actions.Insert( ExpansionActionHelicopterHoverRefill );
		actions.Insert( ExpansionActionSwitchAutoHover );
		actions.Insert( ExpansionActionSwitchAutoHoverInput );
		actions.Insert( ExpansionActionRotateRotors );

		//! Vehicle cover
		actions.Insert( ExpansionActionCoverVehicle );
		actions.Insert( ExpansionActionUncoverVehicle );

		actions.Insert( ExpansionActionSwitchSeats );
	}
};