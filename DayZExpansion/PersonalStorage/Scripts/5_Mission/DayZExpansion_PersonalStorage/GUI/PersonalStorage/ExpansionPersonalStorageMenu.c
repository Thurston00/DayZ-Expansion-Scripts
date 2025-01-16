/**
 * ExpansionPersonalStorageMenu.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

enum ExpansionPersonalStorageMenuViewState
{
	NONE = 0,
	ViewPlayerItems = 1,
	ViewDepositedItems = 2,
	DetailViewPlayerItem = 3,
	DetailViewDepositedItem = 4,
};

class ExpansionPersonalStorageMenu: ExpansionScriptViewMenu
{
	protected ref ExpansionPersonalStorageMenuController m_PersonalStorageMenuController;
	protected ref array<ref ExpansionPersonalStorageItem> m_PlayerItems;

	protected ExpansionPersonalStorageMenuViewState m_ViewState;
	protected ExpansionPersonalStorageMenuViewState m_PreviousViewState;

	protected ExpansionPersonalStorageMenuItem m_SelectedPlayerItem;
	Object m_SelectedPreviewObject;
	protected ref array<ref ExpansionPersonalStorageContainerItem> m_SelectedContainerItems;

	protected ref ExpansionPersonalStorageMenuDetailsView m_ItemDetailsView;
	protected ref ExpansionPersonalStorageMenuBrowseHeader m_BrowseHeader;

	protected ref TStringArray m_BloodContainers = {"BloodContainerBase"};

	protected Widget categories_panel;
	protected Widget inventory_panel;
	protected Widget details_panel;
	protected ScrollWidget inventory_scroller;
	protected WrapSpacerWidget inventory_header;
	protected WrapSpacerWidget inventory_content;
	protected ScrollWidget player_items_scroller;
	protected WrapSpacerWidget player_items_content;

	protected ButtonWidget browse_category_all_button;
	protected TextWidget browse_category_all_text;
	protected ImageWidget browse_category_all_icon;
	protected TextWidget browse_category_all_items_count;

	protected GridSpacerWidget tabs_panel;
	protected ButtonWidget tabs_button_browse;
	protected TextWidget tabs_button_browse_text;
	protected ButtonWidget tabs_button_deposit;
	protected TextWidget tabs_button_deposit_text;

	protected ButtonWidget exit_button;
	protected TextWidget exit_button_text;
	
	protected ButtonWidget toggle_categories_button;
	protected TextWidget toggle_categories_text;

	protected Widget market_filter_panel;
	protected EditBoxWidget market_filter_box;

	protected WrapSpacerWidget browse_categories_content;
	protected WrapSpacerWidget player_categories_content;
	protected Widget loading;
	
	protected GridSpacerWidget toggle_categories_panel;

	protected string m_PlayerUID;
	protected int m_StorageID = -1;
	protected bool m_RequestsLocked;

	protected ref ExpansionPersonalStorageSettings m_PersonalStorageSettings;

	protected bool m_CategoriesToggleState;

	protected bool m_IsPersonalStorageContainer;
	protected ExpansionPersonalStorageContainer m_Container;
	protected ExpansionPersonalStorageHub m_ContainerHub;

	void ExpansionPersonalStorageMenu()
	{
		Class.CastTo(m_PersonalStorageMenuController, GetController());
		ExpansionPersonalStorageModule.GetModuleInstance().GetPersonalStorageMenuCallbackSI().Insert(OnModuleCallback);
		ExpansionPersonalStorageModule.GetModuleInstance().GetPersonalStorageMenuSI().Insert(SetDepositedItems);
		ExpansionPersonalStorageContainer.SI_Expansion_OpenPersonalStorageMenu.Insert(ExSetDepositedItems);
		m_PersonalStorageSettings = GetExpansionSettings().GetPersonalStorage();
		m_BrowseHeader = new ExpansionPersonalStorageMenuBrowseHeader(this);
		m_PlayerUID = GetGame().GetPlayer().GetIdentity().GetId();
	}

	void ~ExpansionPersonalStorageMenu()
	{
		ExpansionPersonalStorageModule.GetModuleInstance().GetPersonalStorageMenuCallbackSI().Remove(OnModuleCallback);
		ExpansionPersonalStorageModule.GetModuleInstance().GetPersonalStorageMenuSI().Remove(SetDepositedItems);
		ExpansionPersonalStorageContainer.SI_Expansion_OpenPersonalStorageMenu.Remove(ExSetDepositedItems);
		
		if (m_BrowseHeader)
			m_BrowseHeader.Destroy();

		if (m_ItemDetailsView)
			m_ItemDetailsView.Destroy();
		
		if (m_PersonalStorageMenuController.DepositedItems)
			m_PersonalStorageMenuController.DepositedItems.Clear();
		
		if (m_PersonalStorageMenuController.PlayerItems)
			m_PersonalStorageMenuController.PlayerItems.Clear();
		
		if (m_PersonalStorageMenuController.BrowseCategories)
			m_PersonalStorageMenuController.BrowseCategories.Clear();
		
		if (m_PersonalStorageMenuController.PlayerCategories)
			m_PersonalStorageMenuController.PlayerCategories.Clear();
	}

	override string GetLayoutFile()
	{
		return "DayZExpansion/PersonalStorage/GUI/layouts/expansion_personal_storage_menu.layout";
	}

	override typename GetControllerType()
	{
		return ExpansionPersonalStorageMenuController;
	}
	
	void ExSetDepositedItems(ExpansionPersonalStorageContainer container)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.PERSONALSTORAGE, this);
#endif
		
		m_Container = container;
		m_ContainerHub = container.Expansion_GetPersonalStorageHub();
		
		int itemsCount;
		
		m_PersonalStorageMenuController.PersonalStorageName = container.GetDisplayName();
		m_PersonalStorageMenuController.NotifyPropertyChanged("PersonalStorageName");
		
		if (m_PersonalStorageMenuController.DepositedItems)
			m_PersonalStorageMenuController.DepositedItems.Clear();

		inventory_header.AddChild(m_BrowseHeader.GetLayoutRoot());
		m_BrowseHeader.SetSort(0, false);
		
		array<EntityAI> items = new array<EntityAI>;
		container.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);
		if (items && items.Count() > 0)
		{
			foreach (EntityAI item: items)
			{
				if (item.GetHierarchyParent() && item.GetHierarchyParent().IsInherited(ExpansionPersonalStorageContainer))
				{
					//! Hardcoded excluded type names where the item should never get added and shown in the menu.
					if (ExpansionPersonalStorageModule.m_HardcodedExcludes.Find(item.GetType()) > -1)
						continue;
					
					ExpansionPersonalStorageItem newDepositedItem = new ExpansionPersonalStorageItem();
					newDepositedItem.SetFromItem(item, GetGame().GetPlayer().GetIdentity().GetId());
					newDepositedItem.SetIsStored(true);
								
					ExpansionPersonalStorageMenuItem storedItem = new ExpansionPersonalStorageMenuItem(newDepositedItem, this);
					m_PersonalStorageMenuController.DepositedItems.Insert(storedItem);
					itemsCount++;
				}
			}
		}
		
		m_PersonalStorageMenuController.BrowseCategories.Clear();

		array<ref ExpansionPersonalStorageMenuCategory> menuCategories = m_PersonalStorageSettings.MenuCategories;
		foreach (ExpansionPersonalStorageMenuCategory category: menuCategories)
		{
			ExpansionPersonalStorageMenuCategoryElement categoryElement = new ExpansionPersonalStorageMenuCategoryElement(this, category);
			m_PersonalStorageMenuController.BrowseCategories.Insert(categoryElement);
		}

		//! Get and set all deposited items count
		string itemsCountText = "[0]";
		if (itemsCount > 0)
			itemsCountText = "[" + itemsCount + "]";

		m_PersonalStorageMenuController.AllItemsCount = itemsCount.ToString();
		m_PersonalStorageMenuController.NotifyPropertyChanged("AllItemsCount");
		
		m_PersonalStorageMenuController.StoredItemsCount = itemsCount.ToString();
		m_PersonalStorageMenuController.NotifyPropertyChanged("StoredItemsCount");
		
		m_PreviousViewState = m_ViewState;
		m_ViewState = ExpansionPersonalStorageMenuViewState.ViewDepositedItems;
		
		loading.Show(false);

		m_IsPersonalStorageContainer = true;
	}

	void SetDepositedItems(int storageID, array<ref ExpansionPersonalStorageItem> depositedItems = null, string displayName = string.Empty, string displayIcon = string.Empty)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.PERSONALSTORAGE, this);
#endif
		
		int itemsCount;
		
		if (displayName != string.Empty)
		{
			m_PersonalStorageMenuController.PersonalStorageName = displayName;
			m_PersonalStorageMenuController.NotifyPropertyChanged("PersonalStorageName");
		}
		
		if (displayIcon != string.Empty)
		{
			string iconPath = ExpansionIcons.GetPath(displayIcon);
			if (iconPath == string.Empty)
				iconPath = displayIcon;
			
			m_PersonalStorageMenuController.PersonalStorageIcon = iconPath;
			m_PersonalStorageMenuController.NotifyPropertyChanged("PersonalStorageIcon");
		}
		
		m_StorageID = storageID;
		
		m_PersonalStorageMenuController.DepositedItems.Clear();

		inventory_header.AddChild(m_BrowseHeader.GetLayoutRoot());
		m_BrowseHeader.SetSort(0, false);

		if (depositedItems && depositedItems.Count() > 0)
		{
			itemsCount = depositedItems.Count();
			foreach (ExpansionPersonalStorageItem item: depositedItems)
			{
				ExpansionPersonalStorageMenuItem storedItem = new ExpansionPersonalStorageMenuItem(item, this);
				m_PersonalStorageMenuController.DepositedItems.Insert(storedItem);
			}

			SortListingsByName();
		}
		
		m_PersonalStorageMenuController.BrowseCategories.Clear();

		array<ref ExpansionPersonalStorageMenuCategory> menuCategories = m_PersonalStorageSettings.MenuCategories;
		foreach (ExpansionPersonalStorageMenuCategory category: menuCategories)
		{
			ExpansionPersonalStorageMenuCategoryElement categoryElement = new ExpansionPersonalStorageMenuCategoryElement(this, category);
			m_PersonalStorageMenuController.BrowseCategories.Insert(categoryElement);
		}

		//! Get and set all deposited items count
		string itemsCountText = "[0]";
		if (itemsCount > 0)
			itemsCountText = "[" + itemsCount + "]";

		m_PersonalStorageMenuController.AllItemsCount = itemsCount.ToString();
		m_PersonalStorageMenuController.NotifyPropertyChanged("AllItemsCount");

		int maxItemsPerStorage = m_PersonalStorageSettings.MaxItemsPerStorage;
	#ifdef EXPANSIONMODHARDLINE
		ExpansionPersonalStorageConfig personalStorageConfig = ExpansionPersonalStorageModule.GetModuleInstance().GetPersonalStorageClientConfig();
		if (personalStorageConfig)
		{
			int reputationToUnlock = personalStorageConfig.GetReputation();
			PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
			if (player)
			{
				int reputation = player.Expansion_GetReputation();
				int limit = ExpansionPersonalStorageModule.GetModuleInstance().GetStorageLimitByReputation(reputation, reputationToUnlock);
				maxItemsPerStorage = limit;
			}
		}
	#endif
		
		m_PersonalStorageMenuController.StoredItemsCount = itemsCount.ToString() + "/" + maxItemsPerStorage.ToString();
		m_PersonalStorageMenuController.NotifyPropertyChanged("StoredItemsCount");
		
		m_PreviousViewState = m_ViewState;
		m_ViewState = ExpansionPersonalStorageMenuViewState.ViewDepositedItems;
	}

	void UpdatePlayerItems()
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.PERSONALSTORAGE, this);
#endif
		
		if (!m_PlayerItems)
			m_PlayerItems = new array <ref ExpansionPersonalStorageItem>;
		else
			m_PlayerItems.Clear();

		array<EntityAI> items = ExpansionPersonalStorageModule.GetModuleInstance().LocalGetEntityInventory();
		if (items && items.Count() > 0)
		{
			PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
			foreach (EntityAI item: items)
			{
				if (item.GetHierarchyParent() && item.GetHierarchyParent().IsInherited(SurvivorBase))
				{
					//! Hardcoded excluded type names where the item should never get added and shown in the menu.
					if (ExpansionPersonalStorageModule.m_HardcodedExcludes.Find(item.GetType()) > -1)
						continue;
					
					ExpansionPersonalStorageItem newPlayerItem = new ExpansionPersonalStorageItem();
					newPlayerItem.SetFromItem(item, item.GetHierarchyRootPlayer().GetIdentity().GetId());
					
					if (newPlayerItem.GetContainerItemsCount() > 0 && m_IsPersonalStorageContainer && m_Container)
					{
						if (!m_Container.m_Expansion_PersonalStorageAllowAttachmentCargo)
						{
							newPlayerItem.SetExcluded(true);
						}
					}

					m_PlayerItems.Insert(newPlayerItem);
				}
			}

			m_PersonalStorageMenuController.PlayerItems.Clear();

			array<EntityAI> slotItems = GetSlotItems(player);
			array<string> slotNames = new array<string>;
			slotNames.Insert("All");

			foreach (ExpansionPersonalStorageItem playerItem: m_PlayerItems)
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

				ExpansionPersonalStorageMenuItem playerItemEntry = new ExpansionPersonalStorageMenuItem(playerItem, this);
				m_PersonalStorageMenuController.PlayerItems.Insert(playerItemEntry);
			}

			m_PersonalStorageMenuController.PlayerCategories.Clear();
			foreach (string slotName: slotNames)
			{
				ExpansionPersonalStorageMenuInventoryCategoryElement inventoryCategoryElement = new ExpansionPersonalStorageMenuInventoryCategoryElement(this, slotName);
				m_PersonalStorageMenuController.PlayerCategories.Insert(inventoryCategoryElement);
			}
		}
	}

	void HideItemDetailElements()
	{
		m_ItemDetailsView.ShowQuantityColor(false);
		m_ItemDetailsView.ShowItemQuantity(false);
		m_ItemDetailsView.ShowItemLiquid(false);
		m_ItemDetailsView.ShowItemCargoCount(false);
		m_ItemDetailsView.ShowItemAttachmentsCount(false);
		m_ItemDetailsView.ShowItemFoodState(false);
	#ifdef EXPANSIONMODHARDLINE
		m_ItemDetailsView.ShowItemRarity(false);
	#endif
	}

	void SetSelectedMenuItem(ExpansionPersonalStorageMenuItem item)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.PERSONALSTORAGE, this);
#endif
		
		m_SelectedPlayerItem = item;
	}
	
	void SetSelectedPlayerItem(ExpansionPersonalStorageMenuItem item)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.PERSONALSTORAGE, this);
#endif
		
		if (!item || !item.GetPlayerItem())
			return;
		
		if (!m_ItemDetailsView)
		{
			m_ItemDetailsView = new ExpansionPersonalStorageMenuDetailsView(this);
		}
		else
		{
			m_ItemDetailsView.ResetPreview();
			m_ItemDetailsView.Show();
		}

		m_SelectedPlayerItem = item;
		m_SelectedPreviewObject = item.GetPreviewObject();
		m_SelectedContainerItems = item.GetPlayerItem().GetContainerItems();

		m_ItemDetailsView.SetViewDeposit();

		m_ItemDetailsView.GetDetailsViewController().SelectedName = item.GetPreviewObject().GetDisplayName();
		m_ItemDetailsView.GetDetailsViewController().NotifyPropertyChanged("SelectedName");

		market_filter_panel.Show(false);
		categories_panel.Show(false);
		player_items_scroller.Show(false);
		inventory_panel.Show(false);
		tabs_panel.Show(false);

		m_ItemDetailsView.ShowConfirmButton(true);

		string buttonText;
		if (item.GetPlayerItem().IsStored())
			buttonText = "Retrieve Item";
		else
			buttonText = "Deposit Item";

		m_ItemDetailsView.GetDetailsViewController().ConfirmButtonText = buttonText;
		m_ItemDetailsView.GetDetailsViewController().NotifyPropertyChanged("ConfirmButtonText");

		details_panel.AddChild(m_ItemDetailsView.GetLayoutRoot());
		details_panel.Show(true);

		HideItemDetailElements();

		UpdateItemInfoDamage(item.GetPlayerItem().GetHealthLevel());

		item.GetPlayerItem().UpdateContainerItems();

		if (item.GetPlayerItem().GetLiquidType() != -1)
		{
			bool isBloodContainer = ExpansionStatic.IsAnyOf(item.GetPlayerItem().GetClassName(), m_BloodContainers);
			int liquidColor;
			m_ItemDetailsView.ShowItemLiquid(true);
			m_ItemDetailsView.GetDetailsViewController().LiquidType = GetLiquidTypeString(item.GetPlayerItem().GetLiquidType(), isBloodContainer, liquidColor);
			m_ItemDetailsView.GetDetailsViewController().NotifyPropertyChanged("LiquidType");
			m_ItemDetailsView.SetLiquidColor(liquidColor);
		}

		if (item.GetPlayerItem().GetFoodStageType() != -1)
		{
			int foodStageColor;
			m_ItemDetailsView.ShowItemFoodState(true);
			m_ItemDetailsView.GetDetailsViewController().FoodState = GetFoodStageString(item.GetPlayerItem().GetFoodStageType(), foodStageColor);
			m_ItemDetailsView.GetDetailsViewController().NotifyPropertyChanged("FoodState");
			m_ItemDetailsView.SetFoodStateColor(foodStageColor);
		}

		if (item.GetPlayerItem().GetQuantity() > 0)
		{
			int quantityColor;
			if (item.GetPlayerItem().GetQuantityType() == ExpansionItemQuantityType.POWER || item.GetPlayerItem().GetQuantityType() == ExpansionItemQuantityType.MILLILITER || item.GetPlayerItem().GetQuantityType() == ExpansionItemQuantityType.PERCENTAGE || item.GetPlayerItem().GetQuantityType() == ExpansionItemQuantityType.GRAM)
			{
				m_ItemDetailsView.GetQuantityBarWidget().SetCurrent(item.GetPlayerItem().GetQuantity());
				m_ItemDetailsView.GetDetailsViewController().ItemQuantity = GetQuantityString(item.GetPlayerItem().GetQuantityType(), item.GetPlayerItem().GetQuantity(), quantityColor);
				m_ItemDetailsView.GetDetailsViewController().NotifyPropertyChanged("ItemQuantity");
				m_ItemDetailsView.ShowQuantityBar(true);
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
				
				m_ItemDetailsView.GetDetailsViewController().QuantityText = "Heat-Isolation:";
				m_ItemDetailsView.GetDetailsViewController().NotifyPropertyChanged("QuantityText");
				m_ItemDetailsView.GetDetailsViewController().ItemQuantity = isolationText;
				m_ItemDetailsView.GetDetailsViewController().NotifyPropertyChanged("ItemQuantity");
				m_ItemDetailsView.ShowQuantityColor(true, isolationColor);
			}
			else if (item.GetPlayerItem().GetQuantityType() == ExpansionItemQuantityType.PC)
			{
				m_ItemDetailsView.GetDetailsViewController().QuantityText = "Quantity:";
				m_ItemDetailsView.GetDetailsViewController().NotifyPropertyChanged("QuantityText");
				m_ItemDetailsView.GetDetailsViewController().ItemQuantity = GetQuantityString(item.GetPlayerItem().GetQuantityType(), item.GetPlayerItem().GetQuantity(), quantityColor);
				m_ItemDetailsView.GetDetailsViewController().NotifyPropertyChanged("ItemQuantity");
				m_ItemDetailsView.ShowItemQuantity(true);
			}
		}

	#ifdef EXPANSIONMODHARDLINE
		if (item.GetPlayerItem().GetRarity() != ExpansionHardlineItemRarity.NONE)
		{
			string rarityName = typename.EnumToString(ExpansionHardlineItemRarity, item.GetPlayerItem().GetRarity());
			string rarityText = "#" + "STR_EXPANSION_HARDLINE_" + rarityName;
			int rarityColor;
			typename type = ExpansionHardlineItemRarityColor;
			ExpansionStatic.GetVariableIntByName(type, rarityName, rarityColor);
			int a, r, g, b;
			ExpansionStatic.IntToARGB(rarityColor, a, r, g, b);

			m_ItemDetailsView.GetDetailsViewController().Rarity = rarityText;
			m_ItemDetailsView.GetDetailsViewController().NotifyPropertyChanged("Rarity");
			m_ItemDetailsView.SetRarityColor(rarityColor);

			if (rarityText.Contains("Common"))
			{
				m_ItemDetailsView.SetRarityTextColor(ARGB(255, 0, 0, 0));
			}
			else
			{
				m_ItemDetailsView.SetRarityTextColor(ARGB(255, 255, 255, 255));
			}

			m_ItemDetailsView.ShowItemRarity(true);
		}
	#endif

		m_ItemDetailsView.ClearAttachments();
		m_ItemDetailsView.ClearCargo();

		if (item.GetPlayerItem().GetContainerItemsCount() > 0)
		{
			int cargoItemsCount = 0;
			int attachmentsCount = 0;
			array<ref ExpansionPersonalStorageContainerItem> containerItems = item.GetPlayerItem().GetContainerItems();
			if (containerItems && containerItems.Count() > 0)
			{
				ExpansionPersonalStorageMenuCargoItem containerElement;
				string rarityType;
				for (int i = 0; i < containerItems.Count(); i++)
				{
					ExpansionPersonalStorageContainerItem containerItem = containerItems[i];					
					containerElement = new ExpansionPersonalStorageMenuCargoItem(containerItem, this);
					if (!containerElement)
						continue;

					if (!containerItem.IsAttached())
					{
						m_ItemDetailsView.AddCargoEntry(containerElement);
						cargoItemsCount++;
					}
					else
					{
						m_ItemDetailsView.AddAttachmentEntry(containerElement);
						attachmentsCount++;
					}

					array<ref ExpansionPersonalStorageContainerItem> containerItemsOfContainerItem = containerItem.GetContainerItems();
					if (containerItemsOfContainerItem && containerItemsOfContainerItem.Count() > 0)
					{
						foreach (ExpansionPersonalStorageContainerItem containerItemOfContainerItem: containerItemsOfContainerItem)
						{
							containerElement = new ExpansionPersonalStorageMenuCargoItem(containerItemOfContainerItem, this);
							
							if (!containerItemOfContainerItem.IsAttached())
							{
								m_ItemDetailsView.AddCargoEntry(containerElement);
								cargoItemsCount++;
							}
							else
							{
								m_ItemDetailsView.AddAttachmentEntry(containerElement);
								attachmentsCount++;
							}
						}
					}
				}
			}
			
			if (cargoItemsCount > 0)
			{
				m_ItemDetailsView.ShowItemCargoCount(true);
				m_ItemDetailsView.GetDetailsViewController().CargoItemsCount = cargoItemsCount.ToString();
				m_ItemDetailsView.GetDetailsViewController().NotifyPropertyChanged("CargoItemsCount");
			}
			
			if (attachmentsCount > 0)
			{
				m_ItemDetailsView.ShowItemAttachmentsCount(true);
				m_ItemDetailsView.GetDetailsViewController().AttachmentItemsCount = attachmentsCount.ToString();
				m_ItemDetailsView.GetDetailsViewController().NotifyPropertyChanged("AttachmentItemsCount");
			}
		}

		m_ItemDetailsView.UpdatePreview();

		m_PreviousViewState = m_ViewState;

		if (item.GetPlayerItem().IsStored())
		{
			m_ViewState = ExpansionPersonalStorageMenuViewState.DetailViewDepositedItem;
		}
		else
		{
			m_ViewState = ExpansionPersonalStorageMenuViewState.DetailViewPlayerItem;
		}

		PlayObjectSound();
	}

	protected void PlayObjectSound()
	{
		if (GetGame().IsClient() || !GetGame().IsMultiplayer())
		{
			if (GetGame().GetPlayer())
			{
				string soundConfig;
				string typeName = m_SelectedPlayerItem.GetPreviewObject().GetType();
				string path = "cfgVehicles " + typeName + " " + "AnimEvents ";
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

	protected string GetQuantityString(int quantityType, float quantity, inout int color)
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

	protected string GetFoodStageString(int foodStage, inout int color)
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

	protected string GetLiquidTypeString(int liquid_type, bool isBloodContainer, inout int color)
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
				text = "ERROR";
				color = Colors.COLOR_PRISTINE;
				break;
			}
		}

		m_ItemDetailsView.GetDetailsViewController().ItemHealth = text;
		m_ItemDetailsView.GetDetailsViewController().NotifyPropertyChanged("ItemHealth");
		m_ItemDetailsView.GetHealthImageWidget().SetColor(color | 0x7F000000);
	}

	protected bool ItemCheck(EntityAI item)
	{
		if (!ItemCheckEx(item))
			return false;

		//! Don't add the optic attachment on AUG's.
		if (item.GetType() == "AugOptic")
			return false;
		
		//! Don`t add items that have excuded items in cargo
		array<EntityAI> cargoItems = new array<EntityAI>;
		item.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, cargoItems);
		foreach (EntityAI cargoItem: cargoItems)
		{
			if (!ItemCheckEx(cargoItem))
				return false;
		}

		return true;
	}

	protected bool ItemCheckEx(EntityAI item)
	{
		if (ExpansionStatic.IsAnyOf(item, GetExpansionSettings().GetPersonalStorage().ExcludedClassNames))
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

		//! Don`t add any active items.
		if (item.HasEnergyManager())
		{
			if (item.GetCompEM().IsWorking())
				return false;
		}

		return true;
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

	void UpdateInventorySlotFilter(string slotName)
	{
		for (int i = 0; i < m_PersonalStorageMenuController.PlayerItems.Count(); i++)
		{
			ExpansionPersonalStorageMenuItem playerItem = m_PersonalStorageMenuController.PlayerItems[i];
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
			return m_PersonalStorageMenuController.PlayerItems.Count();
		}
		else
		{
			for (int i = 0; i < m_PersonalStorageMenuController.PlayerItems.Count(); i++)
			{
				ExpansionPersonalStorageMenuItem playerItem = m_PersonalStorageMenuController.PlayerItems[i];
				if (playerItem.GetPlayerItem().GetSlotName() == slotName)
					count++;
			}
		}

		return count;
	}

	void OnBackClick()
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.PERSONALSTORAGE, this);
#endif
		
		ClearSelected();

		if (m_ViewState == ExpansionPersonalStorageMenuViewState.DetailViewPlayerItem)
		{
			OnDepositButtonClick();
		}
		else if (m_ViewState == ExpansionPersonalStorageMenuViewState.DetailViewDepositedItem)
		{
			OnBrowseButtonClick();
		}
	}

	void OnDepositButtonClick()
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.PERSONALSTORAGE, this, typename.EnumToString(ExpansionPersonalStorageMenuViewState, m_ViewState));
#endif

		if (m_ViewState == ExpansionPersonalStorageMenuViewState.ViewPlayerItems)
		{
			m_BrowseHeader.Hide();
			tabs_panel.Show(false);
		}
			
		ClearSelected();
		
		if (m_ItemDetailsView)
		{
			m_ItemDetailsView.UpdatePreview();
			m_ItemDetailsView.Hide();
		}

		categories_panel.Show(true);
		inventory_panel.Show(true);
		player_items_scroller.Show(true);
		details_panel.Show(false);
		
		ShowDepositView();
	}
	
	void OnBrowseButtonClick()
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.PERSONALSTORAGE, this);
#endif

		ClearSelected();
		
		if (m_ItemDetailsView)
		{
			m_ItemDetailsView.UpdatePreview();
			m_ItemDetailsView.Hide();
		}

		categories_panel.Show(true);
		inventory_panel.Show(true);
		player_items_scroller.Show(false);
		details_panel.Show(false);

		ShowBrowseView();
	}

	protected void ShowDepositView()
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.PERSONALSTORAGE, this);
#endif
				
		tabs_panel.Show(true);
		market_filter_panel.Show(true);
		categories_panel.Show(true);
		player_items_scroller.Show(true);
		player_categories_content.Show(true);

		inventory_scroller.Show(false);
		toggle_categories_panel.Show(false);
		browse_categories_content.Show(false);
		toggle_categories_panel.Show(false);
		
		ExpansionPersonalStorageModule.GetModuleInstance().EnumeratePlayerInventory(PlayerBase.Cast(GetGame().GetPlayer()));
		UpdatePlayerItems();
	
		m_PreviousViewState = m_ViewState;
		m_ViewState = ExpansionPersonalStorageMenuViewState.ViewPlayerItems;
	}

	protected void ShowBrowseView()
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.PERSONALSTORAGE, this);
#endif
				
		m_BrowseHeader.Show();
		tabs_panel.Show(true);
		market_filter_panel.Show(true);
		categories_panel.Show(true);
		inventory_scroller.Show(true);
		browse_categories_content.Show(true);
		toggle_categories_panel.Show(true);
		
		player_items_scroller.Show(false);
		player_categories_content.Show(false);
		toggle_categories_panel.Show(true);
		
		if (m_IsPersonalStorageContainer && m_Container)
		{
			ExSetDepositedItems(m_Container);
		}
		else
		{
			m_PreviousViewState = m_ViewState;
			m_ViewState = ExpansionPersonalStorageMenuViewState.ViewDepositedItems;
		}
	}
	
	protected void SetCategory_All()
	{
		for (int i = 0; i < m_PersonalStorageMenuController.DepositedItems.Count(); i++)
		{
			ExpansionPersonalStorageMenuItem entry = m_PersonalStorageMenuController.DepositedItems[i];
			if (!entry || !entry.GetPlayerItem())
				continue;

			entry.Show();
		}
	}
	
	void OnClickCategory_All()
	{
		SetCategory_All();
	}

	void OnConfirmButtonClick()
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.PERSONALSTORAGE, this);
#endif
		
		if (m_RequestsLocked)
		{
			ModuleDebugPrint("::OnConfirmButtonClick - Request locked! Return..");
			return;
		}

		if (!m_SelectedPlayerItem.GetPlayerItem().IsStored())
		{
			OnConfirmDepositClick();
		}
		else
		{
			OnConfirmRetrieveClick();
		}
	}

	void OnConfirmDepositClick()
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.PERSONALSTORAGE, this);
#endif
		
		if (!m_SelectedPlayerItem || !m_SelectedPlayerItem.GetPlayerItem() || m_RequestsLocked)
		{
			ModuleDebugPrint("::OnConfirmDepositClick - Request is locked or could not get selected player item! Return..");
			return;
		}

		EntityAI playerItem = m_SelectedPlayerItem.GetPlayerItem().GetItem();
		if (!playerItem)
		{
			ModuleDebugPrint("::OnConfirmDepositClick - Could not get player item! Return..");
			return;
		}
				
		if (!m_IsPersonalStorageContainer)
		{		
			ExpansionPersonalStorageModule.GetModuleInstance().RequestDepositItem(m_StorageID, playerItem);
		}
		else
		{
			m_Container.Expansion_RequestMoveItem(playerItem);
			GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(OnModuleCallback, 200, false, ExpansionPersonalStorageModuleCallback.ItemStored);
		}
		
		loading.Show(true);
		m_RequestsLocked = true;
	}

	void OnConfirmRetrieveClick()
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.PERSONALSTORAGE, this);
#endif
		
		if (!m_SelectedPlayerItem || !m_SelectedPlayerItem.GetPlayerItem() || m_RequestsLocked)
		{
			ModuleDebugPrint("::OnConfirmRetrieveClick - Request is locked or could not get selected player item! Return..");
			return;
		}
				
		if (!m_IsPersonalStorageContainer)
		{			
			ExpansionPersonalStorageModule.GetModuleInstance().RequestRetrieveItem(m_SelectedPlayerItem.GetPlayerItem(), m_StorageID);
		}
		else
		{
			EntityAI playerItem = m_SelectedPlayerItem.GetPlayerItem().GetItem();
			if (!playerItem)
			{
				ModuleDebugPrint("::OnConfirmRetrieveClick - Could not get player item! Return..");
				return;
			}

			m_Container.Expansion_RequestMoveItem(playerItem);
			GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(OnModuleCallback, 200, false, ExpansionPersonalStorageModuleCallback.ItemRetrieved);
		}
		
		loading.Show(true);
		m_RequestsLocked = true;
	}

	protected void ClearSelected()
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.PERSONALSTORAGE, this);
#endif
		
		m_SelectedPlayerItem = null;
		m_SelectedPreviewObject = null;
		m_SelectedContainerItems = null;

		if (!m_ItemDetailsView)
			return;

		m_ItemDetailsView.ShowConfirmButton(false);

		if (m_PreviousViewState == ExpansionPersonalStorageMenuViewState.DetailViewPlayerItem || m_PreviousViewState == ExpansionPersonalStorageMenuViewState.DetailViewDepositedItem)
		{
			m_ItemDetailsView.GetDetailsViewController().SelectedName = "";
			m_ItemDetailsView.GetDetailsViewController().NotifyPropertyChanged("SelectedName");
		}
	}

	protected void OnSearchFilterChange()
	{
		if (m_ViewState == ExpansionPersonalStorageMenuViewState.ViewPlayerItems)
		{
			SearchInPlayerItems();
		}
		else if (m_ViewState == ExpansionPersonalStorageMenuViewState.ViewDepositedItems)
		{
			SearchInDepositedItems();
		}
	}
	
	protected void SearchInDepositedItems()
	{
		if (!m_PersonalStorageMenuController.DepositedItems || m_PersonalStorageMenuController.DepositedItems.Count() == 0)
			return;

		bool hasChildWithName;
		string displayName;
		string searchText = market_filter_box.GetText();
		searchText.ToLower();

		for (int i = 0; i < m_PersonalStorageMenuController.DepositedItems.Count(); i++)
		{
			ExpansionPersonalStorageMenuItem entry = m_PersonalStorageMenuController.DepositedItems[i];
			if (entry && entry.GetPreviewObject())
			{
				if (!entry.IsVisible() || searchText == string.Empty)
					entry.Show();
				
				hasChildWithName = false;					
				ObservableCollection<ref ExpansionPersonalStorageMenuItem> cargoEntries = entry.GetCargoItemElemens();
				if (cargoEntries && cargoEntries.Count() > 0)
				{
					for (int k = 0; k < cargoEntries.Count(); k++)
					{
						ExpansionPersonalStorageMenuItem cargoEntry = cargoEntries[k];
						if (!cargoEntry.IsVisible() || searchText == string.Empty)
							cargoEntry.Show();

						displayName = cargoEntry.GetPreviewObject().GetDisplayName();
						displayName.ToLower();
						
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
				
				displayName = entry.GetPreviewObject().GetDisplayName();
				displayName.ToLower();
				
				if (displayName.IndexOf(searchText) == -1 && !hasChildWithName)
					entry.Hide();
			}
		}
	}

	protected void SearchInPlayerItems()
	{
		if (!m_PersonalStorageMenuController.PlayerItems || m_PersonalStorageMenuController.PlayerItems.Count() == 0)
			return;

		bool hasChildWithName;
		string displayName;
		string searchText = market_filter_box.GetText();
		searchText.ToLower();

		for (int i = 0; i < m_PersonalStorageMenuController.PlayerItems.Count(); i++)
		{
			ExpansionPersonalStorageMenuItem entry = m_PersonalStorageMenuController.PlayerItems[i];
			if (entry && entry.GetPreviewObject())
			{
				if (!entry.IsVisible() || searchText == string.Empty)
					entry.Show();
				
				hasChildWithName = false;					
				ObservableCollection<ref ExpansionPersonalStorageMenuItem> cargoEntries = entry.GetCargoItemElemens();
				if (cargoEntries && cargoEntries.Count() > 0)
				{
					for (int k = 0; k < cargoEntries.Count(); k++)
					{
						ExpansionPersonalStorageMenuItem cargoEntry = cargoEntries[k];
						if (!cargoEntry.IsVisible() || searchText == string.Empty)
							cargoEntry.Show();

						displayName = cargoEntry.GetPreviewObject().GetDisplayName();
						displayName.ToLower();
						
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
				
				displayName = entry.GetPreviewObject().GetDisplayName();
				displayName.ToLower();
				
				if (displayName.IndexOf(searchText) == -1 && !hasChildWithName)
					entry.Hide();
			}
		}
	}

	void OnModuleCallback(int callback)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.PERSONALSTORAGE, this, typename.EnumToString(ExpansionPersonalStorageModuleCallback, callback));
#endif 

		switch (callback)
		{
			case ExpansionPersonalStorageModuleCallback.ItemStored:
			{
				m_RequestsLocked = false;
				loading.Show(false);
				
				OnDepositButtonClick();
				
				if (m_ItemDetailsView)
				{
					m_ItemDetailsView.Hide();
				}
			}
			break;
			case ExpansionPersonalStorageModuleCallback.ItemRetrieved:
			{				
				m_RequestsLocked = false;
				loading.Show(false);
				
				OnBrowseButtonClick();
				
				if (m_ItemDetailsView)
				{
					m_ItemDetailsView.Hide();
				}
			}
			break;
			case ExpansionPersonalStorageModuleCallback.Error:
			{
				ExpansionPersonalStorageModule.GetModuleInstance().EnumeratePlayerInventory(PlayerBase.Cast(GetGame().GetPlayer()));
				UpdatePlayerItems();
				
				OnBackClick();
				
				loading.Show(false);
				m_RequestsLocked = false;
			}
			break;
		}
	}
	
	void OnExitClick()
	{
		Hide();
		CloseMenu();
	}

	void OnToggleCategoriesClick()
	{
		m_CategoriesToggleState = !m_CategoriesToggleState;
		for (int i = 0; i < m_PersonalStorageMenuController.BrowseCategories.Count(); i++)
		{
			ExpansionPersonalStorageMenuCategoryElement categoryElement = m_PersonalStorageMenuController.BrowseCategories[i];
			if (!categoryElement)
				continue;

			categoryElement.ShowSubCategories(m_CategoriesToggleState);
		}
	}

	override void OnShow()
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.PERSONALSTORAGE, this);
#endif
		
		super.OnShow();
		
		SetFocus(GetLayoutRoot());
		ShowBrowseView();
	}

	override void OnHide()
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.PERSONALSTORAGE, this);
#endif
		
		if (!GetGame())
			return;

		if (m_ItemDetailsView)
			m_ItemDetailsView.Destroy();

		if (m_BrowseHeader)
			m_BrowseHeader.Destroy();
		
		if (m_IsPersonalStorageContainer && m_ContainerHub)
		{
			PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
			
			if (player)
			{
				ActionManagerClient mngr_client = ActionManagerClient.Cast(player.GetActionManager());
				ActionTarget atrg = new ActionTarget(m_ContainerHub, null, -1, m_ContainerHub.GetPosition(), -1.0);
				if (mngr_client.GetAction(ExpansionActionClosePersonalStorage).Can(player, atrg, null))
				{
					mngr_client.PerformActionStart(player.GetActionManager().GetAction(ExpansionActionClosePersonalStorage) ,atrg, null);
				}
			}
		}

		super.OnHide();
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
			if (w == exit_button)
			{
				exit_button.SetColor(ARGB(255, 255, 255, 255));
				exit_button_text.SetColor(ARGB(255, 0, 0, 0));
			}
			else if (w == browse_category_all_button)
			{
				browse_category_all_button.SetColor(ARGB(255, 255, 255, 255));
				browse_category_all_text.SetColor(ARGB(255, 0, 0, 0));
				browse_category_all_icon.SetColor(ARGB(255, 0, 0, 0));
				browse_category_all_items_count.SetColor(ARGB(255, 0, 0, 0));
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
			if (w == exit_button)
			{
				exit_button.SetColor(ARGB(255, 0, 0, 0));
				exit_button_text.SetColor(ARGB(255, 255, 255, 255));
			}
			else if (w == browse_category_all_button)
			{
				browse_category_all_button.SetColor(ARGB(255, 0, 0, 0));
				browse_category_all_text.SetColor(ARGB(255, 255, 255, 255));
				browse_category_all_icon.SetColor(ARGB(255, 255, 255, 255));
				browse_category_all_items_count.SetColor(ARGB(255, 255, 255, 255));
			}
			else if (w == toggle_categories_button)
			{
				toggle_categories_button.SetColor(ARGB(255, 0, 0, 0));
				toggle_categories_text.SetColor(ARGB(255, 255, 255, 255));
			}
		}

		return false;
	}

	void SortListingsByName(bool reverse = false)
	{
		TStringArray displayName = new TStringArray;
		
		for (int i = 0; i < m_PersonalStorageMenuController.DepositedItems.Count(); i++)
		{
			ExpansionPersonalStorageMenuItem entry = m_PersonalStorageMenuController.DepositedItems[i];
			if (!entry || !entry.GetPlayerItem())
				continue;

			displayName.Insert(entry.GetPreviewObject().GetDisplayName());
		}

		displayName.Sort(reverse);

		for (int j = 0; j < m_PersonalStorageMenuController.DepositedItems.Count(); j++)
		{
			ExpansionPersonalStorageMenuItem currentEntry = m_PersonalStorageMenuController.DepositedItems[j];
			if (!currentEntry || !currentEntry.GetPlayerItem())
				continue;

			string name = currentEntry.GetPreviewObject().GetDisplayName();
			int index = displayName.Find(name) + 1;
			currentEntry.SetSort(index, false);
		}
	}

	void Browse_Filter_ClassNameAZ()
	{
		SortListingsByName();
	}

	void Browse_Filter_ClassNameZA()
	{
		SortListingsByName(true);
	}

	void UpdateMenuCategory(ExpansionPersonalStorageMenuCategoryBase category)
	{
		for (int i = 0; i < m_PersonalStorageMenuController.DepositedItems.Count(); i++)
		{
			ExpansionPersonalStorageMenuItem entry = m_PersonalStorageMenuController.DepositedItems[i];
			if (!entry || !entry.GetPlayerItem())
				continue;

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

	int GetCategoryItemsCount(ExpansionPersonalStorageMenuCategoryBase category)
	{
		int count;
		for (int i = 0; i < m_PersonalStorageMenuController.DepositedItems.Count(); i++)
		{
			ExpansionPersonalStorageMenuItem entry = m_PersonalStorageMenuController.DepositedItems[i];
			if (!entry || !entry.GetPlayerItem())
				continue;

			TStringArray included = category.GetIncluded();
			TStringArray excluded = category.GetExcluded();
			if (ExpansionStatic.IsAnyOf(entry.GetPreviewObject(), included) && !ExpansionStatic.IsAnyOf(entry.GetPreviewObject(), excluded))
				count++;
		}

		return count;
	}

	ExpansionPersonalStorageMenuDetailsView GetDetailsView()
	{
		return m_ItemDetailsView;
	}

	Object GetSelectedPreviewObject()
	{
		return m_SelectedPreviewObject;
	}

	array<ref ExpansionPersonalStorageContainerItem> GetSelectedContainerItems()
	{
		return m_SelectedContainerItems;
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
	
	void ModuleDebugPrint(string text)
	{
	#ifdef EXPANSIONMODPERSONALSTORAGEDEBUG
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.PERSONALSTORAGE, this, text);
#endif
	#endif
	}
};

class ExpansionPersonalStorageMenuController: ExpansionViewController
{
	string AllItemsCount;
	string StoredItemsCount;
	string PersonalStorageName;
	string PersonalStorageIcon;
	ref ObservableCollection<ref ExpansionPersonalStorageMenuItem> DepositedItems = new ObservableCollection<ref ExpansionPersonalStorageMenuItem>(this);
	ref ObservableCollection<ref ExpansionPersonalStorageMenuItem> PlayerItems = new ObservableCollection<ref ExpansionPersonalStorageMenuItem>(this);
	ref ObservableCollection<ref ExpansionPersonalStorageMenuCategoryElement> BrowseCategories = new ObservableCollection<ref ExpansionPersonalStorageMenuCategoryElement>(this);
	ref ObservableCollection<ref ExpansionPersonalStorageMenuInventoryCategoryElement> PlayerCategories = new ObservableCollection<ref ExpansionPersonalStorageMenuInventoryCategoryElement>(this);
};