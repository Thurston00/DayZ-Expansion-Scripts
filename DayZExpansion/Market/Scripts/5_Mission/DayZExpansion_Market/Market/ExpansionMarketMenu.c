/**
 * ExpansionMarketMenu.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

enum ExpansionMarketMenuSortPriority
{
	NAME,
	PRICE
}

class ExpansionMarketMenu: ExpansionScriptViewMenu
{
	protected ref ExpansionMarketMenuController m_MarketMenuController;
	protected ref ExpansionMarketModule m_MarketModule;
	protected ExpansionTraderObjectBase m_TraderObject;
	protected ref ExpansionMarketTrader m_TraderMarket;
	protected ref TStringArray m_CategoryNames;
	protected ref map<string, ref array<ref ExpansionMarketCategory>> m_MarketCategories;
	protected ref array<ExpansionMarketItem> m_TraderItems;
	protected ref TStringArray m_OutOfStockList;
	protected ref ExpansionMarketItem m_SelectedMarketItem;
	protected ref ExpansionMarketMenuItem m_SelectedMarketItemElement;
	protected ref ExpansionMarketMenuTooltip m_AttachmentsTooltip; 
	protected ref ExpansionMenuDialog_MarketSetQuantity m_QuantityDialog;
	protected ref ExpansionMenuDialog_MarketConfirmPurchase m_PurchaseDialog;
	protected ref ExpansionMenuDialog_MarketConfirmSell m_SellDialog;
	protected ref ExpansionMarketMenuTooltip m_CurrenciesTooltip;
	protected ref ExpansionMarketMenuTooltip m_SellDenomsTooltip;
	protected ref ExpansionMarketMenuTooltip m_BuyDenomsTooltip;
	protected ref ExpansionMarketMenuTooltip m_QuatityTooltip;	
	
	protected int m_Quantity = 1;
	protected int m_TraderItemStock;
	protected bool m_TraderHasQuantity;
	protected int m_PlayerStock;
	protected int m_BuyPrice;
	protected ref ExpansionMarketSell m_MarketSell;
	protected ExpansionMarketResult m_LastFindSellPriceResult;
	protected string m_LastFindSellPriceFailedClassName;
	protected int m_SellPrice;
	protected ExpansionMarketMenuState m_CurrentState = ExpansionMarketMenuState.LOADING;
	protected float m_RequestMarketTraderDataAccum;
	protected bool m_CategoriesCreated = false;
	protected bool m_ShowPrimeMag = false;
	protected bool m_ShowSecondMag = false;
	protected bool m_ShowPrimeAttach = false;
	protected bool m_ShowSecondAttach = false;
	protected bool m_ShowHandAttach = false;
	protected bool m_ShowHandMag = false;
	protected bool m_ShowPrimeBullets = false;
	protected bool m_ShowSecondBullets = false;
	protected bool m_ShowHandBullets = false;
	protected bool m_ShowSellable = false;
	protected bool m_ShowPurchasables = false;
	protected ref array<ref ExpansionMarketPlayerItem> m_PlayerItems;
	protected ref ExpansionMarketFilters m_MarketFilters;
	protected ref TStringArray m_FilterOptionStrings;
	protected bool m_FilterUpdateInProgress;
	protected vector m_CharacterOrientation;
	protected int m_CharacterRotationX;
	protected int m_CharacterRotationY;
	protected int m_CharacterScaleDelta;
	protected bool m_FirstCall;
	protected bool m_Complete;
	protected ref map<string, string> m_TypeDisplayNames;
	ref ExpansionPlayerPreview m_PlayerPreview;
	int CategoriesExpanded;
	protected ref ExpansionMarketMenuColorHandler m_ColorHandler;
	Object m_CurrentPreviewObject;
	bool m_KeyInput = false;
	bool m_IsDropdownExpanded = false;
	bool m_IsSkinSelectorExpanded = false;
	protected ref ExpansionMarketMenuItemManager m_MarketMenuItemManager;
	protected bool m_NameSortState;
	protected bool m_PriceSortState;
	protected ExpansionMarketMenuSortPriority m_SortPriority = ExpansionMarketMenuSortPriority.NAME;
	
	protected EditBoxWidget market_filter_box;
	protected Widget market_menu_info;
	protected ButtonWidget market_item_buy;
	protected ButtonWidget market_item_sell;
	protected ButtonWidget market_filter_clear;
	protected ImageWidget market_filter_clear_icon;
	protected ItemPreviewWidget market_item_preview;
	protected PlayerPreviewWidget market_player_preview;
	protected ScrollWidget market_categories_scroller;
	protected Widget market_loading;
	protected ImageWidget market_icon;
	protected Widget menu_item_info_content;
	protected Widget dropdown_selector;
	protected ButtonWidget dropdown_selector_button;
	protected ImageWidget dropdown_expand_image;
	protected ImageWidget dropdown_collapse_image;
	protected ScrollWidget dropdown_container;
	protected GridSpacerWidget dropdown_content;
	protected RichTextWidget market_item_info_buy_price_monies;
	protected TextWidget market_item_sell_price_text;
	protected ImageWidget market_item_sell_price_icon;
	protected RichTextWidget market_item_info_sell_price_monies;
	protected TextWidget market_item_buy_price_text;
	protected ImageWidget market_item_buy_price_icon;
	protected Widget market_item_info_attachments;
	protected ButtonWidget market_item_info_attachments_button;
	protected ImageWidget market_item_info_attachments_icon;
	protected Widget market_item_info_attachments_panel;
	protected Widget market_item_info_attachments_setup_panel;
	protected CheckBoxWidget market_item_info_attachments_checkbox;
	protected ButtonWidget player_info_content;
	protected Widget market_item_buy_price_panel;
	protected Widget market_item_sell_price_panel;
	protected ButtonWidget market_footer_categories_toggle_button;
	protected TextWidget market_footer_categories_label;
	protected ButtonWidget market_item_info_attachments_setup_button;
	protected ButtonWidget market_footer_close_menu_button;
	protected TextWidget market_footer_close_menu_label;
	protected Widget market_item_description_panel;
	protected Widget market_item_description_spacer;
	protected Widget market_item_description_scroll_indicator_header;
	protected Widget market_item_description_scroll_indicator_footer;
	protected RichTextWidget market_item_description;
	protected ScrollWidget market_item_description_scroller;
	protected Widget skin_selector;
	protected ButtonWidget skin_selector_button;
	protected ImageWidget skin_selector_collapse_image;
	protected ImageWidget skin_selector_expand_image;
	protected ScrollWidget skin_selector_container;
	protected WrapSpacerWidget skin_selector_content;
	protected CheckBoxWidget market_item_purchasables_checkbox;
	protected CheckBoxWidget market_item_sellables_checkbox;
	protected ButtonWidget market_item_info_quantity_button;
	protected Widget market_menu_content;
	protected Widget market_menu_attachments_content;
	protected Widget market_item_info_stock;
	protected Widget market_item_info_stock_player;
	protected Widget dropdown_background;
	protected Widget market_item_info_item_manager_panel;
	protected ButtonWidget sort_item_name_button;
	protected TextWidget sort_item_name_text;
	protected ImageWidget sort_item_name_icon;
	protected ButtonWidget sort_item_price_button;
	protected ImageWidget sort_item_price_symbol;
	protected ImageWidget sort_item_price_icon;
	protected Widget market_item_amount_panel;
	protected Widget market_item_price_panel;

	void ExpansionMarketMenu()
	{
		if (GetExpansionSettings().GetMarket().CurrencyIcon != "")
			sort_item_price_symbol.LoadImageFile(0, GetExpansionSettings().GetMarket().CurrencyIcon);

		sort_item_price_symbol.SetAlpha(0.4);
		sort_item_price_icon.SetAlpha(0.4);

		SetIsLoading(true);

		if (!m_MarketMenuController)
			m_MarketMenuController = ExpansionMarketMenuController.Cast(GetController());
		
		if (!m_MarketModule)
			m_MarketModule = ExpansionMarketModule.Cast(CF_ModuleCoreManager.Get(ExpansionMarketModule));
		
		if (!m_FilterOptionStrings)
			m_FilterOptionStrings = new TStringArray;
				
		if (!m_MarketFilters)
			m_MarketFilters = new ExpansionMarketFilters();
		m_MarketFilters.SetMenu(this);  //! Only added this because I didn't want to change signature of ExpansionMarketFilters constructor
				
		if (!m_TypeDisplayNames)
			m_TypeDisplayNames = new map<string, string>;
		
		m_CategoryNames = new TStringArray;
		m_MarketCategories = new map<string, ref array<ref ExpansionMarketCategory>>;
		m_TraderItems = new array<ExpansionMarketItem>;
		m_OutOfStockList = new TStringArray;
		
		ExpansionMarketModule.SI_SetTraderInvoker.Insert(SetTraderObject);
		ExpansionMarketModule.SI_SelectedItemUpdatedInvoker.Insert(OnNetworkItemUpdate);
		ExpansionMarketModule.SI_Callback.Insert(MenuCallback);
		
		if (!m_ColorHandler)
			m_ColorHandler = new ExpansionMarketMenuColorHandler(GetLayoutRoot());
		
		if (!m_QuatityTooltip)
		{
			m_QuatityTooltip = new ExpansionMarketMenuTooltip();
			m_QuatityTooltip.Hide();
			m_QuatityTooltip.SetTitle("#STR_EXPANSION_MARKET_TOOLTIP_QUANTITY_TITLE");
			m_QuatityTooltip.SetText("#STR_EXPANSION_MARKET_TOOLTIP_QUANTITY_DESC");
			m_QuatityTooltip.SetContentOffset(-0.273438, -0.104167);
		}
		
		m_MarketMenuController.ShowSellables = GetExpansionClientSettings().MarketMenuFilterSellableState;
		m_MarketMenuController.ShowPurchasables = GetExpansionClientSettings().MarketMenuFilterPurchasableState;

		market_item_sellables_checkbox.SetChecked(m_MarketMenuController.ShowSellables);
		market_item_purchasables_checkbox.SetChecked(m_MarketMenuController.ShowPurchasables);
		
		SetFilterSellables(GetExpansionClientSettings().MarketMenuFilterSellableState);
		SetFilterPurchasables(GetExpansionClientSettings().MarketMenuFilterPurchasableState);
		
		if (!m_MarketMenuItemManager)
		{
			m_MarketMenuItemManager = new ExpansionMarketMenuItemManager(this);
			m_MarketMenuItemManager.Hide();
		}

		m_PlayerPreview = new ExpansionPlayerPreview(this, market_player_preview);
	}

	void UpdatePlayerItems()
	{
		MarketPrint("UpdatePlayerItems - Start");
		
		if (!m_PlayerItems)
			m_PlayerItems = new array <ref ExpansionMarketPlayerItem>;
		else
			m_PlayerItems.Clear();
		
		array< EntityAI > items = m_MarketModule.LocalGetEntityInventory();

		foreach (EntityAI item: items)
		{
			string name = item.GetType();
			bool added = false;
			
			MarketPrint("UpdatePlayerItems - Add player item: " + name + " to player items array");
			
			foreach (ExpansionMarketPlayerItem currentItem : m_PlayerItems)
			{
				if (currentItem.ClassName == name)
				{
					currentItem.Count++;
					currentItem.UpdateContainerItems();
					added = true;
					break;
				}
			}
			
			if (!added)
			{
				m_PlayerItems.Insert(new ExpansionMarketPlayerItem(item));
			}
		}

		MarketPrint("UpdatePlayerItems - End");
	}

	bool HasPlayerItem(string name)
	{
		MarketPrint("HasPlayerItem - Start");
		
		for (int i = 0; i < m_PlayerItems.Count(); i++)
		{
			string itemName = m_PlayerItems[i].ClassName;
			itemName.ToLower();
			
			itemName = m_MarketModule.GetMarketItemClassName(m_TraderMarket, itemName);

			if (itemName == name)
			{
				MarketPrint("ExpansionMarketMenu::HasPlayerItem - End and return true" + name);
				return true;
			}
		}
		
		MarketPrint("HasPlayerItem - End and return false: " + name);
		return false;
	}

	bool HasPlayerMarketItem(ExpansionMarketItem item)
	{
		return (m_MarketModule.GetAmountInInventory(item, m_MarketModule.LocalGetEntityInventory()) > 0);
	}

	ExpansionMarketPlayerItem GetPlayerItem(string name)
	{		
		ExpansionMarketPlayerItem item;
		for (int i = 0; i < m_PlayerItems.Count(); i++)
		{
			string itemName = m_PlayerItems[i].ClassName;			
			if (itemName == name)
				item = m_PlayerItems[i];
		}
		
		return item;
	}

	array<ref ExpansionMarketPlayerItem> GetPlayerItems()
	{
		return m_PlayerItems;
	}

	override string GetLayoutFile() 
	{
		return "DayZExpansion/Market/GUI/layouts/market/expansion_market_menu.layout";
	}

	override typename GetControllerType() 
	{
		return ExpansionMarketMenuController;
	}

	override bool CanShow()
	{
		return true;
	}

	void InitMarketCategories()
	{
		MarketPrint("InitMarketCategories - Start");
		
		map<int, ref ExpansionMarketCategory> categories = GetExpansionSettings().GetMarket().GetCategories();
		if (!categories)
		{
			Error("ExpansionMarketMenu::InitMarketCategories - Could not get market categories from expansion settings");
			SetIsLoading(false);
			return;
		}

		array<ref ExpansionMarketCategory> categoriesArray;

		foreach (ExpansionMarketCategory category : categories)
		{
			string displayNameTranslated = Widget.TranslateString(category.DisplayName);

			//! Deal with the case where different categories may have the same display name
			if (!m_MarketCategories.Find(displayNameTranslated, categoriesArray))
			{
				categoriesArray = new array<ref ExpansionMarketCategory>;
				m_MarketCategories.Insert(displayNameTranslated, categoriesArray);
			}
			categoriesArray.Insert(category);
		}

		m_CategoryNames = m_MarketCategories.GetKeyArray();
		m_CategoryNames.Sort();

		int idx;
		foreach (string categoryName : m_CategoryNames)
		{
			categoriesArray = m_MarketCategories[categoryName];
			foreach (ExpansionMarketCategory currentCategory: categoriesArray)
			{
				currentCategory.m_Idx = idx++;
			}
		}
		
		MarketPrint("InitMarketCategories - End");
	}

	void CreateMenuCategories()
	{
		MarketPrint("CreateMenuCategories - Start");
		
		array<ref ExpansionMarketCategory> categoriesArray;

		string search = GetSearchText();
		search.ToLower();
				
		foreach (string categoryName : m_CategoryNames)
		{
			categoriesArray = m_MarketCategories[categoryName];
			CreateMenuCategories(categoriesArray, search);
		}

		int count = m_MarketMenuController.MarketCategories.Count();
		for (int i = 0; i < count; i++)
		{
			ExpansionMarketMenuCategory menuCategory = m_MarketMenuController.MarketCategories[i];
			if (GetExpansionClientSettings().MarketMenuCategoriesState || count == 1)
				menuCategory.ToggleCategory(true);
		}

		SetIsLoading(false);

		MarketPrint("CreateMenuCategories - End");
	}

	protected void CreateMenuCategories( array<ref ExpansionMarketCategory> categories, string search = "")
	{
		MarketPrint("CreateMenuCategories - Start");
		
		foreach (ExpansionMarketCategory currentCategory : categories)
		{
			ExpansionMarketMenuCategory marketCategoryElement = NULL;
			map<string, ref array<ExpansionMarketItem>> tempItems = NULL;

			for (int i = 0; i < m_MarketMenuController.MarketCategories.Count(); i++)
			{
				if (m_MarketMenuController.MarketCategories[i].GetCategory().CategoryID == currentCategory.CategoryID)
				{
					marketCategoryElement = m_MarketMenuController.MarketCategories[i];
					tempItems = marketCategoryElement.GetTempItems();
				}
			}

			if (!tempItems)
				tempItems = new map<string, ref array<ExpansionMarketItem>>;

			MarketPrint("-----------------------------------------------------------------------------------------------------------------------------------------------------------------------");
			MarketPrint("ExpansionMarketMenu::CreateMenuCategories - Add category: " + currentCategory.m_FileName + " | " + currentCategory.Items.Count());
			if (currentCategory.Items.Count() == 0)
				continue;
			
			int itemCount;
			int hiddenItemCount;
			foreach (ExpansionMarketItem currentItem : currentCategory.m_Items)
			{
				MarketPrint("ExpansionMarketMenu::CreateMarketCategorys - Check item: " + currentItem.ClassName);
				if (!m_TraderMarket.ItemExists(currentItem.ClassName) || m_TraderMarket.IsAttachmentBuySell(currentItem.ClassName))
					continue;
				
				string displayName = GetDisplayName(GetPreviewClassName(currentItem.ClassName, true));
				
				//! https://feedback.bistudio.com/T173348
				if ((!currentItem.m_IsVariant || (!currentItem.m_Parent.m_ShowInMenu && ShowPurchasables()) || ShowSellables() || search) && ShouldShowItem(currentItem, displayName, search))
					currentItem.m_ShowInMenu = true;
				else
					currentItem.m_ShowInMenu = false;
				
				TempInsertItem(displayName, currentItem, tempItems);
				
				itemCount++;
				
				if (!currentItem.m_ShowInMenu)
					hiddenItemCount++;
			}
			
			if ( tempItems.Count() == 0 )
				continue;
			
			if (!marketCategoryElement)
			{
				//! Create the category with its items here
				marketCategoryElement = new ExpansionMarketMenuCategory(this, currentCategory, tempItems);
				AddMenuCategory(marketCategoryElement);
			}
			else
			{
				marketCategoryElement.UpdateMarketItems();
			}

			if (hiddenItemCount == itemCount)
				marketCategoryElement.Hide();
		}
		
		MarketPrint("CreateMenuCategories - End");
	}
	
	static void CreatePreviewObject(string className, inout EntityAI preview)
	{
		if (preview)
		{
			if (CF_String.EqualsIgnoreCase(preview.GetType(), className))
			{
				//! Same classname, remove all attachments
				for (int i = preview.GetInventory().AttachmentCount() - 1; i >= 0; i--)
				{
					EntityAI attachment = preview.GetInventory().GetAttachmentFromIndex(i);
					if (attachment)
						GetGame().ObjectDelete(attachment);
				}

				return;
			}
			else
			{
				//! Different classname, delete old preview object
				GetGame().ObjectDelete(preview);
			}
		}

		if (!GetGame().IsKindOf(className, "DZ_LightAI"))
		{
			preview = EntityAI.Cast(GetGame().CreateObjectEx(className, vector.Zero, ECE_LOCAL|ECE_NOLIFETIME));
#ifdef EXPANSIONMODHARDLINE
			ItemBase item;
			if (GetExpansionSettings().GetHardline().EnableItemRarity && Class.CastTo(item, preview))
				item.Expansion_SetRarity(GetExpansionSettings().GetHardline().GetItemRarityByType(item.GetType()));
#endif
		}
	}

	bool ShouldShowItem(ExpansionMarketItem currentItem, string displayName, string search = "")
	{
		MarketPrint("CreateMenuCategories - Start");
		
		if (search != "")
		{
			string displayNameLower = displayName;
			displayNameLower.ToLower();

			if (!displayNameLower.Contains(search))
			{
				MarketPrint("ExpansionMarketMenu::ShouldShowItem - Search filter is active. Skip item: " + currentItem.ClassName);
				return false;
			}
		}
		
		if (ShowPrimeMag() || ShowSecondMag() || ShowPrimeAttach() || ShowSecondAttach() || ShowHandMag() || ShowHandAttach() || ShowPrimeBullet() || ShowSecondBullet() || ShowHandBullet() || ShowSellables() || ShowPurchasables())
		{
			foreach (string option_filter : m_FilterOptionStrings)
			{
				option_filter.ToLower();

				if (currentItem.ClassName == option_filter)
				{
					if (ShowSellables() && !HasPlayerItem(currentItem.ClassName))
						return false;
					
					if (!ShouldShowItemEx(currentItem.ClassName))
						return false;

					MarketPrint("ExpansionMarketMenu::ShouldShowItem - A option filter is active. Add item: " + currentItem.ClassName);
					return true;
				}
			}

			//! Skip item if no filter matches
			return false;
		}
		
		MarketPrint("ExpansionMarketMenu::ShouldShowItem - End - No option filter is active. Add item: " + currentItem.ClassName);
		return true;
	}
	
	//! This is here for overrides
	bool ShouldShowItemEx(string className)
	{
		return true;
	}

	void TempInsertItem(string displayName, ExpansionMarketItem currentItem, map<string, ref array<ExpansionMarketItem>> tempItems)
	{
		MarketPrint("TempInsertItem - Start");
		
		//! This deals with the case where different class names have the same display name
		array<ExpansionMarketItem> itemsArray;
		if (!tempItems.Find(displayName, itemsArray))
		{
			itemsArray = new array<ExpansionMarketItem>;
			tempItems.Insert(displayName, itemsArray);
		}
		else
		{
			foreach (ExpansionMarketItem item: itemsArray)
			{
				if (item.ItemID == currentItem.ItemID)
					return;
			}
		}
		itemsArray.Insert(currentItem);
		m_TraderItems.Insert(currentItem);
		
		MarketPrint("TempInsertItem - End");
	}

	void AddMenuCategory(ExpansionMarketMenuCategory marketCategoryElement)
	{
		MarketPrint("AddMenuCategory - Start");
		
		int count = m_MarketMenuController.MarketCategories.Count();

		if (!count || marketCategoryElement.GetCategory().m_Idx > m_MarketMenuController.MarketCategories[count - 1].GetCategory().m_Idx)
		{
			m_MarketMenuController.MarketCategories.Insert(marketCategoryElement);
		}
		else
		{
			for (int i = 0; i < count; i++)
			{
				if (marketCategoryElement.GetCategory().m_Idx < m_MarketMenuController.MarketCategories[i].GetCategory().m_Idx)
					break;
			}

			m_MarketMenuController.MarketCategories.InsertAt(marketCategoryElement, i);
		}
		
		MarketPrint("AddMenuCategory - End");
	}

	void UpdateMarketCategories(bool updateItemViews = false)
	{
		MarketPrint("UpdateMarketCategories - Start");

		string search = GetSearchText();
		search.ToLower();
		
		bool isFiltered = IsFiltered();

		//! This deals with the case where filters are toggled or search text is entered while the initial drawing of menu items hasn't yet completed,
		//! and sets whether items should be shown or not
		foreach (ExpansionMarketItem item : m_TraderItems)
		{
			//! https://feedback.bistudio.com/T173348
			if (!isFiltered && !item.m_IsVariant)
				item.m_ShowInMenu = true;
			else
				item.m_ShowInMenu = false;
			if (!item.m_ShowInMenu && (!item.m_IsVariant || (!item.m_Parent.m_ShowInMenu && ShowPurchasables()) || ShowSellables() || search))
			{
				string displayName = GetDisplayName(GetPreviewClassName(item.ClassName, true));
				item.m_ShowInMenu = ShouldShowItem(item, displayName, search);
			}
		}
		
		bool includeAttachments;
		if (m_SelectedMarketItemElement && m_SelectedMarketItemElement.GetIncludeAttachments() && m_SelectedMarketItem.SpawnAttachments.Count() > 0)
			includeAttachments = true;

		for (int i = 0; i < m_MarketMenuController.MarketCategories.Count(); i++)
		{
			int filteredItemCount = 0;
			ExpansionMarketMenuCategory menuCategory = m_MarketMenuController.MarketCategories[i];
			ObservableCollection<ref ExpansionMarketMenuItem> menuItems = menuCategory.GetItems();
			
			MarketPrint("-----------------------------------------------------------------------------------------------------------------------------------------------------------------------");
			MarketPrint("UpdateMarketCategories - Update category: " + menuCategory.GetCategory().m_FileName + " | " + menuItems.Count());
			
			if (isFiltered)
			{
				array<ExpansionMarketItem> marketItems = menuCategory.GetMarketItems();
				foreach (ExpansionMarketItem tempItem: marketItems)
				{
					if (!tempItem.m_ShowInMenu)
						filteredItemCount++;
				}
			}

			int itemCount = menuItems.Count();
			for (int j = 0; j < itemCount; j++)
			{
				ExpansionMarketMenuItem menuItem =  menuItems[j];
				ExpansionMarketItem currentBaseItem = menuItem.GetBaseItem();
				ExpansionMarketItem currentItem = menuItem.GetMarketItem();

				if (updateItemViews)
				{
					if (!currentItem.m_UpdateView && m_SelectedMarketItem)
					{
						if (includeAttachments)
						{
							//! The selected item was bought/sold with attachments,
							//! update all attachments, all items that have one of the same attachments,
							//! and items that have the selected item as attachment
							foreach (string selectedAttachment: m_SelectedMarketItem.SpawnAttachments)
							{
								if (currentItem.ClassName == selectedAttachment)
								{
									currentItem.m_UpdateView = true;
									break;
								}

								foreach (string attachment: currentItem.SpawnAttachments)
								{
									if (attachment == selectedAttachment || attachment == m_SelectedMarketItem.ClassName)
									{
										currentItem.m_UpdateView = true;
										break;
									}
								}

								if (currentItem.m_UpdateView)
									break;
							}
						}
						else
						{
							//! Update items that have the selected item as attachment
							foreach (string currentAttachment: currentItem.SpawnAttachments)
							{
								if (currentAttachment == m_SelectedMarketItem.ClassName)
								{
									currentItem.m_UpdateView = true;
									break;
								}
							}
						}
					}

					if (currentItem.m_UpdateView)
						menuItem.UpdateView();
				}
		
				if (!currentBaseItem.m_ShowInMenu)
				{
					menuItem.Hide();
					MarketPrint("UpdateMarketCategories - Hide item: " + currentBaseItem.ClassName);
				}
				else
				{
					menuItem.Show();
					MarketPrint("UpdateMarketCategories - Show item: " + currentBaseItem.ClassName);
				}
			}
			
			if (isFiltered && filteredItemCount == menuCategory.GetMarketItems().Count())
			{
				menuCategory.Hide();
				MarketPrint("UpdateMarketCategories - Hide category: " + menuCategory.GetCategoryController().CategoryName);
			}
			else
			{
				menuCategory.Show();
				MarketPrint("UpdateMarketCategories - Show category: " + menuCategory.GetCategoryController().CategoryName);
				if (!menuCategory.IsUpdateTimerRunning() || !menuCategory.GetUpdateItemCount())
				{
					int show;
					if (isFiltered)
						show = menuCategory.GetMarketItems().Count() - filteredItemCount;
					else
						show = menuCategory.GetShowItemsCount();
					menuCategory.UpdateItemCount(show);
				}
			}
		}
		
		MarketPrint("UpdateMarketCategories - End");
	}

	void UpdateOptionFilterStrings()
	{
		MarketPrint("UpdateOptionFilterStrings - Start");

		if (m_FilterOptionStrings.Count() > 0)
			m_FilterOptionStrings.Clear();
		
		if (ShowPrimeMag())
		{
			TStringArray primaryMags = m_MarketFilters.GetPrimaryWeaponMagazines();
			foreach (string primaryMag : primaryMags)
			{
				if (!IsInFilter(primaryMag))
					m_FilterOptionStrings.Insert(primaryMag);
			}
		}
		
		if (ShowSecondMag())
		{
			TStringArray secondaryMags = m_MarketFilters.GetSecondaryWeaponMagazines();
			foreach (string secondaryMag : secondaryMags)
			{
				if (!IsInFilter(secondaryMag))
					m_FilterOptionStrings.Insert(secondaryMag);
			}
		}
		
		if (ShowHandMag())
		{
			TStringArray handMags = m_MarketFilters.GetHandWeaponMagazines();
			foreach (string handMag : handMags)
			{
				if (!IsInFilter(handMag))
					m_FilterOptionStrings.Insert(handMag);
			}
		}
		
		if (ShowPrimeAttach())
		{
			TStringArray primaryAttachments = m_MarketFilters.GetPrimaryWeaponAttachments();
			foreach (string primaryAttachment : primaryAttachments)
			{
				if (!IsInFilter(primaryAttachment))
					m_FilterOptionStrings.Insert(primaryAttachment);
			}
		}
		
		if (ShowSecondAttach())
		{
			TStringArray secondaryAttachments = m_MarketFilters.GetSecondaryWeaponAttachments();
			foreach (string secondaryAttachment : secondaryAttachments)
			{
				if (!IsInFilter(secondaryAttachment))
					m_FilterOptionStrings.Insert(secondaryAttachment);
			}
		}
		
		if (ShowHandAttach())
		{
			TStringArray handAttachments = m_MarketFilters.GetHandWeaponAttachments();
			foreach (string handAttachment : handAttachments)
			{
				if (!IsInFilter(handAttachment))
					m_FilterOptionStrings.Insert(handAttachment);
			}
		}
		
		if (ShowPrimeBullet())
		{
			TStringArray primaryBullets = m_MarketFilters.GetPrimaryWeaponBullets();
			foreach (string primaryBullet : primaryBullets)
			{
				if (!IsInFilter(primaryBullet))
					m_FilterOptionStrings.Insert(primaryBullet);
			}
		}
		
		if (ShowSecondBullet())
		{
			TStringArray secondaryBullets = m_MarketFilters.GetSecondaryWeaponBullets();
			foreach (string secondaryBullet : secondaryBullets)
			{
				if (!IsInFilter(secondaryBullet))
					m_FilterOptionStrings.Insert(secondaryBullet);
			}
		}
		
		if (ShowHandBullet())
		{
			TStringArray handBullets = m_MarketFilters.GetHandWeaponBullets();
			foreach (string handBullet : handBullets)
			{
				if (!IsInFilter(handBullet))
					m_FilterOptionStrings.Insert(handBullet);
			}
		}
		
		int i;

		if (m_FilterOptionStrings.Count() > 0 && (ShowSellables() || ShowPurchasables()))
		{
			//! Apply sellables/purchasables filter to selected filter strings
			for (i = m_FilterOptionStrings.Count() - 1; i >= 0; i--)
			{
				string item = m_FilterOptionStrings[i];
				if ((ShowSellables() && !GetMarketTrader().CanSellItem(item)) || (ShowPurchasables() && !GetMarketTrader().CanBuyItem(item)))
					m_FilterOptionStrings.Remove(i);
			}
		}
		else
		{
			if (ShowSellables())
			{
				TStringArray sellables = m_MarketFilters.GetSellables();
				foreach (string sellable : sellables)
				{
					//! If purchasables filter is also active, show only items that can be sold AND bought, not either-or
					if ((!ShowPurchasables() || GetMarketTrader().CanBuyItem(sellable)) && !IsInFilter(sellable))
						m_FilterOptionStrings.Insert(sellable);
				}
			}
			else if (ShowPurchasables())
			{
				TStringArray purchasables = m_MarketFilters.GetPurchasables();
				foreach (string purchasable : purchasables)
				{
					if (!IsInFilter(purchasable))
						m_FilterOptionStrings.Insert(purchasable);
				}
			}
		}
		
		MarketPrint("UpdateOptionFilterStrings - End");
	}

	bool IsFiltered()
	{
		//! NOTE: ShowSellables/ShowPurchasables needs to be checked separately because it can result in zero option filter strings
		return m_FilterOptionStrings.Count() > 0 || GetSearchText() != "" || ShowSellables() || ShowPurchasables();
	}

	bool IsInFilter(string className)
	{
		int index = m_FilterOptionStrings.Find(className);
		if (index > -1)
			return true;
		
		return false;
	}

	void RemoveFromFilter(string className)
	{
		MarketPrint("RemoveFromFilter - Start");
		
		int index = m_FilterOptionStrings.Find(className);
		if (index > -1)
			m_FilterOptionStrings.Remove(index);
		
		MarketPrint("RemoveFromFilter - End");
	}

	string GetSearchText()
	{
		return market_filter_box.GetText();
	}

	void ClearCategories()
	{
		MarketPrint("ClearCategories - Start");
		
		for (int i = 0; i < m_MarketMenuController.MarketCategories.Count(); i++)
		{
			ExpansionMarketMenuCategory categoryElement = m_MarketMenuController.MarketCategories[i];
			categoryElement.Destroy();
		}
		
		m_MarketMenuController.MarketCategories.Clear();
		CategoriesExpanded = 0;
		ExpansionMarketMenuCategory.UpdateCategoryID = -1;
		
		MarketPrint("ClearCategories - End");
	}

	void ClearCategoryItems()
	{
		MarketPrint("ClearCategoryItems - Start");
		
		for (int i = 0; i < m_MarketMenuController.MarketCategories.Count(); i++)
		{
			ExpansionMarketMenuCategory categoryElement = m_MarketMenuController.MarketCategories[i];
			categoryElement.ClearCategoryItems();
		}
		
		MarketPrint("ClearCategoryItems - End");
	}

	//! Gets called from market module SI_SetTraderInvoker invoker and sets the current intracted market trader and its market class
	void SetTraderObject( ExpansionTraderObjectBase trader, bool complete)
	{
		MarketPrint("SetTraderObject - Start");
		
		m_TraderObject = trader;
		if (!m_TraderObject)
		{
			Error("ExpansionMarketMenu::SetTraderObject - Could not get trader!");
			Hide();
			return;
		}
		
		m_TraderMarket = trader.GetTraderMarket();
		if (!m_TraderMarket)
		{
			Error("ExpansionMarketMenu::SetTraderObject - Could not get trader market!");
			Hide();
			return;
		}
		
		m_KeyInput = false;

		if (m_CurrentState != ExpansionMarketMenuState.REQUESTING_SELECTED_ITEM)
		{
			if (!m_FirstCall || (m_Complete && complete))
			{
				m_MarketModule.EnumeratePlayerInventory(PlayerBase.Cast(GetGame().GetPlayer()));

				UpdatePlayerCurrency();
				UpdatePlayerItems();
			}

			UpdateFilterView();
		}

		if (!m_FirstCall)
		{
			EXPrint("SetTraderObject() - !m_FirstCall");
			InitMarketCategories();
			CreateMenuCategories();
	
			m_MarketMenuController.MarketName = m_TraderMarket.DisplayName;
			m_MarketMenuController.NotifyPropertyChanged("MarketName");
			
			if (m_TraderMarket.TraderIcon != "")
			{
				m_MarketMenuController.MarketIcon = ExpansionIcons.GetPath(m_TraderMarket.TraderIcon);
				if (m_MarketMenuController.MarketIcon == "")
					m_MarketMenuController.MarketIcon = m_TraderMarket.TraderIcon;
				
				m_MarketMenuController.NotifyPropertyChanged("MarketIcon");
			}
			
			if (GetExpansionSettings().GetMarket().CurrencyIcon != "")
			{
				m_MarketMenuController.CurrencySellIcon = GetExpansionSettings().GetMarket().CurrencyIcon;
				m_MarketMenuController.NotifyPropertyChanged("CurrencySellIcon");
				m_MarketMenuController.CurrencyBuyIcon = GetExpansionSettings().GetMarket().CurrencyIcon;
				m_MarketMenuController.NotifyPropertyChanged("CurrencyBuyIcon");
			}
			
			m_FirstCall = true;
		}
		else if (!m_Complete)
		{
			EXPrint("SetTraderObject() - !m_Complete");
			//! Create remaining categories as items arrive
			CreateMenuCategories();
		}
		else
		{
			if (m_CurrentState != ExpansionMarketMenuState.REQUESTING_SELECTED_ITEM)
			{
				EXPrint("SetTraderObject() - m_Complete -> UpdateMarketCategories(true)");
				UpdateMarketCategories(true);
			}
			else
			{
				EXPrint("SetTraderObject() - m_Complete -> GetSelectedMarketItemElement().UpdateView()");
				GetSelectedMarketItemElement().UpdateView();
			}
			
			UpdateItemFieldFromBasicNetSync();
		}

		if (!m_Complete && complete)
			m_Complete = complete;

		SetIsLoading(false);
		
		MarketPrint("SetTraderObject - End");
	}

	void UpdateFilterView()
	{
		MarketPrint("UpdateFilterView - Start");
		
		if (m_FilterOptionStrings.Count() > 0)
		{
			//! Filter(s) active, need to update complete view
			market_menu_info.Show(false);

			//! Store state of selected filters
			int i;
			ExpansionMarketMenuDropdownElement dropdownElement;
			map<string, bool> dropdownCheckedStates = new map<string, bool>;
			for (i = 0; i < m_MarketMenuController.DropdownElements.Count(); i++)
			{
				dropdownElement = m_MarketMenuController.DropdownElements[i];
				dropdownCheckedStates.Insert(dropdownElement.ClassName(), dropdownElement.IsChecked());
			}

			UpdateDropdownFilterElements();

			//! Restore state of selected filters
			for (i = 0; i < m_MarketMenuController.DropdownElements.Count(); i++)
			{
				dropdownElement = m_MarketMenuController.DropdownElements[i];
				bool storedState;
				if (dropdownCheckedStates.Find(dropdownElement.ClassName(), storedState))
				{
					dropdownElement.SetChecked(storedState, false);
				}
			}
		}
		else
		{
			UpdateDropdownFilterElements();
		}

		UpdateOptionFilterStrings();
		
		MarketPrint("UpdateFilterView - End");
	}

	void ClearDropdownElements()
	{
		MarketPrint("ClearDropdownElements - Start");
		
		for (int i = 0; i < m_MarketMenuController.DropdownElements.Count(); i++)
		{
			ExpansionMarketMenuDropdownElement element = m_MarketMenuController.DropdownElements[i];
			element.Destroy();
		}
		
		m_MarketMenuController.DropdownElements.Clear();
		
		MarketPrint("ClearDropdownElements - End");
	}

	void UpdateDropdownFilterElements()
	{
		MarketPrint("UpdateDropdownFilterElements - Start");

		EntityAI shoulder_left = GetGame().GetPlayer().FindAttachmentBySlotName("Shoulder");
		EntityAI shoulder_right = GetGame().GetPlayer().FindAttachmentBySlotName("Melee");
		EntityAI hands = GetGame().GetPlayer().GetHumanInventory().GetEntityInHands();
		
		ClearDropdownElements();
		
		if (shoulder_left && shoulder_left.IsWeapon())
		{
			m_MarketFilters.GenerateWeaponAttachmentDenom(shoulder_left.GetType());

			if (m_MarketFilters.GetWeaponMagazines(shoulder_left).Count() > 0)
			{
				StringLocaliser primary_mag = new StringLocaliser(shoulder_left.GetDisplayName() + " #STR_EXPANSION_MARKET_OPTION_FILTER_MAGAZINES");
				ExpansionMarketMenuDropdownElement_FilterPrimeMag primemag_filter = new ExpansionMarketMenuDropdownElement_FilterPrimeMag(this, primary_mag);
				m_MarketMenuController.DropdownElements.Insert(primemag_filter);
			}

			if (m_MarketFilters.GetWeaponAttachments(shoulder_left).Count() > 0)
			{
				StringLocaliser primary_attach = new StringLocaliser(shoulder_left.GetDisplayName() + " #STR_EXPANSION_MARKET_OPTION_FILTER_ATTACHMENTS");
				ExpansionMarketMenuDropdownElement_FilterPrimeAttach primeattach_filter = new ExpansionMarketMenuDropdownElement_FilterPrimeAttach(this, primary_attach);
				m_MarketMenuController.DropdownElements.Insert(primeattach_filter);
			}

			if (m_MarketFilters.GetWeaponBullets(shoulder_left).Count() > 0)
			{
				StringLocaliser primary_bullets = new StringLocaliser(shoulder_left.GetDisplayName() + " #STR_EXPANSION_MARKET_OPTION_FILTER_BULLETS");
				ExpansionMarketMenuDropdownElement_FilterPrimeBullet primebullet_filter = new ExpansionMarketMenuDropdownElement_FilterPrimeBullet(this, primary_bullets);
				m_MarketMenuController.DropdownElements.Insert(primebullet_filter);
			}
		}

		if (shoulder_right && shoulder_right.IsWeapon())
		{
			m_MarketFilters.GenerateWeaponAttachmentDenom(shoulder_right.GetType());

			if (m_MarketFilters.GetWeaponMagazines(shoulder_right).Count() > 0)
			{
				StringLocaliser second_mag = new StringLocaliser(shoulder_right.GetDisplayName() + " #STR_EXPANSION_MARKET_OPTION_FILTER_MAGAZINES");
				ExpansionMarketMenuDropdownElement_FilterSecondMag secondmag_filter = new ExpansionMarketMenuDropdownElement_FilterSecondMag(this, second_mag);
				m_MarketMenuController.DropdownElements.Insert(secondmag_filter);
			}

			if (m_MarketFilters.GetWeaponAttachments(shoulder_right).Count() > 0)
			{
				StringLocaliser second_attach = new StringLocaliser(shoulder_right.GetDisplayName() + " #STR_EXPANSION_MARKET_OPTION_FILTER_ATTACHMENTS");
				ExpansionMarketMenuDropdownElement_FilterSecondAttach secondattach_filter = new ExpansionMarketMenuDropdownElement_FilterSecondAttach(this, second_attach);
				m_MarketMenuController.DropdownElements.Insert(secondattach_filter);
			}

			if (m_MarketFilters.GetWeaponBullets(shoulder_right).Count() > 0)
			{
				StringLocaliser second_bullets = new StringLocaliser(shoulder_right.GetDisplayName() + " #STR_EXPANSION_MARKET_OPTION_FILTER_BULLETS");
				ExpansionMarketMenuDropdownElement_FilterSecondBullet secondbullet_filter = new ExpansionMarketMenuDropdownElement_FilterSecondBullet(this, second_bullets);
				m_MarketMenuController.DropdownElements.Insert(secondbullet_filter);
			}
		}

		if (hands && hands.IsWeapon())
		{
			m_MarketFilters.GenerateWeaponAttachmentDenom(hands.GetType());

			if (m_MarketFilters.GetWeaponMagazines(hands).Count() > 0)
			{
				StringLocaliser hand_mag = new StringLocaliser(hands.GetDisplayName() + " #STR_EXPANSION_MARKET_OPTION_FILTER_MAGAZINES");		
				ExpansionMarketMenuDropdownElement_FilterHandMag handmag_filter = new ExpansionMarketMenuDropdownElement_FilterHandMag(this, hand_mag);
				m_MarketMenuController.DropdownElements.Insert(handmag_filter);
			}

			if (m_MarketFilters.GetWeaponAttachments(hands).Count() > 0)
			{
				StringLocaliser hand_attach = new StringLocaliser(hands.GetDisplayName() + " #STR_EXPANSION_MARKET_OPTION_FILTER_ATTACHMENTS");
				ExpansionMarketMenuDropdownElement_FilterHandAttach handattach_filter = new ExpansionMarketMenuDropdownElement_FilterHandAttach(this, hand_attach);
				m_MarketMenuController.DropdownElements.Insert(handattach_filter);
			}

			if (m_MarketFilters.GetWeaponBullets(hands).Count() > 0)
			{
				StringLocaliser hand_bullets = new StringLocaliser(hands.GetDisplayName() + " #STR_EXPANSION_MARKET_OPTION_FILTER_BULLETS");
				ExpansionMarketMenuDropdownElement_FilterHandBullet handbullet_filter = new ExpansionMarketMenuDropdownElement_FilterHandBullet(this, hand_bullets);
				m_MarketMenuController.DropdownElements.Insert(handbullet_filter);
			}
		}
		
		if (m_MarketMenuController.DropdownElements.Count() == 0)
		{
			dropdown_selector.Show(false);
		}
		else if (m_MarketMenuController.DropdownElements.Count() > 0)
		{
			dropdown_selector.Show(true);
		}

		MarketPrint("UpdateDropdownFilterElements - End");
	}

	void UpdatePlayerCurrency()
	{
		MarketPrint("UpdatePlayerCurrency - Start");
		
		array<int> monies = new array<int>;
		
		int worth = m_MarketModule.GetPlayerWorth(PlayerBase.Cast(GetGame().GetPlayer()), monies, m_TraderMarket);
		
		TStringArray descriptions = new TStringArray;
		TStringArray excludedCurrencys = new TStringArray;
		for (int i = 0; i < monies.Count(); i++)
		{
			string type = m_MarketModule.GetMoneyDenomination(i);
			int total_monies = monies[i] * m_MarketModule.GetMoneyPrice(type);
			if (m_TraderMarket.Currencies.Find(type) == -1)
			{
				excludedCurrencys.Insert(GetDisplayName(type));
			}
			else
			{
				string currencyString = GetDisplayPrice(total_monies, false, true, true);
				descriptions.Insert(ExpansionStatic.FormatInt(monies[i]) + " × " + GetDisplayName(type) + " (" + currencyString + ")");
			}
		}
		
		string currencyPlayerTotalMoneyString = GetDisplayPrice(worth, false, true, true);
	#ifdef DIAG_DEVELOPER
		EXPrint(ToString() + " Player money: " + currencyPlayerTotalMoneyString);
	#endif
		m_MarketMenuController.PlayerTotalMoney = currencyPlayerTotalMoneyString + " #STR_EXPANSION_MARKET_CURRENCY_TOTAL";
		m_MarketMenuController.NotifyPropertyChanged("PlayerTotalMoney");
		
		if (!m_CurrenciesTooltip)
		{
			m_CurrenciesTooltip = new ExpansionMarketMenuTooltip();
			m_CurrenciesTooltip.SetTitle("#STR_EXPANSION_MARKET_CURRENCY_DENOMS");
			m_CurrenciesTooltip.SetContentOffset(-0.234375, 0.006944);
		}
		else
		{
			m_CurrenciesTooltip.ClearEntries();
		}

		foreach (string desc: descriptions)
		{
			m_CurrenciesTooltip.AddEntry(desc);
		}

		if (excludedCurrencys.Count())
		{
			//! Show currencies this trader does not accept
			m_CurrenciesTooltip.AddEntry("");
			m_CurrenciesTooltip.AddEntry("This trader does not accept:");  //! TODO: localize this
			foreach (string exclDesc: excludedCurrencys)
			{
				m_CurrenciesTooltip.AddEntry(exclDesc);
			}
		}
		
		m_CurrenciesTooltip.Hide();
		
		descriptions.Clear();
		excludedCurrencys.Clear();
		
		MarketPrint("UpdatePlayerCurrency - End");
	}

	void SetItemInfo( ExpansionMarketMenuItem itemElement, bool resetQuantity = true)
	{
		MarketPrint("SetItemInfo - Start");
		
		m_CurrentPreviewObject = NULL;
		
		m_SelectedMarketItemElement = itemElement;
		m_SelectedMarketItem = itemElement.GetMarketItem();
		
		market_menu_info.Show(true);

		string itemClassName = GetPreviewClassName(GetSelectedMarketItem().ClassName, true);
		
		m_MarketMenuController.MarketItemName = GetDisplayName(itemClassName);
		m_MarketMenuController.NotifyPropertyChanged("MarketItemName");
		
		string itemDesc = ExpansionStatic.GetItemDescriptionWithType(itemClassName);
		if (itemDesc.IndexOf("STR_") == 0)
			itemDesc = "";

		m_MarketMenuController.MarketItemDesc = itemDesc;
		m_MarketMenuController.NotifyPropertyChanged("MarketItemDesc");
		
		UpdatePreview();
		
		if (resetQuantity)
			m_Quantity = 1;
		
		UpdateItemFieldFromBasicNetSync();
		SetItemAttachmentsInfo(itemElement);
		PlayMarketSound();
		UpdateItemDescriptionPanel();
		UpdateItemSkins();
		
		bool hasAttachments = false;
		if (itemElement.GetPreviewObject().IsWeapon())
		{
			m_MarketFilters.GenerateWeaponAttachmentDenom(itemElement.GetPreviewObject().GetType());
			hasAttachments = m_MarketFilters.HasWeaponAttachments(itemElement.GetPreviewObject().GetType());
		}
		else if (itemElement.GetPreviewObject().IsClothing())
		{
			if (ExpansionMarketFilters.IsCustomizableClothing(itemElement.GetPreviewObject().GetType()))
			{
				m_MarketFilters.GenerateClothingAttachmentDenom(itemElement.GetPreviewObject().GetType());
				hasAttachments = m_MarketFilters.HasClothingAttachments(itemElement.GetPreviewObject().GetType());
			}
		}
		
		bool hasSpawnAttachments = itemElement.GetMarketItem().SpawnAttachments.Count() > 0;
		bool showAttachmentsInfo;
		if (hasSpawnAttachments && !hasAttachments)
			showAttachmentsInfo = true;
		
		bool showInteractiveSections = true;
		bool canOnlyBuy = itemElement.CanBuy() && !itemElement.CanSell();
		bool canOnlySell = !itemElement.CanBuy() && itemElement.CanSell();
		if (canOnlyBuy)
			showInteractiveSections = itemElement.HasRepBuy();
		else if (canOnlySell)
			showInteractiveSections = itemElement.HasRepSell();
		else if ((!itemElement.CanSell() && !itemElement.CanBuy()) || (!itemElement.HasRepBuy() && !itemElement.HasRepSell()))
			showInteractiveSections = false;
		
		market_item_info_item_manager_panel.Show(hasAttachments && showInteractiveSections);
		market_item_info_attachments_panel.Show(showAttachmentsInfo && showInteractiveSections);
		market_item_info_attachments_checkbox.SetChecked(GetSelectedMarketItemElement().GetIncludeAttachments());
		market_item_amount_panel.Show(showInteractiveSections);
		market_item_price_panel.Show(showInteractiveSections);
		
		MarketPrint("SetItemInfo - End");
	}

	void UpdatePreview()
	{
		MarketPrint("UpdatePreview - Start");
		
		string previewClassName = GetPreviewClassName(GetSelectedMarketItem().ClassName);
		
		if (!UsePlayerPreview())
		{
			market_item_preview.Show(true);
			market_player_preview.Show(false);
			m_MarketMenuController.MarketItemPreview = GetSelectedMarketItemElement().GetPreviewObject();
			m_MarketMenuController.NotifyPropertyChanged("MarketItemPreview");
			
			m_CurrentPreviewObject = GetSelectedMarketItemElement().GetPreviewObject();
		}
		else
		{
			market_item_preview.Show(false);
			market_player_preview.Show(true);

			m_PlayerPreview.Update(previewClassName);
		}
		
		MarketPrint("UpdatePreview - End");
	}

	override void UpdatePlayerPreviewObject(Object previewObject)
	{
		m_MarketMenuController.MarketPlayerPreview = previewObject;
		m_MarketMenuController.NotifyPropertyChanged("MarketPlayerPreview");
	}

	override void SpawnPlayerPreviewAttachments(EntityAI item)
	{
		if (GetSelectedMarketItem().SpawnAttachments.Count() && GetSelectedMarketItemElement().GetIncludeAttachments())
			GetSelectedMarketItemElement().SpawnAttachments(GetSelectedMarketItem(), item);
	}

	override void SetCurrentPreviewObject(Object obj)
	{
		m_CurrentPreviewObject = obj;
	}

	void UpdateItemDescriptionPanel()
	{
		MarketPrint("UpdateItemDescriptionPanel - Start");
		
		string text = m_MarketMenuController.MarketItemDesc;
		int length = text.Length();
		if (length > 220)
		{
			market_item_description_scroll_indicator_header.Show(false);
			market_item_description_scroll_indicator_footer.Show(true);
		}
		else
		{
			market_item_description_scroll_indicator_footer.Show(false);
		}
		
		MarketPrint("UpdateItemDescriptionPanel - End");
	}

	void ClearSkins()
	{
		m_MarketMenuController.SkinsDropdownElements.Clear();
	}

	void UpdateItemSkins()
	{
		MarketPrint("UpdateItemSkins - Start");
		
		ClearSkins();

		if (!GetSelectedMarketItemElement())
			return;
		
		ExpansionMarketItem baseItem = GetSelectedMarketItemElement().GetBaseItem();

		map<string, ref array<ref ExpansionMarketMenuSkinsDropdownElement>> dropdownElements = new map<string, ref array<ref ExpansionMarketMenuSkinsDropdownElement>>;
		ExpansionMarketMenuSkinsDropdownElement dropdownElement;
		array<ref ExpansionMarketMenuSkinsDropdownElement> currentElements;

		ExpansionSkinModule skinModule = ExpansionSkinModule.Cast(CF_ModuleCoreManager.Get(ExpansionSkinModule));
		if (skinModule)
		{
			array<ExpansionSkin> skins = new array<ExpansionSkin>;
			string defaultSkin;
			int currentSkinIndex;
			string currentSkinName;
			
			skinModule.RetrieveSkins(baseItem.ClassName, skins, defaultSkin);
			
			for (int i = 0; i < skins.Count(); i++)
			{
				currentSkinIndex = i;
				currentSkinName = skinModule.GetSkinName(baseItem.ClassName, currentSkinIndex);

				//! If skin is added as variant, skip it
				currentSkinName.ToLower();
				if (baseItem.Variants.Find(baseItem.ClassName + "_" + currentSkinName) > -1)
					continue;

				currentSkinName.ToUpper();
				dropdownElement = new ExpansionMarketMenuSkinsDropdownElement(this, i, currentSkinName);
				if (!dropdownElements.Find(currentSkinName, currentElements))
				{
					currentElements = new array<ref ExpansionMarketMenuSkinsDropdownElement>;
					dropdownElements.Insert(currentSkinName, currentElements);
				}
				currentElements.Insert(dropdownElement);
			}
		}

		TStringArray variants = new TStringArray;

		//! If we have skins, the base class is already included, otherwise we have to add it
		if (dropdownElements.Count() == 0)
			variants.Insert(baseItem.ClassName);

		variants.InsertAll(baseItem.Variants);

		string baseDisplayName = ExpansionStatic.GetItemDisplayNameWithType(baseItem.ClassName, m_TypeDisplayNames);

		map<string, string> displayNames = new map<string, string>;
		string displayName;
		map<string, ref TStringArray> displayNameClasses = new map<string, ref TStringArray>;
		TStringArray classNames;

		foreach (string className : variants)
		{
			displayName = "";

			//! Check if this is a Expansion skin type - e.g. <ClassName>_<SkinName> - and use skin name in that case
			if (skinModule)
				displayName = skinModule.GetSkinName( className );

			//! Fall back to display name
			if (!displayName)
				displayName = ExpansionStatic.GetItemDisplayNameWithType(className, m_TypeDisplayNames);

			displayNames.Insert(className, displayName);
			if (!displayNameClasses.Find(displayName, classNames))
			{
				classNames = new TStringArray;
				displayNameClasses.Insert(displayName, classNames);
			}
			classNames.Insert(className);
		}

		map<string, int> seenDisplayNames = new map<string, int>;
		int count;

		foreach (string variant : variants)
		{
			displayName = displayNames.Get(variant);

			//! Check if it's the same display name as base item and if so, try to derive a suitable diff string from the classname
			classNames = displayNameClasses.Get(displayName);
			if (classNames.Count() > 1)
			{
				string diff = "";
				//! Split base item classname and variant classname using "_" delimiter
				//! Use part with lowest amount of occurences
				//! E.g. given these two classnames (base, variant):
				//! TrackSuitJacket_Black, TrackSuitJacket_Blue -> TrackSuitJacket: 2, Black: 1, Blue: 1 -> Blue will get used
				TStringArray tempPartsVariant = new TStringArray;
				variant.Split("_", tempPartsVariant);
				if (tempPartsVariant.Count() > 1)
				{
					TStringArray tempParts = new TStringArray;
					classNames[0].Split("_", tempParts);
					tempParts.InsertAll(tempPartsVariant);
					TStringArray partNames = new TStringArray;
					map<string, int> parts = new map<string, int>;
					foreach (string tempPart : tempParts)
					{
						if (classNames[0].IndexOf(tempPart) == 0 || variant.IndexOf(tempPart) == 0)
							continue;
						if (!parts.Contains(tempPart))
						{
							partNames.Insert(tempPart);
							parts.Insert(tempPart, 1);
						}
						else
						{
							parts.Set(tempPart, parts.Get(tempPart) + 1);
						}
					}
					int lowestCount = -1;
					foreach (string part : partNames)
					{
						count = parts.Get(part);
						if (lowestCount == -1 || count <= lowestCount)
						{
							lowestCount = count;
							diff = part;
						}
					}
					displayName = diff;
				}
				else if (variant == baseItem.ClassName)
				{
					displayName = "Default";
				}
				else
				{
					//! Fallback
					displayName = variant;
				}
			}

			if (seenDisplayNames.Contains(displayName))
			{
				count = seenDisplayNames.Get(displayName) + 1;
				seenDisplayNames.Set(displayName, count);
				//! Add sequential number
				displayName += " (" + count + ")";
			}
			else
			{
				seenDisplayNames.Insert(displayName, 1);
			}

			displayName.ToUpper();
			dropdownElement = new ExpansionMarketMenuSkinsDropdownElement(this, -1, displayName, variant);
			if (!dropdownElements.Find(displayName, currentElements))
			{
				currentElements = new array<ref ExpansionMarketMenuSkinsDropdownElement>;
				dropdownElements.Insert(displayName, currentElements);
			}
			currentElements.Insert(dropdownElement);
		}

		TStringArray names = dropdownElements.GetKeyArray();
		names.Sort();

		foreach (string name : names)
		{
			currentElements = dropdownElements.Get(name);
			foreach (ExpansionMarketMenuSkinsDropdownElement currentElement : currentElements)
			{
				m_MarketMenuController.SkinsDropdownElements.Insert(currentElement);
			}
		}

		skin_selector.Show(dropdownElements.Count() > 1);
		
		MarketPrint("UpdateItemSkins - End");
	}

	void SetItemAttachmentsInfo( ExpansionMarketMenuItem itemElement)
	{
		MarketPrint("SetItemAttachmentsInfo - Start");
		
		if (m_AttachmentsTooltip)
		{
			if (market_item_info_attachments.IsVisible()) 
				market_item_info_attachments.Show(false);
			
			m_AttachmentsTooltip.Destroy();
		}
		
		if (itemElement)
		{
			if (itemElement.GetMarketItem().SpawnAttachments.Count() > 0 && itemElement.GetIncludeAttachments())
			{
				m_AttachmentsTooltip = new ExpansionMarketMenuTooltip();
				m_AttachmentsTooltip.Hide();
				m_AttachmentsTooltip.SetContentOffset(-0.234375, 0.006944);
				m_AttachmentsTooltip.SetTitle(GetSelectedMarketItemElement().GetPreviewObject().GetDisplayName() + " #STR_EXPANSION_MARKET_TOOLTIP_ATTACHMENTS_TITLE");
				m_AttachmentsTooltip.SetText("#STR_EXPANSION_MARKET_TOOLTIP_ATTACHMENTS_TEXT");
				m_AttachmentsTooltip.ClearEntries();
				foreach (string attachment: GetSelectedMarketItemElement().GetMarketItem().SpawnAttachments)
				{
					m_AttachmentsTooltip.AddEntry(attachment);
				}
				
				market_item_info_attachments.Show(true);
				market_item_info_attachments_icon.SetColor(GetExpansionSettings().GetMarket().MarketMenuColors.Get("ColorItemInfoAttachments"));
			}

			if (!itemElement.GetIncludeAttachments())
			{
				if (market_item_info_attachments.IsVisible()) 
					market_item_info_attachments.Show(false);
			}
		}
		
		MarketPrint("SetItemAttachmentsInfo - End");
	}

	string GetDisplayPrice(int price, bool shorten = false, bool format = true, bool includeDisplayCurrencyName = false)
	{
		return m_TraderObject.GetDisplayPrice(price, shorten, format, includeDisplayCurrencyName);
	}

	void UpdateItemFieldFromBasicNetSync()
	{
		MarketPrint("UpdateItemFieldFromBasicNetSync - Start");
		
		if (!GetSelectedMarketItem())
			return;

		if (GetSelectedMarketItem().IsStaticStock())
		{
			m_TraderItemStock = 1;
			m_TraderHasQuantity = true;
		}
		else
		{
			m_TraderItemStock = m_MarketModule.GetClientZone().GetStock(GetSelectedMarketItem().ClassName);
			m_TraderHasQuantity = m_TraderItemStock >= m_Quantity;
		}
		m_PlayerStock = m_MarketModule.GetAmountInInventory(GetSelectedMarketItem(), m_MarketModule.LocalGetEntityInventory());
		
		MarketPrint("UpdateItemFieldFromBasicNetSync - GetSelectedMarketItem().ClassName: " + GetSelectedMarketItem().ClassName);
		MarketPrint("UpdateItemFieldFromBasicNetSync - m_TraderItemStock: " + m_TraderItemStock);
		
		//! TODO: Give each transaction an unique ID and MarketSell?
		m_MarketSell = new ExpansionMarketSell;
		m_MarketSell.Item = GetSelectedMarketItem();
		m_MarketSell.Trader = m_TraderObject;
		
		int color;

		if (GetSelectedMarketItemElement().CanSell() && GetSelectedMarketItemElement().HasRepSell())
		{
			if (m_PlayerStock >= 0)
			{
				if (m_PlayerStock > 0 && m_Quantity > m_PlayerStock)
				{
					m_MarketMenuController.MarketItemStockPlayer = "#STR_EXPANSION_MARKET_ITEM_NOT_ENOUGH";
				}
				else
				{
					m_MarketMenuController.MarketItemStockPlayer = m_PlayerStock.ToString() + " #STR_EXPANSION_MARKET_ITEM_ONPLAYER";
				}
			}
			else
			{
				m_MarketMenuController.MarketItemStockPlayer = "#STR_EXPANSION_MARKET_ITEM_CANT_SELL";
			}

			array<EntityAI> items;
			if (m_PlayerStock != 0)
			{
				//! Player has the item
				items = m_MarketModule.LocalGetEntityInventory();
				color = GetExpansionSettings().GetMarket().MarketMenuColors.Get("BaseColorText");
			}
			else
			{
				//! Player doesn't have the item
				items = new array<EntityAI>;
				color = COLOR_EXPANSION_NOTIFICATION_EXPANSION;
			}

			//! https://feedback.bistudio.com/T173348
			bool includeAttachments;
			if (m_PlayerStock != 0 || m_SelectedMarketItemElement.GetIncludeAttachments())
				includeAttachments = true;
			m_MarketModule.FindSellPrice(PlayerBase.Cast(GetGame().GetPlayer()), items, m_TraderItemStock, m_Quantity, m_MarketSell, includeAttachments, m_LastFindSellPriceResult, m_LastFindSellPriceFailedClassName);
			m_SellPrice = m_MarketSell.Price;

			market_item_sell_price_text.SetColor(color); 
			market_item_sell_price_icon.SetColor(color);

			m_MarketMenuController.MarketItemTotalSellPrice = GetDisplayPrice(m_SellPrice);
		}
		else
		{
			m_SellPrice = -1;
			if (GetSelectedMarketItemElement().HasRepSell())
				m_MarketMenuController.MarketItemStockPlayer = "#STR_EXPANSION_MARKET_ITEM_CANT_SELL";
			else
				m_MarketMenuController.MarketItemStockPlayer = "NOT ENOUGH REP";
			m_MarketMenuController.MarketItemTotalSellPrice = "";
			
		}
		
		market_item_sell_price_icon.Show(m_SellPrice > -1);
		
		if (GetSelectedMarketItemElement().CanBuy() && GetSelectedMarketItemElement().HasRepBuy())
		{
			if (m_TraderItemStock > 0)
			{
				if (GetSelectedMarketItem().IsStaticStock())
					m_MarketMenuController.MarketItemStockTrader = "#STR_EXPANSION_MARKET_ITEM_INSTOCK";
				else if (m_TraderHasQuantity)
					m_MarketMenuController.MarketItemStockTrader = m_TraderItemStock.ToString() + " #STR_EXPANSION_MARKET_ITEM_INSTOCK";
				else
					m_MarketMenuController.MarketItemStockTrader = "#STR_EXPANSION_MARKET_ITEM_NOT_ENOUGH";
			}
			else
			{
				if (GetSelectedMarketItem().MaxStockThreshold > 0)
					m_MarketMenuController.MarketItemStockTrader = "#STR_EXPANSION_MARKET_ITEM_NOTINSTOCK";
				else
					m_MarketMenuController.MarketItemStockTrader = "N/A";
			}

			int price = 0;
			ExpansionMarketResult result = ExpansionMarketResult.Success;
			m_MarketModule.FindPriceOfPurchase(GetSelectedMarketItem(), m_MarketModule.GetClientZone(), m_TraderMarket, m_Quantity, price, GetSelectedMarketItemElement().GetIncludeAttachments(), result);
			if (result == ExpansionMarketResult.IntegerOverflow)
			{
				m_BuyPrice = -1;
				m_MarketMenuController.MarketItemTotalBuyPrice = "";
				ExpansionNotification("STR_EXPANSION_MARKET_TITLE", "Integer overflow. Reduce the quantity you are trying to buy or sell.", EXPANSION_NOTIFICATION_ICON_ERROR, COLOR_EXPANSION_NOTIFICATION_ERROR, 3, ExpansionNotificationType.MARKET).Create();
			}
			else
			{
				m_BuyPrice = price;
				m_MarketMenuController.MarketItemTotalBuyPrice = GetDisplayPrice(m_BuyPrice);
			}

			if (m_MarketModule.GetPlayerWorth() >= m_BuyPrice)
				color = GetExpansionSettings().GetMarket().MarketMenuColors.Get("BaseColorText");
			else
				color = COLOR_EXPANSION_NOTIFICATION_EXPANSION;

			market_item_buy_price_text.SetColor(color); 
			market_item_buy_price_icon.SetColor(color);
		}
		else
		{
			m_BuyPrice = -1;
			if (GetSelectedMarketItemElement().HasRepBuy())
				m_MarketMenuController.MarketItemStockTrader = "#STR_EXPANSION_MARKET_ITEM_CANT_BUY";
			else
				m_MarketMenuController.MarketItemStockTrader = "NOT ENOUGH REP";
			m_MarketMenuController.MarketItemTotalBuyPrice = "";
		}

		market_item_buy_price_icon.Show(m_BuyPrice > -1);
		m_MarketMenuController.MarketQuantity = m_Quantity.ToString();
		m_MarketMenuController.NotifyPropertiesChanged({"MarketItemStockPlayer", "MarketItemTotalSellPrice", "MarketItemStockTrader", "MarketItemTotalBuyPrice", "MarketQuantity"});

		market_item_buy.Show(GetSelectedMarketItemElement().CanBuy() && GetSelectedMarketItemElement().HasRepBuy() && m_Quantity > 0 && m_TraderHasQuantity && m_BuyPrice > -1 && m_MarketModule.GetPlayerWorth() >= m_BuyPrice);
		market_item_info_stock.SetColor(GetSelectedMarketItemElement().GetMarketStockColor(m_TraderHasQuantity));
		
		market_item_sell.Show(GetSelectedMarketItemElement().CanSell() && GetSelectedMarketItemElement().HasRepSell() && m_Quantity > 0 && m_Quantity <= m_PlayerStock && m_SellPrice > -1);
		market_item_info_stock_player.SetColor(GetSelectedMarketItemElement().GetPlayerStockColor(m_Quantity <= m_PlayerStock));
		UpdateMonieDenominations();

		MarketPrint("UpdateItemFieldFromBasicNetSync - End");
	}

	//! Should update and display the current needed monie denominations for the selected and currently displayed market item
	void UpdateMonieDenominations()
	{
		MarketPrint("UpdateMonieDenominations - Start");
		
		array<string> denoms = m_MarketModule.GetMoneyDenominations();
		array<int> monies = new array<int>;
		int i;
		string prefix;
		
		TStringArray descriptions;
		TStringArray sellDescriptions = new TStringArray;
		TStringArray sellChangeDescriptions = new TStringArray;
		TStringArray buyDescriptions = new TStringArray;
		TStringArray buyChangeDescriptions = new TStringArray;
		
		m_MarketModule.FindMoneyAndCountTypes(NULL, m_SellPrice, monies, false, GetSelectedMarketItem(), m_TraderMarket);
		for (i = 0; i < monies.Count(); i++)
		{
			if (monies[i] > 0)
			{
				//! Player will get during sell transaction
				prefix = "+";
				descriptions = sellDescriptions;
			}
			else if (monies[i] < 0)
			{
				//! Player will give during sell transaction (change)
				prefix = "-";
				descriptions = sellChangeDescriptions;
			}
			else
			{
				prefix = "";
			}
			
			#ifdef EXPANSIONMODMARKET_DEBUG
			MarketPrint("UpdateMonieDenominations - Sell - " + prefix + monies[i].ToString() + " " + denoms[i] + " " + GetDisplayName(denoms[i]));
			#endif
			
			if (monies[i] != 0)
				descriptions.Insert(prefix + Math.AbsInt(monies[i]).ToString() + " × " + GetDisplayName(denoms[i]));
		}
		
		monies.Clear();
		
		if (!m_SellDenomsTooltip)
		{
			m_SellDenomsTooltip = new ExpansionMarketMenuTooltip();
			m_SellDenomsTooltip.Hide();
			m_SellDenomsTooltip.SetTitle("#STR_EXPANSION_MARKET_YOU_GET");
			m_SellDenomsTooltip.SetContentOffset(-0.273438, -0.277778);
		}
		else
		{
			m_SellDenomsTooltip.ClearEntries();
		}

		foreach (string sellDesc: sellDescriptions)
		{
			m_SellDenomsTooltip.AddEntry(sellDesc);
		}
		
		sellDescriptions.Clear();

		if (sellChangeDescriptions.Count())
		{
			m_SellDenomsTooltip.AddEntry("");
			m_SellDenomsTooltip.AddEntry("#STR_EXPANSION_MARKET_YOU_PAY");
			foreach (string sellChangeDesc: sellChangeDescriptions)
			{
				m_SellDenomsTooltip.AddEntry(sellChangeDesc);
			}
		}
		
		sellChangeDescriptions.Clear();
		
		m_MarketModule.FindMoneyAndCountTypes(PlayerBase.Cast(GetGame().GetPlayer()), m_BuyPrice, monies, false, GetSelectedMarketItem(), m_TraderMarket);
		for (i = 0; i < monies.Count(); i++)
		{
			if (monies[i] < 0)
			{
				//! Player will get during buy transaction (change)
				prefix = "+";
				descriptions = buyChangeDescriptions;
			}
			else if (monies[i] > 0)
			{
				//! Player will give during buy transaction
				prefix = "-";
				descriptions = buyDescriptions;
			}
			else
			{
				prefix = "";
			}
			
			#ifdef EXPANSIONMODMARKET_DEBUG
			MarketPrint("UpdateMonieDenominations - Buy - " + prefix + monies[i].ToString() + " " + denoms[i] + " " + GetDisplayName(denoms[i]));
			#endif
		
			if (monies[i] != 0)
				descriptions.Insert(prefix + Math.AbsInt(monies[i]).ToString() + " × " + GetDisplayName(denoms[i]));
		}
		
		monies.Clear();
		
		if (!m_BuyDenomsTooltip)
		{
			m_BuyDenomsTooltip = new ExpansionMarketMenuTooltip();
			m_BuyDenomsTooltip.Hide();
			m_BuyDenomsTooltip.SetTitle("#STR_EXPANSION_MARKET_YOU_PAY");
			m_BuyDenomsTooltip.SetContentOffset(-0.390625, -0.277778);
		}
		else
		{
			m_BuyDenomsTooltip.ClearEntries();
		}

		foreach (string buyDesc: buyDescriptions)
		{
			m_BuyDenomsTooltip.AddEntry(buyDesc);
		}
		
		buyDescriptions.Clear();

		if (buyChangeDescriptions.Count())
		{
			m_BuyDenomsTooltip.AddEntry("");
			m_BuyDenomsTooltip.AddEntry("#STR_EXPANSION_MARKET_YOU_GET");
			foreach (string buyChangeDesc: buyChangeDescriptions)
			{
				m_BuyDenomsTooltip.AddEntry(buyChangeDesc);
			}
		}
		
		buyChangeDescriptions.Clear();
		
		MarketPrint("UpdateMonieDenominations - End");
	}

	string GetDisplayName(string type)
	{
		//! Uses a cache to speed up repeat lookups
		type.ToLower();
		return ExpansionStatic.GetItemDisplayNameWithType(type, m_TypeDisplayNames);
	}

	string GetPreviewClassName(string className, bool ignoreBaseBuildingKits = false)
	{
		if (GetGame().ConfigIsExisting("CfgVehicles " + className + "_ExpansionMarketPreview"))
		{
			return className + "_ExpansionMarketPreview";
		}
		else if (!ignoreBaseBuildingKits && className.IndexOf("kit") == className.Length() - 3)
		{
			/*************************************************************************************************************************
			 * WARNING: Only TESTED basebuilding items!
			 * Most mods do NOT have the necessary rvConfig entries to get a reasonable preview and/or can cause client CTD if used!
			 * Do NOT add other classnames unless they are GUARANTEED to work properly in market menu!
			 *************************************************************************************************************************/

			//! Expansion
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

			//! Vanilla
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

	void OnIncrementQuantityButtonClick()
	{
		MarketPrint("OnIncrementQuantityButtonClick - Start");
		
		if (ExpansionStatic.Key_CTRL())
		{
			m_Quantity = m_Quantity + 10;
		}
		else if (ExpansionStatic.Key_SHIFT())
		{
			m_Quantity = m_Quantity + 100;
		}
		else if (ExpansionStatic.Key_ALT())
		{
			int playerQuantity = m_MarketModule.GetAmountInInventory(GetSelectedMarketItem(), m_MarketModule.LocalGetEntityInventory());
			if (playerQuantity > 0)
			{
				m_Quantity = playerQuantity;
			}
			else
			{
				m_Quantity = 1;
			}
		}
		else
		{
			m_Quantity++;
		}
		
		UpdateQuantity();
		
		MarketPrint("OnIncrementQuantityButtonClick - End");
	}

	void OnDecrementQuantityButtonClick()
	{
		MarketPrint("OnDecrementQuantityButtonClick - Start");
		
		if (ExpansionStatic.Key_CTRL())
		{
			if ( m_Quantity - 10 > 0 )
				m_Quantity = m_Quantity - 10;
		}
		else if (ExpansionStatic.Key_SHIFT())
		{
			if ( m_Quantity - 100 > 0 )
				m_Quantity = m_Quantity - 100;
		}
		else if (ExpansionStatic.Key_ALT())
		{
			m_Quantity = 1;
		}
		else
		{
			if ( m_Quantity > 1 )
				m_Quantity--;
		}

		UpdateQuantity();
		
		MarketPrint("OnDecrementQuantityButtonClick - End");
	}

	void UpdateQuantity()
	{
		MarketPrint("UpdateQuantity - Start");
		
		if (!GetSelectedMarketItem())
			return;
		
		int limit = m_TraderItemStock;
		int absPlayerStock = Math.AbsInt(m_PlayerStock);
		if (limit < absPlayerStock)
		{
			limit = absPlayerStock;
		}

		if (limit > 0)
		{
			if ( m_Quantity > limit && !GetSelectedMarketItem().IsStaticStock() )
			{
				m_Quantity = limit;
			} 
		}
		
		UpdateItemFieldFromBasicNetSync();
		
		MarketPrint("UpdateQuantity - End");
	}

	//! @note option1 and option2 can have different meaning depending on result
	void MenuCallback(string itemClassName, ExpansionMarketResult result, int option1 = -1, int option2 = -1, Object object = NULL)
	{
		MarketPrint("MenuCallback - Start - " + typename.EnumToString(ExpansionMarketResult, result));

		int amount;
		int price;
		bool includeAttachments;
		int skinIndex;

		string title;
		string text;
		string msgId;
		StringLocaliser loc;
		string icon = EXPANSION_NOTIFICATION_ICON_ERROR;
		int color = COLOR_EXPANSION_NOTIFICATION_ERROR;
		bool notify = true;
		bool sale = result == ExpansionMarketResult.SellSuccess;

		if (result >= ExpansionMarketResult.FailedNoVehicleSpawnPositions && result <= ExpansionMarketResult.FailedVehicleSpawnOccupied)
		{
			msgId = typename.EnumToString(ExpansionVehicleType, option1);
		}

		switch (result)
		{
			case ExpansionMarketResult.SellSuccess:
			case ExpansionMarketResult.PurchaseSuccess:
			{
				//! Transaction went through
				PlayMarketSound();

				notify = !GetExpansionClientSettings().MarketMenuDisableSuccessNotifications;
				if (notify)
				{
					string formattedAmount = option1.ToString() + "x";
					string formattedPrice = GetDisplayPrice(option2, false, false, true);

					if (sale)
						msgId = "SELL";
					else	
						msgId = "PURCHASE";

					title = "STR_EXPANSION_TRADER_" + msgId + "_SUCCESS";
					loc = new StringLocaliser("STR_EXPANSION_TRADER_" + msgId + "_SUCCESS_TEXT", GetDisplayName(itemClassName), formattedAmount, formattedPrice);
					text = loc.Format();
					icon = EXPANSION_NOTIFICATION_ICON_INFO;
					color = COLOR_EXPANSION_NOTIFICATION_SUCCESS;
				}

				break;
			}

			case ExpansionMarketResult.FailedStockChange:
			{
				MarketPrint("MenuCallback - The stock has changed meanwhile");
	
				//! If it was a purchase request, option2 will be includeAttachments (0 or 1)
				//! If it was a sell request, option2 will be -1
				if (option2 == -1)
				{
					if (m_MarketSell)
					{
						if (m_MarketSell.Item && m_MarketSell.Item.ClassName != itemClassName)
							EXLogPrint("MenuCallback - WARNING: MarketSell item class name " + m_MarketSell.Item.ClassName + " != " + itemClassName);
						
						EXLogPrint("===============================================================================");

						EXLogPrint("| MARKET SELL REQUEST FAILED!");
						EXLogPrint("| Price mismatch between client and server.");
			
						EXLogPrint("| Result code: " + typename.EnumToString(ExpansionMarketResult, result));

						EXLogPrint("|");
						EXLogPrint("| CLIENT transaction data");
						EXLogPrint("| -----------------------");
						EXLogPrint("| Total sell price: " + m_SellPrice);
						auto sellDebug = new ExpansionMarketSellDebug(m_MarketSell, m_MarketModule.GetClientZone());
						sellDebug.Dump();
						
						EXLogPrint("===============================================================================");
					}
				}
				else
				{
					//! TODO: Add purchase debug?
					EXLogPrint("MenuCallback - Current stock: " + m_MarketModule.GetClientZone().GetStock(itemClassName));
				}
				
				title = "STR_EXPANSION_MARKET_TITLE";
				text = "STR_EXPANSION_TRADER_STOCK_CHANGED";
				break;
			}

			case ExpansionMarketResult.FailedOutOfStock:
			{
				MarketPrint("MenuCallback - The item is out of stock");
				
				title = "STR_EXPANSION_MARKET_TITLE";
				text = "STR_EXPANSION_TRADER_OUT_OF_STOCK";
				break;
			}

			case ExpansionMarketResult.FailedAttachmentOutOfStock:
			{
				MarketPrint("MenuCallback - The item is out of stock");
				
				title = "STR_EXPANSION_MARKET_TITLE";
				text = "STR_EXPANSION_TRADER_ATTACHMENT_OUT_OF_STOCK";

				amount = option1;
				includeAttachments = (bool) option2;
				ExpansionMarketResult resultTmp;
				ExpansionMarketReserve reserve;
				map<string, int> removedStock;
				m_OutOfStockList.Clear();
				m_MarketModule.FindPriceOfPurchase(ExpansionMarketCategory.GetGlobalItem(itemClassName), m_MarketModule.GetClientZone(), m_TraderMarket, amount, price, includeAttachments, resultTmp, reserve, removedStock, m_OutOfStockList);
				if (m_OutOfStockList.Count())
				{
					loc = new StringLocaliser(text);
					TStringArray attDisplayNames = new TStringArray;
					foreach (string attName: m_OutOfStockList)
					{
						attDisplayNames.Insert(ExpansionStatic.GetItemDisplayNameWithType(attName));
					}
					string outOfStock = ExpansionString.JoinStrings(attDisplayNames);
					text = loc.Format() + " (" + outOfStock + ")";
				}
				break;
			}

			case ExpansionMarketResult.FailedNotEnoughMoney:
			{
				MarketPrint("MenuCallback - player did not have enough money");
				
				title = "STR_EXPANSION_MARKET_TITLE";
				text = "STR_EXPANSION_TRADER_NOT_ENOGH_MONEY";
				break;
			}

			case ExpansionMarketResult.FailedReserveTime:
			{ 
				MarketPrint("MenuCallback - 30 seconds have passed");
				
				title = "STR_EXPANSION_MARKET_TITLE";
				text = "STR_EXPANSION_TRADER_RESERVATION_FAILED";
				break;
			}

			case ExpansionMarketResult.FailedNoCount:
			{ 
				MarketPrint("MenuCallback - player did not set item count");
				
				title = "STR_EXPANSION_TRADER_TRANSACTION_FAILED";
				text = "STR_EXPANSION_TRADER_NONZERO";
				break;
			}

			case ExpansionMarketResult.FailedNoVehicleSpawnPositions:
			{
				MarketPrint("MenuCallback - no vehicle spawn positions");
				
				title = "STR_EXPANSION_MARKET_TITLE";
				text = "STR_EXPANSION_TRADER_NO_" + msgId + "_SPAWN_POSITIONS";
				break;
			}

			case ExpansionMarketResult.FailedNotEnoughVehicleSpawnPositionsNear:
			{
				MarketPrint("MenuCallback - not enough vehicle spawn positions near trader");
				
				title = "STR_EXPANSION_MARKET_TITLE";
				text = "STR_EXPANSION_TRADER_NOT_ENOUGH_" + msgId + "_SPAWN_POSITIONS_NEAR";
				float minDistance = GetExpansionSettings().GetMarket().GetMinVehicleDistanceToTrader(itemClassName);
				float maxDistance = GetExpansionSettings().GetMarket().GetMaxVehicleDistanceToTrader(itemClassName);
				loc = new StringLocaliser(text, minDistance.ToString(), maxDistance.ToString());
				text = loc.Format();
				
				MarketPrint("MenuCallback - " + text);
				break;
			}

			case ExpansionMarketResult.FailedVehicleSpawnOccupied:
			{
				MarketPrint("MenuCallback - vehicle spawn occupied");
				
				title = "STR_EXPANSION_MARKET_TITLE";
				text = "STR_EXPANSION_TRADER_ANOTHER_VEHICLE_ON_SPAWN";

				if (object)
				{
					loc = new StringLocaliser(text);
					string displayName;
					PlayerBase player = PlayerBase.Cast(object);
					if (player && player.GetIdentity())
						displayName = player.GetIdentityName();  //! So you can call 'em out in chat - unless it's yourself...
					else
						displayName = object.GetDisplayName();
					text = loc.Format() + " (" + displayName + ")";
				}
				break;
			}

			case ExpansionMarketResult.FailedTooFarAway:
			{
				MarketPrint("MenuCallback - player is too far from trader");
				
				title = "STR_EXPANSION_MARKET_TITLE";
				text = "STR_EXPANSION_TRADER_TOO_FAR_AWAY";
				break;
			}

			case ExpansionMarketResult.FailedCannotSell:
			{
				MarketPrint("MenuCallback - cannot sell this item to this trader");
				
				title = "STR_EXPANSION_MARKET_TITLE";
				text = "STR_EXPANSION_TRADER_CANNOTSELL";
				break;
			}

			case ExpansionMarketResult.FailedCannotBuy:
			{
				MarketPrint("MenuCallback - cannot buy this item at this trader");
				
				title = "STR_EXPANSION_MARKET_TITLE";
				text = "STR_EXPANSION_TRADER_CANNOTBUY";
				break;
			}

			case ExpansionMarketResult.FailedNotInPlayerPossession:
			{
				MarketPrint("MenuCallback - cannot sell something that is not in player's possession");
				
				title = "STR_EXPANSION_MARKET_TITLE";
				text = "STR_EXPANSION_TRADER_NOT_IN_PLAYER_POSSESSION";
				break;
			}

			case ExpansionMarketResult.FailedItemDoesNotExistInTrader:
			{
				MarketPrint("MenuCallback - item does not exist in trader: " + itemClassName);
				
				title = "STR_EXPANSION_MARKET_TITLE";
				text = "Transaction failed: " + GetDisplayName(itemClassName) + " (" + itemClassName + ") does not exist in the trader's item list.";
				break;
			}

			case ExpansionMarketResult.FailedItemSpawn:
			{
				MarketPrint("MenuCallback - item could not be spawned: " + itemClassName);
				
				title = "STR_EXPANSION_MARKET_TITLE";
				text = "Transaction failed: " + GetDisplayName(itemClassName) + " (" + itemClassName + ") could not be spawned.";
				break;
			}

			//! Inventory desync or tampering attempt
			case ExpansionMarketResult.FailedSellListMismatch:
			{
				MarketPrint("MenuCallback - sell list mismatch");
				
				title = "STR_EXPANSION_MARKET_TITLE";
				text = "Transaction failed: Your client sent a list of items to sell that is different from what the server sees.";
				break;
			}
			
			case ExpansionMarketResult.IntegerOverflow:
			{
				MarketPrint("MenuCallback - integer overflow");
				
				title = "STR_EXPANSION_MARKET_TITLE";
				text = "Integer overflow. Reduce the quantity you are trying to buy or sell.";
				break;
			}

			default:
			{
				MarketPrint("MenuCallback - unknown error");

				title = "STR_EXPANSION_MARKET_TITLE";
				text = "UNKNOWN ERROR";
				break;
			}
		}

		if (notify)
			ExpansionNotification(title, text, icon, color, 3, ExpansionNotificationType.MARKET).Create();
	
		switch (result)
		{
			case ExpansionMarketResult.FailedItemDoesNotExistInTrader:
			case ExpansionMarketResult.FailedItemSpawn:
			case ExpansionMarketResult.IntegerOverflow:
				//! Do nothing
				break;

			default:
				RequestSelectedItem(ExpansionMarketMenuState.LOADING, itemClassName, sale);
				break;
		}
				
		MarketPrint("MenuCallback - End");
	}

	void RequestSelectedItem(ExpansionMarketMenuState menuState, string itemClassName = string.Empty, bool sale = false)
	{
		MarketPrint("RequestSelectedItem - Start");
		MarketPrint("RequestSelectedItem - " + itemClassName + " - sale " + sale);
		ExpansionMarketItem item;
		TIntArray itemIDs = new TIntArray;
		if (sale && m_MarketSell && m_MarketSell.Item && m_MarketSell.Item.ClassName == itemClassName)
		{
			//! Last action was a sale. Request stock info for sale item and its attachments
			foreach (ExpansionMarketSellItem sellItem: m_MarketSell.Sell)
			{
				string className = sellItem.ClassName;
				className.ToLower();
				item = ExpansionMarketCategory.GetGlobalItem(className, false);
				if (item && itemIDs.Find(item.ItemID) == -1)
					itemIDs.Insert(item.ItemID);
			}
		}
		else
		{
			//! Last action was not a sale. Request stock info for selected item and its attachments
			item = ExpansionMarketCategory.GetGlobalItem(itemClassName, false);
			itemIDs.Insert(item.ItemID);
			TIntArray attachmentIDs = GetCurrentSelectedAttachmentIDs(true, item);
			foreach (int attachmentID: attachmentIDs)
			{
				if (itemIDs.Find(attachmentID) == -1)
					itemIDs.Insert(attachmentID);
			}
		}
		SetMenuState(menuState);
		m_MarketModule.RequestTraderItems(m_TraderObject, 0, m_TraderMarket.m_StockOnly, itemIDs);
		MarketPrint("RequestSelectedItem - End");
	}

	void OnBuyButtonClick()
	{
		MarketPrint("OnBuyButtonClick - Start");
		
		if (!GetSelectedMarketItem())
			return;
		
		if (!m_MarketModule)
			return;
		
		if (GetExpansionClientSettings().MarketMenuSkipConfirmations)
		{
			OnConfirmBuyButtonClick();
		}
		else
		{
			SetMenuState(ExpansionMarketMenuState.DIALOG);
			ExpansionMarketMenuDialogData dialogData = new ExpansionMarketMenuDialogData;
			dialogData.ClassName = GetSelectedMarketItem().ClassName;
			dialogData.Amount = m_Quantity;
			dialogData.Price = m_BuyPrice;
			if (m_SelectedMarketItemElement.GetIncludeAttachments() && m_SelectedMarketItem.SpawnAttachments.Count() > 0)
				dialogData.IncludeAttachments = true;
			
			m_PurchaseDialog = new ExpansionMenuDialog_MarketConfirmPurchase(this, dialogData);
			m_PurchaseDialog.Show();
		}
		
		MarketPrint("OnBuyButtonClick - End");
	}

	void OnConfirmBuyButtonClick()
	{
		MarketPrint("OnConfirmBuyButtonClick - Start");
		
		if (!GetSelectedMarketItem())
			return;
		
		if (!m_MarketModule)
			return;
		
		SetMenuState(ExpansionMarketMenuState.REQUESTING_PURCHASE);

		//! https://feedback.bistudio.com/T173348
		bool includeAttachments;
		TIntArray attachmentIDs = {};
		if (GetSelectedMarketItem().SpawnAttachments.Count() > 0 && GetSelectedMarketItemElement().GetIncludeAttachments())
		{
			includeAttachments = true;
			attachmentIDs = GetCurrentSelectedAttachmentIDs();
		}

		m_MarketModule.RequestPurchase(GetSelectedMarketItem().ClassName, m_Quantity, m_BuyPrice, m_TraderObject, NULL, includeAttachments, GetSelectedMarketItemElement().GetCurrentSelectedSkinIndex(), attachmentIDs);
		if (m_PurchaseDialog)
			m_PurchaseDialog.Hide();
		
		MarketPrint("OnConfirmBuyButtonClick - End");
	}

	void OnSellButtonClick()
	{
		MarketPrint("OnSellButtonClick - Start");
		
		if (!GetSelectedMarketItem())
			return;

		if (!m_MarketModule)
			return;
		
		if (GetExpansionClientSettings().MarketMenuSkipConfirmations)
		{
			OnConfirmSellButtonClick();
		}
		else
		{
			SetMenuState(ExpansionMarketMenuState.DIALOG);
			ExpansionMarketMenuDialogData dialogData = new ExpansionMarketMenuDialogData;
			dialogData.ClassName = m_SelectedMarketItem.ClassName;
			dialogData.Amount = m_Quantity;
			dialogData.Price = m_SellPrice;

			//! We always sell with attachments if the player's item has any.
			//! Since MarketSell.Sell contains the main item(s) and their attachments, all we need to do to figure out if the item being sold has atts
			//! is to check if any entry does not have the same class name as the selected market item.
			foreach (ExpansionMarketSellItem sellItem: m_MarketSell.Sell)
			{
				if (sellItem.ClassName != m_SelectedMarketItem.ClassName)
				{
					dialogData.IncludeAttachments = true;
					break;
				}
			}
			
			m_SellDialog = new ExpansionMenuDialog_MarketConfirmSell(this, dialogData);
			m_SellDialog.Show();
		}
		
		MarketPrint("OnSellButtonClick - End");
	}

	void OnConfirmSellButtonClick()
	{
		MarketPrint("OnConfirmSellButtonClick - Start");
		
		if (!GetSelectedMarketItem())
			return;

		if (!m_MarketModule)
			return;
		
		if (m_PlayerStock == 0)
		{
			ExpansionNotification("STR_EXPANSION_MARKET_TITLE", "STR_EXPANSION_MARKET_DONT_HAVE_ITEM", EXPANSION_NOTIFICATION_ICON_ERROR, COLOR_EXPANSION_NOTIFICATION_ERROR, 3, ExpansionNotificationType.MARKET).Create();
			return;
		}
		
		SetMenuState(ExpansionMarketMenuState.REQUESTING_SELL);
		m_MarketModule.RequestSell(GetSelectedMarketItem().ClassName, m_Quantity, m_SellPrice, m_TraderObject, m_MarketSell);
		if (m_SellDialog)
			m_SellDialog.Hide();
		
		MarketPrint("OnConfirmSellButtonClick - End");
	}

	void OnQuantityButtonClick()
	{
		MarketPrint("OnQuantityButtonClick - Start");
		
		if (!m_QuantityDialog)
			m_QuantityDialog = new ExpansionMenuDialog_MarketSetQuantity(this);
		
		SetMenuState(ExpansionMarketMenuState.DIALOG);	
		m_QuantityDialog.Show();
		
		MarketPrint("OnQuantityButtonClick - End");
	}

	void SetQuantity(string quantity)
	{
		MarketPrint("SetQuantity - Start");
		
		m_Quantity = quantity.ToInt();

		if (m_QuantityDialog)
		{
			m_QuantityDialog.Destroy();
		}
		
		UpdateQuantity();
		SetMenuState(ExpansionMarketMenuState.NONE);	
		
		MarketPrint("SetQuantity - End");
	}

	int GetQuantity()
	{
		return m_Quantity;
	}

	void OnFilterButtonClick()
	{
		MarketPrint("OnFilterButtonClick - Start");
		
		market_filter_box.SetText("");		
		UpdateMarketCategories();
		
		MarketPrint("OnFilterButtonClick - End");
	}

	override void OnHide()
	{
		MarketPrint("OnHide - Start");
				
		m_MarketModule.ExitTrader();

		ExpansionMarketModule.SI_SetTraderInvoker.Remove(SetTraderObject);
		ExpansionMarketModule.SI_SelectedItemUpdatedInvoker.Remove(OnNetworkItemUpdate);
		ExpansionMarketModule.SI_Callback.Remove(MenuCallback);

		if (m_PlayerPreview)
			m_PlayerPreview = null;
	
		m_SelectedMarketItem = NULL;
		
		ClearCategoryItems();
		ClearCategories();
		ClearDropdownElements();
		ClearSkins();
		
		if (m_FilterOptionStrings)
		{
			m_FilterOptionStrings.Clear();
		}
		
		if (m_MarketFilters)
		{
			m_MarketFilters = null;
		}
		
		if (m_PlayerItems)
		{
			m_PlayerItems.Clear();
		}
		
		if (m_QuantityDialog)
		{
			m_QuantityDialog.Destroy();
		}
		
		if (m_AttachmentsTooltip)
		{
			m_AttachmentsTooltip.Destroy();
		}

		if (m_PurchaseDialog)
		{
			m_PurchaseDialog.Destroy();
		}
		
		if (m_SellDialog)
		{
			m_SellDialog.Destroy();
		}
		
		if (m_CurrenciesTooltip)
		{
			m_CurrenciesTooltip.Destroy();
		}
		
		if (m_BuyDenomsTooltip)
		{
			m_BuyDenomsTooltip.Destroy();
		}
		
		if (m_SellDenomsTooltip)
		{
			m_SellDenomsTooltip.Destroy();
		}
		
		if (m_QuatityTooltip)
		{
			m_QuatityTooltip.Destroy();
		}
		
		if (m_MarketMenuItemManager)
		{
			m_MarketMenuItemManager.Destroy();
		}
		
		m_FirstCall = false;
		
		super.OnHide();
		
		MarketPrint("OnHide - End");
	}

	void OnAttachmentsCheckboxStateChange()
	{
		MarketPrint("OnAttachmentsCheckboxStateChange - Start");
		
		if (!GetSelectedMarketItemElement())
			return;

		GetSelectedMarketItemElement().SetIncludeAttachments(GetMarketMenuController().IncludeAttachments);
		GetSelectedMarketItemElement().UpdatePreviewObject();
		GetSelectedMarketItemElement().UpdateView();
	
		UpdatePreview();
		UpdateItemFieldFromBasicNetSync();
		SetItemAttachmentsInfo(GetSelectedMarketItemElement());
		
		MarketPrint("OnAttachmentsCheckboxStateChange - End");
	}

	override bool OnChange(Widget w, int x, int y, bool finished)
	{				
		if (w == market_filter_box)
		{
			UpdateMarketCategories();
		}
		
		return false;
	}

	override bool OnMouseEnter(Widget w, int x, int y)
	{
		switch (w)
		{
			case market_filter_clear:
				market_filter_clear_icon.SetColor(GetExpansionSettings().GetMarket().MarketMenuColors.Get("ColorSearchFilterButton"));
				break;
			case dropdown_selector:
				dropdown_selector.SetColor(GetExpansionSettings().GetMarket().MarketMenuColors.Get("ColorFilterOptionsButton"));
				dropdown_collapse_image.SetColor(GetExpansionSettings().GetMarket().MarketMenuColors.Get("ColorFilterOptionsIcon"));
				dropdown_expand_image.SetColor(GetExpansionSettings().GetMarket().MarketMenuColors.Get("ColorFilterOptionsIcon"));
				break;
			case market_item_info_attachments_button:
				if (m_AttachmentsTooltip) m_AttachmentsTooltip.Show();
				break;
			case player_info_content:
				if (m_CurrenciesTooltip) m_CurrenciesTooltip.Show();
				break;
			case market_item_buy_price_panel:
				if (m_BuyDenomsTooltip && ExpansionMarketMenuTooltipController.Cast(m_BuyDenomsTooltip.GetController()).SpacerEntries.Count())
					m_BuyDenomsTooltip.Show();
				break;
			case market_item_sell_price_panel:
				if (m_SellDenomsTooltip && ExpansionMarketMenuTooltipController.Cast(m_SellDenomsTooltip.GetController()).SpacerEntries.Count())
					m_SellDenomsTooltip.Show();
				break;
			case market_footer_categories_toggle_button:
				market_footer_categories_label.SetColor(GetExpansionSettings().GetMarket().MarketMenuColors.Get("ColorToggleCategoriesText"));
				break;
			case market_footer_close_menu_button:
				market_footer_close_menu_label.SetColor(GetExpansionSettings().GetMarket().MarketMenuColors.Get("ColorToggleCategoriesText"));
				break;
			case market_item_info_attachments_checkbox:
				market_item_info_attachments_checkbox.SetColor(GetExpansionSettings().GetMarket().MarketMenuColors.Get("ColorSetQuantityButton"));
				break;
			case skin_selector:
				skin_selector.SetColor(GetExpansionSettings().GetMarket().MarketMenuColors.Get("ColorFilterOptionsButton"));
				skin_selector_expand_image.SetColor(GetExpansionSettings().GetMarket().MarketMenuColors.Get("ColorFilterOptionsIcon"));
				skin_selector_collapse_image.SetColor(GetExpansionSettings().GetMarket().MarketMenuColors.Get("ColorFilterOptionsIcon"));
				break;
			case market_item_info_quantity_button:
				if (m_QuatityTooltip) m_QuatityTooltip.Show();
				break;
			case market_item_info_attachments_setup_button:
				market_item_info_attachments_setup_button.SetColor(GetExpansionSettings().GetMarket().MarketMenuColors.Get("ColorSetQuantityButton"));
				break;
		}		
		return false;
	}

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		switch (w)
		{
			case market_filter_clear:
				market_filter_clear_icon.SetColor(ARGB(140, 251, 252, 254));
				break;
			case dropdown_selector:
				dropdown_selector.SetColor(ARGB(0, 0, 0, 0));
				dropdown_collapse_image.SetColor(ARGB(140, 251, 252, 254));
				dropdown_expand_image.SetColor(ARGB(140, 251, 252, 254));
				break;
			case market_item_info_attachments_button:
				if (m_AttachmentsTooltip) m_AttachmentsTooltip.Hide();
				break;
			case player_info_content:
				if (m_CurrenciesTooltip) m_CurrenciesTooltip.Hide();
				break;
			case market_item_buy_price_panel:
				if (m_BuyDenomsTooltip) m_BuyDenomsTooltip.Hide();
				break;
			case market_item_sell_price_panel:
				if (m_SellDenomsTooltip) m_SellDenomsTooltip.Hide();
				break;
			case market_footer_categories_toggle_button:
				market_footer_categories_label.SetColor(ARGB(255, 255, 255, 255));
				break;
			case market_footer_close_menu_button:
				market_footer_close_menu_label.SetColor(ARGB(255, 255, 255, 255));
				break;
			case market_item_info_attachments_checkbox:
				market_item_info_attachments_checkbox.SetColor(GetExpansionSettings().GetMarket().MarketMenuColors.Get("BaseColorText"));
				break;
			case skin_selector:
				skin_selector.SetColor(GetExpansionSettings().GetMarket().MarketMenuColors.Get("BaseColorLabels"));
				skin_selector_expand_image.SetColor(ARGB(240, 251, 252, 254));
				skin_selector_collapse_image.SetColor(ARGB(240, 251, 252, 254));
				break;
			case market_item_info_quantity_button:
				if (m_QuatityTooltip) m_QuatityTooltip.Hide();
				break;
			case market_item_info_attachments_setup_button:
				market_item_info_attachments_setup_button.SetColor(ARGB(0, 0, 0, 0));
				break;
		}
			
		return false;
	}

	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{		
		if (w == market_item_preview)
		{
			GetGame().GetDragQueue().Call(this, "UpdateRotation");
			GetMousePos(m_CharacterRotationX, m_CharacterRotationY);
			return true;
		}
		else if (w == market_player_preview)
		{
			m_PlayerPreview.OnMouseButtonDown(w, x, y, button);
			GetMousePos(m_CharacterRotationX, m_CharacterRotationY);
			return true;
		}
		
		return false;
	}

	override bool OnMouseButtonUp( Widget w, int x, int y, int button )
	{
		if (w == dropdown_selector && button == MouseState.LEFT)
		{
			SetDropdownState();
			return true;
		}
		else if (w == skin_selector && button == MouseState.LEFT)
		{
			SetSkinSelectorState();
			return true;
		}
		return false;
	}

	override bool OnMouseWheel(Widget w, int x, int y, int wheel) 
	{
		if (w == market_item_preview)
		{
			GetGame().GetDragQueue().Call(this, "UpdateScale");
			m_CharacterScaleDelta = wheel ;
		}
		else if (w == market_player_preview)
		{
			m_PlayerPreview.OnMouseWheel(w, x, y, wheel);
			m_CharacterScaleDelta = wheel ;
		}
		
		return false;
	}

	override bool OnItemSelected(Widget w, int x, int y, int row, int column, int oldRow, int oldColumn)
	{
		float itemx, itemy;
		if (w == market_item_preview)
		{
			m_CharacterOrientation = vector.Zero;
			
			market_item_preview.SetModelPosition(Vector(0,0,0.5));
			market_item_preview.SetModelOrientation(m_CharacterOrientation);
		
			market_item_preview.GetPos(itemx, itemy);

			market_item_preview.SetSize(1.5, 1.5);

			//! Align to center 
			market_item_preview.SetPos(-0.225, -0.225);
		}
		else if (w == market_player_preview)
		{
			m_CharacterOrientation = vector.Zero;
			
			m_PlayerPreview.OnItemSelected(w, x, y, row, column, oldRow, oldColumn);
		}
		
		return false;
	}

	void UpdateScale(int mouse_x, int mouse_y, bool is_dragging, int wheel)
	{
		float w, h, x, y;		
		market_item_preview.GetPos(x, y);
		market_item_preview.GetSize(w,h);
		w = w + (m_CharacterScaleDelta / 4);
		h = h + (m_CharacterScaleDelta / 4);
		if ( w > 0.5 && w < 4 )
		{
			market_item_preview.SetSize(w, h);
	
			//! Align to center 
			int screen_w, screen_h;
			GetScreenSize(screen_w, screen_h);
			float new_x = x - (m_CharacterScaleDelta / 8);
			float new_y = y - (m_CharacterScaleDelta / 8);
			market_item_preview.SetPos(new_x, new_y);
		}
	}

	void UpdateRotation(int mouse_x, int mouse_y, bool is_dragging)
	{
		vector o = m_CharacterOrientation;
		o[2] = o[2] + (m_CharacterRotationY - mouse_y);
		o[1] = o[1] - (m_CharacterRotationX - mouse_x);
		
		market_item_preview.SetModelOrientation(o);
		
		if (!is_dragging)
		{
			m_CharacterOrientation = o;
		}
	}

	void SetFilterPrimeMag(bool state)
	{
		m_ShowPrimeMag = state;
	}

	bool ShowPrimeMag()
	{
		return m_ShowPrimeMag;
	}

	void SetFilterSecondMag(bool state)
	{
		m_ShowSecondMag = state;
	}

	bool ShowSecondMag()
	{
		return m_ShowSecondMag;
	}

	void SetFilterPrimeAttach(bool state)
	{
		m_ShowPrimeAttach = state;
	}

	bool ShowPrimeAttach()
	{
		return m_ShowPrimeAttach;
	}

	void SetFilterSecondAttach(bool state)
	{
		m_ShowSecondAttach = state;
	}

	bool ShowSecondAttach()
	{
		return m_ShowSecondAttach;
	}

	void SetFilterHandMag(bool state)
	{
		m_ShowHandMag = state;
	}

	bool ShowHandMag()
	{
		return m_ShowHandMag;
	}

	void SetFilterHandAttach(bool state)
	{
		m_ShowHandAttach = state;
	}

	bool ShowHandAttach()
	{
		return m_ShowHandAttach;
	}

	void SetFilterPrimeBullet(bool state)
	{
		m_ShowPrimeBullets = state;
	}

	bool ShowPrimeBullet()
	{
		return m_ShowPrimeBullets;
	}

	void SetFilterSellables(bool state)
	{
		m_ShowSellable = state;
		m_MarketMenuController.ShowSellables = state;
	}

	bool ShowSellables()
	{
		return m_ShowSellable;
	}

	void SetFilterPurchasables(bool state)
	{
		m_ShowPurchasables = state;
		m_MarketMenuController.ShowPurchasables = state;
	}

	bool ShowPurchasables()
	{
		return m_ShowPurchasables;
	}

	void SetFilterSecondBullet(bool state)
	{
		m_ShowSecondBullets = state;
	}

	bool ShowSecondBullet()
	{
		return m_ShowSecondBullets;
	}

	void SetFilterHandBullet(bool state)
	{
		m_ShowHandBullets = state;
	}

	bool ShowHandBullet()
	{
		return m_ShowHandBullets;
	}

	void SetIsLoading(bool state, ExpansionMarketMenuState menuState = ExpansionMarketMenuState.INVALID)
	{
		if (menuState == ExpansionMarketMenuState.INVALID)
		{
			if (state)
				menuState = ExpansionMarketMenuState.LOADING;
			else
				menuState = ExpansionMarketMenuState.NONE;
		}

		SetMenuState(menuState);

		market_loading.Show(state);
	}

	bool IsLoading()
	{
		return m_CurrentState != ExpansionMarketMenuState.NONE || !m_Complete;
	}

	protected void OnNetworkItemUpdate()
	{
		MarketPrint("OnNetworkItemUpdate - Start");
		
		m_MarketModule.EnumeratePlayerInventory(PlayerBase.Cast(GetGame().GetPlayer()));
		
		if (GetSelectedMarketItemElement() && GetSelectedMarketItem())
		{
			EXPrint("OnNetworkItemUpdate() -> GetSelectedMarketItemElement().UpdateView()");
			GetSelectedMarketItemElement().UpdateView();
			UpdateItemFieldFromBasicNetSync();
		}
		
		MarketPrint("OnNetworkItemUpdate - End");
	}

	ExpansionTraderObjectBase GetTraderObject()
	{
		return m_TraderObject;
	}

	void PlayMarketSound()
	{
		if (!GetGame().IsDedicatedServer())
		{	
			if (GetGame().GetPlayer())
			{
				string soundConfig;
				string path = "cfgVehicles " + GetSelectedMarketItem().ClassName + " " + "AnimEvents ";
				int events_sources_count = GetGame().ConfigGetChildrenCount(path);
				string soundName = "";
				EffectSound sound;
				
				MarketPrint("PlayMarketSound - Search sound: " + path);
				MarketPrint("PlayMarketSound - Effents count: " + events_sources_count);
				
				for (int i = 0; i < events_sources_count; i++)
				{
					string soundWeapons;
					GetGame().ConfigGetChildName(path, i, soundWeapons);
					MarketPrint("PlayMarketSound - Path 2: " + path + soundWeapons + " pickUpItem" + " soundSet");
					
					if (GetGame().ConfigIsExisting(path + soundWeapons + " pickUpItem" + " soundSet"))
					{
						GetGame().ConfigGetText(path + soundWeapons + " pickUpItem" + " soundSet", soundName);
						MarketPrint("PlayMarketSound - Sound: " + soundName);
					}
				}
				
				if (soundName != "")
				{
					sound = SEffectManager.PlaySound(soundName, GetGame().GetPlayer().GetPosition());
				}
				else
				{
					sound = SEffectManager.PlaySound("pickUpBackPack_Metal_SoundSet", GetGame().GetPlayer().GetPosition());
				}
				
				sound.SetSoundAutodestroy(true);
			}
		}
	}

	protected bool UsePlayerPreview()
	{
		if (GetSelectedMarketItemElement().GetPreviewObject())
			return GetSelectedMarketItemElement().GetPreviewObject().IsInherited(Clothing_Base);
		return false;
	}
	
	void MarketPrint(string text)
	{
		#ifdef EXPANSIONMODMARKET_DEBUG
		EXPrint("ExpansionMarketMenu::" + text);
		#endif
	}

	void OnToggleCategoriesButtonClick()
	{
		MarketPrint("OnToggleCategoriesButtonClick - Start");
		
		int i;
		ExpansionMarketMenuCategory category;

		//! This takes a player expanding/collapsing single categories into account
		bool toggleState = CategoriesExpanded == 0;
		GetExpansionClientSettings().MarketMenuCategoriesState = toggleState;
		
		for (i = 0; i < m_MarketMenuController.MarketCategories.Count(); i++)
		{
			category = m_MarketMenuController.MarketCategories[i];
			category.ToggleCategory(toggleState);
		}
		
		MarketPrint("OnToggleCategoriesButtonClick - End");
	}

	void OnCloseMenuButtonClick()
	{
		GetDayZExpansion().GetExpansionUIManager().CloseMenu();
	}

	ExpansionMarketMenuItem GetSelectedMarketItemElement()
	{
		return m_SelectedMarketItemElement;
	}

	ExpansionMarketItem GetSelectedMarketItem()
	{
		return m_SelectedMarketItem;
	}

	override float GetUpdateTickRate()
	{
		return 0.5;
	}

	override void Expansion_Update()
	{
		//! Buy/Sell Shortcuts
		if (GetFocus() != market_filter_box)
		{
			if (ExpansionStatic.Key_B() && !m_KeyInput && m_SelectedMarketItem != NULL && m_TraderItemStock > 0)
			{
				OnBuyButtonClick();
				m_KeyInput = true;
				return;
			}
			else if (ExpansionStatic.Key_S() && !m_KeyInput && m_SelectedMarketItem != NULL && m_PlayerStock > 0)
			{
				OnSellButtonClick();
				m_KeyInput = true;
				return;
			}
		}
		
		//! Quantity Shortcuts
		if (GetSelectedMarketItem() != NULL)
		{
			if (ExpansionStatic.Key_UP() || ExpansionStatic.Key_RIGHT())
			{
				OnIncrementQuantityButtonClick();
			}
			else if (ExpansionStatic.Key_DOWN() || ExpansionStatic.Key_LEFT())
			{
				OnDecrementQuantityButtonClick();
			}
		}
	}

	ExpansionMarketMenuState GetMenuState()
	{
		return m_CurrentState;
	}

	void SetMenuState(ExpansionMarketMenuState state)
	{
		EXPrint(ToString() + "::SetMenuState " + typename.EnumToString(ExpansionMarketMenuState, m_CurrentState) + " -> " + typename.EnumToString(ExpansionMarketMenuState, state));
		m_CurrentState = state;
	}

	int GetCurrentTraderItemStock()
	{
		return m_TraderItemStock;
	}

	int GetCurrentPlayerItemStock()
	{
		return m_PlayerStock;
	}

	int GetCurrentItemBuyPrice()
	{
		return m_BuyPrice;
	}	

	int GetCurrentItemSellPrice()
	{
		return m_SellPrice;
	}

	ExpansionMarketTrader GetMarketTrader()
	{
		return m_TraderMarket;
	}

	void OnSelectAttachmentsButtonClick()
	{
		market_menu_content.Show(false);
		market_menu_attachments_content.Show(true);
	}

	void AllowCategoryButtonInputs(bool state)
	{
		for (int i = 0; i < m_MarketMenuController.MarketCategories.Count(); i++)
		{
			ExpansionMarketMenuCategory category = m_MarketMenuController.MarketCategories[i];
			category.AllowButtonInputs(state);
		}
	}

	void SetDropdownState()
	{
		MarketPrint("SetDropdownState - Start");
		
		m_IsDropdownExpanded = !m_IsDropdownExpanded;
		dropdown_container.Show( m_IsDropdownExpanded );
		dropdown_content.Show( m_IsDropdownExpanded );
		dropdown_expand_image.Show( m_IsDropdownExpanded );
		dropdown_collapse_image.Show( !m_IsDropdownExpanded );
		dropdown_selector.Update();
				
		if (!m_IsDropdownExpanded)
		{
			dropdown_container.SetFlags(WidgetFlags.NOFOCUS | WidgetFlags.IGNOREPOINTER);
		}
		else
		{
			skin_selector_container.ClearFlags(WidgetFlags.NOFOCUS | WidgetFlags.IGNOREPOINTER);
		}
		
		MarketPrint("SetDropdownState - End");
	}

	void SetSkinSelectorState()
	{
		MarketPrint("SetSkinSelectorState - Start");
		
		m_IsSkinSelectorExpanded = !m_IsSkinSelectorExpanded;
		skin_selector_content.Show(m_IsSkinSelectorExpanded);
		skin_selector_expand_image.Show(m_IsSkinSelectorExpanded);
		skin_selector_collapse_image.Show(!m_IsSkinSelectorExpanded);
		skin_selector.Update();
				
		if (!m_IsSkinSelectorExpanded)
		{
			skin_selector_container.SetFlags(WidgetFlags.NOFOCUS | WidgetFlags.IGNOREPOINTER);
			skin_selector_container.Show(false);
			skin_selector_container.VScrollToPos(0);
		}
		else
		{
			skin_selector_container.ClearFlags(WidgetFlags.NOFOCUS | WidgetFlags.IGNOREPOINTER);
			skin_selector_container.Show(true);			
			skin_selector_container.VScrollToPos(0);
		}
		
		MarketPrint("SetSkinSelectorState - End");
	}

	ExpansionMarketFilters GetMarketFilters()
	{
		return m_MarketFilters;
	}

	void HideItemManager()
	{
		market_menu_content.Show(true);
	}
			
	ExpansionMarketMenuController GetMarketMenuController()
	{
		return m_MarketMenuController;
	}

	ExpansionMarketMenuItemManager GetMarketMenuItemManager()
	{
		return m_MarketMenuItemManager;
	}

	void OnOpenItemManagerButtonClick()
	{
		if (!m_MarketMenuItemManager.IsVisible())
		{
			market_menu_content.Show(false);
			m_MarketMenuItemManager.SetMarketItem(GetSelectedMarketItem());
			m_MarketMenuItemManager.Show();
			market_item_preview.Show(false);
		}
		else
		{
			market_menu_content.Show(true);
			m_MarketMenuItemManager.Hide();
			market_item_preview.Show(true);
		}
	}

	array<int> GetCurrentSelectedAttachmentIDs(bool recursive = false, ExpansionMarketItem item = NULL)
	{
		array<int> attachmentIDs = new array<int>;
		if (!item)
			item = GetSelectedMarketItem();
		foreach (string attachment: item.SpawnAttachments)
		{
			ExpansionMarketItem attachmentItem = ExpansionMarketCategory.GetGlobalItem(attachment);
			attachmentIDs.Insert(attachmentItem.ItemID);
			if (recursive)
				attachmentIDs.InsertAll(GetCurrentSelectedAttachmentIDs(recursive, attachmentItem));
		}
		return attachmentIDs;
	}

	void OnNameSortClick()
	{
		if (m_SortPriority == ExpansionMarketMenuSortPriority.NAME)
		{
			m_NameSortState = !m_NameSortState;

			if (m_NameSortState)
			{
				sort_item_name_text.SetText("Z-A");
				sort_item_name_icon.ClearFlags(WidgetFlags.FLIPV);
			}
			else
			{
				sort_item_name_text.SetText("A-Z");
				sort_item_name_icon.SetFlags(WidgetFlags.FLIPV);
			}
		}
		else
		{
			m_SortPriority = ExpansionMarketMenuSortPriority.NAME;
		}

		sort_item_name_text.SetAlpha(1);
		sort_item_name_icon.SetAlpha(1);
		sort_item_price_symbol.SetAlpha(0.4);
		sort_item_price_icon.SetAlpha(0.4);

		SortCategoryItems();
	}
	
	void OnPriceSortClick()
	{
		if (m_SortPriority == ExpansionMarketMenuSortPriority.PRICE)
		{
			m_PriceSortState = !m_PriceSortState;

			if (m_PriceSortState)
			{
				sort_item_price_icon.ClearFlags(WidgetFlags.FLIPV);
			}
			else
			{
				sort_item_price_icon.SetFlags(WidgetFlags.FLIPV);
			}
		}
		else
		{
			m_SortPriority = ExpansionMarketMenuSortPriority.PRICE;
		}

		sort_item_name_text.SetAlpha(0.7);
		sort_item_name_icon.SetAlpha(0.4);
		sort_item_price_symbol.SetAlpha(1);
		sort_item_price_icon.SetAlpha(1);

		SortCategoryItems();
	}

	void SortCategoryItems()
	{
		for (int i = 0; i < m_MarketMenuController.MarketCategories.Count(); i++)
		{
			ExpansionMarketMenuCategory menuCategory = m_MarketMenuController.MarketCategories[i];
			if (menuCategory.IsExpanded())
				menuCategory.SortItems();
		}
	}

	//! @brief Get name sorting. Descending = true, ascending = false
	bool GetNameSortState()
	{
		return m_NameSortState;
	}

	//! @brief Get price sorting. Descending = true, ascending = false
	bool GetPriceSortState()
	{
		return m_PriceSortState;
	}

	//! @brief Get sorting priority
	ExpansionMarketMenuSortPriority GetSortPriority()
	{
		return m_SortPriority;
	}
	
	ExpansionMarketModule GetMarketModule()
	{
		return m_MarketModule;
	}
};

class ExpansionMarketMenuController: ExpansionViewController
{
	ref ObservableCollection<ref ExpansionMarketMenuCategory> MarketCategories = new ObservableCollection<ref ExpansionMarketMenuCategory>(this);
	ref ObservableCollection<ref ExpansionMarketMenuDropdownElement> DropdownElements = new ObservableCollection<ref ExpansionMarketMenuDropdownElement>(this);
	ref ObservableCollection<ref ExpansionMarketMenuSkinsDropdownElement> SkinsDropdownElements = new ObservableCollection<ref ExpansionMarketMenuSkinsDropdownElement>(this);
	
	string MarketName;
	string PlayerTotalMoney;
	string MarketItemName;
	string MarketItemDesc;
	Object MarketItemPreview;	
	string MarketItemTotalSellPrice;
	string MarketItemTotalBuyPrice;	
	string MarketItemStockPlayer;
	string MarketItemStockTrader;
	string MarketQuantity;
	string MarketIcon;
	string MarketItemInfoName;
	Object MarketPlayerPreview;
	string CurrencySellIcon;
	string CurrencyBuyIcon;
	bool ShowSellables;
	bool WasShowingSellables;
	bool ShowPurchasables;
	bool WasShowingPurchasables;
	bool IncludeAttachments = true;
	bool WasIncludingAttachments = true;

	override void PropertyChanged(string property_name)
	{
	#ifdef EXPANSIONMODMARKET_DEBUG
		EXPrint("ExpansionMarketMenuController::PropertyChanged - Start");
	#endif
		ExpansionMarketMenu menu;
		menu = ExpansionMarketMenu.Cast(GetParent());
		if (property_name == "ShowSellables")
		{
			if (menu && ShowSellables != WasShowingSellables)
			{
				EXPrint("ExpansionMarketMenuController::PropertyChanged - ShowSellables " + ShowSellables);
				WasShowingSellables = ShowSellables;
				GetExpansionClientSettings().MarketMenuFilterSellableState = ShowSellables;
				menu.SetFilterSellables(ShowSellables);
				menu.UpdateOptionFilterStrings();
				menu.UpdateMarketCategories();
			}
		}
		else if (property_name == "ShowPurchasables")
		{
			if (menu && ShowPurchasables != WasShowingPurchasables)
			{
				EXPrint("ExpansionMarketMenuController::PropertyChanged - ShowPurchasables " + ShowPurchasables);
				WasShowingPurchasables = ShowPurchasables;
				GetExpansionClientSettings().MarketMenuFilterPurchasableState = ShowPurchasables;
				menu.SetFilterPurchasables(ShowPurchasables);
				menu.UpdateOptionFilterStrings();
				menu.UpdateMarketCategories();
			}
		}
		else if (property_name == "IncludeAttachments")
		{
			if (menu && IncludeAttachments != WasIncludingAttachments)
			{
				EXPrint("ExpansionMarketMenuController::PropertyChanged - IncludeAttachments " + IncludeAttachments);
				WasIncludingAttachments = IncludeAttachments;
				menu.OnAttachmentsCheckboxStateChange();
			}
		}
		
	#ifdef EXPANSIONMODMARKET_DEBUG
		EXPrint("ExpansionMarketMenuController::PropertyChanged - End");
	#endif
	}
};