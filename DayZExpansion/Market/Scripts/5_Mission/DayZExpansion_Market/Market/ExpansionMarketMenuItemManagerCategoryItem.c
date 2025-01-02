/**
 * ExpansionMarketMenuItemManagerCategoryItem.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

class ExpansionMarketMenuItemManagerCategoryItem: ExpansionScriptView
{
	protected ref ExpansionMarketMenuItemManagerCategoryItemController m_MarketItemManagerCategoryItemController;
	protected ExpansionMarketMenu m_MarketMenu;
	protected string m_ItemClassName;
	protected ButtonWidget item_element_lmbutton;
	protected Widget item_element_background;
	protected ButtonWidget item_element_increment;
	protected ButtonWidget item_element_decrement;
	protected ButtonWidget item_element_tooltip;
	protected ImageWidget item_element_tooltip_icon;
	protected bool m_CanBeAttached;
	protected bool m_CanBeAttachedOrReplaceConflicting;
	protected ref ExpansionMarketMenuTooltip m_Tooltip;
	
	protected EntityAI m_Object;
	protected ref ExpansionItemTooltip m_ItemTooltip;
	
	void ExpansionMarketMenuItemManagerCategoryItem(string itemClassName, ExpansionMarketMenu menu)
	{
		m_ItemClassName = itemClassName;
		m_MarketMenu = menu;
		
		if (!m_MarketItemManagerCategoryItemController)
			m_MarketItemManagerCategoryItemController = ExpansionMarketMenuItemManagerCategoryItemController.Cast(GetController());
		
		SetView();
	}
	
	void ~ExpansionMarketMenuItemManagerCategoryItem()
	{
		if (m_Object)
			GetGame().ObjectDelete(m_Object);
	}
	
	override string GetLayoutFile() 
	{
		return "DayZExpansion/Market/GUI/layouts/market/expansion_market_menu_item_manager_category_item_element.layout";
	}
	
	override typename GetControllerType() 
	{
		return ExpansionMarketMenuItemManagerCategoryItemController;
	}
	
	void SetView()
	{	
		GetCategoryItemController().ItemName = ExpansionStatic.GetItemDisplayNameWithType(m_ItemClassName);
		GetCategoryItemController().NotifyPropertyChanged("ItemName");

		UpdateView();
		ExpansionMarketMenu.CreatePreviewObject(m_ItemClassName, m_Object);
	}

	int GetCount()
	{
		int count;
		string classNameLower = m_ItemClassName;
		classNameLower.ToLower();
		foreach (string attachment: m_MarketMenu.GetSelectedMarketItem().SpawnAttachments)
		{
			if (attachment == classNameLower)
				count++;
		}
		EXTrace.Print(EXTrace.MARKET, this, "Attached: " + count + " " + classNameLower);
		return count;
	}
	
	void UpdateView()
	{
		EntityAI parent = EntityAI.Cast(m_MarketMenu.GetSelectedMarketItemElement().GetPreviewObject());

		if (parent)
			m_CanBeAttached = CanAttachAttachment(parent, m_ItemClassName, false);
		else
			m_CanBeAttached = false;

		m_CanBeAttachedOrReplaceConflicting = m_CanBeAttached;

		if (m_CanBeAttached)
		{
			//! This item can be attached - nothing to do here
		}
		else if (IsSpecialCase(parent))
		{
			//! Special case: This item can only be attached to specific other item(s)
			Hide();
		}
		else if (parent)
		{
			//! Item cannot be attached to parent in its current state. Check if another attachment is conflicting (slot only)
			string conflictingAttachment;
			m_CanBeAttachedOrReplaceConflicting = IsAttachmentConflicting(m_ItemClassName, true, conflictingAttachment);
			if (m_CanBeAttachedOrReplaceConflicting && conflictingAttachment)
			{
				//! Temporarily remove conflicting attachment
				RemoveAttachment(parent, conflictingAttachment);
				//! Try to attach
				m_CanBeAttachedOrReplaceConflicting = CanAttachAttachment(parent, m_ItemClassName, false);
				//! Add conflicting attachment back
				ExpansionItemSpawnHelper.SpawnAttachment(conflictingAttachment, parent);
			}
		}

		int count = GetCount();
		string color;
		if (m_CanBeAttachedOrReplaceConflicting || count > 0)
			color = "BaseColorHeaders";
		else
			color = "ColorSellButton";
		item_element_background.SetColor(GetExpansionSettings().GetMarket().MarketMenuColors.Get(color));

		bool showDec;
		if (count > 0)
			showDec = true;

		item_element_decrement.Show(showDec);

		bool showInc;
		if (m_CanBeAttached || (count == 0 && m_CanBeAttachedOrReplaceConflicting))
			showInc = true;

		item_element_increment.Show(showInc);

		ShowTooltipCheck();

		if (!IsVisible() && (m_CanBeAttached || m_CanBeAttachedOrReplaceConflicting))
			Show();
	}
	
	bool ShowTooltipCheck()
	{
		bool showTooltip;
		if (!m_CanBeAttachedOrReplaceConflicting && GetCount() == 0)
			showTooltip = true;

		item_element_tooltip.Show(showTooltip);
		return showTooltip;
	}

	bool IsSpecialCase(EntityAI parent)
	{
		if (!parent)
			return false;

		if ((m_ItemClassName == "M4_CarryHandleOptic" || m_ItemClassName == "BUISOptic") && !parent.IsKindOf("M4A1"))
			return true;

		return false;
	}

	ExpansionMarketMenuItemManagerCategoryItemController GetCategoryItemController()
	{
		return m_MarketItemManagerCategoryItemController;
	}
	
	void OnLMButtonClick()
	{
		if (item_element_decrement.IsVisible())
		{
			OnItemCountDecrement();
		}
		else if (item_element_increment.IsVisible())
		{
			OnItemCountIncrement();
		}
	}
	
	//! Check if this item can be attached to parent at all
	bool CanAttachAttachment(EntityAI parent, string attachment, bool includeExisting = false, out bool hasExistingAttachment = false)
	{
		if (includeExisting)
		{
			//! If an item of the same class is already attached, check if it can be removed & reattached
			InventoryLocation loc = new InventoryLocation;

			for (int i = parent.GetInventory().AttachmentCount() - 1; i >= 0; i--)
			{
				EntityAI item = parent.GetInventory().GetAttachmentFromIndex(i);
				if (item)
				{
					if (CF_String.EqualsIgnoreCase(attachment, item.GetType()))
					{
						hasExistingAttachment = true;
						item.GetInventory().GetCurrentInventoryLocation(loc);
						if (!GameInventory.LocationRemoveEntity(loc))
						{
							EXTrace.Print(EXTrace.MARKET, parent, "Could NOT remove existing " + item.GetType());
						}
						else if (!GameInventory.LocationAddEntity(loc))
						{
							EXTrace.Print(EXTrace.MARKET, parent, "Could NOT reattach existing " + item.GetType());
							GetGame().ObjectDelete(item);
						}
						else
						{
							EXTrace.Print(EXTrace.MARKET, parent, "Can reattach existing " + item.GetType());
							return true;
						}
						return false;
					}
				}
			}
		}

		//! Try to create attachment on parent. If this succeeds, attaching is possible on parent in its current state.
		EntityAI attachmentEntity = ExpansionItemSpawnHelper.SpawnAttachment(attachment, parent);
		if (attachmentEntity)
		{
			GetGame().ObjectDelete(attachmentEntity);

			EXTrace.Print(EXTrace.MARKET, parent, "Can attach " + attachment);
			return true;
		}

		EXTrace.Print(EXTrace.MARKET, parent, "Can NOT attach " + attachment);
		return false;
	}
	
	override bool OnMouseEnter(Widget w, int x, int y)
	{
		super.OnMouseEnter(w, x, y);

		switch (w)
		{
			case item_element_lmbutton:
			{
				if (m_CanBeAttachedOrReplaceConflicting)
				{
					item_element_background.SetColor(GetExpansionSettings().GetMarket().MarketMenuColors.Get("ColorSearchFilterButton"));
				}
				
				if (!m_ItemTooltip && m_Object)
				{
					m_ItemTooltip = MissionGameplay.Expansion_GetItemTooltip();
					m_ItemTooltip.SetItem(m_Object);
					m_ItemTooltip.SetContentOffset(0.019531, -0.076389);
					m_ItemTooltip.SetShowItemPreview(true);
					m_ItemTooltip.Show();
				}
				break;
			}
			case item_element_tooltip:
			{
				item_element_tooltip_icon.SetColor(GetExpansionSettings().GetMarket().MarketMenuColors.Get("ColorItemInfoAttachments"));
				UpdateTooltip();
				break;
			}
			case item_element_increment:
			{
				item_element_increment.SetColor(GetExpansionSettings().GetMarket().MarketMenuColors.Get("ColorIncreaseQuantityButton"));
				break;
			}
			case item_element_decrement:
			{
				item_element_decrement.SetColor(GetExpansionSettings().GetMarket().MarketMenuColors.Get("ColorDecreaseQuantityButton"));
				break;
			}
		}		
		return false;
	}
	
	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		super.OnMouseLeave(w, enterW, x, y);

		switch (w)
		{
			case item_element_lmbutton:
			{
				if (m_CanBeAttachedOrReplaceConflicting)
				{
					item_element_background.SetColor(GetExpansionSettings().GetMarket().MarketMenuColors.Get("BaseColorHeaders"));
				}
				
				if (m_ItemTooltip)
				{
					m_ItemTooltip.Destroy();
				}
				break;
			}
			case item_element_tooltip:
			{
				item_element_tooltip_icon.SetColor(GetExpansionSettings().GetMarket().MarketMenuColors.Get("BaseColorText"));
				if (m_Tooltip) 
				{
					m_Tooltip.Destroy();
				}
				break;
			}
			case item_element_increment:
			{
				item_element_increment.SetColor(ARGB(0,0,0,0));
				break;
			}
			case item_element_decrement:
			{
				item_element_decrement.SetColor(ARGB(0,0,0,0));
				break;
			}
		}
			
		return false;
	}
	
	void UpdateTooltip()
	{
		item_element_tooltip_icon.SetColor(GetExpansionSettings().GetMarket().MarketMenuColors.Get("ColorItemInfoAttachments"));
		if (!m_Tooltip)
		{
			m_Tooltip = new ExpansionMarketMenuTooltip();
		}
		
		m_Tooltip.SetTitle("#STR_EXPANSION_MARKET_TOOLTIP_ATTACHMENTS_TITLE");
		m_Tooltip.SetText("#STR_EXPANSION_MARKET_TOOLTIP_ATTACHMENTS_DESC");
		m_Tooltip.SetContentOffset(0.019531, 0);
		m_Tooltip.Show();
	}

	void OnItemCountIncrement()
	{
		string classNameToLower = m_ItemClassName;
		classNameToLower.ToLower();
		ExpansionMarketMenuItemManagerCategoryItem listItem;
		EntityAI parent = EntityAI.Cast(m_MarketMenu.GetSelectedMarketItemElement().GetPreviewObject());
		
		int findIndexAttachment = -1;
		int findIndexListItem = -1;

		string attachmentToReplace;
		string listItemNameToLower;
		
		//! If it cannot be attached, another attachment may be conflicting.
		//! Check by only looking at the slot for non-weapons and also looking at attachments of the same type for weapons.
		//! Note this is different from the IsAttachmentConflicting call in UpdateView!
		if (!m_CanBeAttached && IsAttachmentConflicting(m_ItemClassName, parent && !parent.IsWeapon(), attachmentToReplace))
		{
			if (attachmentToReplace != "")
			{
				RemoveAttachment(attachmentToReplace);
			}
		}
		
		EXTrace.Print(EXTrace.MARKET, this, "Adding " + classNameToLower);
		m_MarketMenu.GetSelectedMarketItem().SpawnAttachments.Insert(classNameToLower);
		
		//! UpdatePreview needs to be called before calling UpdateAttachments
		m_MarketMenu.GetMarketMenuItemManager().UpdatePreview();
		UpdateAttachments();
		m_MarketMenu.GetMarketMenuItemManager().UpdateMenuViews(false);
	}
		
	string GetSlotName(string className)
	{
		string slotName;
		TStringArray inventory_slots = {};
		string slot;
		string path;
		
		if (GetGame().ConfigIsExisting("CfgVehicles " + className))
			path = "CfgVehicles " + className;
		else if (GetGame().ConfigIsExisting("CfgMagazines " + className))
			path = "CfgMagazines " + className;
			
		switch (GetGame().ConfigGetType(path + " inventorySlot")) 
		{
			case CT_ARRAY: 
			{
				GetGame().ConfigGetTextArray(path + " inventorySlot", inventory_slots);
				break;
			}
			
			case CT_STRING: 
			{
				GetGame().ConfigGetText(path + " inventorySlot", slot);
				break;
			}
		}
		
		if (inventory_slots.Count() > 0)
		{
			slotName = inventory_slots[0];
		}
		else if (slot != "")
		{
			slotName = slot;
		}
		
		return slotName;
	}

	void OnItemCountDecrement()
	{		
		if (RemoveAttachment() > -1)
		{	
			//! UpdatePreview needs to be called before calling UpdateAttachments
			m_MarketMenu.GetMarketMenuItemManager().UpdatePreview();
			UpdateAttachments();
			m_MarketMenu.GetMarketMenuItemManager().UpdateMenuViews(false);
		}
	}

	void RemoveAttachment(EntityAI parent, string attachment)
	{
		for (int i = parent.GetInventory().AttachmentCount() - 1; i >= 0; i--)
		{
			EntityAI item = parent.GetInventory().GetAttachmentFromIndex(i);
			if (item)
			{
				if (CF_String.EqualsIgnoreCase(attachment, item.GetType()))
				{
					GetGame().ObjectDelete(item);
					break;
				}
			}
		}
	}

	int RemoveAttachment(string className)
	{
		string classNameToLower = className;
		classNameToLower.ToLower();

		int findIndexAttachment = m_MarketMenu.GetSelectedMarketItem().SpawnAttachments.Find(classNameToLower);
		if (findIndexAttachment > -1)
		{
			EXTrace.Print(EXTrace.MARKET, this, "Removing " + classNameToLower);
			m_MarketMenu.GetSelectedMarketItem().SpawnAttachments.RemoveOrdered(findIndexAttachment);
		}

		return findIndexAttachment;
	}

	int RemoveAttachment()
	{
		return RemoveAttachment(m_ItemClassName);
	}

	void UpdateAttachments()
	{
		//! Check other attachments if they can still be attached and if not, remove them
		EntityAI parent = EntityAI.Cast(m_MarketMenu.GetSelectedMarketItemElement().GetPreviewObject());
		TStringArray attachments = m_MarketMenu.GetSelectedMarketItem().SpawnAttachments;
		for (int j = attachments.Count() - 1; j >= 0; j--)
		{
			bool hasExistingAttachment = false;
			if (!CanAttachAttachment(parent, attachments[j], true, hasExistingAttachment) || !hasExistingAttachment)
			{
				attachments.RemoveOrdered(j);
			}
		}
	}

	bool IsInSpawnAttachments(string className)
	{
		return (m_MarketMenu.GetSelectedMarketItem().SpawnAttachments.Find(className) > -1);
	}
	
	bool IsAttachmentConflicting(string className, bool checkSlotOnly, out string conflictingAttachment = "")
	{
		//! Check if the attachment we want to add to the spawn attachment is conflicting
		//! with an existing attachment (same slot name or same attachment type)
		TStringArray inventory_slots = {};
		
		GetAttachmentSlots(className, inventory_slots);
		
		if (!checkSlotOnly)
		{
			//! Only used for weapons
			ExpansionMarketAttachType attachType1 = ExpansionMarketFilters.GetWeaponAttachmentAttachType(className);
		}

		foreach (string attachment_name: m_MarketMenu.GetSelectedMarketItem().SpawnAttachments)
		{
			if (!checkSlotOnly)
			{
				//! Only used for weapons
				ExpansionMarketAttachType attachType2 = ExpansionMarketFilters.GetWeaponAttachmentAttachType(attachment_name);
				if (attachType1 == attachType2 && attachType1 != ExpansionMarketAttachType.OTHER)
				{
					//! Only allow one of each attachment type
					conflictingAttachment = attachment_name;
					EXTrace.Print(EXTrace.MARKET, this, className + " is conflicting with existing " + conflictingAttachment);
					return true;
				}
			}

			TStringArray attachment_inventory_slots = {};
			
			GetAttachmentSlots(attachment_name, attachment_inventory_slots);
			
			if (inventory_slots.Count() > 0)
			{
				if (attachment_inventory_slots.Count() > 0)
				{
					foreach(string slotName1: inventory_slots)
					{
						if (attachment_inventory_slots.Find(slotName1) > -1)
						{
							conflictingAttachment = attachment_name;
							break;
						}
					}
				}
			}

			if (conflictingAttachment)
			{
				EXTrace.Print(EXTrace.MARKET, this, className + " is conflicting with existing " + conflictingAttachment + " in slot " + slotName1);
				return true;
			}
		}
		
		return false;
	}
	
	//! TODO: Move this to ExpansionStatic maybe?
	static bool GetAttachmentSlots(string className, out TStringArray inventory_slots)
	{
		string path;
		if (GetGame().ConfigIsExisting("CfgVehicles " + className))
			path = "CfgVehicles " + className;
		else if (GetGame().ConfigIsExisting("CfgMagazines " + className))
			path = "CfgMagazines " + className;
		else
			return false;

		switch (GetGame().ConfigGetType(path + " inventorySlot")) 
		{
			case CT_ARRAY: 
			{
				GetGame().ConfigGetTextArray(path + " inventorySlot", inventory_slots);
				TStringArray tmp = inventory_slots;
				inventory_slots = new TStringArray;
				foreach (string inventory_slot: tmp)
				{
					inventory_slot.ToLower();
					inventory_slots.Insert(inventory_slot);
				}
				return true;
			}
			
			case CT_STRING: 
			{
				string slot;
				GetGame().ConfigGetText(path + " inventorySlot", slot);
				slot.ToLower();
				inventory_slots.Insert(slot);
				return true;
			}
		}

		return false;
	}

	string GetItemClassName()
	{
		return m_ItemClassName;
	}
};

class ExpansionMarketMenuItemManagerCategoryItemController: ExpansionViewController
{
	string ItemName;
};