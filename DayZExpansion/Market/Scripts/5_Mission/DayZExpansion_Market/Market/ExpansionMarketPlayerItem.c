/**
 * ExpansionMarketPlayerItem.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2021 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

class ExpansionMarketPlayerItem
{
	EntityAI Item;
	string ClassName;
	string DisplayName;
	int Count;
	ref map<string, int> ContainerItems = new map<string, int>;
	bool IsWeapon = false;
	bool IsMagazine = false;
	bool IsAttached = false;
	
	void ExpansionMarketPlayerItem(EntityAI item)
	{
		Item = item;
		ClassName = item.GetType();
		DisplayName = item.GetDisplayName();
		
		if (item.IsWeapon())
			IsWeapon = true;
		
		if (item.GetInventory().IsAttachment())
		{
			if (!item.GetHierarchyParent().IsInherited(SurvivorBase))
				IsAttached = true;
		}
		
		if (item.IsInherited(MagazineStorage))
			IsMagazine = true;
		
		array< EntityAI > items = new array< EntityAI >;
		item.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);
		
		string name;
		int count;
		EntityAI attachment;
		int attachmentsCount;
		
		//! Container items
		if (items && items.Count() > 0)
		{
			for (int i = 0; i < items.Count(); i++)
			{
				//! Skip attachments without cargo on vehicles
				if (items[i].GetInventory().IsAttachment() && !items[i].HasAnyCargo())
				{
					if (items[i].GetHierarchyParent().IsInherited(CarScript))
						continue;
					#ifdef EXPANSIONMODVEHICLE
					else if (items[i].GetHierarchyParent().IsInherited(ExpansionVehicleBase))
						continue;
					#endif
				}

				name = items[i].GetType();	
				
				if (items[i] == item)
					continue;
				
				if (ContainerItems.Get(name))
				{
					count = ContainerItems.Get(name) + 1;
					ContainerItems.Set(name, count);
				}
				else
				{
					ContainerItems.Insert(name, 1);
				}
			}
		
			items.Clear();
		}
	}
	
	void UpdateContainerItems()
	{
		if (Item.IsKindOf("Container_Base"))
		{
			array< EntityAI > items = new array< EntityAI >;
			Item.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);
			
			ContainerItems.Clear();
			
			if (items && items.Count() > 0)
			{	
				for ( int i = 0; i < items.Count(); i++ )
				{
					string name = items[i].ClassName();
					int count;
					
					if (ContainerItems.Get(name))
					{
						count = ContainerItems.Get(name) + 1;
						ContainerItems.Set(name, count);
					}
					else
					{
						ContainerItems.Insert(name, 1);
					}
				}
			}
			
			items.Clear();
		}
	}
	
	TStringArray GetWeaponAttachmentSlotNames(EntityAI weapon)
	{
		TStringArray slots = new TStringArray;
		
		if (weapon && weapon.IsWeapon())
		{
			weapon.ConfigGetTextArray("attachments", slots);
		}
		
		return slots;
		slots.Clear();
	}
	
	bool IsWeapon()
	{
		return IsWeapon;
	}
	
	bool IsMagazine()
	{
		return IsMagazine;
	}
	
	bool IsAttached()
	{
		return IsAttached;
	}
	
	EntityAI GetItem()
	{
		return Item;
	}
}