/**
 * ItemManager.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

modded class ItemManager
{
	void ItemManager(Widget root)
	{
		m_TooltipWidget = GetGame().GetWorkspace().CreateWidgets("DayZExpansion/Hardline/GUI/layouts/expansion_inventory_tooltip.layout", root );
		m_TooltipWidget.Show(false);
	}
	
	override void PrepareTooltip(EntityAI item, int x = 0, int y = 0)
	{
		super.PrepareTooltip(item, x, y);
	
		InspectMenuNew.Expansion_UpdateItemInfoRarity(m_TooltipWidget, item);
	}
};