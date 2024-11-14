modded class CarScript
{
	private ref eAIVehicleTargetInformation m_TargetInformation = new eAIVehicleTargetInformation(this);
	ref eAIDamageHandler m_eAI_DamageHandler = new eAIDamageHandler(this, m_TargetInformation);

	eAIVehicleTargetInformation GetTargetInformation()
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_0(this, "GetTargetInformation");
#endif

		return m_TargetInformation;
	}

	override bool EEOnDamageCalculated(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		if (!super.EEOnDamageCalculated(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef))
			return false;

		if (!m_eAI_DamageHandler.OnDamageCalculated(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef))
			return false;

		return true;
	}

	override void EEKilled(Object killer)
	{
		m_TargetInformation.OnDeath(killer);

		super.EEKilled(killer);
	}

	override void EEHitBy(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
	#ifdef DIAG_DEVELOPER
		EXTrace.PrintHit(EXTrace.AI, this, "EEHitBy", damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);
	#endif

		m_TargetInformation.OnHit(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);

		super.EEHitBy(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);
	}

	override void EEHealthLevelChanged(int oldLevel, int newLevel, string zone)
	{
		m_TargetInformation.OnHealthLevelChanged(oldLevel, newLevel, zone);

		super.EEHealthLevelChanged(oldLevel, newLevel, zone);
	}

	override void OnInput(float dt)
	{
		super.OnInput(dt);

		eAIBase driver;
		if (!Class.CastTo(driver, CrewMember(DayZPlayerConstants.VEHICLESEAT_DRIVER)))
		{
			return;
		}

		SetBrake(1);
		SetHandbrake(1);

		if (!EngineIsOn())
		{
			EngineStart();
			return;
		}

		vector position = GetPosition();
		vector direction = GetDirection();

		vector wayPoint = position;
		bool isFinal = driver.m_PathFinding.GetNext(wayPoint);

		vector pathDir2D = vector.Direction(position, Vector(wayPoint[0], position[1], wayPoint[2]));
		float distance = pathDir2D.Normalize();

		float turn = vector.Dot(direction, pathDir2D);

		CarGear gear = CarGear.FIRST;

		if (turn < 0)
		{
			turn = -turn;

			gear = CarGear.REVERSE;
		}

		float steering = Math3D.AngleFromPosition(position, GetDirection(), wayPoint) / Math.PI;

		float rpmCoef = ExpansionMath.LinearConversion(0.0, EngineGetRPMRedline(), EngineGetRPM(), 1.0, 0.0);
		float turnCoef = Math.Max(turn, 0.1);
		float speedCoef = ExpansionMath.LinearConversion(0.0, 50.0, GetSpeedometerAbsolute(), 1.0, 0.0);
		float distanceCoef = ExpansionMath.LinearConversion(0.0, 50.0, distance, 0.0, 1.0);

		float throttle = turnCoef * speedCoef * distanceCoef * rpmCoef;
		float brake = (1.0 - speedCoef) * (1.0 - distanceCoef);

		SetSteering(steering);
		SetThrust(throttle);
		SetBrake(brake);

		ShiftTo(gear);

		SetHandbrake(0);
	}
};
