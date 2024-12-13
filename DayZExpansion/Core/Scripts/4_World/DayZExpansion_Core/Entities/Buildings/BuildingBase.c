/**
 * BuildingBase.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

modded class BuildingBase
{
	ref ExpansionNetsyncData m_Expansion_NetsyncData;
	ref ExpansionRPCManager m_Expansion_RPCManager;

	override bool NameOverride(out string output)
	{
		if (m_Expansion_NetsyncData && m_Expansion_NetsyncData.Get(0, output))
			return true;
		else
			return super.NameOverride(output);
	}

	override void Explode(int damageType, string ammoType = "")
	{
		ExpansionDamageSystem.OnBeforeExplode(this, damageType, ammoType);

		super.Explode(damageType, ammoType);
	}

	override bool EEOnDamageCalculated(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		if (!super.EEOnDamageCalculated(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef))
			return false;

		if (damageType == DT_EXPLOSION)
		{
			if (!ExpansionDamageSystem.OnExplosionDamageCalculated(damageResult, source, this, component, dmgZone, ammo, modelPos, speedCoef, true))
				return false;
		}

		return true;
	}

	override void EEHitBy(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		super.EEHitBy(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);

		if (damageType == DT_EXPLOSION && ExpansionDamageSystem.IsEnabledForExplosionTarget(this))
			ExpansionDamageSystem.OnExplosionHit(source, this, ammo, true, damageResult);
	}

	override void OnVariablesSynchronized()
	{
		super.OnVariablesSynchronized();

		if (m_Expansion_NetsyncData && !m_Expansion_NetsyncData.m_WasDataRequested)
			m_Expansion_NetsyncData.Request();
	}
}
