/**
 * BarrelHoles_ColorBase.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

//! Inherits from FireplaceBase
modded class BarrelHoles_ColorBase
{
	#ifdef EXPANSION_MODSTORAGE
	//! Legacy, handled by ItemBase since BaseBuilding storage version 52
	override bool CF_OnStoreLoad(CF_ModStorageMap storage)
	{
		if (!super.CF_OnStoreLoad(storage))
			return false;

		auto ctx = storage[DZ_Expansion_BaseBuilding];
		if (!ctx) return true;

		int version = ctx.GetVersion();
		if (version < 45 || version >= 52)
			return true;

		if (!m_Expansion_GlobalID.OnStoreLoad(ctx))
			return false;

		return true;
	}
	#endif

	override bool Expansion_CanUseVirtualStorage(bool restoreOverride = false)
	{
		if (IsBurning())
			return false;

		if (super.Expansion_CanUseVirtualStorage(restoreOverride))
			return true;

		return false;
	}

	override void Open()
	{
		super.Open();

		if (GetGame().IsServer() && Expansion_CanUseVirtualStorage(true))
			Expansion_RestoreContents();
	}

	override void Close()
	{
		if (GetGame().IsServer() && !IsBurning() && Expansion_CanUseVirtualStorage())
			Expansion_StoreContents();

		super.Close();
	}

	override bool IsNonExpansionOpenable()
	{
		return true;
	}
}
