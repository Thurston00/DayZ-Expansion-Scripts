modded class CarScript
{
#ifdef EXPANSION_MODSTORAGE
	autoptr CF_ModStorageBase m_CF_ModStorage = new CF_ModStorageObject<CarScript>(this);

	override void OnStoreSave(ParamsWriteContext ctx)
	{
		super.OnStoreSave(ctx);

		m_CF_ModStorage.OnStoreSave(ctx);
	}

	override bool OnStoreLoad(ParamsReadContext ctx, int version)
	{
		if (!super.OnStoreLoad(ctx, version))
		{
			return false;
		}

		return m_CF_ModStorage.OnStoreLoad(ctx, version);
	}
#endif

	void CF_OnStoreSave(CF_ModStorageMap storage)
	{
	}

	bool CF_OnStoreLoad(CF_ModStorageMap storage)
	{
		return true;
	}
};

#ifndef DAYZ_1_25
modded class BoatScript
{
#ifdef EXPANSION_MODSTORAGE
	autoptr CF_ModStorageBase m_CF_ModStorage = new CF_ModStorageObject<BoatScript>(this);

	override void OnStoreSave(ParamsWriteContext ctx)
	{
		super.OnStoreSave(ctx);

		m_CF_ModStorage.OnStoreSave(ctx);
	}

	override bool OnStoreLoad(ParamsReadContext ctx, int version)
	{
		if (!super.OnStoreLoad(ctx, version))
		{
			return false;
		}

		//! Since CF wasn't updated when DayZ 1.26 released, BoatScript storage data does not carry
		//! CF data *at all* if the entity isn't tracked, so we have to return early
		if (!CF_Modules<Expansion_ModStorageModule>.Get().IsEntity(this))
			return true;

		return m_CF_ModStorage.OnStoreLoad(ctx, version);
	}
#endif

	void CF_OnStoreSave(CF_ModStorageMap storage)
	{
	}

	bool CF_OnStoreLoad(CF_ModStorageMap storage)
	{
		return true;
	}
};
#endif
