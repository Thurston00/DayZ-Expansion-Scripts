/**
 * ExpansionActionCoverVehicle.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

class ExpansionActionCoverVehicleCB: ActionContinuousBaseCB
{
	override void CreateActionComponent()
	{
		m_ActionData.m_ActionComponent = new CAContinuousTime(5);
	}
};

class ExpansionActionCoverVehicle: ActionContinuousBase
{
	void ExpansionActionCoverVehicle()
	{
		m_CallbackClass = ExpansionActionCoverVehicleCB;
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONFB_CRAFTING;
		m_FullBody = true;
		m_StanceMask = DayZPlayerConstants.STANCEMASK_ALL;
	}

	override void CreateConditionComponents()  
	{
		m_ConditionTarget = new CCTCursor;
		m_ConditionItem = new CCINone;
	}

	override bool HasTarget()
	{
		return true;
	}

	override bool HasProgress()
	{
		return true;
	}

	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		Object targetObject = target.GetParentOrObject();
		if (!targetObject)
			return false;

		string placeholderType;
		string vehicleModel;

		auto vehicle = ExpansionVehicle.Get(targetObject);
		if (vehicle)
		{
			if (!vehicle.CanCover())
				return false;

			if (!GetGame().IsDedicatedServer())
			{
				placeholderType = vehicle.GetPlaceholderType(item.GetType());
				vehicleModel = vehicle.GetEntity().ConfigGetString("model");
			}
		}
		else
		{
			return false;
		}

		if (!GetGame().IsDedicatedServer())
		{
			string placeHolderModel = GetGame().ConfigGetTextOut("CfgVehicles " + placeholderType + " model");
			if (placeHolderModel == vehicleModel)
				m_Text = "#STR_EXPANSION_ACTION_STORE";
			else
				m_Text = "#STR_EXPANSION_ACTION_COVER";
		}

		return true;
	}
	
	override void OnFinishProgressServer(ActionData action_data)
	{
		EntityAI entity;
		if (!Class.CastTo(entity, action_data.m_Target.GetParentOrObject()))
			return;

		string id;
		string type = entity.GetType();

		auto vehicle = ExpansionVehicle.Get(entity);
		ExpansionEntityStoragePlaceholder placeholder;
		bool result;
		if (vehicle)
		{
			id = vehicle.GetGlobalID().IDToHex();
			result = vehicle.Cover(action_data.m_MainItem, placeholder);
		}
		
		if (GetExpansionSettings().GetLog().VehicleCover)
		{
			if (result)
				GetExpansionSettings().GetLog().PrintLog("[VehicleCover] Player \"%1\" (id=%2 pos=%3) covered vehicle \"%4\" (GlobalID=%5 pos=%6)!", action_data.m_Player.GetIdentity().GetName(), action_data.m_Player.GetIdentity().GetId(), action_data.m_Player.GetPosition().ToString(), type, id, placeholder.GetPosition().ToString());
			else
				GetExpansionSettings().GetLog().PrintLog("[VehicleCover] ERROR: Player \"%1\" (id=%2 pos=%3) tried to cover vehicle \"%4\" (GlobalID=%5 pos=%6) but it failed!", action_data.m_Player.GetIdentity().GetName(), action_data.m_Player.GetIdentity().GetId(), action_data.m_Player.GetPosition().ToString(), type, id, entity.GetPosition().ToString());
		}
	}
}
