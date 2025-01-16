/**
 * ActionDetach.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

modded class ActionDetach
{
	override bool ActionCondition( PlayerBase player, ActionTarget target, ItemBase item )
	{
		auto vehicle = ExpansionVehicle.Get(target.GetParent());
		if ( vehicle )
		{
			if ( vehicle.IsLocked() )
			{
				return false;
			}
		}

		return super.ActionCondition( player, target, item );
	}

	override void OnExecuteServer( ActionData action_data )
	{
		super.OnExecuteServer(action_data);

		auto vehicle = ExpansionVehicle.Get(action_data.m_Target.GetParent());
		if (vehicle)
		{
			if (action_data.m_Player && action_data.m_Player.GetIdentity() && GetExpansionSettings().GetLog().VehicleAttachments)
			{
				GetExpansionSettings().GetLog().PrintLog("[VehicleAttachments] Player \"{1:name}\" (id={1:id}) detached {2:name} (type={2:type}) from vehicle {3:name} (id={3:persistent_id} pos={3:position})", action_data.m_Player, action_data.m_Target.GetObject(), vehicle.GetEntity());
			}
		}
	}
}