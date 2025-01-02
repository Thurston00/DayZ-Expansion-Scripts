/**
 * ExpansionMarketMenuItemManager.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

class ExpansionMarketMenuItemManager: ExpansionScriptView
{
	protected ref ExpansionMarketMenuItemManagerController m_MarketItemManagerController;
	protected ExpansionMarketMenu m_MarketMenu;
	protected ExpansionMarketItem m_MarketItem;
	
	protected ScrollWidget market_item_manager_category_items_scroller;
	
	protected ItemPreviewWidget market_item_manager_preview;
	protected ButtonWidget market_item_manager_back_button;
	protected ImageWidget market_item_manager_back_button_icon;
	
	protected ButtonWidget button_close;
	protected TextWidget button_close_label;
	protected ButtonWidget button_reset;
	protected TextWidget button_reset_label;
	protected ButtonWidget button_save;
	
	protected ImageWidget market_item_manager_header;
	protected TextWidget market_item_manager_header_text;
	protected ImageWidget market_item_manager_options_background
	
	protected ButtonWidget preset_selector;
	protected ScrollWidget preset_selector_container;
	protected ImageWidget preset_selector_collapse_image;
	protected ImageWidget preset_selector_expand_image;
	protected Widget preset_selector_background;
	protected Widget preset_editbox_background;
	protected EditBoxWidget preset_editbox;
	
	protected ButtonWidget preset_editbox_clear;
	protected ImageWidget market_filter_clear_icon;
	
	protected WrapSpacerWidget preset_selector_content;
		
	protected vector m_ItemOrientation;
	protected int m_ItemRotationX;
	protected int m_ItemRotationY;
	protected int m_ItemScaleDelta;
	
	protected ref array<ref ExpansionMarketMenuItemManagerPreset> m_ItemManagerPresets;
	protected ref array<ref ExpansionMarketMenuItemManagerPresetElement> m_PresetsDropdownElements;
	
	void ExpansionMarketMenuItemManager(ExpansionMarketMenu marketMenu)
	{
		m_MarketMenu = marketMenu;
		
		if (!m_MarketItemManagerController)
			m_MarketItemManagerController = ExpansionMarketMenuItemManagerController.Cast(GetController());
		
		market_item_manager_header.SetColor(GetExpansionSettings().GetMarket().MarketMenuColors.Get("BaseColorHeaders"));
		market_item_manager_header_text.SetColor(GetExpansionSettings().GetMarket().MarketMenuColors.Get("BaseColorText"));
		market_item_manager_options_background.SetColor(GetExpansionSettings().GetMarket().MarketMenuColors.Get("BaseColorHeaders"));
		
		button_close_label.SetColor(GetExpansionSettings().GetMarket().MarketMenuColors.Get("BaseColorText"));
		button_reset_label.SetColor(GetExpansionSettings().GetMarket().MarketMenuColors.Get("BaseColorText"));
		preset_selector_background.SetColor(GetExpansionSettings().GetMarket().MarketMenuColors.Get("BaseColorHeaders"));
		preset_editbox_background.SetColor(GetExpansionSettings().GetMarket().MarketMenuColors.Get("BaseColorLabels"));
		
		m_ItemManagerPresets = new array<ref ExpansionMarketMenuItemManagerPreset>;
		m_PresetsDropdownElements = new array<ref ExpansionMarketMenuItemManagerPresetElement>;
	}
	
	override string GetLayoutFile() 
	{
		return "DayZExpansion/Market/GUI/layouts/market/expansion_market_menu_item_manager.layout";
	}
	
	override typename GetControllerType() 
	{
		return ExpansionMarketMenuItemManagerController;
	}
	
	void SetMarketItem(ExpansionMarketItem marketItem)
	{
		m_MarketItem = marketItem;
		UpdatePreview();
		m_MarketMenu.UpdateItemFieldFromBasicNetSync();
		UpdateMainView();
		LoadLocalItemPresets();
		preset_selector_container.VScrollToPos(0);
		m_MarketItemManagerController.PresetName = "";
		m_MarketItemManagerController.NotifyPropertyChanged("PresetName");
	}
	
	void UpdatePreview()
	{
		if (m_MarketMenu.GetSelectedMarketItemElement())
		{
			m_MarketMenu.GetSelectedMarketItemElement().UpdatePreviewObject();
		}
		
		if (m_MarketMenu.GetSelectedMarketItemElement())
		{
			m_MarketMenu.UpdatePreview();
		}

		RefreshPreview();
	}

	void RefreshPreview()
	{
		GetMarketItemManagerController().ItemPreview = m_MarketMenu.GetSelectedMarketItemElement().GetPreviewObject();
		GetMarketItemManagerController().NotifyPropertyChanged("ItemPreview");
	}
	
	void UpdateMainView()
	{
		m_MarketItemManagerController.ItemCategories.Clear();

		RefreshPreview();
				
		auto previewObject = m_MarketMenu.GetSelectedMarketItemElement().GetPreviewObject();
		string className = previewObject.GetType();
		m_MarketItemManagerController.ItemName = previewObject.GetDisplayName();
		m_MarketItemManagerController.NotifyPropertyChanged("ItemName");
			
		if (previewObject.IsWeapon())
		{
			AddCategory(className, "#STR_CfgMagazine0", {ExpansionMarketAttachType.MAGAZINE}, "magazine");
			AddCategory(className, "#STR_CfgWeaponOptics0", {ExpansionMarketAttachType.OPTIC}, "weaponoptics");
			AddCategory(className, "#STR_CfgSuppressor0", {ExpansionMarketAttachType.SUPPRESSOR}, "supressor");
			AddCategory(className, "#STR_CfgWeaponWrap0", {ExpansionMarketAttachType.WRAP}, "weaponwrap");
			AddCategory(className, "#STR_EXPANSION_CfgSwitch0", {ExpansionMarketAttachType.SWITCH}, "weaponflashlight");
			AddCategory(className, "#STR_CfgButtstock0", {ExpansionMarketAttachType.BUTTSTOCK}, "buttstock");
			AddCategory(className, "#STR_CfgHandguard0", {ExpansionMarketAttachType.HANDGUARD}, "handguard");
			AddCategory(className, "#STR_CfgBayonet0", {ExpansionMarketAttachType.BAYONET}, "bayonet");
			AddCategory(className, "#STR_EXPANSION_CfgRail0", {ExpansionMarketAttachType.RAIL}, "handguard");
			AddCategory(className, "#STR_CfgFlashlight0", {ExpansionMarketAttachType.LIGHT}, "weaponflashlight");
			AddCategory(className, "#STR_EXPANSION_CfgOther0", {ExpansionMarketAttachType.OTHER}, "shoulderright");
		}
		else if (previewObject.IsClothing())
		{
			AddCategory(className, "#STR_CfgBelt_Left0", {ExpansionMarketAttachType.CANTEENS}, "canteen");
			AddCategory(className, "#STR_CfgVestGrenade0", {ExpansionMarketAttachType.GRENADES}, "grenade");
			AddCategory(className, "#STR_EXPANSION_CfgVestHolster", {ExpansionMarketAttachType.HOLSTERS}, "vestholster");
			AddCategory(className, "#STR_CfgVestPouch0", {ExpansionMarketAttachType.POUCHES}, "vestpouches");
			AddCategory(className, "#STR_EXPANSION_CfgVestPatch", {ExpansionMarketAttachType.PATCHES}, "vest");
			AddCategory(className, "#STR_EXPANSION_CfgOther0", {ExpansionMarketAttachType.OTHER}, "vestholster");
		}
	}
	
	void AddCategory(string className, string title, array<ExpansionMarketAttachType> attachTypes, string icon)
	{
		TStringArray atts = m_MarketMenu.GetMarketFilters().GetAttachmentsByClassNameAndTypes(className, attachTypes);		
		AddCategoryEx(title, atts, icon);
	}
	
	void AddCategoryEx(string title, TStringArray atts, string icon)
	{
		if (atts.Count() > 0)
		{
			ExpansionMarketMenuItemManagerCategory itemCategory = new ExpansionMarketMenuItemManagerCategory(title, atts, this);
			itemCategory.SetIcon("set:dayz_inventory image:" + icon);
			m_MarketItemManagerController.ItemCategories.Insert(itemCategory);
		}
	}

	void SetListView(TStringArray attachments)
	{
		market_item_manager_category_items_scroller.VScrollToPos(0);
		m_MarketItemManagerController.CategoryItems.Clear();
		TStringArray tempAttachments = new TStringArray;
		foreach (string attachment: attachments)
		{
			int index = -1;
			index = tempAttachments.Find(attachment);
			if (index == -1)
				tempAttachments.Insert(attachment);
		}
		
		foreach (string tempAttachment: tempAttachments)
		{
			ExpansionMarketMenuItemManagerCategoryItem attachmentItem = new ExpansionMarketMenuItemManagerCategoryItem(tempAttachment, m_MarketMenu);
			m_MarketItemManagerController.CategoryItems.Insert(attachmentItem);
		}
		
		foreach (string attachmentName: m_MarketMenu.GetSelectedMarketItem().SpawnAttachments)
		{
			for (int i = 0; i < m_MarketItemManagerController.CategoryItems.Count(); i++)
			{
				ExpansionMarketMenuItemManagerCategoryItem categoryItem = m_MarketItemManagerController.CategoryItems[i];
				string attachmentNameToLower = categoryItem.GetItemClassName();
				attachmentNameToLower.ToLower();
				if (categoryItem && attachmentNameToLower == attachmentName)
					categoryItem.UpdateView();
			}
		}
	}
	
	void UpdateListView()
	{
		for (int i = 0; i < m_MarketItemManagerController.CategoryItems.Count(); i++)
		{
			ExpansionMarketMenuItemManagerCategoryItem categoryItem = m_MarketItemManagerController.CategoryItems[i];
			if (categoryItem)
				categoryItem.UpdateView();
		}
	}
	
	void OnSavePresetButtonClick()
	{
		if (m_MarketItemManagerController.PresetName != "")
		{
			ExpansionMarketMenuItemManagerPreset preset = new ExpansionMarketMenuItemManagerPreset;
			if (preset)
			{
				string className = m_MarketMenu.GetSelectedMarketItem().ClassName;
				preset.ClassName = className;
				preset.PresetName = m_MarketItemManagerController.PresetName;
				if (m_MarketMenu.GetSelectedMarketItem().SpawnAttachments.Count() > 0)
				{
					foreach (string attachment: m_MarketMenu.GetSelectedMarketItem().SpawnAttachments)
					{
						preset.AddAttachment(attachment);
					}
					
					bool saved = false;
					if (m_MarketMenu.GetSelectedMarketItemElement().GetPreviewObject().IsWeapon())
					{
						if (!FileExist(EXPANSION_MARKET_WEAPON_PRESETS_FOLDER + className))
							ExpansionStatic.MakeDirectoryRecursive(EXPANSION_MARKET_WEAPON_PRESETS_FOLDER + className);
						
						preset.SaveItemPreset(EXPANSION_MARKET_WEAPON_PRESETS_FOLDER + className + "\\");
						saved = true;
					}
					else if (m_MarketMenu.GetSelectedMarketItemElement().GetPreviewObject().IsClothing())
					{
						if (ExpansionMarketFilters.IsCustomizableClothing(m_MarketMenu.GetSelectedMarketItemElement().GetPreviewObject().GetType()))
						{
							if (!FileExist(EXPANSION_MARKET_CLOTHING_PRESETS_FOLDER + className))
								ExpansionStatic.MakeDirectoryRecursive(EXPANSION_MARKET_CLOTHING_PRESETS_FOLDER + className);
							
							VestCleanup(preset);
							preset.SaveItemPreset(EXPANSION_MARKET_CLOTHING_PRESETS_FOLDER + className + "\\");
							saved = true;
						}
					}
					
					if (saved)
					{
						CreateNotification("STR_EXPANSION_MARKET_PRESETS_SAVED");
						LoadLocalItemPresets();	
						m_MarketItemManagerController.PresetName = "";
						m_MarketItemManagerController.NotifyPropertyChanged("PresetName");
					}
				}
				else
				{
					CreateErrorNotification("STR_EXPANSION_MARKET_PRESETS_ERROR_NO_ATTACHMENTS");
				}
			}
		}
		else
		{
			CreateErrorNotification("STR_EXPANSION_MARKET_PRESETS_ERROR_NAME");
		}
	}
	
	void VestCleanup(ExpansionMarketMenuItemManagerPreset preset)
	{
		string vestFile = EXPANSION_MARKET_VESTS_PRESETS_FOLDER + preset.ClassName + "\\" + preset.PresetName + ".json";
		if (FileExist(vestFile))
			DeleteFile(vestFile);
	}

	void LoadLocalItemPresets()
	{
		string className = m_MarketMenu.GetSelectedMarketItem().ClassName;
		
		m_ItemManagerPresets.Clear();
		if (m_MarketMenu.GetSelectedMarketItemElement().GetPreviewObject().IsWeapon())
		{
			LoadLocalItemPresets(EXPANSION_MARKET_WEAPON_PRESETS_FOLDER + className);
		}
		else if (m_MarketMenu.GetSelectedMarketItemElement().GetPreviewObject().IsClothing())
		{
			if (ExpansionMarketFilters.IsCustomizableClothing(m_MarketMenu.GetSelectedMarketItemElement().GetPreviewObject().GetType()))
			{
				LoadLocalItemPresets(EXPANSION_MARKET_CLOTHING_PRESETS_FOLDER + className);
				if (FileExist(EXPANSION_MARKET_VESTS_PRESETS_FOLDER))
					LoadLocalItemPresets(EXPANSION_MARKET_VESTS_PRESETS_FOLDER + className);
			}
		}
				
		UpdatePresetElements();
	}
	
	void LoadLocalItemPresets(string path)
	{
		if (FileExist(path))
		{
			array<string> files =  ExpansionStatic.FindFilesInLocation(path + "\\", ".json");
			ExpansionMarketMenuItemManagerPreset preset;
			foreach (string fileName: files)
			{
				fileName = fileName.Substring(0, fileName.Length() - 5);
				preset = ExpansionMarketMenuItemManagerPreset.LoadItemPreset(fileName, path + "\\");
				if (preset && preset.ClassName == m_MarketMenu.GetSelectedMarketItem().ClassName)
				{
					if (IsPresetValidCheck(preset))
						m_ItemManagerPresets.Insert(preset);
				}
			}
		}
	}

	void UpdatePresetElements()
	{
		m_PresetsDropdownElements.Clear();
		
		while (preset_selector_content.GetChildren())
		{
			Widget w = preset_selector_content.GetChildren();
			preset_selector_content.RemoveChild(w);
		}
		
		for (int i = 0; i < m_ItemManagerPresets.Count(); i++)
		{
			ExpansionMarketMenuItemManagerPreset preset = m_ItemManagerPresets[i];
			if (preset)
			{
				ExpansionMarketMenuItemManagerPresetElement presetElement = new ExpansionMarketMenuItemManagerPresetElement(preset, this);
				m_PresetsDropdownElements.Insert(presetElement);
				preset_selector_content.AddChild(presetElement.GetLayoutRoot());			
			}
		}
		
		preset_selector_content.Update();
		preset_selector.Show(m_PresetsDropdownElements.Count() > 0);
	}
	
	void OnPresetEditboxClearButtonClick()
	{
		m_MarketItemManagerController.PresetName = "";
		m_MarketItemManagerController.NotifyPropertyChanged("PresetName");
	}
	
	void OnPresetSelectorClick()
	{
		preset_selector_container.VScrollToPos(0);
		preset_selector_container.Show(!preset_selector_container.IsVisible());
		preset_selector_collapse_image.Show(!preset_selector_collapse_image.IsVisible());
		preset_selector_expand_image.Show(!preset_selector_expand_image.IsVisible());
	}
	
	void SetAttachmentsFromPreset(ExpansionMarketMenuItemManagerPreset preset)
	{
		if (!IsPresetValidCheck(preset))
		{
			CreateErrorNotification("STR_EXPANSION_MARKET_PRESETS_ERROR_INVALID");
			return;
		}
		
		m_MarketMenu.GetSelectedMarketItem().SpawnAttachments.Clear();
		foreach (string attachment: preset.ItemAttachments)
		{
			m_MarketMenu.GetSelectedMarketItem().SpawnAttachments.Insert(attachment);
		}
	
		m_MarketItemManagerController.PresetName = preset.PresetName;
		m_MarketItemManagerController.NotifyPropertyChanged("PresetName");
		
		UpdateMenuViews();
	}
	
	bool IsPresetValidCheck(ExpansionMarketMenuItemManagerPreset preset)
	{
		if (!preset.ItemAttachments || preset.ItemAttachments.Count() == 0)
			return false;
		
		foreach (string attachment: preset.ItemAttachments)
		{
			string classNameToLower = attachment;
			classNameToLower.ToLower();
			
			ExpansionMarketItem marketItem = ExpansionMarketCategory.GetGlobalItem(classNameToLower);
			if (marketItem)
			{
				ExpansionMarketModule marketModule =  ExpansionMarketModule.Cast(CF_ModuleCoreManager.Get(ExpansionMarketModule));
				if (marketModule)
				{
					if (marketModule.GetClientZone().ItemExists(classNameToLower))
						return true;
				}
			}
		}
		
		return false;
	}
	
	void UpdateMenuViews(bool updatePreview = true)
	{
		m_MarketMenu.SetItemAttachmentsInfo(m_MarketMenu.GetSelectedMarketItemElement());

		if (updatePreview)
			UpdatePreview();
		
		m_MarketMenu.UpdateItemFieldFromBasicNetSync();
		m_MarketMenu.GetSelectedMarketItemElement().UpdatePrices();

		UpdateListView();
	}
	
	void DeleteItemPreset(ExpansionMarketMenuItemManagerPreset preset, string path, ExpansionMarketMenuItemManagerPresetElement element)
	{
		string file = path + "\\" + preset.PresetName + ".json";
		if (FileExist(file))
		{
			DeleteFile(file);
		}
		else if (path.IndexOf(EXPANSION_MARKET_CLOTHING_PRESETS_FOLDER) == 0)
		{
			VestCleanup(preset);
		}

		int elementIndex = m_PresetsDropdownElements.Find(element);
		if (elementIndex > -1)
		{
			m_PresetsDropdownElements.Remove(elementIndex);
		}
		
		LoadLocalItemPresets();
	}
	
	void OnCloseButtonClick()
	{
		Hide();
		UpdatePreview();
		m_MarketMenu.UpdateItemFieldFromBasicNetSync();
	}
		
	ExpansionMarketMenuItemManagerController GetMarketItemManagerController()
	{
		return m_MarketItemManagerController;
	}
	
	void OnResetButtonClick()
	{
		m_MarketMenu.GetSelectedMarketItem().SpawnAttachments.Clear();
		
		UpdateMenuViews();
	}
		
	void UpdateRotation(int mouse_x, int mouse_y, bool is_dragging)
	{
		vector o = m_ItemOrientation;
		o[2] = o[2] + (m_ItemRotationY - mouse_y);
		o[1] = o[1] - (m_ItemRotationX - mouse_x);
		
		market_item_manager_preview.SetModelOrientation(o);
		
		if (!is_dragging)
		{
			m_ItemOrientation = o;
		}
	}
	
	void UpdateScale()
	{
		float w, h, x, y;		
		market_item_manager_preview.GetPos(x, y);
		market_item_manager_preview.GetSize(w,h);
		w = w + ( m_ItemScaleDelta / 8);
		if ( w > 0.5 && w < 3 )
		{
			market_item_manager_preview.SetSize( w, h );
			float new_x = x - ( m_ItemScaleDelta / 8 );
			float new_y = y - ( m_ItemScaleDelta / 8 );
			market_item_manager_preview.SetPos( new_x, new_y );
		}
	}
	
	override void OnHide()
	{
		super.OnHide();
		m_MarketItem = NULL;
		m_MarketItemManagerController.CategoryItems.Clear();
		m_MarketItemManagerController.ItemCategories.Clear();
		m_MarketMenu.HideItemManager();
	}
	
	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{	
		super.OnMouseButtonDown(w, x, y, button);
			
		if (w == market_item_manager_preview)
		{
			GetGame().GetDragQueue().Call(this, "UpdateRotation");
			GetMousePos(m_ItemRotationX, m_ItemRotationY);
			return true;
		}		
		return false;
	}
	
	override bool OnMouseWheel(Widget  w, int  x, int  y, int wheel)
	{
		super.OnMouseWheel(w, x, y, wheel);
		
		if ( w == market_item_manager_preview )
		{
			m_ItemScaleDelta = wheel;
			UpdateScale();
		}
		return false;
	}
	
	override bool OnMouseEnter(Widget w, int x, int y)
	{
		super.OnMouseEnter(w, x, y);
		
		switch (w)
		{
			case market_item_manager_back_button:
				market_item_manager_back_button_icon.SetColor(GetExpansionSettings().GetMarket().MarketMenuColors.Get("ColorSearchFilterButton"));
				break;
			case button_close:
				button_close.SetColor(GetExpansionSettings().GetMarket().MarketMenuColors.Get("ColorSearchFilterButton"));
				break;
			case button_reset:
				button_reset.SetColor(GetExpansionSettings().GetMarket().MarketMenuColors.Get("ColorSearchFilterButton"));
				break;
			case preset_selector:
				preset_selector_background.SetColor(GetExpansionSettings().GetMarket().MarketMenuColors.Get("ColorSearchFilterButton"));
				break;
			case button_save:
				button_save.SetColor(GetExpansionSettings().GetMarket().MarketMenuColors.Get("ColorSearchFilterButton"));
				break;
			case preset_editbox_clear:
				market_filter_clear_icon.SetColor(GetExpansionSettings().GetMarket().MarketMenuColors.Get("ColorSearchFilterButton"));
				break;
			case preset_editbox:
				preset_editbox_background.SetColor(GetExpansionSettings().GetMarket().MarketMenuColors.Get("ColorSearchFilterButton"));
				break;
		}		
		return false;
	}
	
	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		super.OnMouseLeave(w, enterW, x, y);
		
		switch (w)
		{
			case market_item_manager_back_button:
				market_item_manager_back_button_icon.SetColor(GetExpansionSettings().GetMarket().MarketMenuColors.Get("BaseColorText"));
				break;
			case button_close:
				button_close.SetColor(ARGB(0, 0, 0, 0));
				break;
			case button_reset:
				button_reset.SetColor(ARGB(0, 0, 0, 0));
				break;
			case preset_selector:
				preset_selector_background.SetColor(GetExpansionSettings().GetMarket().MarketMenuColors.Get("BaseColorHeaders"));
				break;
			case button_save:
				button_save.SetColor(ARGB(0, 0, 0, 0));
				break;
			case preset_editbox_clear:
				market_filter_clear_icon.SetColor(GetExpansionSettings().GetMarket().MarketMenuColors.Get("BaseColorText"));
				break;
			case preset_editbox:
				preset_editbox_background.SetColor(GetExpansionSettings().GetMarket().MarketMenuColors.Get("BaseColorLabels"));
				break;
		}
			
		return false;
	}
	
	void CreateNotification(string text)
	{
		ExpansionNotification("STR_EXPANSION_MARKET_TITLE", text, EXPANSION_NOTIFICATION_ICON_T_Gun, COLOR_EXPANSION_NOTIFICATION_SUCCESS, 3, ExpansionNotificationType.MARKET).Create();
	}
	
	void CreateErrorNotification(string text)
	{
		ExpansionNotification("STR_EXPANSION_MARKET_TITLE", text, EXPANSION_NOTIFICATION_ICON_ERROR, COLOR_EXPANSION_NOTIFICATION_ERROR, 3, ExpansionNotificationType.MARKET).Create();
	}
	
	ExpansionMarketMenu GetMarketMenu()
	{
		return m_MarketMenu;
	}
};

class ExpansionMarketMenuItemManagerController: ExpansionViewController
{
	ref ObservableCollection<ref ExpansionMarketMenuItemManagerCategory> ItemCategories = new ObservableCollection<ref ExpansionMarketMenuItemManagerCategory>(this);
	ref ObservableCollection<ref ExpansionMarketMenuItemManagerCategoryItem> CategoryItems = new ObservableCollection<ref ExpansionMarketMenuItemManagerCategoryItem>(this);
	Object ItemPreview;
	string ItemName;
	string PresetName;
};