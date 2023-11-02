/**
 * ExpansionPersonalStorageContainer.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2023 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

class ExpansionPersonalStorageContainer: ExpansionOwnedContainer
{
	static const string EXPANSION_STORAGE_DIR = "$storage:expansion\\personalstoragenew\\";

	static ref ScriptInvoker SI_Expansion_OpenPersonalStorageMenu = new ScriptInvoker;

	bool m_Expansion_PersonalStorageExcludeSlotShoulder;
	bool m_Expansion_PersonalStorageExcludeSlotMelee;
	bool m_Expansion_PersonalStorageExcludeSlotVest;
	bool m_Expansion_PersonalStorageExcludeSlotBody;
	bool m_Expansion_PersonalStorageExcludeSlotHips;
	bool m_Expansion_PersonalStorageExcludeSlotLegs;
	bool m_Expansion_PersonalStorageExcludeSlotBack;
	bool m_Expansion_PersonalStorageExcludeSlotHeadgear;
	bool m_Expansion_PersonalStorageExcludeSlotMask;
	bool m_Expansion_PersonalStorageExcludeSlotEyewear;
	bool m_Expansion_PersonalStorageExcludeSlotGloves;
	bool m_Expansion_PersonalStorageExcludeSlotFeet;
	bool m_Expansion_PersonalStorageExcludeSlotArmband;
	bool m_Expansion_PersonalStorageAllowAttachmentCargo;

	int m_Expansion_PersonalStorageHub_IdLow;
	int m_Expansion_PersonalStorageHub_IdHigh;
	ExpansionPersonalStorageHub m_Expansion_PersonalStorageHub;
	int m_Expansion_SetPersonalStorageHub;

	void ExpansionPersonalStorageContainer()
	{
		auto trace = EXTrace.Start(EXTrace.PERSONALSTORAGE, this);

		TStringArray slots = {};
		ConfigGetTextArray("attachments", slots);

		foreach (string slot: slots)
		{
			RegisterNetSyncVariableBool("m_Expansion_PersonalStorageExcludeSlot" + slot);
		}

		RegisterNetSyncVariableBool("m_Expansion_PersonalStorageAllowAttachmentCargo");

		RegisterNetSyncVariableInt("m_Expansion_PersonalStorageHub_IdLow");
		RegisterNetSyncVariableInt("m_Expansion_PersonalStorageHub_IdHigh");
		RegisterNetSyncVariableInt("m_Expansion_SetPersonalStorageHub", 1, 2);

#ifndef SERVER
		ClearFlags(EntityFlags.VISIBLE, false);
#else
		string type = GetType();
		int index = type.IndexOf("_Level") + 6;
		int lvl = type.Substring(index, type.Length() - index).ToInt();

		auto settings = GetExpansionSettings().GetPersonalStorageNew();
		auto storageLevel = settings.StorageLevels[lvl];
		if (storageLevel)
		{
			foreach (string excludedSlot: storageLevel.ExcludedSlots)
			{
				EnScript.SetClassVar(this, "m_Expansion_PersonalStorageExcludeSlot" + excludedSlot, 0, true);
			}

			m_Expansion_PersonalStorageAllowAttachmentCargo = storageLevel.AllowAttachmentCargo;
		}
#endif
		if (!m_Expansion_RPCManager)
			m_Expansion_RPCManager = new ExpansionRPCManager(this, ItemBase);
		m_Expansion_RPCManager.RegisterServer("RPC_Expansion_RequestMoveItem");
	}

	override void OnVariablesSynchronized()
	{
		auto trace = EXTrace.Start(EXTrace.PERSONALSTORAGE, this);

		super.OnVariablesSynchronized();

		if (m_Expansion_SetPersonalStorageHub)
		{
			m_Expansion_SetPersonalStorageHub = 0;
			ExpansionPersonalStorageHub hub;
			if (Class.CastTo(hub, GetGame().GetObjectByNetworkId(m_Expansion_PersonalStorageHub_IdLow, m_Expansion_PersonalStorageHub_IdHigh)))
				Expansion_SetPersonalStorageHub(hub);
		}
	}

	void Expansion_SetPersonalStorageHub(ExpansionPersonalStorageHub hub, bool sync = true)
	{
		auto trace = EXTrace.Start(EXTrace.PERSONALSTORAGE, this);

		m_Expansion_PersonalStorageHub = hub;

	#ifdef SERVER
		if (sync)
		{
			hub.GetNetworkID(m_Expansion_PersonalStorageHub_IdLow, m_Expansion_PersonalStorageHub_IdHigh);
			if (m_Expansion_SetPersonalStorageHub > 1)
				m_Expansion_SetPersonalStorageHub = 1;
			else
				m_Expansion_SetPersonalStorageHub = 2;
			SetSynchDirty();
		}
	#endif
	}

	override void Expansion_OnOwnerSync()
	{
		super.Expansion_OnOwnerSync();

		if (!ExpansionIsContainerOwner() || !Expansion_IsOwnerNear())
			return;

		if (GetExpansionSettings().GetPersonalStorageNew().UseCategoryMenu)
		{
			GetDayZGame().GetExpansionGame().GetExpansionUIManager().CreateSVMenu("ExpansionPersonalStorageMenu");
			SI_Expansion_OpenPersonalStorageMenu.Invoke(this);
		}
		else
		{
			GetGame().GetMission().ShowInventory();
		}
	}

	override bool CanDisplayAttachmentSlot(int slot_id)
	{
		if (!super.CanDisplayAttachmentSlot(slot_id))
			return false;

		return !Expansion_IsSlotExcluded(slot_id);
	}

	override bool CanObstruct()
	{
		return false;
	}

	override bool CanPutIntoHands(EntityAI parent)
	{
		return false;
	}

	override bool CanReceiveAttachment(EntityAI attachment, int slotId)
	{
		if (!super.CanReceiveAttachment(attachment, slotId))
			return false;

		if (Expansion_IsSlotExcluded(slotId))
			return false;

		if (!m_Expansion_PersonalStorageAllowAttachmentCargo && attachment.HasAnyCargo())
			return false;

		return true;
	}

	override void EEDelete(EntityAI parent)
	{
		super.EEDelete(parent);

		string uid = ExpansionGetContainerOwnerUID();
		EXTrace.Print(EXTrace.PERSONALSTORAGE, this, "::EEDelete - owner UID " + uid);
		#ifdef SERVER
		ExpansionPersonalStorageHub.s_Expansion_PersonalStorageContainers.Remove(uid);
		#endif
		ExpansionPersonalStorageHub.s_Expansion_PersonalStorageHubs.Remove(uid);
	}

	override void OnCEUpdate()
	{
		super.OnCEUpdate();

		if (Expansion_IsOwnerNear())
			return;

		EXTrace.Print(EXTrace.PERSONALSTORAGE, this, "::OnCEUpdate - abandoned - storing contents");

		Expansion_StoreContentsAndDelete();
	}

	bool Expansion_IsOwnerNear()
	{
		PlayerBase owner = ExpansionGetContainerOwner();
		return owner && vector.DistanceSq(owner.GetPosition(), GetPosition()) < UAMaxDistances.DEFAULT * UAMaxDistances.DEFAULT;
	}

	override bool Expansion_CanUseVirtualStorage(bool restoreOverride = false)
	{
		return false;
	}

	bool Expansion_IsSlotExcluded(int slotId)
	{
		string slot = InventorySlots.GetSlotName(slotId);
		bool exclude;
		EnScript.GetClassVar(this, "m_Expansion_PersonalStorageExcludeSlot" + slot, 0, exclude);
		return exclude;
	}

	string Expansion_GetPersonalStorageFileName()
	{
		return EXPANSION_STORAGE_DIR + ExpansionGetContainerOwnerUID() + ExpansionEntityStorageModule.EXT;
	}

	bool Expansion_StoreContentsAndDelete()
	{
		bool success = Expansion_StoreContents();
		if (success)
			Delete();
		return success;
	}

	override bool Expansion_StoreContents()
	{
		if (!ExpansionGetContainerOwnerUID())
			return false;

		if (!FileExist(EXPANSION_STORAGE_DIR))
			ExpansionStatic.MakeDirectoryRecursive(EXPANSION_STORAGE_DIR);

		string fileName = Expansion_GetPersonalStorageFileName();

		if (ExpansionEntityStorageModule.SaveToFile(this, fileName, true, null, true))
		{
			EXTrace.Print(EXTrace.PERSONALSTORAGE, this, "::Expansion_StoreContents - " + GetPosition() + " - saved inventory to " + fileName);
			return true;
		}
		else
		{
			EXTrace.Print(EXTrace.PERSONALSTORAGE, this, "::Expansion_StoreContents - " + GetPosition() + " - could not save inventory to " + fileName);
		}

		return false;
	}

	override bool Expansion_RestoreContents(PlayerBase player = null)
	{
		if (!ExpansionGetContainerOwnerUID() || GetInventory().CountInventory() > 1)
			return false;

		string fileName = Expansion_GetPersonalStorageFileName();

		if (!FileExist(fileName))
			return false;

		EntityAI entity = this;
		//! When personal storage contents are loaded into the container, keep files
		//! to avoid data loss in case of server crash before the container was persisted.
		//! After container was persisted, storage files are deleted in CF_OnStoreSave.
		if (ExpansionEntityStorageModule.RestoreFromFile(fileName, entity, null, null, false))
		{
			EXTrace.Print(EXTrace.PERSONALSTORAGE, this, "::Expansion_RestoreContents - " + GetPosition() + " - restored inventory from " + fileName);

			return true;
		}
		else
		{
			EXTrace.Print(EXTrace.PERSONALSTORAGE, this, "::Expansion_RestoreContents - " + GetPosition() + " - could not restore inventory from " + fileName);
		}

		return false;
	}

	#ifdef EXPANSION_MODSTORAGE
	override void CF_OnStoreSave(CF_ModStorageMap storage)
	{
		bool wasAlreadyStoreSaved = m_Expansion_IsStoreSaved;

		super.CF_OnStoreSave(storage);

		if (wasAlreadyStoreSaved)
			return;

		string uid = ExpansionGetContainerOwnerUID();
		if (!uid)
			return;

		//! Container and contents have been saved to vanilla storage, delete personal storage files (if any)
		string fileName = Expansion_GetPersonalStorageFileName();
		if (FileExist(fileName))
		{
			EXTrace.Print(EXTrace.PERSONALSTORAGE, this, "::CF_OnStoreSave - " + GetPosition() + " - deleting " + fileName);
			DeleteFile(fileName);
		}

		string folder = EXPANSION_STORAGE_DIR + uid + "\\";
		TStringArray files = ExpansionStatic.FindFilesInLocation(folder, ExpansionEntityStorageModule.EXT);
		foreach (string baseName: files)
		{
			EXTrace.Print(EXTrace.PERSONALSTORAGE, this, "::CF_OnStoreSave - " + GetPosition() + " - deleting " + baseName);
			DeleteFile(folder + baseName);
		}
	}
	#endif

	void Expansion_RequestMoveItem(EntityAI item)
	{
		auto rpc = m_Expansion_RPCManager.CreateRPC("RPC_Expansion_RequestMoveItem");
		rpc.Write(item);
		rpc.Expansion_Send(true);
	}

	void RPC_Expansion_RequestMoveItem(PlayerIdentity sender, ParamsReadContext ctx)
	{
		Man player = sender.GetPlayer();

		if (!player)
			return;

		if (ExpansionGetContainerOwnerUID() != sender.GetId())
		{
			ExpansionNotification("Access denied", "Cannot access another player's personal storage!").Error(sender);
			return;
		}

		EntityAI item;
		if (!ctx.Read(item))
		{
			ExpansionNotification("Couldn't read item", "Couldn't read item").Error(sender);
			return;
		}

		InventoryLocation srcLoc = new InventoryLocation;
		if (!item.GetInventory().GetCurrentInventoryLocation(srcLoc))
		{
			ExpansionNotification("Couldn't get inventory location", "Couldn't get current inventory location for " + item.GetDisplayName()).Error(sender);
			return;
		}

		EntityAI dst;

		if (!item.GetHierarchyParent())
		{
			ExpansionNotification("Can't move item", "Can't move " + item.GetDisplayName() + " from ground").Error(sender);
			return;
		}
		else if (item.GetHierarchyRootPlayer() == player)
		{
			//! deposit
			dst = this;
		}
		else if (item.GetHierarchyRoot() == this)
		{
			//! retrieve
			dst = player;
		}
		else
		{
			ExpansionNotification("Can't remove item", "Can't remove " + item.GetDisplayName() + " from " + item.GetHierarchyRoot().GetDisplayName()).Error(sender);
			return;
		}

		InventoryLocation dstLoc = new InventoryLocation;
		if (!dst.GetInventory().FindFreeLocationFor(item, FindInventoryLocationType.ANY, dstLoc))
		{
			ExpansionNotification("Not enough space in inventory", "Not enough space in inventory of " + dst.GetDisplayName()).Error(sender);
			return;
		}

		if (!player.GetHumanInventory().TakeToDst(InventoryMode.SERVER, srcLoc, dstLoc))
		{
			ExpansionNotification("Couldn't move item", "Couldn't move item into inventory of " + dst.GetDisplayName()).Error(sender);
			return;
		}
	}
	
	ExpansionPersonalStorageHub Expansion_GetPersonalStorageHub()
	{
		return m_Expansion_PersonalStorageHub;
	}
}
