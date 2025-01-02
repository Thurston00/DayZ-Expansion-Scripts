class eAIShot
{
	float m_Time;
	float m_ProcessedTime;
	Weapon_Base m_Weapon;
	vector m_Origin;
	vector m_Direction;
	Object m_HitObject;
	Object m_HitObjectRoot;
	vector m_HitPosition;
	//vector m_HitPositionMS;
	int m_Component;
	string m_Ammo;
	float m_Distance;
	float m_SpeedCoef;
	float m_DamageCoef;
	float m_TravelTime;

	void eAIShot(Weapon_Base weapon, int muzzleIndex, vector origin, vector dir, Object hitObject, vector hitPosition, int component)
	{
		m_Time = GetGame().GetTickTime();

		m_Weapon = weapon;

		m_Origin = origin;
		m_Direction = dir;

		m_HitObject = hitObject;

		EntityAI entity;
		if (Class.CastTo(entity, hitObject))
			m_HitObjectRoot = entity.GetHierarchyRoot();
		else
			m_HitObjectRoot = hitObject;

		m_HitPosition = hitPosition;
		//m_HitPositionMS = m_HitObjectRoot.WorldToModel(hitPosition);

		m_Component = component;

		float ammoDamage;

		weapon.GetCartridgeInfo(muzzleIndex, ammoDamage, m_Ammo);

		float airFriction;
		float initSpeed;

		m_DamageCoef = weapon.eAI_CalculateProjectileDamageCoefAtPosition(origin, m_Ammo, hitPosition, 1.0, airFriction, m_Distance, m_SpeedCoef, initSpeed);
		m_TravelTime = weapon.eAI_CalculateProjectileTravelTime(airFriction, m_Distance, initSpeed);
	}

	string GetInfo()
	{
		string physInfo;

		if (m_TravelTime)
			physInfo = string.Format(" speedCoef=%1 travelTime=%2 dmgCoef=%3", m_SpeedCoef, m_TravelTime, m_DamageCoef);

		string weaponInfo = ExpansionStatic.GetDebugInfo(m_Weapon);
		string hitObjInfo = ExpansionStatic.GetHierarchyInfo(m_HitObject);
		return string.Format("%1 processed=%2 weapon=%3 hitObject=%4 hitPosition=%5 ammo=%6%7", this, m_ProcessedTime, weaponInfo, hitObjInfo, m_HitPosition.ToString(), m_Ammo, physInfo);
	}
}

class eAIDamageHandler
{
	static ref TStringArray s_HumanDmgZonesForRedirect = {
		"Torso",
		"LeftArm",
		"LeftHand",
		"RightArm",
		"RightHand",
		"LeftLeg",
		"LeftFoot",
		"RightLeg",
		"RightFoot"
	};

	EntityAI m_Entity;
	eAIEntityTargetInformation m_TargetInformation;
	bool m_ProcessDamage;
	int m_HitCounter;
	float m_LastHitTime;
	eAIBase m_LastSourceAI;

	void eAIDamageHandler(EntityAI entity, eAIEntityTargetInformation info)
	{
		m_Entity = entity;
		m_TargetInformation = info;
	}

	float GetDebugDamageMultiplier(TotalDamageResult damageResult, string dmgZone)
	{
		if (DayZPlayerImplement.s_eAI_DebugDamage && m_Entity.GetHierarchyRoot().IsMan())
		{
			//! Prevent death by health or blood dmg (can still bleed out from cuts)

			float dmg = damageResult.GetDamage(dmgZone, "Health");
			float transferToGlobalCoef = 1.0;

			if (dmgZone)
			{
				string path;

				if (m_Entity.IsMagazine())
					path = CFG_MAGAZINESPATH;
				else if (m_Entity.IsWeapon())
					path = CFG_WEAPONSPATH;
				else
					path = CFG_VEHICLESPATH;

				path += " " + m_Entity.GetType() + " DamageSystem DamageZones " + dmgZone + " Health transferToGlobalCoef";

				if (GetGame().ConfigIsExisting(path))
					transferToGlobalCoef = GetGame().ConfigGetFloat(path);
			}

			EXTrace.Print(EXTrace.AI, m_Entity, "Global damage: " + (dmg * transferToGlobalCoef));
			EXTrace.Print(EXTrace.AI, m_Entity, "Global health: " + m_Entity.GetHealth("", "Health"));

			if (dmgZone)
				EXTrace.Print(EXTrace.AI, m_Entity, dmgZone + " health: " + m_Entity.GetHealth(dmgZone, "Health"));

			if (m_Entity.IsMan())
			{
				if (dmgZone == "Head")
				{
					//! If head health goes to zero, character dies
					if (dmg >= Math.Floor(m_Entity.GetHealth(dmgZone, "Health")))
						return 0.01;
				}

				//! Any damage to brain is certain death, HP don't matter
				if (dmgZone == "Brain")
					return 0;

				if (Math.Floor(m_Entity.GetHealth("", "Blood")) - damageResult.GetDamage(dmgZone, "Blood") <= 2600)
					return 0.01;
			}

			if (dmg * transferToGlobalCoef >= Math.Floor(m_Entity.GetHealth("", "Health")))
				return 0.01;
		}

		return 1.0;
	}

	bool OnDamageCalculated(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
	#ifdef DIAG_DEVELOPER
		m_HitCounter++;

		EXTrace.PrintHit(EXTrace.AI, m_Entity, ToString() + "::OnDamageCalculated[" + m_HitCounter + "]", damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);
	#endif

		DayZPlayerImplement sourcePlayer;
		if (source)  //! Source can be null if actual source is not an EntityAI but (e.g.) a static object during a vehicle collision
		{
			Transport transport;
			if (Class.CastTo(transport, source))
				sourcePlayer = DayZPlayerImplement.Cast(transport.CrewMember(DayZPlayerConstants.VEHICLESEAT_DRIVER));
			else
				sourcePlayer = DayZPlayerImplement.Cast(source.GetHierarchyRootPlayer());
		}

		if (!m_ProcessDamage)
		{
			EntityAI rootEntity = m_Entity.GetHierarchyRoot();

			eAIBase ai;
			if (Class.CastTo(ai, sourcePlayer))
			{
			#ifdef DIAG_DEVELOPER
				bool dbgObjEnabled = DayZPlayerImplement.s_Expansion_DebugObjects_Enabled;
			#endif

				vector aiPos = ai.GetPosition();
				vector dir = vector.Direction(aiPos, modelPos);

				if (damageType == DT_FIRE_ARM)
				{
					//! Validate each shot.
					//! Work-around for 1st shot on new entity hitting previously hit entity due to vanilla bug with Weapon::Fire (T186177)
					//! The actual underlying issue is that the damage of each shot is only applied after the next shot has been fired.
					//! This can be tested by firing shots from a gun with a long reload time (e.g. a bolt-action rifle),
					//! or simply waiting between shots, and firing over a longer distance (several hundred meters) to ensure the projectile
					//! is in flight for a certain amount of time.
					//! The damage of the previous shot will be applied right at the moment the next shot is fired.

					//! Only redirect for root entity, children will be dealt with by parent dmg handler

					float time = GetGame().GetTickTime();

					eAIShot match;

					array<ref eAIShot> candidates = {};

					foreach (eAIShot shot: ai.m_eAI_FiredShots)
					{
					#ifdef DIAG_DEVELOPER
						EXTrace.Print(EXTrace.AI, ai, shot.GetInfo());
					#endif

						if (source == shot.m_Weapon && ammo == shot.m_Ammo)
						{
							//! Check if shot has not yet been processed or is within 5 ms of previous shot,
							//! well below fastest fire rate of any firearm i.e. consecutive hit due to penetrating projectile
							//! Only allow consecutive hits if correct entity

						/*
							vector toEntityDir = vector.Direction(shot.m_Origin, modelPos).Normalized();
							float dot = vector.Dot(shot.m_Direction, toEntityDir);

						#ifdef DIAG_DEVELOPER
							EXTrace.Print(EXTrace.AI, ai, "eAIShot direction=" + shot.m_Direction + " toEntityDir=" + toEntityDir + " dot=" + dot);
						#endif
						*/

							if (rootEntity == shot.m_HitObjectRoot /*|| dot >= 0.996*/)
							{
								if (!shot.m_ProcessedTime)
									shot.m_ProcessedTime = time;

								if (time - shot.m_ProcessedTime < 0.005)
								{
									match = shot;
									break;
								}
							}
							else if (rootEntity == m_Entity && !shot.m_ProcessedTime)
							{
								//! Only redirect for root entity, children will be dealt with by parent dmg handler

								candidates.Insert(shot);
							}
						}
					}

					if (!match)
					{
						foreach (eAIShot candidate: candidates)
						{
							//float airFriction;
							//float distance;
							//float initSpeed;
							//candidate.m_DamageCoef = candidate.m_Weapon.eAI_CalculateProjectileDamageCoefAtPosition(candidate.m_Origin, ammo, candidate.m_HitPosition, 1.0, airFriction, distance, candidate.m_SpeedCoef, initSpeed);

							//candidate.m_TravelTime = candidate.m_Weapon.eAI_CalculateProjectileTravelTime(airFriction, distance, initSpeed);
							float elapsed = time - candidate.m_Time;
							float travelTimeRemaining = candidate.m_TravelTime - elapsed;

							if (travelTimeRemaining > 0.05)
								GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(CheckCandidate, travelTimeRemaining * 1000, false, candidate, ai, modelPos, dir, travelTimeRemaining, dmgZone);
							else if (CheckCandidate(candidate, ai, modelPos, dir, travelTimeRemaining, dmgZone))
								break;
						}

					#ifdef DIAG_DEVELOPER
						if (!candidates.Count())
						{
							DayZPlayerImplement.s_Expansion_DebugObjects_Enabled = DayZPlayerImplement.s_eAI_DebugDamage;

							ai.Expansion_DebugObject(-5, modelPos - "0 1.5 0", "ExpansionDebugNoticeMe_Blue", Vector(dir[0], 0, dir[2]));
							ai.Expansion_DebugObject(-6, modelPos, "ExpansionDebugBox_Blue", Vector(dir[0], 0, dir[2]));

							DayZPlayerImplement.s_Expansion_DebugObjects_Enabled = dbgObjEnabled;

							EXTrace.Print(EXTrace.AI, ai, "Wrong entity hit " + ExpansionStatic.GetHierarchyInfo(m_Entity) + " and no candidates, ignoring dmg");
						#ifdef EXPANSION_AI_DMGDEBUG_CHATTY
							ExpansionStatic.MessageNearPlayers(modelPos, 100.0, "[" + ExpansionStatic.FormatFloat(time, 3, false) + "] ignore " + m_Entity + " " + dmgZone);
						#endif
						}
					#endif

						return false;
					}

				#ifdef EXPANSION_AI_DMGDEBUG_CHATTY
					DayZPlayerImplement p;
					if (m_Entity == rootEntity || (Class.CastTo(p, rootEntity) && p.m_eAI_DamageHandler.m_ProcessDamage))
						ExpansionStatic.MessageNearPlayers(match.m_HitPosition, 100.0, "[" + ExpansionStatic.FormatFloat(match.m_Time, 3, false) + "] dmgcalc " + m_Entity + " " + dmgZone + " travel " + ExpansionStatic.FormatFloat(match.m_TravelTime, 4, false) + " actual " + ExpansionStatic.FormatFloat(time - match.m_Time, 4, false));
				#endif
				}

			#ifdef DIAG_DEVELOPER
				ai.Expansion_DebugObject(-7, modelPos, "ExpansionDebugBox", Vector(dir[0], 0, dir[2]));

				DayZPlayerImplement.s_Expansion_DebugObjects_Enabled = dbgObjEnabled;
			#endif
			}

		#ifdef DIAG_DEVELOPER
			float damageMultiplier = GetDebugDamageMultiplier(damageResult, dmgZone);
		#else
			float damageMultiplier = 1.0;
		#endif

			DayZPlayerImplement player;
			bool isPlayerItem;
			if (Class.CastTo(player, rootEntity))
			{
				if (m_Entity.IsInventoryItem())
				{
					//! Entity is item on player

					if (player.m_eAI_DamageHandler.m_ProcessDamage)
						return true;  //! Process damage if player processes damage

					isPlayerItem = true;
				}

				if (player.IsAI())
					damageMultiplier *= player.m_eAI_DamageReceivedMultiplier;
			}

			if (sourcePlayer)
			{
				switch (damageType)
				{
					case DT_FIRE_ARM:
						if (ai)
						{
							if (sourcePlayer == m_Entity)
							{
								//! This shouldn't be possible because AI don't use suicide emote
								EXError.Warn(this, "WARNING: Game encountered an impossible state (AI damage source is firearm in AI's own hands)", {});
								return false;
							}

							damageMultiplier *= ai.m_eAI_DamageMultiplier;
						}

						break;

					case DT_CLOSE_COMBAT:
						eAIGroup group = sourcePlayer.GetGroup();
						if (group)
						{
							eAIFaction faction = group.GetFaction();
							float yeetForce;

							if (!isPlayerItem && !m_Entity.GetHierarchyParent())
							{
								eAIEntityTargetInformation info;

								if (player && player.Expansion_IsInVehicleCmd())
								{
									CarScript vehicle;
									if (Class.CastTo(vehicle, player.GetParent()))
										info = vehicle.GetTargetInformation();
								}
								else
								{
									info = m_TargetInformation;
								}

								if (info)
								{
									yeetForce = faction.GetMeleeYeetForce();
									eAIMeleeCombat.eAI_ApplyYeetForce(info, yeetForce, sourcePlayer.GetPosition(), faction.GetMeleeYeetFactors());
								}
							}

							//! @note for player targets, melee dmg mult above 1 and yeet are mutually exclusive
							//! since you can't send players flying in the moment of death
							float meleeDamageMultiplier = faction.GetMeleeDamageMultiplier();
							if (meleeDamageMultiplier < 1.0 || (meleeDamageMultiplier > 1.0 && (!player || (!isPlayerItem && !yeetForce))))
								damageMultiplier *= meleeDamageMultiplier;
						}

						break;
				}
			}

			if (damageMultiplier != 1.0)
			{
				damageMultiplier *= speedCoef;

			#ifdef DIAG_DEVELOPER
				EXTrace.Print(EXTrace.AI, m_Entity, ToString() + "::OnDamageCalculated[" + m_HitCounter + "] override dmg coef " + speedCoef + " -> " + damageMultiplier);
			#endif

				if (!isPlayerItem && damageMultiplier != 0.0)
				{
					//! Need to use Call() to avoid inconsistent damage
					GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Call(ProcessDamage, damageType, source, sourcePlayer, dmgZone, ammo, modelPos, damageMultiplier);
				}

				return false;
			}
		}
		else
		{
			m_ProcessDamage = false;
		}

		if (m_Entity.IsMan() && sourcePlayer && sourcePlayer != m_Entity)
			sourcePlayer.m_eAI_LastAggressionTime = ExpansionStatic.GetTime(true);  //! Aggro guards in area (if any)

		return true;
	}

	void ProcessDamage(int damageType, EntityAI source, DayZPlayerImplement sourcePlayer, string dmgZone, string ammo, vector modelPos, float damageCoef = 1.0)
	{
	#ifdef DIAG_DEVELOPER
		EXTrace.PrintHit(EXTrace.AI, m_Entity, ToString() + "::ProcessDamage[" + m_HitCounter + "]", null, damageType, source, -1, dmgZone, ammo, modelPos, damageCoef);
	#endif

		if (!source)
		{
			//! This can happen since this is not the same frame if (e.g.) AI weapon got deleted in the meantime due to being cloned

			if (sourcePlayer)
				source = sourcePlayer;
			else
				source = m_Entity;
		}

		m_ProcessDamage = true;
		m_Entity.ProcessDirectDamage(damageType, source, dmgZone, ammo, modelPos, damageCoef);
	}

	bool CheckCandidate(notnull eAIShot candidate, eAIBase ai, vector modelPos, vector dir, float travelTimeRemaining, string dmgZone)
	{
	#ifdef DIAG_DEVELOPER
		bool dbgObjEnabled = DayZPlayerImplement.s_Expansion_DebugObjects_Enabled;
	#endif

		if (candidate.m_ProcessedTime)
			return false;

		if (!candidate.m_HitObjectRoot)
			return false;

		vector minMax[2];
		eAIShot match;

		if (candidate.m_HitObjectRoot.GetCollisionBox(minMax))
		{
			//! Check if hit pos is within collision box of target entity at its current position
			vector start = candidate.m_HitObjectRoot.WorldToModel(candidate.m_HitPosition);
			float dist = Math.Min(minMax[1][0], minMax[1][2]) - Math.Min(minMax[0][0], minMax[0][2]) * 0.5;
			vector end = candidate.m_HitObjectRoot.WorldToModel(candidate.m_HitPosition + candidate.m_Direction * dist);

			if (Math3D.IntersectRayBox(start, end, minMax[0], minMax[1]) > -1)
				match = candidate;
		}

	/*
		//! Check if target is still at hit position (+/- 20 cm)
		vector modelPosWS = candidate.m_HitObjectRoot.ModelToWorld(candidate.m_HitPositionMS);
		if (vector.DistanceSq(candidate.m_HitPosition, modelPosWS) <= 0.04)
			match = candidate;
	*/
	
		if (match)
		{
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(CheckCandidate);

			if (match.m_HitObject == match.m_HitObjectRoot)
			{
				dmgZone = match.m_HitObjectRoot.GetDamageZoneNameByComponentIndex(match.m_Component);
			}
			else if (match.m_HitObjectRoot.IsMan())
			{
				if (!m_Entity.IsMan())
				{
					//! Do not redirect from non-human entity to Head or Brain, redirect to Torso instead
					if (s_HumanDmgZonesForRedirect.Find(dmgZone) == -1)
						dmgZone = "Torso";
				}
			}
			else if (match.m_HitObjectRoot.IsInherited(ZombieBase))
			{
				if (!m_Entity.IsZombie())
					dmgZone = "Head";
			}
			else if (match.m_HitObjectRoot.IsInherited(AnimalBase))
			{
				if (!m_Entity.IsAnimal())
					dmgZone = "Zone_Head";
			}

		#ifdef DIAG_DEVELOPER
			vector hitEntityDir = vector.Direction(match.m_Origin, match.m_HitPosition);

			DayZPlayerImplement.s_Expansion_DebugObjects_Enabled = DayZPlayerImplement.s_eAI_DebugDamage;

			ai.Expansion_DebugObject(-1, modelPos, "ExpansionDebugNoticeMe_Red", Vector(dir[0], 0, dir[2]));
			ai.Expansion_DebugObject(-2, modelPos, "ExpansionDebugBox_Red", Vector(dir[0], 0, dir[2]));
			ai.Expansion_DebugObject(-3, match.m_HitPosition, "ExpansionDebugNoticeMe", Vector(hitEntityDir[0], 0, hitEntityDir[2]));
			ai.Expansion_DebugObject(-4, match.m_HitPosition, "ExpansionDebugBox", Vector(hitEntityDir[0], 0, hitEntityDir[2]));

			DayZPlayerImplement.s_Expansion_DebugObjects_Enabled = dbgObjEnabled;

			EXTrace.Print(EXTrace.AI, ai, "Wrong entity hit " + ExpansionStatic.GetHierarchyInfo(m_Entity) + " dist " + dir.Length() + ", redirecting dmg to " + ExpansionStatic.GetDebugInfo(match.m_HitObjectRoot) + " dist " + hitEntityDir.Length() + " speedCoef=" + match.m_SpeedCoef + " travelTime=" + match.m_TravelTime + " remaining=" + travelTimeRemaining + " " + dmgZone + " damageCoef=" + match.m_DamageCoef);

		#ifdef EXPANSION_AI_DMGDEBUG_CHATTY
			ExpansionStatic.MessageNearPlayers(match.m_HitPosition, 100.0, "[" + ExpansionStatic.FormatFloat(match.m_Time, 3, false) + "] redirect " + match.m_HitObjectRoot + " travel " + ExpansionStatic.FormatFloat(match.m_TravelTime, 4, false));
		#endif
		#endif

			match.m_HitObjectRoot.ProcessDirectDamage(DT_FIRE_ARM, match.m_Weapon, dmgZone, match.m_Ammo, match.m_HitPosition, match.m_DamageCoef);

			return true;
		}
	#ifdef DIAG_DEVELOPER
		else
		{
			ai.m_eAI_FiredShots.RemoveItem(candidate);

			DayZPlayerImplement.s_Expansion_DebugObjects_Enabled = DayZPlayerImplement.s_eAI_DebugDamage;

			ai.Expansion_DebugObject(-5, modelPos - "0 1.5 0", "ExpansionDebugNoticeMe_Blue", Vector(dir[0], 0, dir[2]));
			ai.Expansion_DebugObject(-6, modelPos, "ExpansionDebugBox_Blue", Vector(dir[0], 0, dir[2]));

			DayZPlayerImplement.s_Expansion_DebugObjects_Enabled = dbgObjEnabled;

			EXTrace.Print(EXTrace.AI, ai, "Wrong entity hit " + ExpansionStatic.GetHierarchyInfo(m_Entity) + " and candidate wasn't hit, ignoring dmg");

		#ifdef EXPANSION_AI_DMGDEBUG_CHATTY
			ExpansionStatic.MessageNearPlayers(candidate.m_HitPosition, 100.0, "[" + ExpansionStatic.FormatFloat(candidate.m_Time, 3, false) + "] miss " + candidate.m_HitObjectRoot + " travel " + ExpansionStatic.FormatFloat(candidate.m_TravelTime, 4, false) + " elapsed " + ExpansionStatic.FormatFloat(GetGame().GetTickTime() - candidate.m_Time, 4, false));
		#endif
		}
	#endif

		return false;
	}
}
