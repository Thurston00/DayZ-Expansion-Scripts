/**
 * ExpansionMarketMenuItemManagerPresetElement.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

class ExpansionMarketMenuItemManagerPresetElement: ExpansionScriptView
{
	protected ref ExpansionMarketMenuItemManagerPresetElementController m_MarketItemManagerPresetElementController;
	protected ExpansionMarketMenuItemManager m_MarketMenuItemManager;
	protected ExpansionMarketMenuItemManagerPreset m_Preset;
	
	protected ButtonWidget dropdown_element_button;
	protected ButtonWidget dropdown_element_clear;
	protected ImageWidget market_filter_clear_icon;
	protected Widget dropdown_element_background;
	
	void ExpansionMarketMenuItemManagerPresetElement(ExpansionMarketMenuItemManagerPreset preset, ExpansionMarketMenuItemManager itemManager)
	{
		m_Preset = preset;
		m_MarketMenuItemManager = itemManager;
		
		if (!m_MarketItemManagerPresetElementController)
			m_MarketItemManagerPresetElementController = ExpansionMarketMenuItemManagerPresetElementController.Cast(GetController());
		
		SetView();
	}
	
	override string GetLayoutFile() 
	{
		return "DayZExpansion/Market/GUI/layouts/market/expansion_market_menu_item_manager_preset_element.layout";
	}
	
	override typename GetControllerType() 
	{
		return ExpansionMarketMenuItemManagerPresetElementController;
	}
	
	void SetView()
	{
		m_MarketItemManagerPresetElementController.Text = m_Preset.PresetName;
		m_MarketItemManagerPresetElementController.NotifyPropertyChanged("Text");
	}
	
	void OnElementButtonClick()
	{
		m_MarketMenuItemManager.SetAttachmentsFromPreset(m_Preset);
	}
	
	override bool OnMouseEnter(Widget w, int x, int y)
	{		
		switch (w)
		{
			case dropdown_element_button:
				dropdown_element_background.SetColor(GetExpansionSettings().GetMarket().MarketMenuColors.Get("ColorSearchFilterButton"));
				break;
			case dropdown_element_clear:
				market_filter_clear_icon.SetColor(GetExpansionSettings().GetMarket().MarketMenuColors.Get("ColorSearchFilterButton"));
				break;
		}		
		return false;
	}
	
	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{		
		switch (w)
		{
			case dropdown_element_button:
				dropdown_element_background.SetColor(GetExpansionSettings().GetMarket().MarketMenuColors.Get("BaseColorHeaders"));
				break;
			case dropdown_element_clear:
				market_filter_clear_icon.SetColor(GetExpansionSettings().GetMarket().MarketMenuColors.Get("BaseColorText"));
				break;
		}
			
		return false;
	}
	
	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (w == dropdown_element_clear)
		{
			if (m_MarketMenuItemManager.GetMarketMenu().GetSelectedMarketItemElement().GetPreviewObject().IsWeapon())
			{
				m_MarketMenuItemManager.DeleteItemPreset(m_Preset, EXPANSION_MARKET_WEAPON_PRESETS_FOLDER + m_Preset.ClassName + "\\", this);
			}
			else if (m_MarketMenuItemManager.GetMarketMenu().GetSelectedMarketItemElement().GetPreviewObject().IsClothing())
			{
				if (ExpansionMarketFilters.IsCustomizableClothing(m_MarketMenuItemManager.GetMarketMenu().GetSelectedMarketItemElement().GetPreviewObject().GetType()))
				{
					m_MarketMenuItemManager.DeleteItemPreset(m_Preset, EXPANSION_MARKET_CLOTHING_PRESETS_FOLDER + m_Preset.ClassName + "\\", this);
				}
			}
			
			return true;
		}

		return false;
	}
};

class ExpansionMarketMenuItemManagerPresetElementController: ExpansionViewController
{
	string Text;
};
