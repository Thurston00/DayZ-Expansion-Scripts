/**
 * ExpansionQuestObjectiveTargetConfig.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

class ExpansionQuestObjectiveTargetConfigBase: ExpansionQuestObjectiveConfig
{
	vector Position = vector.Zero;
	float MaxDistance = -1;
	autoptr ExpansionQuestObjectiveTarget Target;
};

class ExpansionQuestObjectiveTargetConfig: ExpansionQuestObjectiveTargetConfigBase
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

	void SetTarget(ExpansionQuestObjectiveTarget target)
	{
		Target = target;
	}

	ExpansionQuestObjectiveTarget GetTarget()
	{
		return Target;
	}

	static ExpansionQuestObjectiveTargetConfig Load(string fileName)
	{
		bool save;
		Print("[ExpansionQuestObjectiveTargetConfig] Load existing configuration file:" + EXPANSION_QUESTS_OBJECTIVES_TARGET_FOLDER + fileName);

		ExpansionQuestObjectiveTargetConfig config;
		ExpansionQuestObjectiveTargetConfigBase configBase;

		if (!ExpansionJsonFileParser<ExpansionQuestObjectiveTargetConfigBase>.Load(EXPANSION_QUESTS_OBJECTIVES_TARGET_FOLDER + fileName, configBase))
			return NULL;

		if (configBase.ConfigVersion < CONFIGVERSION)
		{
			Print("[ExpansionQuestObjectiveTargetConfig] Convert existing configuration file:" + EXPANSION_QUESTS_OBJECTIVES_TARGET_FOLDER + fileName + " to version " + CONFIGVERSION);
			config = new ExpansionQuestObjectiveTargetConfig();
			
			//! Copy over old configuration that haven't changed
			config.CopyConfig(configBase);

		#ifdef EXPANSIONMODAI
			if (configBase.ConfigVersion < 13)
			{
				ExpansionQuestObjectiveTarget target = config.GetTarget();
				if (target)
				{
					target.SetCountAIPlayers(false);
				}
			}
		#endif

			config.ConfigVersion = CONFIGVERSION;
			save = true;
		}
		else
		{
			if (!ExpansionJsonFileParser<ExpansionQuestObjectiveTargetConfig>.Load(EXPANSION_QUESTS_OBJECTIVES_TARGET_FOLDER + fileName, config))
				return NULL;
		}

		if (save)
			config.Save(fileName);

		return config;
	}
	
	override void Save(string fileName)
	{
		auto trace = EXTrace.Start(EXTrace.QUESTS, this, EXPANSION_QUESTS_OBJECTIVES_TARGET_FOLDER + fileName);

		if (!ExpansionString.EndsWithIgnoreCase(fileName, ".json"))
			fileName += ".json";
		
		ExpansionJsonFileParser<ExpansionQuestObjectiveTargetConfig>.Save(EXPANSION_QUESTS_OBJECTIVES_TARGET_FOLDER + fileName, this);
	}

	void CopyConfig(ExpansionQuestObjectiveTargetConfigBase configBase)
	{
		ID = configBase.ID;
		ObjectiveType = configBase.ObjectiveType;
		ObjectiveText = configBase.ObjectiveText;
		TimeLimit = configBase.TimeLimit;

		Position = configBase.Position;
		MaxDistance = configBase.MaxDistance;
		Target = configBase.Target;
	}

	override bool Validate()
	{
		if (!super.Validate())
			return false;

		if (!Target)
			return false;

		return true;
	}

	override void QuestDebug()
	{
	#ifdef EXPANSIONMODQUESTSOBJECTIVEDEBUG
		super.QuestDebug();
		if (Target)
			Target.QuestDebug();
	#endif
	}
};