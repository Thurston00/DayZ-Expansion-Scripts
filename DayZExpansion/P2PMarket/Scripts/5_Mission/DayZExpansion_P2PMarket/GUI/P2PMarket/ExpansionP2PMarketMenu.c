/**
 * ExpansionP2PMarketMenu.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

enum ExpansionP2PMarketMenuViewState
{
	NONE = 0,
	ViewBrowse = 1,
	ViewList = 2,
	ViewSales = 3,
	DetailViewPlayerItem = 4,
	DetailViewListingItem = 5
};

class ExpansionP2PMarketMenu: ExpansionScriptViewMenu
{
	protected ref ExpansionP2PMarketMenuController m_P2PMarketMenuController;
	protected ref ExpansionP2PMarketModule m_P2PMarketModule;
	protected ref ExpansionMarketModule m_MarketModule;

	protected ref array<ref ExpansionP2PMarketPlayerItem> m_PlayerItems = {};
	protected ref array<ref ExpansionP2PMarketMenuListing> m_ItemListings = {};
	protected ref array<ref ExpansionP2PMarketMenuListing> m_SoldListings = {};
	protected int m_SoldListingsCount;

	protected ExpansionP2PMarketMenuItem m_SelectedPlayerItem;
	protected ExpansionP2PMarketMenuListing m_SelectedListing;
	Object m_SelectedPreviewObject;
	protected ref array<ref ExpansionP2PMarketContainerItem> m_SelectedContainerItems = {};

	protected int m_TraderID = -1;
	protected ref TStringArray m_Currencies;
	protected int m_DisplayCurrencyValue = 1;
	protected string m_DisplayCurrencyName;
	protected int m_DisplayCurrencyPrecision = 2;
	protected bool m_RequestsLocked;
	protected bool m_DefaultEditboxText;
	protected bool m_EditBoxLocked;
	protected int m_PlayerWorth;
	protected int m_ListCost;

	protected int m_CurrentItemPrice;

	protected ref TStringArray m_BloodContainers = {"BloodContainerBase"};

	protected bool m_CategoriesToggleState;

	protected Widget categories_panel;
	protected Widget inventory_panel;
	protected Widget details_panel;
	protected ScrollWidget inventory_scroller;
	protected WrapSpacerWidget inventory_header;
	protected WrapSpacerWidget inventory_content;
	protected ScrollWidget player_items_scroller;
	protected WrapSpacerWidget player_items_content;
	protected ScrollWidget sold_items_scroller;
	protected WrapSpacerWidget sold_items_content;

	protected ButtonWidget listing_category_listings_button;
	protected TextWidget listing_category_listings_text;
	protected ImageWidget listing_category_listings_icon;
	protected TextWidget listing_category_listings_count;

	protected ButtonWidget listing_category_all_button;
	protected TextWidget listing_category_all_text;
	protected ImageWidget listing_category_all_icon;
	protected TextWidget listing_category_all_count;

	protected ButtonWidget toggle_categories_button;
	protected TextWidget toggle_categories_text;

	protected Widget market_filter_panel;
	protected EditBoxWidget market_filter_box;

	protected GridSpacerWidget tabs_panel;
	protected ButtonWidget tabs_button_browse;
	protected TextWidget tabs_button_browse_text;
	protected ButtonWidget tabs_button_list;
	protected TextWidget tabs_button_list_text;
	protected ButtonWidget tabs_button_sales;
	protected TextWidget tabs_button_sales_text;

	protected ButtonWidget exit_button;
	protected TextWidget exit_button_text;

	protected ButtonWidget retrieve_button;
	protected TextWidget retrieve_button_text;

	protected WrapSpacerWidget listing_categories_content;
	protected WrapSpacerWidget player_categories_content;
	protected Widget loading;

	protected GridSpacerWidget toggle_categories_panel;

	protected GridSpacerWidget retrieve_button_panel;
	protected Widget player_sales_panel;

	protected ref ExpansionP2PMarketSettings m_P2PMarketSettings;

	protected ref ExpansionP2PMarketMenuDetailsView m_ItemDetailsView;
	protected ExpansionP2PMarketMenuViewState m_ViewState = ExpansionP2PMarketMenuViewState.ViewBrowse;
	protected ExpansionP2PMarketMenuViewState m_PreviousViewState;

	protected ref ExpansionP2PMarketMenuListHeader m_ListHeader;

	protected ref ExpansionDialog_P2PMarketMenu m_MenuDialog;

	protected string m_PlayerUID;

	void ExpansionP2PMarketMenu()
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
#endif

		Class.CastTo(m_P2PMarketMenuController, GetController());
		Class.CastTo(m_P2PMarketModule, CF_ModuleCoreManager.Get(ExpansionP2PMarketModule));
		Class.CastTo(m_MarketModule, CF_ModuleCoreManager.Get(ExpansionMarketModule));

		m_P2PMarketModule.GetP2PMarketMenuListingsSI().Insert(SetTraderItems);
		m_P2PMarketModule.GetP2PMarketMenuCallbackSI().Insert(OnModuleCallback);

		m_P2PMarketSettings = GetExpansionSettings().GetP2PMarket();

		m_ItemDetailsView = new ExpansionP2PMarketMenuDetailsView(this);
		m_ItemDetailsView.Hide();

		m_ListHeader = new ExpansionP2PMarketMenuListHeader(this);

		m_PlayerUID = GetGame().GetPlayer().GetIdentity().GetId();
	}

	void ~ExpansionP2PMarketMenu()
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
#endif
		
		if (m_P2PMarketModule)
		{
			m_P2PMarketModule.GetP2PMarketMenuListingsSI().Remove(SetTraderItems);
			m_P2PMarketModule.GetP2PMarketMenuCallbackSI().Remove(OnModuleCallback);
		}

		if (m_ItemDetailsView)
			m_ItemDetailsView.Destroy();

		if (m_ListHeader)
			m_ListHeader.Destroy();

		if (m_MenuDialog)
			m_MenuDialog.Destroy();

		if (m_PlayerItems)
			m_PlayerItems.Clear();

		if (m_ItemListings)
			m_ItemListings.Clear();

		if (m_SoldListings)
			m_SoldListings.Clear();

		m_P2PMarketMenuController.PlayerItems.Clear();
		m_P2PMarketMenuController.ListingCategories.Clear();
		m_P2PMarketMenuController.PlayerCategories.Clear();
	}

	override string GetLayoutFile()
	{
		return "DayZExpansion/P2PMarket/GUI/layouts/expansion_p2pmarket_menu.layout";
	}

	override typename GetControllerType()
	{
		return ExpansionP2PMarketMenuController;
	}

	void SetTraderItems(array<ref ExpansionP2PMarketListing> listings, int traderID, string traderName = string.Empty, string iconName = string.Empty, TStringArray currencies = null, int displayCurrencyValue = 1, string displayCurrencyName = string.Empty)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
#endif
		
		EXPrint(ToString() + "::SetTraderItems - ID: " + traderID);
		EXPrint(ToString() + "::SetTraderItems - Trader Name: " + traderName);
		EXPrint(ToString() + "::SetTraderItems - Listings array: " + listings.ToString());
		EXPrint(ToString() + "::SetTraderItems - Listings count: " + listings.Count());
		EXPrint(ToString() + "::SetTraderItems - Currencies: " + currencies);

		m_TraderID = traderID;

		if (traderName != string.Empty)
		{
			m_P2PMarketMenuController.TraderName = traderName;
			m_P2PMarketMenuController.NotifyPropertyChanged("TraderName");
		}

		if (iconName != string.Empty)
		{
			m_P2PMarketMenuController.MarketIcon = ExpansionIcons.GetPath(iconName);
			if (m_P2PMarketMenuController.MarketIcon == "")
				m_P2PMarketMenuController.MarketIcon = iconName;

			m_P2PMarketMenuController.NotifyPropertyChanged("MarketIcon");
		}

		m_Currencies = currencies;
		m_DisplayCurrencyValue = displayCurrencyValue;
		m_DisplayCurrencyName = displayCurrencyName;
		m_DisplayCurrencyPrecision = ExpansionStatic.GetPrecision(m_DisplayCurrencyValue);

		m_ItemListings.Clear();

		m_SoldListings.Clear();

		inventory_header.AddChild(m_ListHeader.GetLayoutRoot());
		m_ListHeader.SetSort(0, false);

		if (listings && listings.Count() > 0)
		{
			foreach (ExpansionP2PMarketListing listing: listings)
			{
				if (!listing)
					continue;

				ExpansionP2PMarketMenuListing newListing = new ExpansionP2PMarketMenuListing(listing, this);
				if (listing.GetListingState() == ExpansionP2PMarketListingState.LISTED)
				{
					m_ItemListings.Insert(newListing);
				}
				else if (listing.GetListingState() == ExpansionP2PMarketListingState.SOLD && listing.GetOwnerUID() == m_PlayerUID)
				{
					m_SoldListingsCount++;
					m_SoldListings.Insert(newListing);
				}
			}

			SortListingsByName();
		}

		if (m_SoldListingsCount > 0)
		{
			tabs_button_sales.Show(true);
		}
		else
		{
			tabs_button_sales.Show(false);
		}

		for (int j = m_P2PMarketMenuController.ListingCategories.Count() - 1; j >= 0; j--)
		{
			m_P2PMarketMenuController.ListingCategories[j].Destroy();
			m_P2PMarketMenuController.ListingCategories.RemoveOrdered(j);
		}

		array<ref ExpansionP2PMarketMenuCategory> menuCategories = m_P2PMarketSettings.MenuCategories;
		foreach (ExpansionP2PMarketMenuCategory category: menuCategories)
		{
			ExpansionP2PMarketMenuCategoryElement categoryElement = new ExpansionP2PMarketMenuCategoryElement(this, category);
			m_P2PMarketMenuController.ListingCategories.Insert(categoryElement);
		}

		//! Get and set all listings count
		string listingsCountText = "[0]";
		int listingsCount = listings.Count();
		if (listingsCount > 0)
			listingsCountText = "[" + listingsCount + "]";

		m_P2PMarketMenuController.AllListingsCount = listingsCountText;
		m_P2PMarketMenuController.NotifyPropertyChanged("AllListingsCount");

		//! Get and set player listings count
		string playerListingsCountText = "[0]";
		int playerListingsCount = GetOwnedListingsCount();
		if (playerListingsCount > 0)
			playerListingsCountText = "[" + playerListingsCount + "]";

		m_P2PMarketMenuController.PlayerListingsCount = playerListingsCountText;
		m_P2PMarketMenuController.NotifyPropertyChanged("PlayerListingsCount");
		
		loading.Show(false);
	}

	void UpdatePlayerItems()
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
#endif
		
		m_PlayerItems.Clear();

		m_P2PMarketModule.EnumeratePlayerInventory(PlayerBase.Cast(GetGame().GetPlayer()));
		array<EntityAI> items = m_P2PMarketModule.LocalGetEntityInventory();
		if (items && items.Count() > 0)
		{
			foreach (EntityAI item: items)
			{
				//! Only add items that have the player as direct parent.
				if ((item.GetHierarchyParent() && item.GetHierarchyParent().IsInherited(SurvivorBase)) || ExpansionVehicle.Get(item))
				{
					//! Hardcoded excluded type names where the item should never get added and shown in the menu.
					if (ExpansionP2PMarketModule.m_HardcodedExcludes.Find(item.GetType()) > -1)
						continue;

					ExpansionP2PMarketPlayerItem newPlayerItem = new ExpansionP2PMarketPlayerItem();
					newPlayerItem.SetFromItem(item, m_PlayerUID);
					m_PlayerItems.Insert(newPlayerItem);
				}
			}

			m_P2PMarketMenuController.PlayerItems.Clear();

			PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
			array<EntityAI> slotItems = GetSlotItems(player);
			array<string> slotNames = new array<string>;
			slotNames.Insert("All");

			foreach (ExpansionP2PMarketPlayerItem playerItem: m_PlayerItems)
			{
				if (!playerItem.GetItem())
					continue;

				for (int i = 0; i < slotItems.Count(); i++)
				{
					EntityAI slotItem = slotItems[i];
					if (!slotItem)
						continue;

					int playerItemSlotID;
					string playerItemSlotName;
					if (playerItem.GetItem() == slotItem)
					{
						slotItem.GetInventory().GetCurrentAttachmentSlotInfo(playerItemSlotID, playerItemSlotName);
						slotNames.Insert(playerItemSlotName);
						playerItem.SetSlotName(playerItemSlotName);
						continue;
					}
					else if (playerItem.GetItem().GetHierarchyParent() == slotItem)
					{
						playerItem.SetSlotName(playerItemSlotName);
						continue;
					}
				}

				ExpansionP2PMarketMenuItem playerItemEntry = new ExpansionP2PMarketMenuItem(playerItem, this);
				m_P2PMarketMenuController.PlayerItems.Insert(playerItemEntry);
			}

			m_P2PMarketMenuController.PlayerCategories.Clear();
			foreach (string slotName: slotNames)
			{
				ExpansionP2PMarketMenuInventoryCategoryElement inventoryCategoryElement = new ExpansionP2PMarketMenuInventoryCategoryElement(this, slotName);
				m_P2PMarketMenuController.PlayerCategories.Insert(inventoryCategoryElement);
			}
		}
	}

	array<EntityAI> GetSlotItems(PlayerBase player)
	{
		array<EntityAI> slotItems = new array<EntityAI>;
		for (int i = 0; i < player.GetInventory().GetAttachmentSlotsCount(); i++)
		{
			int slot = player.GetInventory().GetAttachmentSlotId(i);
			EntityAI slotItem = player.GetInventory().FindAttachment(slot);
			if (slotItem)
				slotItems.Insert(slotItem);
		}

		return slotItems;
	}

	void UpdatePlayerCurrency()
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
#endif
		
		array<int> monies = new array<int>;
		m_PlayerWorth = m_MarketModule.GetPlayerWorth(PlayerBase.Cast(GetGame().GetPlayer()), monies, m_Currencies);

		m_P2PMarketMenuController.PlayerMoney = GetDisplayPrice(m_PlayerWorth, false, true, true);
		m_P2PMarketMenuController.NotifyPropertyChanged("PlayerMoney");
	}

	void OnModuleCallback(int callback, string type = string.Empty, int price = 0, int listingPrice = 0, Object blockingObject = null)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
#endif

		switch (m_ViewState)
		{
			case ExpansionP2PMarketMenuViewState.ViewList:
				GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(UpdatePlayerItems, 250);
				if (market_filter_box.GetText() != string.Empty)
					OnSearchFilterChange();
				break;

			case ExpansionP2PMarketMenuViewState.ViewBrowse:
				if (market_filter_box.GetText() != string.Empty)
					OnSearchFilterChange();
				break;
		}

		string displayName = ExpansionStatic.GetItemDisplayNameWithType(type);
		string priceString = GetDisplayPrice(price, false, true, true);
		string listingPriceString = GetDisplayPrice(listingPrice, false, true, true);

		switch (callback)
		{
			case ExpansionP2PMarketModuleCallback.ItemListed:
				ExpansionNotification(new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_PLACED_SALE_TITLE"), new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_PLACED_SALE_DESC", displayName, priceString), ExpansionIcons.GetPath("Exclamationmark"), COLOR_EXPANSION_NOTIFICATION_SUCCESS, 7, ExpansionNotificationType.TOAST).Create();
				break;

			case ExpansionP2PMarketModuleCallback.ItemPurchased:
				ExpansionNotification(new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_RETRIEVED_TITLE"), new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_PURCHASED_DESC", displayName, priceString), ExpansionIcons.GetPath("Exclamationmark"), COLOR_EXPANSION_NOTIFICATION_SUCCESS, 7, ExpansionNotificationType.TOAST).Create();
				break;

			case ExpansionP2PMarketModuleCallback.SaleRetrieved:
				ExpansionNotification(new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_RETRIEVED_TITLE"), new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_RETRIEVED_ITEM_DESC", displayName, priceString), ExpansionIcons.GetPath("Exclamationmark"), COLOR_EXPANSION_NOTIFICATION_SUCCESS, 7, ExpansionNotificationType.TOAST).Create();
				break;

			case ExpansionP2PMarketModuleCallback.AllSalesRetrieved:
				ExpansionNotification(new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_RETRIEVED_TITLE"), new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_RETRIEVED_ALL_ITEMS_DESC", priceString), ExpansionIcons.GetPath("Exclamationmark"), COLOR_EXPANSION_NOTIFICATION_SUCCESS, 7, ExpansionNotificationType.TOAST).Create();
				break;

			case ExpansionP2PMarketModuleCallback.ErrorListingPriceTooLow:
				ExpansionNotification(new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_LISTING_PRICE_LOW_TITLE"), new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_LISTING_PRICE_LOW_DESC", displayName, priceString, listingPriceString), ExpansionIcons.GetPath("Exclamationmark"), COLOR_EXPANSION_NOTIFICATION_ERROR, 7, ExpansionNotificationType.TOAST).Create();
				break;

			case ExpansionP2PMarketModuleCallback.ErrorNotEnoughMoneyToList:
				ExpansionNotification(new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_NOT_ENOUGH_MONEY_TITLE"), new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_NOT_ENOUGH_MONEY_DESC", displayName, priceString, listingPriceString), ExpansionIcons.GetPath("Exclamationmark"), COLOR_EXPANSION_NOTIFICATION_ERROR, 7, ExpansionNotificationType.TOAST).Create();
				break;

			case ExpansionP2PMarketModuleCallback.ErrorNotEnoughMoney:
				ExpansionNotification(new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_NOT_ENOUGH_MONEY_TITLE"), new StringLocaliser("STR_EXPANSION_MARKET_NOT_ENOUGH_MONEY", displayName, priceString), ExpansionIcons.GetPath("Exclamationmark"), COLOR_EXPANSION_NOTIFICATION_ERROR, 7, ExpansionNotificationType.TOAST).Create();
				break;

			case ExpansionP2PMarketModuleCallback.ErrorVehicleMissingAttachment:
				ExpansionNotification(new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_LISTING_ERROR_TITLE"), new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_LISTING_VEH_MISSING_ATT_ERROR_DESC", displayName), ExpansionIcons.GetPath("Exclamationmark"), COLOR_EXPANSION_NOTIFICATION_ERROR, 7, ExpansionNotificationType.TOAST).Create();
				break;

			case ExpansionP2PMarketModuleCallback.ErrorVehicleRuinedAttachment:
				ExpansionNotification(new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_LISTING_ERROR_TITLE"), new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_LISTING_RUINED_ATT_ERROR_DESC", displayName), ExpansionIcons.GetPath("Exclamationmark"), COLOR_EXPANSION_NOTIFICATION_ERROR, 7, ExpansionNotificationType.TOAST).Create();
				break;

			case ExpansionP2PMarketModuleCallback.ErrorNoVehicleSpawnPosition:
				ExpansionNotification(new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_PURCHASE_FAILED_TITLE"), new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_NO_VEH_SPAWN_DESC", displayName), ExpansionIcons.GetPath("Exclamationmark"), COLOR_EXPANSION_NOTIFICATION_ERROR, 7, ExpansionNotificationType.TOAST).Create();
				break;

			case ExpansionP2PMarketModuleCallback.ErrorVehicleSpawnPositionBlocked:
				PlayerBase blockingPlayer;
				string blockingObjectDisplayName;
				if (Class.CastTo(blockingPlayer, blockingObject) && blockingPlayer.GetIdentity())
				{
					blockingObjectDisplayName = blockingPlayer.GetIdentityName();  //! So you can call 'em out in chat - unless it's yourself...
				}
				else
				{
					blockingObjectDisplayName = blockingObject.GetDisplayName();
				}
				string positionString = ExpansionStatic.VectorToString(blockingObject.GetPosition(), ExpansionVectorToString.Labels);
				StringLocaliser localiser = new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_BLOCKED_VEH_SPAWN_DESC", displayName, blockingObjectDisplayName, positionString);
				ExpansionNotification(new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_PURCHASE_FAILED_TITLE"), localiser, ExpansionIcons.GetPath("Exclamationmark"), COLOR_EXPANSION_NOTIFICATION_ERROR, 7, ExpansionNotificationType.TOAST).Create();
				break;

			case ExpansionP2PMarketModuleCallback.ErrorVehicleLockpicked:
				ExpansionNotification(new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_LISTING_ERROR_TITLE"), new StringLocaliser("STR_EXPANSION_MARKET_P2P_NOTIF_VEHICLE_LOCKPICKED_DESC", displayName), ExpansionIcons.GetPath("Exclamationmark"), COLOR_EXPANSION_NOTIFICATION_ERROR, 7, ExpansionNotificationType.TOAST).Create();
				break;
		}

		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(UpdatePlayerCurrency, 250);
		OnBackClick();
		m_RequestsLocked = false;
	}

	string GetPreviewClassName(string className, bool ignoreBaseBuildingKits = false)
	{
		if (GetGame().ConfigIsExisting("CfgVehicles " + className + "_ExpansionMarketPreview"))
		{
			return className + "_ExpansionMarketPreview";
		}
		else if (!ignoreBaseBuildingKits && className.IndexOf("kit") == className.Length() - 3)
		{
			//! Special handling for Expansion
			if (GetGame().IsKindOf(className, "ExpansionKitLarge"))
			{
				string path = "CfgVehicles " + className + " placingTypes";
				if (GetGame().ConfigIsExisting(path))
				{
					TStringArray placingTypes = new TStringArray;
					GetGame().ConfigGetTextArray(path, placingTypes);
					foreach (string placingType : placingTypes)
					{
						path = "CfgVehicles " + placingType + " deployType";
						if (GetGame().ConfigIsExisting(path))
						{
							return GetGame().ConfigGetTextOut(path);
						}
					}
				}
			}

			if (className == "fencekit" || className == "watchtowerkit" || className == "territoryflagkit")
			{
				//! Item name is kit name without "kit" at the end
				string previewClassName = className.Substring(0, className.Length() - 3);
				if (GetGame().ConfigIsExisting("CfgVehicles " + previewClassName))
					return previewClassName;
			}
		}
		return className;
	}

	//! Gets price text and checks if it contains only numbers.
	//! Returns price as positive int if check passed, else displays error notification and returns -1.
	int GetCheckPrice()
	{
		int price = GetDetailsView().GetListPrice();
		
		if (price == -1)
			ExpansionNotification("#STR_EXPANSION_MARKET_P2P_NOTIF_ERROR_TITLE" , "#STR_EXPANSION_MARKET_P2P_NOTIF_ERROR_ONLYNUMBERS_DESC").Error();

		return price;
	}

	protected void UpdateItemInfoDamage(int damageLevel)
	{
		string text;
		int color;

		switch (damageLevel)
		{
			case GameConstants.STATE_RUINED:
			{
				text = "#inv_inspect_ruined";
				color = Colors.COLOR_RUINED;
				break;
			}
			case GameConstants.STATE_BADLY_DAMAGED:
			{
				text = "#inv_inspect_badly";
				color = Colors.COLOR_BADLY_DAMAGED;
				break;
			}

			case GameConstants.STATE_DAMAGED:
			{
				text = "#inv_inspect_damaged";
				color = Colors.COLOR_DAMAGED;
				break;
			}

			case GameConstants.STATE_WORN:
			{
				text = "#inv_inspect_worn";
				color = Colors.COLOR_WORN;
				break;
			}

			case GameConstants.STATE_PRISTINE:
			{
				text = "#inv_inspect_pristine";
				color = Colors.COLOR_PRISTINE;
				break;
			}

			default:
			{
				text = "#STR_EXPANSION_ERROR_TITLE";
				color = Colors.COLOR_PRISTINE;
				break;
			}
		}

		GetDetailsView().GetDetailsViewController().ItemHealth = text;
		GetDetailsView().GetDetailsViewController().NotifyPropertyChanged("ItemHealth");
		GetDetailsView().GetHealthImageWidget().SetColor(color | 0x7F000000);
	}

	void HideItemDetailElements()
	{
		GetDetailsView().ShowQuantityColor(false);
		GetDetailsView().ShowItemQuantity(false);
		GetDetailsView().ShowItemLiquid(false);
		GetDetailsView().ShowItemCargoCount(false);
		GetDetailsView().ShowItemAttachmentsCount(false);
		GetDetailsView().ShowItemFoodState(false);
	#ifdef EXPANSIONMODHARDLINE
		GetDetailsView().ShowItemRarity(false);
	#endif
	}

	void SetSelectedPlayerItem(ExpansionP2PMarketMenuItem item)
	{
		if (!item || !item.GetPlayerItem())
			return;

		GetDetailsView().ResetPreview();
		m_SelectedPlayerItem = item;
		m_SelectedListing = null;
		m_SelectedPreviewObject = item.GetPreviewObject();

		m_SelectedContainerItems = item.GetPlayerItem().GetContainerItems();

		GetDetailsView().SetViewList();

		GetDetailsView().GetDetailsViewController().SelectedName = item.GetPreviewObject().GetDisplayName();
		GetDetailsView().GetDetailsViewController().NotifyPropertyChanged("SelectedName");

		market_filter_panel.Show(false);
		categories_panel.Show(false);
		player_items_scroller.Show(false);
		inventory_panel.Show(false);
		tabs_panel.Show(false);

		GetDetailsView().ShowConfirmButton(true);
		GetDetailsView().GetDetailsViewController().ConfirmButtonText = "#STR_EXPANSION_MARKET_P2P_CREATESALE_BUTTON_LABEL";
		GetDetailsView().GetDetailsViewController().NotifyPropertyChanged("ConfirmButtonText");

		details_panel.AddChild(GetDetailsView().GetLayoutRoot());
		details_panel.Show(true);

		HideItemDetailElements();

		UpdateItemInfoDamage(item.GetPlayerItem().GetHealthLevel());

		item.GetPlayerItem().UpdateContainerItems();

		if (item.GetPlayerItem().GetLiquidType() != -1)
		{
			bool isBloodContainer = ExpansionStatic.IsAnyOf(item.GetPlayerItem().GetClassName(), m_BloodContainers);
			int liquidColor;
			GetDetailsView().ShowItemLiquid(true);
			GetDetailsView().GetDetailsViewController().LiquidType = GetLiquidTypeString(item.GetPlayerItem().GetLiquidType(), isBloodContainer, liquidColor);
			GetDetailsView().GetDetailsViewController().NotifyPropertyChanged("LiquidType");
			GetDetailsView().SetLiquidColor(liquidColor);
		}

		if (item.GetPlayerItem().GetFoodStageType() != -1)
		{
			int foodStageColor;
			GetDetailsView().ShowItemFoodState(true);
			GetDetailsView().GetDetailsViewController().FoodState = GetFoodStageString(item.GetPlayerItem().GetFoodStageType(), foodStageColor);
			GetDetailsView().GetDetailsViewController().NotifyPropertyChanged("FoodState");
			GetDetailsView().SetFoodStateColor(foodStageColor);
		}

		if (item.GetPlayerItem().GetQuantity() > 0)
		{
			int quantityColor;
			if (item.GetPlayerItem().GetQuantityType() == ExpansionItemQuantityType.POWER || item.GetPlayerItem().GetQuantityType() == ExpansionItemQuantityType.MILLILITER || item.GetPlayerItem().GetQuantityType() == ExpansionItemQuantityType.PERCENTAGE || item.GetPlayerItem().GetQuantityType() == ExpansionItemQuantityType.GRAM)
			{
				GetDetailsView().GetQuantityBarWidget().SetCurrent(item.GetPlayerItem().GetQuantity());
				GetDetailsView().GetDetailsViewController().ItemQuantity = GetQuantityString(item.GetPlayerItem().GetQuantityType(), item.GetPlayerItem().GetQuantity(), quantityColor);
				GetDetailsView().GetDetailsViewController().NotifyPropertyChanged("ItemQuantity");
				GetDetailsView().ShowQuantityBar(true);
			}
			else if (item.GetPlayerItem().GetQuantityType() == ExpansionItemQuantityType.HEATISOLATION)
			{
				int quantity = item.GetPlayerItem().GetQuantity();
				string isolationText;
				int isolationColor;
				if (quantity <= GameConstants.HEATISO_THRESHOLD_BAD)
				{
					isolationText = "#inv_inspect_iso_bad";
					isolationColor = GetTemperatureColor(10);
				}
				else if ((quantity > GameConstants.HEATISO_THRESHOLD_BAD) && (quantity <= GameConstants.HEATISO_THRESHOLD_LOW))
				{
					isolationText = "#inv_inspect_iso_low";
					isolationColor = GetTemperatureColor(20);
				}
				else if ((quantity > GameConstants.HEATISO_THRESHOLD_LOW) && (quantity <= GameConstants.HEATISO_THRESHOLD_MEDIUM))
				{
					isolationText = "#inv_inspect_iso_medium";
					isolationColor = GetTemperatureColor(30);
				}
				else if ((quantity > GameConstants.HEATISO_THRESHOLD_MEDIUM) && (quantity <= GameConstants.HEATISO_THRESHOLD_HIGH))
				{
					isolationText = "#inv_inspect_iso_high";
					isolationColor = GetTemperatureColor(50);
				}
				else
				{
					isolationText = "#inv_inspect_iso_excel";
					isolationColor = GetTemperatureColor(70);
				}

				GetDetailsView().GetDetailsViewController().QuantityText = "#STR_EXPANSION_MARKET_P2P_ITEMINFO_ISOLATIONLABEL";
				GetDetailsView().GetDetailsViewController().NotifyPropertyChanged("QuantityText");
				GetDetailsView().GetDetailsViewController().ItemQuantity = isolationText;
				GetDetailsView().GetDetailsViewController().NotifyPropertyChanged("ItemQuantity");
				GetDetailsView().ShowQuantityColor(true, isolationColor);
			}
			else if (item.GetPlayerItem().GetQuantityType() == ExpansionItemQuantityType.PC)
			{
				GetDetailsView().GetDetailsViewController().QuantityText = "#STR_EXPANSION_MARKET_P2P_ITEMINFO_QUANTITY_LABEL";
				GetDetailsView().GetDetailsViewController().NotifyPropertyChanged("QuantityText");
				GetDetailsView().GetDetailsViewController().ItemQuantity = GetQuantityString(item.GetPlayerItem().GetQuantityType(), item.GetPlayerItem().GetQuantity(), quantityColor);
				GetDetailsView().GetDetailsViewController().NotifyPropertyChanged("ItemQuantity");
				GetDetailsView().ShowItemQuantity(true);
			}
		}

	#ifdef EXPANSIONMODHARDLINE
		if (item.GetPlayerItem().GetRarity() != ExpansionHardlineItemRarity.NONE)
		{
			string rarityName = typename.EnumToString(ExpansionHardlineItemRarity, item.GetPlayerItem().GetRarity());
			string rarityText = "#STR_EXPANSION_HARDLINE_" + rarityName;
			int rarityColor;
			typename type = ExpansionHardlineItemRarityColor;
			ExpansionStatic.GetVariableIntByName(type, rarityName, rarityColor);
			int a, r, g, b;
			ExpansionStatic.IntToARGB(rarityColor, a, r, g, b);

			GetDetailsView().GetDetailsViewController().Rarity = rarityText;
			GetDetailsView().GetDetailsViewController().NotifyPropertyChanged("Rarity");
			GetDetailsView().SetRarityColor(rarityColor);

			if (rarityName.Contains("Common"))
			{
				GetDetailsView().SetRarityTextColor(ARGB(255, 0, 0, 0));
			}
			else
			{
				GetDetailsView().SetRarityTextColor(ARGB(255, 255, 255, 255));
			}

			GetDetailsView().ShowItemRarity(true);
		}
	#endif

		GetDetailsView().ClearAttachments();
		GetDetailsView().ClearCargo();

		if (item.GetPlayerItem().GetContainerItemsCount() > 0)
		{
			int cargoItemsCount = 0;
			int attachmentsCount = 0;
			array<ref ExpansionP2PMarketContainerItem> containerItems = item.GetPlayerItem().GetContainerItems();
			if (containerItems && containerItems.Count() > 0)
			{
				ExpansionP2PMarketMenuCargoItem containerElement;
				for (int i = 0; i < containerItems.Count(); i++)
				{
					ExpansionP2PMarketContainerItem containerItem = item.GetPlayerItem().GetContainerItems().Get(i);
					if (!containerItem)
						continue;

					EXPrint(ToString() + "::SetSelectedListingItem - Container item: " + containerItem.ToString());
					EXPrint(ToString() + "::SetSelectedListingItem - Container item class name: " + containerItem.GetClassName());

					containerElement = new ExpansionP2PMarketMenuCargoItem(containerItem, this);
					if (!containerItem.IsAttached())
					{
						EXPrint(ToString() + "::SetSelectedListingItem - Add Container item " + containerItem.GetClassName() + " as cargo item.");
						GetDetailsView().AddCargoEntry(containerElement);
						cargoItemsCount++;
					}
					else
					{
						EXPrint(ToString() + "::SetSelectedListingItem - Add Container item " + containerItem.GetClassName() + " as attachment item.");
						GetDetailsView().AddAttachmentEntry(containerElement);
						attachmentsCount++;
					}

					array<ref ExpansionP2PMarketContainerItem> containerItemsOfContainerItem = containerItem.GetContainerItems();
					if (containerItemsOfContainerItem && containerItemsOfContainerItem.Count() > 0)
					{
						foreach (ExpansionP2PMarketContainerItem containerItemOfContainerItem: containerItemsOfContainerItem)
						{
							EXPrint(ToString() + "::SetSelectedListingItem - Cargo item of container item: " + containerItemOfContainerItem.ToString());
							EXPrint(ToString() + "::SetSelectedListingItem - Cargo item of container item class name: " + containerItemOfContainerItem.GetClassName());

							containerElement = new ExpansionP2PMarketMenuCargoItem(containerItemOfContainerItem, this);
							if (!containerItemOfContainerItem.IsAttached())
							{
								EXPrint(ToString() + "::SetSelectedListingItem - Add Cargo item of container item " + containerItemOfContainerItem.GetClassName() + " as cargo item.");
								GetDetailsView().AddCargoEntry(containerElement);
								cargoItemsCount++;
							}
							else
							{
								EXPrint(ToString() + "::SetSelectedListingItem - Add Cargo item of container item " + containerItemOfContainerItem.GetClassName() + " as attachment item.");
								GetDetailsView().AddAttachmentEntry(containerElement);
								attachmentsCount++;
							}
						}
					}
				}
			}

			if (cargoItemsCount > 0)
			{
				GetDetailsView().ShowItemCargoCount(true);
				GetDetailsView().GetDetailsViewController().CargoItemsCount = cargoItemsCount.ToString();
				GetDetailsView().GetDetailsViewController().NotifyPropertyChanged("CargoItemsCount");
			}

			if (attachmentsCount > 0)
			{
				GetDetailsView().ShowItemAttachmentsCount(true);
				GetDetailsView().GetDetailsViewController().AttachmentItemsCount = attachmentsCount.ToString();
				GetDetailsView().GetDetailsViewController().NotifyPropertyChanged("AttachmentItemsCount");
			}
		}

		int lowest = GetLowestListingPrice(item.GetPlayerItem().GetClassName());
		if (lowest > 0)
		{
			GetDetailsView().GetDetailsViewController().LowestPrice = GetDisplayPrice(lowest, false, true, true);
		}
		else
		{
			GetDetailsView().GetDetailsViewController().LowestPrice = "#STR_EXPANSION_MARKET_P2P_NAN";
		}

		GetDetailsView().GetDetailsViewController().NotifyPropertyChanged("LowestPrice");

		int highest = GetHighestListingPrice(item.GetPlayerItem().GetClassName());
		if (highest > 0)
		{
			GetDetailsView().GetDetailsViewController().HighestPrice = GetDisplayPrice(highest, false, true, true);
		}
		else
		{
			GetDetailsView().GetDetailsViewController().HighestPrice = "#STR_EXPANSION_MARKET_P2P_NAN";
		}

		GetDetailsView().GetDetailsViewController().NotifyPropertyChanged("HighestPrice");

		if (GetExpansionSettings().GetMarket().MarketSystemEnabled)
		{
			int marketPrice = GetPlayerItemMarketPrice(item.GetPlayerItem());
			if (marketPrice > 0)
			{
				GetDetailsView().GetDetailsViewController().MarketPrice = GetDisplayPrice(marketPrice, false, true, true);
			}
			else
			{
				GetDetailsView().GetDetailsViewController().MarketPrice = "#STR_EXPANSION_MARKET_P2P_NAN";
			}

			GetDetailsView().GetDetailsViewController().NotifyPropertyChanged("MarketPrice");
		}

		int depositTime = m_P2PMarketSettings.SalesDepositTime;
		int depositTimeDays = Math.Round(depositTime / 86400);

		auto localiser = new CF_Localiser("STR_EXPANSION_MARKET_P2P_INFOBOX_EXPIREDSALES_DESC", depositTimeDays.ToString());
		GetDetailsView().GetDetailsViewController().InfoTextOne = localiser.Format();
		GetDetailsView().GetDetailsViewController().NotifyPropertyChanged("InfoTextOne");

		int maxListingTime = m_P2PMarketSettings.MaxListingTime;
		int listingTimeDays = Math.Round(maxListingTime / 86400);

		localiser = new CF_Localiser("STR_EXPANSION_MARKET_P2P_INFOBOX_EXPIREDLISTING_DESC", depositTimeDays.ToString());
		GetDetailsView().GetDetailsViewController().InfoTextTwo = localiser.Format();
		GetDetailsView().GetDetailsViewController().NotifyPropertyChanged("InfoTextTwo");

		int feePercent = m_P2PMarketSettings.ListingPricePercent;

		localiser = new CF_Localiser("STR_EXPANSION_MARKET_P2P_INFOBOX_LISTINGFEE_DESC", feePercent.ToString());
		GetDetailsView().GetDetailsViewController().InfoTextThree = localiser.Format();
		GetDetailsView().GetDetailsViewController().NotifyPropertyChanged("InfoTextThree");

		GetDetailsView().Show();
		GetDetailsView().UpdatePreview();

		m_PreviousViewState = m_ViewState;
		m_ViewState = ExpansionP2PMarketMenuViewState.DetailViewPlayerItem;

		PlayObjectSound();
	}

	protected int GetPlayerItemMarketPrice(ExpansionP2PMarketPlayerItem playerItem)
	{
		int price = ExpansionP2PMarketModule.GetMarketPrice(playerItem.GetClassName());
		array<ref ExpansionP2PMarketContainerItem> containerItems = playerItem.GetContainerItems();
		if (containerItems && containerItems.Count() > 0)
		{
			foreach (ExpansionP2PMarketContainerItem containerItem: containerItems)
			{
				int containerItemPrice = ExpansionP2PMarketModule.GetMarketPrice(containerItem.GetClassName());
				price += containerItemPrice;
			}
		}

		return price;
	}

	protected string GetQuantityString(int quantityType, float quantity, out int color)
	{
		string quantity_str;
		if (quantityType == ExpansionItemQuantityType.PC)
		{
			if (quantity == 1)
			{
				quantity_str =  quantity.ToString() + " " + "#inv_inspect_piece";
				color = Colors.COLOR_DEFAULT;
			}
			else
			{
				quantity_str =  quantity.ToString() + " " + "#inv_inspect_pieces";
				color = Colors.COLOR_DEFAULT;
			}
		}
		else if (quantityType == ExpansionItemQuantityType.PERCENTAGE)
		{
			quantity_str = "#inv_inspect_remaining " + quantity.ToString() + "#inv_inspect_percent";
			color = Colors.COLOR_DEFAULT;
		}
		else if (quantityType == ExpansionItemQuantityType.GRAM)
		{
			quantity_str = "#inv_inspect_remaining " + quantity.ToString() + "#inv_inspect_percent";
			color = Colors.COLOR_DEFAULT;
		}
		else if (quantityType == ExpansionItemQuantityType.MILLILITER)
		{
			quantity_str = "#inv_inspect_remaining " + quantity.ToString() + "#inv_inspect_percent";
			color = Colors.COLOR_DEFAULT;
		}
		else if (quantityType == ExpansionItemQuantityType.POWER)
		{
			quantity_str = "#inv_inspect_remaining " + quantity.ToString() + "#inv_inspect_percent";
			color = Colors.COLOR_DEFAULT;
		}
		else if (quantityType == ExpansionItemQuantityType.MAGAZINE)
		{
			if (quantity == 1)
			{
				quantity_str = quantity.ToString() + " " + "#inv_inspect_piece";
				color = Colors.COLOR_DEFAULT;
			}
			else
			{
				quantity_str = quantity.ToString() + " " + "#inv_inspect_pieces";
				color = Colors.COLOR_DEFAULT;
			}
		}
		else
		{
			quantity_str = "";
			color = 0;
		}

		return quantity_str;
	}

	protected string GetFoodStageString(int foodStage, out int color)
	{
		if (foodStage == -1)
		{
			color = Colors.COLOR_ROTTEN;
			return "ERROR";
		}

		string text;
		switch (foodStage)
		{
			case FoodStageType.RAW:
			{
				text = "#inv_inspect_raw";
				color = Colors.COLOR_RAW;
				break;
			}
			case FoodStageType.BAKED:
			{
				text = "#inv_inspect_baked";
				color = Colors.COLOR_BAKED;
				break;
			}
			case FoodStageType.BOILED:
			{
				text = "#inv_inspect_boiled";
				color = Colors.COLOR_BOILED;
				break;
			}
			case FoodStageType.DRIED:
			{
				text = "#inv_inspect_dried";
				color = Colors.COLOR_DRIED;
				break;
			}
			case FoodStageType.BURNED:
			{
				text = "#inv_inspect_burned";
				color = Colors.COLOR_BURNED;
				break;
			}
			case FoodStageType.ROTTEN:
			{
				text = "#inv_inspect_rotten";
				color = Colors.COLOR_ROTTEN;
				break;
			}
		}

		return text;
	}

	protected string GetLiquidTypeString(int liquid_type, bool isBloodContainer, out int color)
	{
		string text;
		if (isBloodContainer)
		{
			if (liquid_type > -1)
			{
				string type;
				bool positive;
				string blood_type_name = BloodTypes.GetBloodTypeName(liquid_type, type, positive);
				text = "#inv_inspect_blood: " + blood_type_name;
				color = Colors.COLOR_LIQUID;
			}
			else if (liquid_type == -1)
			{
				text = "#inv_inspect_blood";
				color = Colors.COLOR_LIQUID;
			}
		}
		else if (liquid_type > -1)
		{
			text = ExpansionWorld.GetLiquidDisplayName(liquid_type, true, color);
		}

		return text;
	}

	void SetSelectedListingItem(ExpansionP2PMarketMenuListing listing)
	{
		if (!listing || !listing.GetListing())
			return;

		GetDetailsView().ResetPreview();
		m_SelectedListing = listing;
		m_SelectedPlayerItem = null;
		m_SelectedPreviewObject = listing.GetPreviewObject();

		m_SelectedContainerItems = listing.GetListing().GetContainerItems();

		GetDetailsView().SetViewListing();
		GetDetailsView().GetDetailsViewController().SelectedName = listing.GetPreviewObject().GetDisplayName();
		GetDetailsView().GetDetailsViewController().NotifyPropertyChanged("SelectedName");

		if (!listing.GetPreviewObject().IsInherited(ZombieBase) || !listing.GetPreviewObject().IsTransport())
		{
			GetDetailsView().GetDetailsViewController().ItemDescription = ExpansionStatic.GetItemDescriptionWithType(listing.GetListing().GetClassName());
			GetDetailsView().GetDetailsViewController().NotifyPropertyChanged("ItemDescription");
			GetDetailsView().GetItemDescriptionWidget().Update();
		}

		if (listing.IsOwnerdItem())
		{
			int discountPrice = ExpansionP2PMarketModule.GetDiscountPrice(listing.GetListing().GetPrice());
			GetDetailsView().GetDetailsViewController().ListingPrice = GetDisplayPrice(discountPrice, false, true, true);
		}
		else
		{
			GetDetailsView().GetDetailsViewController().ListingPrice = GetDisplayPrice(listing.GetListing().GetPrice(), false, true, true);
		}

		GetDetailsView().GetDetailsViewController().NotifyPropertyChanged("ListingPrice");

		market_filter_panel.Show(false);
		categories_panel.Show(false);
		inventory_panel.Show(false);
		tabs_panel.Show(false);

		GetDetailsView().ShowConfirmButton(true);
		GetDetailsView().GetDetailsViewController().ConfirmButtonText = "#STR_EXPANSION_MARKET_P2P_PURCHASE_BUTTON_LABEL";
		GetDetailsView().GetDetailsViewController().NotifyPropertyChanged("ConfirmButtonText");

		if (m_ListHeader.IsVisible())
			m_ListHeader.Hide();

		details_panel.AddChild(GetDetailsView().GetLayoutRoot());
		details_panel.Show(true);

		HideItemDetailElements();

		UpdateItemInfoDamage(listing.GetListing().GetHealthLevel());

		listing.GetListing().UpdateContainerItems();

		if (listing.GetListing().GetLiquidType() != -1)
		{
			bool isBloodContainer = ExpansionStatic.IsAnyOf(listing.GetListing().GetClassName(), m_BloodContainers);
			int liquidColor;
			GetDetailsView().ShowItemLiquid(true);
			GetDetailsView().GetDetailsViewController().LiquidType = GetLiquidTypeString(listing.GetListing().GetLiquidType(), isBloodContainer, liquidColor);
			GetDetailsView().GetDetailsViewController().NotifyPropertyChanged("LiquidType");
			GetDetailsView().SetLiquidColor(liquidColor);
		}

		if (listing.GetListing().GetFoodStageType() != -1)
		{
			int foodStageColor;
			GetDetailsView().ShowItemFoodState(true);
			GetDetailsView().GetDetailsViewController().FoodState = GetFoodStageString(listing.GetListing().GetFoodStageType(), foodStageColor);
			GetDetailsView().GetDetailsViewController().NotifyPropertyChanged("FoodState");
			GetDetailsView().SetFoodStateColor(foodStageColor);
		}

		if (listing.GetListing().GetQuantity() > 0)
		{
			int quantityColor;
			if (listing.GetListing().GetQuantityType() == ExpansionItemQuantityType.POWER || listing.GetListing().GetQuantityType() == ExpansionItemQuantityType.MILLILITER || listing.GetListing().GetQuantityType() == ExpansionItemQuantityType.PERCENTAGE || listing.GetListing().GetQuantityType() == ExpansionItemQuantityType.GRAM)
			{
				GetDetailsView().GetDetailsViewController().QuantityText = "#STR_EXPANSION_MARKET_P2P_ITEMINFO_QUANTITY_LABEL";
				GetDetailsView().GetDetailsViewController().NotifyPropertyChanged("QuantityText");
				GetDetailsView().GetQuantityBarWidget().SetCurrent(listing.GetListing().GetQuantity());
				GetDetailsView().GetDetailsViewController().ItemQuantity = GetQuantityString(listing.GetListing().GetQuantityType(), listing.GetListing().GetQuantity(), quantityColor);;
				GetDetailsView().GetDetailsViewController().NotifyPropertyChanged("ItemQuantity");
				GetDetailsView().ShowQuantityBar(true);
			}
			else if (listing.GetListing().GetQuantityType() == ExpansionItemQuantityType.HEATISOLATION)
			{
				int quantity = listing.GetListing().GetQuantity();
				string isolationText;
				int isolationColor;
				if (quantity <= GameConstants.HEATISO_THRESHOLD_BAD)
				{
					isolationText = "#inv_inspect_iso_bad";
					isolationColor = GetTemperatureColor(10);
				}
				else if ((quantity > GameConstants.HEATISO_THRESHOLD_BAD) && (quantity <= GameConstants.HEATISO_THRESHOLD_LOW))
				{
					isolationText = "#inv_inspect_iso_low";
					isolationColor = GetTemperatureColor(20);
				}
				else if ((quantity > GameConstants.HEATISO_THRESHOLD_LOW) && (quantity <= GameConstants.HEATISO_THRESHOLD_MEDIUM))
				{
					isolationText = "#inv_inspect_iso_medium";
					isolationColor = GetTemperatureColor(30);
				}
				else if ((quantity > GameConstants.HEATISO_THRESHOLD_MEDIUM) && (quantity <= GameConstants.HEATISO_THRESHOLD_HIGH))
				{
					isolationText = "#inv_inspect_iso_high";
					isolationColor = GetTemperatureColor(50);
				}
				else
				{
					isolationText = "#inv_inspect_iso_excel";
					isolationColor = GetTemperatureColor(70);
				}

				GetDetailsView().GetDetailsViewController().QuantityText = "#STR_EXPANSION_MARKET_P2P_ITEMINFO_ISOLATIONLABEL";
				GetDetailsView().GetDetailsViewController().NotifyPropertyChanged("QuantityText");
				GetDetailsView().GetDetailsViewController().ItemQuantity = isolationText;
				GetDetailsView().GetDetailsViewController().NotifyPropertyChanged("ItemQuantity");
				GetDetailsView().ShowQuantityColor(true, isolationColor);
			}
			else if (listing.GetListing().GetQuantityType() == ExpansionItemQuantityType.PC)
			{
				GetDetailsView().GetDetailsViewController().QuantityText = "#STR_EXPANSION_MARKET_P2P_ITEMINFO_QUANTITY_LABEL";
				GetDetailsView().GetDetailsViewController().NotifyPropertyChanged("QuantityText");
				GetDetailsView().GetDetailsViewController().ItemQuantity = GetQuantityString(listing.GetListing().GetQuantityType(),listing.GetListing().GetQuantity(), quantityColor);
				GetDetailsView().GetDetailsViewController().NotifyPropertyChanged("ItemQuantity");
				GetDetailsView().ShowItemQuantity(true);
			}
		}

		GetDetailsView().ClearAttachments();
		GetDetailsView().ClearCargo();

		if (listing.GetListing().GetContainerItemsCount() > 0)
		{
			GetDetailsView().ShowItemCargoCount(true);
			GetDetailsView().GetDetailsViewController().CargoItemsCount = listing.GetListing().GetContainerItemsCount().ToString();
			GetDetailsView().GetDetailsViewController().NotifyPropertyChanged("CargoItemsCount");

			int cargoItemsCount;
			int attachmentsCount;
			array<ref ExpansionP2PMarketContainerItem> containerItems = listing.GetListing().GetContainerItems();
			if (containerItems && containerItems.Count() > 0)
			{
				ExpansionP2PMarketMenuCargoItem containerElement;
				for (int i = 0; i < containerItems.Count(); i++)
				{
					ExpansionP2PMarketContainerItem containerItem = containerItems[i];
					if (!containerItem)
						continue;

					EXPrint(ToString() + "::SetSelectedListingItem - Container item: " + containerItem.ToString());
					EXPrint(ToString() + "::SetSelectedListingItem - Container item class name: " + containerItem.GetClassName());

					containerElement = new ExpansionP2PMarketMenuCargoItem(containerItem, this);
					if (!containerElement)
						continue;

					if (!containerItem.IsAttached())
					{
						GetDetailsView().AddCargoEntry(containerElement);
						cargoItemsCount++;
					}
					else
					{
						GetDetailsView().AddAttachmentEntry(containerElement);
						attachmentsCount++;
					}

					array<ref ExpansionP2PMarketContainerItem> containerItemsOfContainerItem = containerItem.GetContainerItems();
					if (containerItemsOfContainerItem && containerItemsOfContainerItem.Count() > 0)
					{
						foreach (ExpansionP2PMarketContainerItem containerItemOfContainerItem: containerItemsOfContainerItem)
						{
							containerElement = new ExpansionP2PMarketMenuCargoItem(containerItemOfContainerItem, this);
							if (!containerItemOfContainerItem.IsAttached())
							{
								GetDetailsView().AddCargoEntry(containerElement);
								cargoItemsCount++;
							}
							else
							{
								GetDetailsView().AddAttachmentEntry(containerElement);
								attachmentsCount++;
							}
						}
					}
				}
			}

			if (cargoItemsCount > 0)
			{
				GetDetailsView().ShowItemCargoCount(true);
				GetDetailsView().GetDetailsViewController().CargoItemsCount = cargoItemsCount.ToString();
				GetDetailsView().GetDetailsViewController().NotifyPropertyChanged("CargoItemsCount");
			}

			if (attachmentsCount > 0)
			{
				GetDetailsView().ShowItemAttachmentsCount(true);
				GetDetailsView().GetDetailsViewController().AttachmentItemsCount = attachmentsCount.ToString();
				GetDetailsView().GetDetailsViewController().NotifyPropertyChanged("AttachmentItemsCount");
			}
		}

	#ifdef EXPANSIONMODHARDLINE
		if (listing.GetListing().GetRarity() != ExpansionHardlineItemRarity.NONE)
		{
			string rarityName = typename.EnumToString(ExpansionHardlineItemRarity, listing.GetListing().GetRarity());
			string rarityText = "#STR_EXPANSION_HARDLINE_" + rarityName;
			int rarityColor;
			typename type = ExpansionHardlineItemRarityColor;
			ExpansionStatic.GetVariableIntByName(type, rarityName, rarityColor);
			int a, r, g, b;
			ExpansionStatic.IntToARGB(rarityColor, a, r, g, b);

			GetDetailsView().GetDetailsViewController().Rarity = rarityText;
			GetDetailsView().GetDetailsViewController().NotifyPropertyChanged("Rarity");
			GetDetailsView().SetRarityColor(rarityColor);

			if (rarityName.Contains("Common"))
			{
				GetDetailsView().SetRarityTextColor(ARGB(255, 0, 0, 0));
			}
			else
			{
				GetDetailsView().SetRarityTextColor(ARGB(255, 255, 255, 255));
			}

			GetDetailsView().ShowItemRarity(true);
		}
	#endif

		GetDetailsView().Show();
		GetDetailsView().UpdatePreview();

		m_PreviousViewState = m_ViewState;
		m_ViewState = ExpansionP2PMarketMenuViewState.DetailViewListingItem;

		PlayObjectSound();
	}

	void OnBackClick()
	{
		ClearSelected();

		if (m_ViewState == ExpansionP2PMarketMenuViewState.DetailViewPlayerItem)
		{
			OnListButtonClick();
		}
		else if (m_ViewState == ExpansionP2PMarketMenuViewState.DetailViewListingItem)
		{
			OnBrowseButtonClick();
		}
		if (m_ViewState == ExpansionP2PMarketMenuViewState.ViewSales)
		{
			OnSalesButtonClick();
		}
	}

	void GetSaleFromListing(ExpansionP2PMarketListing listing)
	{
		if (m_SoldListingsCount - 1 == 0)
			OnBrowseButtonClick();	//! Switch from sales to browse view when there is no sales left.

		m_P2PMarketModule.GetSaleFromListing(listing, m_TraderID);
	}

	protected void ClearSelected()
	{
		m_SelectedListing = null;
		m_SelectedPlayerItem = null;
		m_SelectedPreviewObject = null;
		m_SelectedContainerItems.Clear();
		m_CurrentItemPrice = 0;

		GetDetailsView().ShowConfirmButton(false);

		if (m_PreviousViewState == ExpansionP2PMarketMenuViewState.DetailViewPlayerItem || m_PreviousViewState == ExpansionP2PMarketMenuViewState.DetailViewListingItem)
		{
			GetDetailsView().GetDetailsViewController().SelectedName = "";
			GetDetailsView().GetDetailsViewController().NotifyPropertyChanged("SelectedName");

			GetDetailsView().GetEditBoxWidget().SetText("");
			GetDetailsView().GetDetailsViewController().ListCost = "";
			GetDetailsView().GetDetailsViewController().NotifyPropertyChanged("ListCost");
		}
	}

	void SortListingsByName(bool reverse = false)
	{
		TStringArray displayName = new TStringArray;
		foreach (ExpansionP2PMarketMenuListing entry: m_ItemListings)
		{
			if (!entry || !entry.GetListing())
				continue;

			displayName.Insert(entry.GetPreviewObject().GetDisplayName());
		}

		displayName.Sort(reverse);

		foreach (ExpansionP2PMarketMenuListing currentEntry: m_ItemListings)
		{
			if (!currentEntry || !currentEntry.GetListing())
				continue;

			string name = currentEntry.GetPreviewObject().GetDisplayName();
			int index = displayName.Find(name) + 1;
			currentEntry.SetSort(index, false);
		}
	}

	void SortListingsByOwnerName(bool reverse = false)
	{
		TStringArray ownerName = new TStringArray;
		foreach (ExpansionP2PMarketMenuListing entry: m_ItemListings)
		{
			if (!entry || !entry.GetListing())
				continue;

			ownerName.Insert(entry.GetListing().GetOwnerName());
		}

		ownerName.Sort(reverse);

		foreach (ExpansionP2PMarketMenuListing currentEntry: m_ItemListings)
		{
			if (!currentEntry || !currentEntry.GetListing())
				continue;

			string name = entry.GetListing().GetOwnerName();
			int index = ownerName.Find(name) + 1;
			currentEntry.SetSort(index, false);
		}
	}

	void SortListingsByPrice(bool reverse = false)
	{
		TIntArray prices = new TIntArray;
		foreach (ExpansionP2PMarketMenuListing entry: m_ItemListings)
		{
			if (!entry || !entry.GetListing())
				continue;

			prices.Insert(entry.GetListing().GetPrice());
		}

		prices.Sort(reverse);

		foreach (ExpansionP2PMarketMenuListing currentEntry: m_ItemListings)
		{
			if (!currentEntry || !currentEntry.GetListing())
				continue;

			int price = currentEntry.GetListing().GetPrice();
			int index = prices.Find(price) + 1;
			currentEntry.SetSort(index, false);
		}
	}

	void SortListingsByTime(bool reverse = false)
	{
		TIntArray times = new TIntArray;
		foreach (ExpansionP2PMarketMenuListing entry: m_ItemListings)
		{
			if (!entry || !entry.GetListing())
				continue;

			times.Insert(entry.GetRemainingTime());
		}

		times.Sort(reverse);

		foreach (ExpansionP2PMarketMenuListing currentEntry: m_ItemListings)
		{
			if (!currentEntry || !currentEntry.GetListing())
				continue;

			int time = currentEntry.GetRemainingTime();
			int index = times.Find(time) + 1;
			currentEntry.SetSort(index, false);
		}
	}

	void SetCategory_OwnedListings()
	{
		foreach (ExpansionP2PMarketMenuListing entry: m_ItemListings)
		{
			if (!entry || !entry.GetListing())
				continue;

			if (entry.GetListing().GetOwnerUID() != m_PlayerUID)
			{
				entry.Hide();
			}
			else
			{
				entry.Show();
			}
		}
	}

	int GetOwnedListingsCount()
	{
		int count;
		foreach (ExpansionP2PMarketMenuListing entry: m_ItemListings)
		{
			if (!entry || !entry.GetListing())
				continue;

			if (entry.GetListing().GetOwnerUID() == m_PlayerUID)
				count++;
		}

		return count;
	}

	void SetCategory_All()
	{
		foreach (ExpansionP2PMarketMenuListing entry: m_ItemListings)
		{
			if (!entry || !entry.GetListing())
				continue;

			entry.Show();
		}
	}

	//! FILTERS
	void OnListButtonClick()
	{
		if (m_ViewState == ExpansionP2PMarketMenuViewState.ViewBrowse)
		{
			m_ListHeader.Hide();
			tabs_panel.Show(false);
		}

		if (m_ViewState == ExpansionP2PMarketMenuViewState.DetailViewPlayerItem || m_ViewState == ExpansionP2PMarketMenuViewState.DetailViewListingItem)
		{
			ClearSelected();
			GetDetailsView().Hide();

			categories_panel.Show(true);
			inventory_panel.Show(true);
			player_items_scroller.Show(true);
			details_panel.Show(false);
		}

		tabs_panel.Show(true);
		market_filter_panel.Show(true);
		categories_panel.Show(true);
		m_PreviousViewState = m_ViewState;
		m_ViewState = ExpansionP2PMarketMenuViewState.ViewList;
		ShowListView();
	}

	void OnBrowseButtonClick()
	{
		if (m_ViewState == ExpansionP2PMarketMenuViewState.DetailViewPlayerItem || m_ViewState == ExpansionP2PMarketMenuViewState.DetailViewListingItem)
		{
			ClearSelected();
			GetDetailsView().Hide();

			categories_panel.Show(true);
			inventory_panel.Show(true);
			player_items_scroller.Show(false);
			details_panel.Show(false);
		}

		m_ListHeader.Show();
		tabs_panel.Show(true);
		market_filter_panel.Show(true);
		categories_panel.Show(true);
		m_PreviousViewState = m_ViewState;
		m_ViewState = ExpansionP2PMarketMenuViewState.ViewBrowse;
		ShowBrowseView();
	}

	void OnSalesButtonClick()
	{
		if (m_ViewState == ExpansionP2PMarketMenuViewState.ViewBrowse)
		{
			m_ListHeader.Hide();
		}

		if (m_ViewState == ExpansionP2PMarketMenuViewState.DetailViewPlayerItem || m_ViewState == ExpansionP2PMarketMenuViewState.DetailViewListingItem)
		{
			ClearSelected();
			GetDetailsView().Hide();

			player_items_scroller.Show(false);
			details_panel.Show(false);
		}

		tabs_panel.Show(true);
		market_filter_panel.Show(false);
		categories_panel.Show(false);
		inventory_panel.Show(true);
		retrieve_button_panel.Show(true);
		player_sales_panel.Show(true);

		int allSales;
		foreach (ExpansionP2PMarketMenuListing soldListing: m_SoldListings)
		{
			allSales += soldListing.GetListing().GetPrice();
		}

		m_P2PMarketMenuController.PlayerSales = GetDisplayPrice(allSales, false, true, true);
		m_P2PMarketMenuController.NotifyPropertyChanged("PlayerSales");

		m_PreviousViewState = m_ViewState;
		m_ViewState = ExpansionP2PMarketMenuViewState.ViewSales;
		ShowSalesView();
	}

	void OnRetrieveAllSalesClick()
	{
		if (m_ViewState != ExpansionP2PMarketMenuViewState.ViewSales)
			return;

		m_P2PMarketModule.RequestAllPlayerSales(m_TraderID);
	}

	protected void ShowBrowseView()
	{
		inventory_scroller.Show(true);
		player_items_scroller.Show(false);
		sold_items_scroller.Show(false);

		listing_categories_content.Show(true);
		player_categories_content.Show(false);
		toggle_categories_panel.Show(true);

		m_P2PMarketModule.RequestSendBMTraderData(m_TraderID);
	}

	protected void ShowListView()
	{
		inventory_scroller.Show(false);
		player_items_scroller.Show(true);
		sold_items_scroller.Show(false);

		listing_categories_content.Show(false);
		player_categories_content.Show(true);
		toggle_categories_panel.Show(false);

		UpdatePlayerItems();
	}

	protected void ShowSalesView()
	{
		inventory_scroller.Show(false);
		player_items_scroller.Show(false);
		sold_items_scroller.Show(true);
		categories_panel.Show(false);
		toggle_categories_panel.Show(false);

		listing_categories_content.Show(false);
		player_categories_content.Show(false);
	}

	void Listings_Filter_ClassNameAZ()
	{
		SortListingsByName();
	}

	void Listings_Filter_ClassNameZA()
	{
		SortListingsByName(true);
	}

	void Listings_Filter_OwnerNameAZ()
	{
		SortListingsByName();
	}

	void Listings_Filter_OwnerNameZA()
	{
		SortListingsByOwnerName(true);
	}

	void Listings_Filter_PriceLH()
	{
		SortListingsByPrice();
	}

	void Listings_Filter_PriceHL()
	{
		SortListingsByPrice(true);
	}

	void Listings_Filter_TimeSL()
	{
		SortListingsByTime();
	}

	void Listings_Filter_TimeLS()
	{
		SortListingsByTime(true);
	}

	//! CATEGORIES
	void OnClickCategory_Listings()
	{
		SetCategory_OwnedListings();
	}

	void OnClickCategory_All()
	{
		SetCategory_All();
	}

	void UpdateMenuCategory(ExpansionP2PMarketMenuCategoryBase category)
	{
		foreach (ExpansionP2PMarketMenuListing entry: m_ItemListings)
		{
			TStringArray included = category.GetIncluded();
			TStringArray excluded = category.GetExcluded();
			if (ExpansionStatic.IsAnyOf(entry.GetPreviewObject(), included) && !ExpansionStatic.IsAnyOf(entry.GetPreviewObject(), excluded))
			{
				entry.Show();
			}
			else
			{
				entry.Hide();
			}
		}
	}

	//! MENU EVENTS
	void OnConfirmButtonClick()
	{
		if (m_RequestsLocked)
			return;

		CF_Localiser localiser;

		if (m_ViewState == ExpansionP2PMarketMenuViewState.DetailViewListingItem && m_SelectedListing)
		{
			int price = m_SelectedListing.GetListing().GetPrice();
			if (m_SelectedListing.IsOwnerdItem())
				price = ExpansionP2PMarketModule.GetDiscountPrice(m_SelectedListing.GetListing().GetPrice());

			string priceString = GetDisplayPrice(price, false, true, true);
			localiser = new CF_Localiser("STR_EXPANSION_MARKET_P2P_CONFIRM_BUTTON_BUY_LABEL", m_SelectedListing.GetPreviewObject().GetDisplayName(), priceString);

			SetDialogView(localiser.Format());
		}
		else if (m_ViewState == ExpansionP2PMarketMenuViewState.DetailViewPlayerItem && m_SelectedPlayerItem)
		{
			//! Only numbers are allowed
			m_CurrentItemPrice = GetCheckPrice();
			if (m_CurrentItemPrice == -1 || m_CurrentItemPrice == 0)
				return;

			int listingPrice = Math.Ceil(m_CurrentItemPrice * m_P2PMarketSettings.ListingPricePercent / 100);
			string currentItemPriceString = GetDisplayPrice(m_CurrentItemPrice, false, true, true);
			string listingPriceString = GetDisplayPrice(listingPrice, false, true, true);
			localiser = new CF_Localiser("STR_EXPANSION_MARKET_P2P_CONFIRM_BUTTON_SELL_LABEL", m_SelectedPlayerItem.GetPreviewObject().GetDisplayName(), currentItemPriceString, listingPriceString);

			SetDialogView(localiser.Format());
		}
	}

	protected void SetDialogView(string text)
	{
		if (m_MenuDialog)
			m_MenuDialog.Destroy();

		m_MenuDialog = new ExpansionDialog_P2PMarketMenu(this, text);
		m_MenuDialog.Show();
	}

	void CancelDialogView()
	{
		GetDetailsView().ShowConfirmButton(true);
		GetDetailsView().Show();
		tabs_button_browse.Show(true);
		tabs_button_list.Show(true);
		exit_button.Show(true);
	}

	void OnConfirmListingClick()
	{
		if (!m_SelectedPlayerItem || !m_SelectedPlayerItem.GetPlayerItem() || m_RequestsLocked)
			return;

		EntityAI playerItem = m_SelectedPlayerItem.GetPlayerItem().GetItem();
		if (!playerItem)
			return;

		m_RequestsLocked = true;
		m_P2PMarketModule.RequestListBMItem(m_TraderID, playerItem, m_CurrentItemPrice);

		GetDetailsView().UpdatePreview();

		OnBackClick();
	}

	void OnPurchaseListingClick()
	{
		if (!m_SelectedListing || !m_SelectedListing.GetListing() || m_RequestsLocked)
			return;

		m_RequestsLocked = true;
		m_P2PMarketModule.RequestPurchaseBMItem(m_SelectedListing.GetListing());

		GetDetailsView().UpdatePreview();

		OnBackClick();
	}

	void QuickbuyListing(ExpansionP2PMarketMenuListing listing)
	{
		if (!listing || !listing.GetListing() || m_RequestsLocked)
			return;

		m_RequestsLocked = true;
		m_P2PMarketModule.RequestPurchaseBMItem(listing.GetListing());
		
		GetDetailsView().UpdatePreview();
		
		OnBrowseButtonClick();
	}

	void OnExitClick()
	{
		CloseMenu();
	}

	void OnFilterButtonClick()
	{
		market_filter_box.SetText("");
		OnSearchFilterChange();
	}

	void UpdateInventorySlotFilter(string slotName)
	{
		for (int i = 0; i < m_P2PMarketMenuController.PlayerItems.Count(); i++)
		{
			ExpansionP2PMarketMenuItem playerItem = m_P2PMarketMenuController.PlayerItems[i];
			if (playerItem.GetPlayerItem().GetSlotName() != slotName && slotName != "All")
			{
				playerItem.Hide();
			}
			else
			{
				playerItem.Show();
			}
		}
	}

	int GetInventoryCategoryItemsCount(string slotName)
	{
		int count;
		if (slotName == "All")
		{
			return m_P2PMarketMenuController.PlayerItems.Count();
		}
		else
		{
			for (int i = 0; i < m_P2PMarketMenuController.PlayerItems.Count(); i++)
			{
				ExpansionP2PMarketMenuItem playerItem = m_P2PMarketMenuController.PlayerItems[i];
				if (playerItem.GetPlayerItem().GetSlotName() == slotName)
					count++;
			}
		}

		return count;
	}

	protected void OnSearchFilterChange()
	{
		bool hasChildWithName;
		ObservableCollection<ref ExpansionP2PMarketMenuItem> cargoEntries;
		ExpansionP2PMarketMenuItem cargoEntry;
		string displayName;
		string searchText = market_filter_box.GetText();
		searchText.ToLower();

		if (m_ViewState == ExpansionP2PMarketMenuViewState.ViewList)
		{
			SearchInPlayerItems();
		}
		else if (m_ViewState == ExpansionP2PMarketMenuViewState.ViewBrowse)
		{
			SearchInListings();
		}
	}

	protected void SearchInPlayerItems()
	{
		if (!m_P2PMarketMenuController.PlayerItems || m_P2PMarketMenuController.PlayerItems.Count() == 0)
			return;

		bool hasChildWithName;
		string displayName;
		string searchText = market_filter_box.GetText();
		searchText.ToLower();

		for (int j = 0; j < m_P2PMarketMenuController.PlayerItems.Count(); j++)
		{
			ExpansionP2PMarketMenuItem playerItem = m_P2PMarketMenuController.PlayerItems[j];
			if (playerItem && playerItem.GetPreviewObject())
			{
				if (!playerItem.IsVisible() || searchText == string.Empty)
					playerItem.Show();

				hasChildWithName = false;
				ObservableCollection<ref ExpansionP2PMarketMenuItem> cargoEntries = playerItem.GetCargoItemElemens();
				if (cargoEntries && cargoEntries.Count() > 0)
				{
					for (int k = 0; k < cargoEntries.Count(); k++)
					{
						ExpansionP2PMarketMenuItem cargoEntry = cargoEntries[k];
						displayName = cargoEntry.GetPreviewObject().GetDisplayName();
						displayName.ToLower();

						if (!cargoEntry.IsVisible() || searchText == string.Empty)
							cargoEntry.Show();

						if (displayName.IndexOf(searchText) == -1)
						{
							cargoEntry.Hide();
						}
						else
						{
							hasChildWithName = true;
						}
					}
				}
			}

			displayName = playerItem.GetPreviewObject().GetDisplayName();
			displayName.ToLower();

			if (displayName.IndexOf(searchText) == -1 && !hasChildWithName)
				playerItem.Hide();
		}
	}

	protected void SearchInListings()
	{
		if (!m_ItemListings || m_ItemListings.Count() == 0)
			return;

		bool hasChildWithName;
		string displayName;
		string searchText = market_filter_box.GetText();
		searchText.ToLower();

		for (int i = 0; i < m_ItemListings.Count(); i++)
		{
			ExpansionP2PMarketMenuListing entry = m_ItemListings[i];
			if (entry && entry.GetPreviewObject())
			{
				if (!entry.IsVisible() || searchText == string.Empty)
					entry.Show();

				hasChildWithName = false;
				array<ref ExpansionP2PMarketContainerItem> containerItems = entry.GetListing().GetContainerItems();
				if (containerItems && containerItems.Count() > 0)
				{
					for (int k = 0; k < containerItems.Count(); k++)
					{
						ExpansionP2PMarketContainerItem containerItem = containerItems[k];
						displayName = ExpansionStatic.GetItemDisplayNameWithType(containerItem.GetClassName());
						displayName.ToLower();

						if (displayName.IndexOf(searchText) != -1)
							hasChildWithName = true;
					}
				}
			}

			displayName = entry.GetPreviewObject().GetDisplayName();
			displayName.ToLower();

			if (displayName.IndexOf(searchText) == -1 && !hasChildWithName)
				entry.Hide();
		}
	}

	protected void PlayObjectSound()
	{
		if (GetGame().IsClient() || !GetGame().IsMultiplayer())
		{
			if (GetGame().GetPlayer())
			{
				string typeName;
				string soundConfig;
				string path;
				if (m_SelectedPlayerItem)
					typeName = m_SelectedPlayerItem.GetPreviewObject().GetType();
				else if (m_SelectedListing)
					typeName = m_SelectedListing.GetPreviewObject().GetType();

				path = "cfgVehicles " + typeName + " " + "AnimEvents ";

				int events_sources_count = GetGame().ConfigGetChildrenCount(path);
				string soundName = "";
				EffectSound sound;

				for (int i = 0; i < events_sources_count; i++)
				{
					string soundWeapons;
					GetGame().ConfigGetChildName(path, i, soundWeapons);

					if (GetGame().ConfigIsExisting(path + soundWeapons + " pickUpItem" + " soundSet"))
						GetGame().ConfigGetText(path + soundWeapons + " pickUpItem" + " soundSet", soundName);
					else if (GetGame().ConfigIsExisting(path + soundWeapons + " pickUpItem_Light" + " soundSet"))
						GetGame().ConfigGetText(path + soundWeapons + " pickUpItem_Light" + " soundSet", soundName);
				}

				if (soundName != "")
					sound = SEffectManager.PlaySound(soundName, GetGame().GetPlayer().GetPosition());
				else
					sound = SEffectManager.PlaySound("pickUpBackPack_Metal_SoundSet", GetGame().GetPlayer().GetPosition());

				sound.SetSoundAutodestroy(true);
			}
		}
	}

	void OnToggleCategoriesClick()
	{
		m_CategoriesToggleState = !m_CategoriesToggleState;
		for (int i = 0; i < m_P2PMarketMenuController.ListingCategories.Count(); i++)
		{
			ExpansionP2PMarketMenuCategoryElement categoryElement = m_P2PMarketMenuController.ListingCategories[i];
			if (!categoryElement)
				continue;

			categoryElement.ShowSubCategories(m_CategoriesToggleState);
		}
	}

	override void OnShow()
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
#endif
		
		super.OnShow();

		loading.Show(true);
		SetFocus(GetLayoutRoot());
	}

	override bool OnChange(Widget w, int x, int y, bool finished)
	{
		if (w == market_filter_box)
		{
			OnSearchFilterChange();
			return true;
		}

		return false;
	}

	override bool OnMouseEnter(Widget w, int x, int y)
	{
		if (w != NULL)
		{
			if (w == tabs_button_browse)
			{
				tabs_button_browse.SetColor(ARGB(255, 255, 255, 255));
				tabs_button_browse_text.SetColor(ARGB(255, 0, 0, 0));
			}
			else if (w == tabs_button_list)
			{
				tabs_button_list.SetColor(ARGB(255, 255, 255, 255));
				tabs_button_list_text.SetColor(ARGB(255, 0, 0, 0));
			}
			else if (w == exit_button)
			{
				exit_button.SetColor(ARGB(255, 255, 255, 255));
				exit_button_text.SetColor(ARGB(255, 0, 0, 0));
			}
			else if (w == retrieve_button)
			{
				retrieve_button.SetColor(ARGB(255, 255, 255, 255));
				retrieve_button_text.SetColor(ARGB(255, 0, 0, 0));
			}
			else if (w == listing_category_listings_button)
			{
				listing_category_listings_button.SetColor(ARGB(255, 255, 255, 255));
				listing_category_listings_text.SetColor(ARGB(255, 0, 0, 0));
				listing_category_listings_icon.SetColor(ARGB(255, 0, 0, 0));
				listing_category_listings_count.SetColor(ARGB(255, 0, 0, 0));
			}
			else if (w == listing_category_all_button)
			{
				listing_category_all_button.SetColor(ARGB(255, 255, 255, 255));
				listing_category_all_text.SetColor(ARGB(255, 0, 0, 0));
				listing_category_all_icon.SetColor(ARGB(255, 0, 0, 0));
				listing_category_all_count.SetColor(ARGB(255, 0, 0, 0));
			}
			else if (w == toggle_categories_button)
			{
				toggle_categories_button.SetColor(ARGB(255, 255, 255, 255));
				toggle_categories_text.SetColor(ARGB(255, 0, 0, 0));
			}
		}

		return false;
	}

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		if (w != NULL)
		{
			if (w == tabs_button_browse)
			{
				tabs_button_browse.SetColor(ARGB(255, 19, 23, 27));
				tabs_button_browse_text.SetColor(ARGB(255, 255, 255, 255));
			}
			else if (w == tabs_button_list)
			{
				tabs_button_list.SetColor(ARGB(255, 19, 23, 27));
				tabs_button_list_text.SetColor(ARGB(255, 255, 255, 255));
			}
			else if (w == exit_button)
			{
				exit_button.SetColor(ARGB(255, 0, 0, 0));
				exit_button_text.SetColor(ARGB(255, 255, 255, 255));
			}
			else if (w == retrieve_button)
			{
				retrieve_button.SetColor(ARGB(255, 0, 0, 0));
				retrieve_button_text.SetColor(ARGB(255, 255, 255, 255));
			}
			else if (w == listing_category_listings_button)
			{
				listing_category_listings_button.SetColor(ARGB(255, 0, 0, 0));
				listing_category_listings_text.SetColor(ARGB(255, 255, 255, 255));
				listing_category_listings_icon.SetColor(ARGB(255, 255, 255, 255));
				listing_category_listings_count.SetColor(ARGB(255, 255, 255, 255));
			}
			else if (w == listing_category_all_button)
			{
				listing_category_all_button.SetColor(ARGB(255, 0, 0, 0));
				listing_category_all_text.SetColor(ARGB(255, 255, 255, 255));
				listing_category_all_icon.SetColor(ARGB(255, 255, 255, 255));
				listing_category_all_count.SetColor(ARGB(255, 255, 255, 255));
			}
			else if (w == toggle_categories_button)
			{
				toggle_categories_button.SetColor(ARGB(255, 0, 0, 0));
				toggle_categories_text.SetColor(ARGB(255, 255, 255, 255));
			}
		}

		return false;
	}

	ExpansionP2PMarketMenuDetailsView GetDetailsView()
	{
		return m_ItemDetailsView;
	}

	WrapSpacerWidget GetListingsWrapSpacer()
	{
		return inventory_content;
	}

	WrapSpacerWidget GetSoldItemsWrapSpacer()
	{
		return sold_items_content;
	}

	Object GetSelectedPreviewObject()
	{
		return m_SelectedPreviewObject;
	}

	array<ref ExpansionP2PMarketContainerItem> GetSelectedContainerItems()
	{
		return m_SelectedContainerItems;
	}

	protected int GetLowestListingPrice(string typeName)
	{
		int lowest;
		foreach (ExpansionP2PMarketMenuListing entry: m_ItemListings)
		{
			if (!entry || !entry.GetListing())
				continue;

			if (entry.GetListing().GetClassName() != typeName)
				continue;

			if (lowest == 0 || lowest != 0 && entry.GetListing().GetPrice() < lowest)
				lowest = entry.GetListing().GetPrice();
		}

		return lowest;
	}

	protected int GetHighestListingPrice(string typeName)
	{
		int highest;
		foreach (ExpansionP2PMarketMenuListing entry: m_ItemListings)
		{
			if (!entry || !entry.GetListing())
				continue;

			if (entry.GetListing().GetClassName() != typeName)
				continue;

			if (highest == 0 || highest != 0 && entry.GetListing().GetPrice() > highest)
				highest = entry.GetListing().GetPrice();
		}

		return highest;
	}

	int GetCategoryListingsCount(ExpansionP2PMarketMenuCategoryBase category)
	{
		int count;
		foreach (ExpansionP2PMarketMenuListing entry: m_ItemListings)
		{
			if (!entry || !entry.GetListing())
				continue;

			TStringArray included = category.GetIncluded();
			TStringArray excluded = category.GetExcluded();
			if (ExpansionStatic.IsAnyOf(entry.GetPreviewObject(), included) && !ExpansionStatic.IsAnyOf(entry.GetPreviewObject(), excluded))
				count++;
		}

		return count;
	}

	int GetViewState()
	{
		return m_ViewState;
	}

	override void OnHide()
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.P2PMARKET, this);
#endif

		super.OnHide();

		m_P2PMarketModule.RemoveTradingPlayer(m_PlayerUID);

		if (m_ItemDetailsView)
			m_ItemDetailsView.Destroy();

		if (m_ListHeader)
			m_ListHeader.Destroy();

		if (m_MenuDialog)
			m_MenuDialog.Destroy();
	}

	string GetDisplayPrice(int price, bool shorten = false, bool format = true, bool includeDisplayCurrencyName = false)
	{
		return ExpansionMarketModule.GetDisplayPriceEx(price, shorten, format, includeDisplayCurrencyName, m_DisplayCurrencyValue, m_DisplayCurrencyPrecision, m_DisplayCurrencyName);
	}

	int GetDisplayCurrencyValue()
	{
		return m_DisplayCurrencyValue;
	}
};

class ExpansionP2PMarketMenuController: ExpansionViewController
{
	string PlayerMoney;
	string ConfirmButtonText;
	string PlayerListingsCount;
	string AllListingsCount;
	string TraderName;
	string MarketIcon;
	string PlayerSales;
	ref ObservableCollection<ref ExpansionP2PMarketMenuItem> PlayerItems = new ObservableCollection<ref ExpansionP2PMarketMenuItem>(this);
	ref ObservableCollection<ref ExpansionP2PMarketMenuCategoryElement> ListingCategories = new ObservableCollection<ref ExpansionP2PMarketMenuCategoryElement>(this);
	ref ObservableCollection<ref ExpansionP2PMarketMenuInventoryCategoryElement> PlayerCategories = new ObservableCollection<ref ExpansionP2PMarketMenuInventoryCategoryElement>(this);
};