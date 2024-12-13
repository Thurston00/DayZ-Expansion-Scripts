/**
 * ExpansionP2PMarketModule.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

enum ExpansionP2PMarketModuleCallback
{
	Silent,  //! Silent callback w/o notification (just used to update menu)
	ItemListed,
	ItemPurchased,
	SaleRetrieved,
	AllSalesRetrieved,
	MsgItemGotSold,
	MsgTotalSold,
	Error,
	ErrorListingPriceTooLow,
	ErrorNotEnoughMoney,
	ErrorNotEnoughMoneyToList,
	ErrorVehicleMissingAttachment,
	ErrorVehicleRuinedAttachment,
	ErrorNoVehicleSpawnPosition,
	ErrorVehicleSpawnPositionBlocked,
	ErrorVehicleLockpicked
};

class ExpansionP2PMarketPlayerInventory extends ExpansionMarketPlayerInventory
{
	override bool IsVehicleNearby(Object vehicle)
	{
		float maxDistance = 150.0;
		if (vector.Distance(m_Player.GetPosition(), vehicle.GetPosition()) <= maxDistance)
			return true;
		return false;
	}
};

[CF_RegisterModule(ExpansionP2PMarketModule)]
class ExpansionP2PMarketModule: CF_ModuleWorld
{
	protected static ExpansionP2PMarketModule s_Instance;
	static string s_P2PMarketConfigFolderPath = "$mission:expansion\\p2pmarket\\";

	protected static ref TStringArray s_DoorSlotKeywords = {"door", "hood", "trunk", "dver", "kapot", "bagazhnik"};
	protected static ref TStringArray s_WheelSlotKeywords = {"wheel", "koleso"};

	protected bool m_Initialized;
	protected float m_CheckListingsTime;
	protected const float CHECK_TICK_TIME = 60.0;
	protected ref ExpansionMarketModule m_MarketModule;

	//! Server
	protected ref map<int, ref ExpansionP2PMarketTraderConfig> m_P2PTraderConfig = new map<int, ref ExpansionP2PMarketTraderConfig>;
	protected ref map<int, ref array<ref ExpansionP2PMarketListing>> m_P2PListingsData = new map<int, ref array<ref ExpansionP2PMarketListing>>;
	protected ref map<string, int> m_TradingPlayers = new map<string, int>;
	
	//! Client
	protected ref ExpansionP2PMarketPlayerInventory m_LocalEntityInventory;
	protected ref ScriptInvoker m_P2PMarketMenuListingsInvoker; //! Client
	protected ref ScriptInvoker m_P2PMarketMenuCallbackInvoker; //! Client

	static ref TStringArray m_HardcodedExcludes = {"AugOptic", "Magnum_Cylinder", "Magnum_Ejector", "M97DummyOptics"};

	void ExpansionP2PMarketModule()
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
#endif 
				
		s_Instance = this;
	}

	override void OnInit()
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
#endif 
			
		EnableMissionStart();
		EnableMissionLoaded();
		EnableInvokeConnect();
		#ifdef SERVER
		EnableUpdate();
		#endif
		Expansion_EnableRPCManager();
		
		Expansion_RegisterServerRPC("RPC_RemoveTradingPlayer");
		Expansion_RegisterServerRPC("RPC_RequestSaleFromListing");
		Expansion_RegisterServerRPC("RPC_RequestAllPlayerSales");
		Expansion_RegisterServerRPC("RPC_RequestBMTraderData");
		Expansion_RegisterClientRPC("RPC_SendBMTraderData");
		Expansion_RegisterServerRPC("RPC_RequestListBMItem");
		Expansion_RegisterClientRPC("RPC_Callback");
		Expansion_RegisterServerRPC("RPC_RequestPurchaseBMItem");
	}

	protected void CreateDirectoryStructure()
	{
		if (!FileExist(s_P2PMarketConfigFolderPath))
			ExpansionStatic.MakeDirectoryRecursive(s_P2PMarketConfigFolderPath);

		if (!FileExist(GetP2PMarketDataDirectory()))
			ExpansionStatic.MakeDirectoryRecursive(GetP2PMarketDataDirectory());
	}

	override void OnMissionStart(Class sender, CF_EventArgs args)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
#endif 
				
		#ifdef SERVER
			if (GetExpansionSettings().GetP2PMarket().Enabled)
			{
				CreateDirectoryStructure();
				LoadP2PMarketServerData();

				m_Initialized = true;
			}
		#endif
	}

	override void OnMissionLoaded(Class sender, CF_EventArgs args)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
#endif
		
		if (GetGame().IsServer() && GetGame().IsMultiplayer())
			ServerModuleInit();

		if (GetGame().IsClient())
			ClientModuleInit();
	}

	override void OnInvokeConnect(Class sender, CF_EventArgs args)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
#endif 
		
		super.OnInvokeConnect(sender, args);

		auto cArgs = CF_EventPlayerArgs.Cast(args);

		if (GetGame().IsServer() && GetGame().IsMultiplayer())
		{
			int moneyFromSales;
			int salesCount;
			foreach (int traderID, array<ref ExpansionP2PMarketListing>> listings: m_P2PListingsData)
			{
				foreach (ExpansionP2PMarketListing traderListing: listings)
				{
					if (traderListing.GetListingState() != ExpansionP2PMarketListingState.SOLD || traderListing.GetOwnerUID() != cArgs.Identity.GetId())
						continue;

					moneyFromSales += traderListing.GetPrice();
					salesCount++;
				}
			}

			if (moneyFromSales > 0)
			{
				Callback(cArgs.Identity, ExpansionP2PMarketModuleCallback.MsgTotalSold, "", moneyFromSales, salesCount);
			}
		}
	}

	protected void ServerModuleInit()
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
#endif 
		
		//! Server only
		if (GetGame().IsServer() && GetGame().IsMultiplayer())
		{
			m_MarketModule = ExpansionMarketModule.Cast(CF_ModuleCoreManager.Get(ExpansionMarketModule));
			
			foreach (ExpansionP2PMarketTraderConfig config: m_P2PTraderConfig)
			{
				//! Spawn NPCs late so mapping already loaded
				config.Spawn();
			}
		}
	}

	protected void ClientModuleInit()
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
#endif 
		
		if (GetGame().IsClient())
		{
			m_P2PMarketMenuListingsInvoker = new ScriptInvoker();
			m_P2PMarketMenuCallbackInvoker = new ScriptInvoker();
		}
	}

	protected void LoadP2PMarketServerData()
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
#endif 
		
		//! Move existing configs (if any) from old to new location
		string dataDir = GetP2PMarketDataDirectory();
		array<string> p2pMarketFilesExisting = ExpansionStatic.FindFilesInLocation(dataDir, ".json");
		foreach (string existingFile: p2pMarketFilesExisting)
		{
			ExpansionStatic.CopyFileOrDirectoryTree(dataDir + existingFile, s_P2PMarketConfigFolderPath + existingFile, "", true);
		}

		array<string> p2pMarketFiles = ExpansionStatic.FindFilesInLocation(s_P2PMarketConfigFolderPath, ".json");
		if (p2pMarketFiles.Count() > 0)
		{
			foreach (string fileName: p2pMarketFiles)
			{
				LoadP2PMarketTraderData(fileName, s_P2PMarketConfigFolderPath);
			}
		}
		else
		{
			CreateDefaultP2PTraderConfig();
		}
	}

	protected void CreateDefaultP2PTraderConfig()
	{
		string worldname;
		GetGame().GetWorldName(worldname);
		worldname.ToLower();

		vector mapPos = GetDayZGame().GetWorldCenterPosition();
		bool addToConfigArray = false;
		ExpansionP2PMarketTraderConfig bmTrader01;
		
		bmTrader01 = new ExpansionP2PMarketTraderConfig();
		bmTrader01.SetID(1);
	#ifdef EXPANSIONMODAI
		bmTrader01.SetClassName("ExpansionP2PTraderAIIrena");
	#else
		bmTrader01.SetClassName("ExpansionP2PTraderIrena");
	#endif
		bmTrader01.SetLoadoutFile("YellowKingLoadout");
		bmTrader01.AddCurrency("expansionbanknotehryvnia");

		bmTrader01.m_RequiredFaction = "";
		bmTrader01.m_UseReputation = false;
		bmTrader01.m_MinRequiredReputation = 0;
		bmTrader01.m_MaxRequiredReputation = int.MAX;
		bmTrader01.m_RequiredCompletedQuestID = -1;
		
		if (worldname.IndexOf("chernarus") > -1)
		{
			bmTrader01.SetPosition(Vector(3697.77, 402.012, 5971.12));
			bmTrader01.SetOrientation(Vector(150.132, 0, 0));
			bmTrader01.SetVehicleSpawnPosition(Vector(3728.44, 401.666, 6011.51));
			addToConfigArray = true;
		}
		else if (worldname.IndexOf("namalsk") > -1)
		{
			bmTrader01.SetPosition(Vector(3696.6, 402.012, 5970.54));
			bmTrader01.SetOrientation(Vector(156.132, 0, -0));
			bmTrader01.SetVehicleSpawnPosition(Vector(3741.68, 402.833, 5996.14));
			addToConfigArray = true;
		}
		else
		{
			//! @note: NPC entity is not spawned here as its just a config template.
			bmTrader01.SetPosition(mapPos);
			bmTrader01.SetOrientation(Vector(0, 0, 0));
			bmTrader01.SetVehicleSpawnPosition(mapPos);
		}
		
		bmTrader01.Save();
		if (addToConfigArray)
			m_P2PTraderConfig.Insert(1, bmTrader01);
	}

	protected void LoadP2PMarketTraderData(string fileName, string path)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
#endif 
		
		ExpansionP2PMarketTraderConfig traderConfig = ExpansionP2PMarketTraderConfig.Load(path + fileName);
		if (!traderConfig)
			return;

		if (m_P2PTraderConfig.Contains(traderConfig.GetID()))
			return;

		m_P2PTraderConfig.Insert(traderConfig.GetID(), traderConfig);

		int traderID = traderConfig.GetID();
		string traderListingsPath = GetP2PMarketDataDirectory() + "P2PTrader_" + traderID + "_Listings\\";
		if (!FileExist(traderListingsPath))
			return;

		array<string> traderListings = ExpansionStatic.FindFilesInLocation(traderListingsPath, ".json");
		if (traderListings && traderListings.Count() > 0)
		{
			foreach (string listingFileName: traderListings)
			{
				if (!FileExist(traderListingsPath + listingFileName))
					continue;

				LoadListingData(traderID, listingFileName, traderListingsPath);
			}
		}
	}

	protected void LoadListingData(int traderID, string fileName, string path)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
#endif 
			
		ExpansionP2PMarketListing listingData = ExpansionP2PMarketListing.Load(path + fileName);
		if (!listingData)
			return;

		//! Check if the entity storage file still exists if the listing sate is not SOLD otherwise we delete the listing file and dont add it to the system.
		if (listingData.GetListingState() == ExpansionP2PMarketListingState.LISTED)
		{
			P2PDebugPrint("::LoadListingData - Check if entity storage file still exists: " + listingData.GetEntityStorageFileName());
			if (!FileExist(listingData.GetEntityStorageFileName()))
			{
				P2PDebugPrint("::LoadListingData - Entity stoage file " + listingData.GetEntityStorageFileName() + " does not exist anymore. Delete listing JSON..");
				DeleteFile(path + fileName); //! Delete the listing JSON file.
				listingData = null;
				return;
			}
		}

		listingData.SetTraderID(traderID);
		array<ref ExpansionP2PMarketListing> listings;
		if (m_P2PListingsData.Find(traderID, listings))
		{
			listings.Insert(listingData);
		}
		else
		{
			listings = new array<ref ExpansionP2PMarketListing>;
			listings.Insert(listingData);
			m_P2PListingsData.Insert(traderID, listings);
		}
	}

	// ------------------------------------------------------------------------------------------------------------------------
	//! RPC Event classes
	// ------------------------------------------------------------------------------------------------------------------------
	//! Server
	void AddTradingPlayer(int traderID, string playerUID)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
#endif 
		
		int id;
		if (!m_TradingPlayers.Find(playerUID, id))
			m_TradingPlayers.Insert(playerUID, traderID);
	}

	//! Client
	void RemoveTradingPlayer(string playerUID)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
#endif 
		
		auto rpc = Expansion_CreateRPC("RPC_RemoveTradingPlayer");
		rpc.Write(playerUID);
		rpc.Expansion_Send(true);
	}

	//! Server
	protected void RPC_RemoveTradingPlayer(PlayerIdentity identity, Object target, ParamsReadContext ctx)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
#endif 
		
		string playerUID;
		if (!ctx.Read(playerUID))
			return;

		int traderID;
		if (m_TradingPlayers.Find(playerUID, traderID))
			m_TradingPlayers.Remove(playerUID);
	}

	//! Server
	void SendUpdatedTraderData(int traderID, string senderUID, ExpansionP2PMarketModuleCallback callback = 0, string type = string.Empty, int price = 0)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
#endif 
		
		ExpansionP2PMarketModuleCallback menuCallback;
		foreach (string playerUID, int id: m_TradingPlayers)
		{
			if (id != traderID)
				continue;

			PlayerBase player = PlayerBase.GetPlayerByUID(playerUID);
			if (!player)
				continue;

			if (playerUID == senderUID)
				menuCallback = callback;
			else
				menuCallback = ExpansionP2PMarketModuleCallback.Silent;

			SendBMTraderData(traderID, player.GetIdentity(), "", "", menuCallback, type, price);
		}
	}
	
	//! Client
	void GetSaleFromListing(ExpansionP2PMarketListing listing, int traderID)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
#endif 
		
		TIntArray globalID = listing.GetGlobalID();

		auto rpc = Expansion_CreateRPC("RPC_RequestSaleFromListing");
		rpc.Write(traderID);
		rpc.Write(globalID);
		rpc.Expansion_Send(true);
	}
	
	//! Server
	protected void RPC_RequestSaleFromListing(PlayerIdentity identity, Object target, ParamsReadContext ctx)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
#endif 
		
		int traderID;
		if (!ctx.Read(traderID))
		{
			Error(ToString() + "::RPC_RequestSaleFromListing - couldn't read trader ID!");
			return;
		}

		if (traderID == -1)
		{
			Error(ToString() + "::RPC_RequestSaleFromListing - Trader ID is -1 (configuration error)");
			ExpansionNotification("RPC_RequestSaleFromListing", "Trader ID is -1 (configuration error)").Error(identity);
			return;
		}
		
		ExpansionP2PMarketTraderConfig traderConfig = GetP2PTraderConfigByID(traderID);
		if (!traderConfig)
		{
			Error(ToString() + "::RPC_RequestSaleFromListing - Could not get P2P trader data for ID " + traderID);
			return;
		}

		TIntArray globalID;
		if (!ctx.Read(globalID))
		{
			Error(ToString() + "::RPC_RequestSaleFromListing - couldn't read global ID!");
			return;
		}

		string playerUID = identity.GetId();
		PlayerBase player = PlayerBase.ExpansionGetPlayerByIdentity(identity);
		if (!player)
			return;

		ExpansionP2PMarketListing listing = GetListingByGlobalID(traderID, globalID, traderConfig.IsGlobalTrader());
		string globalIDText = ExpansionStatic.IntToHex(globalID);
		if (!listing)
		{
			Error(ToString() + "::RPC_RequestSaleFromListing - couldn't find listing with global ID " + globalIDText);
			ExpansionNotification("RPC_RequestSaleFromListing", "Couldn't find listing with global ID " + globalIDText).Error(identity);
			return;
		}

		if (listing.GetListingState() != ExpansionP2PMarketListingState.SOLD)
		{
			EXPrint(ToString() + " ::RPC_RequestSaleFromListing - player " + playerUID + " tried to retrieve profits from a listing that is not yet sold at trader ID " + traderID);
			ExpansionNotification("RPC_RequestSaleFromListing", "This listing has not yet been sold").Error(identity);
			return;
		}

		if (listing.GetOwnerUID() != playerUID)
		{
			EXPrint(ToString() + " ::RPC_RequestSaleFromListing - player " + playerUID + " tried to retrieve profits from a listing that is not his own at trader ID " + traderID);
			ExpansionNotification("RPC_RequestSaleFromListing", "You can't retrieve another player's profits").Error(identity);
			return;
		}

		int price = listing.GetPrice();
		if (price <= 0)
		{
			EXPrint(ToString() + " ::RPC_RequestSaleFromListing - player " + playerUID + " tried to retrieve profits from a listing that has a price of 0. Price: " + price);
			ExpansionNotification("RPC_RequestSaleFromListing", "You can't retrieve profits from a listing with a price of zero.").Error(identity);
			return;
		}
	
		string className = listing.GetClassName();
		if (className == "")
		{
			EXPrint(ToString() + " ::RPC_RequestSaleFromListing - player " + playerUID + " tried to retrieve profits from a listing that has no type name. Class name: " + className);
			ExpansionNotification("RPC_RequestSaleFromListing", "You can't retrieve profits from a listing with no associated item(s).").Error(identity);
			return;
		}

		if (!RemoveListingByGlobalID(traderID, globalID, traderConfig.IsGlobalTrader()))
		{
			EXPrint(ToString() + " ::RPC_RequestSaleFromListing - could not remove listing " + globalIDText);
			ExpansionNotification("RPC_RequestSaleFromListing", "Could not remove listing " + globalIDText).Error(identity);
			return;
		}

		EntityAI playerEntity = player;
		TStringArray currencies = traderConfig.GetCurrencies();
		m_MarketModule.SpawnMoneyInCurrency(player, playerEntity, price, currencies, false);

		SendBMTraderData(traderID, identity, "", "", ExpansionP2PMarketModuleCallback.SaleRetrieved, className, price);
	
		if (GetExpansionSettings().GetLog().Market)
		{
			string priceStringLog = string.Format("%1 (%2)", price, GetDisplayPrice(traderConfig, price, false, false, true));
			GetExpansionSettings().GetLog().PrintLog("[P2P Market] Player \"" + identity.GetName() + "\" (id=" + identity.GetId() + ")" + " has retrieved the sale of \"" + className + "\" for a price of " + priceStringLog + " (globalID=" + globalIDText + ")");
		}
	}
	
	//! Client
	void RequestAllPlayerSales(int traderID)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
#endif 
		
		auto rpc = Expansion_CreateRPC("RPC_RequestAllPlayerSales");
		rpc.Write(traderID);
		rpc.Expansion_Send(true);
	}

	//! Server
	protected void RPC_RequestAllPlayerSales(PlayerIdentity identity, Object target, ParamsReadContext ctx)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
#endif 
		
		int traderID;
		if (!ctx.Read(traderID))
			return;

		if (traderID == -1)
		{
			Error(ToString() + "::RPC_RequestAllPlayerSales - Trader ID is -1 (configuration error)");
			ExpansionNotification("RPC_RequestAllPlayerSales", "Trader ID is -1 (configuration error)").Error(identity);
			return;
		}

		string playerUID = identity.GetId();
		PlayerBase player = PlayerBase.Cast(identity.GetPlayer());
		if (!player)
			return;
		
		ExpansionP2PMarketTraderConfig traderConfig = GetP2PTraderConfigByID(traderID);
		if (!traderConfig)
		{
			Error(ToString() + "::RPC_RequestAllPlayerSales - Could not get P2P trader data for ID " + traderID);
			return;
		}

		map<int, ref array<ref ExpansionP2PMarketListing>> listingsData;
		if (!traderConfig.IsGlobalTrader())
		{
			//! If it's not a global trader, we have one entry for this trader + its listings
			listingsData = new map<int, ref array<ref ExpansionP2PMarketListing>>;
			listingsData[traderID] = m_P2PListingsData[traderID];
			if (!listingsData[traderID])
			{
				Error(ToString() + "::RPC_RequestAllPlayerSales - No listings for trader ID " + traderID);
				ExpansionNotification("RPC_RequestAllPlayerSales", "No listings for trader ID " + traderID).Error(identity);
				return;
			}
		}
		else
		{
			//! If it's a global trader, we have an entry for each trader + its listings
			listingsData = m_P2PListingsData;
		}

		int sold;
		int price;

		foreach (int listingsTraderID, array<ref ExpansionP2PMarketListing> traderListings: listingsData)
		{
			for (int j = traderListings.Count() - 1; j >= 0; j--)
			{
				ExpansionP2PMarketListing listing = traderListings[j];
				if (listing.GetListingState() == ExpansionP2PMarketListingState.SOLD && listing.GetOwnerUID() == playerUID)
				{
					if (RemoveListing(listingsTraderID, traderListings, j))
					{
						sold++;
						price += listing.GetPrice();
					}
					else 
					{
						string globalIDText = listing.GetEntityStorageBaseName();	//! @note: For logging purposes only
						Error(ToString() + "::RPC_RequestAllPlayerSales - could not remove listing " + globalIDText + " from trader ID " + listingsTraderID);
						ExpansionNotification("RPC_RequestAllPlayerSales", "Could not remove listing " + globalIDText + " from trader ID " + listingsTraderID).Error(identity);
						return;
					}
				}
			}
		}

		if (sold == 0)
		{
			EXPrint(ToString() + " ::RPC_RequestAllPlayerSales - No listings in SOLD state for player " + playerUID + " at trader ID " + traderID);
			ExpansionNotification("RPC_RequestAllPlayerSales", "You have no sold listings at this trader").Error(identity);
			return;
		}

		EntityAI playerEntity = player;
		TStringArray currencies = traderConfig.GetCurrencies();
		m_MarketModule.SpawnMoneyInCurrency(player, playerEntity, price, currencies, false);

		SendBMTraderData(traderID, identity, "", "", ExpansionP2PMarketModuleCallback.AllSalesRetrieved, "", price);
	
		if (GetExpansionSettings().GetLog().Market)
		{
			string priceStringLog = string.Format("%1 (%2)", price, GetDisplayPrice(traderConfig, price, false, false, true));
			GetExpansionSettings().GetLog().PrintLog("[P2P Market] Player \"" + identity.GetName() + "\" (id=" + identity.GetId() + ")" + " has retrieved " + sold + " sales for a total of " + priceStringLog + ".");
		}
	}

	//! Client
	void RequestSendBMTraderData(int traderID)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
#endif 
		
		auto rpc = Expansion_CreateRPC("RPC_RequestBMTraderData");
		rpc.Write(traderID);
		rpc.Expansion_Send(true);
	}

	//! Server
	protected void RPC_RequestBMTraderData(PlayerIdentity identity, Object target, ParamsReadContext ctx)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
#endif 
		
		int traderID = -1;
		if (!ctx.Read(traderID))
			return;

		SendBMTraderData(traderID, identity);
	}

	//! Server
	void SendBMTraderData(int traderID, PlayerIdentity identity, string traderName = string.Empty, string iconName = string.Empty, ExpansionP2PMarketModuleCallback callback = 0, string type = string.Empty, int price = 0)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
#endif 
		
		ExpansionP2PMarketTraderConfig traderConfig = GetP2PTraderConfigByID(traderID);
		if (!traderConfig)
		{
			Error(ToString() + "::SendBMTraderData - Could not get P2P trader data for ID " + traderID);
			return;
		}

		int listingsCount;
		array<ref ExpansionP2PMarketListing> listings;
		if (!traderConfig.IsGlobalTrader())
		{
			listings = m_P2PListingsData[traderID];
			if (!listings)
				listings = new array<ref ExpansionP2PMarketListing>;
		}
		else
		{
			listings = new array<ref ExpansionP2PMarketListing>;
			foreach (int listingsTraderID, array<ref ExpansionP2PMarketListing> traderListings: m_P2PListingsData)
			{
				foreach (auto traderListing: traderListings)
				{
					listings.Insert(traderListing);
				}
			}
		}

		P2PDebugPrint(ToString() + "::SendBMTraderData - global: " + traderConfig.IsGlobalTrader());
		P2PDebugPrint(ToString() + "::SendBMTraderData - listings count: " + listings.Count());

		TStringArray currencies = traderConfig.GetCurrencies();

		auto rpc = Expansion_CreateRPC("RPC_SendBMTraderData");
		rpc.Write(traderID);
		rpc.Write(listings.Count());

		foreach (auto listing: listings)
		{
			listing.OnSend(rpc);
		}

		rpc.Write(traderName);
		rpc.Write(iconName);
		rpc.Write(currencies);
		rpc.Write(traderConfig.m_DisplayCurrencyValue);
		rpc.Write(traderConfig.m_DisplayCurrencyName);
		rpc.Write(callback);
		rpc.Write(type);
		rpc.Write(price);
		rpc.Expansion_Send(true, identity);
	}

	//! Client
	protected void RPC_SendBMTraderData(PlayerIdentity identity, Object target, ParamsReadContext ctx)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
#endif 
		
		int traderID;
		if (!ctx.Read(traderID))
		{
			Error(ToString() + "::RPC_SendBMTraderData - couldn't read trader ID");
			return;
		}

		int listingsCount;
		if (!ctx.Read(listingsCount))
		{
			Error(ToString() + "::RPC_SendBMTraderData - couldn't read listing count");
			return;
		}

		array<ref ExpansionP2PMarketListing> listings = new array<ref ExpansionP2PMarketListing>;
		for (int i = 0; i < listingsCount; ++i)
		{
			ExpansionP2PMarketListing listing = new ExpansionP2PMarketListing();
			if (!listing.OnRecieve(ctx))
			{
				Error(ToString() + "::RPC_SendBMTraderData - couldn't receive listing " + i);
				return;
			}

			listings.Insert(listing);
		}

		string traderName;
		if (!ctx.Read(traderName))
		{
			Error(ToString() + "::RPC_SendBMTraderData - couldn't read trader name");
			return;
		}

		string iconName;
		if (!ctx.Read(iconName))
		{
			Error(ToString() + "::RPC_SendBMTraderData - couldn't read trader icon");
			return;
		}

		TStringArray currencies;
		if (!ctx.Read(currencies))
		{
			Error(ToString() + "::RPC_SendBMTraderData - couldn't read trader currencies");
			return;
		}

		int displayCurrencyValue;
		if (!ctx.Read(displayCurrencyValue))
		{
			Error(ToString() + "::RPC_SendBMTraderData - couldn't read trader currency value");
			return;
		}

		string displayCurrencyName;
		if (!ctx.Read(displayCurrencyName))
		{
			Error(ToString() + "::RPC_SendBMTraderData - couldn't read trader currency name");
			return;
		}

		int callback;
		if (!ctx.Read(callback))
		{
			Error(ToString() + "::RPC_Callback - couldn't read callback");
			return;
		}

		string type;
		if (!ctx.Read(type))
		{
			Error(ToString() + "::RPC_Callback - couldn't read type");
			return;
		}

		int price;
		if (!ctx.Read(price))
		{
			Error(ToString() + "::RPC_Callback - couldn't read price");
			return;
		}

		m_P2PMarketMenuListingsInvoker.Invoke(listings, traderID, traderName, iconName, currencies, displayCurrencyValue, displayCurrencyName);

		m_P2PMarketMenuCallbackInvoker.Invoke(callback, type, price, 0, null);
	}

	//! Client
	void RequestListBMItem(int traderID, Entity item, int price)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
#endif 
		
		auto rpc = Expansion_CreateRPC("RPC_RequestListBMItem");
		rpc.Write(traderID);
		rpc.Write(price);
		rpc.Expansion_Send(item, true);
	}

	//! Server
	protected void RPC_RequestListBMItem(PlayerIdentity identity, Object target, ParamsReadContext ctx)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
#endif 
		
		auto settings = GetExpansionSettings().GetP2PMarket();
		int playerListingsCount = GetPlayerListingsCount(identity.GetId());
		if (settings.MaxListings != -1 && playerListingsCount >= settings.MaxListings)
		{
			ExpansionNotification(new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_MAX_LISTING_TITLE"), new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_MAX_LISTING_DESC", playerListingsCount.ToString(), settings.MaxListings.ToString()), ExpansionIcons.GetPath("Exclamationmark"), COLOR_EXPANSION_NOTIFICATION_ERROR, 7, ExpansionNotificationType.TOAST).Create(identity);
			CallbackError(identity);
			return;
		}

		int traderID;
		if (!ctx.Read(traderID))
		{
			Error(ToString() + "::RPC_RequestListBMItem - Could not read traderID.");
			return;
		}

		int price;
		if (!ctx.Read(price))
		{
			Error(ToString() + "::RPC_RequestListBMItem - Could not read price.");
			return;
		}

		EntityAI objEntity;
		if (!Class.CastTo(objEntity, target))
		{
			Error(ToString() + "::RPC_RequestListBMItem - Could not get target object.");
			return;
		}

		if (objEntity.IsSetForDeletion())
		{
			Error(ToString() + "::RPC_RequestListBMItem - target is about to be deleted.");
			return;
		}

		PlayerBase player = PlayerBase.Cast(identity.GetPlayer());
		if (!player)
		{
			Error(ToString() + "::RPC_RequestListBMItem - Could not get player.");
			return;
		}
		
		ExpansionP2PMarketTraderConfig traderConfig = GetP2PTraderConfigByID(traderID);
		if (!traderConfig)
		{
			Error(ToString() + "::RPC_RequestListBMItem - Could not get P2P trader data for ID " + traderID);
			return;
		}

		string priceString = GetDisplayPrice(traderConfig, price, false, true, true);

		string type = objEntity.GetType();
		int listingPrice = Math.Ceil(price * settings.ListingPricePercent / 100);
		string listingPriceString = GetDisplayPrice(traderConfig, listingPrice, false, true, true);
		if (listingPrice <= 0)
		{
			CallbackError(identity, ExpansionP2PMarketModuleCallback.ErrorListingPriceTooLow, type, price, listingPrice);
			return;
		}

		array<int> monies = new array<int>;
		TStringArray currencies = traderConfig.GetCurrencies();
		int playerWorth = m_MarketModule.GetPlayerWorth(player, monies, currencies);
		if (playerWorth <= 0 || playerWorth < listingPrice)
		{
			CallbackError(identity, ExpansionP2PMarketModuleCallback.ErrorNotEnoughMoneyToList, type, price, listingPrice);
			return;
		}

		int doorsCount;
		int doorsRequiredAmount;
		int wheelsCount;
		int wheelsRequiredAmount;
		array<EntityAI> slotItems;
		//! If the object is a vehicle we check for paired keys and unpair/delete them all and remove the pairing from the vehicle.
		ExpansionVehicle vehicle;
		if (ExpansionVehicle.Get(vehicle, objEntity))
		{
		#ifdef EXPANSIONMODVEHICLE
			if (vehicle.GetLockState() == ExpansionVehicleLockState.FORCEDUNLOCKED)
			{
				CallbackError(identity, ExpansionP2PMarketModuleCallback.ErrorVehicleLockpicked, type);
				return;
			}
		#endif

			slotItems = GetSlotItems(vehicle.GetEntity(), doorsRequiredAmount, wheelsRequiredAmount);

			foreach (EntityAI slotItemCar: slotItems)
			{
				EXPrint(ToString() + "::RPC_RequestListBMItem - " + vehicle.GetType() + " | Slots Item: " + slotItemCar.GetType());
				if (slotItemCar.IsInherited(CarDoor))
					doorsCount++;
				else if (slotItemCar.IsInherited(CarWheel))
					wheelsCount++;
			}

			if (doorsCount < doorsRequiredAmount || wheelsCount < wheelsRequiredAmount || !vehicle.IsFunctional(true))
			{
				CallbackError(identity, ExpansionP2PMarketModuleCallback.ErrorVehicleMissingAttachment, type);
				return;
			}

			if (!CheckItemsCondition(slotItems))
			{
				CallbackError(identity, ExpansionP2PMarketModuleCallback.ErrorVehicleRuinedAttachment, type);
				return;
			}

		#ifdef EXPANSIONMODVEHICLE
			if (vehicle.HasKey())
			{
				array<ExpansionCarKey> carKeys = new array<ExpansionCarKey>;
				ExpansionCarKey.GetKeysForVehicle(vehicle, carKeys);

				for (int i = 0; i < carKeys.Count(); ++i)
				{
					ExpansionCarKey carKey = carKeys[i];
					if (!carKey)
						continue;

					carKey.Unpair(true);
					GetGame().ObjectDelete(carKey);
				}

				vehicle.ResetKeyPairing();
			}
		#endif
		}

	#ifdef SERVER
		if (settings.DisallowUnpersisted && !ExpansionWorld.IsStoreLoaded(objEntity) && !ExpansionWorld.IsStoreSaved(objEntity))
		{
			ExpansionNotification("STR_EXPANSION_MARKET_TITLE", string.Format("You can't list %1 right now because it hasn't been persisted to game storage yet. Please wait at least 15-20 seconds.", objEntity.GetDisplayName())).Error(player.GetIdentity());
			return;
		}
	#endif

		ExpansionP2PMarketListing newListing = new ExpansionP2PMarketListing();
		newListing.SetFromItem(objEntity, player);
		if (!newListing.IsGlobalIDValid())
		{
			Error(ToString() + "::RPC_RequestListBMItem - Global ID for new listing is invalid! Global ID: " + newListing.GetGlobalID());
			return;
		}

		newListing.SetPrice(price);
		newListing.SetListingTime();
		newListing.SetTraderID(traderID);
		newListing.SetListingState(ExpansionP2PMarketListingState.LISTED);

		if (!m_MarketModule.RemoveMoney(listingPrice, player, currencies))
		{
			Error(ToString() + "::RPC_RequestListBMItem - Could not remove money from player!");
			return;
		}
		
		if (!StoreItem(newListing, objEntity))
		{
			Error(ToString() + "::RPC_RequestListBMItem - Could not store listing item!");
			return;
		}

		AddListing(traderID, newListing);
		newListing.Save();

		SendUpdatedTraderData(traderID, identity.GetId(), ExpansionP2PMarketModuleCallback.ItemListed, type, price);
		
		if (GetExpansionSettings().GetLog().Market)
		{
			string globalIDText = ExpansionStatic.IntToHex(newListing.GetGlobalID());
			string priceStringLog = string.Format("%1 (%2)", price, GetDisplayPrice(traderConfig, price, false, false, true));
			GetExpansionSettings().GetLog().PrintLog("[P2P Market] Player \"" + identity.GetName() + "\" (id=" + identity.GetId() + ")" + " has listed \"" + newListing.GetClassName() + "\" for a price of " + priceStringLog + " (globalID=" + globalIDText + ")");
		}
	}

	void AddListing(int traderID, ExpansionP2PMarketListing listing)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
#endif 
		
		array<ref ExpansionP2PMarketListing> listings;
		if (m_P2PListingsData.Find(traderID, listings))
		{
			if (listings.Find(listing) == -1)
				listings.Insert(listing);
		}
		else
		{
			listings = new array<ref ExpansionP2PMarketListing>;
			listings.Insert(listing);
			m_P2PListingsData.Insert(traderID, listings);
		}
	}
	
	//! Server
	void CallbackError(PlayerIdentity identity, ExpansionP2PMarketModuleCallback error = ExpansionP2PMarketModuleCallback.Error, string type = string.Empty, int price = 0, int listingPriceString = 0, Object blockingObject = null)
	{
		Callback(identity, error, type, price, listingPriceString, blockingObject);
	}

	void Callback(PlayerIdentity identity, ExpansionP2PMarketModuleCallback callback, string type = string.Empty, int price = 0, int listingPriceString = 0, Object blockingObject = null)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
#endif 
		
		auto rpc = Expansion_CreateRPC("RPC_Callback");
		rpc.Write(callback);
		rpc.Write(type);
		rpc.Write(price);
		rpc.Write(listingPriceString);
		rpc.Write(blockingObject);
		rpc.Expansion_Send(true, identity);
	}

	//! Client
	void RequestPurchaseBMItem(ExpansionP2PMarketListing listing)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
#endif 
		
		if (!listing.IsGlobalIDValid())
		{
			Error(ToString() + "::RequestPurchaseBMItem - Listing global ID is invalid!");
			return;
		}

		int traderID = listing.GetTraderID();
		TIntArray globalID = listing.GetGlobalID();

		auto rpc = Expansion_CreateRPC("RPC_RequestPurchaseBMItem");
		rpc.Write(traderID);
		rpc.Write(globalID);
		rpc.Expansion_Send(true);
	}

	//! Server
	protected void RPC_RequestPurchaseBMItem(PlayerIdentity identity, Object target, ParamsReadContext ctx)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
#endif 
		
		int traderID;
		if (!ctx.Read(traderID))
		{
			Error(ToString() + "::RPC_RequestPurchaseBMItem - Could not read traderID.");
			CallbackError(identity);
			return;
		}

		TIntArray globalID = new TIntArray;
		if (!ctx.Read(globalID))
		{
			Error(ToString() + "::RPC_RequestPurchaseBMItem - Could not read globalID.");
			CallbackError(identity);
			return;
		}

		ExpansionP2PMarketTraderConfig traderConfig = GetP2PTraderConfigByID(traderID);
		if (!traderConfig)
		{
			Error(ToString() + "::RPC_RequestPurchaseBMItem - Could not read traderConfig for ID: " + traderID);
			CallbackError(identity);
			return;
		}

		ExpansionP2PMarketListing listing = GetListingByGlobalID(traderID, globalID, traderConfig.IsGlobalTrader());
		if (!listing)
		{
			Error(ToString() + "::RPC_RequestPurchaseBMItem - Could not get listing for ID: " + globalID + " | Trader ID: " + traderID);
			CallbackError(identity);
			return;
		}

		string listingOwnerUID = listing.GetOwnerUID();
		string playerUID = identity.GetId();
		if (!identity || playerUID == string.Empty)
		{
			Error(ToString() + "::RPC_RequestPurchaseBMItem - Could not get player UID.");
			CallbackError(identity);
			return;
		}

		PlayerBase player = PlayerBase.Cast(identity.GetPlayer());
		if (!player)
		{
			Error(ToString() + "::RPC_RequestPurchaseBMItem - Could not get player.");
			CallbackError(identity);
			return;
		}

		bool isOwner;
		if (listingOwnerUID == playerUID)
			isOwner = true;

		auto settings = GetExpansionSettings().GetP2PMarket();
		int price = listing.GetPrice();
		string priceString = GetDisplayPrice(traderConfig, price, false, true, true);
		int ownerDiscount = ((float) price / 100) * settings.ListingOwnerDiscountPercent;
		int ownerPrice = price - ownerDiscount;
		string ownerPriceString = GetDisplayPrice(traderConfig, ownerPrice, false, true, true);
		array<int> monies = new array<int>;
		TStringArray currencies = traderConfig.GetCurrencies();
		int playerWorth = m_MarketModule.GetPlayerWorth(player, monies, currencies);
		if (!isOwner && playerWorth < price)
		{
			CallbackError(identity, ExpansionP2PMarketModuleCallback.ErrorNotEnoughMoney, listing.GetClassName(), price);
			return;
		}
		else if (isOwner && playerWorth < ownerPrice)
		{
			CallbackError(identity, ExpansionP2PMarketModuleCallback.ErrorNotEnoughMoney, listing.GetClassName(), ownerPrice);
			return;
		}

		vector spawnPositionVehicle;
		bool isVehicle = false;

		ExpansionVehicleType vehicleType = ExpansionVehicle.GetVehicleType(listing.GetClassName());
		switch (vehicleType)
		{
			case ExpansionVehicleType.WATER:
				spawnPositionVehicle = traderConfig.GetWatercraftSpawnPosition();
				break;
			case ExpansionVehicleType.AIR:
				spawnPositionVehicle = traderConfig.GetAircraftSpawnPosition();
				break;
			case ExpansionVehicleType.LAND:
			case ExpansionVehicleType.RAILTRACK:
				spawnPositionVehicle = traderConfig.GetVehicleSpawnPosition();
				break;
		}

		if (vehicleType != ExpansionVehicleType.NONE)
		{
			isVehicle = true;
			EXPrint(ToString() + "::RPC_RequestPurchaseBMItem - Vehicle type is " + typename.EnumToString(ExpansionVehicleType, vehicleType) + " | Spawn position: " + spawnPositionVehicle);
		}

		if (isVehicle && spawnPositionVehicle == vector.Zero)
		{
			CallbackError(identity, ExpansionP2PMarketModuleCallback.ErrorNoVehicleSpawnPosition, listing.GetClassName());
			return;
		}

		Object blockingObject;
		if (isVehicle && !ExpansionItemSpawnHelper.IsSpawnPositionFree(spawnPositionVehicle, Vector(0, 0, 0), listing.GetClassName(), blockingObject))
		{
			CallbackError(identity, ExpansionP2PMarketModuleCallback.ErrorVehicleSpawnPositionBlocked, listing.GetClassName(), 0, 0, blockingObject);
			return;
		}
		
		string globalIDText = listing.GetEntityStorageBaseName();	//! @note: For logging purposes only
			
		EntityAI loadedEntity;
		if (!LoadItem(listing, player, loadedEntity))
		{
			ExpansionNotification(new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_PURCHASE_FAILED_TITLE"), new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_CANT_RESTORE_ITEM_DESC", globalIDText), ExpansionIcons.GetPath("Exclamationmark"), COLOR_EXPANSION_NOTIFICATION_ERROR, 7, ExpansionNotificationType.TOAST).Create(identity);
			CallbackError(identity);
			Error(ToString() + "::RPC_RequestPurchaseBMItem - Could not restore stored item for listing: " + globalIDText);
			return;
		}

	#ifdef EXPANSIONMODVEHICLE
		//! If the object is a vehicle we spawn a car key on the player and pair it to the vehicle
		ExpansionCarKey key;
		EntityAI playerEntity;
		Class.CastTo(playerEntity, player);
		int amount = 1;
		ExpansionVehicle vehicle;
		if (ExpansionVehicle.Get(vehicle, loadedEntity))
		{
			EXPrint(ToString() + "::RPC_RequestPurchaseBMItem - Change position of " + vehicle.GetType() + " | Current position: " + vehicle.GetPosition());

			EntityAI vehicleEntity = vehicle.GetEntity();
			vehicleEntity.SetPosition(spawnPositionVehicle);
			vehicleEntity.PlaceOnSurface();
			dBodyActive(vehicleEntity, ActiveState.ACTIVE);
			vehicle.Synchronize();

			EXPrint(ToString() + "::RPC_RequestPurchaseBMItem - New position of " + vehicle.GetType() + " | Position: " + vehicle.GetPosition());

			key = ExpansionCarKey.Cast(ExpansionItemSpawnHelper.SpawnOnParent("ExpansionCarKey", player, playerEntity, amount));
			vehicle.PairKey(key);
			if (!vehicle.IsLocked())
				vehicle.Lock(key); //! Lock vehicle if not locked already
		}
	#endif

		if (!isOwner && !m_MarketModule.RemoveMoney(price, player, currencies))
		{
			Error(ToString() + "::RPC_RequestPurchaseBMItem - Could not remove money from player!");
			ExpansionNotification(new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_PURCHASE_FAILED_TITLE"), new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_CANT_REMOVE_MONEY_DESC"), ExpansionIcons.GetPath("Exclamationmark"), COLOR_EXPANSION_NOTIFICATION_ERROR, 7, ExpansionNotificationType.TOAST).Create(identity);
			CallbackError(identity);
			return;
		}
		else if (isOwner && !m_MarketModule.RemoveMoney(ownerPrice, player, currencies))
		{
			Error(ToString() + "::RPC_RequestPurchaseBMItem - Could not remove money from player!");
			ExpansionNotification(new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_PURCHASE_FAILED_TITLE"), new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_CANT_REMOVE_MONEY_DESC"), ExpansionIcons.GetPath("Exclamationmark"), COLOR_EXPANSION_NOTIFICATION_ERROR, 7, ExpansionNotificationType.TOAST).Create(identity);
			CallbackError(identity);
			return;
		}

		if (!isOwner)
		{
			listing.SetListingState(ExpansionP2PMarketListingState.SOLD);
			listing.SetListingTime();
			listing.Save();
		}
		else
		{
			//! We remove the listing directly from the trader data as there is no need to
			//! change the listing to the sold state when it is purchased by the listing owner.
			if (!RemoveListingByGlobalID(traderID, globalID, traderConfig.IsGlobalTrader()))
			{
				EXPrint(ToString() + " ::RPC_RequestPurchaseBMItem - could not remove listing " + globalIDText);
				ExpansionNotification("RPC_RequestPurchaseBMItem", "Could not remove listing " + globalIDText).Error(identity);
			}
		}

		int messagePrice;
		if (!isOwner)
		{
			messagePrice = price;
		}
		else
		{
			messagePrice = ownerPrice;
		}

		SendUpdatedTraderData(traderID, identity.GetId(), ExpansionP2PMarketModuleCallback.ItemPurchased, listing.GetClassName(), messagePrice);
		
		if (!isOwner)
		{
			PlayerBase ownerPlayer = PlayerBase.GetPlayerByUID(listingOwnerUID);
			if (ownerPlayer)
			{
				Callback(ownerPlayer.GetIdentity(), ExpansionP2PMarketModuleCallback.MsgItemGotSold, listing.GetClassName(), price);
			}
		}
		
		if (GetExpansionSettings().GetLog().Market)
		{
			string messagePriceStringLog = string.Format("%1 (%2)", messagePrice, GetDisplayPrice(traderConfig, messagePrice, false, false, true));
			GetExpansionSettings().GetLog().PrintLog("[P2P Market] Player \"" + identity.GetName() + "\" (id=" + identity.GetId() + ")" + " has purchased \"" + listing.GetClassName() + "\" for a price of " + messagePriceStringLog + " (globalID=" + globalIDText + ")");
		}
		
		MissionBaseWorld missionWorld = MissionBaseWorld.Cast(GetGame().GetMission());
		if (missionWorld)
		{
			missionWorld.Expansion_OnP2PMarketPurchase(playerUID, messagePrice, loadedEntity);
		}
	}

	//! Client
	protected void RPC_Callback(PlayerIdentity identity, Object target, ParamsReadContext ctx)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
#endif 
		
		int callback;
		if (!ctx.Read(callback))
		{
			Error(ToString() + "::RPC_Callback - couldn't read callback");
			return;
		}

		string type;
		if (!ctx.Read(type))
		{
			Error(ToString() + "::RPC_Callback - couldn't read type");
			return;
		}

		int price;
		if (!ctx.Read(price))
		{
			Error(ToString() + "::RPC_Callback - couldn't read price");
			return;
		}

		int option;
		if (!ctx.Read(option))
		{
			Error(ToString() + "::RPC_Callback - couldn't read option");
			return;
		}

		Object blockingObject;
		if (!ctx.Read(blockingObject))
		{
			Error(ToString() + "::RPC_Callback - couldn't read price");
			return;
		}

		string priceString;
		CF_Localiser localiser;

		switch (callback)
		{
			case ExpansionP2PMarketModuleCallback.MsgTotalSold:
				priceString = ExpansionMarketModule.GetDisplayPriceEx(price);
				int salesCount = option;
				localiser = new CF_Localiser("STR_EXPANSION_MARKET_P2P_MSG_TOTAL_SOLD_NOTIFIER", priceString, salesCount.ToString());
				GetGame().Chat(localiser.Format(), "colorAction");
				break;

			case ExpansionP2PMarketModuleCallback.MsgItemGotSold:
				string displayName = ExpansionStatic.GetItemDisplayNameWithType(type);
				priceString = ExpansionMarketModule.GetDisplayPriceEx(price);
				localiser = new CF_Localiser("STR_EXPANSION_MARKET_P2P_MSG_ITEM_GOT_SOLD_NOTIFIER", displayName, priceString);
				GetGame().Chat(localiser.Format(), "colorAction");
				break;

			default:
				m_P2PMarketMenuCallbackInvoker.Invoke(callback, type, price, option, blockingObject);
				break;
		}
	}

	static int GetMarketPrice(string typeName)
	{
		typeName.ToLower();
		ExpansionMarketItem marketItem = ExpansionMarketCategory.GetGlobalItem(typeName);
		if (marketItem)
			return marketItem.CalculatePrice(marketItem.MaxStockThreshold);

		return 0;
	}
	
	static string GetDisplayPrice(ExpansionP2PMarketTraderConfig trader, int price, bool shorten = false, bool format = true, bool includeDisplayCurrencyName = false)
	{
		return ExpansionMarketModule.GetDisplayPriceEx(price, shorten, format, includeDisplayCurrencyName, trader.m_DisplayCurrencyValue, trader.m_DisplayCurrencyPrecision, trader.m_DisplayCurrencyName);
	}
	
	bool CheckCanUseTrader(PlayerBase player, ExpansionP2PMarketTraderConfig trader)
	{
	#ifdef ENFUSION_AI_PROJECT
		if (GetGame().IsServer() && trader.m_RequiredFaction != "")
		{
			if (!player.GetGroup() || player.GetGroup().GetFaction().GetName() != trader.m_RequiredFaction)
			{
				string factionDisplayname;
			#ifdef EXPANSIONMODAI
				eAIFaction faction = eAIFaction.Create(trader.m_RequiredFaction);
				factionDisplayname = faction.GetDisplayName();
			#else
				factionDisplayname = trader.m_RequiredFaction;
			#endif
				ExpansionNotification("STR_EXPANSION_AI_FACTION", new StringLocaliser("STR_EXPANSION_AI_REQUIRED_FACTION_TRADER", factionDisplayname), EXPANSION_NOTIFICATION_ICON_ERROR, COLOR_EXPANSION_NOTIFICATION_ERROR, 7, ExpansionNotificationType.TOAST).Create(player.GetIdentity());

				return false;
			}
		}
	#endif

		return true;
	}

	protected bool StoreItem(ExpansionP2PMarketListing listing, EntityAI itemEntity)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
#endif 
		
		if (!itemEntity)
		{
			Error(ToString() + "::StoreItem - Could not get item entity!");
			return false;
		}

		bool success = ExpansionEntityStorageModule.SaveToFile(itemEntity, listing.GetEntityStorageFileName());
		if (!success)
		{
			Error(ToString() + "::StoreVehicle - Could not store item entity!");
			return false;
		}

		ExpansionEntityStorageModule.DeleteSavedEntity(itemEntity);

		return true;
	}

	protected bool LoadItem(ExpansionP2PMarketListing listing, PlayerBase player, out EntityAI loadedEntity = null)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
#endif 
		
		if (!ExpansionEntityStorageModule.RestoreFromFile(listing.GetEntityStorageFileName(), loadedEntity, null, player))
		{
			Error(ToString() + "::LoadItem - Could not restore item " + listing.GetClassName() + " from file " + listing.GetEntityStorageFileName());
			return false;
		}

		return true;
	}

	array<EntityAI> GetSlotItems(EntityAI entity, out int doorsRequiredAmount, out int wheelsRequiredAmount)
	{
		array<EntityAI> slotItems = new array<EntityAI>;
		for (int i = 0; i < entity.GetInventory().GetAttachmentSlotsCount(); i++)
		{
			int slotID = entity.GetInventory().GetAttachmentSlotId(i);
			string slotName = InventorySlots.GetSlotName(slotID);

			slotName.ToLower();

			foreach (string doorSlotKeyword: s_DoorSlotKeywords)
			{
				if (slotName.IndexOf(doorSlotKeyword) > -1)
				{
					doorsRequiredAmount++;
					break;
				}
			}

			foreach (string wheelSlotKeyword: s_WheelSlotKeywords)
			{
				if (slotName.IndexOf(wheelSlotKeyword) > -1 && slotName.IndexOf("spare") < 0)
				{
					wheelsRequiredAmount++;
					break;
				}
			}

			EntityAI slotItem = entity.GetInventory().FindAttachment(slotID);
			if (slotItem)
				slotItems.Insert(slotItem);
		}

		return slotItems;
	}

	protected bool CheckItemsCondition(array<EntityAI> items)
	{
		foreach (EntityAI item: items)
		{
			if (item.IsRuined())
				return false;
		}

		return true;
	}

	ExpansionP2PMarketTraderConfig GetP2PTraderConfigByID(int id)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
#endif 
		
		ExpansionP2PMarketTraderConfig config;
		if (m_P2PTraderConfig.Find(id, config))
			return config;

		return NULL;
	}

	array<EntityAI> LocalGetEntityInventory()
	{
		return m_LocalEntityInventory.m_Inventory;
	}

	void EnumeratePlayerInventory(PlayerBase player)
	{
		m_LocalEntityInventory = new ExpansionP2PMarketPlayerInventory(player);
	}

	ScriptInvoker GetP2PMarketMenuListingsSI()
	{
		return m_P2PMarketMenuListingsInvoker;
	}

	ScriptInvoker GetP2PMarketMenuCallbackSI()
	{
		return m_P2PMarketMenuCallbackInvoker;
	}

	static int GetDiscountPrice(int price)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, ExpansionP2PMarketModule);
#endif 
		
		int discountPrice = Math.Round((price / 100) * GetExpansionSettings().GetP2PMarket().ListingOwnerDiscountPercent);
		return (price - discountPrice);
	}

	//! Check if one of the listed items is listed longer then MaxListingTime from the settings or sold listings are saved longer then SalesDepositTime.
	//! If so we clean the listed item up form the market and entity storage.
	protected void CheckListingsTimes()
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
#endif 
		
		int timedif;
		int maxListingTime = GetExpansionSettings().GetP2PMarket().MaxListingTime;
		int salesDepositTime = GetExpansionSettings().GetP2PMarket().SalesDepositTime;
		bool save;

		if (!m_P2PListingsData || m_P2PListingsData.Count() == 0)
			return;

		int currentTime = CF_Date.Now(true).GetTimestamp();

		foreach (int traderID, array<ref ExpansionP2PMarketListing> listings: m_P2PListingsData)
		{
			if (listings && listings.Count() > 0)
			{
				for (int i = listings.Count() - 1; i >= 0; i--)
				{
					ExpansionP2PMarketListing listing = listings[i];	
					P2PDebugPrint("::CheckListingsTimes - Check lisitng: " + listing.GetEntityStorageFileName() + " | State: " + typename.EnumToString(ExpansionP2PMarketListingState, listing.GetListingState()) + " | Cooldown: " + listing.HasCooldown(salesDepositTime, timedif, currentTime));				
					if (listing && (listing.GetListingState() == ExpansionP2PMarketListingState.LISTED || listing.GetListingState() == ExpansionP2PMarketListingState.SOLD) && !listing.HasCooldown(salesDepositTime, timedif, currentTime))
					{
						P2PDebugPrint("::CheckListingsTimes - Cleanup listed item from BM Trader with ID: " + traderID + " | Item Name: " + listing.GetClassName());
						ExpansionEntityStorageModule.DeleteFiles(listing.GetEntityStorageBaseName());
						listings.RemoveOrdered(i);
					}
				}
			}
		}
	}

	protected int GetPlayerListingsCount(string playerUID)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
#endif 
		
		int count;
		foreach (int traderID, array<ref ExpansionP2PMarketListing> listings: m_P2PListingsData)
		{
			foreach (ExpansionP2PMarketListing listing: listings)
			{
				if (listing.GetOwnerUID() == playerUID)
					count++;
			}
		}

		return count;
	}

	protected array<ref ExpansionP2PMarketListing> GetTraderListings(int traderID)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
#endif 
		
		return m_P2PListingsData[traderID];
	}

	protected ExpansionP2PMarketListing GetListingByGlobalID(int traderID, TIntArray globalID, bool globalTrader = false)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
#endif 

		ExpansionP2PMarketListing listing;

		if (traderID > -1 && !globalTrader)
		{
			array<ref ExpansionP2PMarketListing> listings = m_P2PListingsData[traderID];
			if (listings)
				listing = GetListingByGlobalID(listings, globalID);
		}
		else
		{
			foreach (int listingsTraderID, array<ref ExpansionP2PMarketListing> traderListings: m_P2PListingsData)
			{
				listing = GetListingByGlobalID(traderListings, globalID);
				if (listing)
					break;
			}
		}

		return listing;
	}

	protected ExpansionP2PMarketListing GetListingByGlobalID(array<ref ExpansionP2PMarketListing> listings, TIntArray globalID)
	{
		foreach (ExpansionP2PMarketListing listing: listings)
		{
			if (listing.IsGlobalIDValid() && listing.IsGlobalIDEqual(globalID))
				return listing;
		}

		return null;
	}

	protected bool RemoveListingByGlobalID(int traderID, TIntArray globalID, bool globalTrader = false)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
#endif 

		if (traderID > -1 && !globalTrader)
		{
			array<ref ExpansionP2PMarketListing> listings = m_P2PListingsData[traderID];
			if (listings)
				return RemoveListingByGlobalID(traderID, listings, globalID);
		}
		else
		{
			foreach (int listingsTraderID, array<ref ExpansionP2PMarketListing> traderListings: m_P2PListingsData)
			{
				if (RemoveListingByGlobalID(traderID, traderListings, globalID))
					return true;
			}
		}

		return false;
	}

	protected bool RemoveListingByGlobalID(int traderID, array<ref ExpansionP2PMarketListing> listings, TIntArray globalID)
	{
		for (int i = listings.Count() - 1; i >= 0; i--)
		{
			ExpansionP2PMarketListing listing = listings[i];
			if (listing.IsGlobalIDValid() && listing.IsGlobalIDEqual(globalID))
			{
				if (RemoveListing(traderID, listings, i))
					return true;
			}
		}

		return false;
	}

	protected bool RemoveListing(int traderID, array<ref ExpansionP2PMarketListing> listings, int index)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
#endif 
		
		ExpansionP2PMarketListing listing = listings[index];
		listings.RemoveOrdered(index);

		string baseName = listing.GetEntityStorageBaseName();
		ExpansionEntityStorageModule.DeleteFiles(baseName);
		string filePath = GetP2PMarketDataDirectory() + "P2PTrader_" + traderID + "_Listings\\" + baseName + ".json";
		if (FileExist(filePath))
			return DeleteFile(filePath);
		return false;
	}

	static bool ItemCheckEx(EntityAI item)
	{
		if (ExpansionStatic.IsAnyOf(item, GetExpansionSettings().GetP2PMarket().ExcludedClassNames))
			return false;

		if (item.IsRuined())
			return false;

		//! Don`t add rotten food items
		Edible_Base foodItem;
		if (Class.CastTo(foodItem, item) && foodItem.HasFoodStage())
		{
			FoodStage foodStage = foodItem.GetFoodStage();
			FoodStageType foodStageType = foodStage.GetFoodStageType();
			if (foodStageType == FoodStageType.ROTTEN || foodStageType == FoodStageType.BURNED)
				return false;
		}

	#ifdef WRDG_DOGTAGS
		//! Don`t add players own dogtag
		if (item.IsInherited(Dogtag_Base))
		{
			if (item.GetHierarchyRootPlayer())
				return false;
		}
	#endif

	#ifdef EXPANSIONMODQUESTS
		//! Don`t add quest items
		ItemBase itemIB;
		if (Class.CastTo(itemIB, item))
		{
			if (itemIB.Expansion_IsQuestItem() || itemIB.Expansion_IsQuestGiver())
				return false;
		}
	#endif

		if (!item.CanPutInCargo(null))
			return false;

		return true;
	}

	#ifdef SERVER
	override void OnUpdate(Class sender, CF_EventArgs args)
	{
		super.OnUpdate(sender, args);

		if (!m_Initialized)
			return;

		auto update = CF_EventUpdateArgs.Cast(args);

		m_CheckListingsTime += update.DeltaTime;
		if (m_CheckListingsTime >= CHECK_TICK_TIME)
		{
			CheckListingsTimes();
			m_CheckListingsTime = 0.0;
		}
	}
	#endif

	static ExpansionP2PMarketModule GetModuleInstance()
	{
		return s_Instance;
	}

	static string GetP2PMarketDataDirectory()
	{
		int instance_id = GetGame().ServerConfigGetInt("instanceId");
		return "$mission:storage_" + instance_id + "\\expansion\\p2pmarket\\";
	}

	void P2PDebugPrint(string text)
	{
	#ifdef EXPANSIONMODP2PMARKET_DEBUG
		EXPrint(ToString() + text);
	#endif
	}
};
