/**
 * ExpansionSpawnLocation.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2024 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

class ExpansionSpawnLocation
{
	string Name;
	ref array< vector > Positions;
	bool UseCooldown;

	[NonSerialized()]
	int TerritoryID = -1;

	void SetLocation(string name, array< vector > positions, int territoryID = -1)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(ExpansionTracing.SETTINGS, this);
#endif
				
		Name = name;
		
		Positions = new array< vector >;
		for ( int i = 0; i < positions.Count(); i++ )
			Positions.Insert( positions[i] );
		
		TerritoryID = territoryID;
	}

	//! This is not used anywhere?!
	/*static ExpansionSpawnLocation Copy(  ExpansionSpawnLocation src )
	{
		ExpansionSpawnLocation dst = new ExpansionSpawnLocation( src.Name, new array< vector >(), src.TerritoryID );
		return dst;
	}*/

	void SetUseCooldown(bool state)
	{
		UseCooldown = state;
	}
	
	bool UseCooldown()
	{
		return UseCooldown;
	}

	string GetKey()
	{
		string key;

		if (TerritoryID != -1)
			key = "T=" + TerritoryID.ToString();
		else
			key = "N=" + Name;

		key += "|" + ExpansionStatic.VectorToString(Positions[0], ExpansionVectorToString.Plain);

		return key;
	}

	void OnSend(ParamsWriteContext ctx)
	{
		ctx.Write(Name);
		ctx.Write(Positions);
		ctx.Write(UseCooldown);
		ctx.Write(TerritoryID);
	}

	bool OnReceive(ParamsReadContext ctx)
	{
		if (!ctx.Read(Name))
		{
			Error(ToString() + "::OnReceive - Name");
			return false;
		}

		if (!ctx.Read(Positions))
		{
			Error(ToString() + "::OnReceive - Positions");
			return false;
		}

		if (!ctx.Read(UseCooldown))
		{
			Error(ToString() + "::OnReceive - UseCooldown");
			return false;
		}

		if (!ctx.Read(TerritoryID))
		{
			Error(ToString() + "::OnReceive - TerritoryID");
			return false;
		}

		return true;
	}
};