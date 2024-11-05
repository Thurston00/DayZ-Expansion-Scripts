/**
 * Weapon_Base.c
 * 
 * Partly based on Enfusion AI Project Copyright 2021 William Bowers
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

modded class Weapon_Base
{
	static ref map<string, float> s_Expansion_MinSafeFiringDistance = new map<string, float>;

	float m_eAI_LastFiredTime;

	bool Hitscan(vector begin_point, vector direction, eAIBase ai, out Object hitObject, out vector hitPosition, out vector hitNormal, out int contactComponent)
	{
		return Hitscan(begin_point, direction, 1100.0, ai, hitObject, hitPosition, hitNormal, contactComponent);
	}

	bool Hitscan(vector begin_point, vector direction, float distance, Object ignore, out Object hitObject, out vector hitPosition, out vector hitNormal, out int contactComponent)
	{
		#ifdef EAI_TRACE
		auto trace = CF_Trace_0(this, "Hitscan");
		#endif

		vector end_point = begin_point + direction * distance;

		set<Object> results = new set<Object>();
		bool hit = DayZPhysics.RaycastRV(begin_point, end_point, hitPosition, hitNormal, contactComponent, results, this, ignore, false, false, ObjIntersectFire, 0.01);
		
		if (hit)
		{
			if (results.Count() > 0)
				hitObject = results[0];
			return true;
		}

		hitPosition = end_point;

		return false;
	}

	bool eAI_Fire(int muzzleIndex, eAIBase ai)
	{
	#ifdef SERVER
		vector pos = ai.GetBonePositionWS(ai.GetBoneIndexByName("neck"));
		vector dir = ai.GetWeaponAimDirection();

		eAITarget target = ai.GetTarget();
		EntityAI targetEntity = target.GetEntity();

		typename type = target.info.Type();

#ifdef DIAG_DEVELOPER
		if (EXTrace.AI && ai.m_eAI_LastEngagedTargetType != type)
			EXTrace.Print(true, this, "Last engaged target type " + type);
#endif

		ai.m_eAI_LastEngagedTargetType = type;

		Object ignore = ai;
		Object hitObject;
		vector hitPosition;
		vector hitNormal;
		int contactComponent;

		//! We check if we hit something in aim direction.
		//! Because projectiles may penetrate bushes, trees and even buildings,
		//! we continue the check until the hit object is not one of those, we exhaust the max distance or we reach max iterations.
		//! Note that it's not relevant here if the projectile cannot actually penetrate an obstacle, since that is checked by the damage handler.
		//! We are only interested in the object that could potentially be hit in the projectile's path.
		vector begPos = pos;
		float distance = 1100.0;
		bool hit;

		for (int i = 0; i < 3; i++)
		{
			hit = Hitscan(begPos, dir, distance, ignore, hitObject, hitPosition, hitNormal, contactComponent);

			if (hitObject && hitObject != targetEntity)
			{
				if (hitObject.IsBush() || hitObject.IsTree() || hitObject.IsBuilding() || hitObject.IsPlainObject())
				{
					float hitDist = vector.Distance(begPos, hitPosition);

					if (hitDist > 0)
					{
						distance -= hitDist;

						if (distance > 0)
						{
							begPos = begPos + dir * hitDist;
							ignore = hitObject;
							continue;
						}
					}
				}
			}

			break;
		}

		eAIShot shot;

		if (hitObject)
		{
			shot = new eAIShot(this, muzzleIndex, pos, ai.GetAimingProfile().GetAimDirection(), hitObject, hitPosition, contactComponent);
			ai.m_eAI_FiredShots.Insert(shot);
		}
		else
		{
			//! We didn't hit a valid target, but we still want to correct for bullet drop.
			//! Just set hit position to aim position as we only use it for drop calc.
			shot = new eAIShot(this, muzzleIndex, pos, ai.GetAimingProfile().GetAimDirection(), null, ai.GetAimPosition(), contactComponent);
		}

	#ifdef DIAG_DEVELOPER
		EXTrace.Print(EXTrace.AI, ai, shot.GetInfo());
	#endif

		eAI_DebugFire(hit, pos, dir, ai, target, hitObject, hitPosition);

		//! Compensate for bullet drop
		float drop = eAI_CalculateProjectileDrop(shot.m_TravelTime);
		if (drop > 0.0)
		{
			vector projecedPosition = pos + dir * shot.m_Distance;
			projecedPosition[1] = projecedPosition[1] + drop * 0.9;
			dir = vector.Direction(pos, projecedPosition).Normalized();
		}

		pos = pos + dir * 0.2;

		return Fire(muzzleIndex, pos, dir, vector.Forward);
	#else
		return TryFireWeapon(this, muzzleIndex);
	#endif
	}

	override void EEFired(int muzzleType, int mode, string ammoType)
	{
		super.EEFired(muzzleType, mode, ammoType);

		Man owner = GetHierarchyRootPlayer();

		if (GetGame().IsServer())
		{
			float time = GetGame().GetTickTime();
			if (time - m_eAI_LastFiredTime > 1.0)
			{
				//! Because shots may be fired rapidly, we only update this once every second
				m_eAI_LastFiredTime = time;
				float strengthMultiplier = GetPropertyModifierObject().eAI_GetNoiseShootModifier();
				if (strengthMultiplier)
					eAINoiseSystem.AddNoise(this, CFG_WEAPONSPATH + " " + GetType() + " NoiseShoot", strengthMultiplier, eAINoiseType.SHOT);
			}
		}
		else if (owner)
		{
			ExpansionGame exGame = GetDayZGame().GetExpansionGame();
			if (!exGame.m_FirearmFXSource || owner.GetIdentity())
				exGame.m_FirearmFXSource = this;
		}
	}

	void eAI_DebugFire(bool hit, vector begin_point, vector direction, eAIBase ai, eAITarget target, Object hitObject, vector hitPosition)
	{
#ifdef DIAG_DEVELOPER
		if (!EXTrace.AI)
			return;
		
		EXTrace.Print(true, this, "eAI_DebugFire " + direction);

		if (!hit)  //! Nothing hit
		{
			vector aimPosition = ai.GetAimPosition();
			float aimDistance = vector.Distance(begin_point, aimPosition);
			vector missedPosition = begin_point + (direction * aimDistance);
			string missed;
			float missedUD = Math.AbsFloat(missedPosition[1] - aimPosition[1]);
			float missedLR = vector.Distance(missedPosition, aimPosition) - missedUD;
			if (missedUD > missedLR)
			{
				if (missedPosition[1] > aimPosition[1])
					missed = "high";
				else
					missed = "low";
			}
			else
			{
				if (ExpansionMath.Side(begin_point, aimPosition, missedPosition) > 0)
					missed = "left";
				else
					missed = "right";
			}
			EXTrace.Print(true, ai, "didn't hit because it was aiming " + missed + " (actual " + aimPosition + " missed " + missedPosition + ")");

			ai.Expansion_DebugObject_Deferred(1818, "0 0 0", "ExpansionDebugBox");
			ai.Expansion_DebugObject_Deferred(1919, missedPosition, "ExpansionDebugBox_Red", direction, begin_point);
		}
		else
		{
			//EntityAI targetEntity = target.GetEntity();
			//if (targetEntity && targetEntity == hitObject)
			if (hitObject)
			{
				ai.Expansion_DebugObject_Deferred(1818, hitPosition, "ExpansionDebugBox", direction, begin_point);
				ai.Expansion_DebugObject_Deferred(1919, "0 0 0", "ExpansionDebugBox_Red");
			}
			else
			{
				ai.Expansion_DebugObject_Deferred(1818, "0 0 0", "ExpansionDebugBox");
				ai.Expansion_DebugObject_Deferred(1919, hitPosition, "ExpansionDebugBox_Red", direction, begin_point);
			}
		}
#endif
	}

	/**
	 * @brief calculate projectile speed coefficient at position based on ammo
	 * 
	 * @param ammoType  e.g. "Bullet_308"
	 * @param hitPosition  position of bullet impact
	 * @param [out] airFriction
	 * @param [out] distance
	 * 
	 * @return speed coefficient
	 */
	float eAI_CalculateProjectileSpeedCoefAtPosition(string ammoType, vector hitPosition, out float airFriction = 0.0, out float distance = 0.0)
	{
		//! Cannot use barrel position since highly inaccurate on server
		vector origin = GetPosition();

		return eAI_CalculateProjectileSpeedCoefAtPosition(origin, ammoType, hitPosition, airFriction, distance);
	}

	float eAI_CalculateProjectileSpeedCoefAtPosition(vector origin, string ammoType, vector hitPosition, out float airFriction = 0.0, out float distance = 0.0)
	{
		airFriction = GetGame().ConfigGetFloat("CfgAmmo " + ammoType + " airFriction");
		distance = vector.Distance(origin, hitPosition);

		return Math.Pow(Math.EULER, airFriction * distance);
	}

	/**
	 * @brief calculate projectile damage coefficient at position based on ammo
	 * 
	 * @param ammoType  e.g. "Bullet_308"
	 * @param hitPosition  position of bullet impact
	 * @param damageOverride
	 * @param [out] airFriction
	 * @param [out] distance
	 * @param [out] speedCoef
	 * @param [out] initSpeed
	 * @param [out] speed
	 * 
	 * @return damage coefficient
	 */
	float eAI_CalculateProjectileDamageCoefAtPosition(string ammoType, vector hitPosition, float damageOverride = 1.0, out float airFriction = 0.0, out float distance = 0.0, out float speedCoef = 0.0, out float initSpeed = 0.0, out float speed = 0.0)
	{
		//! Cannot use barrel position since highly inaccurate on server
		vector origin = GetPosition();

		return eAI_CalculateProjectileDamageCoefAtPosition(origin, ammoType, hitPosition, damageOverride, airFriction, distance, speedCoef, initSpeed, speed);
	}

	float eAI_CalculateProjectileDamageCoefAtPosition(vector origin, string ammoType, vector hitPosition, float damageOverride = 1.0, out float airFriction = 0.0, out float distance = 0.0, out float speedCoef = 0.0, out float initSpeed = 0.0, out float speed = 0.0)
	{
		initSpeed = GetGame().ConfigGetFloat("CfgAmmo " + ammoType + " initSpeed");
		float initSpeedMultiplier = ConfigGetFloat("initSpeedMultiplier");

		if (initSpeedMultiplier)
			initSpeed *= initSpeedMultiplier;

		float typicalSpeed = GetGame().ConfigGetFloat("CfgAmmo " + ammoType + " typicalSpeed");

		typicalSpeed *= damageOverride;

		speedCoef = eAI_CalculateProjectileSpeedCoefAtPosition(origin, ammoType, hitPosition, airFriction, distance);

		speed = initSpeed * speedCoef;

		float dmgCoef;

		if (typicalSpeed != initSpeed)
		{
			if (speed > typicalSpeed)
				dmgCoef = 1.0;
			else
				dmgCoef = speed / typicalSpeed;
		}
		else
		{
			dmgCoef = speedCoef;
		}

		return dmgCoef;
	}

	/**
	 * @brief Calculate projectile travel time over distance in seconds
	 */
	float eAI_CalculateProjectileTravelTime(float airFriction, float distance, float initSpeed, float simulationStep = 0.05)
	{
		float distanceTraveled;
		float distanceTraveledPrev;

		float timeTraveled;
		float timeTraveledPrev;

		float speed;

		//! In DayZ, max projectile travel time is 6 seconds
		while (distanceTraveled < distance && timeTraveled < 6.0)
		{
			distanceTraveledPrev = distanceTraveled;
			timeTraveledPrev = timeTraveled;
			speed = Math.Pow(Math.EULER, airFriction * distanceTraveled) * initSpeed;
			distanceTraveled += speed * simulationStep;
			timeTraveled += simulationStep;
		}

		if (distance)
			return ExpansionMath.LinearConversion(distanceTraveledPrev, distanceTraveled, distance, timeTraveledPrev, timeTraveled);

		return 0.0;
	}

	/**
	 * @brief Calculate projectile drop after travel time in meters
	 */
	float eAI_CalculateProjectileDrop(float travelTime)
	{
		float g = 9.81;  //! Gravity in m/s^2

		return 0.5 * g * Math.Pow(travelTime, 2.0);
	}

	/**
	 * @fn	ProcessWeaponEvent
	 * @brief	weapon's fsm handling of events
	 * @NOTE: warning: ProcessWeaponEvent can be called only within DayZPlayer::HandleWeapons (or ::CommandHandler)
	 */
	override bool ProcessWeaponEvent(WeaponEventBase e)
	{
		#ifdef EAI_TRACE
		auto trace = CF_Trace_1(this, "ProcessWeaponEvent").Add(e);
		#endif

		eAIBase ai;
		if (Class.CastTo(ai, e.m_player))
		{
			ScriptRemoteInputUserData ctx = new ScriptRemoteInputUserData;
			ctx.Write(INPUT_UDT_WEAPON_REMOTE_EVENT);
			e.WriteToContext(ctx);
			ai.StoreInputForRemotes(ctx);

			// @NOTE: synchronous events not handled by fsm
			if (e.GetEventID() == WeaponEventID.SET_NEXT_MUZZLE_MODE)
			{
				SetNextMuzzleMode(GetCurrentMuzzle());
				return true;
			}

			if (m_fsm.ProcessEvent(e) == ProcessEventResult.FSM_OK)
				return true;
				
			return false;
		}

		return super.ProcessWeaponEvent(e);
	}

	override bool ProcessWeaponAbortEvent(WeaponEventBase e)
	{
		if (PlayerBase.Cast(e.m_player).IsAI())
		{
			if (GetGame().IsServer())
			{
				ProcessEventResult result;
				m_fsm.ProcessAbortEvent(e, result);
				if (result == ProcessEventResult.FSM_OK)
					return true;
			}

			return false;
		}

		return super.ProcessWeaponAbortEvent(e);
	}

	override bool Expansion_TryTurningOnAnyLightsOrNVG(out float nightVisibility, PlayerBase player, bool skipNonNVG = false, bool skipNVG = false)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI, this);
#endif

		ItemOptics optic;
		if (!skipNVG && Class.CastTo(optic, GetAttachedOptics()) && optic.GetCurrentNVType() != NVTypes.NONE)
		{
			nightVisibility = optic.GetZeroingDistanceZoomMax() * 0.001;
			EXTrace.Print(EXTrace.AI, player, "switched on " + optic.ToString());
			return true;
		}

		if ( skipNonNVG )
			return false;

		ActionTarget atrg;
		ActionManagerClient mngr_client;
		CastTo(mngr_client, player.GetActionManager());
		atrg = new ActionTarget(this, null, -1, vector.Zero, -1.0);

		if ( mngr_client.GetAction(ActionTurnOnWeaponFlashlight).Can(player, atrg, this) )
		{
			ItemBase itemChild;

			if ( IsInherited(Rifle_Base) )
			{
				itemChild = ItemBase.Cast(FindAttachmentBySlotName("weaponFlashlight"));
			}
			else if (IsInherited(Pistol_Base))
			{
				itemChild = ItemBase.Cast(FindAttachmentBySlotName("pistolFlashlight"));
			}

			if ( itemChild && itemChild.Expansion_TryTurningOn() )
			{
				FlashlightOn();
				nightVisibility = 0.15;
				EXTrace.Print(EXTrace.AI, player, "switched on " + itemChild.ToString());
				return true;
			}
		}

		return false;
	}

	override bool Expansion_TryTurningOffAnyLightsOrNVG(PlayerBase player, bool skipNVG = false)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI, this);
#endif

		ActionTarget atrg;
		ActionManagerClient mngr_client;
		CastTo(mngr_client, player.GetActionManager());
		atrg = new ActionTarget(this, null, -1, vector.Zero, -1.0);

		if ( mngr_client.GetAction(ActionTurnOffWeaponFlashlight).Can(player, atrg, this) )
		{
			ItemBase itemChild;
			if ( IsInherited(Rifle_Base) )
			{
				itemChild = ItemBase.Cast(FindAttachmentBySlotName("weaponFlashlight"));
			}
			else if (IsInherited(Pistol_Base))
			{
				itemChild = ItemBase.Cast(FindAttachmentBySlotName("pistolFlashlight"));
			}

			if ( itemChild && itemChild.Expansion_TryTurningOff() )
			{
				FlashlightOff();
				EXTrace.Print(EXTrace.AI, player, "switched off " + itemChild.ToString());
				return true;
			}
		}

		return false;
	}

	float Expansion_GetMinSafeFiringDistance()
	{
		int muzzleIndex = GetCurrentMuzzle();

		string projectile;
		if (IsChamberFull(muzzleIndex))
			projectile = GetChamberedCartridgeMagazineTypeName(muzzleIndex);

		float minDist;

		if (!s_Expansion_MinSafeFiringDistance.Find(projectile, minDist))
		{
			float indirectHitRange = GetGame().ConfigGetFloat(CFG_AMMO + " " + projectile + " indirectHitRange");
			float indirectHitRangeMultiplier = GetGame().ConfigGetFloat(CFG_AMMO + " " + projectile + " indirectHitRangeMultiplier");

			minDist = indirectHitRange;
			if (indirectHitRangeMultiplier)
				minDist *= indirectHitRangeMultiplier;

		#ifdef DIAG_DEVELOPER
			EXTrace.Print(EXTrace.AI, this, projectile + " indirect hit range " + minDist);
		#endif

			//! TODO: Vanilla does this so awkward. The fired projectile is different from the explosive projectile
			//! which is only referenced in DayZGame::FirearmEffects, and there is no canonical way to get it from the fired projectile.
			//! Need to fall back to a hardcoded value for now :-(
			minDist = Math.Max(minDist, 20.0);

			s_Expansion_MinSafeFiringDistance[projectile] = minDist;
		}

		return minDist;
	}
};
