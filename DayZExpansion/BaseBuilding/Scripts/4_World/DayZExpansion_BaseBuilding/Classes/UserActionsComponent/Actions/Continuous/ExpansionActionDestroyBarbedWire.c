/**
 * ExpansionActionDestroyBarbedWire.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

class ExpansionActionDestroyBarbedWire : ExpansionActionDestroyBase
{
	override bool SetupCondition()
	{
		return GetExpansionSettings().GetRaid(false).IsLoaded();
	}

	override void Setup( PlayerBase player, ActionTarget target, ItemBase item )
	{
		m_Time = GetExpansionSettings().GetRaid().BarbedWireRaidToolTimeSeconds;
		m_Cycles = GetExpansionSettings().GetRaid().BarbedWireRaidToolCycles;
		m_MinHealth01 = 0.01;  //! 1% health
		m_ToolDamagePercent = GetExpansionSettings().GetRaid().BarbedWireRaidToolDamagePercent;
	}

	override bool DestroyCondition( PlayerBase player, ActionTarget target, ItemBase item, bool camera_check )
	{
		if ( !super.DestroyCondition( player, target, item, camera_check ) )
			return false;

		if ( !ExpansionStatic.IsAnyOf(item, GetExpansionSettings().GetRaid().BarbedWireRaidTools) )
			return false;

		Object targetObject = target.GetObject();

		EntityAI targetEntity;
		if ( targetObject.IsInherited( ExpansionBarbedWire ) )
		{
			return true;
		}
		else if (Class.CastTo(targetEntity, targetObject))
		{			
			string selection = targetObject.GetActionComponentName( target.GetComponentIndex() );
			
			if ( selection.Length() > 0 )
			{
				int delimiter_index = selection.IndexOfFrom( 0, "_mounted" );
				if ( delimiter_index > -1 )
				{
					selection = selection.Substring( 0, delimiter_index );
					
					BarbedWire barbed_wire = BarbedWire.Cast( targetEntity.FindAttachmentBySlotName( selection ) );
					if ( barbed_wire && barbed_wire.IsMounted() )
					{
					#ifndef SERVER
						m_TargetName = barbed_wire.GetDisplayName();
					#endif

						return true;
					}
				}
			}
		}

		return false;
	}

	override bool CanBeDestroyed( Object targetObject )
	{
		auto settings = GetExpansionSettings().GetRaid();
		return settings.CanRaidBarbedWire && settings.IsRaidableNow();
	}

	override Object GetActualTargetObject( Object targetObject )
	{
		EntityAI targetEntity;
		if ( targetObject.IsInherited( ExpansionBarbedWire ) )
		{
			return targetObject;
		}
		else if (Class.CastTo(targetEntity, targetObject))
		{
			return targetEntity.GetAttachmentByType( BarbedWire );
		}

		return null;
	}

	override bool Expansion_CheckSuccess(ActionData action_data)
	{
		ItemBase barbedWire;
		if (Class.CastTo(barbedWire, GetActualTargetObject(action_data.m_Target.GetParentOrObject())))
			return barbedWire.IsDamageDestroyed();

		return false;
	}
}
