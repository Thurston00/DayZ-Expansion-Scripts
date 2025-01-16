/**
 * ExpansionRespawnDelayTimer.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2024 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

class ExpansionRespawnDelayTimer
{
	string Key;
	int Timestamp;
	int Now;
	int Punishment;
	bool IsTerritory;
	
	void ExpansionRespawnDelayTimer(string key, bool isTerritory = false)
	{
		Key = key;
		IsTerritory = isTerritory;
		
		SetTime();
	}
	
	void SetTime()
	{
		Timestamp = CF_Date.Now(true).GetTimestamp();
	}
	
	bool HasCooldown()
	{
		if (GetTimeDiff() < (GetExpansionSettings().GetSpawn().GetCooldown(IsTerritory) + Punishment))
			return true;
		
		return false;
	}
	
	int GetTimeDiff()
	{
		int currentTime = CF_Date.Now(true).GetTimestamp();
		return (currentTime - Timestamp);
	}
	
	void SetPunishment(int sec)
	{
		Punishment = sec;
	}
	
	void AddPunishment(int sec)
	{
		Punishment += sec;
	}
	
	int GetPunishment()
	{
		return Punishment;
	}
		
	bool IsTerritory()
	{
		return IsTerritory;
	}

	void WriteTo(ParamsWriteContext ctx)
	{
		ctx.Write(Key);
		ctx.Write(Timestamp);
		ctx.Write(Now);
		ctx.Write(Punishment);
		ctx.Write(IsTerritory);
	}

	bool ReadFrom(ParamsReadContext ctx)
	{
		if (!ctx.Read(Key))
			return false;

		if (!ctx.Read(Timestamp))
			return false;

		if (!ctx.Read(Now))
			return false;

		if (!ctx.Read(Punishment))
			return false;

		if (!ctx.Read(IsTerritory))
			return false;

		return true;
	}
}

class ExpansionRespawnDelayTimers: map<string, ref ExpansionRespawnDelayTimer>
{
	void RemoveExpiredCooldowns()
	{
		TStringArray toDelete = {};

		foreach (ExpansionRespawnDelayTimer playerTimer: this)
		{
			if (!playerTimer.HasCooldown())
				toDelete.Insert(playerTimer.Key);
		}

		foreach (string key: toDelete)
		{
			Remove(key);
			EXTrace.Print(EXTrace.SPAWNSELECTION, this, "Removed expired cooldown " + key);
		}
	}
}
