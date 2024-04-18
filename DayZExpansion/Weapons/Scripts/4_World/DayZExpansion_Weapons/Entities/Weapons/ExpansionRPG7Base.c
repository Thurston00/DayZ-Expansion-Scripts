/**
 * ExpansionRPG7Base.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

class ExpansionRPG7Base: ExpansionLauncher_Base
{	
	override RecoilBase SpawnRecoilObject()
	{
		return new Izh18Recoil(this);
	}

	override void EEFired(int muzzleType, int mode, string ammoType)
  	{
		if ( IsMissionClient() )
		{
			Particle.PlayOnObject( ParticleList.EXPANSION_ROCKET_DUST, this, GetMemoryPointPos("usti hlavne") );
		}

		super.EEFired(muzzleType, mode, ammoType);
	}
	
	override bool ShootsExplosiveAmmo()
	{
		return true;
	}
};

class ExpansionRPG7: ExpansionRPG7Base {};