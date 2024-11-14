modded class WeaponFire
{
	override void OnEntry(WeaponEventBase e)
	{
		if (e)
		{
			eAIBase p;
			if (Class.CastTo(p, e.m_player))
			{
				m_dtAccumulator = 0;
		
				if (LogManager.IsWeaponLogEnable()) { wpnPrint("[wpnfsm] " + Object.GetDebugName(m_weapon) + " WeaponFire bang!"); }
				int mi = m_weapon.GetCurrentMuzzle();
				if (m_weapon.eAI_Fire(mi, p))
				{
					p.GetAimingModel().SetRecoil(m_weapon);
					m_weapon.OnFire(mi);
				}
				super.eAI_Vanilla_OnEntry(e);
				return;
			}
		}
		super.OnEntry(e);
	}
}

modded class WeaponFireWithEject
{
	override void OnEntry(WeaponEventBase e)
	{
		if (e)
		{
			eAIBase p;
			if (Class.CastTo(p, e.m_player))
			{
				m_dtAccumulator = 0;

				if (LogManager.IsWeaponLogEnable()) { wpnPrint("[wpnfsm] " + Object.GetDebugName(m_weapon) + " WeaponFire bang!"); }
				int mi = m_weapon.GetCurrentMuzzle();
				if (m_weapon.eAI_Fire(mi, p))
				{
					p.GetAimingModel().SetRecoil(m_weapon);
					m_weapon.EjectCasing(mi);
					m_weapon.EffectBulletHide(mi);
					m_weapon.OnFire(mi);
				}
				super.eAI_Vanilla_OnEntry(e);
				return;
			}
		}
		super.OnEntry(e);
	}
}

modded class WeaponFireMultiMuzzle
{
	override void OnEntry (WeaponEventBase e)
	{
		if (e)
		{
			eAIBase p;
			if (Class.CastTo(p, e.m_player))
			{
				m_dtAccumulator = 0;
		
				if (LogManager.IsWeaponLogEnable()) { wpnPrint("[wpnfsm] " + Object.GetDebugName(m_weapon) + " WeaponFire bang bang!"); }

				int mi = m_weapon.GetCurrentMuzzle();
				int b = m_weapon.GetCurrentModeBurstSize(mi);
				if(b > 1 )
				{
					
					for (int i = 0; i < b; i++)
					{
						if (m_weapon.eAI_Fire(i, p))
						{
							p.GetAimingModel().SetRecoil(m_weapon);
							m_weapon.OnFire(i);
						}
					}
				}
				else
				{
					if (m_weapon.eAI_Fire(mi, p))
					{
						p.GetAimingModel().SetRecoil(m_weapon);
						m_weapon.OnFire(mi);
					}
				}
				if(mi >= m_weapon.GetMuzzleCount() - 1 )
					m_weapon.SetCurrentMuzzle(0);
				else
					m_weapon.SetCurrentMuzzle(mi + 1);
				super.eAI_Vanilla_OnEntry(e);
				return;
			}
		}
		super.OnEntry(e);
	}
}

modded class WeaponFireToJam
{
	override void OnEntry (WeaponEventBase e)
	{
		if (e)
		{
			eAIBase p;
			if (Class.CastTo(p, e.m_player))
			{
				m_dtAccumulator = 0;

				if (LogManager.IsWeaponLogEnable()) { wpnPrint("[wpnfsm] " + Object.GetDebugName(m_weapon) + " WeaponFire bang! and jam?"); }
				int mi = m_weapon.GetCurrentMuzzle();
				if (m_weapon.eAI_Fire(mi, p))
				{
					m_weapon.SetJammed(true);
					p.GetAimingModel().SetRecoil(m_weapon);
					m_weapon.OnFire(mi);
				}
				m_weapon.ResetBurstCount();
				super.eAI_Vanilla_OnEntry(e);
				return;
			}
		}
		super.OnEntry(e);
	}
}
