/**
 * ExpansionMarketModule.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

enum ExpansionMarketResult
{
	Success = 0,
	SellSuccess,
	PurchaseSuccess,
	FailedReserveTime,
	FailedNoCount,
	FailedUnknown,
	FailedStockChange,
	FailedOutOfStock,
	FailedAttachmentOutOfStock,
	FailedNotEnoughMoney,

	//! NOTE: Make sure that vehicle enums are together with no others in between!
	FailedNoVehicleSpawnPositions,
	FailedNotEnoughVehicleSpawnPositionsNear,
	FailedVehicleSpawnOccupied,

	FailedTooFarAway,
	FailedCannotSell,
	FailedCannotBuy,
	FailedNotInPlayerPossession,
	FailedItemDoesNotExistInTrader,
	FailedItemSpawn,
	FailedSellListMismatch,

	IntegerOverflow
}

class ExpansionMarketPlayerInventory
{
	PlayerBase m_Player;
	ref array<EntityAI> m_Inventory;

	void ExpansionMarketPlayerInventory(PlayerBase player)
	{
		m_Player = player;
		m_Inventory = new array<EntityAI>;
		Enumerate();
	}

	void ~ExpansionMarketPlayerInventory()
	{
	#ifdef EXPANSIONMODMARKET_DEBUG
		EXPrint("~ExpansionMarketPlayerInventory");
	#endif
	}

	void Enumerate()
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(ExpansionTracing.MARKET, this);
#endif

		array<EntityAI> items = new array<EntityAI>;
		items.Reserve(m_Player.GetInventory().CountInventory());

		m_Player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);
		AddPlayerItems(items);

		array<EntityAI> driven = GetNearbyDrivenVehicles();
		AddPlayerItems(driven, m_Inventory);
	}

	private void AddPlayerItems(array<EntityAI> items, array<EntityAI> existing = NULL)
	{
		foreach (EntityAI item: items)
		{
			if (!item || item.IsSetForDeletion())
				continue;

			item = SubstituteOwnedVehicle(item);

			if (existing && existing.Find(item) > -1)
				continue;

			m_Inventory.Insert( item );
		}
	}
	
	EntityAI SubstituteOwnedVehicle(EntityAI item)
	{
		#ifdef EXPANSIONMODVEHICLE
		//! If this is a master key of a vehicle, substitute the vehicle itself if it's close
		ExpansionCarKey key;
		if (Class.CastTo(key, item) && key.IsMaster())
		{
			Object keyObject = key.GetKeyObject();

			if (keyObject && IsVehicleNearby(keyObject))
				item = EntityAI.Cast(keyObject);
		}
		#endif

		return item;
	}
	
	array<EntityAI> GetNearbyDrivenVehicles(string className = "", int amount = -1)
	{
		array<EntityAI> driven = new array<EntityAI>;

		string type;
		EntityAI vehicleEntity;

		auto node = ExpansionVehicle.s_All.m_Head;
		while (node)
		{
			auto vehicle = node.m_Value;
			node = node.m_Next;
			type = vehicle.GetType();
			type.ToLower();
			if ((!className || type == className) && vehicle.GetLastDriverUID() == m_Player.GetIdentityUID())
			{
				#ifdef EXPANSIONMODVEHICLE
				if (vehicle.IsLocked())
					continue;
				#endif
				
				if (vehicle.GetCrew().Count() > 0)
					continue;

				vehicleEntity = vehicle.GetEntity();
				if (IsVehicleNearby(vehicleEntity))
					driven.Insert(vehicleEntity);

				if (driven.Count() == amount)
					return driven;
			}
		}

		return driven;
	}
	
	bool IsVehicleNearby(Object vehicle)
	{
		float maxDistance = GetExpansionSettings().GetMarket().GetMaxVehicleDistanceToTrader(vehicle.GetType());

		if (vector.Distance(m_Player.GetPosition(), vehicle.GetPosition()) <= maxDistance)
			return true;

		return false;
	}
}

[CF_RegisterModule(ExpansionMarketModule)]
class ExpansionMarketModule: CF_ModuleWorld
{
	static const float MAX_TRADER_INTERACTION_DISTANCE = 5;

	static ref ExpansionMarketModule s_Instance;

	static ref ScriptInvoker SI_SetTraderInvoker = new ScriptInvoker();
	static ref ScriptInvoker SI_SelectedItemUpdatedInvoker = new ScriptInvoker();
	static ref ScriptInvoker SI_Callback = new ScriptInvoker();
	static ref ScriptInvoker SI_ATMMenuInvoker = new ScriptInvoker();
	static ref ScriptInvoker SI_ATMMenuCallback = new ScriptInvoker();
	static ref ScriptInvoker SI_ATMMenuTransferCallback = new ScriptInvoker();
	static ref ScriptInvoker SI_ATMMenuPartyCallback = new ScriptInvoker();

	//! Client
	protected ref ExpansionMarketPlayerInventory m_LocalEntityInventory;
	protected ref TIntArray m_TmpVariantIds;
	protected int m_TmpVariantIdIdx;
	protected ref map<int, ref ExpansionMarketCategory> m_TmpNetworkCats;
	protected ref array<ref ExpansionMarketNetworkBaseItem> m_TmpNetworkBaseItems;
	protected int m_PlayerWorth;

	ref map<string, int> m_MoneyTypes;
	ref array<string> m_MoneyDenominations;

	protected ref ExpansionMarketTraderZone m_ClientMarketZone;
	
	protected ExpansionTraderObjectBase m_OpenedClientTrader;
	protected EntityAI m_TraderEntity;
	
	ref array<ref ExpansionMarketATM_Data> m_ATMData;

	ref map<string, ExpansionMarketItem> m_AmmoItems;

	static ref map<string, string> s_AmmoBullets = new map<string, string>;

	// ------------------------------------------------------------
	// ExpansionMarketModule Constructor
	// ------------------------------------------------------------	
	void ExpansionMarketModule()
	{
		s_Instance = this;

		m_TmpVariantIds = new TIntArray;
		m_TmpNetworkCats = new map<int, ref ExpansionMarketCategory>;
		m_TmpNetworkBaseItems = new array<ref ExpansionMarketNetworkBaseItem>;

		m_MoneyTypes = new map<string, int>;
		m_MoneyDenominations = new array<string>;

		m_AmmoItems = new map<string, ExpansionMarketItem>;

		m_ClientMarketZone = new ExpansionMarketClientTraderZone;

		m_ATMData = new array<ref ExpansionMarketATM_Data>;
	}
	
	static ExpansionMarketModule GetInstance()
	{
		return s_Instance;
	}

	// ------------------------------------------------------------
	// ExpansionMarketModule OnInit
	// ------------------------------------------------------------	
	override void OnInit()
	{
		super.OnInit();

		EnableMissionStart();
		EnableInvokeConnect();
		EnableMissionFinish();
		EnableMissionLoaded();
		Expansion_EnableRPCManager();

		Expansion_RegisterClientRPC("RPC_Callback");
		Expansion_RegisterClientRPC("RPC_MoneyDenominations");
		Expansion_RegisterServerRPC("RPC_RequestPurchase");
		Expansion_RegisterServerRPC("RPC_CancelPurchase");
		Expansion_RegisterServerRPC("RPC_RequestSell");
		Expansion_RegisterServerRPC("RPC_CancelSell");
		Expansion_RegisterServerRPC("RPC_RequestTraderData");
		Expansion_RegisterClientRPC("RPC_LoadTraderData");
		Expansion_RegisterServerRPC("RPC_RequestTraderItems");
		Expansion_RegisterClientRPC("RPC_LoadTraderItems");
		Expansion_RegisterServerRPC("RPC_ExitTrader");
		Expansion_RegisterServerRPC("RPC_RequestPlayerATMData");
		Expansion_RegisterClientRPC("RPC_SendPlayerATMData");
		Expansion_RegisterServerRPC("RPC_RequestDepositMoney");
		Expansion_RegisterClientRPC("RPC_ConfirmDepositMoney");
		Expansion_RegisterServerRPC("RPC_RequestWithdrawMoney");
		Expansion_RegisterClientRPC("RPC_ConfirmWithdrawMoney");
		Expansion_RegisterServerRPC("RPC_RequestTransferMoneyToPlayer");
		Expansion_RegisterClientRPC("RPC_ConfirmTransferMoneyToPlayer");
	#ifdef EXPANSIONMODGROUPS
		Expansion_RegisterServerRPC("RPC_RequestPartyTransferMoney");
		Expansion_RegisterClientRPC("RPC_ConfirmPartyTransferMoney");
		Expansion_RegisterServerRPC("RPC_RequestPartyWithdrawMoney");
		Expansion_RegisterClientRPC("RPC_ConfirmPartyWithdrawMoney");
	#endif
	}
	
	// ------------------------------------------------------------
	// Override OnMissionStart
	// ------------------------------------------------------------	
	override void OnMissionStart(Class sender, CF_EventArgs args)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		super.OnMissionStart(sender, args);
		
		if (!IsMissionClient() && IsMissionHost())
		{
			if (!FileExist(EXPANSION_ATM_FOLDER))
			{
				ExpansionStatic.MakeDirectoryRecursive(EXPANSION_ATM_FOLDER);
			}
			else if (FileExist(EXPANSION_ATM_FOLDER))
			{
				LoadATMData();
			}
		}
		
		if (IsMissionClient() && !IsMissionHost())
		{
			if (!FileExist(EXPANSION_MARKET_PRESETS_FOLDER))
			{
				ExpansionStatic.MakeDirectoryRecursive(EXPANSION_MARKET_PRESETS_FOLDER);
			}
			
			if (!FileExist(EXPANSION_MARKET_WEAPON_PRESETS_FOLDER))
			{
				ExpansionStatic.MakeDirectoryRecursive(EXPANSION_MARKET_WEAPON_PRESETS_FOLDER);
			}
			
			if (!FileExist(EXPANSION_MARKET_CLOTHING_PRESETS_FOLDER))
			{
				ExpansionStatic.MakeDirectoryRecursive(EXPANSION_MARKET_CLOTHING_PRESETS_FOLDER);
			}
		}
	}

	// ------------------------------------------------------------
	// Override OnMissionLoaded
	// ------------------------------------------------------------
	override void OnMissionLoaded(Class sender, CF_EventArgs args)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		super.OnMissionLoaded(sender, args);

		if (!GetGame().IsServer())
			return;
		
		LoadMoneyPrice();
	}
	
	// ------------------------------------------------------------
	// Override OnMissionFinish
	// ------------------------------------------------------------
	override void OnMissionFinish(Class sender, CF_EventArgs args)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		super.OnMissionFinish(sender, args);

		m_MoneyTypes.Clear();
		m_MoneyDenominations.Clear();
		
		m_AmmoItems.Clear();

		if (IsMissionHost())
		{
			SaveATMData();
		}
		
		if (IsMissionClient())
		{
			//! Clear cached categories and traders so that they are requested from server again after (e.g.) reconnect, to make sure they are in sync
			auto settings = GetExpansionSettings().GetMarket(false);
			if (settings.IsLoaded())
				settings.ClearMarketCaches();
		}
	}

	// ------------------------------------------------------------
	// Expansion GetClientZone
	// ------------------------------------------------------------
	ExpansionMarketTraderZone GetClientZone()
	{
		return m_ClientMarketZone;
	}
	
	// ------------------------------------------------------------
	// Expansion int GetMoneyPrice
	// ------------------------------------------------------------	
	int GetMoneyPrice(string type)
	{
		int price;
		if (m_MoneyTypes && m_MoneyTypes.Contains(type))
		{
			price = m_MoneyTypes.Get(type);
			MarketModulePrint("GetMoneyPrice - Got price: " + string.ToString(price) + "| Type: " + type);
			return price;
		}
		
		MarketModulePrint("GetMoneyPrice - Failed to get price: " + string.ToString(price) + "| Type: " + type);
		return price;
	}

	// ------------------------------------------------------------
	// Expansion LoadMoneyPrice
	// ------------------------------------------------------------
	void LoadMoneyPrice()
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		ExpansionMarketSettings market = GetExpansionSettings().GetMarket();
		if (!market.MarketSystemEnabled && !market.ATMSystemEnabled)
			return;

		int i;
		int j;
		int min_idx;
		
		map<int, ref ExpansionMarketCategory> categories = market.GetCategories();

		foreach (int categoryID, ExpansionMarketCategory category : categories)
		{
			if (!category.IsExchange)
				continue;

			//! Loop through all the items in this category to get the different price denominations
			//! It's OK to not use m_Items here since we don't need variants
			foreach (ExpansionMarketItem marketItem: category.Items)
			{
				int worth = marketItem.MinPriceThreshold;
				
				Print(marketItem);
				Print(worth);
							
				string name = marketItem.ClassName;
				
				name.ToLower();
				
				m_MoneyTypes.Insert(name, worth);
				m_MoneyDenominations.Insert(name);
			}
		}

		//! Sort lowest to highest value currency
		for (i = 0; i < m_MoneyDenominations.Count() - 1; i++) 
		{
			min_idx = i;
			for (j = i + 1; j < m_MoneyDenominations.Count(); j++) 
			{
				int jMoney = GetMoneyPrice(m_MoneyDenominations[j]);
				int minIndexMoney = GetMoneyPrice(m_MoneyDenominations[min_idx]);
				if (jMoney < minIndexMoney)
				{
					min_idx = j;
				}
			}

			m_MoneyDenominations.SwapItems(min_idx, i);
		}

		//! Invert so array is now ordered from highest to lowest value currency
		m_MoneyDenominations.Invert();

		if (!m_MoneyDenominations.Count() && ExpansionGame.IsMultiplayerServer())
			Error("No market exchange found - cannot determine currency values! Make sure you have at least one market category containing currencies with IsExchange set to 1");
	}
	
	bool MoneyCheck(PlayerIdentity identity = NULL)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		if (!m_MoneyDenominations.Count())
		{
			ExpansionNotification("STR_EXPANSION_MARKET_TITLE", "No market exchange found - cannot determine currency values!").Error(identity);
			return false;
		}

		return true;
	}
	
	// ------------------------------------------------------------
	// Expansion GetTrader
	// ------------------------------------------------------------
	ExpansionTraderObjectBase GetTrader()
	{
		if (IsMissionClient())
		{
			MarketModulePrint("GetTrader - End and return!");
			
			return m_OpenedClientTrader;
		}

		Error( "GetTrader - Invalid operation" );
		MarketModulePrint("GetTrader - [ERROR]: End and NULL!");
				
		return NULL;
	}
	
	// ------------------------------------------------------------
	// Expansion FindSellPrice
	// ------------------------------------------------------------
	//! Find sell price and check if item can be sold. `ExpansionMarketResult result` indicates reason if cannot be sold.
	bool FindSellPrice(notnull PlayerBase player, array<EntityAI> items, int stock, int amountWanted, ExpansionMarketSell sell, bool includeAttachments = true, out ExpansionMarketResult result = ExpansionMarketResult.Success, out string failedClassName = "")
	{
		MarketModulePrint("FindSellPrice - " + sell.Item.ClassName + " - stock " + stock + " wanted " + amountWanted);
		
		result = ExpansionMarketResult.Success;  //! Always set initial result to success, this is changed accordingly below where necessary

		if (!player)
		{
			Error("FindSellPrice - [ERROR]: Player Base is NULL!");
			result = ExpansionMarketResult.FailedUnknown;
			return false;
		}
		
		if (amountWanted < 0)
		{
			Error("FindSellPrice - [ERROR]: Amount wanted is smaller then 0: " + amountWanted);
			result = ExpansionMarketResult.FailedUnknown;
			return false;
		}
		
		if (!sell)
		{
			Error("FindSellPrice - [ERROR]: ExpansionMarketSell is NULL!");
			result = ExpansionMarketResult.FailedUnknown;
			return false;
		}

		if (!sell.Item)
		{		
			Error("FindSellPrice - [ERROR]: ExpansionMarketItem is NULL!");
			result = ExpansionMarketResult.FailedUnknown;
			return false;
		}
		
		if (!sell.Trader)
		{
			Error("FindSellPrice - [WARNING]: ExpansionMarketSell.Trader is NULL! Stock cannot be taken into account");
		}

		sell.Price = 0;
		sell.TotalAmount = 0;

		if (!sell.Trader.GetTraderMarket().CanSellItem(sell.Item.ClassName))
		{
			EXPrint(ToString() + "::FindSellPrice - Cannot sell " + sell.Item.ClassName + " to " + sell.Trader.GetTraderMarket().m_FileName + " trader");
			result = ExpansionMarketResult.FailedCannotSell;
			return false;
		}
		
		map<string, float> addedStock = new map<string, float>;

		float curAddedStock;

		ExpansionMarketTraderZone zone;

		if (GetGame().IsClient())
			zone = GetClientZone();
		else
			zone = sell.Trader.GetTraderZone();
		
		float initialSellPriceModifier = 1;

		if (!GetItemCategory(sell.Item).IsExchange)
		{
			float sellPricePct = sell.Item.SellPricePercent;
			if (sellPricePct < 0)
				sellPricePct = zone.SellPricePercent;
			if (sellPricePct < 0)
				sellPricePct = GetExpansionSettings().GetMarket().SellPricePercent;
			initialSellPriceModifier = sellPricePct / 100;
		}

		MarketModulePrint("FindSellPrice - player inventory: " + items.Count() + " - looking for " + sell.Item.ClassName);
		
		int unsellablePrice;

		foreach (EntityAI itemEntity: items) 
		{
			string itemClassName = itemEntity.GetType();
			itemClassName.ToLower();
			
			itemClassName = GetMarketItemClassName(sell.Trader.GetTraderMarket(), itemClassName);

			if (itemClassName == sell.Item.ClassName)
			{
				int playerInventoryAmount = GetItemAmount(itemEntity);
				int amountTaken;  //! Amount taken from inventory
				int amountLeft;   //! Amount left in inventory after deducting taken
				bool canSell = true;
				
				if (playerInventoryAmount < 0)
				{
					//! Can't sell this
					result = ExpansionMarketResult.FailedCannotSell;
					playerInventoryAmount = Math.AbsInt(playerInventoryAmount);
					canSell = false;
				}

				//! If the item is stackable then we want to remove the wanted amount from this item pile.
				if (playerInventoryAmount >= 1)
				{
					//! If the item pile contains more or exacly the amount of units we wanted then we take that requested amount from that pile.
					if (playerInventoryAmount >= amountWanted)
					{
						amountTaken = amountWanted;
					}
					else
					{
						amountTaken = playerInventoryAmount;
					}
				}
				else
				{
					//! This should never happen
					//! (the only way it could happen is if a stackable item with a quantity of zero doesn't autodestroy)
					//! Just ignore the item
					continue;
				}

				if (canSell)
				{
					amountLeft = playerInventoryAmount - amountTaken;
					amountWanted -= amountTaken;
									
					MarketModulePrint("FindSellPrice - original amount in inventory: " + playerInventoryAmount);
					MarketModulePrint("FindSellPrice - amount taken: " + amountTaken);
					MarketModulePrint("FindSellPrice - amount left in inventory: " + amountLeft);
					MarketModulePrint("FindSellPrice - amount still wanted: " + amountWanted);
				}

				float incrementStockModifier;
				float modifier = GetSellPriceModifier(itemEntity, incrementStockModifier, initialSellPriceModifier);

				if (canSell)
				{
					sell.AddSellItem(amountTaken, amountTaken * incrementStockModifier, itemEntity, sell.Item.ClassName);
					sell.TotalAmount += amountTaken;
				}

				//! Process all attachments (and attachments of attachments)
				int currentPrice = sell.Price;
				if (includeAttachments && !FindAttachmentsSellPrice(itemEntity, sell, addedStock, canSell, failedClassName))
				{
					result = ExpansionMarketResult.FailedItemDoesNotExistInTrader;
					return false;
				}

				int price = 0;
				int singleItemPrice;
				for (int j = 0; j < amountTaken; j++)
				{
					if (canSell && !sell.Item.IsStaticStock())
						curAddedStock += incrementStockModifier;

					if (ExpansionGame.IsMultiplayerServer())
						MarketModulePrint(ToString() + "::FindSellPrice - " + sell.Item.ClassName + " stock " + stock + " increment stock " + curAddedStock);

					singleItemPrice = sell.Item.CalculatePrice(stock + curAddedStock, modifier);

					if (ExpansionMath.TestAdditionOverflow(price, singleItemPrice))
					{
						result = ExpansionMarketResult.IntegerOverflow;
						return false;
					}

					price += singleItemPrice;
				}

				if (canSell)
				{
					if (ExpansionMath.TestAdditionOverflow(sell.Price, price))
					{
						result = ExpansionMarketResult.IntegerOverflow;
						return false;
					}

					sell.Price += price;
				}
				else
				{
					int unsellableAttachmentsPrice = sell.Price - currentPrice;

					if (ExpansionMath.TestAdditionOverflow(price, unsellableAttachmentsPrice))
					{
						result = ExpansionMarketResult.IntegerOverflow;
						return false;
					}

					int currentUnsellablePrice = price + unsellableAttachmentsPrice;

					if (ExpansionMath.TestAdditionOverflow(unsellablePrice, currentUnsellablePrice))
					{
						result = ExpansionMarketResult.IntegerOverflow;
						return false;
					}

					unsellablePrice += currentUnsellablePrice;
					sell.Price -= unsellableAttachmentsPrice;
				}

				if (amountWanted == 0)
				{
					MarketModulePrint("FindSellPrice - End and return true");
					result = ExpansionMarketResult.Success;
					return true;
				}
			}
		}
		
		if (result == ExpansionMarketResult.Success)
		{
			sell.Price = sell.Item.CalculatePrice(stock, initialSellPriceModifier);
			result = ExpansionMarketResult.FailedNotInPlayerPossession;
			MarketModulePrint("FindSellPrice - not in player possession");
		}
		else if (result == ExpansionMarketResult.FailedCannotSell && !sell.Price)
		{
			if (ExpansionMath.TestAdditionOverflow(sell.Price, unsellablePrice))
				result = ExpansionMarketResult.IntegerOverflow;

			sell.Price += unsellablePrice;
			MarketModulePrint("FindSellPrice - cannot sell");
		}

		MarketModulePrint("FindSellPrice - End and return false");
		return false;
	}

	bool FindAttachmentsSellPrice(EntityAI itemEntity, ExpansionMarketSell sell, inout map<string, float> addedStock = NULL, bool canSell = true, out string failedClassName = "")
	{
		ExpansionMarketTraderZone zone;

		if (GetGame().IsClient())
			zone = GetClientZone();
		else
			zone = sell.Trader.GetTraderZone();

		int i;

		if (itemEntity.IsInherited(MagazineStorage))
		{
			//! Magazines
			Magazine mag;
			Class.CastTo(mag, itemEntity);

			map<string, ref TFloatArray> sellPriceModifiers = new map<string, ref TFloatArray>;

			for (i = 0; i < mag.GetAmmoCount(); i++)
			{
				float damage;  //! NOTE: Damage is the damage of the cartridge itself (0..1), NOT the damage it inflicts!
				string cartTypeName;
				mag.GetCartridgeAtIndex(i, damage, cartTypeName);
	
				//! Need to round, otherwise value might not match between server and client
				//! TODO: Fuck it, my assumption is this still leads to different values on client and server sometimes due to DayZ being DayZ, so just set damage to 0
				damage = 0; //Math.Round(damage * 10) * 0.01;
				MarketModulePrint("Bullet: " + cartTypeName + ", " + "Damage: "+ damage.ToString());

				float sellPriceModifierCur = 1 - damage; 
				TFloatArray sellPriceModifiersCur;
				if (!sellPriceModifiers.Find(cartTypeName, sellPriceModifiersCur))
				{
					sellPriceModifiersCur = new TFloatArray;
					sellPriceModifiers.Insert(cartTypeName, sellPriceModifiersCur);
				}
				sellPriceModifiersCur.Insert(sellPriceModifierCur);
			}

			foreach (string bulletClassName, TFloatArray modifiers: sellPriceModifiers)
			{
				ExpansionMarketItem ammoItem = NULL;
				if (!m_AmmoItems.Find(bulletClassName, ammoItem))
				{
					string ammoClassName = GetGame().ConfigGetTextOut("CfgAmmo " + bulletClassName + " spawnPileType");
					ammoClassName.ToLower();
					ammoItem = ExpansionMarketCategory.GetGlobalItem(ammoClassName, false);
					if (!ammoItem)
					{
						EXPrint("FindAttachmentsSellPrice - market item " + ammoClassName + " (" + bulletClassName + ") does not exist");
						failedClassName = ammoClassName;
						return false;
					}
					m_AmmoItems.Insert(bulletClassName, ammoItem);
				}
				if (ammoItem)
				{
					int increaseStockBy = 0;
					float sellPriceModifier = 0;
					foreach (int modifier: modifiers)
					{
						//! Ruined bullets shall not increase stock
						if (modifier)
							increaseStockBy += 1;

						sellPriceModifier += modifier;
					}
					if (increaseStockBy)
						sellPriceModifier /= increaseStockBy;

					if (!FindAttachmentsSellPriceInternal(ammoItem, NULL, sell, addedStock, zone, canSell, increaseStockBy, sellPriceModifier, failedClassName))
						return false;
				}
			}

			return true;
		}

		//! Everything else

		if (!itemEntity.GetInventory())
			return true;

		for (i = 0; i < itemEntity.GetInventory().AttachmentCount(); i++)
		{
			EntityAI attachmentEntity = itemEntity.GetInventory().GetAttachmentFromIndex(i);

			if (!attachmentEntity)
				continue;

			string attachmentName = attachmentEntity.GetType();
			attachmentName.ToLower();

			attachmentName = GetMarketItemClassName(sell.Trader.GetTraderMarket(), attachmentName);

			ExpansionMarketItem attachment = ExpansionMarketCategory.GetGlobalItem(attachmentName, false);

			if (!attachment)
				continue;

			if (!FindAttachmentsSellPriceInternal(attachment, attachmentEntity, sell, addedStock, zone, canSell, 1, 0.75, failedClassName))
				return false;
		}

		return true;
	}

	protected bool FindAttachmentsSellPriceInternal(ExpansionMarketItem attachment, EntityAI attachmentEntity, ExpansionMarketSell sell, inout map<string, float> addedStock, ExpansionMarketTraderZone zone, bool canSell = true, int amount = 1, float modifier = 0.75, out string failedClassName = "")
	{
		if (!sell.Trader.GetTraderMarket().Items[attachment.ClassName])
		{
			//! This attachment does not exist in the trader, but we still allow to get rid of it (it will be deleted, and players won't receive money for it)
			return true;
		}
		else if (!sell.Trader.GetTraderMarket().CanSellItem(attachment.ClassName))
		{
			EXPrint(ToString() + "::FindSellPrice - INFO: Cannot sell attachment " + attachment.ClassName + " to " + sell.Trader.GetTraderMarket().m_FileName + " trader");
			//! This attachment cannot be sold to the trader, but we still allow to get rid of it (it will be deleted, and players won't receive money for it)
			return true;
		}

		int stock = 1;

		float curAddedStock;

		if (!attachment.IsStaticStock())
		{
			stock = zone.GetStock(attachment.ClassName);
			if (stock < 0)  //! Attachment does not exist in trader zone - set min stock so price calc can work correctly
				stock = attachment.MinStockThreshold;

			curAddedStock = addedStock.Get(attachment.ClassName);
		}

		float initialSellPriceModifier = 1;

		if (!GetItemCategory(attachment).IsExchange)
		{
			float sellPricePct = attachment.SellPricePercent;
			if (sellPricePct < 0)
				sellPricePct = zone.SellPricePercent;
			if (sellPricePct < 0)
				sellPricePct = GetExpansionSettings().GetMarket().SellPricePercent;
			initialSellPriceModifier = sellPricePct / 100;
		}

		float incrementStockModifier;
		if (attachmentEntity)
		{
			modifier = GetSellPriceModifier(attachmentEntity, incrementStockModifier, initialSellPriceModifier);
		}
		else
		{
			modifier *= initialSellPriceModifier;
			incrementStockModifier = modifier > 0;
			MarketModulePrint("Sell price modifier " + attachment.ClassName + " -> " + modifier + " incrementStockModifier " + incrementStockModifier);
		}

		if (canSell)
			sell.AddSellItem(0, amount * incrementStockModifier, attachmentEntity, attachment.ClassName);

		int price = 0;
		int singleAttachmentPrice;
		for (int j = 0; j < amount; j++)
		{
			if (canSell && !attachment.IsStaticStock())
				curAddedStock += incrementStockModifier;

			if (ExpansionGame.IsMultiplayerServer())
				MarketModulePrint(ToString() + "::FindAttachmentsSellPriceInternal - " + attachment.ClassName + " stock " + stock + " increment stock " + curAddedStock);

			singleAttachmentPrice = attachment.CalculatePrice(stock + curAddedStock, modifier);

			//! TODO: Need to carry IntegerOverflow result back through call chain so it can be passed along
			if (ExpansionMath.TestAdditionOverflow(price, singleAttachmentPrice))
				return false;

			price += singleAttachmentPrice;
		}

		//! TODO: Need to carry IntegerOverflow result back through call chain so it can be passed along
		if (ExpansionMath.TestAdditionOverflow(sell.Price, price))
			return false;

		sell.Price += price;

		if (canSell && !attachment.IsStaticStock())
		{
			if (addedStock.Contains(attachment.ClassName))
				addedStock.Set(attachment.ClassName, curAddedStock);
			else
				addedStock.Insert(attachment.ClassName, curAddedStock);
		}

		if (attachmentEntity)
			return FindAttachmentsSellPrice(attachmentEntity, sell, addedStock, canSell, failedClassName);

		return true;
	}

	//! Get sell price modifier, taking into account item condition (including quantity and food stage for food)
	//! Ruined items will always return a sell price modifier of 0.0
	//! If sell price modifier reaches zero, `incrementStockModifier` will be zero as well, otherwise 1 for non-ruined non-food items,
	//! and from 0.0 to <modifier> for food items, depending on quantity and food stage (when applicable, non-raw food will have a value of 0.0).
	static float GetSellPriceModifier(EntityAI item, out float incrementStockModifier, float modifier = 0.75)
	{
		float conditionModifier = 1.0;
		float quantityModifier = 1.0;

		switch (item.GetHealthLevel())
		{
			case GameConstants.STATE_PRISTINE:
				break;

			case GameConstants.STATE_WORN:
				conditionModifier = 0.75;
				break;

			case GameConstants.STATE_DAMAGED:
				conditionModifier = 0.5;
				break;

			case GameConstants.STATE_BADLY_DAMAGED:
				conditionModifier = 0.25;
				break;

			case GameConstants.STATE_RUINED:
				conditionModifier = 0;
				break;
		}

		//! Selling ruined items shall not increase stock
		incrementStockModifier = conditionModifier > 0;  //! 0.0 or 1.0

		//! Any item with quantity except storage containers
		//! @note only non-splittable items, except edibles - needs to have compatible logic to ExpansionItemSpawnHelper::SpawnOnParent
		ItemBase consumable;
		if (conditionModifier && Class.CastTo(consumable, item) && (item.IsInherited(Edible_Base) || (!item.IsKindOf("Container_Base") && !consumable.Expansion_IsStackable())))
		{
			Edible_Base edible = Edible_Base.Cast(item);

			float minFactor;
			if (edible && !edible.ConfigGetBool("varQuantityDestroyOnMin"))
				minFactor = 0.25;  //! Quarter price at zero quantity for e.g. liquid containers

			if (consumable.HasQuantity() && !consumable.IsFullQuantity())  //! Full modifier at full quantity
				quantityModifier = ExpansionMath.LinearConversion(0.0, 1.0, consumable.GetQuantity() / consumable.GetQuantityMax(), minFactor, 1, true);

			if (edible && edible.HasFoodStage())
			{
				switch (edible.GetFoodStageType())
				{
					case FoodStageType.RAW:
						//! Let quantity and condition influence stock increment modifier
						if (conditionModifier == 1 && quantityModifier >= 0.75)  //! 0.0 or 1.0
							incrementStockModifier = 1.0;
						else
							incrementStockModifier = 0.0;
						break;

					//! Selling non-raw food shall not increase stock

					case FoodStageType.BAKED:
						quantityModifier *= 0.75;
						incrementStockModifier = 0;
						break;

					case FoodStageType.BOILED:
						quantityModifier *= 0.5;
						incrementStockModifier = 0;
						break;

					case FoodStageType.DRIED:
						quantityModifier *= 0.25;
						incrementStockModifier = 0;
						break;

					case FoodStageType.BURNED:
					case FoodStageType.ROTTEN:
						quantityModifier = 0;
						incrementStockModifier = 0;
						break;
				}
			}
		}

		modifier *= conditionModifier * quantityModifier;

		MarketModulePrint("GetSellPriceModifier " + item.ToString() + " (" + item.GetType() + ") -> " + modifier + " incrementStock " + incrementStockModifier);

		return modifier;
	}
	
	//! Check if item exists in trader, and if not, check if this is an Expansion skinned class (<Name>_<Skinname>) and return the skin base classname
	string GetMarketItemClassName(ExpansionMarketTrader trader, string itemClassName)
	{
		if (!trader.Items.Contains(itemClassName))
		{
			bool isCfgVehicleSkin;
			if (GetGame().ConfigIsExisting("CfgVehicles " + itemClassName + " skinBase"))
				isCfgVehicleSkin = true;
			bool isCfgWeaponSkin;
			if (!isCfgVehicleSkin && GetGame().ConfigIsExisting("CfgWeapons " + itemClassName + " skinBase"))
				isCfgWeaponSkin = true;
			bool isCfgMagazineSkin;
			if (!isCfgVehicleSkin && !isCfgWeaponSkin && GetGame().ConfigIsExisting("CfgMagazines " + itemClassName + " skinBase"))
				isCfgMagazineSkin = true;

			if (isCfgVehicleSkin || isCfgWeaponSkin || isCfgMagazineSkin)
			{
				if (isCfgVehicleSkin)
					GetGame().ConfigGetText("CfgVehicles " + itemClassName + " skinBase", itemClassName);
				else if (isCfgWeaponSkin)
					GetGame().ConfigGetText("CfgWeapons " + itemClassName + " skinBase", itemClassName);
				else if (isCfgMagazineSkin)
					GetGame().ConfigGetText("CfgMagazines " + itemClassName + " skinBase", itemClassName);

				itemClassName.ToLower();
			}
		}

		return itemClassName;
	}

	// ------------------------------------------------------------
	// Expansion GetItemAmount
	// Returns the amount of the given item the player has, taking into account whether it's stackable or not.
	// If the item is not sellable (might be a permanent condition because it's ruined
	// or a temporary one because it's attached to something or has cargo) returns -1
	// If the item is not stackable returns 1.
	// ------------------------------------------------------------
	int GetItemAmount(EntityAI item)
	{
		int amount;
		ItemBase itemBase;
		
		MarketModulePrint("GetItemAmount - Item type:" + item.GetType());
		
		//! @note the exception for Edible_Base is for things like TetracyclineAntibiotics that are
		//! stackable/splittable in vanilla but are awkward to buy/sell as individual pills
		if (!item.IsInherited(Edible_Base) && Class.CastTo(itemBase, item))
		{
			amount = itemBase.Expansion_GetStackAmount();
		}
		else
		{
			amount = 1;
		}
		
		//! Make sure we don't use items as money that should not be included, like attachments (e.g. Dogtags that are attached to the player itself) or that are in nested containers (1.25 change).
		if (!MiscGameplayFunctions.Expansion_IsLooseEntity(item))
			amount = -amount;
		
		return amount;
	}
	
	// ------------------------------------------------------------
	// Expansion Bool FindPurchasePriceAndReserve
	// ------------------------------------------------------------
	private bool FindPurchasePriceAndReserve(ExpansionMarketItem item, int amountWanted, out ExpansionMarketReserve reserved, bool includeAttachments = true, out ExpansionMarketResult result = ExpansionMarketResult.Success)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		if (!item)
		{		
			Error("FindPurchasePriceAndReserve - [ERROR]: ExpansionMarketItem is NULL!");
			return false;
		}
		
		ExpansionMarketTraderZone zone = reserved.Trader.GetTraderZone();
		
		if (!zone)
		{	
			Error("FindPurchasePriceAndReserve - [ERROR]: ExpansionMarketTraderZone is NULL!");
			return false;
		}

		ExpansionMarketTrader trader = reserved.Trader.GetTraderMarket();
		
		if (!trader)
		{	
			Error("FindPurchasePriceAndReserve - [ERROR]: ExpansionMarketTrader is NULL!");
			return false;
		}
		
		if (amountWanted < 0)
		{
			Error("FindPurchasePriceAndReserve - [ERROR]: Amount wanted is smaller then 0: " + amountWanted);
			return false;
		}
		
		MarketModulePrint("FindPurchasePriceAndReserve - Amount wanted: " + amountWanted);
		
		reserved.RootItem = item;
		reserved.TotalAmount = amountWanted;
		
		int price;
		if (!FindPriceOfPurchase(item, zone, trader, amountWanted, price, includeAttachments, result, reserved))
		{
			MarketModulePrint("FindPurchasePriceAndReserve - ExpansionMarketItem " + item.ClassName + " is out of stock, item is set to not be buyable or integer overflow! End and return false!");
			return false;
		}

		MarketModulePrint("FindPurchasePriceAndReserve - price: " + string.ToString(price));
					
		MarketModulePrint("FindPurchasePriceAndReserve - End and return true!");		

		return true;
	}
	
	// ------------------------------------------------------------
	// Expansion Bool FindPriceOfPurchase
	// ------------------------------------------------------------
	//! Returns true if item and attachments (if any) are in stock, false otherwise
	bool FindPriceOfPurchase(ExpansionMarketItem item, ExpansionMarketTraderZone zone, ExpansionMarketTrader trader, int amountWanted, inout int price, bool includeAttachments = true, out ExpansionMarketResult result = ExpansionMarketResult.Success, out ExpansionMarketReserve reserved = NULL, inout map<string, int> removedStock = NULL, out TStringArray outOfStockList = NULL, int level = 0)
	{
		int stock;

		if (item.IsStaticStock())
			stock = 1;
		else
			stock = zone.GetStock(item.ClassName);

		MarketModulePrint("FindPriceOfPurchase - " + item.ClassName + " - stock " + stock + " wanted " + amountWanted);

		if (!removedStock)
			removedStock = new map<string, int>;
		
		if (!outOfStockList)
			outOfStockList = new TStringArray;

		int curRemovedStock;
		if (!removedStock.Find(item.ClassName, curRemovedStock))
			removedStock.Insert(item.ClassName, 0);
		
		if (amountWanted > stock - curRemovedStock && !item.IsStaticStock())
		{
			result = ExpansionMarketResult.FailedOutOfStock;
		}
		
		if (!trader.CanBuyItem(item.ClassName))
		{
			result = ExpansionMarketResult.FailedCannotBuy;
			return false;
		}

		MarketModulePrint("FindPriceOfPurchase - Class name: " + item.ClassName);
		MarketModulePrint("FindPriceOfPurchase - Stock: " + (stock - curRemovedStock));
		MarketModulePrint("FindPriceOfPurchase - Amount wanted: " + amountWanted);

		float priceModifier = zone.BuyPricePercent / 100;

		int itemPrice;  //! Item price (chosen amount, no atts)
		int singleItemPrice;
		for (int i = 0; i < amountWanted; i++)
		{

			singleItemPrice = item.CalculatePrice(stock - curRemovedStock, 1.0, true);

			if (ExpansionMath.TestAdditionOverflow(itemPrice, singleItemPrice))
			{
				result = ExpansionMarketResult.IntegerOverflow;
				return false;
			}

			itemPrice += singleItemPrice;

			if (!item.IsStaticStock())
			{
				removedStock.Set(item.ClassName, curRemovedStock + 1);

				curRemovedStock += 1;
			}

			if (includeAttachments && level < 3)
			{
				int magAmmoCount = 0;
				map<string, bool> attachmentTypes = item.GetAttachmentTypes(magAmmoCount);
				map<string, int> magAmmoQuantities = item.GetMagAmmoQuantities(attachmentTypes, magAmmoCount);

				foreach (string attachmentName: item.SpawnAttachments)
				{
					ExpansionMarketItem attachment = ExpansionMarketCategory.GetGlobalItem(attachmentName, false);
					if (attachment)
					{
						int quantity = 1;

						//! If parent item is a mag and we are buying with ammo "attachment", set quantity to ammo quantity
						bool isMagAmmo = attachmentTypes.Get(attachmentName);
						if (isMagAmmo)
						{
							quantity = magAmmoQuantities.Get(attachmentName);
							if (!quantity)
								continue;
						}

						if (!FindPriceOfPurchase(attachment, zone, trader, quantity, price, !isMagAmmo, result, reserved, removedStock, outOfStockList, level + 1))
						{
							switch (result)
							{
								case ExpansionMarketResult.FailedOutOfStock:
									result = ExpansionMarketResult.FailedAttachmentOutOfStock;
									outOfStockList.Insert(attachmentName);
									break;
								case ExpansionMarketResult.IntegerOverflow:
									return false;
							}
						}
					}
				}
			}
		}
				
		MarketModulePrint("FindPriceOfPurchase - " + item.ClassName + " - stock " + (stock - curRemovedStock) + " item price " + itemPrice);

		itemPrice = Math.Round(itemPrice * priceModifier);

		if (ExpansionMath.TestAdditionOverflow(price, itemPrice))
		{
			result = ExpansionMarketResult.IntegerOverflow;
			return false;
		}

		price += itemPrice;

		if (result == ExpansionMarketResult.Success && reserved)
		{
			reserved.AddReserved(zone, item.ClassName, amountWanted, (int) itemPrice);
			removedStock.Set(item.ClassName, 0);
		}

		if (result == ExpansionMarketResult.Success)
			MarketModulePrint("FindPriceOfPurchase - End and return true! price: " + price);
		else
			MarketModulePrint("FindPriceOfPurchase - End and return false! Zone stock is lower then requested amount or item is set to not be buyable!");
		
		return result == ExpansionMarketResult.Success;
	}

	// ------------------------------------------------------------
	// Expansion Array<Object> Spawn
	// ------------------------------------------------------------
	array<Object> Spawn(ExpansionMarketReserve reserve, PlayerBase player, inout EntityAI parent, bool includeAttachments = true, int skinIndex = -1, inout bool attachmentNotAttached = false, TStringIntMap spawnedAmounts = null)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		array< Object > objs = new array<Object>;

		ExpansionMarketItem item = reserve.RootItem;
		if (!item)
		{		
			MarketModulePrint("Spawn - End and return objects: " + objs.ToString() );		
			
			return objs;
		}

		int remainingAmount = reserve.TotalAmount;
		while (remainingAmount > 0)
		{
			int remainingAmountBefore = remainingAmount;

			vector position = "0 0 0";
			vector orientation = "0 0 0";
			if (item.IsVehicle())
			{
				if ( !reserve.Trader.HasVehicleSpawnPosition(item.ClassName, position, orientation) )
				{
					remainingAmount--;
					continue;
				}
			}

			bool result = Spawn(reserve.Trader.GetTraderMarket(), item, player, parent, position, orientation, objs, remainingAmount, includeAttachments, skinIndex, attachmentNotAttached, spawnedAmounts);

			if (!result)
			{
				Error("Error: Couldn't spawn " + item.ClassName);
				break;
			}

			if (remainingAmount == remainingAmountBefore)
			{
				//! Should not be possible, just in case...
				Error("Error: Spawning " + item.ClassName + " did not affect remaining amount!");
				break;
			}
		}		

		string objsInfo;
		foreach (int i, Object obj: objs)
		{
			if (i)
				objsInfo += ", ";
			objsInfo += obj.ToString();
		}	
		
		MarketModulePrint("Spawn - End and return objects: " + objsInfo);
		
		return objs;
	}
	
	//! DEPRECATED
	Object Spawn(ExpansionMarketTrader trader, ExpansionMarketItem item, PlayerBase player, inout EntityAI parent, vector position, vector orientation, out int remainingAmount, bool includeAttachments = true, int skinIndex = -1, int level = 0, inout bool attachmentNotAttached = false, TStringIntMap spawnedAmounts = null)
	{
		EXError.WarnOnce(this, "DEPRECATED");
		array<Object> objs = {};
		Spawn(trader, item, player, parent, position, orientation, objs, remainingAmount, includeAttachments, skinIndex, attachmentNotAttached, spawnedAmounts, level);
		return objs[0];
	}

	/**
	 * @brief spawn market item (including attachments up to three levels)
	 * 
	 * @param trader Trader instance
	 * @param item Market item
	 * @param player Player that is purchasing the item
	 * @param [inout] parent Contains the parent the item object was spawned on when the method returns
	 * @param position
	 * @param orientation
	 * @param objs Array of objects. Spawned objects (main item and potentially attachments) will be inserted.
	 * @param [out] remainingAmount Will be set to the remaining amount of items to spawn when the method returns
	 * @param includeAttachments Whether to spawn attachments (if any) of market item or not
	 * @param skinIndex Skin to use or -1 to ignore
	 * @param [inout] attachmentNotAttached will be set to true if any of the market item attachments could not be spawned attached but in a temporary container instead
	 * @param spawnedAmounts If non-null, each spawned item (including attachments) will have an entry with the spawned amount
	 * @param level Tracks attachment spawn level (internal use only)
	 * 
	 * @return true if success
	 */
	bool Spawn(ExpansionMarketTrader trader, ExpansionMarketItem item, PlayerBase player, inout EntityAI parent, vector position, vector orientation, array<Object> objs, out int remainingAmount, bool includeAttachments = true, int skinIndex = -1, inout bool attachmentNotAttached = false, TStringIntMap spawnedAmounts = null, int level = 0)
	{
		MarketModulePrint("Spawn - Start " + player + " " + parent);

		Object obj;

		int spawnAmount = remainingAmount;

		if (!item.IsVehicle())
			obj = ExpansionItemSpawnHelper.SpawnOnParent( item.ClassName, player, parent, remainingAmount, item.QuantityPercent, NULL, skinIndex, false );
		else
			obj = ExpansionItemSpawnHelper.SpawnVehicle( item.ClassName, player, parent, position, orientation, remainingAmount, NULL, skinIndex, GetExpansionSettings().GetMarket().VehicleKeys.GetRandomElement() );

		if (!obj)
			return false;

		objs.Insert(obj);

		ExpansionMarketReserve reserve = player.GetMarketReserve();
		foreach (ExpansionMarketReserveItem reserved: reserve.Reserved)
		{
			if (!reserved.CreatedObj && reserved.ClassName == item.ClassName)
				reserved.CreatedObj = obj;
		}

		if (spawnedAmounts)
			spawnedAmounts[item.ClassName] = spawnedAmounts[item.ClassName] + spawnAmount - remainingAmount;
		
		//! Now deal with attachments and attachments on attachments
		if (includeAttachments && level < 3)
		{
			EntityAI objEntity = EntityAI.Cast(obj);
			if (objEntity)
			{
				int magAmmoCount = 0;
				map<string, bool> attachmentTypes = item.GetAttachmentTypes(magAmmoCount);

				foreach (string attachmentName: item.SpawnAttachments)
				{
					ExpansionMarketItem attachment = ExpansionMarketCategory.GetGlobalItem(attachmentName);
					if (attachment)
					{
						bool isMagAmmoTmp = attachmentTypes.Get(attachmentName);
						if (isMagAmmoTmp)
							continue;  //! Ammo "attachment" on mag

						//! Everything else
						int attachmentQuantity = 1;
						Spawn(trader, attachment, player, objEntity, position, orientation, objs, attachmentQuantity, true, skinIndex, attachmentNotAttached, spawnedAmounts, level + 1);
					}
				}

				if (objEntity.IsInherited(ExpansionTemporaryOwnedContainer))
				{
					parent = objEntity;
					if (level > 0)
						attachmentNotAttached = true;
				}

				MagazineStorage mag;
				if (Class.CastTo(mag, obj) && magAmmoCount > 0)
				{
					//! Fill up mag. If we have several ammo types, alternate them.
					int totalAmmo;
					int quantityPercent = item.QuantityPercent;
					if (quantityPercent < 0)
						quantityPercent = 100;
					int ammoMax = Math.Ceil(mag.GetAmmoMax() * quantityPercent / 100);
					while (totalAmmo < ammoMax)
					{
						foreach (string ammoName, bool isMagAmmo: attachmentTypes)
						{
							if (isMagAmmo)
							{
								string bulletName = "";
								if (!s_AmmoBullets.Find(ammoName, bulletName))
								{
									bulletName = GetGame().ConfigGetTextOut("CfgMagazines " + ammoName + " ammo");
									s_AmmoBullets.Insert(ammoName, bulletName);
								}

								if (bulletName)
								{
									mag.ServerStoreCartridge(0, bulletName);
								}

								if (spawnedAmounts)
									spawnedAmounts[ammoName] = spawnedAmounts[ammoName] + 1;

								totalAmmo++;
								if (totalAmmo == ammoMax)
									break;
							}
						}
					}

					mag.SetSynchDirty();
				}
			}
		}
		
		return true;
	}

	// ------------------------------------------------------------
	// Expansion MarketMessageGM
	// ------------------------------------------------------------
	void MarketMessageGM(string message)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		GetGame().GetMission().OnEvent(ChatMessageEventTypeID, new ChatMessageEventParams(CCDirect, "", message, ""));
		
	}
	
	// ------------------------------------------------------------
	// Expansion SpawnMoney
	// ------------------------------------------------------------
	array<ItemBase> SpawnMoney(PlayerBase player, inout EntityAI parent, int amount, bool useExistingStacks = true, ExpansionMarketItem marketItem = NULL, ExpansionMarketTrader trader = NULL, bool isATM = false)
	{
		TStringArray currencies;

		if (trader)
			currencies = trader.Currencies;
		else if (isATM)
			currencies = GetExpansionSettings().GetMarket().Currencies;

		return SpawnMoneyInCurrency(player, parent, amount, currencies, useExistingStacks, marketItem);
	}
	
	array<ItemBase> SpawnMoneyInCurrency(PlayerBase player, inout EntityAI parent, int amount, TStringArray currencies, bool useExistingStacks = true, ExpansionMarketItem marketItem = NULL)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		array<ItemBase> monies = new array<ItemBase>;

		array<ref array<ItemBase>> foundMoney;

		if (useExistingStacks)
		{
			//! Will increment existing stacks and only spawn new money when needed
			foundMoney = new array<ref array<ItemBase>>;

			foreach (string moneyName: m_MoneyDenominations)
			{
				foundMoney.Insert(new array<ItemBase>);
			}

			ItemBase existingMoney;

			array<EntityAI> items = new array<EntityAI>;
			items.Reserve(player.GetInventory().CountInventory());

			player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);

			foreach (EntityAI item: items)
			{
				if (Class.CastTo(existingMoney, item) && existingMoney.ExpansionIsMoney())
				{
					string existingType = existingMoney.GetType();
					existingType.ToLower();

					//! Ignore currencies this trader/ATM does not accept
					if (currencies && currencies.Find(existingType) == -1)
						continue;

					int idx = m_MoneyDenominations.Find(existingType);
					MarketModulePrint("SpawnMoney: Found " + existingMoney.GetQuantity() + " " + existingType + " worth " + GetMoneyPrice(existingType) + " a piece on player");
					foundMoney[idx].Insert(existingMoney);
				}
			}
		}
		
		int remainingAmount = amount;
		int lastCurrencyIdx = m_MoneyDenominations.Count() - 1;
		int minAmount = GetMoneyPrice(m_MoneyDenominations[lastCurrencyIdx]);

		for (int currentDenomination = 0; currentDenomination < m_MoneyDenominations.Count(); currentDenomination++)
		{
			string type = m_MoneyDenominations[currentDenomination];

			//! Ignore currencies this trader/ATM does not accept
			if (currencies && currencies.Find(type) == -1)
				continue;

			if (marketItem && GetItemCategory(marketItem).IsExchange && currentDenomination < lastCurrencyIdx && type == marketItem.ClassName)
			{
				//! Apply exchange logic, exclude this currency
				continue;
			}

			int denomPrice = GetMoneyPrice(type);
			int divAmount = remainingAmount / denomPrice;

			int toSpawn = divAmount;

			int amountSpawned = denomPrice * toSpawn;

			if (divAmount < 1)
				continue;

			remainingAmount -= amountSpawned;

			MarketModulePrint("SpawnMoney - need to spawn " + toSpawn + " " + type);

			while (toSpawn > 0)
			{
				ItemBase money = NULL;
				int stack = 0;

				if (useExistingStacks)
				{
					MarketModulePrint("SpawnMoney - check for existing stack of " + type);
					array<ItemBase> existingMonies = foundMoney[currentDenomination];
					for (int i = 0; i < existingMonies.Count(); i++)
					{
						existingMoney = existingMonies[i];
						if (existingMoney.GetQuantity() < existingMoney.GetQuantityMax())
						{
							//! Player already has money of that type that's not full quantity, increase existing stack
							money = existingMoney;
							stack = existingMoney.GetQuantity();
							MarketModulePrint("SpawnMoney - player has " + stack + " " + type);
							existingMonies.Remove(i);
							break;
						}
					}
				}

				if (!money)
				{
					//! Create new money item in player inventory (or in temporary storage if inventory full)
					MarketModulePrint("SpawnMoney - spawning " + type);
					money = ItemBase.Cast(ExpansionItemSpawnHelper.SpawnInInventorySecure(type, player, parent));
				}

				if (money)
				{
					int max = money.GetQuantityMax();

					if (stack + toSpawn <= max)
					{
						money.SetQuantity(stack + toSpawn);

						toSpawn = 0;
					}
					else
					{
						money.SetQuantity(max);

						toSpawn -= max - stack;
					}

					monies.Insert(money);
				} 
				else
				{
					//! Force this loop to end
					toSpawn = 0;
				}
			}

			if (remainingAmount < minAmount)
				break;
		}	
		
		MarketModulePrint("SpawnMoney - money: " + monies.ToString());
		
		return monies;
	}
	
	// From Tyler so CallFunctionParams works
	array<ItemBase> SpawnMoneyEx(PlayerBase player, inout EntityAI parent, int amount, bool useExistingStacks = true, ExpansionMarketItem marketItem = NULL, ExpansionMarketTrader trader = NULL, bool isATM = false)
	{
		return SpawnMoney(player, parent, amount, useExistingStacks, marketItem, trader, isATM);
	}

	// ------------------------------------------------------------
	// Expansion Bool FindMoneyAndCountTypes
	// ------------------------------------------------------------
	//! Return true if total <amount> monies have been found, false otherwise
	//! If player is given, use money in player inventory (if enough), otherwise use amounts that would be needed
	//! Out array <monies> contains needed amounts for each money type to reach total <amount>
	//! Note: Out array <monies> is always ordered from highest to lowest value currency
	bool FindMoneyAndCountTypes(PlayerBase player, int amount, out array<int> monies, bool reserve = false, ExpansionMarketItem marketItem = NULL, ExpansionMarketTrader trader = NULL, bool isATM = false)
	{
		TStringArray currencies;

		if (trader)
			currencies = trader.Currencies;
		else if (isATM)
			currencies = GetExpansionSettings().GetMarket().Currencies;

		return FindMoneyInCurrency(player, amount, monies, currencies, reserve, marketItem);
	}

	bool FindMoneyInCurrency(PlayerBase player, int amount, out array<int> monies, TStringArray currencies, bool reserve = false, ExpansionMarketItem marketItem = NULL)
	{	
		MarketModulePrint("FindMoneyAndCountTypes - player " + player + " - amount " + amount);

		if (amount < 0)
		{
			MarketModulePrint("FindMoneyAndCountTypes - amount is not a positive value - end and return false!");
			return false;
		}

		if (!monies)
			monies = new array<int>;
		
		if (amount == 0)
			return true;

		array<ref array<ItemBase>> foundMoney = new array<ref array<ItemBase>>;
		array<int> playerMonies = new array<int>;
		
		foreach (string moneyName: m_MoneyDenominations)
		{
			MarketModulePrint("FindMoneyAndCountTypes: " + moneyName);
			foundMoney.Insert(new array<ItemBase>);
			monies.Insert(0);
			playerMonies.Insert(0);
		}

		ItemBase money;
		int playerWorth;

	#ifdef SERVER
		auto settings = GetExpansionSettings().GetMarket();
	#endif
		TStringArray disallowedMoney = {};

		if (player)
		{
			array<EntityAI> items = new array<EntityAI>;
			items.Reserve(player.GetInventory().CountInventory());

			player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);

			foreach (EntityAI item: items)
			{
				if (Class.CastTo(money, item) && money.ExpansionIsMoney())
				{
					if (money.IsSetForDeletion())
						continue;

					//! Make sure we don't use items as money that should not be included, like attachments (e.g. Dogtags that are attached to the player itself) or that are in nested containers (1.25 change).
					if (!MiscGameplayFunctions.Expansion_IsLooseEntity(money))
						continue;

				#ifdef SERVER
					if (settings.DisallowUnpersisted && !ExpansionWorld.IsStoreLoaded(money) && !ExpansionWorld.IsStoreSaved(money))
					{
						//! Only allow to use money items that have been persisted
						disallowedMoney.Insert(money.GetDisplayName());
						continue;
					}
				#endif

					string type = money.GetType();
					type.ToLower();

					//! Ignore currencies this trader/ATM does not accept
					if (currencies && currencies.Find(type) == -1)
						continue;

					int idx = m_MoneyDenominations.Find(type);
					MarketModulePrint("FindMoneyAndCountTypes: Found " + money.GetQuantity() + " " + type + " worth " + GetMoneyPrice(type) + " a piece on player ");
					foundMoney[idx].Insert(money);
					playerMonies[idx] = playerMonies[idx] + money.GetQuantity();
					playerWorth += GetMoneyPrice(type) * money.GetQuantity();
				}
			}
		}

		int foundAmount = 0;
		int lastCurrencyIdx = m_MoneyDenominations.Count() - 1;
		int minAmount = GetMoneyPrice(m_MoneyDenominations[lastCurrencyIdx]);
		int remainingAmount = amount;

		string info = "Would reserve";
		if (reserve)
			info = "Reserved";

		for (int j = 0; j < foundMoney.Count(); j++)
		{
			//! Ignore currencies this trader/ATM does not accept
			if (!player && currencies && currencies.Find(m_MoneyDenominations[j]) == -1)
				continue;

			if (marketItem && GetItemCategory(marketItem).IsExchange && j < lastCurrencyIdx && m_MoneyDenominations[j] == marketItem.ClassName)
			{
				//! Apply exchange logic, exclude this currency
				continue;
			}

			int denomPrice = GetMoneyPrice(m_MoneyDenominations[j]);
			int divAmount = remainingAmount / denomPrice;
			int toReserve = divAmount;
			int reserved = 0;

			int countCurrentDenom = foundMoney[j].Count();
			int checkedCurrentDenom = 0;

			while (toReserve > 0)
			{
				if (checkedCurrentDenom >= countCurrentDenom && player)
					break;

				int number = 0;

				if (!player || playerWorth < amount)
				{
					number = toReserve;
				}
				else
				{
					money = foundMoney[j][checkedCurrentDenom];
					int stack = money.GetQuantity();

					if (stack >= toReserve)
					{
						number = toReserve;
					} 
					else
					{
						number = stack;
					}

					if (reserve)
						money.ExpansionReserveMoney(number);
				}

				monies[j] = monies[j] + number;

				MarketModulePrint("FindMoneyAndCountTypes: " + info + " " + number + " " + m_MoneyDenominations[j] + " worth " + (number * denomPrice));

				toReserve -= number;
				reserved += number;
				checkedCurrentDenom++;
			}

			int amountReserve = reserved * denomPrice;
			foundAmount += amountReserve;
			remainingAmount -= amountReserve;

			if (foundAmount > amount - minAmount)
			{
				return CheckMoney(playerWorth, amount, disallowedMoney, player);
			}
		}

		//! Failed to reserve exact amounts needed from each currency, but player may have actually more than needed overall
		//! Find lowest value currency that would push us to or over the required amount
		for (int i = foundMoney.Count() - 1; i >= 0; i--)
		{
			//! Ignore currencies this trader/ATM does not accept
			if (!player && currencies && currencies.Find(m_MoneyDenominations[i]) == -1)
				continue;

			if (marketItem && GetItemCategory(marketItem).IsExchange && i < lastCurrencyIdx && m_MoneyDenominations[i] == marketItem.ClassName)
			{
				//! Apply exchange logic, exclude this currency
				continue;
			}

			denomPrice = GetMoneyPrice(m_MoneyDenominations[i]);
			if ((!player || monies[i] < playerMonies[i]) && denomPrice >= remainingAmount)
			{
				if (reserve)
				{
					foreach (ItemBase existingMoney : foundMoney[i])
					{
						if (existingMoney.ExpansionGetReservedMoneyAmount() < existingMoney.GetQuantity())
							existingMoney.ExpansionReserveMoney(existingMoney.ExpansionGetReservedMoneyAmount() + 1);
					}
				}
				
				monies[i] = monies[i] + 1;
				MarketModulePrint("FindMoneyAndCountTypes: " + info + " one additional " + m_MoneyDenominations[i] + " worth " + denomPrice);
				foundAmount += denomPrice;
				remainingAmount = amount - foundAmount;
				
				for (int k = i + 1; k < foundMoney.Count(); k++)
				{
					denomPrice = GetMoneyPrice(m_MoneyDenominations[k]);
					divAmount = remainingAmount / denomPrice;
					toReserve = divAmount;
					if (remainingAmount % denomPrice > 0)
						toReserve++;
					monies[k] = toReserve;
					remainingAmount -= toReserve * denomPrice;
				}
				
				return CheckMoney(playerWorth, amount, disallowedMoney, player);
			}
			else
			{
				if (reserve)
				{
					foreach (ItemBase existingReserved : foundMoney[i])
					{
						if (existingReserved.ExpansionIsMoneyReserved())
							existingReserved.ExpansionReserveMoney(0);
					}
				}
				foundAmount -= monies[i] * denomPrice;
			}
		}

		CheckMoney(playerWorth, amount, disallowedMoney, player);

		MarketModulePrint("FindMoneyAndCountTypes - not enough money found - end and return false!");
		return false;
	}

	bool CheckMoney(int playerWorth, int amount, TStringArray disallowedMoney, PlayerBase player)
	{
		if (playerWorth >= amount)
			return true;

		if (disallowedMoney.Count() > 0 && player)
			ExpansionNotification("STR_EXPANSION_MARKET_TITLE", string.Format("You can't use %1 right now because it hasn't been persisted to game storage yet. Please wait at least 15-20 seconds.", ExpansionString.JoinStrings(disallowedMoney)), EXPANSION_NOTIFICATION_ICON_TRADER, COLOR_EXPANSION_NOTIFICATION_ERROR, 7, ExpansionNotificationType.MARKET).Error(player.GetIdentity());

		return false;
	}

	// Tyler requested, <3
	bool FindMoneyAndCountTypesEx(PlayerBase player, int amount, out array<int> monies, bool reserve = false, ExpansionMarketItem marketItem = NULL, ExpansionMarketTrader trader = NULL, bool isATM = false)
	{
		return FindMoneyAndCountTypes(player, amount, monies, reserve, marketItem, trader, isATM);
	}
		
	// ------------------------------------------------------------
	// Expansion Int GetPlayerWorth
	// ------------------------------------------------------------
	int GetPlayerWorth(PlayerBase player, out array<int> monies, ExpansionMarketTrader trader = NULL, bool isATM = false)
	{
		TStringArray currencies;

		if (trader)
			currencies = trader.Currencies;
		else if (isATM)
			currencies = GetExpansionSettings().GetMarket().Currencies;

		return GetPlayerWorth(player, monies, currencies);
	}

	int GetPlayerWorth(PlayerBase player, out array<int> monies, TStringArray currencies)
	{
		m_PlayerWorth = 0;

		if (!monies)
		{
			monies = new array<int>;
		}
		else
		{
			monies.Clear();
		}

		for (int i = 0; i < m_MoneyDenominations.Count(); i++)
		{
			monies.Insert(0);
		}

		if (!player)
		{
			return m_PlayerWorth;
		}

		array<EntityAI> items = new array<EntityAI>;
		items.Reserve(player.GetInventory().CountInventory());

	   	player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);

		for (int j = 0; j < items.Count(); j++)
		{
			ItemBase money;
			if (Class.CastTo(money, items[j]) && money.ExpansionIsMoney())
			{
				//! Make sure we don't use items as money that should not be included, like attachments (e.g. Dogtags that are attached to the player itself) or that are in nested containers (1.25 change).
				if (!MiscGameplayFunctions.Expansion_IsLooseEntity(money))
					continue;

				string type = money.GetType();
				type.ToLower();
				
				//! Always include all money types the player has, even if trader/ATM would not accept
				int idx = m_MoneyDenominations.Find(type);
				monies[idx] = monies[idx] + money.GetQuantity();

				//! Do not include currencies this trader/ATM does not accept in player overall worth calc
				if (currencies && currencies.Find(type) == -1)
					continue;

				m_PlayerWorth += GetMoneyPrice(type) * money.GetQuantity();
			}
		}

		return m_PlayerWorth;
	}
	
	// ------------------------------------------------------------
	// Expansion Int GetPlayerWorth
	// ------------------------------------------------------------
	int GetPlayerWorth()
	{
		return m_PlayerWorth;
	}
	
	static string GetDisplayPrice(ExpansionMarketTrader trader, int price, bool shorten = false, bool format = true, bool includeDisplayCurrencyName = false)
	{
		return GetDisplayPriceEx(price, shorten, format, includeDisplayCurrencyName, trader.DisplayCurrencyValue, trader.m_DisplayCurrencyPrecision, trader.DisplayCurrencyName);
	}

	static string GetDisplayPriceEx(int price, bool shorten = false, bool format = true, bool includeDisplayCurrencyName = false, int displaycurrencyValue = 1, int displayCurrencyPrecision = 2, string displayCurrencyName = string.Empty)
	{
		string priceString;

		if (displaycurrencyValue <= 1)
		{
			if (format)
				priceString = ExpansionStatic.FormatInt(price, shorten);
			else
				priceString = price.ToString();
		}
		else
		{
			float displayPrice = price / displaycurrencyValue;

			priceString = ExpansionStatic.FormatFloat(displayPrice, displayCurrencyPrecision, format);
		}

		if (includeDisplayCurrencyName && displayCurrencyName)
			priceString += " " + displayCurrencyName;

		return priceString;
	}

	// ------------------------------------------------------------
	// Expansion GetMoneyBases
	// ------------------------------------------------------------
	array<ref array<ItemBase>> GetMoneyBases(PlayerBase player)
	{
		array<ref array<ItemBase>> foundMoney = new array<ref array<ItemBase>>;
		
		for (int i = 0; i < m_MoneyDenominations.Count(); i++)
		{
			array<ItemBase> money_array = new array<ItemBase>;
			foundMoney.Insert(money_array);
		}
		
		array<EntityAI> items = new array<EntityAI>;
		items.Reserve(player.GetInventory().CountInventory());

	   	player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);

		for (int j = 0; j < items.Count(); j++)
		{
			ItemBase money;
			if (Class.CastTo(money, items[j]) && money.ExpansionIsMoney())
			{
				string type = money.GetType();
				type.ToLower();

				int idx = m_MoneyDenominations.Find(type);
				MarketModulePrint("GetMoneyBases - idx: " + idx);
				MarketModulePrint("GetMoneyBases - foundMoney[idx]: " + foundMoney[idx]);
				foundMoney[idx].Insert(money);
			}
		}
		
		return foundMoney;
	}
				
	// ------------------------------------------------------------
	// Expansion UnlockMoney
	// ------------------------------------------------------------
	void UnlockMoney(PlayerBase player)
	{
		if (!player)
		{
			return;
		}

		array<EntityAI> items = new array<EntityAI>;
		items.Reserve(player.GetInventory().CountInventory());

	   	player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);

		for (int i = 0; i < items.Count(); i++)
		{
			ItemBase money;
			if (Class.CastTo(money, items[i]) && money.ExpansionIsMoneyReserved())
			{
				money.ExpansionReserveMoney(0);
			}
		}
	}
	
	// ------------------------------------------------------------
	// Expansion RemoveMoney
	// ------------------------------------------------------------
	//! Remove reserved money amounts from player, return removed price
	//! If limitAmount is given and greater than -1, remove up to but not more than limitAmount
	int RemoveMoney(PlayerBase player, int limitAmount = -1)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		if (!player)
		{
			return 0;
		}

		int removed;

		array<EntityAI> items = new array<EntityAI>;
		items.Reserve(player.GetInventory().CountInventory());

	   	player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);

		foreach (EntityAI item : items)
		{
			ItemBase money;
			if (Class.CastTo(money, item) && money.ExpansionIsMoneyReserved())
			{
				int removeAmount = money.ExpansionGetReservedMoneyAmount();
				if (limitAmount > -1)
				{
					if (removeAmount > limitAmount)
						removeAmount = limitAmount;
				}
				int quantity = money.GetQuantity() - removeAmount;
				if (removeAmount)
				{
					string type = money.GetType();
					type.ToLower();
					removed += removeAmount * GetMoneyPrice(type);
					MarketModulePrint("RemoveMoney - Removed " + removeAmount + " from " + money);
				}
				if (!quantity)
				{
					GetGame().ObjectDelete(money);
				}
				else
				{
					money.ExpansionReserveMoney(0);
					if (removeAmount)
						money.SetQuantity(quantity);
				}
				if (limitAmount > -1)
					limitAmount -= removeAmount;
			}
		}

		return removed;
	}

	/*
	 * Called Server Only: 
	*/
	// ------------------------------------------------------------
	// Expansion RemoveReservedStock
	// ------------------------------------------------------------
	private void RemoveReservedStock(PlayerBase player, string itemClassName)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		if (GetGame().IsServer() || !GetGame().IsMultiplayer())
		{
			if (!player)
				return;
	
			ExpansionMarketReserve reserve = player.GetMarketReserve();
			if (reserve)
			{
				if (player.IsMarketItemReserved(itemClassName))
				{
					EXPrint("RemoveReservedStock: FailedReserveTime");
					
					UnlockMoney(player);
					
					reserve.ClearReserved(reserve.Trader.GetTraderZone());
		
					Callback(reserve.RootItem.ClassName, ExpansionMarketResult.FailedReserveTime, player.GetIdentity());
		
					player.ClearMarketReserve();
				}
			}
		}
	}
	
	/*
	 * Called Server Only: 
	*/
	// ------------------------------------------------------------
	// Expansion Callback
	// ------------------------------------------------------------
	void Callback(string itemClassName, ExpansionMarketResult result, PlayerIdentity playerIdent, int option1 = -1, int option2 = -1, Object object = NULL)
	{
		auto rpc = Expansion_CreateRPC("RPC_Callback");
		rpc.Write(itemClassName);
		rpc.Write(result);
		rpc.Write(option1);
		rpc.Write(option2);
		rpc.Write(object);
		rpc.Expansion_Send(true, playerIdent);
	}
	
	// ------------------------------------------------------------
	// Expansion RPC_Callback
	// ------------------------------------------------------------
	private void RPC_Callback(PlayerIdentity senderRPC, Object target, ParamsReadContext ctx)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		string itemClassName;
		if (!ctx.Read(itemClassName))
		{
			Error("RPC_Callback - Could not read param string itemClassName!");
			return;
		}

		int result;
		if (!ctx.Read(result))
		{
			Error("RPC_Callback - Could not read param int result!");
			return;
		}

		int option1;
		if (!ctx.Read(option1))
		{
			Error("RPC_Callback - Could not read param int option1!");
			return;
		}

		int option2;
		if (!ctx.Read(option2))
		{
			Error("RPC_Callback - Could not read param int option2!");
			return;
		}

		Object object;
		if (!ctx.Read(object))
		{
			Error("RPC_Callback - Could not read param Object object!");
			return;
		}

		MarketModulePrint("RPC_Callback - result: " + result + " option1: " + option1 + " option2: " + option2 + " object: " + object);
		
		SI_Callback.Invoke(itemClassName, result, option1, option2, object);
	}

#ifdef SERVER
	// -----------------------------------------------------------
	// Expansion OnInvokeConnect
	// -----------------------------------------------------------
	override void OnInvokeConnect(Class sender, CF_EventArgs args)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		super.OnInvokeConnect(sender, args);

		auto cArgs = CF_EventPlayerArgs.Cast(args);

		auto settings = GetExpansionSettings().GetMarket();
		if (!settings.MarketSystemEnabled && !settings.ATMSystemEnabled)
			return;
		
		//! If this is a respawn, need to do nothing of the below
		if (SyncEvents.s_Expansion_RespawningUIDs[cArgs.Identity.GetId()])
			return;

		SendMoneyDenominations(cArgs.Identity);
		
		if (!GetPlayerATMData(cArgs.Identity.GetId()))
		{
			CreateATMData(cArgs.Identity);
		}
	}
#endif
	
	// -----------------------------------------------------------
	// Expansion SendMoneyDenominations
	// -----------------------------------------------------------
	private void SendMoneyDenominations(PlayerIdentity identity)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		if (GetGame().IsServer() || !GetGame().IsMultiplayer())
		{
			if (!MoneyCheck(identity))
				return;

			auto rpc = Expansion_CreateRPC("RPC_MoneyDenominations");

			//! Order needs to match highest to lowest currency value
			rpc.Write(m_MoneyDenominations);
			array < int > values = new array< int >;
			foreach ( string moneyName: m_MoneyDenominations )
			{
				values.Insert(GetMoneyPrice(moneyName));
			}
			rpc.Write(values);

			rpc.Expansion_Send(true, identity);
		}
	}
	
	// -----------------------------------------------------------
	// Expansion RPC_MoneyDenominations
	// -----------------------------------------------------------
	private void RPC_MoneyDenominations(PlayerIdentity senderRPC, Object target, ParamsReadContext ctx)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		array<string> keys = new array<string>;
		array<int> values = new array<int>;

		if (!ctx.Read(keys) || !ctx.Read(values))
			return;

		int count = keys.Count();
		if (count != values.Count())
			return;

		m_MoneyTypes.Clear();
		m_MoneyDenominations.Clear();

		for (int i = 0; i < count; i++)
		{
			m_MoneyDenominations.Insert(keys[i]);
			m_MoneyTypes.Insert(keys[i], values[i]);
		}
	}

	/* TODO: GET MORE INFO FROM JACOB ABOUT THIS!
	 * Called Client Only: The client would send this RPC to request a purchase 
	 * to be made and to lock in the price it is at. This lock will last
	 * 30 seconds and on any new clients will show the new price as if
	 * the stock of those items were released.
	 */
	// ------------------------------------------------------------
	// Expansion RequestPurchase
	// Client only
	// ------------------------------------------------------------
	void RequestPurchase(string itemClassName, int count, int currentPrice, ExpansionTraderObjectBase trader, PlayerBase player = NULL, bool includeAttachments = true, int skinIndex = -1, TIntArray attachmentIDs = NULL)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		if (!GetGame().IsDedicatedServer())
		{
			if (!trader)
				return;

			auto rpc = Expansion_CreateRPC("RPC_RequestPurchase");
			rpc.Write(itemClassName);
			rpc.Write(count);
			rpc.Write(currentPrice);
			rpc.Write(includeAttachments);
			rpc.Write(skinIndex);
			rpc.Write(attachmentIDs);
			rpc.Expansion_Send(trader.GetTraderEntity(), true);
		}
	}
	
	// ------------------------------------------------------------
	// Expansion RPC_RequestPurchase
	// Server only
	// ------------------------------------------------------------
	private void RPC_RequestPurchase(PlayerIdentity senderRPC, Object target, ParamsReadContext ctx)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		if (!GetExpansionSettings().GetMarket().MarketSystemEnabled)
			return;

		string itemClassName;
		if (!ctx.Read(itemClassName))
			return;

		int count;
		if (!ctx.Read(count))
			return;
			
		int currentPrice;
		if (!ctx.Read(currentPrice))
			return;

		bool includeAttachments;
		if (!ctx.Read(includeAttachments))
			return;
		
		int skinIndex;
		if (!ctx.Read(skinIndex))
			return;

		TIntArray attachmentIDs;
		if (!ctx.Read(attachmentIDs))
			return;

		ExpansionTraderObjectBase trader = GetTraderFromObject(target);
		if (!trader)
			return;
			
		PlayerBase player = PlayerBase.GetPlayerByUID(senderRPC.GetId());
		if (!player)
			return;
			
		if (!CheckCanUseTrader(player, trader))
			return;

		Exec_RequestPurchase(player, itemClassName, count, currentPrice, trader, includeAttachments, skinIndex, attachmentIDs);
	}
	
	static ExpansionTraderObjectBase GetTraderFromObject(Object obj, bool errorOnNoTrader = true)
	{
#ifdef EXTRACE
		EXTrace trace = EXTrace.Profile(EXTrace.MARKET, ExpansionMarketModule);
#endif

		ExpansionTraderNPCBase traderNPC;
		ExpansionTraderStaticBase traderStatic;
		ExpansionTraderZombieBase traderZombie;
		#ifdef ENFUSION_AI_PROJECT
		ExpansionTraderAIBase traderAI;
		#endif

		ExpansionTraderObjectBase trader;
		if (Class.CastTo(traderNPC, obj))
			trader = traderNPC.GetTraderObject();
		else if (Class.CastTo(traderStatic, obj))
			trader = traderStatic.GetTraderObject();
		else if (Class.CastTo(traderZombie, obj))
			trader = traderZombie.GetTraderObject();
		#ifdef ENFUSION_AI_PROJECT
		else if (Class.CastTo(traderAI, obj))
			trader = traderAI.GetTraderObject();
		#endif

#ifdef EXTRACE
		if (trace && trader && errorOnNoTrader)
		{
			trace.SetSilent(false);
			trace.InitialDump(0);
			trace.Add(trader);
		}
#endif
		
		if (!obj)
		{
			//! It shouldn't be possible for this to happen, but just to be safe...
			Error("ExpansionMarketModule::GetTraderFromObject - entity is NULL!");
		}
		else if (!trader && errorOnNoTrader)
		{
			Error("ExpansionMarketModule::GetTraderFromObject - trader object of " + obj.GetType() + " (" + obj + ") at " + obj.GetPosition() + " is NULL!");
		}

		return trader;
	}
	
	// ------------------------------------------------------------
	// Expansion Exec_RequestPurchase
	//	Server only
	// ------------------------------------------------------------
	private void Exec_RequestPurchase(notnull PlayerBase player, string itemClassName, int count, int currentPrice, ExpansionTraderObjectBase trader, bool includeAttachments = true, int skinIndex = -1, TIntArray attachmentIDs = NULL)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		if (!player)
		{
			return;
		}
		
		if (!count)
		{
			MarketModulePrint("Exec_RequestPurchase - Callback_FailedNoCount");
			
			Callback(itemClassName, ExpansionMarketResult.FailedNoCount, player.GetIdentity());
			return;
		}
		
		if (!trader)
		{
			MarketModulePrint("Exec_RequestPurchase - Callback_FailedUnknown: 1");
			
			Callback(itemClassName, ExpansionMarketResult.FailedUnknown, player.GetIdentity());
			return;
		}
		
		MarketModulePrint("Exec_RequestPurchase - count: " + count);
		MarketModulePrint("Exec_RequestPurchase - currentPrice: " + string.ToString(currentPrice));
		
		//! Get current market Trader Zone from given trader
		ExpansionMarketTraderZone zone = trader.GetTraderZone();
		if (!zone)
		{			
			MarketModulePrint("Exec_RequestPurchase - Callback_FailedUnknown: 3");
			
			Callback(itemClassName, ExpansionMarketResult.FailedUnknown, player.GetIdentity());
			return;
		}

		//! Afterwards calculate the price of the items at that stock		
		ExpansionMarketReserve reservedList = player.GetMarketReserve();		
		reservedList.Trader = trader;
		
		MarketModulePrint("Exec_RequestPurchase - reservedList: " + reservedList);
		MarketModulePrint("Exec_RequestPurchase - reservedList.Trader: " + reservedList.Trader);
		
		ExpansionMarketItem item = ExpansionMarketCategory.GetGlobalItem(itemClassName);
		if (!item /* || !reservedList.Valid*/)
		{		
			MarketModulePrint("Exec_RequestPurchase - Callback_FailedUnknown: 4 itemClassName " + itemClassName);
			MarketModulePrint("Exec_RequestPurchase - Callback_FailedUnknown: 4");		

			Callback(itemClassName, ExpansionMarketResult.FailedUnknown, player.GetIdentity());
			return;
		}

		//! If custom attachments are chosen, create a derivative of the market item with them and cache it
		if (attachmentIDs && attachmentIDs.Count())
		{
			ExpansionMarketItem derivative = new ExpansionMarketItem(item.CategoryID, itemClassName, item.MinPriceThreshold, item.MaxPriceThreshold, item.MinStockThreshold, item.MaxStockThreshold, NULL, item.Variants, item.SellPricePercent, item.QuantityPercent, item.ItemID, attachmentIDs);
			derivative.SetAttachmentsFromIDs();
			item = derivative;
		}

		ExpansionMarketResult result;

		//! Compare that price to the one the player sent
		if (!FindPurchasePriceAndReserve(item, count, reservedList, includeAttachments, result) || reservedList.Price != currentPrice)
		{
			EXPrint("Exec_RequestPurchase - Player sent price: " + currentPrice);
			EXPrint("Exec_RequestPurchase - Current stock: " + zone.GetStock(itemClassName, true));
			#ifdef EXPANSIONMODMARKET_DEBUG
			reservedList.Debug();
			#endif

			if (result == ExpansionMarketResult.Success)
			{
				if (reservedList.Price != currentPrice)
				{
					//! We don't know where this difference of exactly one sometimes comes from.
					//! Rounding differences between server and client? EnfuckScript?
					//! We just ignore it and fixup the price
					if (Math.AbsInt(reservedList.Price - currentPrice) == 1)
					{
						//! When buying from trader, match the player sent price if it's higher
						if (reservedList.Price < currentPrice)
							reservedList.Price = currentPrice;

						EXPrint("Fixed purchase price to " + reservedList.Price);
					}
					else
					{
						//! Result if the price the player has seen and agreed to in menu doesn't match anymore
						//! the current item price of the trader because stock has changed enough to affect it
						//! (another player was quicker to get his transaction through)
						result = ExpansionMarketResult.FailedStockChange;
					}
				}
				else
				{
					result = ExpansionMarketResult.FailedUnknown;
				}
			}
		}

		if (result != ExpansionMarketResult.Success)
		{
			reservedList.ClearReserved(zone);
			player.ClearMarketReserve();

			EXPrint("Callback " + typename.EnumToString(ExpansionMarketResult, result));

			Callback(itemClassName, result, player.GetIdentity(), count, (int) includeAttachments);
			
			string cbtype = typename.EnumToString(ExpansionMarketResult, result);

			MarketModulePrint("Exec_RequestPurchase - Callback " + cbtype + " Item: " + reservedList.RootItem);
			MarketModulePrint("Exec_RequestPurchase - Callback " + cbtype + " Zone: " + zone.ToString());
			MarketModulePrint("Exec_RequestPurchase - Callback " + cbtype + " Count: " + count);
			MarketModulePrint("Exec_RequestPurchase - Callback " + cbtype + ": 1");
			MarketModulePrint("Exec_RequestPurchase - Callback " + cbtype + ": itemClassName " + itemClassName);
			MarketModulePrint("Exec_RequestPurchase - Callback " + cbtype + ": player " + player);
			MarketModulePrint("Exec_RequestPurchase - Callback " + cbtype + ": identity " + player.GetIdentity());

			return;
		}
		
		UnlockMoney(player);

		array<int> monies;
		if (!FindMoneyAndCountTypes(player, reservedList.Price, monies, true, reservedList.RootItem, trader.GetTraderMarket())) // currentPrice -> reservedList.Price
		{
			UnlockMoney( player );

			reservedList.ClearReserved(zone);
			player.ClearMarketReserve();

			Callback(itemClassName, ExpansionMarketResult.FailedNotEnoughMoney, player.GetIdentity());
					
			MarketModulePrint("Callback_FailedNotEnoughMoney: 1");				
			MarketModulePrint("Callback_FailedNotEnoughMoney: player: " + player);	
			MarketModulePrint("Callback_FailedNotEnoughMoney: current price: " + string.ToString(currentPrice));
			MarketModulePrint("Callback_FailedNotEnoughMoney: reserved list price: " + string.ToString(reservedList.Price));

			return;
		}

		reservedList.Valid = true;
		reservedList.Time = GetGame().GetTime();

		#ifdef EXPANSIONMODMARKET_DEBUG
		reservedList.Debug();
		#endif

		// !TODO: Finish method RemoveReservedStock in PlayerBase
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(RemoveReservedStock, 30000, false, player, itemClassName);

		Exec_ConfirmPurchase(player, itemClassName, includeAttachments, skinIndex);
	}
	
	// ------------------------------------------------------------
	// Expansion Exec_ConfirmPurchase
	// ------------------------------------------------------------
	private void Exec_ConfirmPurchase(notnull PlayerBase player, string itemClassName, bool includeAttachments = true, int skinIndex = -1)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		ExpansionMarketReserve reserve = player.GetMarketReserve();
		if (!reserve)
		{
			MarketModulePrint("Exec_ConfirmPurchase - Callback_FailedUnknown: Could not get reserved data from player!");
			Callback(itemClassName, ExpansionMarketResult.FailedUnknown, player.GetIdentity());
			return;
		}
		
		if (!reserve.Trader || !reserve.Trader.GetTraderEntity())
		{
			MarketModulePrint("Exec_ConfirmPurchase - Callback_FailedUnknown: Could not get trader data from reserved data!");
			Callback(itemClassName, ExpansionMarketResult.FailedUnknown, player.GetIdentity());
			return;
		}
		
		ExpansionMarketTraderZone zone = reserve.Trader.GetTraderZone();
		if (!zone)
		{
			MarketModulePrint("Exec_ConfirmPurchase - Callback_FailedUnknown: Could not get trader zone data from reserved trader data!");
			Callback(itemClassName, ExpansionMarketResult.FailedUnknown, player.GetIdentity());
			return;
		}
		
		if (vector.Distance(player.GetPosition(), reserve.Trader.GetTraderEntity().GetPosition()) > MAX_TRADER_INTERACTION_DISTANCE)
		{
			MarketModulePrint("Exec_ConfirmPurchase - Callback_FailedTooFarAway: Player is too far from trader!");

			ClearReserved(player, true);

			Callback(itemClassName, ExpansionMarketResult.FailedTooFarAway, player.GetIdentity());
			return;
		}

		if (!player.IsMarketItemReserved(itemClassName))
		{
			EXPrint("Exec_ConfirmPurchase - Callback_FailedReserveTime: Could not get reserved item data from player!");

			ClearReserved(player, true);

			Callback(itemClassName, ExpansionMarketResult.FailedReserveTime, player.GetIdentity());
			return;
		}

		ExpansionMarketItem item = reserve.RootItem;
		if (!item)
		{
			MarketModulePrint("Exec_ConfirmPurchase - Callback_FailedUnknown: Could not get item data from market!");

			ClearReserved(player, true);

			Callback(itemClassName, ExpansionMarketResult.FailedUnknown, player.GetIdentity());
			return;
		}

		vector spawnPos;
		vector spawnDir;
		ExpansionVehicleType spawnType;
		ExpansionMarketResult result;
		Object blockingObject;
		if (item.IsVehicle() && !reserve.Trader.HasVehicleSpawnPosition(itemClassName, spawnPos, spawnDir, spawnType, result, blockingObject, reserve.TotalAmount))
		{
			MarketModulePrint("Exec_ConfirmPurchase - HasVehicleSpawnPosition - Type: " + typename.EnumToString(ExpansionVehicleType, spawnType));
			MarketModulePrint("Exec_ConfirmPurchase - HasVehicleSpawnPosition - Callback: " + typename.EnumToString(ExpansionMarketResult, result));

			ClearReserved(player, true);

			Callback(itemClassName, result, player.GetIdentity(), spawnType, -1, blockingObject);
			return;
		}

		EntityAI parent = player;
		
		//! Filled by spawn function:
		//! objs - spawned top-level objects
		//! spawnedAmounts - classnames + amounts of ALL spawned objects (top level + hierarchy of attachments)
		array<Object> objs = new array<Object>;
		bool attachmentNotAttached;
		TStringIntMap spawnedAmounts = new TStringIntMap;
		objs = Spawn(reserve, player, parent, includeAttachments, skinIndex, attachmentNotAttached, spawnedAmounts);
		
		MarketModulePrint("Exec_ConfirmPurchase " + reserve.RootItem.ClassName + " " + reserve.TotalAmount + " " + reserve.Reserved.Count());
		
		if (objs.Count())
		{
			TStringArray itemIDs = {};
			itemIDs.Reserve(objs.Count());
			
			foreach (ExpansionMarketReserveItem currentReservedItem: reserve.Reserved)
			{
				int spawnedAmount = spawnedAmounts[currentReservedItem.ClassName];
				if (spawnedAmount < currentReservedItem.Amount)
				{
					if (spawnedAmount)
						zone.RemoveStock(currentReservedItem.ClassName, spawnedAmount, false);
					spawnedAmounts.Remove(currentReservedItem.ClassName);
					reserve.Price -= currentReservedItem.Price / currentReservedItem.Amount * (currentReservedItem.Amount - spawnedAmount);
				}
				else
				{
					zone.RemoveStock(currentReservedItem.ClassName, currentReservedItem.Amount, false);
					spawnedAmounts[currentReservedItem.ClassName] = spawnedAmount - currentReservedItem.Amount;
				}

				string itemID;
				if (currentReservedItem.ClassName == itemClassName)
					itemID = ExpansionStatic.GetInstanceID(currentReservedItem.CreatedObj);
				else if (!currentReservedItem.CreatedObj)  //! Ammo in mags/ammopiles are not entities
					itemID = string.Format("%1 x%2", currentReservedItem.ClassName, currentReservedItem.Amount);
				else
					itemID = currentReservedItem.CreatedObj.ToString();
				itemIDs.Insert(itemID);
			}

			int removed = RemoveMoney(player, reserve.Price);

			MarketModulePrint("Exec_ConfirmPurchase - Total money removed: " + removed);
			MarketModulePrint("Exec_ConfirmPurchase - Change owed to player: " + (removed - reserve.Price));

			if (removed - reserve.Price > 0)
			{
				SpawnMoney(player, parent, removed - reserve.Price, true, NULL, reserve.Trader.GetTraderMarket());
			}
		
			CheckSpawn(player, parent, attachmentNotAttached);

			string itemsDetail = string.Format("x%1 (%2)", reserve.TotalAmount, ExpansionString.JoinStrings(itemIDs, ", ", true));

			ExpansionLogMarket(string.Format("Player \"%1\" (id=%2) has bought %3 %4 from the trader \"%5 (%6)\" in market zone \"%7\" (pos=%8) for a price of %9.", player.GetIdentity().GetName(), player.GetIdentity().GetId(), reserve.RootItem.ClassName, itemsDetail, reserve.Trader.GetTraderMarket().m_FileName, reserve.Trader.GetDisplayName(), reserve.Trader.GetTraderZone().m_DisplayName, reserve.Trader.GetTraderZone().Position.ToString(), reserve.Price));	
			
			Callback(itemClassName, ExpansionMarketResult.PurchaseSuccess, player.GetIdentity(), reserve.TotalAmount, reserve.Price);
		}
		else
		{
			Callback(itemClassName, ExpansionMarketResult.FailedItemSpawn, player.GetIdentity());
		}
		
		//! Need to clear reserved after a purchase
		ClearReserved(player);
		
		if (objs.Count())
			zone.Save();
	}

	void ClearReserved(PlayerBase player, bool unlockMoney = false)
	{
		if (unlockMoney)
			UnlockMoney(player);

		ExpansionMarketReserve reserve = player.GetMarketReserve();
		ExpansionMarketTraderZone zone = reserve.Trader.GetTraderZone();

		reserve.ClearReserved(zone);
		player.ClearMarketReserve();
	}

	protected void ClearTmpNetworkCaches()
	{
		m_TmpVariantIds.Clear();
		m_TmpVariantIdIdx = 0;
		m_TmpNetworkCats.Clear();
		m_TmpNetworkBaseItems.Clear();
	}

	/*
	 * Called Client Only: The server will clear the reserved stock that the player made
	 */
	// ------------------------------------------------------------
	// Expansion CancelPurchase
	// ------------------------------------------------------------
	void CancelPurchase(string itemClassName, PlayerBase player = NULL)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		if (!GetGame().IsDedicatedServer())
		{
			auto rpc = Expansion_CreateRPC("RPC_CancelPurchase");
			rpc.Write(itemClassName);
			rpc.Expansion_Send(true);
		}
	}
		
	// ------------------------------------------------------------
	// Expansion Exec_CancelPurchase
	// ------------------------------------------------------------
	private void RPC_CancelPurchase(PlayerIdentity senderRPC, Object target, ParamsReadContext ctx)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		string itemClassName;
		if (!ctx.Read(itemClassName))
			return;

		PlayerBase player = PlayerBase.GetPlayerByUID(senderRPC.GetId());
		if (!player)
			return;
	
		Exec_CancelPurchase(player, itemClassName);
	}
	
	// ------------------------------------------------------------
	// Expansion Exec_CancelPurchase
	// ------------------------------------------------------------
	private void Exec_CancelPurchase(notnull PlayerBase player, string itemClassName)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		RemoveReservedStock(player, itemClassName);
	}
	
	/*
	 * Called Client Only: Request sell
	 */
	// ------------------------------------------------------------
	// Expansion RequestSell
	// ------------------------------------------------------------
	void RequestSell(string itemClassName, int count, int currentPrice, ExpansionTraderObjectBase trader, ExpansionMarketSell sell)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		if (!GetGame().IsDedicatedServer())
		{
			if (!trader)
			{
				return;
			}
				
			auto rpc = Expansion_CreateRPC("RPC_RequestSell");
			rpc.Write(itemClassName);
			rpc.Write(count);
			rpc.Write(currentPrice);

			auto sellDebug = new ExpansionMarketSellDebug(sell, GetClientZone());
			sellDebug.OnSend(rpc);

			rpc.Expansion_Send(trader.GetTraderEntity(), true);
		}
	}
	
	// ------------------------------------------------------------
	// Expansion RPC_RequestSell
	// ------------------------------------------------------------
	private void RPC_RequestSell(PlayerIdentity senderRPC, Object target, ParamsReadContext ctx)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		if (!GetExpansionSettings().GetMarket().MarketSystemEnabled)
			return;

		string itemClassName;
		if (!ctx.Read(itemClassName))
			return;

		int count;
		if (!ctx.Read(count))
			return;
			
		int currentPrice;
		if (!ctx.Read(currentPrice))
			return;

		auto playerSentSellDebug = new ExpansionMarketSellDebug();
		playerSentSellDebug.OnReceive(ctx, itemClassName);

		ExpansionTraderObjectBase trader = GetTraderFromObject(target);
		if (!trader)
			return;

		PlayerBase player = PlayerBase.GetPlayerByUID(senderRPC.GetId());
		if (!player) 
			return;
			
		if (!CheckCanUseTrader(player, trader))
			return;

		Exec_RequestSell(player, itemClassName, count, currentPrice, trader, playerSentSellDebug);
	}
	
	// ------------------------------------------------------------
	// Expansion Exec_RequestSell
	// ------------------------------------------------------------
	private void Exec_RequestSell(notnull PlayerBase player, string itemClassName, int count, int playerSentPrice, ExpansionTraderObjectBase trader, ExpansionMarketSellDebug playerSentSellDebug)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		if (!player)
		{
			return;
		}
		
		ExpansionMarketTraderZone zone = trader.GetTraderZone();
		if (!zone)
		{
			MarketModulePrint("Callback_FailedUnknown: 7");

			Callback(itemClassName, ExpansionMarketResult.FailedUnknown, player.GetIdentity());
			return;
		}

		// Afterwards calculate the price of the items at that stock		
		ExpansionMarketSell sellList = player.GetMarketSell();
		sellList.Trader = trader;

		sellList.Item = ExpansionMarketCategory.GetGlobalItem( itemClassName );
		if (!sellList.Item /*|| !sellList.Valid*/)
		{
			MarketModulePrint("Callback_FailedUnknown: 8");

			Callback(itemClassName, ExpansionMarketResult.FailedUnknown, player.GetIdentity());
			return;
		}

		//! Check if we can sell this item to this specific trader
		if (!sellList.Trader.GetTraderMarket().CanSellItem(itemClassName))
		{
			Callback(itemClassName, ExpansionMarketResult.FailedCannotSell, player.GetIdentity());
			return;
		}
		
		ExpansionMarketPlayerInventory inventory = new ExpansionMarketPlayerInventory(player);

		int stock = 1;
		if (!sellList.Item.IsStaticStock())
			stock = zone.GetStock(itemClassName);

		ExpansionMarketResult result;
		string failedClassName;
		ExpansionMarketSellDebug sellDebug;

		//! Compare that price to the one the player sent
		if (!FindSellPrice(player, inventory.m_Inventory, stock, count, sellList, true, result, failedClassName) || sellList.Price != playerSentPrice)
		{
			if (result == ExpansionMarketResult.Success)
			{
				if (sellList.Price != playerSentPrice)
				{
					//! Check if there is a mismatch in the classnames the client sent to what the server sees

					sellDebug = new ExpansionMarketSellDebug(sellList, sellList.Trader.GetTraderZone());

					bool clientSellListMismatch;
					if (playerSentSellDebug.m_Items.Count() == sellDebug.m_Items.Count())
					{
						//! We got the expected number of items from client. Need to check if they are identical (= sell request likely failed due to stock change) or not
						//! (= failed due to client inventory desync, although unlikely since then it's more likely the count would have already been different)

						auto playerSentItems = playerSentSellDebug.GetItemClassNames();
						auto items = sellDebug.GetItemClassNames();

						//! Sort so we can directly compare and don't have to worry about order
						playerSentItems.Sort();
						items.Sort();

						for (int i = 0; i < items.Count(); i++)
						{
							if (playerSentItems[i] != items[i])
							{
								clientSellListMismatch = true;
								break;
							}
						}
					}
					else
					{
						clientSellListMismatch = true;
					}

					if (!clientSellListMismatch)
					{
						//! We don't know where this difference of exactly one sometimes comes from.
						//! Rounding differences between server and client? EnfuckScript?
						//! We just ignore it and fixup the price
						if (Math.AbsInt(sellList.Price - playerSentPrice) == 1)
						{
							EXPrint("Player sent sell price: " + playerSentPrice);
							EXPrint("Actual sell price: " + sellList.Price);

							//! When selling to trader, match the player sent price if it's lower
							if (sellList.Price > playerSentPrice)
								sellList.Price = playerSentPrice;

							EXPrint("Fixed sell price to " + sellList.Price);
						}
						else
						{
							//! The price the player has seen and agreed to in menu doesn't match anymore
							//! the current item price of the trader because stock has changed enough to affect it
							//! (another player was quicker to get his transaction through)
							result = ExpansionMarketResult.FailedStockChange;
						}
					}
					else
					{
						result = ExpansionMarketResult.FailedSellListMismatch;
					}
				}
				else
				{
					result = ExpansionMarketResult.FailedUnknown;
				}
			}
		}

		if (result != ExpansionMarketResult.Success)
		{
			EXLogPrint("===============================================================================");
			EXLogPrint("| MARKET SELL REQUEST FAILED!");

			if (result == ExpansionMarketResult.FailedStockChange)
			{
				EXLogPrint("| Price mismatch between client and server.");
			}
			else if (result == ExpansionMarketResult.FailedSellListMismatch)
			{
				EXLogPrint("| Item list mismatch between client and server.");
			}
			else if (result == ExpansionMarketResult.FailedItemDoesNotExistInTrader)
			{
				EXLogPrint("| Item '" + failedClassName + "' does not exist in trader.");
				itemClassName = failedClassName;
			}
			
			EXLogPrint("| Result code: " + typename.EnumToString(ExpansionMarketResult, result));

			if (result == ExpansionMarketResult.FailedStockChange || result == ExpansionMarketResult.FailedSellListMismatch)
			{
				MarketSellDebug(playerSentPrice, sellList.Price, playerSentSellDebug, sellDebug);
			}

			EXLogPrint("===============================================================================");

			player.ClearMarketSell();

			Callback(itemClassName, result, player.GetIdentity(), count);
			
			return;
		}
		#ifdef EXPANSIONMODMARKET_DEBUG
		else
		{
			EXLogPrint("===============================================================================");
			EXLogPrint("| MARKET SELL REQUEST SUCCEEDED!");

			sellDebug = new ExpansionMarketSellDebug(sellList, sellList.Trader.GetTraderZone());

			MarketSellDebug(playerSentPrice, sellList.Price, playerSentSellDebug, sellDebug);

			EXLogPrint("===============================================================================");
		}
		#endif
		
		sellList.Valid = true;
		sellList.Time = GetGame().GetTime();

		Exec_ConfirmSell(player, itemClassName);
	}
	
	void MarketSellDebug(int playerSentPrice, int actualPrice, ExpansionMarketSellDebug playerSentSellDebug, ExpansionMarketSellDebug sellDebug)
	{
		EXLogPrint("|");
		EXLogPrint("| CLIENT transaction data");
		EXLogPrint("| -----------------------");
		EXLogPrint("| Total sell price: " + playerSentPrice);
		playerSentSellDebug.Dump();

		EXLogPrint("|");
		EXLogPrint("| SERVER transaction data");
		EXLogPrint("| -----------------------");
		EXLogPrint("| Total sell price: " + actualPrice);
		sellDebug.Dump();
	}
	
	// ------------------------------------------------------------
	// Expansion Exec_ConfirmSell
	// ------------------------------------------------------------
	private void Exec_ConfirmSell(notnull PlayerBase player, string itemClassName)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		if (!player)
		{
			return;
		}
		
		ExpansionMarketSell sell = player.GetMarketSell();
		if (!sell || !sell.Valid || !sell.Trader || !sell.Trader.GetTraderEntity())
		{
			player.ClearMarketSell();

			EXPrint("Exec_ConfirmSell - Callback_FailedReserveTime");
			
			Callback(itemClassName, ExpansionMarketResult.FailedReserveTime, player.GetIdentity());
			return;
		}

		ExpansionMarketTraderZone zone = sell.Trader.GetTraderZone();
		if (!zone)
		{
			player.ClearMarketSell();

			MarketModulePrint("Exec_ConfirmSell - Callback_FailedUnknown");
			
			Callback(itemClassName, ExpansionMarketResult.FailedUnknown, player.GetIdentity());
			return;
		}
		
		if (vector.Distance(player.GetPosition(), sell.Trader.GetTraderEntity().GetPosition()) > MAX_TRADER_INTERACTION_DISTANCE)
		{
			MarketModulePrint("Exec_ConfirmSell - Callback_FailedTooFarAway: Player is too far from trader!");

			player.ClearMarketSell();

			Callback(itemClassName, ExpansionMarketResult.FailedTooFarAway, player.GetIdentity());
			return;
		}

		int count = sell.Sell.Count();
		TStringArray itemIDs = {};
		itemIDs.Reserve(count);
		TStringArray disallowedItems = {};
		
	#ifdef SERVER
		auto settings = GetExpansionSettings().GetMarket();
	#endif

		for (int j = count - 1; j >= 0; j--)
		{
			auto sellItem = sell.Sell[j];
			string itemID;
			if (sellItem.ClassName == itemClassName)
				itemID = ExpansionStatic.GetInstanceID(sellItem.ItemRep);
			else if (!sellItem.IsEntity)  //! Ammo in mags/ammopiles are not entities
				itemID = string.Format("%1 x%2", sellItem.ClassName, (int)sellItem.AddStockAmount);
			else
				itemID = sellItem.ItemRep.ToString();
			itemIDs.Insert(itemID);
			if (sellItem.IsEntity)
			{
			#ifdef SERVER
				if (settings.DisallowUnpersisted && !ExpansionWorld.IsStoreLoaded(sellItem.ItemRep) && !ExpansionWorld.IsStoreSaved(sellItem.ItemRep))
				{
					//! Only allow to sell items that have been persisted
					disallowedItems.Insert(sellItem.ItemRep.GetDisplayName());
					continue;
				}
			#endif

				if (!sellItem.ItemRep.IsPendingDeletion())
					sellItem.DestroyItem();
			}
			zone.AddStock(sellItem.ClassName, sellItem.AddStockAmount);
		}

		if (disallowedItems.Count() > 0)
		{
			ExpansionNotification("STR_EXPANSION_MARKET_TITLE", string.Format("You can't sell %1 right now because it hasn't been persisted to game storage yet. Please wait at least 15-20 seconds.", ExpansionString.JoinStrings(disallowedItems, ", ", true)), EXPANSION_NOTIFICATION_ICON_TRADER, COLOR_EXPANSION_NOTIFICATION_ERROR, 7, ExpansionNotificationType.MARKET).Error(player.GetIdentity());

			player.ClearMarketSell();

			return;
		}

		string itemsDetail = string.Format("x%1 (%2)", sell.TotalAmount, ExpansionString.JoinStrings(itemIDs, ", ", true));

		EntityAI parent = player;
		
		SpawnMoney(player, parent, sell.Price, true, sell.Item, sell.Trader.GetTraderMarket());
		
		zone.Save();

		ExpansionLogMarket(string.Format("Player \"%1\" (id=%2) has sold %3 %4 at the trader \"%5 (%6)\" in market zone \"%7\" (pos=%8) and got %9.", player.GetIdentity().GetName(), player.GetIdentity().GetId(), itemClassName, itemsDetail, sell.Trader.GetTraderMarket().m_FileName, sell.Trader.GetDisplayName(), sell.Trader.GetTraderZone().m_DisplayName, sell.Trader.GetTraderZone().Position.ToString(), sell.Price));	
		
		Callback(itemClassName, ExpansionMarketResult.SellSuccess, player.GetIdentity(), sell.TotalAmount, sell.Price);
		
		player.ClearMarketSell();
		
		CheckSpawn(player, parent);
	}

	// ------------------------------------------------------------
	// Expansion CancelSell
	// ------------------------------------------------------------
	void CancelSell(string itemClassName, PlayerBase player = NULL)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		if (!GetGame().IsDedicatedServer())
		{
			auto rpc = Expansion_CreateRPC("RPC_CancelSell");
			rpc.Write(itemClassName);
			rpc.Expansion_Send(true);
		}
	}
	
	// ------------------------------------------------------------
	// Expansion RPC_CancelSell
	// ------------------------------------------------------------
	private void RPC_CancelSell(PlayerIdentity senderRPC, Object target, ParamsReadContext ctx)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		string itemClassName;
		if (!ctx.Read(itemClassName))
			return;

		PlayerBase player = PlayerBase.GetPlayerByUID(senderRPC.GetId());
		if (!player)
			return;

		Exec_CancelSell(player, itemClassName);
	}
	
	// ------------------------------------------------------------
	// Expansion Exec_CancelSell
	// ------------------------------------------------------------
	private void Exec_CancelSell(notnull PlayerBase player, string itemClassName)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		player.ClearMarketSell();
	}

	// ------------------------------------------------------------
	// Expansion RequestTraderData
	// ------------------------------------------------------------
	void RequestTraderData(ExpansionTraderObjectBase trader)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		if (!GetGame().IsDedicatedServer())
		{
			if (!trader)
			{
				Error("ExpansionMarketModule::RequestTraderData - Trader is NULL!");
				return;
			}

			auto rpc = Expansion_CreateRPC("RPC_RequestTraderData");
			rpc.Expansion_Send(trader.GetTraderEntity(), true);
		}
	}
	
	//! @note server
	private void RPC_RequestTraderData(PlayerIdentity senderRPC, Object target, ParamsReadContext ctx)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		ExpansionTraderObjectBase trader = GetTraderFromObject(target);
		if (!trader)
		{
			Error("ExpansionMarketModule::RPC_RequestTraderData - Trader object is NULL!");
			return;
		}

		StartTrading(trader, senderRPC);
	}

	//! @note server
	void StartTrading(ExpansionTraderObjectBase trader, PlayerIdentity identity)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		if (!trader)
		{
			Error("ExpansionMarketModule::StartTrading - Trader object is NULL!");
			return;
		}

		if (!identity)
		{
			Error("ExpansionMarketModule::StartTrading - Player identity is NULL!");
			return;
		}

		trader.AddInteractingPlayer(identity.GetPlayer());

		auto hitch = new EXHitch(ToString() + "::RPC_RequestTraderData - LoadTraderData ");

		auto rpc = Expansion_CreateRPC("RPC_LoadTraderData");
		rpc.Write(trader.GetTraderZone().BuyPricePercent);
		rpc.Write(trader.GetTraderZone().SellPricePercent);

		rpc.Expansion_Send(trader.GetTraderEntity(), true, identity);
	}
	
	//! Send trader items to client in batches
	protected void LoadTraderItems(ExpansionTraderObjectBase trader, PlayerIdentity ident, int start = 0, bool stockOnly = false, TIntArray itemIDs = NULL)
	{
		MarketModulePrint("LoadTraderItems - Start - start: " + start + " stockOnly: " + stockOnly);

		if (!trader)
		{
			Error("ExpansionMarketModule::LoadTraderItems - Trader object is NULL!");
			return;
		}

		if (!ident)
		{
			Error("ExpansionMarketModule::LoadTraderItems - Player identity is NULL!");
			return;
		}

		array<ref ExpansionMarketNetworkItem> networkItemsTmp = new array<ref ExpansionMarketNetworkItem>;

		auto hitch = new EXHitch(ToString() + "::LoadTraderItems - GetNetworkSerialization ");
		
		TIntArray itemIDsTmp;
		if (itemIDs && itemIDs.Count())
		{
			//! Make sure we do not have duplicate IDs so counts are correct
			itemIDsTmp = new TIntArray;
			foreach (int itemID: itemIDs)
			{
				if (itemIDsTmp.Find(itemID) == -1)
					itemIDsTmp.Insert(itemID);
			}
			MarketModulePrint(ToString() + "::LoadTraderItems - IDs: " + itemIDsTmp);
		}

		int next = trader.GetNetworkSerialization(networkItemsTmp, start, stockOnly, itemIDsTmp);

		delete hitch;

		if (next < 0)
		{
			Error("ExpansionMarketModule::LoadTraderItems - GetNetworkSerialization failed!");
			return;
		}

		array<ref ExpansionMarketNetworkBaseItem> networkBaseItems = new array<ref ExpansionMarketNetworkBaseItem>;
		array<ref ExpansionMarketNetworkItem> networkItems = new array<ref ExpansionMarketNetworkItem>;

		foreach (ExpansionMarketNetworkItem item : networkItemsTmp)
		{
			if (stockOnly || item.m_StockOnly)
				networkBaseItems.Insert(new ExpansionMarketNetworkBaseItem(item.ItemID, item.Stock));
			else
				networkItems.Insert(item);
		}

		auto rpc = Expansion_CreateRPC("RPC_LoadTraderItems");
		rpc.Write(start);
		rpc.Write(next);
		if (itemIDsTmp && itemIDsTmp.Count())
			rpc.Write(itemIDsTmp.Count());
		else
			rpc.Write(trader.GetTraderMarket().m_Items.Count());
		rpc.Write(stockOnly);
		rpc.Write(networkBaseItems);
		rpc.Write(networkItems);
		rpc.Expansion_Send(trader.GetTraderEntity(), true, ident);

		MarketModulePrint("LoadTraderItems - End - start: " + start + " end: " + next);
	}
	
	// ------------------------------------------------------------
	// Expansion RPC_LoadTraderData - client
	// ------------------------------------------------------------
	private void RPC_LoadTraderData(PlayerIdentity senderRPC, Object target, ParamsReadContext ctx)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this, "" + target);
#endif

		ExpansionTraderObjectBase trader = GetTraderFromObject(target);
		if (!trader)
		{
			Error("ExpansionMarketModule::RPC_LoadTraderData - Could not get ExpansionTraderObjectBase!");
			return;
		}

		EXTrace.Print(EXTrace.MARKET, this, "Client market zone: " + m_ClientMarketZone);
		
		EXTrace.Print(EXTrace.MARKET, this, "Reading buy price percent...");
		if (!ctx.Read(m_ClientMarketZone.BuyPricePercent))
		{
			Error("ExpansionMarketModule::RPC_LoadTraderData - Could not read buy price percent!");
			return;
		}
		
		EXTrace.Print(EXTrace.MARKET, this, "Reading sell price percent...");
		if (!ctx.Read(m_ClientMarketZone.SellPricePercent))
		{
			Error("ExpansionMarketModule::RPC_LoadTraderData - Could not read sell price percent!");
			return;
		}

		EXTrace.Print(EXTrace.MARKET, this, "Setting client trader: " + trader);
		m_OpenedClientTrader = trader;
		m_TraderEntity = trader.GetTraderEntity();

		EXTrace.Print(EXTrace.MARKET, this, "Opening trader menu...");
		if (!OpenTraderMenu())
			return;

		bool stockOnly = trader.GetTraderMarket().m_StockOnly;  //! If already netsynched, request stock only
		RequestTraderItems(trader, 0, stockOnly);
	}

	// ------------------------------------------------------------
	// Expansion RequestTraderItems - client
	// ------------------------------------------------------------
	void RequestTraderItems(ExpansionTraderObjectBase trader, int start = 0, bool stockOnly = false, TIntArray itemIDs = NULL)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		if (!trader)
		{
			Error("ExpansionMarketModule::RequestTraderItems - Trader is NULL!");
			return;
		}
		
		auto rpc = Expansion_CreateRPC("RPC_RequestTraderItems");
		rpc.Write(start);
		rpc.Write(stockOnly);
		rpc.Write(itemIDs);
		rpc.Expansion_Send(trader.GetTraderEntity(), true);
	}

	// ------------------------------------------------------------
	// Expansion RPC_RequestTraderItems - server
	// ------------------------------------------------------------
	private void RPC_RequestTraderItems(PlayerIdentity senderRPC, Object target, ParamsReadContext ctx)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		ExpansionTraderObjectBase trader = GetTraderFromObject(target);
		if (!trader)
		{
			Error("ExpansionMarketModule::RPC_RequestTraderItems - Could not get ExpansionTraderObjectBase!");
			return;
		}

		int start;
		if (!ctx.Read(start))
		{
			Error("ExpansionMarketModule::RPC_RequestTraderItems - Could not read remaining items start!");
			return;
		}

		bool stockOnly;
		if (!ctx.Read(stockOnly))
		{
			Error("ExpansionMarketModule::RPC_RequestTraderItems - Could not read stockOnly!");
			return;
		}

		TIntArray itemIDs;
		if (!ctx.Read(itemIDs))
		{
			Error("ExpansionMarketModule::RPC_RequestTraderItems - Could not read itemIDs!");
			return;
		}

		LoadTraderItems(trader, senderRPC, start, stockOnly, itemIDs);
	}

	// ------------------------------------------------------------
	// Expansion RPC_LoadTraderItems - client
	// ------------------------------------------------------------
	private void RPC_LoadTraderItems(PlayerIdentity senderRPC, Object target, ParamsReadContext ctx)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this, "" + target);
#endif

		ExpansionTraderObjectBase trader = GetTraderFromObject(target);
		if (!trader)
		{
			Error("ExpansionMarketModule::RPC_LoadTraderItems - Could not get ExpansionTraderObjectBase!");
			return;
		}

		if (trader != m_OpenedClientTrader)
		{
			EXPrint("ExpansionMarketModule::RPC_LoadTraderItems - ignoring items received for different trader");
			return;
		}

		int start;
		if (!ctx.Read(start))
		{
			Error("ExpansionMarketModule::RPC_LoadTraderItems - Could not read items start index!");
			SI_SetTraderInvoker.Invoke(trader, true);
			return;
		}

		int next;
		if (!ctx.Read(next))
		{
			Error("ExpansionMarketModule::RPC_LoadTraderItems - Could not read items next index!");
			SI_SetTraderInvoker.Invoke(trader, true);
			return;
		}

		int count;
		if (!ctx.Read(count))
		{
			Error("ExpansionMarketModule::RPC_LoadTraderItems - Could not read items count!");
			SI_SetTraderInvoker.Invoke(trader, true);
			return;
		}

		bool stockOnly;
		if (!ctx.Read(stockOnly))
		{
			Error("ExpansionMarketModule::RPC_LoadTraderItems - Could not read stockOnly!");
			SI_SetTraderInvoker.Invoke(trader, true);
			return;
		}

		EXPrint("RPC_LoadTraderItems - received batch total: " + next + " remaining: " + (count - next));

		auto hitch = new EXHitch(ToString() + "::RPC_LoadTraderItems - update market items ");
	
		array<ref ExpansionMarketNetworkBaseItem> networkBaseItems = new array<ref ExpansionMarketNetworkBaseItem>;
		if (!ctx.Read(networkBaseItems))
		{
			Error("ExpansionMarketModule::RPC_LoadTraderItems - Could not read networkBaseItems array!");
			SI_SetTraderInvoker.Invoke(trader, true);
			return;
		}

		array<ref ExpansionMarketNetworkItem> networkItems = new array<ref ExpansionMarketNetworkItem>;
		if (!ctx.Read(networkItems))
		{
			Error("ExpansionMarketModule::RPC_LoadTraderItems - Could not read networkItems array!");
			SI_SetTraderInvoker.Invoke(trader, true);
			return;
		}
	
		if (networkBaseItems.Count() + networkItems.Count() <= 0)
		{
			Error("ExpansionMarketModule::RPC_LoadTraderItems - networkBaseItems + networkItems count is 0!");
			SI_SetTraderInvoker.Invoke(trader, true);
			return;
		}

		int i;
		ExpansionMarketItem item;

		if (start == 0)
		{
			//! 1st batch, make sure cache is clear
			ClearTmpNetworkCaches();
		}

		if (networkItems.Count())
		{
			//! Add full items + set stock
			EXPrint(ToString() + "::RPC_LoadTraderItems - Adding and setting stock for " + networkItems.Count() + " items");

			for (i = 0; i < networkItems.Count(); i++)
			{
				//EXPrint("RPC_LoadTraderItems - " + networkItems[i].ClassName + " (ID " + networkItems[i].ItemID + ") - stock: " + networkItems[i].Stock);
				item = GetExpansionSettings().GetMarket().UpdateMarketItem_Client(networkItems[i]);
				m_ClientMarketZone.SetStock(networkItems[i].ClassName, networkItems[i].Stock);
				int param1 = networkItems[i].Packed >> 24;
				//! @note EnfScript bug: Bit-shifting produces negative value. Workaround: Apply mask first (bitwise AND)
				//! https://feedback.bistudio.com/T177670
				int rarity = (param1 & 0xf0) >> 4;
			#ifdef EXPANSIONMODHARDLINE
				if (rarity)
				{
					GetExpansionSettings().GetHardline().ItemRarity[networkItems[i].ClassName] = rarity;
				
					foreach (string variant: networkItems[i].Variants)
					{
						GetExpansionSettings().GetHardline().ItemRarity[variant] = rarity;
					}
				}
			#endif
				int buySell = param1 & 0x0f;
				trader.GetTraderMarket().AddItemInternal(item, buySell);
				if (!m_TmpNetworkCats.Contains(networkItems[i].CategoryID))
					m_TmpNetworkCats.Insert(networkItems[i].CategoryID, GetExpansionSettings().GetMarket().GetCategory(networkItems[i].CategoryID));
			}
		}

		if (m_TmpNetworkCats.Count())
		{
			foreach (ExpansionMarketNetworkBaseItem networkBaseItem: networkBaseItems)
			{
				m_TmpVariantIds.Insert(networkBaseItem.ItemID);
			}
		}

		//! Set stock only
		for (i = 0; i < networkBaseItems.Count(); i++)
		{
			m_TmpNetworkBaseItems.Insert(networkBaseItems[i]);
		}
		
		delete hitch;

		if (count - next == 0)
		{
			//! Last batch

			if (m_TmpVariantIds.Count())
			{
				foreach (ExpansionMarketTraderItem tItem: trader.GetTraderMarket().m_Items)
				{
					if (tItem.MarketItem.Variants.Count())
					{
						//EXPrint("RPC_LoadTraderItems - adding variants for " + tItem.MarketItem.ClassName + " (ID " + tItem.MarketItem.ItemID + ")");
						ExpansionMarketCategory itemCat = GetExpansionSettings().GetMarket().GetCategory(tItem.MarketItem.CategoryID);
						itemCat.AddVariants(tItem.MarketItem, m_TmpVariantIds, m_TmpVariantIdIdx);
					}
				}
			}

			if (m_TmpNetworkCats.Count())
			{
				foreach (ExpansionMarketCategory cat : m_TmpNetworkCats)
				{
					cat.SetAttachmentsFromIDs();
					cat.Finalize(false);
				}

				trader.GetTraderMarket().Finalize();
			}

			EXPrint(ToString() + "::RPC_LoadTraderItems - Setting stock for " + m_TmpNetworkBaseItems.Count() + " items");
			foreach (ExpansionMarketNetworkBaseItem tmpNetworkBaseItem: m_TmpNetworkBaseItems)
			{
				item = ExpansionMarketCategory.GetGlobalItem(tmpNetworkBaseItem.ItemID, false);
				if (!item)
				{
					EXPrint(ToString() + "::RPC_LoadTraderItems - WARNING - item ID " + tmpNetworkBaseItem.ItemID + " does not exist!");
					continue;
				}
				item.m_UpdateView = true;
				//EXPrint("RPC_LoadTraderItems - " + item.ClassName + " (ID " + item.ItemID + ") - stock: " + tmpNetworkBaseItem.Stock);
				m_ClientMarketZone.SetStock(item.ClassName, tmpNetworkBaseItem.Stock);
			}

			ClearTmpNetworkCaches();

			trader.GetTraderMarket().m_StockOnly = true;
			SI_SetTraderInvoker.Invoke(trader, true);
		}
		else
		{
			//! Client can draw received items so far
			SI_SetTraderInvoker.Invoke(trader, false);

			//! Request next batch
			RequestTraderItems(trader, next, stockOnly);
		}
	}

	//! Exit trader - client
	void ExitTrader()
	{
		auto rpc = Expansion_CreateRPC("RPC_ExitTrader");
		rpc.Expansion_Send(m_TraderEntity, true);
	}
	
	//! Exit trader - server
	void RPC_ExitTrader(PlayerIdentity senderRPC, Object target, ParamsReadContext ctx)
	{
		ExpansionTraderObjectBase trader = GetTraderFromObject(target);
		if (!trader)
		{
			Error("ExpansionMarketModule::RPC_ExitTrader - Could not get ExpansionTraderObjectBase!");
			return;
		}

		trader.RemoveInteractingPlayer(senderRPC.GetPlayer());
	}

	bool IsMoney(string type)
	{
		return m_MoneyTypes.Contains(type);
	}

	bool IsMoney(EntityAI item)
	{
		string type = item.GetType();
		type.ToLower();
		return IsMoney(type);
	}

	// -----------------------------------------------------------
	// Expansion array< EntityAI > LocalGetEntityInventory
	// -----------------------------------------------------------
	array<EntityAI> LocalGetEntityInventory()
	{
		return m_LocalEntityInventory.m_Inventory;
	}

	// -----------------------------------------------------------
	// Expansion EnumeratePlayerInventory
	// -----------------------------------------------------------
	void EnumeratePlayerInventory(PlayerBase player)
	{
		m_LocalEntityInventory = new ExpansionMarketPlayerInventory(player);
	}
	
	// ------------------------------------------------------------
	// Expansion Int GetAmountInInventory
	// Gets the amount of market items the player has in his inventroy. Only to be used on client while in trade menu!
	// ------------------------------------------------------------
	//! Returns positive number if at least one sellable item found, negative number if only unsellable items found
	int GetAmountInInventory(ExpansionMarketItem item, array< EntityAI > entities)
	{
		MarketModulePrint("GetAmountInInventory - Start");
		
		string itemName = item.ClassName;
		itemName.ToLower();
		
		int sellable;
		int unsellable;

		foreach (EntityAI entity: entities)
		{
			if (entity == NULL)
				continue;

			string entName = entity.GetType();
			entName.ToLower();
			
			entName = GetMarketItemClassName(m_OpenedClientTrader.GetTraderMarket(), entName);

			if (entName != itemName)
				continue;

			int amount = GetItemAmount(entity);

			if (amount > 0)
				sellable += amount;
			else
				unsellable += amount;
		}

		if (sellable > 0)
		{
			MarketModulePrint("GetAmountInInventory - End and return sellable: " + sellable + " for item " + itemName);
			return sellable;
		}
		else
		{
			MarketModulePrint("GetAmountInInventory - End and return unsellable: " + unsellable + " for item " + itemName);
			return unsellable;
		}
	}
	
	// ------------------------------------------------------------
	// Expansion Array GetEnitysOfItemInventory
	// ------------------------------------------------------------
	array<EntityAI> GetEnitysOfItemInventory(ExpansionMarketItem item, array< EntityAI > entitys)
	{
		MarketModulePrint("GetEnitysOfItemInventory - Start");
		
		array<EntityAI> itemsArray = new array<EntityAI>;
		int totalAmount = 0;

		for (int i = 0; i < entitys.Count(); i++)
		{
			EntityAI entity = entitys.Get(i);
			if (entity == NULL)
				continue;

			string entName = entity.GetType();
			entName.ToLower();
			
			string itemName = item.ClassName;
			itemName.ToLower();
			
			if (entName != itemName)
				continue;
			
			itemsArray.Insert(entity);
		}

		return itemsArray;
	}

	// ------------------------------------------------------------
	// Expansion Bool CanSellItem
	// ------------------------------------------------------------
	bool CanSellItem(EntityAI item, bool checkIfRuined = false)
	{
		Error("DEPRECATED, use MiscGameplayFunctions.Expansion_IsLooseEntity");

		return MiscGameplayFunctions.Expansion_IsLooseEntity(item, checkIfRuined);
	}

	// ------------------------------------------------------------
	// Expansion Bool CanOpenMenu
	// ------------------------------------------------------------
	bool CanOpenMenu()
	{
		if (!GetGame().IsDedicatedServer())
		{
			if (GetGame().GetUIManager().GetMenu())
				return false;
			
			if (GetDayZGame().GetExpansionGame().GetExpansionUIManager().GetMenu())
				return false;
		}

		return true;
	}
	
	bool CheckCanUseTrader(PlayerBase player, ExpansionTraderObjectBase trader)
	{
	#ifdef ENFUSION_AI_PROJECT
		if (GetGame().IsServer() && trader.GetTraderMarket().RequiredFaction != "")
		{
			if (!player.GetGroup() || player.GetGroup().GetFaction().GetName() != trader.GetTraderMarket().RequiredFaction)
			{
				if (player.GetIdentity())
				{
					string factionDisplayname;
				#ifdef EXPANSIONMODAI
					eAIFaction faction = eAIFaction.Create(trader.GetTraderMarket().RequiredFaction);
					factionDisplayname = faction.GetDisplayName();
				#else
					factionDisplayname = trader.GetTraderMarket().RequiredFaction;
				#endif
					ExpansionNotification("STR_EXPANSION_AI_FACTION", new StringLocaliser("STR_EXPANSION_AI_REQUIRED_FACTION_TRADER", factionDisplayname), EXPANSION_NOTIFICATION_ICON_ERROR, COLOR_EXPANSION_NOTIFICATION_ERROR, 7, ExpansionNotificationType.TOAST).Create(player.GetIdentity());
				}

				return false;
			}
		}
	#endif

		return true;
	}

	// ------------------------------------------------------------
	// Expansion OpenTraderMenu
	// ------------------------------------------------------------
	bool OpenTraderMenu()
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		if (!MoneyCheck())
			return false;

		GetDayZGame().GetExpansionGame().GetExpansionUIManager().CreateSVMenu("ExpansionMarketMenu");

		return true;
	}
	
	// ------------------------------------------------------------
	// Expansion OpenTraderMenu
	// ------------------------------------------------------------
	bool OpenATMMenu()
	{
		if (!MoneyCheck())
			return false;

		GetDayZGame().GetExpansionGame().GetExpansionUIManager().CreateSVMenu("ExpansionATMMenu");

		return true;
	}

	// ------------------------------------------------------------
	// Expansion MarketModulePrint
	// ------------------------------------------------------------
	static void MarketModulePrint(string text)
	{
	#ifdef DIAG_DEVELOPER
		EXPrint("ExpansionMarketModule::" + text);
	#endif
	}

	void CheckSpawn(PlayerBase player, EntityAI parent, bool attachmentNotAttached = false)
	{
	}
	
	// ------------------------------------------------------------
	// Expansion GetMoneyDenominations
	// ------------------------------------------------------------
	array<string> GetMoneyDenominations()
	{
		return m_MoneyDenominations;
	}

	string GetMoneyDenomination(int i)
	{
		return m_MoneyDenominations[i];
	}
	
	ExpansionMarketCategory GetItemCategory(ExpansionMarketItem item)
	{
		return GetExpansionSettings().GetMarket().GetCategory(item.CategoryID);
	}
	
	// ------------------------------------------------------------
	// Expansion GetATMData
	// ------------------------------------------------------------	
	array<ref ExpansionMarketATM_Data> GetATMData()
	{
		return m_ATMData;
	}
	
	// ------------------------------------------------------------
	// Expansion GetPlayerATMData
	// ------------------------------------------------------------		
	ExpansionMarketATM_Data GetPlayerATMData(string id)
	{
		array<ref ExpansionMarketATM_Data> data = GetATMData();
		foreach (ExpansionMarketATM_Data currentData : data)
		{
			if (currentData && currentData.PlayerID == id)
				return currentData;
		}
		
		return NULL;
	}
	
	// ------------------------------------------------------------
	// Expansion LoadATMData
	// ------------------------------------------------------------
	void LoadATMData()
	{
		if (!GetExpansionSettings().GetMarket().ATMSystemEnabled)
			return;

		array<string> files = ExpansionStatic.FindFilesInLocation(EXPANSION_ATM_FOLDER, ".json");
		
		foreach (string fileName : files)
		{
			//! Strip '.json' extension
			fileName = fileName.Substring(0, fileName.Length() - 5);
			ExpansionMarketATM_Data data = ExpansionMarketATM_Data.Load(fileName);
			m_ATMData.Insert(data);
		}
	}
	
	// ------------------------------------------------------------
	// Expansion SaveATMData
	// ------------------------------------------------------------
	void SaveATMData()
	{
		foreach (ExpansionMarketATM_Data atmData: m_ATMData)
		{
			atmData.Save();
		}
	}
	
	
	// ------------------------------------------------------------
	// Expansion CreateATMData
	// ------------------------------------------------------------
	void CreateATMData(PlayerIdentity ident)
	{
		ExpansionMarketATM_Data newData = new ExpansionMarketATM_Data;
		newData.m_FileName = ident.GetId();
		newData.PlayerID = ident.GetId();
			
		newData.MoneyDeposited = GetExpansionSettings().GetMarket().DefaultDepositMoney;
		
		newData.Save();
		
		m_ATMData.Insert(newData);
	}
	
	// ------------------------------------------------------------
	// Expansion RequestPlayerATMData
	// ------------------------------------------------------------
	void RequestPlayerATMData()
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		if (!GetGame().IsDedicatedServer())
		{
			auto rpc = Expansion_CreateRPC("RPC_RequestPlayerATMData");
			rpc.Expansion_Send(true);
		}
	}
	
	// ------------------------------------------------------------
	// Expansion RPC_RequestPlayerATMData
	// ------------------------------------------------------------
	private void RPC_RequestPlayerATMData(PlayerIdentity senderRPC, Object target, ParamsReadContext ctx)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif
		
		SendPlayerATMData(senderRPC);
	}

	void SendPlayerATMData(PlayerIdentity ident)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		if (!ident)
		{
			Error("ExpansionMarketModule::SendPlayerATMData - Could not get sender indentity!");
			return;
		}
		
		ExpansionMarketATM_Data data = GetPlayerATMData(ident.GetId());
		if (!data)
		{
			Error("ExpansionMarketModule::SendPlayerATMData - Could not get ExpansionMarketATM_Data!");
			return;
		}
			
		auto rpc = Expansion_CreateRPC("RPC_SendPlayerATMData");
		rpc.Write(data);
		rpc.Expansion_Send(true, ident);
	}
	
	// ------------------------------------------------------------
	// Expansion RPC_SendPlayerATMData
	// ------------------------------------------------------------
	private void RPC_SendPlayerATMData(PlayerIdentity senderRPC, Object target, ParamsReadContext ctx)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		if (!GetExpansionSettings().GetMarket().ATMSystemEnabled)
			return;

		ExpansionMarketATM_Data data;
		if (!ctx.Read(data))
		{
			Error("ExpansionMarketModule::RPC_SendPlayerATMData - Could not get ExpansionMarketATM_Data!");
			return;
		}
		
		Exec_SendPlayerATMData(data);
	}

	// ------------------------------------------------------------
	// Expansion Exec_SendPlayerATMData
	// ------------------------------------------------------------
	private void Exec_SendPlayerATMData(ExpansionMarketATM_Data data)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		if (!OpenATMMenu())
			return;

		SetPlayerATMData(data);
	}
	
	// ------------------------------------------------------------
	// Expansion SetPlayerATMData
	// ------------------------------------------------------------
	void SetPlayerATMData(ExpansionMarketATM_Data data)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		SI_ATMMenuInvoker.Invoke(data);
	}
	
	// ------------------------------------------------------------
	// Expansion RequestDepositMoney
	// ------------------------------------------------------------	
	void RequestDepositMoney(int amount)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		if (!GetGame().IsDedicatedServer())
		{
			auto rpc = Expansion_CreateRPC("RPC_RequestDepositMoney");
			rpc.Write(amount);
			rpc.Expansion_Send(true);
		}
	}
	
	// ------------------------------------------------------------
	// Expansion RPC_RequestDepositMoney
	// ------------------------------------------------------------
	private void RPC_RequestDepositMoney(PlayerIdentity senderRPC, Object target, ParamsReadContext ctx)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		if (!GetExpansionSettings().GetMarket().ATMSystemEnabled)
			return;

		int amount;
		if (!ctx.Read(amount))
		{
			Error("ExpansionMarketModule::RPC_RequestDepositMoney - Could not get amount!");
			return;
		}
		
		Exec_RequestDepositMoney(amount, senderRPC);
	}
	
	// ------------------------------------------------------------
	// Expansion Exec_RequestDepositMoney
	// ------------------------------------------------------------
	private void Exec_RequestDepositMoney(int amount, PlayerIdentity ident)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		if (!ident)
		{
			Error("ExpansionMarketModule::Exec_RequestDepositMoney - Could not get player identity!");
			return;
		}

		PlayerBase player = PlayerBase.GetPlayerByUID(ident.GetId());
		if (!player)
		{
			Error("ExpansionMarketModule::Exec_RequestDepositMoney - Could not get player base enity!");
			return;
		}
				
		ExpansionMarketATM_Data data = GetPlayerATMData(ident.GetId());
		if (!data)
		{
			Error("ExpansionMarketModule::Exec_RequestDepositMoney - Could not find player atm data!");
			
			return;
		}
		
		//! We can only deposit money until we reach max. depending on server setting
		if (data.MoneyDeposited + amount > GetExpansionSettings().GetMarket().MaxDepositMoney)
		{
			Error("ExpansionMarketModule::Exec_RequestDepositMoney - Receiving player would go over max allowed deposit money value!");
			
			return;
		}
		
		if (!GetExpansionSettings().GetMarket().Currencies.Count())
		{
			Error("ExpansionMarketModule::Exec_RequestDepositMoney - No currencies defined for ATM!");
			return;
		}

		array<int> monies = new array<int>;		
		if (!FindMoneyAndCountTypes(player, amount, monies, true, NULL, NULL, true))
		{
			Error("ExpansionMarketModule::Exec_RequestDepositMoney - Could not find player money!");			
			UnlockMoney(player);
			
			return;
		}

		EntityAI parent = player;
		
		int removed = RemoveMoney(player);
		if (removed - amount > 0)
		{
		    SpawnMoney(player, parent, removed - amount, true, NULL, NULL, true);

			CheckSpawn(player, parent);
		}
		
		data.AddMoney(amount);
		data.Save();
		
		ExpansionLogATM(string.Format("Player \"%1\" (id=%2) has deposited %3 on his ATM account.", ident.GetName(), ident.GetId(), amount));
		
		ConfirmDepositMoney(amount, ident, data);
	}
		
	// ------------------------------------------------------------
	// Expansion ConfirmDepositMoney
	// ------------------------------------------------------------	
	void ConfirmDepositMoney(int amount, PlayerIdentity ident, ExpansionMarketATM_Data data)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		auto rpc = Expansion_CreateRPC("RPC_ConfirmDepositMoney");
		rpc.Write(amount);
		rpc.Write(data);
		rpc.Expansion_Send(true, ident);
	}
	
	// ------------------------------------------------------------
	// Expansion RPC_ConfirmDepositMoney
	// ------------------------------------------------------------
	private void RPC_ConfirmDepositMoney(PlayerIdentity senderRPC, Object target, ParamsReadContext ctx)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		if (!GetExpansionSettings().GetMarket().ATMSystemEnabled)
			return;

		int amount;
		if (!ctx.Read(amount))
		{
			Error("ExpansionMarketModule::RPC_ConfirmDepositMoney - Could not get amount!");
			return;
		}
		
		ExpansionMarketATM_Data data;
		if (!ctx.Read(data))
		{
			Error("ExpansionMarketModule::RPC_ConfirmDepositMoney - Could not get player ATM data!");
			return;
		}
		
		Exec_ConfirmDepositMoney(amount, data);
	}
	
	// ------------------------------------------------------------
	// Expansion Exec_ConfirmDepositMoney
	// ------------------------------------------------------------
	private void Exec_ConfirmDepositMoney(int amount, ExpansionMarketATM_Data data)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		SI_ATMMenuCallback.Invoke(amount, data, 1);
	}
	
	// ------------------------------------------------------------
	// Expansion RequestWithdraw
	// ------------------------------------------------------------	
	void RequestWithdrawMoney(int amount)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		if (!GetGame().IsDedicatedServer())
		{
			auto rpc = Expansion_CreateRPC("RPC_RequestWithdrawMoney");
			rpc.Write(amount);
			rpc.Expansion_Send(true);
		}
	}
	
	// ------------------------------------------------------------
	// Expansion RPC_RequestWithdrawMoney
	// ------------------------------------------------------------
	private void RPC_RequestWithdrawMoney(PlayerIdentity senderRPC, Object target, ParamsReadContext ctx)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		if (!GetExpansionSettings().GetMarket().ATMSystemEnabled)
			return;

		int amount;
		if (!ctx.Read(amount))
		{
			Error("ExpansionMarketModule::RPC_RequestWithdrawMoney - Could not get amount!");
			return;
		}
		
		Exec_RequestWithdrawMoney(amount, senderRPC);
	}
	
	// ------------------------------------------------------------
	// Expansion Exec_RequestWithdrawMoney
	// ------------------------------------------------------------
	private void Exec_RequestWithdrawMoney(int amount, PlayerIdentity ident)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		if (!ident)
		{
			Error("ExpansionMarketModule::Exec_RequestWithdrawMoney - Could not get sender indentity!");
			return;
		}	
					
		ExpansionMarketATM_Data data = GetPlayerATMData(ident.GetId());
		if (!data)
		{
			Error("ExpansionMarketModule::Exec_RequestWithdrawMoney - Could not get player ATM data!");			
			return;
		}
		
		if (data.MoneyDeposited < amount)
		{
			Error("ExpansionMarketModule::Exec_RequestWithdrawMoney - Tried to withdraw more money than in account!");
			return;
		}
		
		PlayerBase player = PlayerBase.GetPlayerByUID(ident.GetId());
		if (!player)
		{
			Error("ExpansionMarketModule::Exec_RequestWithdrawMoney - Could not get player base entity!");
			return;
		}	
		
		EntityAI parent = player;
		if (!parent)
		{
			Error("ExpansionMarketModule::Exec_RequestWithdrawMoney - Could not get player entity!");
			return;
		}	
		
		if (!GetExpansionSettings().GetMarket().Currencies.Count())
		{
			Error("ExpansionMarketModule::Exec_RequestWithdrawMoney - No currencies defined for ATM!");
			return;
		}

		SpawnMoney(player, parent, amount, true, NULL, NULL, true);

		CheckSpawn(player, parent);
		
		data.RemoveMoney(amount);
		data.Save();
		
		ExpansionLogATM(string.Format("Player \"%1\" (id=%2) has withdrawn %3 from his ATM account.", ident.GetName(), ident.GetId(), amount));
		
		ConfirmWithdrawMoney(amount, ident, data);
	}
	
	// ------------------------------------------------------------
	// Expansion ConfirmWithdrawMoney
	// ------------------------------------------------------------	
	void ConfirmWithdrawMoney(int amount, PlayerIdentity ident, ExpansionMarketATM_Data data)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		auto rpc = Expansion_CreateRPC("RPC_ConfirmWithdrawMoney");
		rpc.Write(amount);
		rpc.Write(data);
		rpc.Expansion_Send(true, ident);
	}
	
	// ------------------------------------------------------------
	// Expansion RPC_ConfirmWithdrawMoney
	// ------------------------------------------------------------
	private void RPC_ConfirmWithdrawMoney(PlayerIdentity senderRPC, Object target, ParamsReadContext ctx)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		if (!GetExpansionSettings().GetMarket().ATMSystemEnabled)
			return;

		int amount;
		if (!ctx.Read(amount))
		{
			Error("ExpansionMarketModule::RPC_ConfirmWithdrawMoney - Could not get amount!");
			return;
		}
		
		ExpansionMarketATM_Data data;
		if (!ctx.Read(data))
		{
			Error("ExpansionMarketModule::RPC_ConfirmWithdrawMoney - Could not get player ATM data!");
			return;
		}
		
		Exec_ConfirmWithdrawMoney(amount, data);
	}
	
	// ------------------------------------------------------------
	// Expansion Exec_ConfirmWithdrawMoney
	// ------------------------------------------------------------
	private void Exec_ConfirmWithdrawMoney(int amount, ExpansionMarketATM_Data data)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		SI_ATMMenuCallback.Invoke(amount, data, 2);
	}
	
	// ------------------------------------------------------------
	// Expansion RequestTransferMoneyToPlayer
	// Called from the client
	// ------------------------------------------------------------	
	void RequestTransferMoneyToPlayer(int amount, string playerID)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		if (!GetGame().IsDedicatedServer())
		{
			auto rpc = Expansion_CreateRPC("RPC_RequestTransferMoneyToPlayer");
			rpc.Write(amount);
			rpc.Write(playerID);
			rpc.Expansion_Send(true);
		}
	}
	
	// ------------------------------------------------------------
	// Expansion RPC_RequestTransferMoneyToPlayer
	// Called on the server
	// ------------------------------------------------------------
	private void RPC_RequestTransferMoneyToPlayer(PlayerIdentity senderRPC, Object target, ParamsReadContext ctx)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		if (!GetExpansionSettings().GetMarket().ATMSystemEnabled)
			return;
		
		int amount;
		if (!ctx.Read(amount))
		{
			Error("ExpansionMarketModule::RPC_RequestTransferMoneyToPlayer - Could not get amount!");
			return;
		}
		
		string playerID;
		if (!ctx.Read(playerID))
		{
			Error("ExpansionMarketModule::RPC_RequestTransferMoneyToPlayer - Could not get player id!");
			return;
		}
		
		Exec_RequestTransferMoneyToPlayer(amount, playerID, senderRPC);
	}
	
	// ------------------------------------------------------------
	// Expansion Exec_RequestTransferMoneyToPlayer
	// Called on the server
	// ------------------------------------------------------------
	private void Exec_RequestTransferMoneyToPlayer(int amount, string receiverID, PlayerIdentity ident)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		if (!ident)
		{
			Error("ExpansionMarketModule::Exec_RequestTransferMoneyToPlayer - Could not get sender identity!");
			return;
		}
		
		ExpansionMarketATM_Data data_sender = GetPlayerATMData(ident.GetId());
		if (!data_sender)
		{
			Error("ExpansionMarketModule::Exec_RequestTransferMoneyToPlayer - Could not get senders player ATM data!");			
			return;
		}
		
		ExpansionMarketATM_Data data_receiver = GetPlayerATMData(receiverID);
		if (!data_receiver)
		{
			Error("ExpansionMarketModule::Exec_RequestTransferMoneyToPlayer - Could not get receivers player ATM data!");			
			return;
		}
		
		if (data_sender.MoneyDeposited < amount)
		{
			Error("ExpansionMarketModule::Exec_RequestTransferMoneyToPlayer - Tried to transfer more money than in inventory!");
			return;
		}
		
		if (data_receiver.MoneyDeposited + amount > GetExpansionSettings().GetMarket().MaxDepositMoney)
		{
			ExpansionNotification("STR_EXPANSION_ATM_DEPOSIT_FAILED", new StringLocaliser("STR_EXPANSION_ATM_DEPOSIT_MAX_ERROR", GetExpansionSettings().GetMarket().MaxDepositMoney.ToString())).Error(ident);
			Error("ExpansionMarketModule::Exec_RequestTransferMoneyToPlayer - Receiving player would go over max allowed deposit money value!");
			return;
		}
		
		//! Remove the money from the sender players deposit
		data_sender.RemoveMoney(amount);
		data_sender.Save();
		
		//! Add the money to the receiver players deposit
		data_receiver.AddMoney(amount);
		data_receiver.Save();
		
		ConfirmTransferMoneyToPlayer(ident, data_sender);
		
		PlayerBase receiverPlayer = PlayerBase.GetPlayerByUID(receiverID);
		if (!receiverPlayer)
		{
			Error("ExpansionMarketModule::Exec_RequestTransferMoneyToPlayer - Could not get reciver player base enity!");
			return;
		}
		
		ConfirmTransferMoneyToPlayer(receiverPlayer.GetIdentity(), data_receiver);
		
		string senderName = ident.GetName();
		string senderID = ident.GetId();
		string revicerName = receiverPlayer.GetIdentity().GetName();

		StringLocaliser senderText = new StringLocaliser("STR_EXPANSION_ATM_TRANSFER_SENDER", amount.ToString(), revicerName);
		StringLocaliser reciverText = new StringLocaliser("STR_EXPANSION_ATM_TRANSFER_RECEIVER", amount.ToString(), senderName);
		
		ExpansionNotification("STR_EXPANSION_MARKET_TITLE", senderText, EXPANSION_NOTIFICATION_ICON_TRADER, COLOR_EXPANSION_NOTIFICATION_SUCCESS, 3, ExpansionNotificationType.MARKET).Create(ident);
		ExpansionNotification("STR_EXPANSION_MARKET_TITLE", reciverText, EXPANSION_NOTIFICATION_ICON_TRADER, COLOR_EXPANSION_NOTIFICATION_SUCCESS, 3, ExpansionNotificationType.MARKET).Create(receiverPlayer.GetIdentity());
		
		//! This could potentaly cause an identity issue (need to test)
		ExpansionLogATM(string.Format("Player %1 (id=%2) has transfered %3 to the player %4 (id=%5).", senderName, senderID, amount.ToString(), revicerName, receiverID));
	}
	
	// ------------------------------------------------------------
	// Expansion ConfirmTransferMoneyToPlayer
	// ------------------------------------------------------------	
	void ConfirmTransferMoneyToPlayer(PlayerIdentity ident, ExpansionMarketATM_Data data)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		auto rpc = Expansion_CreateRPC("RPC_ConfirmTransferMoneyToPlayer");
		rpc.Write(data);
		rpc.Expansion_Send(true, ident);
	}
	
	// ------------------------------------------------------------
	// Expansion RPC_ConfirmTransferMoneyToPlayer
	// ------------------------------------------------------------
	private void RPC_ConfirmTransferMoneyToPlayer(PlayerIdentity senderRPC, Object target, ParamsReadContext ctx)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		if (!GetExpansionSettings().GetMarket().ATMSystemEnabled)
			return;

		ExpansionMarketATM_Data data;
		if (!ctx.Read(data))
		{
			Error("ExpansionMarketModule::RPC_ConfirmTransferMoneyToPlayer - Could not get sender player ATM data!");
			return;
		}
		
		Exec_ConfirmTransferMoneyToPlayer(data);
	}
	
	// ------------------------------------------------------------
	// Expansion Exec_ConfirmTransferMoneyToPlayer
	// ------------------------------------------------------------
	private void Exec_ConfirmTransferMoneyToPlayer(ExpansionMarketATM_Data data)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		SI_ATMMenuTransferCallback.Invoke(data);
	}
	
	#ifdef EXPANSIONMODGROUPS
	// ------------------------------------------------------------
	// Expansion RequestPartyTransferMoney
	// ------------------------------------------------------------	
	void RequestPartyTransferMoney(int amount, int partyID)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		if (!GetGame().IsDedicatedServer())
		{
			auto rpc = Expansion_CreateRPC("RPC_RequestPartyTransferMoney");
			rpc.Write(amount);
			rpc.Write(partyID);
			rpc.Expansion_Send(true);
		}
	}
	
	// ------------------------------------------------------------
	// Expansion RPC_RequestPartyTransferMoney
	// ------------------------------------------------------------
	private void RPC_RequestPartyTransferMoney(PlayerIdentity senderRPC, Object target, ParamsReadContext ctx)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		if (!GetExpansionSettings().GetMarket().ATMSystemEnabled)
			return;

		int amount;
		if (!ctx.Read(amount))
		{
			Error("ExpansionMarketModule::RPC_RequestPartyTransferMoney - Could not get amount!");
			return;
		}
		
		int partyID;
		if (!ctx.Read(partyID))
		{
			Error("ExpansionMarketModule::RPC_RequestPartyTransferMoney - Could not get party id!");
			return;
		}
		
		Exec_RequestPartyTransferMoney(amount, partyID, senderRPC);
	}
	
	// ------------------------------------------------------------
	// Expansion Exec_RequestDepositMoney
	// ------------------------------------------------------------
	private void Exec_RequestPartyTransferMoney(int amount, int partyID, PlayerIdentity ident)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		if (!ident)
		{
			Error("ExpansionMarketModule::Exec_RequestPartyTransferMoney - Could not get sender indetity!");
			return;
		}
		
		StringLocaliser title;
		StringLocaliser text;
				
		ExpansionMarketATM_Data data = GetPlayerATMData(ident.GetId());
		if (!data)
		{
			Error("ExpansionMarketModule::Exec_RequestPartyTransferMoney - Could not find player atm data!");
			
			return;
		}
		
		if (data.MoneyDeposited < amount)
		{
			Error("ExpansionMarketModule::Exec_RequestPartyTransferMoney - Tried to deposit more money then in inventory!");
			return;
		}
		
		ExpansionPartyModule module = ExpansionPartyModule.Cast(CF_ModuleCoreManager.Get(ExpansionPartyModule));
		if (!module)
		{
			Error("ExpansionMarketModule::Exec_RequestPartyTransferMoney - Could not get party party module!");
			return;
		}
		
		ExpansionPartyData party = module.GetPartyByID(partyID);
		
		if (party.GetMoneyDeposited() + amount > GetExpansionSettings().GetMarket().MaxPartyDepositMoney)
		{
			Error("ExpansionMarketModule::Exec_RequestPartyTransferMoney - Receiving party would go over max allowed deposit money value!");
			return;
		}
		
		party.AddMoney(amount);
		party.Save();

		/*int removed = RemoveMoney(player);
		if (removed - amount > 0)
		{
		    SpawnMoney(player, removed - amount);
		}*/
		
		data.RemoveMoney(amount);
		data.Save();
		
		ExpansionLogATM(string.Format("Player \"%1\" (id=%2) has deposited %3 on the party \"%4\" (partyid=%5 | ownerid=%6) ATM account.", ident.GetName(), ident.GetId(), amount, party.GetPartyName(), partyID, party.GetOwnerUID()));
		
		ConfirmPartyTransferMoney(amount, party, data, ident);
	}
	
	// ------------------------------------------------------------
	// Expansion Exec_RequestDepositMoney
	// ------------------------------------------------------------
	private void ConfirmPartyTransferMoney(int amount, ExpansionPartyData party, ExpansionMarketATM_Data data, PlayerIdentity ident)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		auto rpc = Expansion_CreateRPC("RPC_ConfirmPartyTransferMoney");
		rpc.Write(amount);
		party.OnSend(rpc, false);
		rpc.Write(data);
		rpc.Expansion_Send(true, ident);
	}
	
	// ------------------------------------------------------------
	// Expansion RPC_ConfirmPartyTransferMoney
	// ------------------------------------------------------------
	private void RPC_ConfirmPartyTransferMoney(PlayerIdentity senderRPC, Object target, ParamsReadContext ctx)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		if (!GetExpansionSettings().GetMarket().ATMSystemEnabled)
			return;

		int amount;
		if (!ctx.Read(amount))
		{
			Error("ExpansionMarketModule::RPC_ConfirmPartyTransferMoney - Could not get amount!");
			return;
		}
		
		int partyID;
		if (!ctx.Read(partyID))
		{
			Error("ExpansionMarketModule::RPC_ConfirmPartyWithdrawMoney - Could not get party ID!");
			return;
		}

		ExpansionPartyData party = new ExpansionPartyData(partyID);
		if (!party.OnRecieve(ctx))
		{
			Error("ExpansionMarketModule::RPC_ConfirmPartyTransferMoney - Could not get party data!");
			return;
		}
		
		ExpansionMarketATM_Data data;
		if (!ctx.Read(data))
		{
			Error("ExpansionMarketModule::RPC_ConfirmPartyTransferMoney - Could not get sender player ATM data!");
			return;
		}
		
		Exec_ConfirmPartyTransferMoney(amount, party, data);
	}
	
	// ------------------------------------------------------------
	// Expansion Exec_ConfirmPartyTransferMoney
	// ------------------------------------------------------------
	private void Exec_ConfirmPartyTransferMoney(int amount, ExpansionPartyData party, ExpansionMarketATM_Data data)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		SI_ATMMenuPartyCallback.Invoke(amount, party, data, 1);
	}
	
	// ------------------------------------------------------------
	// Expansion RequestPartyWithdrawMoney
	// ------------------------------------------------------------	
	void RequestPartyWithdrawMoney(int amount, int partyID)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		if (!GetGame().IsDedicatedServer())
		{
			auto rpc = Expansion_CreateRPC("RPC_RequestPartyWithdrawMoney");
			rpc.Write(amount);
			rpc.Write(partyID);
			rpc.Expansion_Send(true);
		}
	}
	
	// ------------------------------------------------------------
	// Expansion RPC_RequestPartyWithdrawMoney
	// ------------------------------------------------------------
	private void RPC_RequestPartyWithdrawMoney(PlayerIdentity senderRPC, Object target, ParamsReadContext ctx)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		if (!GetExpansionSettings().GetMarket().ATMSystemEnabled)
			return;

		int amount;
		if (!ctx.Read(amount))
		{
			Error("ExpansionMarketModule::RPC_RequestPartyWithdrawMoney - Could not get amount!");
			return;
		}
		
		int partyID;
		if (!ctx.Read(partyID))
		{
			Error("ExpansionMarketModule::RPC_RequestPartyWithdrawMoney - Could not get party id!");
			return;
		}
				
		Exec_RequestPartyWithdrawMoney(amount, partyID, senderRPC);
	}
	
	// ------------------------------------------------------------
	// Expansion Exec_RequestPartyWithdrawMoney
	// ------------------------------------------------------------
	private void Exec_RequestPartyWithdrawMoney(int amount, int partyID, PlayerIdentity ident)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		StringLocaliser title;
		StringLocaliser text;
						
		ExpansionMarketATM_Data data = GetPlayerATMData(ident.GetId());
		if (!data)
		{
			Error("ExpansionMarketModule::Exec_RequestPartyWithdrawMoney - Could not find player atm data!");
			
			return;
		}
		
		ExpansionPartyModule module = ExpansionPartyModule.Cast(CF_ModuleCoreManager.Get(ExpansionPartyModule));
		if (!module)
		{
			Error("ExpansionMarketModule::Exec_RequestPartyWithdrawMoney - Could not get party party module!");
			return;
		}
		
		ExpansionPartyData party = module.GetPartyByID(partyID);
		if (!party)
		{
			Error("ExpansionMarketModule::Exec_RequestPartyWithdrawMoney - Could not get party data!");
			return;
		}
		
		ExpansionPartyPlayerData player = party.GetPlayer(ident.GetId());
		if (!player)
		{
			Error("ExpansionMarketModule::Exec_RequestPartyWithdrawMoney - Could not get party player data!");
			return;
		}
		
		if (!player.CanWithdrawMoney())
			return;
		
		if (party.GetMoneyDeposited() < amount)
		{
			Error("ExpansionMarketModule::Exec_RequestPartyWithdrawMoney - Tried to withdraw more money than in account!");
			return;
		}
		
		if (data.MoneyDeposited + amount > GetExpansionSettings().GetMarket().MaxDepositMoney)
		{
			Error("ExpansionMarketModule::Exec_RequestPartyWithdrawMoney - Receiving player would go over max allowed deposit money value!");
			return;
		}
		
		party.RemoveMoney(amount);
		party.Save();

		/*int removed = RemoveMoney(player);
		if (removed - amount > 0)
		{
		    SpawnMoney(player, removed - amount);
		}*/
		
		data.AddMoney(amount);
		data.Save();
		
		ExpansionLogATM(string.Format("Player \"%1\" (id=%2) has withdrawn %3 from the party \"%4\" (partyid=%5 | ownerid=%6) ATM account.", ident.GetName(), ident.GetId(), amount, party.GetPartyName(), partyID, party.GetOwnerUID()));
		
		ConfirmPartyWithdrawMoney(amount, party, data, ident);
	}
	
	// ------------------------------------------------------------
	// Expansion Exec_RequestWithdrawMoney
	// ------------------------------------------------------------
	private void ConfirmPartyWithdrawMoney(int amount, ExpansionPartyData party, ExpansionMarketATM_Data data, PlayerIdentity ident)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		auto rpc = Expansion_CreateRPC("RPC_ConfirmPartyWithdrawMoney");
		rpc.Write(amount);
		party.OnSend(rpc, false);
		rpc.Write(data);
		rpc.Expansion_Send(true, ident);
	}
	
	// ------------------------------------------------------------
	// Expansion RPC_ConfirmPartyWithdrawMoney
	// ------------------------------------------------------------
	private void RPC_ConfirmPartyWithdrawMoney(PlayerIdentity senderRPC, Object target, ParamsReadContext ctx)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		if (!GetExpansionSettings().GetMarket().ATMSystemEnabled)
			return;

		int amount;
		if (!ctx.Read(amount))
		{
			Error("ExpansionMarketModule::RPC_ConfirmPartyWithdrawMoney - Could not get amount!");
			return;
		}
		
		int partyID;
		if (!ctx.Read(partyID))
		{
			Error("ExpansionMarketModule::RPC_ConfirmPartyWithdrawMoney - Could not get party ID!");
			return;
		}

		ExpansionPartyData party = new ExpansionPartyData(partyID);
		if (!party.OnRecieve(ctx))
		{
			Error("ExpansionMarketModule::RPC_ConfirmPartyWithdrawMoney - Could not get party data!");
			return;
		}
		
		ExpansionMarketATM_Data data;
		if (!ctx.Read(data))
		{
			Error("ExpansionMarketModule::RPC_ConfirmPartyWithdrawMoney - Could not get sender player ATM data!");
			return;
		}
		
		Exec_ConfirmPartyWithdrawMoney(amount, party, data);
	}
	
	// ------------------------------------------------------------
	// Expansion Exec_ConfirmPartyWithdrawMoney
	// ------------------------------------------------------------
	private void Exec_ConfirmPartyWithdrawMoney(int amount, ExpansionPartyData party, ExpansionMarketATM_Data data)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.MARKET, this);
#endif

		SI_ATMMenuPartyCallback.Invoke(amount, party, data, 2);
	}
	#endif
	
	// ------------------------------------------------------------
	// Expansion RemoveMoney
	// ------------------------------------------------------------
	bool RemoveMoney(int amount, PlayerBase player, TStringArray currencies = NULL)
	{
		if (currencies == NULL)
			currencies = GetExpansionSettings().GetMarket().Currencies;

		if (!currencies.Count())
		{
			Error(ToString() + "::RemoveMoney - No currencies defined in market settings!");
			return false;
		}

		array<int> monies = new array<int>;
		if (!FindMoneyInCurrency(player, amount, monies, currencies, true))
		{
			Error(ToString() + "::RemoveMoney - Could not find player money!");
			UnlockMoney(player);
			return false;
		}

		EntityAI parent = player;
		int removed = RemoveMoney(player);
		if (removed - amount > 0)
		{
		    SpawnMoneyInCurrency(player, parent, removed - amount, currencies, true);
			CheckSpawn(player, parent);
		}
		else if (removed != amount)
		{
			return false;
		}

		return true;
	}
	
	// ------------------------------------------------------------
	// Expansion ExpansionLogMarket
	// ------------------------------------------------------------
	private void ExpansionLogMarket(string message)
	{
		if (GetExpansionSettings().GetLog().Market)
			GetExpansionSettings().GetLog().PrintLog("[Market] " + message);
	}
	
	// ------------------------------------------------------------
	// Expansion ExpansionLogATM
	// ------------------------------------------------------------
	private void ExpansionLogATM(string message)
	{
		if (GetExpansionSettings().GetLog().ATM)
			GetExpansionSettings().GetLog().PrintLog("[ATM] " + message);
	}
}
