/**
 * ExpansionActionRestoreEntity.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

class ExpansionActionRestoreEntityCB: ActionContinuousBaseCB
{
	override void CreateActionComponent()
	{
		m_ActionData.m_ActionComponent = new CAContinuousTime(5);
	}
};

class ExpansionActionRestoreEntity: ActionContinuousBase
{
	void ExpansionActionRestoreEntity()
	{
		m_CallbackClass = ExpansionActionRestoreEntityCB;
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONFB_INTERACT;
		m_FullBody = true;
		m_StanceMask = DayZPlayerConstants.STANCEMASK_CROUCH | DayZPlayerConstants.STANCEMASK_ERECT;
		m_Text = "[ADMIN] Ungoatify";
	}

	override void CreateConditionComponents()  
	{
		m_ConditionTarget = new CCTCursor;
		m_ConditionItem = new CCINone;
	}

	override typename GetInputType()
	{
		return ContinuousInteractActionInput;
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
		auto placeholder = ExpansionEntityStoragePlaceholder.Cast(target.GetParentOrObject());
		if (!placeholder)
			return false;

		if (placeholder.Type() == ExpansionDebugGoat)
		{
			auto permissionsManager = GetPermissionsManager();
		#ifdef SERVER
			if (!permissionsManager.HasPermission("Expansion.EntityStorage.Goatify", player.GetIdentity()))
		#else
			if (!permissionsManager.IsAdminToolsToggledOn() || !permissionsManager.HasPermission("Expansion.EntityStorage.Goatify"))
		#endif
				return false;
		}

		if (!placeholder.Expansion_HasStoredEntity())
		{
			if (GetGame().IsServer() && GetGame().IsMultiplayer())
			{
				ExpansionNotification("Entity Storage", "Could not restore " + placeholder.Expansion_GetStoredEntityDisplayName() + " (missing data?)").Error(player.GetIdentity());

				if (GetExpansionSettings().GetLog().EntityStorage)
				{
					auto type = new ExpansionPrimitiveT<string>(placeholder.Expansion_GetStoredEntityType());
					auto id = new ExpansionPrimitiveT<string>(placeholder.m_Expansion_StoredEntityGlobalID.IDToHex());
					GetExpansionSettings().GetLog().PrintLog("[EntityStorage] ERROR: Player \"{1:name}\" (id={1:id} pos={1:position}) tried to restore entity \"{2}\" (GlobalID={3} pos={4:position}) but it failed! (missing data?)", player, type, id, placeholder);
				}
			}

			return false;
		}

		return true;
	}

	override void OnStartServer(ActionData action_data)
	{
		Object targetObject = action_data.m_Target.GetParentOrObject();
		if (targetObject.Type() == ExpansionDebugGoat)
			ExpansionSound.Play("cartent_deploy_SoundSet", targetObject);
	}

	override void OnEndServer(ActionData action_data)
	{
		Object targetObject = action_data.m_Target.GetParentOrObject();
		if (targetObject && targetObject.Type() == ExpansionDebugGoat)
			ExpansionSound.Stop("cartent_deploy_SoundSet", targetObject, 0.1);
	}

	bool Expansion_OnFinishProgressServer(ActionData action_data, bool deletePlaceholder = true, out ExpansionEntityStoragePlaceholder placeholder = null, out EntityAI entity = null, bool log = true)
	{
		if (!Class.CastTo(placeholder, action_data.m_Target.GetParentOrObject()))
			return false;

		string id = placeholder.m_Expansion_StoredEntityGlobalID.IDToHex();

		vector position = placeholder.GetPosition();
		placeholder.SetPosition("0 0 0");
		if (ExpansionEntityStorageModule.RestoreFromFile(placeholder.Expansion_GetEntityStorageFileName(), entity, placeholder))
		{
			if (deletePlaceholder)
			{
				//! If we are deleting placeholder, move restored entity to placeholder position

				vector orientation = placeholder.GetOrientation();

				vector placeholderMinMax[2];
				if (!placeholder.GetCollisionBox(placeholderMinMax))
					placeholder.ClippingInfo(placeholderMinMax);

				float placeHolderOffsetY = placeholderMinMax[0][1];
				if (placeHolderOffsetY > 0)
					placeHolderOffsetY = 0;

				vector entityMinMax[2];
				if (!entity.GetCollisionBox(entityMinMax))
					entity.ClippingInfo(entityMinMax);

				float entityOffsetY = entityMinMax[0][1];
				if (entityOffsetY > 0)
					entityOffsetY = 0;

				position[1] = position[1] + placeHolderOffsetY - entityOffsetY;

				GetGame().ObjectDelete(placeholder);

				entity.SetPosition(position);
				entity.SetOrientation(orientation);
			}
		}
		else
		{
			placeholder.SetPosition(position);
	
			string type;
			placeholder.m_Expansion_NetsyncData.Get(0, type);

			Error(ToString() + "::Expansion_OnFinishProgressServer - Could not restore vehicle " + type + " from file " + placeholder.Expansion_GetEntityStorageFileName());

			ExpansionNotification("Entity Storage", "Could not restore " + placeholder.Expansion_GetStoredEntityDisplayName()).Error(action_data.m_Player.GetIdentity());

			if (log && GetExpansionSettings().GetLog().EntityStorage)
				GetExpansionSettings().GetLog().PrintLog("[EntityStorage] ERROR: Player \"{1:name}\" (id={1:id} pos={1:position}) tried to restore entity \"{2}\" (GlobalID={3} pos={4:position}) but it failed!", action_data.m_Player, new ExpansionPrimitiveT<string>(type), new ExpansionPrimitiveT<string>(id), placeholder);

			return false;
		}
		
		if (log && GetExpansionSettings().GetLog().EntityStorage)
			GetExpansionSettings().GetLog().PrintLog("[EntityStorage] Player \"{1:name}\" (id={1:id} pos={1:position}) restored entity \"{2:type}\" (GlobalID={3} pos={2:position})", action_data.m_Player, entity, new ExpansionPrimitiveT<string>(id));

		return true;
	}

	override void OnFinishProgressServer(ActionData action_data)
	{
		Expansion_OnFinishProgressServer(action_data);
	}
}
