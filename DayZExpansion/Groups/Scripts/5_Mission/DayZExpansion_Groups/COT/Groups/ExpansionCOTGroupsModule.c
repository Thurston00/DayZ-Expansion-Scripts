/**
 * ExpansionCOTGroupModule.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

#ifdef JM_COT
class ExpansionCOTGroupModule: JMRenderableModuleBase
{
	private ref map<int, ref ExpansionPartyData> m_Parties; //! Client
	static ref ScriptInvoker m_COTGroupModuleSI = new ScriptInvoker();
	
	// ------------------------------------------------------------
	// ExpansionCOTGroupModule Contrusctor
	// ------------------------------------------------------------	
	void ExpansionCOTGroupModule()
	{
		GetPermissionsManager().RegisterPermission("Expansion.Groups.View");
		GetPermissionsManager().RegisterPermission("Expansion.Groups.EditGroup");
		GetPermissionsManager().RegisterPermission("Expansion.Groups.DeleteGroup");
		
		if (IsMissionClient() && !IsMissionHost())
		{
			m_Parties = new map<int, ref ExpansionPartyData>;
		}
	}
	
	// ------------------------------------------------------------
	// ExpansionCOTGroupModule Destructor
	// ------------------------------------------------------------
	void ~ExpansionCOTGroupModule()
	{
		
	}
	
	override void EnableUpdate()
	{
	}
	
	override void EnableRPC()
	{
	}
	
	override void OnInit()
	{
		super.OnInit();

		Expansion_EnableRPCManager();
		Expansion_RegisterServerRPC("RPC_EditGroupName");
		Expansion_RegisterServerRPC("RPC_DeleteGroup");
		Expansion_RegisterServerRPC("RPC_ChangeOwner");
		Expansion_RegisterServerRPC("RPC_UpdatePermissions");
		Expansion_RegisterServerRPC("RPC_InvitePlayer");
		Expansion_RegisterServerRPC("RPC_KickMember");
		Expansion_RegisterServerRPC("RPC_RequestGroups");
		Expansion_RegisterClientRPC("RPC_SendGroupsToClient");
		Expansion_RegisterClientRPC("RPC_SendGroupUpdate");
		Expansion_RegisterClientRPC("RPC_Callback");
	#ifdef EXPANSIONMODMARKET
		Expansion_RegisterServerRPC("RPC_ChangeMoney");
	#endif
	#ifdef EXPANSIONMODNAVIGATION
		Expansion_RegisterServerRPC("RPC_CreateGroupMarker");
	#endif
	}

	// ------------------------------------------------------------
	// ExpansionCOTGroupModule HasAccess
	// ------------------------------------------------------------	
	override bool HasAccess()
	{
		return GetPermissionsManager().HasPermission("Expansion.Groups.View");
	}
	
	// ------------------------------------------------------------
	// ExpansionCOTGroupModule IsEnabled
	// ------------------------------------------------------------
	override bool IsEnabled()
	{
		return true;
	}
	
	// ------------------------------------------------------------
	// ExpansionCOTGroupModule GetLayoutRoot
	// ------------------------------------------------------------	
	override string GetLayoutRoot()
	{
		return "DayZExpansion/Groups/GUI/layouts/COT/groups/Groups_Menu.layout";
	}
	
	// ------------------------------------------------------------
	// ExpansionCOTGroupModule GetTitle
	// ------------------------------------------------------------	
	override string GetTitle()
	{
		return "[EX] Group Management";
	}
	
	// ------------------------------------------------------------
	// ExpansionCOTGroupModule GetIconName
	// ------------------------------------------------------------	
	override string GetIconName()
	{
		return "G";
	}
	
	// ------------------------------------------------------------
	// ExpansionCOTGroupModule ImageIsIcon
	// ------------------------------------------------------------
	override bool ImageIsIcon()
	{
		return false;
	}
	
	// ------------------------------------------------------------
	// ExpansionCOTGroupModule RequestGroups
	// Called on Client
	// ------------------------------------------------------------		
	void RequestGroups(int callBack)
	{
		auto trace = EXTrace.Start(ExpansionTracing.COT_GROUPS, this);

		if (!IsMissionClient())
			return;
		
		if (!GetPermissionsManager().HasPermission("Expansion.Groups.View"))
			return;
		
		string playerUID = GetGame().GetPlayer().GetIdentity().GetId();
		auto rpc = Expansion_CreateRPC("RPC_RequestGroups");
		rpc.Write(callBack);
		rpc.Write(playerUID);
 		rpc.Expansion_Send(true);
	}
	
	// ------------------------------------------------------------
	// ExpansionCOTGroupModule RPC_RequestGroups
	// Called on Server
	// ------------------------------------------------------------	
	void RPC_RequestGroups(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
		auto trace = EXTrace.Start(ExpansionTracing.COT_GROUPS, this);
		
		if (!GetPermissionsManager().HasPermission("Expansion.Groups.View", sender))
			return;

		if (!IsMissionHost())
			return;
		
		int callBack;
		if (!ctx.Read(callBack))
			return;
		
		string playerUID;
		if (!ctx.Read(playerUID))
			return;
		
		ExpansionPartyModule module;
		if (!CF_Modules<ExpansionPartyModule>.Get(module))
			return;

		auto rpc = Expansion_CreateRPC("RPC_SendGroupsToClient");		
		int partiesCount = module.GetAllParties().Count();
		rpc.Write(callBack);
		rpc.Write(partiesCount);
		
		map<int, ref ExpansionPartyData> parties = module.GetAllParties();
		foreach (int partyID, ExpansionPartyData party: parties)
		{
			if (!party)
				continue;
			
			party.OnSend(rpc, true);
		}
		
 		rpc.Expansion_Send(true, sender);
	}
	
	// ------------------------------------------------------------
	// ExpansionCOTGroupModule RPC_SendGroupsToClient
	// Called on Client
	// ------------------------------------------------------------	
	void RPC_SendGroupsToClient(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
		auto trace = EXTrace.Start(ExpansionTracing.COT_GROUPS, this);

		if (!IsMissionClient())
			return;
		
		int callBack;
		if (!ctx.Read(callBack))
			return;
		
		int partiesCount;
		if (!ctx.Read(partiesCount))
			return;
		
		m_Parties.Clear();
		
		for (int i = 0; i < partiesCount; i++)
		{
			if (!OnReceiveGroupClient(ctx))
				continue;
		}
		
		GetModuleSI().Invoke(callBack);
	}
	
	// ------------------------------------------------------------
	// ExpansionCOTGroupModule OnReceiveGroupClient
	// Called on Client
	// ------------------------------------------------------------
	bool OnReceiveGroupClient(ParamsReadContext ctx)
	{
		auto trace = EXTrace.Start(ExpansionTracing.COT_GROUPS, this);

		int partyID;
		if (Expansion_Assert_False(ctx.Read(partyID), "Failed to read party ID"))
			return false;

		ExpansionPartyData party = m_Parties.Get(partyID);
		if (!party)
		{
			party = new ExpansionPartyData(partyID);
		}

		if (Expansion_Assert_False(party.OnRecieve(ctx), "Failed to read party"))
			return false;

		m_Parties.Insert(partyID, party);
				
		return true;
	}
	
	// ------------------------------------------------------------
	// ExpansionCOTGroupModule EditGroupName
	// Called on Client
	// ------------------------------------------------------------	
	void EditGroupName(int groupID, string groupName)
	{
		auto trace = EXTrace.Start(ExpansionTracing.COT_GROUPS, this);

		if (Expansion_Assert_False(IsMissionClient(), "[" + this + "] EditGroupName shall only be called on client!"))
			return;
		
		if (!GetPermissionsManager().HasPermission("Expansion.Groups.EditGroup"))
			return;
		
		auto rpc = Expansion_CreateRPC("RPC_EditGroupName");
		rpc.Write(groupID);
		rpc.Write(groupName);
 		rpc.Expansion_Send(true);
	}
	
	// ------------------------------------------------------------
	// ExpansionCOTGroupModule RPC_EditGroupName
	// Called on Server
	// ------------------------------------------------------------	
	private void RPC_EditGroupName(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
		auto trace = EXTrace.Start(ExpansionTracing.COT_GROUPS, this);
		
		if (!GetPermissionsManager().HasPermission("Expansion.Groups.EditGroup", sender))
			return;

		int groupID;
		if (!ctx.Read(groupID))
			return;
		
		string groupName;
		if (!ctx.Read(groupName))
			return;
		
		ExpansionPartyModule partyModule;
		if (!CF_Modules<ExpansionPartyModule>.Get(partyModule))
			return;
		
		ExpansionPartyData party = partyModule.GetPartyByID(groupID);
		if (!party)
			return;
		
		string currentName = party.GetPartyName();
		
		party.SetPartyName(groupName);
		party.Save();
		partyModule.UpdatePartyMembersServer(groupID);
		ExpansionNotification("[COT] Expansion Groups Manager", "Changed group name of party with ID " + party.GetPartyID() + ".").Success(sender);
		
		SendGroupUpdate(party, sender);
		
		Callback(ExpansionCOTGroupsMenuCallback.GroupUpdate, sender);
		
		AdminLog(sender.GetName() + " [" + sender.GetId() + "] has changed the group name of [" + party.GetPartyID() + "] " + party.GetPartyName() + " | Old: " + currentName + " | New: " + party.GetPartyName());
	}
	
	// ------------------------------------------------------------
	// ExpansionCOTGroupModule DeleteGroup
	// Called on Client
	// ------------------------------------------------------------	
	void DeleteGroup(int partyID)
	{
		auto trace = EXTrace.Start(ExpansionTracing.COT_GROUPS, this);

		if (Expansion_Assert_False(IsMissionClient(), "[" + this + "] DeleteGroup shall only be called on client!"))
			return;

		if (!GetPermissionsManager().HasPermission("Expansion.Groups.DeleteGroup"))
			return;
		
		if (m_Parties.Get(partyID))
			m_Parties.Remove(partyID);
		
		auto rpc = Expansion_CreateRPC("RPC_DeleteGroup");
		rpc.Write(partyID);
		rpc.Expansion_Send(true);
	}
	
	// ------------------------------------------------------------
	// ExpansionCOTGroupModule RPC_DeleteGroup
	// Called on Server
	// ------------------------------------------------------------	
	private void RPC_DeleteGroup(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
		auto trace = EXTrace.Start(ExpansionTracing.COT_GROUPS, this);
		
		if (!GetPermissionsManager().HasPermission("Expansion.Groups.DeleteGroup", sender))
			return;

		int partyID;
		if (!ctx.Read(partyID))
			return;
		
		ExpansionPartyModule partyModule;
		if (!CF_Modules<ExpansionPartyModule>.Get(partyModule))
			return;
		
		ExpansionPartyData party = partyModule.GetPartyByID(partyID);
		if (!party || partyID == -1)
		{
			ExpansionNotification("[COT] Expansion Groups Manager", "STR_EXPANSION_PARTY_ERROR_NOT_EXIST").Error(sender);
			return;
		}
		
		string partyName = party.GetPartyName();		
		if (!partyModule.DeletePartyServer(party))
		{
			Error("ExpansionCOTGroupModule::Exec_DissolvePartyServer - Could not get delete party from party module parties array!");
		}
		else
		{
			ExpansionNotification("[COT] Expansion Groups Manager", "Deleted group - ID:" + partyID + " - " + partyName + ".").Success(sender);
		}
		
		Callback(ExpansionCOTGroupsMenuCallback.GroupsUpdate, sender);
		
		AdminLog(sender.GetName() + " [" + sender.GetId() + "] has deleted the group [" + party.GetPartyID() + "] " + party.GetPartyName());
	}
	
	// ------------------------------------------------------------
	// ExpansionCOTGroupModule ChangeOwner
	// Called on Client
	// ------------------------------------------------------------	
	void ChangeOwner(string playerUID, int partyID, bool isMember)
	{
		auto trace = EXTrace.Start(ExpansionTracing.COT_GROUPS, this);

		if (Expansion_Assert_False(IsMissionClient(), "[" + this + "] ChangeOwner shall only be called on client!"))
			return;

		if (!GetPermissionsManager().HasPermission("Expansion.Groups.EditGroup"))
			return;
		
		auto rpc = Expansion_CreateRPC("RPC_ChangeOwner");
		rpc.Write(playerUID);
		rpc.Write(partyID);
		rpc.Write(isMember);
		rpc.Expansion_Send(true);
	}
	
	// ------------------------------------------------------------
	// ExpansionCOTGroupModule RPC_ChangeOwner
	// Called on Server
	// ------------------------------------------------------------	
	private void RPC_ChangeOwner(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
		auto trace = EXTrace.Start(ExpansionTracing.COT_GROUPS, this);
		
		if (!GetPermissionsManager().HasPermission("Expansion.Groups.EditGroup", sender))
			return;

		string playerUID;
		if (!ctx.Read(playerUID))
			return;
		
		int partyID;
		if (!ctx.Read(partyID))
			return;
		
		bool isMember;
		if (!ctx.Read(isMember))
			return;	
		
		ExpansionPartyModule partyModule;
		if (!CF_Modules<ExpansionPartyModule>.Get(partyModule))
			return;
		
		ExpansionPartyData party = partyModule.GetPartyByID(partyID);
		if (!party || partyID == -1)
		{
			Error("ExpansionCOTGroupModule::RPC_ChangeOwner - Could not get party from party module parties array!");
			ExpansionNotification("[COT] Expansion Groups Manager", "STR_EXPANSION_PARTY_ERROR_NOT_EXIST").Error(sender);
			return;
		}
				
		if (party.GetOwnerUID() == playerUID)
			return;
		
		if (!isMember && party.GetPlayers().Count() >= GetExpansionSettings().GetParty().MaxMembersInParty)
			return;

		if (!isMember)
		{
			if (!InvitePlayerServer(playerUID, partyID, sender))
				return;
		}
		
		ExpansionPartyPlayerData playerPartyData = party.GetPlayer(playerUID);
		if (!playerPartyData)
		{
			Error("ExpansionCOTGroupModule::RPC_ChangeOwner - Could not get party party data from party module!");
			ExpansionNotification("[COT] Expansion Groups Manager", "Could not get member's party player data!").Error(sender);
			return;
		}
		
		string oldOwnerID = party.GetOwnerUID();
		string oldOwnerName = party.GetOwnerName();
		playerPartyData.SetPermissions(ExpansionPartyPlayerPermissions.CAN_EDIT | ExpansionPartyPlayerPermissions.CAN_INVITE | ExpansionPartyPlayerPermissions.CAN_KICK | ExpansionPartyPlayerPermissions.CAN_DELETE | ExpansionPartyPlayerPermissions.CAN_WITHDRAW_MONEY);
		party.SetOwnerUID(playerUID);
		party.Save();
		partyModule.UpdatePartyMembersServer(partyID);
		ExpansionNotification("[COT] Expansion Groups Manager", "Changed group owner of Group: " + partyID + " - " + party.GetPartyName() + " | Old owner: " + oldOwnerName + " [" + oldOwnerID + "] | New owner: " + party.GetOwnerName() + " [" + party.GetOwnerUID() + "]").Success(sender);
		
		SendGroupUpdate(party, sender);
		
		Callback(ExpansionCOTGroupsMenuCallback.GroupUpdate, sender);
		
		AdminLog(sender.GetName() + " [" + sender.GetId() + "] has changed the owner of the group [" + party.GetPartyID() + "] " + party.GetPartyName() + " | Old owner: " + oldOwnerName + " [" + oldOwnerID + "] | New owner: " + party.GetOwnerName() + " [" + party.GetOwnerUID() + "]");
	}
	
	// ------------------------------------------------------------
	// ExpansionCOTGroupModule InvitePlayer
	// Called on Client
	// ------------------------------------------------------------
	void InvitePlayer(string playerUID, int partyID)
	{
		auto trace = EXTrace.Start(ExpansionTracing.COT_GROUPS, this);

		if (Expansion_Assert_False(IsMissionClient(), "[" + this + "] InvitePlayer shall only be called on client!"))
			return;

		if (!GetPermissionsManager().HasPermission("Expansion.Groups.EditGroup"))
			return;
				
		auto rpc = Expansion_CreateRPC("RPC_InvitePlayer");
		rpc.Write(playerUID);
		rpc.Write(partyID);
		rpc.Expansion_Send(true);
	}
	
	// ------------------------------------------------------------
	// ExpansionCOTGroupModule RPC_InvitePlayer
	// Called on Server
	// ------------------------------------------------------------	
	private void RPC_InvitePlayer(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
		auto trace = EXTrace.Start(ExpansionTracing.COT_GROUPS, this);
		
		if (!GetPermissionsManager().HasPermission("Expansion.Groups.EditGroup", sender))
			return;

		string playerUID;
		if (!ctx.Read(playerUID))
			return;
		
		int partyID;
		if (!ctx.Read(partyID))
			return;
		
		InvitePlayerServer(playerUID, partyID, sender);
	}
		
	// ------------------------------------------------------------
	// ExpansionCOTGroupModule InvitePlayerServer
	// Called on Server
	// ------------------------------------------------------------
	private bool InvitePlayerServer(string playerUID, int partyID, PlayerIdentity sender)
	{
		auto trace = EXTrace.Start(ExpansionTracing.COT_GROUPS, this);

		ExpansionPartyModule partyModule;
		if (!CF_Modules<ExpansionPartyModule>.Get(partyModule))
			return false;
		
		PlayerBase targetPlayer = PlayerBase.GetPlayerByUID(playerUID);
		if (!targetPlayer)
		{
			ExpansionNotification("[COT] Expansion Groups Manager", "STR_EXPANSION_PARTY_ERROR_PLAYERBASE").Error(sender);
			return false;
		}

		ExpansionPartyData party = partyModule.GetPartyByID(partyID);
		if (!party || partyID == -1)
		{
			ExpansionNotification("[COT] Expansion Groups Manager", "STR_EXPANSION_PARTY_ERROR_NOT_EXIST").Error(sender);
			return false;
		}

		if (GetExpansionSettings().GetParty().MaxMembersInParty >= 1 && party.GetPlayers().Count() >= GetExpansionSettings().GetParty().MaxMembersInParty)
		{
			ExpansionNotification("[COT] Expansion Groups Manager", new StringLocaliser("STR_EXPANSION_PARTY_ERROR_MAX_PARTY", GetExpansionSettings().GetParty().MaxMembersInParty.ToString())).Error(sender);
			return false;
		}
		
		if (targetPlayer.Expansion_GetParty())
		{
			ExpansionNotification("[COT] Expansion Groups Manager", "STR_EXPANSION_PARTY_ERROR_IN_PARTY").Error(sender);
			return false;
		}

		party.AddPlayer(targetPlayer, false);
		party.Save();
		ExpansionNotification("[COT] Expansion Groups Manager", "STR_EXPANSION_PARTY_MEMBER_ADDED").Success(sender);
		partyModule.UpdatePartyMembersServer(partyID);
		SendNotificationToMembers(new StringLocaliser("STR_EXPANSION_PARTY_MEMBER_ADDED_SENDER", targetPlayer.GetIdentity().GetName()), party, sender);
		
		SendGroupUpdate(party, sender);
		
		Callback(ExpansionCOTGroupsMenuCallback.GroupUpdate, sender);
		
		AdminLog(sender.GetName() + " [" + sender.GetId() + "] has invited the player " + targetPlayer.GetIdentity().GetName() + " into the group [" + party.GetPartyID() + "] " + party.GetPartyName());

		return true;
	}
	
	// ------------------------------------------------------------
	// ExpansionCOTGroupModule UpdatePermissions
	// Called on Client
	// ------------------------------------------------------------
	void UpdatePermissions(string playerUID, int partyID, int playerPerm)
	{
		auto trace = EXTrace.Start(ExpansionTracing.COT_GROUPS, this);

		if (Expansion_Assert_False(IsMissionClient(), "[" + this + "] UpdatePermissions shall only be called on client!"))
			return;
		
		if (!GetPermissionsManager().HasPermission("Expansion.Groups.EditGroup"))
			return;
		
		auto rpc = Expansion_CreateRPC("RPC_UpdatePermissions");
		rpc.Write(playerUID);
		rpc.Write(partyID);
		rpc.Write(playerPerm);
		rpc.Expansion_Send(true);
	}
	
	// ------------------------------------------------------------
	// ExpansionCOTGroupModule RPC_UpdatePermissions
	// Called on Server
	// ------------------------------------------------------------
	private void RPC_UpdatePermissions(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
		auto trace = EXTrace.Start(ExpansionTracing.COT_GROUPS, this);
		
		if (!GetPermissionsManager().HasPermission("Expansion.Groups.EditGroup", sender))
			return;

		string playerUID;
		if (!ctx.Read(playerUID))
		{
			ExpansionNotification("[COT] Expansion Groups Manager", "Could not get player's ID!").Error(sender);
			return;
		}

		int partyID;
		if (!ctx.Read(partyID))
		{
			ExpansionNotification("[COT] Expansion Groups Manager", "Could not get party id !").Error(sender);
			return;
		}
		
		int playerPerm;
		if (!ctx.Read(playerPerm))
		{
			ExpansionNotification("[COT] Expansion Groups Manager", "Could not get player's party player permissions data!").Error(sender);
			return;
		}

		ExpansionPartyModule partyModule;
		if (!CF_Modules<ExpansionPartyModule>.Get(partyModule))
			return;
		
		ExpansionPartyData party = partyModule.GetPartyByID(partyID);
		if (!party || partyID == -1)
		{
			ExpansionNotification("[COT] Expansion Groups Manager", "Could not get player's party data!").Error(sender);
			return;
		}

		ExpansionPartyPlayerData playerPartyData = party.GetPlayer(playerUID);
		if (!playerPartyData)
		{
			ExpansionNotification("[COT] Expansion Groups Manager", "Could not get member's party player data!").Error(sender);
			return;
		}
		
		if (party.GetOwnerUID() == playerUID)
		{
			ExpansionNotification("[COT] Expansion Groups Manager", "Cant change permissions of the group owner!").Error(sender);
			return;
		}

		int currentPerm = playerPartyData.GetPermissions();
		
		playerPartyData.SetPermissions(playerPerm);
		party.Save();
		partyModule.UpdatePartyMembersServer(party);

		string title = "[COT] Expansion Groups Manager";
		StringLocaliser text_sender = new StringLocaliser("STR_EXPANSION_PARTY_PERMISSIONS_CHANGED_SENDER", playerPartyData.GetName());

		//! Send notification to player who changed permissions
		ExpansionNotification(title, text_sender).Success(sender);

		//! Send notification to player whose permissions were changed (if online)
		PlayerBase targetPlayer = PlayerBase.GetPlayerByUID(playerUID);
		if (targetPlayer)
			ExpansionNotification(title, "STR_EXPANSION_PARTY_PERMISSIONS_CHANGED_RECIVER").Success(targetPlayer.GetIdentity());
		
		SendGroupUpdate(party, sender);
		
		Callback(ExpansionCOTGroupsMenuCallback.MemberUpdate, sender);
		
		AdminLog(sender.GetName() + " [" + sender.GetId() + "] has changed the group permissons of the player " + playerPartyData.GetName() + " from " + currentPerm + " to " + playerPartyData.GetPermissions());
	}
	
	// ------------------------------------------------------------
	// ExpansionCOTGroupModule ChangeMoney
	// Called on Client
	// ------------------------------------------------------------
	void ChangeMoney(int partyID, int value)
	{
		auto trace = EXTrace.Start(ExpansionTracing.COT_GROUPS, this);

		if (Expansion_Assert_False(IsMissionClient(), "[" + this + "] ChangeMoney shall only be called on client!"))
			return;
		
		if (!GetPermissionsManager().HasPermission("Expansion.Groups.EditGroup"))
			return;
		
		auto rpc = Expansion_CreateRPC("RPC_ChangeMoney");
		rpc.Write(partyID);
		rpc.Write(value);
		rpc.Expansion_Send(true);
	}
	
#ifdef EXPANSIONMODMARKET
	// ------------------------------------------------------------
	// ExpansionCOTGroupModule RPC_UpdatePermissions
	// Called on Server
	// ------------------------------------------------------------
	private void RPC_ChangeMoney(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
		auto trace = EXTrace.Start(ExpansionTracing.COT_GROUPS, this);
		
		if (!GetPermissionsManager().HasPermission("Expansion.Groups.EditGroup", sender))
			return;

		int partyID;
		if (!ctx.Read(partyID))
		{
			ExpansionNotification("[COT] Expansion Groups Manager", "Could not get party id!").Error(sender);
			return;
		}
		
		int value;
		if (!ctx.Read(value))
		{
			ExpansionNotification("[COT] Expansion Groups Manager", "Could not get money value!").Error(sender);
			return;
		}
		
		ExpansionPartyModule partyModule;
		if (!CF_Modules<ExpansionPartyModule>.Get(partyModule))
			return;
		
		ExpansionPartyData party = partyModule.GetPartyByID(partyID);
		if (!party || partyID == -1)
		{
			ExpansionNotification("[COT] Expansion Groups Manager", "Could not get party data!").Error(sender);
			return;
		}
		
		int currentDeposited = party.GetMoneyDeposited();
		if ((currentDeposited + value) >= GetExpansionSettings().GetMarket().MaxPartyDepositMoney || value < 0)
		{
			ExpansionNotification("[COT] Expansion Groups Manager", "Cant change money to " + value + " - [MIN:0, Max:" + GetExpansionSettings().GetMarket().MaxPartyDepositMoney + "].").Error(sender);
			return;
		}
		
		party.SetMoney(value);
		party.Save();
		ExpansionNotification("[COT] Expansion Groups Manager", "Group money changed to: " + value + ".").Success(sender);
		partyModule.UpdatePartyMembersServer(partyID);
		
		SendGroupUpdate(party, sender);
		
		Callback(ExpansionCOTGroupsMenuCallback.GroupUpdate, sender);
		
		AdminLog(sender.GetName() + " [" + sender.GetId() + "] has changed the group money from" + currentDeposited + " to " + party.GetMoneyDeposited() + " for the group [" + party.GetPartyID() + "] " + party.GetPartyName());
	}
#endif
	
	// ------------------------------------------------------------
	// ExpansionCOTGroupModule KickMember
	// Called on Client
	// ------------------------------------------------------------
	void KickMember(int partyID, string playerUID)
	{
		auto trace = EXTrace.Start(ExpansionTracing.COT_GROUPS, this);

		if (Expansion_Assert_False(IsMissionClient(), "[" + this + "] KickMember shall only be called on client!"))
			return;
		
		if (!GetPermissionsManager().HasPermission("Expansion.Groups.EditGroup"))
			return;
		
		auto rpc = Expansion_CreateRPC("RPC_KickMember");
		rpc.Write(partyID);
		rpc.Write(playerUID);
		rpc.Expansion_Send(true);
	}
	
	// ------------------------------------------------------------
	// ExpansionCOTGroupModule RPC_KickMember
	// Called on Server
	// ------------------------------------------------------------
	private void RPC_KickMember(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
		auto trace = EXTrace.Start(ExpansionTracing.COT_GROUPS, this);
		
		if (!GetPermissionsManager().HasPermission("Expansion.Groups.EditGroup", sender))
			return;

		int partyID;
		if (!ctx.Read(partyID))
		{
			ExpansionNotification("[COT] Expansion Groups Manager", "Could not get party id!").Error(sender);
			return;
		}
		
		string playerUID;
		if (!ctx.Read(playerUID))
		{
			ExpansionNotification("[COT] Expansion Groups Manager", "Could not get player UID!").Error(sender);
			return;
		}
		
		ExpansionPartyModule partyModule;
		if (!CF_Modules<ExpansionPartyModule>.Get(partyModule))
			return;
		
		ExpansionPartyData party = partyModule.GetPartyByID(partyID);
		if (!party || partyID == -1)
		{
			ExpansionNotification("[COT] Expansion Groups Manager", "Could not get party data!").Error(sender);
			return;
		}
		
		if (party.GetOwnerUID() == playerUID)
		{
			ExpansionNotification("[COT] Expansion Groups Manager", "STR_EXPANSION_PARTY_ERROR_REMOVE_OWNER").Error(sender);
			return;
		}
		
		ExpansionPartyPlayerData playerPartyData = partyModule.GetPartyPlayerData(playerUID);
		if (!playerPartyData)
			return;
		
		string targetName = playerPartyData.GetName();

		if (!party.RemoveMember(playerUID))
		{
			ExpansionNotification("[COT] Expansion Groups Manager", "STR_EXPANSION_PARTY_ERROR_PLAYER_NOT_IN").Error(sender);
			return;
		}

		party.Save();
		partyModule.UpdatePartyMembersServer(partyID);
		
		PlayerBase targetPlayer = PlayerBase.GetPlayerByUID(playerUID);
		PlayerIdentity targetIdentity;

		if (targetPlayer)
		{
			targetIdentity = targetPlayer.GetIdentity();
			partyModule.UpdatePlayerServer(NULL, targetPlayer);
			ExpansionNotification("[COT] Expansion Groups Manager", "STR_EXPANSION_PARTY_MEMBER_REMOVED").Success(targetPlayer.GetIdentity());
		}
		
		SendNotificationToMembers(new StringLocaliser("STR_EXPANSION_PARTY_MEMBER_REMOVED_SENDER"), party, targetIdentity);
		
		SendGroupUpdate(party, sender);
		
		Callback(ExpansionCOTGroupsMenuCallback.GroupUpdate, sender);
		
		AdminLog(sender.GetName() + " [" + sender.GetId() + "] has kicked the player " + targetName + " [" + playerUID + "] from the group [" + party.GetPartyID() + "] " + party.GetPartyName());
	}
	
	// ------------------------------------------------------------
	// ExpansionCOTGroupModule SendGroupUpdate
	// Called on Server
	// ------------------------------------------------------------		
	private void SendGroupUpdate(ExpansionPartyData party, PlayerIdentity sender)
	{
		auto trace = EXTrace.Start(ExpansionTracing.COT_GROUPS, this);

		int partyID = party.GetPartyID();
		auto rpc = Expansion_CreateRPC("RPC_SendGroupUpdate");
		party.OnSend(rpc, true);
		rpc.Expansion_Send(true, sender);
	}
	
	// ------------------------------------------------------------
	// ExpansionCOTGroupModule RPC_SendGroupUpdate
	// Called on Client
	// ------------------------------------------------------------
	private void RPC_SendGroupUpdate(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
		auto trace = EXTrace.Start(ExpansionTracing.COT_GROUPS, this);

		if (!OnReceiveGroupClient(ctx))
				return;
	}
	
#ifdef EXPANSIONMODNAVIGATION
	// ------------------------------------------------------------
	// ExpansionCOTGroupModule CreateGroupMarker
	// Called on Client
	// ------------------------------------------------------------
	void CreateGroupMarker(ExpansionMarkerData marker, int partyID)
	{
		auto trace = EXTrace.Start(ExpansionTracing.COT_GROUPS, this);

		if (Expansion_Assert_False(IsMissionClient(), "[" + this + "] CreateGroupMarker shall only be called on client!"))
			return;
		
		auto rpc = Expansion_CreateRPC("RPC_CreateGroupMarker");
		rpc.Write(partyID);
		marker.OnSend(rpc);
		rpc.Expansion_Send(true);
	}
	
	// ------------------------------------------------------------
	// ExpansionCOTGroupModule RPC_CreateGroupMarker
	// Called on Server
	// ------------------------------------------------------------
	private void RPC_CreateGroupMarker(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
		auto trace = EXTrace.Start(ExpansionTracing.COT_GROUPS, this);
		
		if (!GetPermissionsManager().HasPermission("Expansion.Groups.EditGroup", sender))
			return;

		int partyID;
		if (!ctx.Read(partyID))
			return;
		
		ExpansionMarkerData marker = ExpansionMarkerData.Create(ExpansionMapMarkerType.PARTY);
		if (!marker.OnRecieve(ctx))
			return;
		
		ExpansionPartyModule partyModule;
		if (!CF_Modules<ExpansionPartyModule>.Get(partyModule))
			return;
		
		ExpansionPartyData party = partyModule.GetPartyByID(partyID);
		if (!party || partyID == -1)
		{
			ExpansionNotification("STR_EXPANSION_PARTY_NOTIF_TITLE", "STR_EXPANSION_PARTY_ERROR_NOT_EXIST").Error(sender);
			return;
		}

		party.AddMarker(marker);
		party.Save();

		ExpansionNotification("STR_EXPANSION_PARTY_NOTIF_TITLE", "STR_EXPANSION_PARTY_MARKER_ADDED").Success(sender);

		partyModule.UpdatePartyMembersServer(partyID);
		
		SendGroupUpdate(party, sender);
		
		Callback(ExpansionCOTGroupsMenuCallback.MarkersUpdate, sender);
	}
#endif
	
	// ------------------------------------------------------------
	// ExpansionCOTGroupModule Callback
	// Called on Server
	// ------------------------------------------------------------	
	void Callback(int callBack, PlayerIdentity receiver)
	{
		auto trace = EXTrace.Start(ExpansionTracing.COT_GROUPS, this);

		auto rpc = Expansion_CreateRPC("RPC_Callback");
		rpc.Write(callBack);
		rpc.Expansion_Send(true, receiver);
	}
	
	// ------------------------------------------------------------
	// ExpansionCOTGroupModule RPC_Callback
	// Called on Client
	// ------------------------------------------------------------
	private void RPC_Callback(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
		auto trace = EXTrace.Start(ExpansionTracing.COT_GROUPS, this);

		int callBack;
		if (!ctx.Read(callBack))
			return;
		
		switch (callBack)
		{
			case ExpansionCOTGroupsMenuCallback.GroupsUpdate:
				GetModuleSI().Invoke(ExpansionCOTGroupsMenuCallback.GroupsUpdate);
				break;
			case ExpansionCOTGroupsMenuCallback.GroupUpdate:
				GetModuleSI().Invoke(ExpansionCOTGroupsMenuCallback.GroupUpdate);
				break;
			case ExpansionCOTGroupsMenuCallback.MemberUpdate:
				GetModuleSI().Invoke(ExpansionCOTGroupsMenuCallback.MemberUpdate);
				break;
			case ExpansionCOTGroupsMenuCallback.MarkersUpdate:
				GetModuleSI().Invoke(ExpansionCOTGroupsMenuCallback.MarkersUpdate);
				break;
		}
	}
	
	// ------------------------------------------------------------
	// ExpansionCOTGroupModule SendNotificationToMembers
	// Called on Server
	// ------------------------------------------------------------
	void SendNotificationToMembers(StringLocaliser text, ExpansionPartyData party, PlayerIdentity sender)
	{
		auto trace = EXTrace.Start(ExpansionTracing.COT_GROUPS, this);

		if (Expansion_Assert_False(IsMissionHost(), "[" + this + "] SendNotificationToMembers shall only be called on server!"));
			return;
		
		array<ref ExpansionPartyPlayerData> players = party.GetPlayers();
	   	if ( !players )
			return;

		foreach (ExpansionPartyPlayerData playerData : players)
		{
			if (!playerData)
				continue;

			PlayerBase partyPlayer = PlayerBase.GetPlayerByUID(playerData.UID);
			if (partyPlayer && partyPlayer.GetIdentity())
			{
				if (partyPlayer.GetIdentityUID() != sender.GetId())
				{
					ExpansionNotification("[COT] Expansion Groups Manager", text).Success(partyPlayer.GetIdentity());
				}
			}
		}
	}
	
	// ------------------------------------------------------------
	// ExpansionCOTGroupModule GetAllGroups
	// Called on Client
	// ------------------------------------------------------------	
	map<int, ref ExpansionPartyData> GetAllGroups()
	{
		return m_Parties;
	}
	
	// ------------------------------------------------------------
	// ExpansionCOTGroupModule GetGroupPlayerData
	// Called on Client
	// ------------------------------------------------------------	
	ExpansionPartyPlayerData GetGroupPlayerData(string uid)
	{
		auto trace = EXTrace.Start(ExpansionTracing.COT_GROUPS, this);

		foreach (int i, ExpansionPartyData data : m_Parties)
		{
			ExpansionPartyPlayerData party_player = data.GetPlayer(uid);
			if (!party_player)
				continue;

			return party_player;
		}

		return NULL;
	}
	
	// ------------------------------------------------------------
	// ExpansionCOTGroupModule HasGroup
	// Called on Client
	// ------------------------------------------------------------		
	bool HasGroup(string uid)
	{
		auto trace = EXTrace.Start(ExpansionTracing.COT_GROUPS, this);

		foreach (int i, ExpansionPartyData party : m_Parties)
		{
			if (party && party.GetPlayer(uid))
				return true;
		}

		return false;
	}
	
	// ------------------------------------------------------------
	// ExpansionCOTGroupModule GetModuleSI
	// Called on Client
	// ------------------------------------------------------------	
	ScriptInvoker GetModuleSI()
	{
		return m_COTGroupModuleSI;
	}
	
	// ------------------------------------------------------------
	// ExpansionCOTGroupModule AdminLog
	// Called on Server
	// ------------------------------------------------------------		
	void AdminLog(string msg)
	{
		auto trace = EXTrace.Start(ExpansionTracing.COT_GROUPS, this);

		if (GetExpansionSettings().GetLog().Party)
			GetExpansionSettings().GetLog().PrintLog("[COT] Expansion Groups Manager - " + msg);
	}
};
#endif
