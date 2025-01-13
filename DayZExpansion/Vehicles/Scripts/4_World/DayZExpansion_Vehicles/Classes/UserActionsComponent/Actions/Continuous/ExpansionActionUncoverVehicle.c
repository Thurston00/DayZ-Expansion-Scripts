/**
 * ExpansionActionUncoverVehicle.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

class ExpansionActionUncoverVehicle: ExpansionActionRestoreEntity
{
	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		if (item)
			return false;

		if (!super.ActionCondition(player, target, item))
			return false;

		Object targetObject = target.GetParentOrObject();

		if (!GetGame().IsDedicatedServer())
		{
			string placeholderType = targetObject.GetType();
			string placeholderModel = targetObject.ConfigGetString("model");
			string vehicleType = placeholderType.Substring(0, placeholderType.Length() - 6);
			string vehicleModel = GetGame().ConfigGetTextOut("CfgVehicles " + vehicleType + " model");
			if (placeholderModel == vehicleModel)
				m_Text = "#STR_EXPANSION_ACTION_RESTORE";
			else
				m_Text = "#STR_EXPANSION_ACTION_UNCOVER";
		}

		if (GetGame().IsServer())
		{
			auto placeholder = ExpansionEntityStoragePlaceholder.Cast(targetObject);
			if (placeholder)
			{
				string type = placeholder.Expansion_GetStoredEntityType();
				if (!GetGame().ConfigIsExisting("CfgVehicles " + type))
				{
					ExpansionNotification("Entity Storage", "Cannot restore " + type + " because the mod providing this vehicle is not loaded").Error(player.GetIdentity());
					return false;
				}
			}
		}

		return true;
	}

	override void OnFinishProgressServer(ActionData action_data)
	{
		ExpansionEntityStoragePlaceholder placeholder;
		EntityAI entity;
		bool result = Expansion_OnFinishProgressServer(action_data, false, placeholder, entity, false);

		string id;
		string type;

		if (placeholder)
		{
			id = placeholder.m_Expansion_StoredEntityGlobalID.IDToHex();
			type = placeholder.Expansion_GetStoredEntityType();
		}

		if (!result)
		{
			if (GetExpansionSettings().GetLog().VehicleCover)
				GetExpansionSettings().GetLog().PrintLog("[VehicleCover] ERROR: Player \"{1:name}\" (id={1:id} pos={1:position}) tried to uncover vehicle \"{2}\" (GlobalID={3} pos={4:position}) but it failed!", action_data.m_Player, new ExpansionPrimitiveT<string>(type), new ExpansionPrimitiveT<string>(id), placeholder);
			return;
		}

		string placeholderType = placeholder.GetType();
		string coverType = "CamoNet";
	
		if (placeholderType.Contains("_Civil"))
			coverType += "Civil";
		else if (placeholderType.Contains("_Desert"))
			coverType += "Desert";
		else if (placeholderType.Contains("_Winter"))
			coverType += "Winter";

		auto camoNetAttachment = placeholder.FindAttachmentBySlotName("CamoNet");
		if (camoNetAttachment)
		{
			//! TODO: Find a way to take existing attachment to hand instead of recreating. ServerTakeEntityToHands doesn't work :-(
			EntityAI camoNet = action_data.m_Player.GetHumanInventory().CreateInHands(coverType);
			if (!camoNet)
				camoNet = EntityAI.Cast(GetGame().CreateObject(coverType, action_data.m_Player.GetPosition()));
			if (camoNet)
				camoNet.SetHealth(camoNetAttachment.GetHealth());
		}

		int slotId = InventorySlots.GetSlotIdFromString("KeyChain");
		auto keychain = ExpansionKeyChainBase.Cast(placeholder.GetInventory().FindAttachment(slotId));
		if (keychain)
		{
			if (entity.GetInventory().HasAttachmentSlot(slotId))
			{
				entity.ServerTakeEntityAsAttachmentEx(keychain, slotId);
			}
		}

		GetGame().ObjectDelete(placeholder);
		
		if (GetExpansionSettings().GetLog().VehicleCover)
			GetExpansionSettings().GetLog().PrintLog("[VehicleCover] Player \"{1:name}\" (id={1:id} pos={1:position}) uncovered vehicle \"{2:type}\" (GlobalID={3} pos={2:position})!", action_data.m_Player, entity, new ExpansionPrimitiveT<string>(id));
	}
}
