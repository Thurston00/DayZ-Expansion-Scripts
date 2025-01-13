/**
 * ExpansionEntityStorage.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

class ExpansionCartridgeInfo
{
	int Count;
	float Damage;
	string Type;

	void OnSave(ParamsWriteContext ctx)
	{
		ctx.Write(Count);
		ctx.Write(Damage);
		ctx.Write(Type);
	}

	bool OnLoad(ParamsReadContext ctx)
	{
		if (!ctx.Read(Count))
			return false;
		if (!ctx.Read(Damage))
			return false;
		if (!ctx.Read(Type))
			return false;

		return true;
	}
}

class ExpansionEntityStorageContext
{
	ref FileSerializer m_Context;
	string m_FileName;
	int m_FailedCount;

	void ExpansionEntityStorageContext()
	{
		m_Context = new FileSerializer();
	}

	bool Open(string fileName, int mode)
	{
		m_FileName = fileName;
		return m_Context.Open(fileName, mode);
	}

	void Close()
	{
		m_Context.Close();
	}
}


/**
 * Vanilla order:
 * 
 * OnStoreLoad
 * For each attachment/cargo (if any):
 *   attachment OnStoreLoad
 *   attachment AfterStoreLoad
 * AfterStoreLoad
 * 
 * EntityStorage order:
 * 
 * For each attachment/cargo (if any):
 *   attachment OnStoreLoad
 *   attachment AfterStoreLoad
 * OnStoreLoad
 * AfterStoreLoad
 * 
 * @note This difference is intentional so that restored parent entity state is less likely to interfere with creation of child entities.
 */

[CF_RegisterModule(ExpansionEntityStorageModule)]
class ExpansionEntityStorageModule: CF_ModuleWorld
{
	static const int VERSION = 13;
	static const string EXT = ".bin";

	static const int FAILURE = 0;
	static const int SUCCESS = 1;
	static const int SKIP = 2;

	static ref map<string, ref ExpansionEntityStorageContext> s_SubContexts = new map<string, ref ExpansionEntityStorageContext>;

	static int Now;

	static EntityAI s_SavedEntityToBeDeleted;
	static EntityAI s_CurrentEntity;
	static EntityAI s_CurrentRootEntity;
	static int s_LastRestoredVersion;

	void ExpansionEntityStorageModule()
	{
	#ifdef JM_COT
		GetPermissionsManager().Expansion_RegisterPermission("Expansion.EntityStorage.Goatify", JMPermissionType.DISALLOW);
	#endif
	}

#ifdef SERVER
	override void OnInit()
	{
		super.OnInit();

		EnableMissionStart();
	}

	override void OnMissionStart(Class sender, CF_EventArgs args)
	{
	#ifdef EXTRACE
		auto trace = EXTrace.Start(ExpansionTracing.GENERAL_ITEMS, this);
	#endif

		super.OnMissionStart(sender, args);

		if (!FileExist(GetStorageDirectory()))
			ExpansionStatic.MakeDirectoryRecursive(GetStorageDirectory());
	}
#endif

	//! @brief save entity and all its children (attachments/cargo) to ctx. Will not delete the entity!
	static bool Save(EntityAI entity, ParamsWriteContext ctx, string basePath, bool inventoryOnly = false, EntityAI placeholder = null, int level = 0, TStringArray orphanedFiles = null)
	{
		if (!level)
		{
			Reset();
			ResetState();

			ctx.Write(VERSION);
			ctx.Write(Now);
		}

		//! @note order of operations matters! DO NOT CHANGE!

		if (!inventoryOnly || level > 0)
		{
			int result = Save_Phase1(ctx, entity, inventoryOnly, level);
			if (result == FAILURE)
				return false;
			else if (result == SKIP)
				return true;
		}

		ExpansionEntityStorageContext context;
		FileSerializer file;

		if (level > 0)
		{
			string type = entity.GetType();

			if (!s_SubContexts.Find(type, context))
			{
				string childFileName = string.Format("%1\\%2%3", basePath, type, EXT);
				context = new ExpansionEntityStorageContext();
				if (!context.Open(childFileName, FileMode.WRITE))
					return ErrorFalse("Couldn't open file for writing " + childFileName);
				s_SubContexts[type] = context;
			}

			file = context.m_Context;
		}
		else
		{
			file = FileSerializer.Cast(ctx);
		}

		if (!Save_Phase2(file, basePath, entity, inventoryOnly, placeholder, level, orphanedFiles))
			return false;

		if (!inventoryOnly || level > 0)
			Save_Phase3(file, entity);

		s_CurrentEntity = null;

		if (!level)
		{
			EXTrace.Print(EXTrace.GENERAL_ITEMS, ExpansionEntityStorageModule, "Save " + s_CurrentRootEntity + " -> null");
			s_CurrentRootEntity = null;
		}

		return true;
	}

	static int Save_Phase1(ParamsWriteContext ctx, EntityAI entity, bool inventoryOnly = false, int level = 0)
	{
		InventoryLocation il = new InventoryLocation();
		GameInventory inventory = entity.GetInventory();
		if (inventory)
		{
			inventory.GetCurrentInventoryLocation(il);
		}
		else
		{
			vector mat[4];
			entity.GetTransform(mat);
			il.SetGround(null, mat);
		}
		if (!il.IsValid())
		{
			EXError.Error(entity, "Invalid location");
			return FAILURE;
		}
		InventoryLocationType ilt = il.GetType();
		EntityAI parent = entity.GetHierarchyParent();
		if (inventoryOnly && ilt == InventoryLocationType.ATTACHMENT && level == 1)
		{
			//! If only saving inventory, don't save attachments on hierarchy root if it is openable or attachment slot is locked
			bool isOpenable;
#ifdef EXPANSIONMODBASEBUILDING
			ItemBase itemParent;
			if (Class.CastTo(itemParent, parent) && (itemParent.ExpansionIsOpenable() || itemParent.IsNonExpansionOpenable()))
				isOpenable = true;
#endif
			if (isOpenable || parent.GetInventory().GetSlotLock(il.GetSlot()) || entity.IsKindOf("CombinationLock") || entity.IsKindOf("ExpansionCodeLock") || entity.IsKindOf("RA_CodeLock") || entity.IsKindOf("CodeLock"))
			{
				EXTrace.Print(EXTrace.GENERAL_ITEMS, parent, "ExpansionEntityStorage::Save_Phase1 - skippping " + entity.GetType() + " in locked slot");
				ctx.Write("");  //! Have to write empty entry because we already have written inventory count and no way to retroactively overwrite it
				return SKIP;
			}
		}
		EXTrace.Print(EXTrace.GENERAL_ITEMS, parent, "ExpansionEntityStorage::Save_Phase1 " + entity.GetType() + " inventory location type " + typename.EnumToString(InventoryLocationType, ilt));

		//! 1a) entity type
		ctx.Write(entity.GetType());

		//! 1b) location
		ctx.Write(ilt);
		switch (ilt)
		{
			case InventoryLocationType.GROUND:
				ctx.Write(entity.GetPosition());
				ctx.Write(entity.GetOrientation());
				break;
			case InventoryLocationType.ATTACHMENT:
				int slotId = il.GetSlot();
				ctx.Write(slotId);
				ctx.Write(parent.GetInventory().GetSlotLock(slotId));
				break;
			case InventoryLocationType.HANDS:
				//! Nothing to do
				break;
			case InventoryLocationType.CARGO:
			case InventoryLocationType.PROXYCARGO:
				ctx.Write(il.GetIdx());
				ctx.Write(il.GetRow());
				ctx.Write(il.GetCol());
				ctx.Write(il.GetFlip());
				break;
			default:
				EXError.Error(entity, "Unknown location type " + typename.EnumToString(InventoryLocationType, ilt));
				return FAILURE;
				break;
		}

		s_CurrentEntity = entity;

		if (!level)
		{
			EXTrace.Print(EXTrace.GENERAL_ITEMS, ExpansionEntityStorageModule, "Save_Phase1 " + s_CurrentRootEntity + " -> " + entity);
			s_CurrentRootEntity = entity;
		}

		return SUCCESS;
	}

	static int Save_Phase2a(ParamsWriteContext ctx, int itemCount, string basePath, int level)
	{
		ctx.Write(itemCount);

		if (itemCount && !level && !FileExist(basePath))
			MakeDirectory(basePath);

		return itemCount;
	}

	static bool Save_Phase2(ParamsWriteContext ctx, string basePath, EntityAI entity, bool inventoryOnly = false, EntityAI placeholder = null, int level = 0, TStringArray orphanedFiles = null)
	{
		//! 2) attachments + cargo
		GameInventory inventory = entity.GetInventory();
		CargoBase cargo;
		int attCount;
		int cargoItemCount;

		if (inventory)
		{
			if (!level && placeholder && entity.HasAnyCargo() && !MiscGameplayFunctions.Expansion_MoveCargo(entity, placeholder))
				EXPrint("[EntityStorage] " + entity.GetType() + ": Couldn't move cargo to placeholder " + placeholder.GetType() + " - serializing instead");
			attCount = inventory.AttachmentCount();
			cargo = inventory.GetCargo();
			if (cargo)
				cargoItemCount = cargo.GetItemCount();
		}

		int inventoryCount = attCount + cargoItemCount;

		bool success = true;

		if (entity.IsMan())
		{
			Man player = Man.Cast(entity);
			EntityAI entityInHands = player.GetHumanInventory().GetEntityInHands();
			if (entityInHands)
			{
				Save_Phase2a(ctx, inventoryCount + 1, basePath, level);
				success = Save(entityInHands, ctx, basePath, inventoryOnly, null, level + 1);
			}
			else if (Save_Phase2a(ctx, inventoryCount, basePath, level) == 0)
			{
				return true;
			}
		}
		else if (Save_Phase2a(ctx, inventoryCount, basePath, level) == 0)
		{
			return true;
		}

		for (int i = 0; i < inventoryCount; i++)
		{
			EntityAI item;
			if (i < attCount)
				item = inventory.GetAttachmentFromIndex(i);
			else
				item = cargo.GetItem(i - attCount);

			success = Save(item, ctx, basePath, inventoryOnly, null, level + 1);

			if (!success)
				break;
		}

		if (!level)
		{
			Reset(false, orphanedFiles);

			if (orphanedFiles)
			{
				//! Remove orphaned files
				foreach (string orphanedFile: orphanedFiles)
				{
					EXTrace.Print(EXTrace.GENERAL_ITEMS, entity, "ExpansionEntityStorage::Save_Phase2 - deleting orphaned " + orphanedFile);
					DeleteFile(basePath + "\\" + orphanedFile);
				}
			}
		}

		if (!success)
			return false;

		return true;
	}

	static void Save_Phase3(ParamsWriteContext ctx, EntityAI entity)
	{
		int i;

		s_CurrentEntity = entity;

		//! 3) special treatment for weapons
		Weapon_Base weapon;
		if (Class.CastTo(weapon, entity))
		{
			float ammoDamage;
			string ammoTypeName;

			ctx.Write(weapon.GetMuzzleCount());
			for (int mi = 0; mi < weapon.GetMuzzleCount(); ++mi)
			{
				ctx.Write(weapon.IsChamberEmpty(mi));
				if (!weapon.IsChamberEmpty(mi))
				{
					if (weapon.GetCartridgeInfo(mi, ammoDamage, ammoTypeName))
					{
						ctx.Write(ammoDamage);
						ctx.Write(ammoTypeName);
					}
				}

				ctx.Write(weapon.GetInternalMagazineCartridgeCount(mi));
				for (int ci = 0; ci < weapon.GetInternalMagazineCartridgeCount(mi); ++ci)
				{
					if (weapon.GetInternalMagazineCartridgeInfo(mi, ci, ammoDamage, ammoTypeName))
					{
						ctx.Write(ammoDamage);
						ctx.Write(ammoTypeName);
					}
				}
			}
		}

		//! 4) storesave
		int version = GetGame().SaveVersion();
		ctx.Write(version);
		entity.OnStoreSave(ctx);

		DayZPlayerImplement player;
		if (Class.CastTo(player, entity))
		{
			ctx.Write(player.Expansion_GetEntityStorageAdditionalDataVersion());
			player.Expansion_OnEntityStorageAdditionalDataSave(ctx);
		}

		//! 5) special treatment for mags/ammo
		Magazine mag;
		if (Class.CastTo(mag, entity))
		{
			if (mag.IsAmmoPile())
			{
				ctx.Write(mag.GetAmmoCount());
			}
			else
			{
				//! Need to store actual cartridges
				//! Try to optimize amount of data written
				auto cartridges = new array<ref ExpansionCartridgeInfo>;
				ExpansionCartridgeInfo info;
				float damage;  //! @note damage is the damage of the cartridge itself (0..1), NOT the damage it inflicts!
				string cartTypeName;
				for (i = 0; i < mag.GetAmmoCount(); i++)
				{
					mag.GetCartridgeAtIndex(i, damage, cartTypeName);
					if (!info || damage != info.Damage || cartTypeName != info.Type)
					{
						info = new ExpansionCartridgeInfo;
						info.Damage = damage;
						info.Type = cartTypeName;
						cartridges.Insert(info);
					}
					info.Count++;
				}
				ctx.Write(cartridges.Count());
				foreach (auto cartridge: cartridges)
				{
					cartridge.OnSave(ctx);
				}
			}
		}

		//! 6) Special treatment for vehicles (no special treatment needed for ExpansionVehicleBase since it inherits from ItemBase)
		ExpansionVehicle vehicle;
		if (!entity.IsItemBase() && ExpansionVehicle.Get(vehicle, entity))
		{
			vehicle.WriteFluidsTo(ctx);
		}

		//! 7) global health and damage zones
		ctx.Write(entity.GetHealth());
		TStringArray dmgZones = new TStringArray();
		entity.GetDamageZones(dmgZones);
		ctx.Write(dmgZones.Count());
		foreach (string dmgZone: dmgZones)
		{
			float dmgZoneHealth = entity.GetHealth(dmgZone, "Health");
			ctx.Write(dmgZone);
			ctx.Write(dmgZoneHealth);
		}
	}

	static bool ErrorFalse(string message)
	{
		EXError.Error(null, message);
		return false;
	}

	//! @brief restore entity and all its children from ctx (creates entities)
	static bool Restore(ParamsReadContext ctx, FileSerializer file, string basePath = string.Empty, inout EntityAI entity = null, EntityAI parent = null, EntityAI placeholder = null, PlayerBase player = null, int entityStorageVersion = 0, string type = string.Empty, int level = 0, int elapsed = 0, bool deleteRestored = true)
	{
	#ifdef EXTRACE_DIAG
		auto trace = EXTrace.Start(EXTrace.GENERAL_ITEMS, ExpansionEntityStorageModule, basePath, "entity=" + ExpansionStatic.GetDebugInfo(entity, false), "parent=" + ExpansionStatic.GetDebugInfo(parent, false), "placeholder=" + ExpansionStatic.GetDebugInfo(placeholder, false), "player=" + ExpansionStatic.GetDebugInfo(player, false), "version=" + entityStorageVersion, "type=" + type, "level=" + level, "elapsed=" + elapsed, "deleteRestored=" + deleteRestored);
	#endif

		if (!level)
		{
			Reset();
			ResetState();

			if (!ctx.Read(entityStorageVersion))
				return ErrorFalse("Couldn't read entity storage version");

			s_LastRestoredVersion = entityStorageVersion;

			if (entityStorageVersion >= 5)
			{
				int timestamp;
				if (!ctx.Read(timestamp))
					return ErrorFalse("Couldn't read timestamp");

				elapsed = Now - timestamp;
			}
			else if (entityStorageVersion < 4)
			{
				return ErrorFalse("Outdated entity storage version " + entityStorageVersion);
			}
		}

		//! @note order of operations matters! DO NOT CHANGE!

		bool createEntity;
		//! https://feedback.bistudio.com/T173348
		if (level || !entity)
			createEntity = true;
		if (createEntity)
		{
			int result = Restore_Phase1(ctx, entity, parent, player, entityStorageVersion, type, level);
			if (result == FAILURE)
				return false;
			else if (result == SKIP)
				return true;

			if (!parent && entity.IsTransport() && dBodyIsSet(entity))
				dBodyActive(entity, ActiveState.INACTIVE);
		}

		if (!file)
			file = FileSerializer.Cast(ctx);

		bool restored;

		if (createEntity && entityStorageVersion == 10)
			restored = Restore_Phase3a(file, entity, entityStorageVersion);
		else
			restored = true;

		if (restored)
			restored = Restore_Phase2(file, basePath, entity, placeholder, player, entityStorageVersion, type, level, elapsed, deleteRestored);

		if (restored && createEntity)
		{
			if (entityStorageVersion != 10)
			{
				restored = Restore_Phase3a(file, entity, entityStorageVersion);
				if (restored)
					Restore_Phase3b(entity, elapsed);
			}
			else
			{
				Restore_Phase3b(entity, elapsed);
			}
		}

		if (!restored && createEntity)
		{
			if (!level && placeholder && entity.HasAnyCargo() && !MiscGameplayFunctions.Expansion_MoveCargo(entity, placeholder))
				EXError.Error(null, "Couldn't move cargo from " + entity + " back to placeholder " + placeholder);
			entity.Delete();
		}

		s_CurrentEntity = null;

		if (!level)
		{
			EXTrace.Print(EXTrace.GENERAL_ITEMS, ExpansionEntityStorageModule, "Restore " + s_CurrentRootEntity + " -> null");
			s_CurrentRootEntity = null;
		}

		return restored;
	}

	static int Restore_Phase1(ParamsReadContext ctx, out EntityAI entity, EntityAI parent, PlayerBase player = null, int entityStorageVersion = 0, string type = string.Empty, int level = 0)
	{
	#ifdef EXTRACE_DIAG
		auto trace = EXTrace.Start(EXTrace.GENERAL_ITEMS, ExpansionEntityStorageModule, "entity=" + ExpansionStatic.GetDebugInfo(entity, false), "parent=" + ExpansionStatic.GetDebugInfo(parent, false), "player=" + ExpansionStatic.GetDebugInfo(player, false), "version=" + entityStorageVersion, "type=" + type, "level=" + level);
	#endif

		if (entity)
		{
			EXError.Warn(null, "[EntityStorage] Passed in entity is not NULL! " + ExpansionStatic.GetHierarchyInfo(entity));
			entity = null;
		}

		//! 1a) entity type
		if (!level || !type)
		{
			if (!ctx.Read(type))
				return ErrorFalse("Couldn't read type");

			if (type == string.Empty)
				return SKIP;
		}

		//! 1b) location (creates entity)
		InventoryLocationType ilt;
		if (!ctx.Read(ilt))
			return ErrorFalse(type + ": Couldn't read inventory location type");
		if (!level && !parent && player)
			parent = player;
		EXTrace.Print(EXTrace.GENERAL_ITEMS, parent, "ExpansionEntityStorage::Restore_Phase1 " + type + " inventory location type " + typename.EnumToString(InventoryLocationType, ilt));
		InventoryLocation il;
		string errorMsg;
		switch (ilt)
		{
			case InventoryLocationType.GROUND:
				vector position;
				if (!ctx.Read(position))
					return ErrorFalse(type + ": Couldn't read position");
				vector orientation;
				if (!ctx.Read(orientation))
					return ErrorFalse(type + ": Couldn't read orientation");
				int flags = ECE_OBJECT_SWAP;
				if (GetGame().IsKindOf(type, "DZ_LightAI"))
					flags |= ECE_INITAI;
				if (Class.CastTo(entity, GetGame().CreateObjectEx(type, position, flags, RF_DEFAULT)))
				{
					entity.SetPosition(position);
					entity.SetOrientation(orientation);
				}
				else
				{
					errorMsg = "Couldn't create " + type + " on ground at " + position;
				}
				break;
			case InventoryLocationType.ATTACHMENT:
				int slotId;
				if (!ctx.Read(slotId))
					return ErrorFalse(type + ": Couldn't read slot ID");
				bool slotLocked;
				if (entityStorageVersion >= 9 && !ctx.Read(slotLocked))
					return ErrorFalse(type + ": Couldn't read slot locked state");
				il = new InventoryLocation();
				if (level)
				{
					//! Some 3rd party mods create default attachments in EEInit without checking slots, boo!
					//! If it's a different type, create restored entity elsewhere, else use existing attachment
					entity = parent.GetInventory().FindAttachment(slotId);
					if (entity)
					{
						if (entity.GetType() != type)
						{
							EXError.Warn(null, "[EntityStorage] Existing attachment " + entity.GetType() + " on " + parent + " is blocking entity to be restored " + type + ", will try to create in inventory");
							entity = null;
						}
						else
						{
							EXError.Warn(null, "[EntityStorage] Using existing attachment " + entity.GetType() + " on " + parent);
						}
					}
				}
				if (!entity)
				{
					il.SetAttachment(parent, null, slotId);
					entity = GameInventory.LocationCreateEntity(il, type, ECE_IN_INVENTORY, RF_DEFAULT);
				}
				if (entity)
					parent.GetInventory().SetSlotLock(slotId, slotLocked);
				else
					errorMsg = "Couldn't create " + type + " on " + parent + " in slot " + InventorySlots.GetSlotName(slotId);
				break;
			case InventoryLocationType.HANDS:
				il = new InventoryLocation();
				il.SetHands(parent, null);
				entity = GameInventory.LocationCreateEntity(il, type, ECE_IN_INVENTORY, RF_DEFAULT);
				if (!entity)
					errorMsg = "Couldn't create " + type + " in hands of " + parent;
				break;
			case InventoryLocationType.CARGO:
			case InventoryLocationType.PROXYCARGO:
				int idx, row, col;
				bool flip;
				if (!ctx.Read(idx))
					return ErrorFalse(type + ": Couldn't read cargo index");
				if (!ctx.Read(row))
					return ErrorFalse(type + ": Couldn't read cargo row");
				if (!ctx.Read(col))
					return ErrorFalse(type + ": Couldn't read cargo col");
				if (!ctx.Read(flip))
					return ErrorFalse(type + ": Couldn't read cargo flip");
				entity = parent.GetInventory().CreateEntityInCargoEx(type, idx, row, col, flip);  //! Only way to get flip correct
				if (!entity)
					errorMsg = "Couldn't create " + type + " in cargo of " + parent;
				break;
			default:
				return ErrorFalse(type + ": Unknown inventory location type " + typename.EnumToString(InventoryLocationType, ilt));
				break;
		}

		if (!entity && ilt != InventoryLocationType.GROUND)
		{
			string locInfo;
			if (!level && player)
			{
				//! Try to create in player inventory, falling back to owned container
				entity = ExpansionItemSpawnHelper.SpawnInInventorySecure(type, player, parent);
				//! Try to create on ground at player pos
				if (!entity && Class.CastTo(entity, GetGame().CreateObjectEx(type, player.GetPosition(), ECE_PLACE_ON_SURFACE, RF_DEFAULT)))
				{
					if (il)
						locInfo = " " + ExpansionStatic.DumpToString(il);
					EXError.Warn(null, "ExpansionEntityStorage::Restore_Phase1 - WARNING: Couldn't create " + type + " on " + parent + locInfo + ", created at player position " + player.GetPosition() + " instead");
				}
			}
			else if (parent)
			{
				//! Try to create at parent position
				if (Class.CastTo(entity, GetGame().CreateObjectEx(type, parent.GetPosition(), ECE_PLACE_ON_SURFACE, RF_DEFAULT)))
				{
					if (il)
						locInfo = " " + ExpansionStatic.DumpToString(il);
					EXError.Warn(null, "ExpansionEntityStorage::Restore_Phase1 - WARNING: Couldn't create " + type + " on " + parent + locInfo + ", created at parent position " + parent.GetPosition() + " instead");
				}
			}
		}

		if (!entity)
		{
			EXError.Error(null, errorMsg);
			return false;
		}

		s_CurrentEntity = entity;

		if (!level)
		{
			EXTrace.Print(EXTrace.GENERAL_ITEMS, ExpansionEntityStorageModule, "Restore_Phase1 " + s_CurrentRootEntity + " -> " + entity);
			s_CurrentRootEntity = entity;
		}

		return SUCCESS;
	}

	static bool Restore_Phase2(ParamsReadContext ctx, string basePath, EntityAI entity, EntityAI placeholder = null, PlayerBase player = null, int entityStorageVersion = 0, string type = string.Empty, int level = 0, int elapsed = 0, bool deleteRestored = true)
	{
	#ifdef EXTRACE_DIAG
		auto trace = EXTrace.Start(EXTrace.GENERAL_ITEMS, ExpansionEntityStorageModule, basePath, "entity=" + ExpansionStatic.GetDebugInfo(entity, false), "placeholder=" + ExpansionStatic.GetDebugInfo(placeholder, false), "player=" + ExpansionStatic.GetDebugInfo(player, false), "version=" + entityStorageVersion, "type=" + type, "level=" + level, "elapsed=" + elapsed, "deleteRestored=" + deleteRestored);
	#endif

		//! 2) attachments + cargo
		if (!level && placeholder && placeholder.HasAnyCargo() && !MiscGameplayFunctions.Expansion_MoveCargo(placeholder, entity))
			EXError.Error(null, "Couldn't move cargo from placeholder " + placeholder + " to " + entity);
		int inventoryCount;
		if (!ctx.Read(inventoryCount))
			return ErrorFalse(entity.GetType() + ": Couldn't read inventory count");

		if (!inventoryCount)
			return true;

		string childFileName;
		ExpansionEntityStorageContext context;
		FileSerializer file;

		int restored;

		EntityAI child;
		for (int i = 0; i < inventoryCount; i++)
		{
			if (entityStorageVersion >= 7)
			{
				if (!ctx.Read(type))
					return ErrorFalse("Couldn't read type");

				if (type == string.Empty)
					continue;

				if (!s_SubContexts.Find(type, context))
				{
					childFileName = string.Format("%1\\%2%3", basePath, type, EXT);
					context = new ExpansionEntityStorageContext();
					if (!context.Open(childFileName, FileMode.READ))
						return ErrorFalse("Couldn't open file for reading " + childFileName);
					s_SubContexts[type] = context;
				}

				file = context.m_Context;
			}

			child = null;
			if (!Restore(ctx, file, basePath, child, entity, null, player, entityStorageVersion, type, level + 1, elapsed))
			{
				if (entityStorageVersion < 7)
					return false;
				else
					context.m_FailedCount++;
			}
			else
			{
				restored++;
			}
		}

		if (entityStorageVersion >= 7 && !level)
		{
			if (Reset(deleteRestored))
				DeleteFile(basePath);
		}

		return restored > 0;
	}

	static bool Restore_Phase3a(ParamsReadContext ctx, EntityAI entity, int entityStorageVersion)
	{
	#ifdef EXTRACE_DIAG
		auto trace = EXTrace.Start(EXTrace.GENERAL_ITEMS, ExpansionEntityStorageModule, "entity=" + ExpansionStatic.GetDebugInfo(entity, false), "version=" + entityStorageVersion);
	#endif

		int i;

		s_CurrentEntity = entity;

		//! 3) special treatment for weapons
		Weapon_Base weapon;
		if (Class.CastTo(weapon, entity))
		{
			int muzzleCount;
			if (!ctx.Read(muzzleCount))
				return ErrorFalse(entity.GetType() + ": Couldn't read muzzle count");

		#ifdef DIAG_DEVELOPER
			EXTrace.Print(EXTrace.GENERAL_ITEMS, null, "Muzzle count: " + muzzleCount);
		#endif

			float ammoDamage;
			string ammoTypeName;

			for (int mi = 0; mi < muzzleCount; ++mi)
			{
				bool isChamberEmpty;
				if (!ctx.Read(isChamberEmpty))
					return ErrorFalse(entity.GetType() + ": Couldn't read chamber empty");
				if (!isChamberEmpty)
				{
					if (!ctx.Read(ammoDamage))
						return ErrorFalse(entity.GetType() + ": Couldn't read ammo damage");
					if (!ctx.Read(ammoTypeName))
						return ErrorFalse(entity.GetType() + ": Couldn't read ammo type");
					weapon.PushCartridgeToChamber(mi, ammoDamage, ammoTypeName);
				}

				int internalMagCartridgeCount;
				if (!ctx.Read(internalMagCartridgeCount))
					return ErrorFalse(entity.GetType() + ": Couldn't read internal mag cartridge count");
			#ifdef DIAG_DEVELOPER
				EXTrace.Print(EXTrace.GENERAL_ITEMS, null, "Internal magazine cartridge count: " + internalMagCartridgeCount);
			#endif
				for (int ci = 0; ci < internalMagCartridgeCount; ++ci)
				{
					if (!ctx.Read(ammoDamage))
						return ErrorFalse(entity.GetType() + ": Couldn't read ammo damage");
					if (!ctx.Read(ammoTypeName))
						return ErrorFalse(entity.GetType() + ": Couldn't read ammo type");
					weapon.PushCartridgeToInternalMagazine(mi, ammoDamage, ammoTypeName);
				}
			}
		}

		//! 4) storeload
		int version;
		if (!ctx.Read(version))
			return ErrorFalse(entity.GetType() + ": Couldn't read game version");
	#ifdef DIAG_DEVELOPER
		EXTrace.Print(EXTrace.GENERAL_ITEMS, null, "Storage version: " + version);
	#endif
		if (!entity.OnStoreLoad(ctx, version))
			return ErrorFalse(entity.GetType() + ": Couldn't OnStoreLoad");

	#ifdef DIAG_DEVELOPER
		EXTrace.Print(EXTrace.GENERAL_ITEMS, null, "Entity OnStoreLoad SUCCESS");
	#endif

		if (weapon)
			weapon.Expansion_ResetMuzzleModes();

		DayZPlayerImplement player;
		if (version >= 12 && Class.CastTo(player, entity))
		{
			int entityStorageAdditionalDataVersion;
			if (!ctx.Read(entityStorageAdditionalDataVersion))
				return ErrorFalse(entity.GetType() + ": Couldn't read additional data version");
		#ifdef DIAG_DEVELOPER
			EXTrace.Print(EXTrace.GENERAL_ITEMS, null, "Additional data version: " + entityStorageAdditionalDataVersion);
		#endif

			if (!player.Expansion_OnEntityStorageAdditionalDataLoad(ctx, entityStorageAdditionalDataVersion))
				return ErrorFalse(entity.GetType() + ": Couldn't read additional data");
		}

		//! 5) special treatment for mags/ammo
		Magazine mag;
		int ammoCount;
		if (Class.CastTo(mag, entity))
		{
			if (mag.IsAmmoPile() || entityStorageVersion < 8)
			{
				if (!ctx.Read(ammoCount))
					return ErrorFalse(entity.GetType() + ": Couldn't read ammo count");
			#ifdef DIAG_DEVELOPER
				EXTrace.Print(EXTrace.GENERAL_ITEMS, null, "Ammo count: " + ammoCount);
			#endif
				mag.ServerSetAmmoCount(ammoCount);
			}
			else
			{
				int count;
				if (!ctx.Read(count))
					return ErrorFalse(entity.GetType() + ": Couldn't read cartridge count");
			#ifdef DIAG_DEVELOPER
				EXTrace.Print(EXTrace.GENERAL_ITEMS, null, "Cartridge count: " + count);
			#endif
				mag.ServerSetAmmoCount(0);
				auto cartridges = new array<ref ExpansionCartridgeInfo>;
				for (i = 0; i < count; i++)
				{
					auto info = new ExpansionCartridgeInfo;
					if (!info.OnLoad(ctx))
						return ErrorFalse(entity.GetType() + ": Couldn't read cartridge info");
					cartridges.Insert(info);
				}
				foreach (auto cartridge: cartridges)
				{
					for (i = 0; i < cartridge.Count; i++)
					{
						mag.ServerStoreCartridge(cartridge.Damage, cartridge.Type);
					}
				}
			}
		}

		//! 6) Special treatment for vehicles (no special treatment needed for ExpansionVehicleBase since it inherits from ItemBase)
		ExpansionVehicle vehicle;
		if (!entity.IsItemBase() && ExpansionVehicle.Get(vehicle, entity))
		{
			if (!vehicle.ReadFluidsFrom(ctx, entityStorageVersion))
				return false;
		}

		//! 7) global health and damage zones
		float globalHealth;
		if (!ctx.Read(globalHealth))
			return ErrorFalse(entity.GetType() + ": Couldn't read global health");
		entity.SetHealth(globalHealth);
		//TStringArray dmgZones = new TStringArray();
		//entity.GetDamageZones(dmgZones);
		int dmgZoneCount;
		if (!ctx.Read(dmgZoneCount))
			return ErrorFalse(entity.GetType() + ": Couldn't read damage zone count");
	#ifdef DIAG_DEVELOPER
		EXTrace.Print(EXTrace.GENERAL_ITEMS, null, "Damage zone count: " + dmgZoneCount);
	#endif
		for (i = 0; i < dmgZoneCount; i++)
		{
			string dmgZone;
			if (!ctx.Read(dmgZone))
				return ErrorFalse(entity.GetType() + ": Couldn't read damage zone");
			float dmgZoneHealth;
			if (!ctx.Read(dmgZoneHealth))
				return ErrorFalse(entity.GetType() + ": Couldn't read damage zone health");
			//if (dmgZones.Find(dmgZone) > -1)
			entity.SetHealth(dmgZone, "Health", dmgZoneHealth);
			//else
				//EXPrint("[EntityStorage] " + entity.GetType() + ": Couldn't find damage zone \"" + dmgZone + "\"");
		}

		return true;
	}

	static void Restore_Phase3b(EntityAI entity, int elapsed)
	{
	#ifdef EXTRACE_DIAG
		auto trace = EXTrace.Start(EXTrace.GENERAL_ITEMS, ExpansionEntityStorageModule, "entity=" + ExpansionStatic.GetDebugInfo(entity, false), "elapsed=" + elapsed);
	#endif

		//! 8) Process wetness/temperature/decay
		ItemBase item;
		if (elapsed > 0 && Class.CastTo(item, entity))
			item.Expansion_ProcessWTD(elapsed);

		//! FINAL
		entity.AfterStoreLoad();
		entity.SetSynchDirty();
		Weapon_Base weapon;
		if (Class.CastTo(weapon, entity))
			weapon.Synchronize();
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Call(entity.EEOnAfterLoad);  //! Make sure EEOnAfterLoad gets called AFTER whole hierarchy has loaded
	}

	//! Legacy
	static string GetFileName(int id)
	{
		return GetStorageDirectory() + id.ToString() + EXT;
	}

	static string GetFileName(string name)
	{
		return GetStorageDirectory() + name + EXT;
	}

	static string GetStorageDirectory()
	{
		int instance_id = GetGame().ServerConfigGetInt("instanceId");
		return "$mission:storage_" + instance_id + "\\expansion\\entitystorage\\";
	}

	static void DeleteSavedEntity(EntityAI entity)
	{
		s_SavedEntityToBeDeleted = entity;

		GetGame().ObjectDelete(entity);

		s_SavedEntityToBeDeleted = null;
	}

	static EntityAI GetSavedEntityToBeDeleted()
	{
		return s_SavedEntityToBeDeleted;
	}

	/** @brief get current entity during entity storage save/restore operations
	 * 
	 * @note use only during Save/Restore! Will be nulled afterwards
	 */
	static EntityAI GetCurrentEntity()
	{
		return s_CurrentEntity;
	}

	static bool IsCurrentEntity(EntityAI entity)
	{
		if (s_CurrentEntity == entity)
			return true;

		return false;
	}

	/** @brief get current root entity during entity storage save/restore operations
	 * 
	 * @note use only during Save/Restore! Will be nulled afterwards
	 */
	static EntityAI GetCurrentRootEntity()
	{
		return s_CurrentRootEntity;
	}

	static bool IsCurrentRootEntity(EntityAI entity)
	{
		if (s_CurrentRootEntity == entity)
			return true;

		return false;
	}

	static bool IsInCurrentHierarchy(EntityAI entity)
	{
		while (entity)
		{
			if (entity == s_CurrentRootEntity)
				return true;

			entity = entity.GetHierarchyParent();
		}

		return false;
	}

	/** @brief get last restored entity storage version
	 */
	static int GetRestoredVersion()
	{
		return s_LastRestoredVersion;
	}

	static bool DeleteFiles(string name)
	{
		string folderName = GetStorageDirectory() + name;
		string fileName = folderName + EXT;

		if (FileExist(fileName))
		{
			if (!DeleteFile(fileName))
				EXError.Error(null, "[EntityStorage] couldn't delete " + fileName);

			if (FileExist(folderName))
			{
				TStringArray files = ExpansionStatic.FindFilesInLocation(folderName + "\\", EXT);

				foreach (string baseName: files)
				{
					if (!DeleteFile(folderName + "\\" + baseName))
						EXError.Error(null, "[EntityStorage] couldn't delete " + folderName + "\\" + baseName);
				}

				if (!DeleteFile(folderName))
					EXError.Error(null, "[EntityStorage] couldn't delete " + folderName);
				else
					return true;
			}
		}

		return false;
	}

	//! @brief saves entity and all its children (attachments/cargo) to file.
	static bool SaveToFile(EntityAI entity, string fileName, bool inventoryOnly = false, EntityAI placeholder = null, bool removeOrphaned = false)
	{
		FileSerializer file = new FileSerializer();

		if (!file.Open(fileName, FileMode.WRITE))
			return ErrorFalse("Couldn't open file for writing " + fileName);

		Now = CF_Date.Now(true).DateToEpoch();

		auto hitch = new EXHitch("[ExpansionEntityStorage] ");

		string basePath;
		TStringArray orphanedFiles;
		ExpansionString exFileName = fileName;
		if (exFileName.EndsWith(EXT))
		{
			basePath = fileName.Substring(0, fileName.Length() - EXT.Length());
			if (removeOrphaned)
				orphanedFiles = ExpansionStatic.FindFilesInLocation(basePath + "\\", EXT);
		}

		bool success = Save(entity, file, basePath, inventoryOnly, placeholder, 0, orphanedFiles);

		file.Close();

		return success;
	}

	//! @brief restores entity and all its children from file (creates entities). Deletes file on success.
	//! @note if you pass in an existing entity, only its children (inventory) will be restored.
	static bool RestoreFromFile(string fileName, inout EntityAI entity = null, EntityAI placeholder = null, PlayerBase player = null, bool deleteRestored = true)
	{
	#ifdef EXTRACE_DIAG
		auto trace = EXTrace.Start(EXTrace.GENERAL_ITEMS, ExpansionEntityStorageModule, fileName, "entity=" + ExpansionStatic.GetDebugInfo(entity, false), "placeholder=" + ExpansionStatic.GetDebugInfo(placeholder, false), "player=" + ExpansionStatic.GetDebugInfo(player, false), "deleteRestored=" + deleteRestored);
	#endif

		FileSerializer file = new FileSerializer();

		if (!file.Open(fileName, FileMode.READ))
			return ErrorFalse("Couldn't open file for reading " + fileName);

		GameInventory inventory;
		bool isInventoryLocked;
		if (entity)
		{
			inventory = entity.GetInventory();
			if (inventory && inventory.IsInventoryLockedForLockType(HIDE_INV_FROM_SCRIPT))
			{
				isInventoryLocked = true;
				inventory.UnlockInventory(HIDE_INV_FROM_SCRIPT);
			}
		}

		Now = CF_Date.Now(true).DateToEpoch();

		auto hitch = new EXHitch("[ExpansionEntityStorage] ");

		string basePath;
		ExpansionString exFileName = fileName;
		if (exFileName.EndsWith(EXT))
			basePath = fileName.Substring(0, fileName.Length() - EXT.Length());

		bool success = Restore(file, null, basePath, entity, entity, placeholder, player, 0, string.Empty, 0, 0, deleteRestored);

		if (isInventoryLocked)
			inventory.LockInventory(HIDE_INV_FROM_SCRIPT);

		file.Close();

		if (!success)
			return false;

		if (deleteRestored)
			DeleteFile(fileName);

		return true;
	}

	//! @brief saves entity and all its children (attachments/cargo) and replaces original entity with placeholder. Deletes original entity on success!
	//! @note if storeCargo is false (default), move cargo to placeholder, else save cargo to virtual storage
	static bool SaveToFileAndReplace(EntityAI entity, string fileName, string placeholderType, vector position, int iFlags = ECE_OBJECT_SWAP, out EntityAI placeholder = null, bool storeCargo = false, array<EntityAI> transferAttachments = null)
	{
		Object placeholderObject = GetGame().CreateObjectEx(placeholderType, position, iFlags);

		if (!Class.CastTo(placeholder, placeholderObject))
		{
			EXError.Error(entity, "ExpansionEntityStorageModule::SaveToFileAndReplace - Couldn't cast to EntityAI " + placeholderObject);
			GetGame().ObjectDelete(placeholderObject);
			return false;
		}

		GameInventory inventory = entity.GetInventory();
		bool isInventoryLocked;
		if (inventory && inventory.IsInventoryLockedForLockType(HIDE_INV_FROM_SCRIPT))
		{
			isInventoryLocked = true;
			inventory.UnlockInventory(HIDE_INV_FROM_SCRIPT);
		}

		InventoryLocation transferAttachmentInvLoc;
		if (transferAttachments)
		{
			transferAttachmentInvLoc = new InventoryLocation();
			foreach (EntityAI transferAttachment: transferAttachments)
			{
				transferAttachment.GetInventory().GetCurrentInventoryLocation(transferAttachmentInvLoc);
				placeholder.ServerTakeEntityAsAttachmentEx(transferAttachment, transferAttachmentInvLoc.GetSlot());
			}
		}

		bool success;

		if (storeCargo)
			success = SaveToFile(entity, fileName);
		else
			success = SaveToFile(entity, fileName, false, placeholder);

		if (!success)
		{
			//! Need to undo...
			if (transferAttachments)
			{
				foreach (EntityAI undoTransferAttachment: transferAttachments)
				{
					undoTransferAttachment.GetInventory().GetCurrentInventoryLocation(transferAttachmentInvLoc);
					entity.ServerTakeEntityAsAttachmentEx(undoTransferAttachment, transferAttachmentInvLoc.GetSlot());
				}
			}

			if (isInventoryLocked)
				inventory.LockInventory(HIDE_INV_FROM_SCRIPT);
			if (placeholder)
				GetGame().ObjectDelete(placeholder);

			return false;
		}

		vector orientation = entity.GetOrientation();

		if ((iFlags & ECE_KEEPHEIGHT) == 0)
		{
			EXTrace.Print(EXTrace.GENERAL_ITEMS, entity, "ExpansionEntityStorageModule::SaveToFileAndReplace - position " + position);

			vector surface = ExpansionStatic.GetSurfaceWaterPosition(position);
			EXTrace.Print(EXTrace.GENERAL_ITEMS, null, "ExpansionEntityStorageModule::SaveToFileAndReplace - surface position " + surface);

			PhxInteractionLayers layerMask;
			layerMask |= PhxInteractionLayers.BUILDING;
			layerMask |= PhxInteractionLayers.DOOR;
			layerMask |= PhxInteractionLayers.VEHICLE;
			layerMask |= PhxInteractionLayers.ROADWAY;
			layerMask |= PhxInteractionLayers.TERRAIN;
			layerMask |= PhxInteractionLayers.ITEM_LARGE;
			layerMask |= PhxInteractionLayers.FENCE;
			vector hitPosition;
			if (DayZPhysics.RayCastBullet(position, surface, layerMask, entity, null, hitPosition, null, null))
			{
				EXTrace.Print(EXTrace.GENERAL_ITEMS, entity, "ExpansionEntityStorageModule::SaveToFileAndReplace - hit position " + hitPosition);
				position = hitPosition;
			}
			else
			{
				position = surface;
			}
		}

		DeleteSavedEntity(entity);

		if (placeholder)
		{
			placeholder.SetPosition(position);
			placeholder.SetOrientation(orientation);
		}

		return true;
	}

	static bool Reset(bool deleteRestored = false, TStringArray orphanedFiles = null)
	{
		int failedCount;

		foreach (string type, ExpansionEntityStorageContext context: s_SubContexts)
		{
			context.Close();
			failedCount += context.m_FailedCount;
			if (deleteRestored && !context.m_FailedCount)
				DeleteFile(context.m_FileName);

			if (orphanedFiles)
			{
				int index = orphanedFiles.Find(type + EXT);
				if (index > -1)
					orphanedFiles.Remove(index);
			}
		}

		s_SubContexts.Clear();

		return !deleteRestored || failedCount == 0;
	}

	static void ResetState()
	{
		s_LastRestoredVersion = 0;
		EXTrace.Print(EXTrace.GENERAL_ITEMS, ExpansionEntityStorageModule, "ResetState " + s_CurrentRootEntity + " -> null");
		s_CurrentEntity = null;
		s_CurrentRootEntity = null;
	}
}
