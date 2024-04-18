/**
 * Expansion_AWM_Base.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

class Expansion_AWM_Base: BoltActionRifle_ExternalMagazine_Base
{
	override bool CanDisplayAttachmentSlot(int slot_id)
	{	
		if (!super.CanDisplayAttachmentSlot(slot_id))
			return false;

		string slot_name = InventorySlots.GetSlotName(slot_id);   
		
		if ( slot_name == "ExpansionSniperOptics" )
		{
			return this.FindAttachmentBySlotName("ExpansionSniperOptics") != NULL;
		}

		if ( slot_name == "weaponOptics" )
		{
			return this.FindAttachmentBySlotName("ExpansionSniperOptics") == NULL;	
		} 

		return true;
	}	

	override bool CanReceiveAttachment(EntityAI attachment, int slotId)
	{
		if  ( attachment.IsKindOf("ItemOptics") )
		{			
			if (  FindAttachmentBySlotName("ExpansionSniperOptics") != NULL )
			{
				return false;
			}	
		}		  
		if  ( attachment.IsKindOf("Expansion_PMII25Optic") )
		{			
			if ( FindAttachmentBySlotName("weaponOptics") != NULL)
			{
				return false;
			}	
		}	 
		return super.CanReceiveAttachment(attachment, slotId);
	}

	override RecoilBase SpawnRecoilObject()
	{
		return new CZ527Recoil(this);
	}

	//Debug menu Spawn Ground Special
	override void OnDebugSpawn()
	{
		EntityAI entity;
		if ( Class.CastTo(entity, this) )
		{
			entity.GetInventory().CreateInInventory( "Expansion_PMII25Optic" );
			entity.SpawnEntityOnGroundPos("Mag_Expansion_AWM_5Rnd", entity.GetPosition());
		}
	}
};

class Expansion_AWM: Expansion_AWM_Base {};