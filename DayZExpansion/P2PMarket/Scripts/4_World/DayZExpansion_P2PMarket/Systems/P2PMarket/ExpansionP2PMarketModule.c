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
	ItemListed = 1,
	ItemPurchased = 2,
	SaleRetrieved = 4,
	Error = 8
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

	protected ref TStringArray m_Vehicles = {"CarScript"};
	protected ref TStringArray m_Aircraft = {"ExpansionHelicopterScript"};
	protected ref TStringArray m_Watercraft = {"ExpansionBoatScript"};
	static ref TStringArray m_HardcodedExcludes = {"AugOptic", "Magnum_Cylinder", "Magnum_Ejector", "M97DummyOptics"};

	void ExpansionP2PMarketModule()
	{
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
		
		s_Instance = this;
	}

	override void OnInit()
	{
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
		
		EnableMissionStart();
		EnableMissionLoaded();
		EnableInvokeConnect();
		EnableRPC();
		#ifdef SERVER
		EnableUpdate();
		#endif
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
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
		
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
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
		
		if (GetGame().IsServer() && GetGame().IsMultiplayer())
			ServerModuleInit();

		if (GetGame().IsClient())
			ClientModuleInit();
	}

	override void OnInvokeConnect(Class sender, CF_EventArgs args)
	{
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
		
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
				auto localiser = new CF_Localiser("STR_EXPANSION_MARKET_P2P_MSG_TOTAL_SOLD_NOTIFIER", moneyFromSales.ToString(), salesCount.ToString());
				string message = localiser.Format();
				GetGame().RPCSingleParam(cArgs.Player, ERPCs.RPC_USER_ACTION_MESSAGE, new Param1<string>(message), true, cArgs.Identity);
			}
		}
	}

	protected void ServerModuleInit()
	{
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
		
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
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
		
		if (GetGame().IsClient())
		{
			m_P2PMarketMenuListingsInvoker = new ScriptInvoker();
			m_P2PMarketMenuCallbackInvoker = new ScriptInvoker();
		}
	}

	protected void LoadP2PMarketServerData()
	{
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);

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
		ExpansionP2PMarketTraderConfig bmTrader01;
		if (worldname.IndexOf("chernarus") > -1)
		{
			bmTrader01 = new ExpansionP2PMarketTraderConfig();
			bmTrader01.SetID(1);
		#ifdef EXPANSIONMODAI
			bmTrader01.SetClassName("ExpansionP2PTraderAIIrena");
		#else
			bmTrader01.SetClassName("ExpansionP2PTraderIrena");
		#endif
			bmTrader01.SetPosition(Vector(3697.77, 402.012, 5971.12));
			bmTrader01.SetOrientation(Vector(150.132, 0, 0));
			bmTrader01.SetLoadoutFile("YellowKingLoadout");

			bmTrader01.SetVehicleSpawnPosition(Vector(3728.44, 401.666, 6011.51));

			bmTrader01.Save();
			m_P2PTraderConfig.Insert(1, bmTrader01);
		}
		else if (worldname.IndexOf("namalsk") > -1)
		{
			bmTrader01 = new ExpansionP2PMarketTraderConfig();
			bmTrader01.SetID(1);
		#ifdef EXPANSIONMODAI
			bmTrader01.SetClassName("ExpansionP2PTraderAIIrena");
		#else
			bmTrader01.SetClassName("ExpansionP2PTraderIrena");
		#endif
			bmTrader01.SetPosition(Vector(3696.6, 402.012, 5970.54));
			bmTrader01.SetOrientation(Vector(156.132, 0, -0));
			bmTrader01.SetLoadoutFile("YellowKingLoadout");

			bmTrader01.SetVehicleSpawnPosition(Vector(3741.68, 402.833, 5996.14));

			bmTrader01.Save();
			m_P2PTraderConfig.Insert(1, bmTrader01);
		}
		else
		{
			//! @note: NPC entity is not spawned here as its just a config template.
			bmTrader01 = new ExpansionP2PMarketTraderConfig();
			bmTrader01.SetID(1);
		#ifdef EXPANSIONMODAI
			bmTrader01.SetClassName("ExpansionP2PTraderAIIrena");
		#else
			bmTrader01.SetClassName("ExpansionP2PTraderIrena");
		#endif
			bmTrader01.SetPosition(mapPos);
			bmTrader01.SetOrientation(Vector(0, 0, 0));
			bmTrader01.SetLoadoutFile("YellowKingLoadout");

			bmTrader01.SetVehicleSpawnPosition(mapPos);

			bmTrader01.Save();
		}
	}

	protected void LoadP2PMarketTraderData(string fileName, string path)
	{
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
		
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
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
		
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

	override int GetRPCMin()
	{
		return ExpansionP2PMarketModuleRPC.INVALID;
	}

	override int GetRPCMax()
	{
		return ExpansionP2PMarketModuleRPC.COUNT;
	}

	override void OnRPC(Class sender, CF_EventArgs args)
	{
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
		
		super.OnRPC(sender, args);
		auto rpc = CF_EventRPCArgs.Cast(args);

		switch (rpc.ID)
		{
			case ExpansionP2PMarketModuleRPC.RequestBMTraderData:
			{
				RPC_RequestBMTraderData(rpc.Context, rpc.Sender, rpc.Target);
				break;
			}
			case ExpansionP2PMarketModuleRPC.SendBMTraderData:
			{
				RPC_SendBMTraderData(rpc.Context, rpc.Sender, rpc.Target);
				break;
			}
			case ExpansionP2PMarketModuleRPC.RequestListBMItem:
			{
				RPC_RequestListBMItem(rpc.Context, rpc.Sender, rpc.Target);
				break;
			}
			case ExpansionP2PMarketModuleRPC.RequestPurchaseBMItem:
			{
				RPC_RequestPurchaseBMItem(rpc.Context, rpc.Sender, rpc.Target);
				break;
			}
			case ExpansionP2PMarketModuleRPC.RequestSaleFromListing:
			{
				RPC_RequestSaleFromListing(rpc.Context, rpc.Sender, rpc.Target);
				break;
			}
			case ExpansionP2PMarketModuleRPC.RequestAllPlayerSales:
			{
				RPC_RequestAllPlayerSales(rpc.Context, rpc.Sender, rpc.Target);
				break;
			}
			case ExpansionP2PMarketModuleRPC.RemoveTradingPlayer:
			{
				RPC_RemoveTradingPlayer(rpc.Context, rpc.Sender, rpc.Target);
				break;
			}
			case ExpansionP2PMarketModuleRPC.Callback:
			{
				RPC_Callback(rpc.Context, rpc.Sender, rpc.Target);
				break;
			}
		}
	}

	//! --------------------------------------------------------------------------------------------------------------------------------------------------------------
	void AddTradingPlayer(int traderID, string playerUID)
	{
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
		
		if (!GetGame().IsServer() && !GetGame().IsMultiplayer())
		{
			Error(ToString() + "::AddTradingPlayer - Tried to call AddTradingPlayer on Client!");
			return;
		}

		int id;
		if (!m_TradingPlayers.Find(playerUID, id))
			m_TradingPlayers.Insert(playerUID, traderID);
	}

	void RemoveTradingPlayer(string playerUID)
	{
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
		
		if (GetGame() && !GetGame().IsClient())
		{
			Error(ToString() + "::RemoveTradingPlayer - Tried to call RemoveTradingPlayer on Server!");
			return;
		}

		auto rpc = ExpansionScriptRPC.Create();
		rpc.Write(playerUID);
		rpc.Send(NULL, ExpansionP2PMarketModuleRPC.RemoveTradingPlayer, true);
	}

	protected void RPC_RemoveTradingPlayer(ParamsReadContext ctx, PlayerIdentity senderRPC, Object target)
	{
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);

		if (!ExpansionScriptRPC.CheckMagicNumber(ctx))
			return;

		if (!GetGame().IsServer() && !GetGame().IsMultiplayer())
		{
			Error(ToString() + "::RPC_RemoveTradingPlayer - Tried to call RPC_RemoveTradingPlayer on Client!");
			return;
		}

		string playerUID;
		if (!ctx.Read(playerUID))
			return;

		int traderID;
		if (m_TradingPlayers.Find(playerUID, traderID))
			m_TradingPlayers.Remove(playerUID);
	}

	void SendUpdatedTraderData(int traderID, ExpansionP2PMarketModuleCallback callback = 0)
	{
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
		
		if (!GetGame().IsServer() && !GetGame().IsMultiplayer())
		{
			Error(ToString() + "::SendUpdatedTraderData - Tried to call SendUpdatedTraderData on Client!");
			return;
		}

		foreach (string playerUID, int id: m_TradingPlayers)
		{
			if (id != traderID)
				continue;

			PlayerBase player = PlayerBase.GetPlayerByUID(playerUID);
			if (!player)
				continue;

			SendBMTraderData(traderID, player.GetIdentity(), "", "", callback);
		}
	}

	void GetSaleFromListing(ExpansionP2PMarketListing listing, int traderID)
	{
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
		
		if (!GetGame().IsClient())
		{
			Error(ToString() + "::GetSaleFromListing - Tried to call GetSaleFromListing on Server!");
			return;
		}

		TIntArray globalID = listing.GetGlobalID();

		auto rpc = ExpansionScriptRPC.Create();
		rpc.Write(traderID);
		rpc.Write(globalID);
		rpc.Send(NULL, ExpansionP2PMarketModuleRPC.RequestSaleFromListing, true);
	}

	protected void RPC_RequestSaleFromListing(ParamsReadContext ctx, PlayerIdentity senderRPC, Object target)
	{
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);

		if (!ExpansionScriptRPC.CheckMagicNumber(ctx))
			return;

		if (!GetGame().IsServer() && !GetGame().IsMultiplayer())
		{
			Error(ToString() + "::RPC_RequestSaleFromListing - Tried to call on Client!");
			return;
		}

		int traderID;
		if (!ctx.Read(traderID))
		{
			Error(ToString() + "::RPC_RequestSaleFromListing - couldn't read trader ID!");
			return;
		}

		if (traderID == -1)
		{
			Error(ToString() + "::RPC_RequestSaleFromListing - Trader ID is -1 (configuration error)");
			ExpansionNotification("RPC_RequestSaleFromListing", "Trader ID is -1 (configuration error)").Error(senderRPC);
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

		string playerUID = senderRPC.GetId();
		PlayerBase player = PlayerBase.Cast(senderRPC.GetPlayer());
		if (!player)
			return;

		ExpansionP2PMarketListing listing = GetListingByGlobalID(traderID, globalID, traderConfig.IsGlobalTrader());
		string globalIDText = ExpansionStatic.IntToHex(listing.GetGlobalID());
		if (!listing)
		{
			Error(ToString() + "::RPC_RequestSaleFromListing - couldn't find listing with global ID " + globalIDText);
			ExpansionNotification("RPC_RequestSaleFromListing", "Couldn't find listing with global ID " + globalIDText).Error(senderRPC);
			return;
		}

		if (listing.GetListingState() != ExpansionP2PMarketListingState.SOLD)
		{
			EXPrint(this, "::RPC_RequestSaleFromListing - player " + playerUID + " tried to retrieve profits from a listing that is not yet sold at trader ID " + traderID);
			ExpansionNotification("RPC_RequestSaleFromListing", "This listing has not yet been sold").Error(senderRPC);
			return;
		}

		if (listing.GetOwnerUID() != playerUID)
		{
			EXPrint(this, "::RPC_RequestSaleFromListing - player " + playerUID + " tried to retrieve profits from a listing that is not his own at trader ID " + traderID);
			ExpansionNotification("RPC_RequestSaleFromListing", "You can't retrieve another player's profits").Error(senderRPC);
			return;
		}

		if (!RemoveListingByGlobalID(traderID, globalID, traderConfig.IsGlobalTrader()))
		{
			EXPrint(this, "::RPC_RequestSaleFromListing - could not remove listing " + globalIDText);
			ExpansionNotification("RPC_RequestSaleFromListing", "Could not remove listing " + globalIDText).Error(senderRPC);
			return;
		}
		
		int price = listing.GetPrice();
		EntityAI playerEntity = player;
		m_MarketModule.SpawnMoney(player, playerEntity, price, false, NULL, NULL, true);

		SendBMTraderData(traderID, senderRPC, "", "", ExpansionP2PMarketModuleCallback.SaleRetrieved);

		string displayName = ExpansionStatic.GetItemDisplayNameWithType(listing.GetClassName());
		ExpansionNotification(new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_RETRIEVED_TITLE"), new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_RETRIEVED_ITEM_DESC", displayName, price.ToString()), ExpansionIcons.GetPath("Exclamationmark"), COLOR_EXPANSION_NOTIFICATION_SUCCESS, 7, ExpansionNotificationType.TOAST).Create(senderRPC);
	
		if (GetExpansionSettings().GetLog().Market)
		{
			GetExpansionSettings().GetLog().PrintLog("[P2P Market] Player \"" + senderRPC.GetName() + "\" (id=" + senderRPC.GetId() + ")" + " has retrieved the sale of \"" + listing.GetClassName() + "\" for a price of " + price.ToString() + " (globalID=" + globalIDText + ")");
		}
	}

	void RequestAllPlayerSales(int traderID)
	{
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
		
		if (!GetGame().IsClient())
		{
			Error(ToString() + "::RequestAllPlayerSales - Tried to call RequestAllPlayerSales on Server!");
			return;
		}

		auto rpc = ExpansionScriptRPC.Create();
		rpc.Write(traderID);
		rpc.Send(NULL, ExpansionP2PMarketModuleRPC.RequestAllPlayerSales, true);
	}

	protected void RPC_RequestAllPlayerSales(ParamsReadContext ctx, PlayerIdentity senderRPC, Object target)
	{
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);

		if (!ExpansionScriptRPC.CheckMagicNumber(ctx))
			return;

		if (!GetGame().IsServer() && !GetGame().IsMultiplayer())
		{
			Error(ToString() + "::RPC_RequestAllPlayerSales - Tried to call RPC_RequestAllPlayerSales on Client!");
			return;
		}

		int traderID;
		if (!ctx.Read(traderID))
			return;

		if (traderID == -1)
		{
			Error(ToString() + "::RPC_RequestAllPlayerSales - Trader ID is -1 (configuration error)");
			ExpansionNotification("RPC_RequestAllPlayerSales", "Trader ID is -1 (configuration error)").Error(senderRPC);
			return;
		}

		string playerUID = senderRPC.GetId();
		PlayerBase player = PlayerBase.Cast(senderRPC.GetPlayer());
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
				ExpansionNotification("RPC_RequestAllPlayerSales", "No listings for trader ID " + traderID).Error(senderRPC);
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
						string globalIDText = ExpansionStatic.IntToHex(listing.GetGlobalID());	//! @note: For logging purposes only
						Error(ToString() + "::RPC_RequestAllPlayerSales - could not remove listing " + globalIDText + " from trader ID " + listingsTraderID);
						ExpansionNotification("RPC_RequestAllPlayerSales", "Could not remove listing " + globalIDText + " from trader ID " + listingsTraderID).Error(senderRPC);
						return;
					}
				}
			}
		}

		if (sold == 0)
		{
			EXPrint(this, "::RPC_RequestAllPlayerSales - No listings in SOLD state for player " + playerUID + " at trader ID " + traderID);
			ExpansionNotification("RPC_RequestAllPlayerSales", "You have no sold listings at this trader").Error(senderRPC);
			return;
		}

		EntityAI playerEntity = player;
		m_MarketModule.SpawnMoney(player, playerEntity, price, false, NULL, NULL, true);

		SendBMTraderData(traderID, senderRPC, "", "", ExpansionP2PMarketModuleCallback.SaleRetrieved);

		ExpansionNotification(new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_RETRIEVED_TITLE"), new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_RETRIEVED_ALL_ITEMS_DESC", price.ToString()), ExpansionIcons.GetPath("Exclamationmark"), COLOR_EXPANSION_NOTIFICATION_SUCCESS, 7, ExpansionNotificationType.TOAST).Create(senderRPC);
	
		if (GetExpansionSettings().GetLog().Market)
		{
			GetExpansionSettings().GetLog().PrintLog("[P2P Market] Player \"" + senderRPC.GetName() + "\" (id=" + senderRPC.GetId() + ")" + " has retrieved " + sold + " sales for a total of " + price.ToString() + ".");
		}
	}

	void RequestSendBMTraderData(int traderID)
	{
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
		
		if (!GetGame().IsClient())
		{
			Error(ToString() + "::RequestSendBMTraderData - Tried to call RequestSendBMTraderData on Server!");
			return;
		}

		auto rpc = ExpansionScriptRPC.Create();
		rpc.Write(traderID);
		rpc.Send(NULL, ExpansionP2PMarketModuleRPC.RequestBMTraderData, true);
	}

	protected void RPC_RequestBMTraderData(ParamsReadContext ctx, PlayerIdentity senderRPC, Object target)
	{
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);

		if (!ExpansionScriptRPC.CheckMagicNumber(ctx))
			return;

		if (!GetGame().IsServer() && !GetGame().IsMultiplayer())
		{
			Error(ToString() + "::RPC_RequestBMTraderData - Tried to call RPC_RequestBMTraderData on Client!");
			return;
		}

		int traderID = -1;
		if (!ctx.Read(traderID))
			return;

		SendBMTraderData(traderID, senderRPC);
	}

	void SendBMTraderData(int traderID, PlayerIdentity identity, string traderName = string.Empty, string iconName = string.Empty, ExpansionP2PMarketModuleCallback callback = 0)
	{
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
		
		if (!GetGame().IsServer() && !GetGame().IsMultiplayer())
		{
			Error(ToString() + "::SendBMTraderData - Tried to call RequestPlayerVehicles on Client!");
			return;
		}

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

		auto rpc = ExpansionScriptRPC.Create();
		rpc.Write(traderID);
		rpc.Write(listings.Count());

		foreach (auto listing: listings)
		{
			listing.OnSend(rpc);
		}

		rpc.Write(traderName);
		rpc.Write(iconName);

		rpc.Write(callback);

		rpc.Send(NULL, ExpansionP2PMarketModuleRPC.SendBMTraderData, true, identity);
	}

	protected void RPC_SendBMTraderData(ParamsReadContext ctx, PlayerIdentity senderRPC, Object target)
	{
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
		
		if (!ExpansionScriptRPC.CheckMagicNumber(ctx))
			return;

		if (!GetGame().IsClient())
		{
			Error(ToString() + "::RPC_SendBMTraderData - Tried to call RequestPlayerVehicles on Server!");
			return;
		}

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

		m_P2PMarketMenuListingsInvoker.Invoke(listings, traderID, traderName, iconName);

		int callback;
		if (!ctx.Read(callback))
		{
			Error(ToString() + "::RPC_Callback - couldn't read callback");
			return;
		}

		m_P2PMarketMenuCallbackInvoker.Invoke(callback);
	}

	//! --------------------------------------------------------------------------------------------------------------------------------------------------------------

	void RequestListBMItem(int traderID, Entity item, int price)
	{
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);

		if (!GetGame().IsClient())
		{
			Error(ToString() + "::RequestListBMItem - Tried to call RequestSendBMTraderData on Server!");
			return;
		}

		auto rpc = ExpansionScriptRPC.Create();
		rpc.Write(traderID);
		rpc.Write(price);
		rpc.Send(item, ExpansionP2PMarketModuleRPC.RequestListBMItem, true);
	}

	protected void RPC_RequestListBMItem(ParamsReadContext ctx, PlayerIdentity senderRPC, Object target)
	{
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);

		if (!ExpansionScriptRPC.CheckMagicNumber(ctx))
		{
			Error(ToString() + "::RPC_RequestListBMItem - Magic number check failed!");
			return;
		}

		if (!GetGame().IsServer() && !GetGame().IsMultiplayer())
		{
			Error(ToString() + "::RPC_RequestListBMItem - Tried to call RPC_RequestListBMItem on Server!");
			return;
		}

		if (!senderRPC)
		{
			Error(ToString() + "::RPC_RequestListBMItem - Could not get player identity!");
			return;
		}

		auto settings = GetExpansionSettings().GetP2PMarket();
		int playerListingsCount = GetPlayerListingsCount(senderRPC.GetId());
		if (settings.MaxListings != -1 && playerListingsCount >= settings.MaxListings)
		{
			ExpansionNotification(new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_MAX_LISTING_TITLE"), new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_MAX_LISTING_DESC", playerListingsCount.ToString(), settings.MaxListings.ToString()), ExpansionIcons.GetPath("Exclamationmark"), COLOR_EXPANSION_NOTIFICATION_ERROR, 7, ExpansionNotificationType.TOAST).Create(senderRPC);
			CallbackError(senderRPC);
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

		PlayerBase player = PlayerBase.Cast(senderRPC.GetPlayer());
		if (!player)
		{
			Error(ToString() + "::RPC_RequestListBMItem - Could not get player.");
			return;
		}

		string displayName = objEntity.GetDisplayName();
		int listingPrice = Math.Ceil(price * settings.ListingPricePercent / 100);
		if (listingPrice <= 0)
		{
			ExpansionNotification(new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_LISTING_PRICE_LOW_TITLE"), new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_LISTING_PRICE_LOW_DESC", displayName, price.ToString(), listingPrice.ToString()), ExpansionIcons.GetPath("Exclamationmark"), COLOR_EXPANSION_NOTIFICATION_ERROR, 7, ExpansionNotificationType.TOAST).Create(senderRPC);
			CallbackError(senderRPC);
			return;
		}

		array<int> monies = new array<int>;
		int playerWorth = m_MarketModule.GetPlayerWorth(player, monies, NULL, true);
		if (playerWorth <= 0)
		{
			ExpansionNotification(new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_NOT_ENOUGH_MONEY_TITLE"), new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_NOT_ENOUGH_MONEY_DESC", displayName, price.ToString(), listingPrice.ToString()), ExpansionIcons.GetPath("Exclamationmark"), COLOR_EXPANSION_NOTIFICATION_ERROR, 7, ExpansionNotificationType.TOAST).Create(senderRPC);
			CallbackError(senderRPC);
			return;
		}

		if (playerWorth < listingPrice)
		{
			ExpansionNotification(new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_NOT_ENOUGH_MONEY_TITLE"), new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_NOT_ENOUGH_MONEY_DESC", displayName, price.ToString(), listingPrice.ToString()), ExpansionIcons.GetPath("Exclamationmark"), COLOR_EXPANSION_NOTIFICATION_ERROR, 7, ExpansionNotificationType.TOAST).Create(senderRPC);
			CallbackError(senderRPC);
			return;
		}

	#ifdef EXPANSIONMODVEHICLE
		int doorsCount;
		int doorsRequiredAmount;
		int wheelsCount;
		int wheelsRequiredAmount;
		array<EntityAI> slotItems;
		//! If the object is a vehicle we check for paired keys and unpair/delete them all and remove the pairing from the vehicle.
		CarScript car;
		if (Class.CastTo(car, objEntity))
		{
			slotItems = GetSlotItems(car, doorsRequiredAmount, wheelsRequiredAmount);

			foreach (EntityAI slotItemCar: slotItems)
			{
				EXPrint(ToString() + "::RPC_RequestListBMItem - " + car.GetType() + " | Slots Item: " + slotItemCar.GetType());
				if (slotItemCar.IsInherited(CarDoor))
					doorsCount++;
				else if (slotItemCar.IsInherited(CarWheel))
					wheelsCount++;
			}

			if (doorsCount < doorsRequiredAmount || wheelsCount < wheelsRequiredAmount || !car.Expansion_IsVehicleFunctional(true))
			{
				ExpansionNotification(new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_LISTING_ERROR_TITLE"), new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_LISTING_VEH_MISSING_ATT_ERROR_DESC", displayName), ExpansionIcons.GetPath("Exclamationmark"), COLOR_EXPANSION_NOTIFICATION_ERROR, 7, ExpansionNotificationType.TOAST).Create(senderRPC);
				CallbackError(senderRPC);
				return;
			}

			if (!CheckItemsCondition(slotItems))
			{
				ExpansionNotification(new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_LISTING_ERROR_TITLE"), new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_LISTING_RUINED_ATT_ERROR_DESC", displayName), ExpansionIcons.GetPath("Exclamationmark"), COLOR_EXPANSION_NOTIFICATION_ERROR, 7, ExpansionNotificationType.TOAST).Create(senderRPC);
				CallbackError(senderRPC);
				return;
			}

			if (car.HasKey())
			{
				array<ExpansionCarKey> carKeys = new array<ExpansionCarKey>;
				ExpansionCarKey.GetKeysForVehicle(car, carKeys);

				for (int i = 0; i < carKeys.Count(); ++i)
				{
					ExpansionCarKey carKey = carKeys[i];
					if (!carKey)
						continue;

					carKey.Unpair(true);
					GetGame().ObjectDelete(carKey);
				}

				car.ResetKeyPairing();
			}
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

		if (!StoreItem(newListing, objEntity))
		{
			Error(ToString() + "::RPC_RequestListBMItem - Could not store listing item!");
			return;
		}
	
		AddListing(traderID, newListing);
		newListing.Save();

		if (!m_MarketModule.RemoveMoney(listingPrice, player))
		{
			Error(ToString() + "::RPC_RequestListBMItem - Could not remove money from player!");
			return;
		}

		SendUpdatedTraderData(traderID, ExpansionP2PMarketModuleCallback.ItemListed);

		ExpansionNotification(new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_PLACED_SALE_TITLE"), new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_PLACED_SALE_DESC", displayName, price.ToString()), ExpansionIcons.GetPath("Exclamationmark"), COLOR_EXPANSION_NOTIFICATION_SUCCESS, 7, ExpansionNotificationType.TOAST).Create(senderRPC);
		
		if (GetExpansionSettings().GetLog().Market)
		{
			string globalIDText = ExpansionStatic.IntToHex(newListing.GetGlobalID());
			GetExpansionSettings().GetLog().PrintLog("[P2P Market] Player \"" + senderRPC.GetName() + "\" (id=" + senderRPC.GetId() + ")" + " has listed \"" + newListing.GetClassName() + "\" for a price of " + price.ToString() + " (globalID=" + globalIDText + ")");
		}
	}

	void AddListing(int traderID, ExpansionP2PMarketListing listing)
	{
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
		
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

	void CallbackError(PlayerIdentity senderRPC)
	{
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
		
		auto rpc = ExpansionScriptRPC.Create();
		rpc.Write(ExpansionP2PMarketModuleCallback.Error);
		rpc.Send(NULL, ExpansionP2PMarketModuleRPC.Callback, true, senderRPC);
	}

	void RequestPurchaseBMItem(ExpansionP2PMarketListing listing)
	{
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
		
		if (!GetGame().IsClient())
		{
			Error(ToString() + "::RequestPurchaseBMItem - Tried to call RequestSendBMTraderData on Server!");
			return;
		}

		if (!listing.IsGlobalIDValid())
		{
			Error(ToString() + "::RequestPurchaseBMItem - Listing global ID is invalid!");
			return;
		}

		int traderID = listing.GetTraderID();
		TIntArray globalID = listing.GetGlobalID();

		auto rpc = ExpansionScriptRPC.Create();
		rpc.Write(traderID);
		rpc.Write(globalID);
		rpc.Send(NULL, ExpansionP2PMarketModuleRPC.RequestPurchaseBMItem, true);
	}

	protected void RPC_RequestPurchaseBMItem(ParamsReadContext ctx, PlayerIdentity senderRPC, Object target)
	{
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);

		if (!ExpansionScriptRPC.CheckMagicNumber(ctx))
		{
			Error(ToString() + "::RPC_RequestPurchaseBMItem - Magic number check failed!");
			return;
		}

		if (!GetGame().IsServer() && !GetGame().IsMultiplayer())
		{
			Error(ToString() + "::RPC_RequestPurchaseBMItem - Tried to call RPC_RequestPurchaseBMItem on Server!");
			return;
		}

		int traderID;
		if (!ctx.Read(traderID))
		{
			Error(ToString() + "::RPC_RequestPurchaseBMItem - Could not read traderID.");
			CallbackError(senderRPC);
			return;
		}

		TIntArray globalID = new TIntArray;
		if (!ctx.Read(globalID))
		{
			Error(ToString() + "::RPC_RequestPurchaseBMItem - Could not read globalID.");
			CallbackError(senderRPC);
			return;
		}

		ExpansionP2PMarketTraderConfig traderConfig = GetP2PTraderConfigByID(traderID);
		if (!traderConfig)
		{
			Error(ToString() + "::RPC_RequestPurchaseBMItem - Could not read traderConfig for ID: " + traderID);
			CallbackError(senderRPC);
			return;
		}

		ExpansionP2PMarketListing listing = GetListingByGlobalID(traderID, globalID, traderConfig.IsGlobalTrader());
		if (!listing)
		{
			Error(ToString() + "::RPC_RequestPurchaseBMItem - Could not get listing for ID: " + globalID + " | Trader ID: " + traderID);
			CallbackError(senderRPC);
			return;
		}

		string listingOwnerUID = listing.GetOwnerUID();
		string playerUID = senderRPC.GetId();
		if (!senderRPC || playerUID == string.Empty)
		{
			Error(ToString() + "::RPC_RequestPurchaseBMItem - Could not get player UID.");
			CallbackError(senderRPC);
			return;
		}

		PlayerBase player = PlayerBase.Cast(senderRPC.GetPlayer());
		if (!player)
		{
			Error(ToString() + "::RPC_RequestPurchaseBMItem - Could not get player.");
			CallbackError(senderRPC);
			return;
		}

		string displayName = ExpansionStatic.GetItemDisplayNameWithType(listing.GetClassName());
		bool isOwner;
		if (listingOwnerUID == playerUID)
			isOwner = true;

		auto settings = GetExpansionSettings().GetP2PMarket();
		int price = listing.GetPrice();
		int ownerDiscount = (price / 100) * settings.ListingOwnerDiscountPercent;
		int ownerPrice = price - ownerDiscount;
		array<int> monies = new array<int>;
		int playerWorth = m_MarketModule.GetPlayerWorth(player, monies, NULL, true);
		if (!isOwner && playerWorth < price)
		{
			ExpansionNotification(new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_NOT_ENOUGH_MONEY_TITLE"), new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_NOT_ENOUGH_MONEY_DESC", displayName, price.ToString()), ExpansionIcons.GetPath("Exclamationmark"), COLOR_EXPANSION_NOTIFICATION_ERROR, 7, ExpansionNotificationType.TOAST).Create(senderRPC);
			CallbackError(senderRPC);
			return;
		}
		else if (isOwner && playerWorth < ownerPrice)
		{
			ExpansionNotification(new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_NOT_ENOUGH_MONEY_TITLE"), new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_NOT_ENOUGH_MONEY_DESC", displayName, ownerPrice.ToString()), ExpansionIcons.GetPath("Exclamationmark"), COLOR_EXPANSION_NOTIFICATION_ERROR, 7, ExpansionNotificationType.TOAST).Create(senderRPC);
			CallbackError(senderRPC);
			return;
		}

		vector spawnPositionVehicle;
		bool isVehicle = false;
		string vehicleType = "";
		if (ExpansionStatic.IsAnyOf(listing.GetClassName(), m_Aircraft))
		{
			spawnPositionVehicle = traderConfig.GetAircraftSpawnPosition();
			isVehicle = true;
			vehicleType = "Aircraft";
		}
		else if (ExpansionStatic.IsAnyOf(listing.GetClassName(), m_Watercraft))
		{
			spawnPositionVehicle = traderConfig.GetWatercraftSpawnPosition();
			isVehicle = true;
			vehicleType = "Watercraft";
		}
		else if (ExpansionStatic.IsAnyOf(listing.GetClassName(), m_Vehicles))
		{
			spawnPositionVehicle = traderConfig.GetVehicleSpawnPosition();
			isVehicle = true;
			vehicleType = "Car";
		}

		EXPrint(ToString() + "::RPC_RequestPurchaseBMItem - Object is " + vehicleType + " | Spawn position: " + spawnPositionVehicle);

		if (isVehicle && spawnPositionVehicle == vector.Zero)
		{
			ExpansionNotification(new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_PURCHASE_FAILED_TITLE"), new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_NO_VEH_SPAWN_DESC", ExpansionStatic.GetItemDisplayNameWithType(listing.GetClassName())), ExpansionIcons.GetPath("Exclamationmark"), COLOR_EXPANSION_NOTIFICATION_ERROR, 7, ExpansionNotificationType.TOAST).Create(senderRPC);
			CallbackError(senderRPC);
			return;
		}

		StringLocaliser localiser;

		Object blockingObject;
		if (isVehicle && !ExpansionItemSpawnHelper.IsSpawnPositionFree(spawnPositionVehicle, Vector(0, 0, 0), listing.GetClassName(), blockingObject))
		{
			PlayerBase blockingPlayer;
			if (Class.CastTo(blockingPlayer, blockingObject) && blockingPlayer.GetIdentity())
			{
				displayName = blockingPlayer.GetIdentityName();  //! So you can call 'em out in chat - unless it's yourself...
			}
			else
			{
				displayName = blockingObject.GetDisplayName();
			}

			localiser = new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_BLOCKED_VEH_SPAWN_DESC", ExpansionStatic.GetItemDisplayNameWithType(listing.GetClassName()), displayName, ExpansionStatic.VectorToString(spawnPositionVehicle, ExpansionVectorToString.Labels));
			ExpansionNotification(new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_PURCHASE_FAILED_TITLE"), localiser, ExpansionIcons.GetPath("Exclamationmark"), COLOR_EXPANSION_NOTIFICATION_ERROR, 7, ExpansionNotificationType.TOAST).Create(senderRPC);
			CallbackError(senderRPC);
			return;
		}
		
		string globalIDText = ExpansionStatic.IntToHex(listing.GetGlobalID());	//! @note: For logging purposes only
			
		EntityAI loadedEntity;
		if (!LoadItem(listing, player, loadedEntity))
		{
			ExpansionNotification(new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_PURCHASE_FAILED_TITLE"), new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_CANT_RESTORE_ITEM_DESC", globalIDText), ExpansionIcons.GetPath("Exclamationmark"), COLOR_EXPANSION_NOTIFICATION_ERROR, 7, ExpansionNotificationType.TOAST).Create(senderRPC);
			CallbackError(senderRPC);
			Error(ToString() + "::RPC_RequestPurchaseBMItem - Could not restore stored item for listing: " + globalIDText);
			return;
		}

	#ifdef EXPANSIONMODVEHICLE
		//! If the object is a vehicle we spawn a car key on the player and pair it to the vehicle
		ExpansionCarKey key;
		EntityAI playerEntity;
		Class.CastTo(playerEntity, player);
		int amount = 1;
		CarScript car;
		if (Class.CastTo(car, loadedEntity))
		{
			EXPrint(ToString() + "::RPC_RequestPurchaseBMItem - Change position of " + car.GetType() + " | Current position: " + car.GetPosition());

			car.SetPosition(spawnPositionVehicle);
			car.PlaceOnSurface();
			dBodyActive(car, ActiveState.ACTIVE);
			car.Synchronize();

			EXPrint(ToString() + "::RPC_RequestPurchaseBMItem - New position of " + car.GetType() + " | Position: " + car.GetPosition());

			key = ExpansionCarKey.Cast(ExpansionItemSpawnHelper.SpawnOnParent("ExpansionCarKey", player, playerEntity, amount));
			car.PairKeyTo(key);
			if (!car.IsLocked())
			{
				car.LockCar(key); //! Lock car if not locked already
			}
		}
	#endif

		if (!isOwner && !m_MarketModule.RemoveMoney(price, player))
		{
			Error(ToString() + "::RPC_RequestPurchaseBMItem - Could not remove money from player!");
			ExpansionNotification(new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_PURCHASE_FAILED_TITLE"), new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_CANT_REMOVE_MONEY_DESC"), ExpansionIcons.GetPath("Exclamationmark"), COLOR_EXPANSION_NOTIFICATION_ERROR, 7, ExpansionNotificationType.TOAST).Create(senderRPC);
			CallbackError(senderRPC);
			return;
		}
		else if (isOwner && !m_MarketModule.RemoveMoney(ownerPrice, player))
		{
			Error(ToString() + "::RPC_RequestPurchaseBMItem - Could not remove money from player!");
			ExpansionNotification(new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_PURCHASE_FAILED_TITLE"), new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_CANT_REMOVE_MONEY_DESC"), ExpansionIcons.GetPath("Exclamationmark"), COLOR_EXPANSION_NOTIFICATION_ERROR, 7, ExpansionNotificationType.TOAST).Create(senderRPC);
			CallbackError(senderRPC);
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
				EXPrint(this, "::RPC_RequestPurchaseBMItem - could not remove listing " + globalIDText);
				ExpansionNotification("RPC_RequestPurchaseBMItem", "Could not remove listing " + globalIDText).Error(senderRPC);
			}
		}

		SendUpdatedTraderData(traderID, ExpansionP2PMarketModuleCallback.ItemPurchased);

		int messagePrice;
		if (!isOwner)
		{
			messagePrice = price;
		}
		else
		{
			messagePrice = ownerPrice;
		}

		ExpansionNotification(new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_RETRIEVED_TITLE"), new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_PURCHASED_DESC", displayName, messagePrice.ToString()), ExpansionIcons.GetPath("Exclamationmark"), COLOR_EXPANSION_NOTIFICATION_SUCCESS, 7, ExpansionNotificationType.TOAST).Create(senderRPC);
		
		if (!isOwner)
		{
			PlayerBase ownerPlayer = PlayerBase.GetPlayerByUID(listingOwnerUID);
			if (ownerPlayer)
			{
				localiser = new StringLocaliser("STR_EXPANSION_MARKET_P2P_MSG_ITEM_GOT_SOLD_NOTIFIER", displayName, price.ToString());
				string message = localiser.Format();
				GetGame().RPCSingleParam(ownerPlayer, ERPCs.RPC_USER_ACTION_MESSAGE, new Param1<string>(message), true, ownerPlayer.GetIdentity());
			}
		}
		
		if (GetExpansionSettings().GetLog().Market)
		{
			GetExpansionSettings().GetLog().PrintLog("[P2P Market] Player \"" + senderRPC.GetName() + "\" (id=" + senderRPC.GetId() + ")" + " has purchased \"" + listing.GetClassName() + "\" for a price of " + messagePrice.ToString() + " (globalID=" + globalIDText + ")");
		}
	}

	//! --------------------------------------------------------------------------------------------------------------------------------------------------------------
	static int GetMarketPrice(string typeName)
	{
		typeName.ToLower();
		ExpansionMarketItem marketItem = ExpansionMarketCategory.GetGlobalItem(typeName);
		if (marketItem)
			return marketItem.CalculatePrice(marketItem.MaxStockThreshold);

		return 0;
	}

	protected bool StoreItem(ExpansionP2PMarketListing listing, EntityAI itemEntity)
	{
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);

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

		GetGame().ObjectDelete(itemEntity);

		return true;
	}

	protected bool LoadItem(ExpansionP2PMarketListing listing, PlayerBase player, out EntityAI loadedEntity = null)
	{
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);

		if (!ExpansionEntityStorageModule.RestoreFromFile(listing.GetEntityStorageFileName(), loadedEntity, null, player))
		{
			Error(ToString() + "::LoadItem - Could not restore item " + listing.GetClassName() + " from file " + listing.GetEntityStorageFileName());
			return false;
		}

		return true;
	}

	//! --------------------------------------------------------------------------------------------------------------------------------------------------------------

	protected void RPC_Callback(ParamsReadContext ctx, PlayerIdentity senderRPC, Object target)
	{
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);

		if (!ExpansionScriptRPC.CheckMagicNumber(ctx))
			return;

		if (!GetGame().IsClient())
		{
			Error(ToString() + "::RPC_Callback - Tried to call RPC_Callback on Server!");
			return;
		}

		int callback;
		if (!ctx.Read(callback))
		{
			Error(ToString() + "::RPC_Callback - couldn't read callback");
			return;
		}

		m_P2PMarketMenuCallbackInvoker.Invoke(callback);
	}

	//! --------------------------------------------------------------------------------------------------------------------------------------------------------------

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
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
		
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
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, ExpansionP2PMarketModule);
		
		int discountPrice = Math.Round((price / 100) * GetExpansionSettings().GetP2PMarket().ListingOwnerDiscountPercent);
		return (price - discountPrice);
	}

	//! Check if one of the listed items is listed longer then MaxListingTime from the settings or sold listings are saved longer then SalesDepositTime.
	//! If so we clean the listed item up form the market and entity storage.
	protected void CheckListingsTimes()
	{
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
		
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
						DeleteFile(listing.GetEntityStorageFileName());
						listings.RemoveOrdered(i);
					}
				}
			}
		}
	}

	protected int GetPlayerListingsCount(string playerUID)
	{
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
		
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
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
		
		return m_P2PListingsData[traderID];
	}

	protected ExpansionP2PMarketListing GetListingByGlobalID(int traderID, TIntArray globalID, bool globalTrader = false)
	{
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);

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
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);

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
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
		
		ExpansionP2PMarketListing listing = listings[index];
		listings.RemoveOrdered(index);

		DeleteFile(listing.GetEntityStorageFileName());
		string fileName = ExpansionStatic.IntToHex(listing.GetGlobalID());
		string filePath = GetP2PMarketDataDirectory() + "P2PTrader_" + traderID + "_Listings\\" + fileName + ".json";
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
