class eAIItemTargetInformation: eAIEntityTargetInformation
{
	protected ItemBase m_Item;

	void eAIItemTargetInformation(EntityAI target)
	{
		Class.CastTo(m_Item, target);
	}

	void UpdateItemInfo()
	{
	}

	override bool IsInanimate()
	{
		return true;
	}

	override bool IsExplosive()
	{
		return m_Item.Expansion_IsExplosive();
	}

	override bool IsMechanicalTrap()
	{
		return m_Item.Expansion_IsMechanicalTrap();
	}

	override bool IsActive()
	{
		if (!super.IsActive())
			return false;

		Man player = m_Item.GetHierarchyRootPlayer();
		if (player && player.IsAlive() && (!player.IsUnconscious() || !player.GetIdentity()))
			return false;

		return true;
	}

	override vector GetPosition(eAIBase ai = null, bool actual = false)
	{
		return m_Item.GetCenter();
	}

	override float CalculateThreat(eAIBase ai = null)
	{
		if (m_Item.IsDamageDestroyed() || m_Item.IsSetForDeletion())
			return 0.0;

		Man player = m_Item.GetHierarchyRootPlayer();
		if (player && player.IsAlive() && (!player.IsUnconscious() || !player.GetIdentity()))
			return 0.0;  //! Has been picked up since we last saw it

		if (ai)
		{
			if (ai.eAI_GetThreatOverride(m_Item) || ai.IsRestrained())
				return 0.1;

			float distance = GetDistanceSq(ai, true);

			//if (distance > 900.0)
				//return 0.0;

			eAITarget target = ai.GetTarget();
			//! @note The dBodyIsactive check is to avoid checking items while they are thrown physically
			if (target && target.info == this && !dBodyIsActive(m_Item))
			{
				bool isUnreachable;
				bool isNear;
				//if (ai.GetPathFinding().m_TargetPosition == GetPosition(ai))
				//{
					if (ai.GetPathFinding().m_IsUnreachable || ai.GetPathFinding().m_IsTargetUnreachable)
					{
						isUnreachable = true;
					}
					else
					{
						vector waypoint = ai.GetPosition();
						if (ai.GetPathFinding().GetNext(waypoint) <= 2 && Math.IsPointInCircle(ai.GetPosition(), 0.55, waypoint))
							isNear = true;
					}
				//}
				//if (ai.eAI_IsUnreachable(distance, 4.0, m_Item.GetCenter()) || (distance <= 4.0 && ai.eAI_IsItemObstructed(m_Item)))
				if (isUnreachable || (isNear && ai.eAI_IsItemObstructed(m_Item)) || GetGame().GetTime() - target.found_at_time > target.max_time)
				{
					//! Item is above or below where AI can reach or item is obstructed
					ai.eAI_ThreatOverride(m_Item, true);
					return 0.1;
				}
			}

			bool canBandage;
			bool shouldBandage;
			//! https://feedback.bistudio.com/T173348
			if (m_Item.Expansion_CanBeUsedToBandage())
			{
				canBandage = true;
				shouldBandage = ai.eAI_ShouldBandage();
			}

			Magazine targetMag;

			if (canBandage && ((shouldBandage && ai.GetBandageToUse()) || ai.m_eAI_Bandages.Count() >= 3))
			{
				//! Don't need to pick up bandage if AI should bandage and already has at least one in inventory,
				//! or if AI shouldn't bandage and has 3 or more
				return 0.0;
			}
			else if (m_Item.IsWeapon() || m_Item.Expansion_IsMeleeWeapon() || canBandage || (Class.CastTo(targetMag, m_Item) && targetMag.GetAmmoCount()) || m_Item.IsClothing())
			{
				ItemBase itemInHands = ai.GetItemInHands();
				Weapon_Base gunInHands;
				Weapon_Base gun;
				bool hasAmmoForTargetFirearm;

				Magazine mag;
				if (Class.CastTo(gun, m_Item))
				{
					hasAmmoForTargetFirearm = ai.eAI_HasAmmoForFirearm(gun, mag, false);
				}

				if (!canBandage && itemInHands && !itemInHands.IsDamageDestroyed())
				{
					if (Class.CastTo(gunInHands, itemInHands) && m_Item.IsWeapon())
					{
						//! Ignore item while weapon manager or action is running (e.g. reload etc.)
						if (ai.IsRaised() || ai.GetWeaponManager().IsRunning() || ai.GetActionManager().GetRunningAction())
							return 0.0;

						if (gunInHands.Expansion_HasAmmo())
						{
							if (ai.eAI_ShouldPreferExplosiveAmmo())
							{
								//EXTrace.Print(EXTrace.AI, this, "Prefer explosive ammo, target " + ai.GetTarget().GetDebugName());
								if (gunInHands.ShootsExplosiveAmmo() || !m_Item.ShootsExplosiveAmmo())
								{
									//EXTrace.Print(EXTrace.AI, this, "Shoots explosive ammo: In hands " + gunInHands.ShootsExplosiveAmmo() + " on ground " + m_Item.ShootsExplosiveAmmo());
									return 0.0;
								}
							}
							else if (!hasAmmoForTargetFirearm || (mag && mag.IsAmmoPile() && !gun.HasInternalMagazine(gun.GetCurrentMuzzle())) || !ai.eAI_WeaponSelection(gunInHands, gun))
							{
								//! Ignore gun if we have no ammo/mag for it, or the mag we have for it is an ammo pile but the gun doesn't have an internal magazine, or our current gun in hands is considered better
								//EXTrace.Print(EXTrace.AI, this, "Prefer non-explosive ammo, target " + ai.GetTarget().GetDebugName());
								if (!gunInHands.ShootsExplosiveAmmo() || m_Item.ShootsExplosiveAmmo())
								{
									//EXTrace.Print(EXTrace.AI, this, "Shoots explosive ammo: In hands " + gunInHands.ShootsExplosiveAmmo() + " on ground " + m_Item.ShootsExplosiveAmmo());
									return 0.0;
								}
							}
						}
					}
					else if ((itemInHands.Expansion_IsMeleeWeapon() || gunInHands) && m_Item.Expansion_IsMeleeWeapon())
					{
						return 0.0;
					}
				}

				//! Ignore magazines and ammo if AI has no gun for it, or we have enough ammo
				if (targetMag)
				{
					if (!ai.eAI_HasWeaponForMagazine(targetMag))
						return 0.0;

					if (ai.m_eAI_IsUnlimitedReloadAll || (ai.m_eAI_LootingBehavior & eAILootingBehavior.UPGRADE) == 0)
					{
						if (ai.eAI_HasMagazineType(m_Item.GetType()))
							return 0.0;
					}
					else if (ai.eAI_GetMagazineTypeWithAmmoCount(m_Item.GetType()) >= 3)
					{
						return 0.0;
					}
				}

				if (gun)
				{
					if (target && target.IsMechanicalTrap() && (!itemInHands || itemInHands.Expansion_IsMeleeWeapon()))
						return 0.0;

					//! Ignore guns we don't have ammo for so we don't keep picking up guns that we can't use
					//! unless we don't yet have a weapon of that type (firearm or launcher)
					if (!hasAmmoForTargetFirearm)
					{
						if (m_Item.ShootsExplosiveAmmo())
						{
							if (ai.m_eAI_Launchers.Count() > 0)
								return 0.0;
						}
						else if (m_Item.IsKindOf("Pistol_Base"))
						{
							if (ai.m_eAI_Handguns.Count() > 0)
								return 0.0;
						}
						else if (ai.m_eAI_Firearms.Count() > 0)
						{
							return 0.0;
						}
					}
				}

				if (!ai.eAI_HasLOS(this))
				{
					//! Make sure we can react to other targets first
					distance = Math.Sqrt(distance);
					return ExpansionMath.PowerConversion(0.0, 227.968982, distance, 1.0, 0.0, 3.0);  //! Threat level 0.4 at 60 m
				}

				float q;
				if (m_Item.IsWeapon())
				{
					q = 1000000;
				}
				else if (m_Item.IsAmmoPile())
				{
					q = 1000;
				}
				else if (m_Item.Expansion_IsMeleeWeapon() || m_Item.IsMagazine())
				{
					q = 10000;
				}
				else if (canBandage && shouldBandage)
				{
					q = 900000900;  //! URGENT - other targets are clamped to 1000000, maxdist squared = 900 (30m) so adding 1 to make AI go for the bandage
				}
				else
				{
					distance = Math.Sqrt(distance);
					return ExpansionMath.PowerConversion(0.0, 227.968982, distance, 1.0, 0.0, 3.0);  //! Threat level 0.4 at 60 m
				}

				if (distance < 1.0)
					distance = 1.0;

				return q / distance;
			}
		}

		return 0.0;
	}

	override bool ShouldRemove(eAIBase ai = null)
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_1(this, "ShouldRemove").Add(ai);
#endif

		return GetThreat(ai) <= 0.1;
	}

	override void OnHealthLevelChanged(int oldLevel, int newLevel, string zone)
	{
		if (GetGame().IsClient())
			GetDayZGame().GetExpansionGame().m_FirearmFXHitObject = m_Item.GetHierarchyRoot();
	}
};
