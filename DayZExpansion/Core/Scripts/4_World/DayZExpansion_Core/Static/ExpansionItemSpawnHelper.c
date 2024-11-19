/**
 * ExpansionItemSpawnHelper.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

class ExpansionItemSpawnHelper
{
	static ref map<string, vector> s_VehicleSizes = new map<string, vector>;

	// ------------------------------------------------------------
	// Expansion Object SpawnOnParent
	// ------------------------------------------------------------
	static Object SpawnOnParent(string className, PlayerBase player, inout EntityAI parent, out int remainingAmount, int quantityPercent = -1, TStringArray attachments = NULL, int skinIndex = -1, bool magFillAmmo = true, inout bool attachmentNotAttached = false)
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.GLOBAL, "ExpansionItemSpawnHelper", "SpawnOnParent");
#endif

		if (quantityPercent < 0)
			quantityPercent = 100;

		//! Try to spawn in inventory first
		Object obj = SpawnInInventorySecure(className, player, parent, skinIndex, true);

		ItemBase item;
		if (!Class.CastTo(item, obj))
		{
			//! NULL or not an item

			remainingAmount--;

			if (obj)
			{
				//! Not an item, but was spawned in inventory. Get rid of it.
				//! TODO: This is just here for historical reasons, do we really need it?

				GetGame().ObjectDelete(obj);

				return NULL;
			}

			//! Spawn on ground
			obj = GetGame().CreateObject(className, parent.GetPosition(), false, GetGame().IsKindOf(className, "DZ_LightAI"));
			if (!Class.CastTo(item, obj))
			{
				if (obj)
				{
					return obj;
				}

				return NULL;
			}

			if (skinIndex > -1 && item.ExpansionHasSkin(skinIndex))
				item.ExpansionSetSkin(skinIndex);
		}
		else if (remainingAmount == -1)
		{
			//! Ignore remaining amount
		}
		else if (item.IsAmmoPile())
		{
			//! Ammo/Bullets
			Ammunition_Base ammo = Ammunition_Base.Cast(item);
			int maxAmmo = ammo.GetAmmoMax();

			if (remainingAmount < maxAmmo)
			{
				ammo.ServerSetAmmoCount(remainingAmount);
				remainingAmount = 0;
			}
			else if (!maxAmmo)
			{
				Error("Error: " + className + " max ammo count is zero! Faulty config.cpp");
				remainingAmount = 0;
			}
			else
			{
				ammo.ServerSetAmmoCount(maxAmmo);
				remainingAmount -= maxAmmo;
			}
		}
		else if (item.IsMagazine())
		{
			//! Magazines
			Magazine mag = Magazine.Cast(item);

			remainingAmount--;

			if (magFillAmmo)
				mag.ServerSetAmmoCount(Math.Round(mag.GetAmmoMax() * quantityPercent / 100));
			else
				mag.ServerSetAmmoCount(0);
		}
		else if (!item.IsInherited(Edible_Base) && item.Expansion_IsStackable())
		{
			//! Stackable items
			int max = item.GetQuantityMax();

			if (remainingAmount < max)
			{
				item.SetQuantity(remainingAmount);
				remainingAmount = 0;
			}
			else if (!max)
			{
				Error("Error: " + className + " max quantity/stack is zero! Faulty config.cpp");
				remainingAmount = 0;
			}
			else
			{
				item.SetQuantity(max);
				remainingAmount -= max;
			}
		}
		else
		{
			//! Everything else

			remainingAmount--;

			if (item.HasQuantity())
			{
				float itemQuantity = item.GetQuantityMax() * quantityPercent / 100;
				if (item.Expansion_IsStackable())
					itemQuantity = Math.Round(itemQuantity);
				item.SetQuantity(itemQuantity);

				if (item.GetCompEM())
					item.GetCompEM().SetEnergy0To1(quantityPercent / 100);
			}
		}
		
		if (item.CanBeDisinfected())
			item.SetCleanness(1);

		if (attachments)
			SpawnAttachments( attachments, player, item, quantityPercent, skinIndex, magFillAmmo, attachmentNotAttached );

		return obj;
	}
	
	//! Spawn in parent inventory, use player-owned temporary storage container if inventory full
	static EntityAI SpawnInInventorySecure(string className, notnull PlayerBase player, inout EntityAI parent, int skinIndex = -1, bool forceCheckCanReceiveItem = false)
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.GLOBAL, "ExpansionItemSpawnHelper", "SpawnInInventorySecure");
#endif

		bool canSpawnInInventory = ExpansionStatic.IsInventoryItem(className);

		if (!canSpawnInInventory)
		{
			Error("Not an inventory item: " + className);
			return NULL;
		}

		EntityAI entity;
		ExpansionTemporaryOwnedContainer newStorage;
		ExpansionTemporaryOwnedContainer existingPlayerStorage = player.Expansion_GetTemporaryOwnedContainer();
		ExpansionTemporaryOwnedContainer playerStorage = existingPlayerStorage;
		ExpansionTemporaryOwnedContainer storage;

		/**
		 * To spawn in inventory, we try in order:
		 * 
		 * 1. In parent inventory
		 * 2. In player's existing temporary container, if any
		 * 3. In new temporary container
		 */
		while (true)
		{
			if (Class.CastTo(storage, parent))
				storage.ExpansionSetCanReceiveItems(true);

			entity = SpawnInInventory(className, parent, skinIndex, false, forceCheckCanReceiveItem);

			if (storage)
				storage.ExpansionSetCanReceiveItems(false);

			//! If entity couldn't be spawned but we have a new temporary container, sth definitely is wrong and we need to abort
			if (entity || newStorage)
				break;

			//! If it's an inventory item and couldn't be spawned in parent inventory (likely because it's full),
			//! use existing player temporary container or create new temporary storage container and set as parent
			if (playerStorage)
			{
				parent = playerStorage;
				playerStorage = null;  //! null it so next iteration of the while loop we create a new one
			}
			else
			{
				if (!Class.CastTo(newStorage, GetGame().CreateObjectEx("ExpansionTemporaryOwnedContainer", player.GetPosition(), ECE_PLACE_ON_SURFACE)))
				{
					Error("Failed to create temporary storage container!");

					return NULL;
				}

				player.Expansion_SetTemporaryOwnedContainer(newStorage);

				parent = newStorage;
				playerStorage = newStorage;
			}
		}

		if (storage && !existingPlayerStorage)
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Call(TemporaryStorageNotification, player.GetIdentity());

		return entity;
	}

	static void TemporaryStorageNotification(notnull PlayerIdentity identity)
	{
		ExpansionNotification("STR_EXPANSION_TEMPORARY_STORAGE", "STR_EXPANSION_TEMPORARY_STORAGE_INFO", EXPANSION_NOTIFICATION_ICON_INFO, COLOR_EXPANSION_NOTIFICATION_SUCCESS, 6, ExpansionNotificationType.TOAST).Create(identity);
	}

	// ------------------------------------------------------------
	// Expansion SpawnAttachments
	// ------------------------------------------------------------
	//! Use this form when spawning locally (e.g. only client-side)
	static void SpawnAttachments(TStringArray attachments, EntityAI parent, int skinIndex = -1)
	{
		foreach (string attachmentName: attachments)
		{
			SpawnInInventory(attachmentName, parent, skinIndex);
		}
	}

	//! Use this form when spawning normally (makes sure items get spawned when they don't fit in inventory)
	static void SpawnAttachments(TStringArray attachments, PlayerBase player, EntityAI parent, int quantityPercent = -1, int skinIndex = -1, bool magFillAmmo = true, inout bool attachmentNotAttached = false)
	{
		int remainingAmount = -1;
		foreach (string attachmentName: attachments)
		{
			EntityAI attachmentParent = parent;
			SpawnOnParent(attachmentName, player, attachmentParent, remainingAmount, quantityPercent, NULL, skinIndex, magFillAmmo);
			if (attachmentParent != parent)
				attachmentNotAttached = true;
		}
	}
	
	// ------------------------------------------------------------
	// Expansion EntityAI SpawnInInventory
	// ------------------------------------------------------------
	static EntityAI SpawnInInventory(string name, EntityAI parent, int skinIndex = -1, bool attachOnly = false, bool forceCheckCanReceiveItem = false)
	{
		EntityAI entity;

		if (parent.GetInventory())
		{
			ItemBase parentItem = ItemBase.Cast(parent);
			
			if (parentItem)
				entity = parentItem.ExpansionCreateInInventory(name, attachOnly);
			else if (attachOnly)
				entity = parent.GetInventory().CreateAttachment(name);
			else
				entity = CreateInInventoryEx(parent, name, forceCheckCanReceiveItem);

			if (entity)
			{
				ItemBase item = ItemBase.Cast(entity);

				if (item && skinIndex > -1 && item.ExpansionHasSkin(skinIndex))
					item.ExpansionSetSkin(skinIndex);
			}
		}

		return entity;
	}

	static EntityAI CreateInInventoryEx(EntityAI parent, string type, bool forceCheckCanReceiveItem = false)
	{
		EntityAI newEntity;

		//! First, we try to spawn item in parent inventory directly.
		//! This can fail if parent is player even if player has clothing that still has enough space, so, ...
		if (forceCheckCanReceiveItem)
			newEntity = CreateAndMoveToInventory(parent, type, true);
		else
			newEntity = parent.GetInventory().CreateInInventory(type);

		if (!newEntity)
		{
			//! ...second, we check if the item would fit in cargo by creating it locally
			//! (unlike CreateInInventory/FindFirstFreeLocationForNewEntity, this deals with rotating the item if it doesn't fit otherwise)
			//! @note since this uses CreateAndMoveToInventory, we only need to do this if forceCheckCanReceiveItem is false,
			//! else it will already have been done above
			if (!forceCheckCanReceiveItem)
			{
				CargoBase cargo = parent.GetInventory().GetCargo();
				if (cargo)
				{
					TIntArray itemSize = {};

					string path;
					if (GetGame().ConfigIsExisting(CFG_MAGAZINESPATH + " " + type))
						path = CFG_MAGAZINESPATH;
					else if (GetGame().ConfigIsExisting(CFG_WEAPONSPATH + " " + type))
						path = CFG_WEAPONSPATH;
					else if (GetGame().ConfigIsExisting(CFG_VEHICLESPATH + " " + type))
						path = CFG_VEHICLESPATH;
					else
						return null;

					GetGame().ConfigGetIntArray(path + " "  + type + " itemSize", itemSize);

					int cargoMax = Math.Max(cargo.GetWidth(), cargo.GetHeight());
					int cargoMin = Math.Min(cargo.GetWidth(), cargo.GetHeight());
					int itemMax = Math.Max(itemSize[0], itemSize[1]);
					int itemMin = Math.Min(itemSize[0], itemSize[1]);

					//! We don't even attempt to create the item if it wouldn't fit anyway due to size
					if (cargoMax >= itemMax && cargoMin >= itemMin)
					{
						newEntity = CreateAndMoveToInventory(parent, type, forceCheckCanReceiveItem);
						if (newEntity)
							return newEntity;
					}
				}
			}

			//! ...third, if all else failed, we look through all attachments manually to see if item can be created.
			//! We deliberately ignore items in cargo even if they could receive items because having to dig through
			//! inventories of items in clothing to find (e.g.) sth you have just purchased at a trader is annoying for players.
			for (int i = 0; i < parent.GetInventory().AttachmentCount(); i++)
			{
				EntityAI att = parent.GetInventory().GetAttachmentFromIndex(i);
				newEntity = CreateInInventoryEx(att, type, forceCheckCanReceiveItem);
				if (newEntity)
				{
				#ifdef DIAG_DEVELOPER
					EXTrace.Print(EXTrace.GENERAL_ITEMS, ExpansionItemSpawnHelper, "::CreateInInventoryEx - created " + newEntity + " on " + parent);
				#endif
					break;
				}
			}
		}

		return newEntity;
	}

	//! Unlike GameInventory::CreateInInventory, this deals with rotating the item if it doesn't fit otherwise
	//! and checks if parent can actually receive the created item
	static EntityAI CreateAndMoveToInventory(EntityAI parent, string type, bool forceCheckCanReceiveItem = true)
	{
		EntityAI newEntity;

		Object obj = GetGame().CreateObjectEx(type, "0 0 0", ECE_LOCAL);
		if (Class.CastTo(newEntity, obj))
		{
			auto src = new InventoryLocation();
			auto dst = new InventoryLocation();

			if (newEntity.GetInventory().GetCurrentInventoryLocation(src) && parent.GetInventory().FindFreeLocationFor(newEntity, FindInventoryLocationType.ATTACHMENT | FindInventoryLocationType.CARGO | FindInventoryLocationType.HANDS, dst))
			{
				//! @note GameInventory::TakeToDst performs the below checks for atts/cargo but we need to deal with hands
				bool canTakeToDst = !forceCheckCanReceiveItem;

				if (forceCheckCanReceiveItem)
				{
					switch (dst.GetType())
					{
						case InventoryLocationType.ATTACHMENT:
							//! TakeToDst already deals with this, but for good measure
							canTakeToDst = parent.CanReceiveAttachment(newEntity, dst.GetSlot());
							break;

						case InventoryLocationType.CARGO:
							//! TakeToDst already deals with this, but for good measure
							canTakeToDst = parent.CanReceiveItemIntoCargo(newEntity);
							break;

						case InventoryLocationType.HANDS:
							//! Special snowflake: Hands, since it redirects to hand event (async!)
							//! TakeToDst can NOT check this when creating several items in the same server tick!
							PlayerBase player;
							if (Class.CastTo(player, parent) && player.Expansion_PrepareTakeEntityToHands(newEntity))
								canTakeToDst = true;
							break;
					}
				}

				//! Special snowflake: Hands. Has to use ServerTakeToDst, not LocalTakeDst, else desync. WHY game, WHY.
				//! Just use ServerTakeToDst always.
				if (canTakeToDst)
				{
				#ifdef SERVER
					//! This has to be called before ServerTakeToDst else weird things can happen with lifetime
					GetGame().RemoteObjectTreeCreate(newEntity);
				#endif

					if (parent.ServerTakeToDst(src, dst))
					{
					#ifdef DIAG_DEVELOPER
						EXTrace.Print(EXTrace.GENERAL_ITEMS, ExpansionItemSpawnHelper, "::CreateAndMoveToInventory - created & moved " + newEntity + " to " + parent);
					#endif

						return newEntity;
					}
				}
			}

			GetGame().ObjectDelete(obj);
		}
		else if (obj)
		{
			Error(obj.ToString() + " is not EntityAI");
			GetGame().ObjectDelete(obj);
		}

		return null;
	}

	static EntityAI SpawnAttachment(string name, EntityAI parent, int skinIndex = -1)
	{
		return SpawnInInventory(name, parent, skinIndex, true);
	}

	static bool IsSpawnPositionFree(vector position, vector orientation, string className, out Object blockingObject = NULL)
	{
#ifdef EXPANSIONTRACE
		auto trace = EXTrace.Start(ExpansionTracing.GENERAL_ITEMS, ExpansionItemSpawnHelper);
#endif

		//className.ToLower();

		vector size;
		if (!s_VehicleSizes.Find(className, size))
		{
			vector minMax[2];
			if (ExpansionStatic.GetCollisionBox(className, minMax))
			{
				size = Vector(minMax[1][0] - minMax[0][0], minMax[1][1] - minMax[0][1], minMax[1][2] - minMax[0][2]);

				#ifdef DIAG_DEVELOPER
				Object debugBox;

				//! Bottom left
				debugBox = GetGame().CreateObjectEx("ExpansionDebugBox_Blue", position, ECE_NOLIFETIME);
				debugBox.SetOrientation(orientation);
				debugBox.SetPosition(debugBox.ModelToWorld(Vector(-size[0] / 2, 0, -size[2] / 2)));

				//! Bottom right
				debugBox = GetGame().CreateObjectEx("ExpansionDebugBox_Orange", position, ECE_NOLIFETIME);
				debugBox.SetOrientation(orientation);
				debugBox.SetPosition(debugBox.ModelToWorld(Vector(size[0] / 2, 0, -size[2] / 2)));

				//! Top right
				debugBox = GetGame().CreateObjectEx("ExpansionDebugBox_Red", position, ECE_NOLIFETIME);
				debugBox.SetOrientation(orientation);
				debugBox.SetPosition(debugBox.ModelToWorld(Vector(size[0] / 2, size[1], size[2] / 2)));

				//! Top left
				debugBox = GetGame().CreateObjectEx("ExpansionDebugBox_Purple", position, ECE_NOLIFETIME);
				debugBox.SetOrientation(orientation);
				debugBox.SetPosition(debugBox.ModelToWorld(Vector(-size[0] / 2, size[1], size[2] / 2)));
				#endif
			}
			else
			{
				//! Fallback just in case
				size = "5 5 10";
			}
			s_VehicleSizes.Insert(className, size);
		}

		//! Reduce check dimensions by 5% so we don't get blocked by objects the vehicle may stand on
		vector checkDimensions = Vector(size[0] * 0.95, size[1] * 0.95, size[2] * 0.95);
		vector checkPosition = Vector(position[0], position[1] + size[1] * 0.05, position[2]);

		blockingObject = GetObjectBlockingPosition(checkPosition, orientation, checkDimensions);

		CF_Log.Debug("IsVehicleSpawnPositionFree " + className + " size " + size + " pos " + position + " ori " + orientation + " blocking " + blockingObject);
		
		return blockingObject == NULL;
	}
	
	static Object GetObjectBlockingPosition(vector position, vector orientation, vector size, array<Object> excluded_objects = null)
	{
#ifdef EXPANSIONTRACE
		auto trace = EXTrace.Start(ExpansionTracing.GENERAL_ITEMS, ExpansionItemSpawnHelper);
#endif

		array<Object> objects = new array<Object>;
		
		if (!GetGame().IsBoxColliding( position + Vector(0, size[1] / 2, 0), orientation, size, excluded_objects, objects))
			return NULL;

		foreach (Object obj: objects)
		{
			bool match = false;
			//! https://feedback.bistudio.com/T173348
			if (obj.IsInherited(Man) || (obj.IsInherited(ItemBase) && obj.ConfigGetString("physLayer") == "item_large") || ExpansionVehicle.Get(obj))
				match = true;

			if (match)
				return obj;
		}

		return NULL;
	}

	// ------------------------------------------------------------
	// Expansion Object SpawnVehicle
	// ------------------------------------------------------------
	static Object SpawnVehicle( string className, PlayerBase player, inout EntityAI parent, vector position, vector orientation, out int remainingAmount, TStringArray attachments = NULL, int skinIndex = -1,  string carKeyName = "", inout bool attachmentNotAttached = false )
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.GLOBAL, "ExpansionItemSpawnHelper", "SpawnVehicle");
#endif

		int flags = ECE_CREATEPHYSICS|ECE_UPDATEPATHGRAPH;

		bool placeOnSurface;
		//! https://feedback.bistudio.com/T173348
		if (!GetGame().IsKindOf(className, "ExpansionBoatScript") && !GetGame().IsKindOf(className, "ExpansionVehicleBoatBase"))
			placeOnSurface = true;
		if (placeOnSurface)
			flags |= ECE_TRACE;

		Object obj = GetGame().CreateObjectEx(className, position, flags);

		remainingAmount--;

		ExpansionVehicle vehicle;
		ItemBase keyb;
		#ifdef EXPANSIONMODVEHICLE
		ExpansionCarKey expkey;
		#endif
		#ifdef HypeTrain
		HypeTrain_PartBase hypeTrain;
		#endif
		if (ExpansionVehicle.Get(vehicle, obj))
		{
			vehicle.FillFluids();

			if (skinIndex > -1)
				vehicle.SetSkin(skinIndex);

			if (carKeyName != "")
			{
				Class.CastTo(keyb, SpawnInInventorySecure(carKeyName, player, parent));

				#ifdef EXPANSIONMODVEHICLE
				if (Class.CastTo(expkey, keyb) && vehicle.CanBeLocked())
				{
					vehicle.PairKey(expkey);
					vehicle.Lock(expkey);
				}
				#endif
			}

			#ifdef EXPANSIONMODVEHICLE
			float dist = vehicle.GetModelZeroPointDistanceFromGround();
			if (placeOnSurface && position[1] + dist > vehicle.GetPosition()[1])
				vehicle.GetEntity().SetPosition(position + Vector(0, dist, 0));
			#endif
		}
		#ifdef HypeTrain
		else if (Class.CastTo(hypeTrain, obj))
		{
			if (hypeTrain.GetLiquidQuantityMax())
				hypeTrain.SetLiquidQuantity(hypeTrain.GetLiquidQuantityMax());
		}
		#endif
		else
		{
			if (obj)
			{
				GetGame().ObjectDelete(obj);
			}
			
			return NULL;
		}

		#ifdef EXPANSIONMODVEHICLE
		//! Prevent boat getting flung in the air or stuck on ground due to random physics interaction
		if (obj.IsInherited(ExpansionBoatScript))
		{
			SetVelocity( obj, Vector( 0, 0, 0 ) );
			dBodySetAngularVelocity( obj, Vector( 0, 0, 0 ) );
			dBodyActive( obj, ActiveState.INACTIVE );  //! Boat physics will get reactivated by its DeferredInit
		}
		#endif

		obj.SetOrientation(orientation);

		if (attachments)
			SpawnAttachments(attachments, player, EntityAI.Cast(obj), skinIndex, false, attachmentNotAttached);

		return obj;
	}

	protected static void ISHDebugPrint(string text)
	{
	}

	static string DumpLocationToString(InventoryLocation loc)
	{
		EXError.WarnOnce(null, "DEPRECATED, use ExpansionStatic::DumpToString");

		return ExpansionStatic.DumpToString(loc);
	}

	//! https://pastebin.com/FFuaPFiT, except without bugs :P
	static EntityAI Clone(EntityAI src, bool recursively = true, InventoryLocation location = null, bool keepUnlootable = false)
	{
		int i;

		//! 1) create entity at location
		EntityAI dst;
		switch (location.GetType())
		{
			case InventoryLocationType.GROUND:
				//! XXX: LocationCreateEntity with ECE_PLACE_ON_SURFACE broke somewhere around 1.20 (object rotation isn't applied)
				//dst = GameInventory.LocationCreateEntity(location, src.GetType(), ECE_PLACE_ON_SURFACE, RF_DEFAULT);
				dst = EntityAI.Cast(GetGame().CreateObjectEx(src.GetType(), location.GetPos(), ECE_PLACE_ON_SURFACE));
				break;
			case InventoryLocationType.ATTACHMENT:
				dst = GameInventory.LocationCreateEntity(location, src.GetType(), ECE_IN_INVENTORY, RF_DEFAULT);
				int slotId = location.GetSlot();
				if (location.GetParent() && src.GetHierarchyParent())
					location.GetParent().GetInventory().SetSlotLock(slotId, src.GetHierarchyParent().GetInventory().GetSlotLock(slotId));
				break;
			case InventoryLocationType.HANDS:
				dst = GameInventory.LocationCreateEntity(location, src.GetType(), ECE_IN_INVENTORY, RF_DEFAULT);
				break;
			case InventoryLocationType.CARGO:
			case InventoryLocationType.PROXYCARGO:
				dst = location.GetParent().GetInventory().CreateEntityInCargoEx(src.GetType(), location.GetIdx(), location.GetRow(), location.GetCol(), location.GetFlip());  //! Only way to get flip correct
				break;
			default:
				EXTrace.Print(EXTrace.GENERAL_ITEMS, ExpansionItemSpawnHelper, "::Clone - unknown location type " + location.GetType());
				return null;
		}

		if (!dst)
		{
			EXTrace.Print(EXTrace.GENERAL_ITEMS, ExpansionItemSpawnHelper, "::Clone - failed to create " + src.GetType() + " at location " + ExpansionStatic.DumpToString(location));
			return null;
		}

		location.SetItem(dst);

		EXTrace.Print(EXTrace.GENERAL_ITEMS, ExpansionItemSpawnHelper, "::Clone - created " + Object.GetDebugName(dst) + " at location " + ExpansionStatic.DumpToString(location));

		//! @note order of operations matters! DO NOT CHANGE!

		//! 2) attachments + cargo
		if (recursively)
		{
			EntityAI cSrc;
			InventoryLocation cLocation();
			InventoryLocation dLocation();
			for (i = 0; i < src.GetInventory().AttachmentCount(); i++)
			{
				cSrc = src.GetInventory().GetAttachmentFromIndex(i);
				cSrc.GetInventory().GetCurrentInventoryLocation(cLocation);
                dLocation.SetAttachment(dst, null, cLocation.GetSlot());
				Clone(cSrc, recursively, dLocation);
			}

			if (src.GetInventory().GetCargo())
			{
				for (i = 0; i < src.GetInventory().GetCargo().GetItemCount(); i++)
				{
					cSrc = src.GetInventory().GetCargo().GetItem(i);
					cSrc.GetInventory().GetCurrentInventoryLocation(cLocation);
					dLocation.SetCargo(dst, null, cLocation.GetIdx(), cLocation.GetRow(), cLocation.GetCol(), cLocation.GetFlip());
					Clone(cSrc, recursively, dLocation);
				}
			}
		}

		//! 3) special treatment for weapons
		Weapon_Base srcWeapon;
		Weapon_Base dstWeapon;
		if (Class.CastTo(srcWeapon, src) && Class.CastTo(dstWeapon, dst))
		{
			float ammoDamage;
			string ammoTypeName;

			for (int mi = 0; mi < srcWeapon.GetMuzzleCount(); ++mi)
			{
				if (!srcWeapon.IsChamberEmpty(mi))
				{
					if (srcWeapon.GetCartridgeInfo(mi, ammoDamage, ammoTypeName))
					{
						dstWeapon.PushCartridgeToChamber(mi, ammoDamage, ammoTypeName);
					}
				}
				
				for (int ci = 0; ci < srcWeapon.GetInternalMagazineCartridgeCount(mi); ++ci)
				{
					if (srcWeapon.GetInternalMagazineCartridgeInfo(mi, ci, ammoDamage, ammoTypeName))
					{
						dstWeapon.PushCartridgeToInternalMagazine(mi, ammoDamage, ammoTypeName);
					}
				}
			}
		}

		//! 4) storesave/storeload
		ScriptReadWriteContext ctx = new ScriptReadWriteContext;
		src.OnStoreSave(ctx.GetWriteContext());
		dst.OnStoreLoad(ctx.GetReadContext(), GetGame().SaveVersion());

		if (dstWeapon)
			dstWeapon.Expansion_ResetMuzzleModes();

		//! 4b) Volatile variables
		ItemBase srcItem;
		ItemBase dstItem;
		if (Class.CastTo(srcItem, src) && Class.CastTo(dstItem, dst))
		{
			if (!srcItem.Expansion_IsLootable() && keepUnlootable)
				dstItem.Expansion_SetLootable(false);
			dstItem.m_Expansion_PreviousOwner = srcItem.m_Expansion_PreviousOwner;
		}

		//! 5) special treatment for mags
		Magazine srcMag;
		Magazine dstMag;
		if (Class.CastTo(srcMag, src) && Class.CastTo(dstMag, dst))
		{
			float damage;  //! @note damage is the damage of the cartridge itself (0..1), NOT the damage it inflicts!
			string cartTypeName;
			dstMag.ServerSetAmmoCount(0);
			for (i = 0; i < srcMag.GetAmmoCount(); i++)
			{
				srcMag.GetCartridgeAtIndex(i, damage, cartTypeName);
				dstMag.ServerStoreCartridge(damage, cartTypeName);
			}
		}

		//! 6) Special treatment for vehicles (no special treatment needed for ExpansionVehicleBase since it inherits from ItemBase)
		ExpansionVehicle srcVehicle;
		ExpansionVehicle dstVehicle;
		if (ExpansionVehicle.Get(srcVehicle, src) && ExpansionVehicle.Get(dstVehicle, dst))
		{
			srcVehicle.CloneFluidsTo(dstVehicle);
		}

		//! 7) global health and damage zones
		TStringArray dmgZones();
		src.GetDamageZones(dmgZones);
		dst.SetHealth(src.GetHealth());
		foreach (string dmgZone: dmgZones)
		{
			dst.SetHealth(dmgZone, "Health", src.GetHealth(dmgZone, "Health"));
		}

		//! FINAL
		dst.AfterStoreLoad();
		dst.SetSynchDirty();
		if (dstWeapon)
			dstWeapon.Synchronize();
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Call(dst.EEOnAfterLoad);  //! Make sure EEOnAfterLoad gets called AFTER whole hierarchy has loaded

		return dst;
	}
}
