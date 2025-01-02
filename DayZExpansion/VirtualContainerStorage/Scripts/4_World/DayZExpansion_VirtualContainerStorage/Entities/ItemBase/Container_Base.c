/**
 * Container_Base.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

modded class Container_Base
{
	int m_Expansion_EntityStorageID = -1;  //! Legacy, was only on Expansion Experimental

	#ifdef SERVER
	override void DeferredInit()
	{
		//! Convert legacy
		if (m_ExpansionSaveVersion == 42 && m_Expansion_EntityStorageID != -1)
		{
			if (!m_Expansion_GlobalID.m_IsSet)
				m_Expansion_GlobalID.Acquire();

			string oldFileName = ExpansionEntityStorageModule.GetFileName(m_Expansion_EntityStorageID);
			if (FileExist(oldFileName) && CopyFile(oldFileName, Expansion_GetEntityStorageFileName()))
				DeleteFile(oldFileName);
		}

		super.DeferredInit();
	}
	#endif

	#ifdef EXPANSION_MODSTORAGE
	//! Legacy, handled by ItemBase since BaseBuilding storage version 52
	override bool CF_OnStoreLoad(CF_ModStorageMap storage)
	{
		if (!super.CF_OnStoreLoad(storage))
			return false;

		auto ctx = storage[DZ_Expansion_BaseBuilding];
		if (!ctx) return true;

		int version = ctx.GetVersion();
		if (version < 42)
			return true;

		if (version == 42)
		{
			if (!ctx.Read(m_Expansion_EntityStorageID))
				return false;
		}

		if (version < 43 || version >= 52)
			return true;

		if (!m_Expansion_GlobalID.OnStoreLoad(ctx))
			return false;

		return true;
	}
	#endif

	override void Open()
	{
		super.Open();

		if (GetGame().IsServer() && Expansion_CanUseVirtualStorage(true))
			Expansion_RestoreContents();
	}

	override void Close()
	{
		if (GetGame().IsServer() && Expansion_CanUseVirtualStorage())
			Expansion_StoreContents();

		super.Close();
	}
}
