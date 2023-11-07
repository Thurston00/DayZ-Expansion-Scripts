/**
 * ExpansionQuestObjectiveAIEscortConfigBase.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

class ExpansionQuestObjectiveAIEscortConfigBase: ExpansionQuestObjectiveConfig
{
	vector Position = vector.Zero;
	float MaxDistance = 0;
	ref ExpansionQuestObjectiveAIVIP AIVIP;
	string MarkerName = string.Empty;
	bool ShowDistance = true;
	bool CanLootAI = false;
};

class ExpansionQuestObjectiveAIEscortConfig: ExpansionQuestObjectiveAIEscortConfigBase
{
	void SetPosition(vector pos)
	{
		Position = pos;
	}

	override vector GetPosition()
	{
		return Position;
	}

	void SetMaxDistance(float max)
	{
		MaxDistance = max;
	}

	float GetMaxDistance()
	{
		return MaxDistance;
	}

	void SetAIVIP(ExpansionQuestObjectiveAIVIP vip)
	{
		AIVIP = vip;
	}

	ExpansionQuestObjectiveAIVIP GetAIVIP()
	{
		return AIVIP;
	}

	void SetMarkerName(string name)
	{
		MarkerName = name;
	}

	string GetMarkerName()
	{
		return MarkerName;
	}

	bool ShowDistance()
	{
		return ShowDistance;
	}
	
	void SetCanLootAI(bool state)
	{
		CanLootAI = state;
	}
	
	bool CanLootAI()
	{
		return CanLootAI;
	}

	static ExpansionQuestObjectiveAIEscortConfig Load(string fileName)
	{
		bool save;
		Print("[ExpansionQuestObjectiveAIEscortConfig] Load existing configuration file:" + EXPANSION_QUESTS_OBJECTIVES_AIVIP_FOLDER + fileName);

		ExpansionQuestObjectiveAIEscortConfig config;
		ExpansionQuestObjectiveAIEscortConfigBase configBase;

		if (!ExpansionJsonFileParser<ExpansionQuestObjectiveAIEscortConfigBase>.Load(EXPANSION_QUESTS_OBJECTIVES_AIVIP_FOLDER + fileName, configBase))
			return NULL;

		if (configBase.ConfigVersion < CONFIGVERSION)
		{
			Print("[ExpansionQuestObjectiveAIEscortConfig] Convert existing configuration file:" + EXPANSION_QUESTS_OBJECTIVES_AIVIP_FOLDER + fileName + " to version " + CONFIGVERSION);
			config = new ExpansionQuestObjectiveAIEscortConfig();

			//! Copy over old configuration that haven't changed
			config.CopyConfig(configBase);

			config.ConfigVersion = CONFIGVERSION;
			save = true;
		}
		else
		{
			if (!ExpansionJsonFileParser<ExpansionQuestObjectiveAIEscortConfig>.Load(EXPANSION_QUESTS_OBJECTIVES_AIVIP_FOLDER + fileName, config))
				return NULL;
		}
		
		string removeExt = ExpansionString.StripExtension(config.AIVIP.GetNPCLoadoutFile(), ".json");
		if (removeExt != config.AIVIP.GetNPCLoadoutFile())
		{
			config.AIVIP.SetNPCLoadoutFile(removeExt);
			save = true;
		}

		if (save)
		{
			config.Save(fileName);
		}

		return config;
	}
	
	override void Save(string fileName)
	{
		auto trace = EXTrace.Start(EXTrace.QUESTS, this, EXPANSION_QUESTS_OBJECTIVES_AIVIP_FOLDER + fileName);

		if (!ExpansionString.EndsWithIgnoreCase(fileName, ".json"))
			fileName += ".json";
		
		ExpansionJsonFileParser<ExpansionQuestObjectiveAIEscortConfig>.Save(EXPANSION_QUESTS_OBJECTIVES_AIVIP_FOLDER + fileName, this);
	}

	void CopyConfig(ExpansionQuestObjectiveAIEscortConfigBase configBase)
	{
		ID = configBase.ID;
		ObjectiveType = configBase.ObjectiveType;
		ObjectiveText = configBase.ObjectiveText;
		TimeLimit = configBase.TimeLimit;

		Position = configBase.Position;
		MaxDistance = configBase.MaxDistance;
		AIVIP = configBase.AIVIP;
		MarkerName = configBase.MarkerName;
		ShowDistance = configBase.ShowDistance;
		CanLootAI = configBase.CanLootAI;
	}

	override void OnSend(ParamsWriteContext ctx)
	{
		super.OnSend(ctx);

		ctx.Write(Position);
		ctx.Write(ShowDistance);
		ctx.Write(MarkerName);
	}

	override bool OnRecieve(ParamsReadContext ctx)
	{
		if (!super.OnRecieve(ctx))
			return false;

		if (!ctx.Read(Position))
			return false;

		if (!ctx.Read(ShowDistance))
			return false;
		
		if (!ctx.Read(MarkerName))
			return false;

		return true;
	}

	override bool Validate()
	{
		if (!super.Validate())
			return false;

		if (!AIVIP)
			return false;

		return true;
	}

	override void QuestDebug()
	{
	#ifdef EXPANSIONMODQUESTSOBJECTIVEDEBUG
		super.QuestDebug();
		if (AIVIP)
			AIVIP.QuestDebug();
	#endif
	}
};
