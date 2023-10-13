/**
 * ExpansionQuestObjectiveTravelConfig.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/
class ExpansionQuestObjectiveTravelConfigBase: ExpansionQuestObjectiveConfig
{
	vector Position = vector.Zero;
	float MaxDistance = 10.0;
	string MarkerName = string.Empty;
	bool ShowDistance = true;
	bool TriggerOnEnter = true;
	bool TriggerOnExit = false;
};

class ExpansionQuestObjectiveTravelConfig: ExpansionQuestObjectiveTravelConfigBase
{
	void SetPosition(vector pos)
	{
		Position = pos;
	}

	vector GetPosition()
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

	void SetTriggerOnEnter(bool state)
	{
		TriggerOnEnter = state;
	}

	bool TriggerOnEnter()
	{
		return TriggerOnEnter;
	}

	void SetTriggerOnExit(bool state)
	{
		TriggerOnExit = state;
	}

	bool TriggerOnExit()
	{
		return TriggerOnExit;
	}

	static ExpansionQuestObjectiveTravelConfig Load(string fileName)
	{
		bool save;
		Print("[ExpansionQuestObjectiveTravelConfig] Load existing configuration file:" + EXPANSION_QUESTS_OBJECTIVES_TRAVEL_FOLDER + fileName);

		ExpansionQuestObjectiveTravelConfig config;
		ExpansionQuestObjectiveTravelConfigBase configBase;

		if (!ExpansionJsonFileParser<ExpansionQuestObjectiveTravelConfigBase>.Load(EXPANSION_QUESTS_OBJECTIVES_TRAVEL_FOLDER + fileName, configBase))
			return NULL;

		if (configBase.ConfigVersion < CONFIGVERSION)
		{
			Print("[ExpansionQuestObjectiveTravelConfig] Convert existing configuration file:" + EXPANSION_QUESTS_OBJECTIVES_TRAVEL_FOLDER + fileName + " to version " + CONFIGVERSION);
			config = new ExpansionQuestObjectiveTravelConfig();

			//! Copy over old configuration that haven't changed
			config.CopyConfig(configBase);

			config.ConfigVersion = CONFIGVERSION;
			save = true;
		}
		else
		{
			if (!ExpansionJsonFileParser<ExpansionQuestObjectiveTravelConfig>.Load(EXPANSION_QUESTS_OBJECTIVES_TRAVEL_FOLDER + fileName, config))
				return NULL;
		}

		if (save)
		{
			config.Save(fileName);
		}

		return config;
	}

	override void Save(string fileName)
	{
		auto trace = EXTrace.Start(EXTrace.QUESTS, this, EXPANSION_QUESTS_OBJECTIVES_TRAVEL_FOLDER + fileName);

		if (!ExpansionString.EndsWithIgnoreCase(fileName, ".json"))
			fileName += ".json";
		
		ExpansionJsonFileParser<ExpansionQuestObjectiveTravelConfig>.Save(EXPANSION_QUESTS_OBJECTIVES_TRAVEL_FOLDER + fileName, this);
	}

	void CopyConfig(ExpansionQuestObjectiveTravelConfigBase configBase)
	{
		ID = configBase.ID;
		ObjectiveType = configBase.ObjectiveType;
		ObjectiveText = configBase.ObjectiveText;
		TimeLimit = configBase.TimeLimit;

		Position = configBase.Position;
		MaxDistance = configBase.MaxDistance;
		MarkerName = configBase.MarkerName;
		ShowDistance = configBase.ShowDistance;
		
		TriggerOnEnter = configBase.TriggerOnEnter;
		TriggerOnExit = configBase.TriggerOnExit;
	}

	override void OnSend(ParamsWriteContext ctx)
	{
		super.OnSend(ctx);

		ctx.Write(Position);
		ctx.Write(MaxDistance);
		ctx.Write(MarkerName);
		ctx.Write(ShowDistance);
	}

	override bool OnRecieve(ParamsReadContext ctx)
	{
		if (!super.OnRecieve(ctx))
			return false;

		if (!ctx.Read(Position))
			return false;

		if (!ctx.Read(MaxDistance))
			return false;

		if (!ctx.Read(MarkerName))
			return false;

		if (!ctx.Read(ShowDistance))
			return false;

		return true;
	}
};