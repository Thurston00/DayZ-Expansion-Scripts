/**
 * ExpansionDamageSystem.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

//! Consistent explosion damage
class ExpansionDamageSystem
{
	static const float BLOCKING_ANGLE_THRESHOLD = 0.5;  //! Math.Cos(60 * Math.DEG2RAD);

	static ref map<EntityAI, ref map<string, ref map<string, int>>> s_ExplosionQueue = new map<EntityAI, ref map<string, ref map<string, int>>>;

	static bool IsEnabledForExplosionTarget(EntityAI target)
	{
		if (!GetExpansionSettings().GetDamageSystem().Enabled)
			return false;

		return ExpansionStatic.IsAnyOf(target, GetExpansionSettings().GetDamageSystem().ExplosionTargets, true);
	}

	static bool GetExplosionConfigValues(EntityAI source, inout string ammoType, out float explosionRange, out float explosionDropoffRange, out bool directional, out float indirectHitAngle1, out float indirectHitAngle2)
	{
		if (!ammoType)
			ammoType = source.ConfigGetString("ammoType");

		if (ammoType)
		{
			if (GetGame().ConfigIsExisting("CfgAmmo " + ammoType + " indirectHitRange"))
			{
				explosionDropoffRange = GetGame().ConfigGetFloat("CfgAmmo " + ammoType + " indirectHitRange");
			}
			else
			{
				EXError.WarnOnce(null, "WARNING: Ammo " + ammoType + " has no indirectHitRange! Assuming 1.0", {});
				explosionDropoffRange = 1.0;
			}

			if (GetGame().ConfigIsExisting("CfgAmmo " + ammoType + " indirectHitRangeMultiplier"))
			{
				explosionRange = explosionDropoffRange * GetGame().ConfigGetFloat("CfgAmmo " + ammoType + " indirectHitRangeMultiplier");
			}
			else
			{
				EXError.WarnOnce(null, "WARNING: Ammo " + ammoType + " has no indirectHitRangeMultiplier! Assuming 1.0", {});
				explosionRange = explosionDropoffRange;
			}

			directional = GetGame().ConfigGetTextOut("CfgAmmo " + ammoType + " explosionType") == "directional";
			indirectHitAngle1 = GetGame().ConfigGetFloat("CfgAmmo " + ammoType + " indirectHitAngle1");
			indirectHitAngle2 = GetGame().ConfigGetFloat("CfgAmmo " + ammoType + " indirectHitAngle2");

			return true;
		}
		else
		{
			EXError.ErrorOnce(null, "ERROR: " + source.GetType() + " has no ammo!", {});
		}

		return false;
	}

	static float GetExplosionDamage(EntityAI source, EntityAI target, string ammoType = "", out float dmgNominal = 0.0)
	{
		float explosionRange;
		float explosionDropoffRange;
		bool directional;
		float indirectHitAngle1;
		float indirectHitAngle2;
		GetExplosionConfigValues(source, ammoType, explosionRange, explosionDropoffRange, directional, indirectHitAngle1, indirectHitAngle2);

		if (ammoType)
		{
			dmgNominal = GetGame().ConfigGetFloat("CfgAmmo " + ammoType + " DamageApplied Health damage");
			return dmgNominal * GetExplosionDamageNormalized(source, target, explosionRange, explosionDropoffRange);
		}

		return 0.0;
	}

	static float GetExplosionDamageNormalized(EntityAI source, EntityAI target, float explosionRange, float explosionDropoffRange, vector position = vector.Zero)
	{
		if (position == vector.Zero)
			position = source.GetPosition();

		float blastDistance = vector.Distance(position, target.GetPosition());

		if (explosionRange > explosionDropoffRange)
			return ExpansionMath.LinearConversion(explosionRange, explosionDropoffRange, blastDistance);

		if (blastDistance > explosionDropoffRange)
			return 0.0;

		return 1.0;
	}

	static void OnBeforeExplode(EntityAI source, int damageType, string ammoType = "", vector position = vector.Zero)
	{
		if (!GetGame().IsServer() || damageType != DamageType.EXPLOSION || !GetExpansionSettings().GetDamageSystem().Enabled)
			return;

		if (position == vector.Zero)
			position = source.GetPosition();

		array<Object> nearest_objects = new array<Object>;
		array<CargoBase> proxy_cargos = new array<CargoBase>;

		float explosionRange;
		float explosionDropoffRange;
		bool directional;
		float indirectHitAngle1;
		float indirectHitAngle2;
		GetExplosionConfigValues(source, ammoType, explosionRange, explosionDropoffRange, directional, indirectHitAngle1, indirectHitAngle2);

		Log("About to explode " + ammoType + " from " + source + " at " + ExpansionStatic.VectorToString(position) + " " + typename.EnumToString(DamageType, damageType) + " range " + explosionRange + " dropoff " + explosionDropoffRange);

		map<EntityAI, float> targets();

		if (directional)
		{
			//! XXX: GetEntitiesInCone doesn't work very well, often includes objects that are not in cone
			//vector dir = source.GetDirection();
			//dir[1] = 0;
			//dir.Normalize();

			////! Using values for min/max height that are suitable for Claymore, and probably really should limit range or the search is going to be expensive
			//DayZPlayerUtils.GetEntitiesInCone(position, dir, indirectHitAngle1, explosionRange, -0.5, 2.0, nearest_objects);

			////! Annoyingly, GetEntitiesInCone can include the same object multiple times, so we have to deal with that
			//map<Object, bool> seen();

			vector sourceAngles = source.GetDirection().VectorToAngles();
		}
		//else
		//{
			GetGame().GetObjectsAtPosition3D(position, explosionRange, nearest_objects, proxy_cargos);
		//}

		ItemBase nearest_item;
		DayZPlayerImplement player;
		ExpansionVehicle vehicle;
		EntityAI nearest_entity;

		bool checkForBlockingObj = GetExpansionSettings().GetDamageSystem().CheckForBlockingObjects;
		bool targetHit;
		Object blockingObject;

		foreach (Object nearest_object: nearest_objects)
		{
			if (nearest_object == source)
				continue;

			if (!nearest_object.GetAllowDamage())
				continue;

			if (Class.CastTo(nearest_item, nearest_object))
			{
				if (!nearest_item.CanBeDamaged())
					continue;
				nearest_entity = nearest_item;
			}
			else if (Class.CastTo(player, nearest_object))
			{
				if (!player.Expansion_CanBeDamaged(ammoType))
					continue;
				nearest_entity = player;
			}
			else if (ExpansionVehicle.Get(vehicle, nearest_object))
			{
				if (!vehicle.CanBeDamaged())
					continue;
				nearest_entity = vehicle.GetEntity();
			}
			else if (!Class.CastTo(nearest_entity, nearest_object))
			{
				continue;
			}

			if (!IsEnabledForExplosionTarget(nearest_entity))
				continue;

			if (directional)
			{
				//if (seen.Contains(nearest_object))  //! Deal w/ GetEntitiesInCone including the same object multiple times
					//continue;

				//seen[nearest_object] = true;

				//! @note Important: Do not use position override here, use source position
				vector targetDir = vector.Direction(source.GetPosition(), nearest_object.GetPosition());
				vector targetAngles = targetDir.VectorToAngles();

				float hitAngle1;
				float hitAngle2;

				if (targetDir.LengthSq() > 1.0)
				{
					hitAngle1 = indirectHitAngle1 / 2.0;
					hitAngle2 = indirectHitAngle2 / 2.0;
				}
				else
				{
					//! Just check if roughly in front
					hitAngle1 = 90.0;
					hitAngle2 = 90.0;
				}

				if (Math.AbsFloat(ExpansionMath.AngleDiff2(sourceAngles[0], targetAngles[0])) > hitAngle1)
				{
#ifdef DIAG_DEVELOPER
					Log(nearest_object.ToString() + " at " + ExpansionStatic.VectorToString(nearest_object.GetPosition()) + " is NOT a target for " + source + " at " + ExpansionStatic.VectorToString(position) + " (not in blast direction)");
#endif
					continue;
				}

				if (Math.AbsFloat(ExpansionMath.AngleDiff2(sourceAngles[1], targetAngles[1])) > hitAngle2)
				{
#ifdef DIAG_DEVELOPER
					Log(nearest_object.ToString() + " at " + ExpansionStatic.VectorToString(nearest_object.GetPosition()) + " is NOT a target for " + source + " at " + ExpansionStatic.VectorToString(position) + " (not in blast direction)");
#endif
					continue;
				}
			}

			float explosionDamageNormalized = GetExplosionDamageNormalized(source, nearest_entity, explosionRange, explosionDropoffRange, position);
			if (!explosionDamageNormalized)
			{
#ifdef DIAG_DEVELOPER
				Log(nearest_object.ToString() + " at " + ExpansionStatic.VectorToString(nearest_object.GetPosition()) + " is NOT a target for " + source + " at " + ExpansionStatic.VectorToString(position) + " (too far away)");
#endif
				continue;
			}

			if (checkForBlockingObj)
				PerformRaycast(source, nearest_entity, position, targetHit, blockingObject);

			if (!blockingObject)
			{
#ifdef DIAG_DEVELOPER
				Log(nearest_object.ToString() + " at " + ExpansionStatic.VectorToString(nearest_object.GetPosition()) + " is a target for " + source + " at " + ExpansionStatic.VectorToString(position));
#endif
				targets.Insert(nearest_entity, explosionDamageNormalized);
				QueueExplosion(nearest_entity, source.ToString(), ammoType);
			}
#ifdef DIAG_DEVELOPER
			else
			{
				Log(nearest_object.ToString() + " at " + ExpansionStatic.VectorToString(nearest_object.GetPosition()) + " is NOT a target for " + source + " at " + ExpansionStatic.VectorToString(position) + " (blocking " + blockingObject + " is in the way)");
			}
#endif
		}

		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(OnAfterExplode, 34, false, source, source.ToString(), ExpansionStatic.VectorToString(position), damageType, ammoType, targets);
	}

	static bool PerformRaycast(EntityAI source, EntityAI target, vector position, out bool targetHit, out Object blockingObject)
	{
		vector hitPosition;
		vector hitNormal;
		int contactComponent;
		set<Object> results();

		targetHit = false;
		blockingObject = null;

		if (!DayZPhysics.RaycastRV(position, target.GetPosition(), hitPosition, hitNormal, contactComponent, results, null, source, true, false, ObjIntersectView, 0.1, CollisionFlags.ALLOBJECTS))
			return false;  //! Ray didn't hit anything, but this also means nothing is in the way

		vector offsetPos = position - source.GetDirection();
		vector targetDir = vector.Direction(position, target.GetPosition());
		vector targetDirNorm = targetDir.Normalized();
		float targetDistSq = targetDir.LengthSq();

		foreach (Object obj: results)
		{
			if (obj == target)
			{
				targetHit = true;
			}
			else if (obj.IsBuilding() || obj.ConfigGetString("physLayer") == "item_large")
			{
				vector objDirNorm = vector.Direction(offsetPos, obj.GetPosition()).Normalized();
				if (vector.Dot(objDirNorm, targetDirNorm) > BLOCKING_ANGLE_THRESHOLD && vector.DistanceSq(position, obj.GetPosition()) < targetDistSq)
				{
					blockingObject = obj;
					break;
				}
			}
		}

		return true;
	}

	static void OnAfterExplode(EntityAI source, string sourceIdentifier, string sourcePosition, int damageType, string ammoType, map<EntityAI, float> targets)
	{
		EntityAI directDamageSource = source;
		foreach (EntityAI target, float dmg: targets)
		{
			if (!target)
				continue;

			if (GetQueuedExplosionCount(target, sourceIdentifier, ammoType))
			{
				Log(target.ToString() + " at " + ExpansionStatic.VectorToString(target.GetPosition()) + " (destroyed " + target.IsDamageDestroyed() + ") was not hit by " + ammoType + " from " + sourceIdentifier + " at " + sourcePosition + " - applying direct " + typename.EnumToString(DamageType, damageType) + " damage coef " + dmg);

				if (!source)
				{
					directDamageSource = target;
					DequeueExplosion(target, sourceIdentifier, ammoType);
				}

				if (target.IsDamageDestroyed())
					DequeueExplosion(target, sourceIdentifier, ammoType);
				else
					target.ProcessDirectDamage(damageType, directDamageSource, "", ammoType, "0 0 0", dmg);
			}
		}
	}

	static void OnExplosionHit(EntityAI source, EntityAI target, string ammoType, bool applyDamageCorrection = false, TotalDamageResult damageResult = null, string dmgZone = string.Empty)
	{
	}

	static bool OnExplosionDamageCalculated(TotalDamageResult damageResult, EntityAI source, EntityAI target, int component, string dmgZone, string ammoType, vector modelPos, float speedCoef, bool applyDamageCorrection = false)
	{
		if (!IsEnabledForExplosionTarget(target))
			return true;

		if (!DequeueExplosion(target, source.ToString(), ammoType))
			return true;

		if (applyDamageCorrection && !target.IsDamageDestroyed() && !target.GetHierarchyParent() && source && !source.GetHierarchyRootPlayer())
		{
			//! Use our own damage system for consistent explosion damage
			//! Note that this only works as intended if damage source root is not a player,
			//! else won't be able to get actual source's position in relation to target

			float dmg = damageResult.GetDamage(dmgZone, "Health");

			float dmgDirectHit;
			float baseDmg = GetExplosionDamage(source, target, ammoType, dmgDirectHit) * speedCoef;
			if (baseDmg > dmg && dmgDirectHit > 0.0)
			{
				Log("Overriding " + source.ToString() + " damage dealt to " + target.ToString() + " at " + target.GetPosition() + " " + dmg.ToString() + " -> " + baseDmg.ToString() + " by dealing damage difference");

				float dmgCoef = (baseDmg - dmg) / dmgDirectHit;

				GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Call(ApplyExplosionDamage, source, target, dmgZone, ammoType, modelPos, dmgCoef);
				return false;
			}
		}

		return true;
	}

	static void ApplyExplosionDamage(EntityAI source, EntityAI target, string dmgZone, string ammoType, vector modelPos, float dmgCoef)
	{
		if (source && target)
			target.ProcessDirectDamage(DamageType.EXPLOSION, source, dmgZone, ammoType, modelPos, dmgCoef);
	}

	static int GetQueuedExplosionCount(EntityAI target, string sourceIdentifier, string ammoType)
	{
		if (!s_ExplosionQueue[target])
			return 0;

		if (!s_ExplosionQueue[target][sourceIdentifier])
			return 0;

		return s_ExplosionQueue[target][sourceIdentifier][ammoType];
	}

	static void QueueExplosion(EntityAI target, string sourceIdentifier, string ammoType)
	{
		if (!s_ExplosionQueue[target])
			s_ExplosionQueue[target] = new map<string, ref map<string, int>>;

		if (!s_ExplosionQueue[target][sourceIdentifier])
			s_ExplosionQueue[target][sourceIdentifier] = new map<string, int>;

		int count = s_ExplosionQueue[target][sourceIdentifier][ammoType] + 1;

		s_ExplosionQueue[target][sourceIdentifier][ammoType] = count;

		Log("Queued " + sourceIdentifier + " " + ammoType + " for " + target + " - count: " + count);
	}

	static bool DequeueExplosion(EntityAI target, string sourceIdentifier, string ammoType)
	{
		int count = GetQueuedExplosionCount(target, sourceIdentifier, ammoType);

		if (count)
		{
			count--;

			if (count)
			{
				s_ExplosionQueue[target][sourceIdentifier][ammoType] = count;
			}
			else
			{
				s_ExplosionQueue[target][sourceIdentifier].Remove(ammoType);
				if (!s_ExplosionQueue[target][sourceIdentifier].Count())
				{
					s_ExplosionQueue[target].Remove(sourceIdentifier);
					if (!s_ExplosionQueue[target].Count())
						s_ExplosionQueue.Remove(target);
				}
			}

			Log("Dequeued " + sourceIdentifier + " " + ammoType + " for " + target + " - count: " + count);

			return true;
		}

		return false;
	}

	static void Log(string msg)
	{
		auto settings = GetExpansionSettings().GetLog();
		if (settings.ExplosionDamageSystem)
			settings.PrintLog("[ExplosionDamageSystem] " + msg);
	}
}
