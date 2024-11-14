modded class DayZGame
{
	void DayZGame()
	{
		TStringArray supportedGraphNames = {
			"dz\\anims\\workspaces\\player\\player_main\\player_main.agr",  //! Vanilla
			"DayZExpansion\\Animations\\Player\\player_main.agr"  //! Expansion Animations
		};

		TStringArray supportedDefaultInstances = {
			"dz\\anims\\workspaces\\player\\player_main\\player_main.asi",  //! Vanilla
			"DayZExpansion\\Animations\\Player\\player_main.asi"  //! Expansion Animations
		};

		bool supported;

		string graphName;
		string defaultInstance;

		ConfigGetText(CFG_VEHICLESPATH + " SurvivorBase enfAnimSys graphName", graphName);
		ConfigGetText(CFG_VEHICLESPATH + " SurvivorBase enfAnimSys defaultInstance", defaultInstance);

		FileHandle file = OpenFile(graphName, FileMode.READ);

		int max;

		if (file)
		{
			string line;

			while (FGets(file, line) >= 0)
			{
				if (line.Contains("#Var VehicleType "))
				{
					line.Replace("\t", " ");

					TStringArray split = {};
					line.Split(" ", split);

					max = split[5].ToInt();

					if (max == 10 || max == 100)
						supported = true;

					break;
				}
			}
		}
		else
		{
			EXError.Warn(null, "Couldn't open animation graph: " + graphName, {});
		}

		string warnTitle = "Potentially unsupported modded animation graph/instance for Expansion AI";
		string warnText = "If you notice AI stuck in specific poses, this could be the reason.";

		if (supportedGraphNames.Find(graphName) == -1)
			EXError.Warn(null, warnTitle + ": " + graphName + "\n" + warnText, {});
		else if (!supported)
			EXError.Warn(null, warnTitle + " (VehicleType max " + max + "): " + graphName + "\n" + warnText, {});

		if (supportedDefaultInstances.Find(defaultInstance) == -1)
			EXError.Warn(null, warnTitle + ": " + defaultInstance + "\n" + warnText, {});
	}

	override void ExplosionEffectsEx(Object source, Object directHit, int componentIndex, float energyFactor, float explosionFactor, HitInfo hitInfo)
	{
		super.ExplosionEffectsEx(source, directHit, componentIndex, energyFactor, explosionFactor, hitInfo);
		
		if (IsServer())
		{
			eAINoiseSystem.AddNoise(hitInfo.GetPosition(), 21.0, string.Format("cfgAmmo %1 NoiseExplosion", hitInfo.GetAmmoType()), hitInfo.GetSurfaceNoiseMultiplier());
		}
	}

	override void FirearmEffects(Object source, Object directHit, int componentIndex, string surface, vector pos, vector surfNormal,
		 vector exitPos, vector inSpeed, vector outSpeed, bool isWater, bool deflected, string ammoType) 
	{
		GetExpansionGame().FixAIFirearmFX(source, directHit, componentIndex, surface, pos, surfNormal, exitPos, inSpeed, outSpeed, isWater, deflected, ammoType);

		super.FirearmEffects(source, directHit, componentIndex, surface, pos, surfNormal, exitPos, inSpeed, outSpeed, isWater, deflected, ammoType);

	/* Probably not necessary for AI to react to bullet impact, enough if they can hear the shot (source)
		if (IsServer())
		{
			float surfaceCoef = SurfaceGetNoiseMultiplier(directHit, pos, componentIndex);
			float coefAdjusted = surfaceCoef * inSpeed.Length() / ConfigGetFloat("cfgAmmo " + ammoType + " initSpeed");
			eAINoiseSystem.AddNoise(pos, 10.0, string.Format("cfgAmmo %1 NoiseHit", ammoType), coefAdjusted);
		}
	*/
	}

	override void CloseCombatEffects(Object source, Object directHit, int componentIndex, string surface, vector pos, vector surfNormal, bool isWater, string ammoType) 
	{
		super.CloseCombatEffects(source, directHit, componentIndex, surface, pos, surfNormal,
		 isWater, ammoType);

		if (IsServer())
		{
			float surfaceCoef = SurfaceGetNoiseMultiplier(directHit, pos, componentIndex);
			eAINoiseSystem.AddNoise(EntityAI.Cast(source), pos, string.Format("cfgAmmo %1 NoiseHit", ammoType), surfaceCoef);
		}
	}
}
