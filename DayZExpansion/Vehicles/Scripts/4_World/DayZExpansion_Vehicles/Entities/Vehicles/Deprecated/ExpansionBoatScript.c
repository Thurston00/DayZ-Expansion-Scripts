/**
 * ExpansionBoatScript.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
 */

/**@class		ExpansionBoat
 * @brief		This class handle boat movement and physics
 **/
class ExpansionBoatScript: CarScript
{
	protected float m_BoatTime;

	protected float m_Offset;

	protected float m_TurnCoef;

	private float m_Thrust;
	private float m_ThrustTarget;

	private float m_Turn;
	private float m_TurnTarget;

	private Particle m_ParticleFirst;
	private Particle m_ParticleSecond;

	private Particle m_ParticleSideFirst;
	private Particle m_ParticleSideSecond;

	private Particle m_ParticleEngine;

	private float m_RotorAnimationPosition;

	private bool m_IsInitialized;

	float m_Expansion_SDSCheckTime;

	float m_ExpansionSink;

	void ExpansionBoatScript()
	{
		//! Values
		m_Offset = 0.75;
		m_ExpansionSink = 1.0;

		int i;
		int count;

		string path;

		/*
		path = "CfgVehicles " + GetType() + " SimulationModule Props";
		count = GetGame().ConfigGetChildrenCount(path);

		for (i = 0; i < count; i++)
		{
			string propName;
			GetGame().ConfigGetChildName(path, i, propName);

			string propPath = path + " " + propName;
			AddModule(new ExpansionVehicleProp(this, propPath));
		}

		path = "CfgVehicles " + GetType() + " SimulationModule Buoyancy";
		count = GetGame().ConfigGetChildrenCount(path);

		for (i = 0; i < count; i++)
		{
			string buoyancyName;
			GetGame().ConfigGetChildName(path, i, buoyancyName);

			string buoyancyPath = path + " " + buoyancyName;
			AddModule(new ExpansionVehicleBuoyantPoint(this, buoyancyPath));
		}

		path = "CfgVehicles " + GetType() + " SimulationModule Throttle";
		AddModule(new ExpansionVehicleCarThrottle(this, path));
		*/

		m_EngineStartOK = "offroad_engine_start_SoundSet";
		m_EngineStartBattery = "offroad_engine_failed_start_battery_SoundSet";
		m_EngineStartPlug = "offroad_engine_failed_start_sparkplugs_SoundSet";
		m_EngineStartFuel = "offroad_engine_failed_start_fuel_SoundSet";
		m_EngineStopFuel = "offroad_engine_stop_fuel_SoundSet";
		
		m_CarDoorOpenSound = "offroad_door_open_SoundSet";
		m_CarDoorCloseSound = "offroad_door_close_SoundSet";

		//if ( GetGame().IsServer() )
		//{
			//int selectionIndex = GetHiddenSelectionIndex("antiwater");
			//SetObjectTexture(selectionIndex, "dz\\data\\data\\antiwater_ca.paa");
			//SetObjectMaterial(selectionIndex, "dayzexpansion\\particles\\materials\\world\\data\\expansion_material_antiwater.rvmat");
			//GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Call(UpdateVisuals);
		//}
	}

	void ~ExpansionBoatScript()
	{
		if (m_ParticleEngine)
		{
			m_ParticleEngine.Stop();
		}

		if (m_ParticleFirst)
		{
			m_ParticleFirst.Stop();
		}

		if (m_ParticleSecond)
		{
			m_ParticleSecond.Stop();
		}

		if (m_ParticleSideFirst)
		{
			m_ParticleSideFirst.Stop();
		}

		if (m_ParticleSideSecond)
		{
			m_ParticleSideSecond.Stop();
		}
	}

	override void LongDeferredInit()
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.VEHICLES, this, "LongDeferredInit");
#endif

		super.LongDeferredInit();

		m_IsInitialized = true;
	}

	override void EEHitBy(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		super.EEHitBy(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);

		if (!dmgZone || dmgZone == "GlobalHealth")
		{
			//! Always damage engine proportionally when taking global damage
			float engineMaxHealth = GetMaxHealth("Engine", "");
			float engineHealth = GetHealth("Engine", "");
			float engineHealthNew = engineMaxHealth * GetHealth01(dmgZone, "");
			if (engineHealthNew < engineHealth)
			{
				SetHealth("Engine", "", engineHealthNew);
			}
		}
	}

	override void SetActions()
	{
		super.SetActions();

	#ifdef DAYZ_1_25
		if (!IsInherited(ExpansionLHD))
			AddAction(ExpansionActionBoardBuoyantObject);
	#endif
	}

	override void Expansion_OnHandleController(DayZPlayerImplement driver, float dt)
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_2(ExpansionTracing.VEHICLES, this, "Expansion_OnHandleController").Add(driver).Add(dt);
#endif

		float steering;
		float brake;
		float thrust;

		if (driver)
		{
			UAInterface input = driver.GetInputInterface();

#ifdef COMPONENT_SYSTEM
			float forward = input.SyncedValue("UAExpansionBoatMoveForward");
			float backward = input.SyncedValue("UAExpansionBoatMoveBackward");
			float left = input.SyncedValue("UAExpansionBoatRotateLeft");
			float right = input.SyncedValue("UAExpansionBoatRotateRight");
			float turbo = input.SyncedValue("UAExpansionBoatTurbo");
#else
			float forward = input.SyncedValue_ID(UAExpansionBoatMoveForward);
			float backward = input.SyncedValue_ID(UAExpansionBoatMoveBackward);
			float left = input.SyncedValue_ID(UAExpansionBoatRotateLeft);
			float right = input.SyncedValue_ID(UAExpansionBoatRotateRight);
			float turbo = input.SyncedValue_ID(UAExpansionBoatTurbo);
#endif

			steering = right - left;
			brake = backward;
			thrust = forward;
		}
		//! 1.19
		int gear = GetGear();
		
		if (brake > 0)
		{
			m_ThrustTarget = -m_State.m_LinearVelocityMS[2] * (1.0 / m_MaxSpeedMS) * 2.0;
		}
		else
		{
			float ratio = 0;

			if (gear == CarGear.REVERSE)
			{
				thrust *= -1.0;
				if ( thrust > 0.0 )
					thrust = -thrust;

				steering = -steering;
			}
			else if (gear == CarGear.NEUTRAL)
			{
				thrust = 0;

				steering = 0;
			}
			else
			{
				thrust *= 1.0;
				if ( thrust < 0.0 )
					thrust = -thrust;

				steering = steering;
			}

			m_TurnTarget += Math.Clamp(steering - m_TurnTarget, -40.0 * dt, 40.0 * dt);
			m_ThrustTarget += Math.Clamp(thrust - m_ThrustTarget, -40.0 * dt, 40.0 * dt);
		}

		//! Not used ATM
		m_Controller.SetSteering(m_TurnTarget);
		m_Controller.SetThrottle(thrust);
		m_Controller.SetBrake(brake);
	}

	protected override void OnParticleUpdate(float pDt)
	{
		vector enginePosition = ModelToWorld(GetMemoryPointPos("engine"));
		//! TODO: Make particles for reverse gear too
		if (enginePosition[1] <= GetGame().SurfaceGetSeaLevel() && Math.AbsFloat(GetSpeedometer()) > 5)
		{
			if (!m_ParticleEngine && MemoryPointExists("engine"))
			{
				m_ParticleEngine = Particle.PlayOnObject(ParticleList.EXPANSION_BOAT_ENGINE, this, GetMemoryPointPos("engine"), Vector(0, 90, 0));
			}

			if (!m_ParticleFirst && MemoryPointExists("waterdecal"))
			{
				m_ParticleFirst = Particle.PlayOnObject(ParticleList.EXPANSION_BOAT_DECAL, this, GetMemoryPointPos("waterdecal") + Vector(0, 0, 2.5), Vector(-180, 0, 0));
			}

			if (!m_ParticleSideFirst && MemoryPointExists("waterstream1_side"))
			{
				m_ParticleSideFirst = Particle.PlayOnObject(ParticleList.EXPANSION_BOAT_WATER, this, GetMemoryPointPos("waterstream1_side"), Vector(-25, 90, 0));
			}

			if (!m_ParticleSideSecond && MemoryPointExists("waterstream2_side"))
			{
				m_ParticleSideSecond = Particle.PlayOnObject(ParticleList.EXPANSION_BOAT_WATER, this, GetMemoryPointPos("waterstream2_side"), Vector(25, 90, 0));
			}
		}
		else
		{
			if (m_ParticleEngine)
			{
				m_ParticleEngine.Stop();
			}

			if (m_ParticleFirst)
			{
				m_ParticleFirst.Stop();
			}

			if (m_ParticleSecond)
			{
				m_ParticleSecond.Stop();
			}

			if (m_ParticleSideFirst)
			{
				m_ParticleSideFirst.Stop();
			}

			if (m_ParticleSideSecond)
			{
				m_ParticleSideSecond.Stop();
			}
		}
	}

	protected override void OnAnimationUpdate(float pDt)
	{
		m_RotorAnimationPosition += m_Thrust * 10 * 0.0001;

		if (m_RotorAnimationPosition >= 1)
			m_RotorAnimationPosition -= 1;

		SetAnimationPhase("rotor", m_RotorAnimationPosition);

		SetAnimationPhase("compasspointer", GetOrientation()[0] * Math.DEG2RAD);

		SetAnimationPhase("drivingWheel", m_Controller.GetSteering());

		super.OnAnimationUpdate(pDt);
	}

	override protected void CheckVitalItem(bool isVital, string itemName)
	{
		if (!isVital)
			return;

		EntityAI item = FindAttachmentBySlotName(itemName);

		if (!item)
			Expansion_EngineStop(1);
		else if (item.IsRuined())
			Expansion_EngineStop(1);
	}

#ifdef CF_DebugUI
	override bool CF_OnDebugUpdate(CF_Debug instance, CF_DebugUI_Type type)
	{
		super.CF_OnDebugUpdate(instance, type);

		instance.Add("Turn Target", m_TurnTarget);
		instance.Add("Thrust Target", m_ThrustTarget);

		instance.Add("Turn", m_Turn);
		instance.Add("Thrust", m_Thrust);

		return true;
	}
#endif

	protected override void OnSimulation(ExpansionPhysicsState pState)
	{
		vector force;
		vector torque;
		float pDt = pState.m_DeltaTime;

		bool isAboveWater;
		float buoyancyForce;

		if (m_ExpansionSink < 0.1)
		{
			if (dBodyIsActive(this))
			{
				// floaty mc float
			}

			return;
		}

		// To whomever reads this, this will be re-written
		// Will be re-written soon :)

		if (!Expansion_EngineIsOn(1))
		{
			m_TurnTarget = 0;
			m_ThrustTarget = 0;
		}

		vector tForce = vector.Zero;
		vector tCenter = vector.Zero;

		float change;

		change = m_ThrustTarget - m_Thrust;
		m_Thrust += Math.Clamp(change, -1.0 * pDt, 1.0 * pDt);
		m_Thrust = Math.Clamp(m_Thrust, -1.0, 1.0);

		change = m_TurnTarget - m_Turn;
		m_Turn += Math.Clamp(change, -1.0 * pDt, 1.0 * pDt);
		m_Turn = Math.Clamp(m_Turn, -1.0, 1.0);

		float linVel = 0;
		if (m_MaxSpeedMS > 0)
			linVel = m_State.m_LinearVelocityMS[2] * (1.0 / m_MaxSpeedMS);

		buoyancyForce = ExpansionPhysics.CalculateBuoyancyAtPosition(GetPosition(), m_Offset, m_State.m_Mass, 0.5, m_State.m_LinearVelocity, isAboveWater);

		if ( !isAboveWater )
		{
			bool allDoorsClosed = m_ExpansionVehicle.AllDoorsClosed();

			if ( allDoorsClosed && !m_EngineDestroyed && !m_Exploded )
			{
				m_ExpansionSink += pDt * 0.001;
			}
			else
			{
				if ( !allDoorsClosed )
					m_ExpansionSink -= pDt * 0.025;

				if ( m_EngineDestroyed )
					m_ExpansionSink -= pDt * 0.025;

				if ( m_Exploded )
					m_ExpansionSink -= pDt * 0.05;
			}
		}
		else if ( !m_EngineDestroyed && !m_Exploded )
		{
			if ( m_ExpansionVehicle.AllDoorsClosed() )
			{
				m_ExpansionSink += pDt * 0.005;
			} else {
				m_ExpansionSink += pDt * 0.05;
			}
		}

		if ( m_ExpansionSink > 1.0 )
		{
			m_ExpansionSink = 1.0;
		}
		else if (m_ExpansionSink < 0.001)
		{
			m_ExpansionSink = 0.0;
		}

		buoyancyForce = buoyancyForce * m_ExpansionSink;

		float waterContactCoef = Math.Clamp(Math.Sign(buoyancyForce), 0, 1);

		// thrust
		if (buoyancyForce > 0)
		{
			float thrust = (3.0 * m_Thrust) - (2.0 * linVel);

			tForce[0] = 0;
			tForce[1] = 0;
			tForce[2] = thrust * m_State.m_Mass;

			float linVelAbs = Math.AbsFloat(linVel);

			float thrustCoef = 4.0 * linVelAbs;
			thrustCoef = Math.Clamp(thrustCoef, 0.0, 4.0);
			thrustCoef *= thrustCoef;

			tCenter[0] = -m_Turn * m_BoundingRadius * thrustCoef * m_TurnCoef;
			tCenter[1] = -Math.Clamp(m_BoundingRadius * 2.0, 4, 8);
			tCenter[2] = -m_BoundingRadius;

			force += tForce;
			torque += tCenter * tForce;
		}

		// friction
		if (buoyancyForce > 0)
		{
			force += GetLinearFrictionForce();
		}

		// convert forces to worldspace
		{
			force = force.Multiply3(m_State.m_Transform);
			torque = torque.Multiply3(m_State.m_Transform);
		}

		// bouyancy forces
		if (buoyancyForce > 0)
		{
			tForce[0] = 0;
			tForce[1] = buoyancyForce;
			tForce[2] = 0;

			force += tForce;
		}

		// stabilize
		if (buoyancyForce > 0)
		{
			// https://www.youtube.com/watch?v=weUDuqA6dF4?t=9
			vector upWanted = Vector(0, 1, 0);
			vector estOrient = m_State.EstimateDirection(0.025, 1);

			vector stabilize = vector.Zero;
			stabilize[0] = upWanted[0] - estOrient[0];
			stabilize[1] = 0;
			stabilize[2] = upWanted[2] - estOrient[2];

			// convert to local space
			stabilize = stabilize.InvMultiply3(m_State.m_Transform);

			// in local space, limit the axis of movement
			stabilize[0] = Math.Clamp(stabilize[0], -0.06, 0.06);
			stabilize[2] = Math.Clamp(stabilize[2], -0.3, 0.3);

			// convert to world space
			stabilize = stabilize.Multiply3(m_State.m_Transform);

			// apply 800N*mass of torque to keep the ship upright, and then copy this to bike scripts
			torque += Vector(0, 400.0 * m_State.m_Mass, 0) * stabilize * waterContactCoef;
		}

		dBodySetDamping(this, 0.0, 0.5);

		pState.m_Force += force;
		pState.m_Torque += torque;
	}

	override void EOnPostSimulate(IEntity other, float timeSlice)
	{
		m_BoatTime += timeSlice;

		if (m_BoatTime >= GameConstants.CARS_FLUIDS_TICK)
		{
			m_BoatTime = 0;

			if (GetGame().IsServer() && Expansion_EngineIsOn(1))
			{
				if (GetFluidFraction(CarFluid.FUEL) <= 0 || m_EngineHealth <= 0)
					Expansion_EngineStop(1);

				CheckVitalItem(IsVitalTruckBattery(), "TruckBattery");
				CheckVitalItem(IsVitalGlowPlug(), "GlowPlug");
			}
		}
	}

	override void OnUpdate( float dt )
	{
		super.OnUpdate( dt );

		if ( m_ExpansionVehicle.AllDoorsClosed() )
			m_DrownTime = 0;  //! Prevent vanilla engine drown damage
	}

	private vector GetLinearFrictionForce()
	{
		vector friction = vector.Zero;

		friction[0] = -Math.SquareSign(m_State.m_LinearVelocityMS[0]) * m_State.m_Mass * 0.8;
		friction[1] = -Math.SquareSign(m_State.m_LinearVelocityMS[1]) * m_State.m_Mass * 0.8;

		friction[2] = -Math.SquareSign(m_State.m_LinearVelocityMS[2]) * m_State.m_Mass * 0.0001;

		return friction;
	}

	override void ExpansionOnExplodeClient(int damageType, string ammoType)
	{
		super.ExpansionOnExplodeClient(damageType, ammoType);
	}

	override void ExpansionOnSpawnExploded()
	{
		super.ExpansionOnSpawnExploded();

		if (m_ParticleEngine)
			m_ParticleEngine.Stop();

		if (m_ParticleFirst)
			m_ParticleFirst.Stop();

		if (m_ParticleSecond)
			m_ParticleSecond.Stop();
	}

	override void DeferredInit()
	{
		super.DeferredInit();

		HideSelection("hiderotorblur");
		ShowSelection("hiderotor");

		//! There is no need to run any of the below on client in multiplayer
		if (ExpansionGame.IsMultiplayerClient())
			return;

		SetVelocity(this, "0 0 0");
		dBodySetAngularVelocity(this, "0 0 0");

		vector position = GetPosition();
		vector orientation = GetOrientation();
		if (ExpansionStatic.SurfaceIsWater(position))
		{
			float waterLineCorrection = 1.08; //! Value works for all boats, makes waterline roughly match simulated waterline
			float tideCorrection = GetTideCorrection(position);
			if (tideCorrection > 0.2)
				waterLineCorrection += tideCorrection - 0.2; //! If surface is sea, place boat roughly below half point of tide to make it easier to climb in
			float depth = g_Game.GetWaterDepth(position);
			position[1] = position[1] + depth + m_Offset - waterLineCorrection;
			SetPosition(position);
			SetOrientation(Vector(orientation[0], 0, 0));
		}

		//! Activate boat so it doesn't sink to the sea floor if spawned by (e.g.) admin tool
		//! and not loaded from storage/spawned by CE
		if (!m_Expansion_IsStoreLoaded && !m_ExpansionVehicle.IsCECreated())
		{
			m_IsInitialized = true;
			dBodyActive(this, ActiveState.ACTIVE);
		}
	}

	float GetTideCorrection(vector position)
	{
		if (GetGame().SurfaceIsSea(position[0], position[2]))
		{
			vector waterSurfacePos = ExpansionStatic.GetSurfaceWaterPosition(position);
			//! Based on Chernarus, where lowest and highest tide point are roughly 0.320801 and 0.820602, respectively
			if (waterSurfacePos[1] > 0.32 && waterSurfacePos[1] < 1.0)
				return waterSurfacePos[1] - 0.32;
		}

		return 0;
	}

	override int Get3rdPersonCameraType()
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.VEHICLES, this, "Get3rdPersonCameraType");
#endif

		return DayZPlayerCameras.DAYZCAMERA_3RD_VEHICLE;
	}

	override int GetAnimInstance()
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.VEHICLES, this, "GetAnimInstance");
#endif

		return VehicleAnimInstances.HATCHBACK;
	}

	override bool Expansion_CanSimulate()
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.VEHICLES, this, "Expansion_CanSimulate");
#endif

		if ((GetGame().IsServer() && GetGame().IsMultiplayer()) && !m_IsInitialized)
			return false;

		return true;
	}

	override bool Expansion_ShouldDisableSimulation()
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.VEHICLES, this, "Expansion_ShouldDisableSimulation");
#endif

		//! Vanilla WILL NOT deactivate boats that are in water, so we need to do that ourself (handled in CarScript::EOnSimulate if no driver).
		//! If a boat is on land though, DON'T return true here as it may cause boats to get pushed into the ground while vanilla collision code is running
		//! (CarScript doesn't have collision in inactive state, so will move through terrain as if it weren't there if it's pushed by an outside force).
		//! Vanilla WILL deactivate boats that are not in use and are on land the same as it does cars,
		//! so there is no need for us to do that explicitly in that case.
		if (!ExpansionStatic.SurfaceIsWater(GetPosition()))
			return false;

		//! Make sure to let simulation run for at least 20s so that collision can be handled by vanilla, otherwise boat may get deactivated
		//! in CarScript::EOnSimulate prematurely if no driver and get pushed under water by still running vanilla collision code
		if (GetGame().GetTickTime() - m_Expansion_SDSCheckTime < 20)
			return false;

		vector velocity = GetVelocity(this);
		// if velocity is greater than gravity in either direction then that means the boat is not floating
		if (Math.AbsFloat(velocity[1]) > Math.AbsFloat(dGetGravity(this)[1]))
			return false;

		// checking the velocity speed to see if it is moving (pushed/pulled/towed)
		velocity[1] = 0;
		if (velocity.LengthSq() > 0.1)
			return false;

		if (m_ThrustTarget > 0.001)
			return false;

		if (m_Thrust > 0.001)
			return false;

		bool result = true;

		float tideCorrection = GetTideCorrection(GetPosition());
		if (tideCorrection > 0)
		{
			//! If surface is sea, deactivate boat at roughly below half point of tide to make it easier to climb in again
			if (tideCorrection > 0.2 && tideCorrection < 0.21)
				result = true;
			else
				result = false;
		}

		if (result)
			m_Expansion_SDSCheckTime = GetGame().GetTickTime();

		return result;
	}

	override int GetSeatAnimationType(int posIdx)
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_1(ExpansionTracing.VEHICLES, this, "GetSeatAnimationType").Add(posIdx);
#endif

		switch (posIdx)
		{
		case 0:
			return DayZPlayerConstants.VEHICLESEAT_DRIVER;
		case 1:
			return DayZPlayerConstants.VEHICLESEAT_CODRIVER;
		default:
			return 0;
		}

		return 0;
	}

	override bool CrewCanGetThrough(int posIdx)
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_1(ExpansionTracing.VEHICLES, this, "CrewCanGetThrough").Add(posIdx);
#endif

		return true;
	}

	override bool IsVitalGlowPlug()
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.VEHICLES, this, "IsVitalGlowPlug");
#endif

		return true;
	}

	override bool IsVitalTruckBattery()
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.VEHICLES, this, "IsVitalTruckBattery");
#endif

		return true;
	}

	override bool IsVitalCarBattery()
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.VEHICLES, this, "IsVitalHelicopterBattery");
#endif

		return false;
	}

	override bool IsVitalRadiator()
	{
		return false;
	}

	override void OnVariablesSynchronized()
	{
		super.OnVariablesSynchronized();
	}

	/**
	 * @brief This updates the sound for the car.
	 *
	 * @param ctrl, sound control (in config) which will be updated
	 * @param oldValue, engine defined value for the sound control
	 */
	override float OnSound(CarSoundCtrl ctrl, float oldValue)
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_2(ExpansionTracing.VEHICLES, this, "OnSound").Add(ctrl).Add(oldValue);
#endif

		if (Expansion_EngineIsOn(0))
		{
			return super.OnSound(ctrl, oldValue);
		}

		if (Expansion_EngineIsOn(1))
		{
			switch (ctrl)
			{
			case CarSoundCtrl.RPM:
			{
				float speed = Math.AbsFloat(GetSpeedometer());
				if (speed > 100)
				{
					return 50;
				}
				else
				{
					return speed / 2;
				}

				break;
			}

			case CarSoundCtrl.ENGINE:
			{
				return 1;
			}
			}

			return oldValue;
		}

		return super.OnSound(ctrl, oldValue);
	}

	override CarLightBase CreateRearLight()
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.VEHICLES, this, "CreateRearLight");
#endif

		return CarRearLightBase.Cast(ScriptedLightBase.CreateLight(ExpansionRearBoatLights));
	}

	override bool CanReachDoorsFromSeat(string pDoorsSelection, int pCurrentSeat)
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_2(ExpansionTracing.VEHICLES, this, "CanReachDoorsFromSeat").Add(pDoorsSelection).Add(pCurrentSeat);
#endif

		return true;
	}

	override bool CanReachSeatFromDoors(string pSeatSelection, vector pFromPos, float pDistance = 1.0)
	{
		return true;
	}

	override bool Expansion_IsCar()
	{
		return false;
	}

	override bool Expansion_IsBoat()
	{
		return true;
	}

	override vector GetTransportCameraOffset()
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.VEHICLES, this, "GetTransportCameraOffset");
#endif

		return "0 1.5 0";
	}

	override float GetTransportCameraDistance()
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.VEHICLES, this, "GetTransportCameraDistance");
#endif

		return 3.5;
	}

	override void UpdateVisuals()
	{
		ShowSelection("antiwater");
	}
};
