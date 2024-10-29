/**
 * ExpansionActionConnectTow.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

class ExpansionActionConnectTowReciveData : ActionReciveData
{
	Object m_ToTow;
	int m_Index;
}

class ExpansionActionConnectTowData : ActionData
{
	Object m_ToTow;
	int m_Index;
};

class ExpansionActionConnectTow : ActionInteractBase
{
	bool m_IsWinch;

	void ExpansionActionConnectTow()
	{
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_OPENDOORFW;
		m_StanceMask = DayZPlayerConstants.STANCEMASK_ALL;
		m_HUDCursorIcon = CursorIcons.OpenDoors;
	}

	override ActionData CreateActionData()
	{
		ExpansionActionConnectTowData action_data = new ExpansionActionConnectTowData;
		return action_data;
	}

	override void CreateConditionComponents()
	{
		m_ConditionItem = new CCINone;
		m_ConditionTarget = new CCTNone;
	}

	override string GetText()
	{
		if (m_IsWinch)
			return "#STR_EXPANSION_CONNECT_WINCH";

		return "#STR_EXPANSION_CONNECT_TOW";
	}

	override void WriteToContext(ParamsWriteContext ctx, ActionData action_data)
	{
		super.WriteToContext(ctx, action_data);

		ExpansionActionConnectTowData poActionData;
		poActionData = ExpansionActionConnectTowData.Cast(action_data);

		ctx.Write(poActionData.m_ToTow);
	}

	override bool ReadFromContext(ParamsReadContext ctx, out ActionReciveData action_recive_data)
	{
		if (!action_recive_data)
		{
			action_recive_data = new ExpansionActionConnectTowReciveData;
		}

		super.ReadFromContext(ctx, action_recive_data);

		ExpansionActionConnectTowReciveData action_data_po = ExpansionActionConnectTowReciveData.Cast(action_recive_data);

		if (!ctx.Read(action_data_po.m_ToTow))
			return false;

		return true;
	}

	override void HandleReciveData(ActionReciveData action_recive_data, ActionData action_data)
	{
		super.HandleReciveData(action_recive_data, action_data);

		ExpansionActionConnectTowReciveData recive_data_po = ExpansionActionConnectTowReciveData.Cast(action_recive_data);
		ExpansionActionConnectTowData action_data_po = ExpansionActionConnectTowData.Cast(action_data);

		action_data_po.m_ToTow = recive_data_po.m_ToTow;
	}

	override bool SetupAction(PlayerBase player, ActionTarget target, ItemBase item, out ActionData action_data, Param extra_data = NULL)
	{
		if (super.SetupAction(player, target, item, action_data, extra_data))
		{
			ExpansionActionConnectTowData action_data_b = ExpansionActionConnectTowData.Cast(action_data);
			return GetObjectToTow(player, action_data_b.m_ToTow, action_data_b.m_Index);
		}

		return false;
	}

	bool GetCarToTow(PlayerBase player, out Object out_car, out int out_index)
	{
		EXError.Error(this, "DEPRECATED");
		return GetObjectToTow(player, out_car, out_index);
	}

	bool GetObjectToTow(PlayerBase player, out Object toTow, out int out_index)
	{
		int i;
		Human crew;
		bool hasCrew;

		auto vehicle = ExpansionVehicle.Get(player);
		if (vehicle)
		{
			if (vehicle.IsTowing())
				return false;

			if (vehicle.CrewMemberIndex(player) == DayZPlayerConstants.VEHICLESEAT_DRIVER)
			{
				array<Object> excluded = new array<Object>;
				array<Object> collided = new array<Object>;
				excluded.Insert(vehicle.GetEntity());
				excluded.Insert(player);

				vector towPosition = vehicle.GetTowPositionWS();
				float towRadius = vehicle.GetTowLength();

				GetGame().IsBoxColliding(towPosition, vehicle.GetTowDirection(), "5 5 5", excluded, collided);
				
				//Shape.CreateSphere(0xFF00FF00, ShapeFlags.NOZBUFFER | ShapeFlags.TRANSP | ShapeFlags.ONCE, towPosition, towRadius);

				foreach (Object o : collided)
				{
					auto otherVehicle = ExpansionVehicle.Get(o);
					ItemBase other;
					if (otherVehicle)
					{
						// while we technically could allow towing a vehicle itself towing another one
						// dayz will get more and more fucky so we limit it to only pairs
						if (!otherVehicle.IsTowing() && vehicle.CanConnectTow(otherVehicle) && !otherVehicle.GetTowedEntity())
						{
							m_IsWinch = vehicle.IsHelicopter();

							if (otherVehicle.GetOverlappingTowConnection(towPosition, towRadius, out_index))
							{
								toTow = o;
								return true;
							}
						}
					}
					else if (Class.CastTo(other, o))
					{
						if (!other.Expansion_IsTowing() && vehicle.CanConnectTow(other))
						{
							m_IsWinch = vehicle.IsHelicopter();

							if (other.Expansion_GetOverlappingTowConnection(towPosition, towRadius, out_index))
							{
								toTow = o;
								return true;
							}
						}
					}
				}
			}
		}

		return false;
	}

	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		// only check on connection
		if (!GetExpansionSettings().GetVehicle(false).IsLoaded() || !GetExpansionSettings().GetVehicle().Towing)
			return false;

		if (!super.ActionCondition(player, target, item))
			return false;

		Object toTow;
		int index;
		return GetObjectToTow(player, toTow, index);
	}

	override void OnStartServer(ActionData action_data)
	{
		super.OnStartServer(action_data);

		ExpansionActionConnectTowData action_data_b = ExpansionActionConnectTowData.Cast(action_data);

		auto vehicle = ExpansionVehicle.Get(action_data_b.m_Player);
		if (vehicle)
		{
			if (vehicle.CrewMemberIndex(action_data_b.m_Player) == DayZPlayerConstants.VEHICLESEAT_DRIVER)
			{
				vehicle.CreateTow(action_data_b.m_ToTow, action_data_b.m_Index);

				if (GetGame().IsMultiplayer() && GetExpansionSettings().GetLog().VehicleTowing)
				{
					string msg = "[VehicleTowing] Player \"" + action_data.m_Player.GetIdentity().GetName() + "\" (id=" + action_data.m_Player.GetIdentity().GetId() + " pos=" + action_data.m_Player.GetPosition() + ")" + " towed " + action_data_b.m_ToTow.GetType() + " (id=" + ExpansionStatic.GetPersistentIDString(EntityAI.Cast(action_data_b.m_ToTow)) + " pos=" + action_data_b.m_ToTow.GetPosition();
					msg += " with " + vehicle.GetType() + " (id=" + vehicle.GetPersistentIDString() + " pos=" + vehicle.GetPosition() + ")";
					GetExpansionSettings().GetLog().PrintLog(msg);
				}
			}
		}
	}

	override bool CanBeUsedInVehicle()
	{
		return true;
	}
};
