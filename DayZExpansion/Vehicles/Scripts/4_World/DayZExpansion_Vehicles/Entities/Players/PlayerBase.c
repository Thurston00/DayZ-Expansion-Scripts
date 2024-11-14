/**
 * ExpansionPlayerBase.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

modded class PlayerBase
{
	private int m_ExpansionSaveVersion;

	protected bool m_Expansion_WasInVehicleSeatOrAttached;
	protected int m_Expansion_SessionTimeStamp;
	
	ref array<ExpansionCarKey> m_Expansion_CarKeys;

	void PlayerBase()
	{
		if (IsMissionHost())
			m_Expansion_SessionTimeStamp = GetDayZGame().ExpansionGetStartTimestamp(true);
		
		m_Expansion_CarKeys = new array<ExpansionCarKey>;
	}

	override void Init()
	{
		super.Init();
		
		if (!m_ExpansionST)
			m_ExpansionST = new ExpansionHumanST(this);
	}

	override void SetActions( out TInputActionMap InputActionMap )
	{
		super.SetActions( InputActionMap );

		AddAction( ExpansionActionHelicopterHoverRefill, InputActionMap );

		AddAction( ExpansionActionNextEngine, InputActionMap );
		AddAction( ExpansionActionNextEngineInput, InputActionMap );

		//! Unlock action on player so we can use it inside vehicle as well
		AddAction(ExpansionActionUnlockVehicle, InputActionMap);

		AddAction( ExpansionActionPickVehicleLock, InputActionMap );
		AddAction( ExpansionActionChangeVehicleLock, InputActionMap );

		#ifdef EXPANSION_VEHICLE_TOWING
		AddAction( ExpansionActionConnectTow, InputActionMap );
		AddAction( ExpansionActionDisconnectTow, InputActionMap );
		#endif

		AddAction( ExpansionActionSwitchSeats, InputActionMap );
	}
	
	override void EOnContact( IEntity other, Contact extra )
	{
		if( !IsAlive() || Expansion_GetParent() == other || !IsMissionHost() )
			return;

		Transport transport;
		if ( Class.CastTo( transport, other ) )
		{
			ExpansionRegisterTransportHit( transport );
		}

		ExpansionVehicleBase vehicle;
		if ( Class.CastTo( vehicle, other ) )
		{
			ExpansionRegisterTransportHit( vehicle );
		}
	}
	
	override void RegisterTransportHit( Transport transport )
	{
		// Preventing vanilla (and other mods) code from running
	}
	
	void ExpansionRegisterTransportHit( EntityAI transport )
	{
		bool hasParent = false;

		if ( Expansion_GetParent() )
			hasParent = true;

#ifdef DAYZ_1_25
		if ( m_ExPlayerLinkType != ExpansionPlayerLink.NONE )
			hasParent = true;
#endif

		if ( m_TransportHitRegistered )
			return;

		m_TransportHitRegistered = hasParent;

		if ( !m_TransportHitRegistered )
		{
			m_TransportHitRegistered = true;
			m_TransportHitVelocity = GetVelocity( transport );

			if ( m_TransportHitVelocity.Length() > 2.5 )
			{
				float damage = m_TransportHitVelocity.Length() * GetExpansionSettings().GetVehicle().VehicleRoadKillDamageMultiplier;
				if ( transport && damage )
					ProcessDirectDamage( DT_CUSTOM, transport, "", "TransportHit", "0 0 0", damage );
			} else
			{
				m_TransportHitRegistered = false;
			}

			if ( IsDamageDestroyed() && m_TransportHitVelocity.Length() > 3 )
			{
				vector impulse = 40 * m_TransportHitVelocity;
				impulse[1] = 40 * 1.5;
				dBodyApplyImpulse(this, impulse);
			}
		}
	}
	
	override bool ModCommandHandlerInside( float pDt, int pCurrentCommandID, bool pCurrentCommandFinished )	
	{
		if ( super.ModCommandHandlerInside( pDt, pCurrentCommandID, pCurrentCommandFinished ) )
		{
			return true;
		}

		if ( pCurrentCommandID == DayZPlayerConstants.COMMANDID_SCRIPT )
		{
			auto ehcv = ExpansionHumanCommandVehicle.Cast( GetCommand_Script() );
			if (ehcv)
			{
				ExpansionVehicleBase expansionVehicle;
				if (Class.CastTo(expansionVehicle, ehcv.GetObject()))
				{
					if (!ehcv.IsGettingIn() && !ehcv.IsGettingOut() && !ehcv.IsSwitchSeat())
					{
						expansionVehicle.HandleController(this, pDt);
					}
				}

				if ( ehcv.WasGearChange() )
				{
					ExpansionGearChangeActionCallback cb = ExpansionGearChangeActionCallback.Cast( AddCommandModifier_Action( DayZPlayerConstants.CMD_ACTIONMOD_SHIFTGEAR, ExpansionGearChangeActionCallback ) );
					cb.SetVehicleCommand( ehcv );
				}

				// Don't allow vanilla command handler to run from this point on, this is a replacement
				return true;
			}
		}

		if ( pCurrentCommandID == DayZPlayerConstants.COMMANDID_VEHICLE )
		{
			auto hcv = GetCommand_Vehicle();

			CarScript car;
			if (Class.CastTo(car, hcv.GetTransport()))
			{
				if (!hcv.IsGettingIn() && !hcv.IsGettingOut() && !hcv.IsSwitchSeat())
				{
					car.Expansion_HandleController(this, pDt);
				}
			}

			// Let vanilla handle the break for vehicles, this is an addition to functionality, not replacement
			return false;
		}

		return false;
	}

	override ExpansionHumanCommandVehicle StartCommand_ExpansionVehicle( ExpansionVehicleBase vehicle, int seatIdx, int seat_anim )
	{
		//WARNING: memory leak
		ExpansionHumanCommandVehicle cmd = new ExpansionHumanCommandVehicle( this, m_ExpansionST, vehicle, seatIdx, seat_anim, false );
		StartCommand_Script( cmd );
		return cmd;
	}

    override void OnDisconnect()
    {
        CarScript car;
        HumanCommandVehicle hcv = GetCommand_Vehicle();

        if (hcv && CarScript.CastTo(car, hcv.GetTransport()))
        {
            if (hcv.GetVehicleSeat() == DayZPlayerConstants.VEHICLESEAT_DRIVER && car.Expansion_IsTowing())
            {
				car.Expansion_DestroyTow();
            }
        }

        super.OnDisconnect();
    }

#ifdef DAYZ_1_25
	override void TryHideItemInHands(bool hide, bool force = false)
	{
		if (!hide && Expansion_IsAttached())
		{
			//! Vanilla turns false (= show) into true (= hide) if parent is transport (additional check added to IsInVehicle with 1.16). Force show item in hands.
			super.TryHideItemInHands(false, true);
			return;
		}
		
		super.TryHideItemInHands(hide, force);
	}
#endif

	bool Expansion_IsInVehicleSeatOrAttached( )
	{
	#ifndef DAYZ_1_25
		EXError.WarnOnce(this, "DEPRECATED");
	#endif

		return m_Expansion_WasInVehicleSeatOrAttached;
	}

	void Expansion_SetIsInVehicleSeatOrAttached( bool state )
	{
	#ifndef DAYZ_1_25
		EXError.WarnOnce(this, "DEPRECATED");
	#endif

		m_Expansion_WasInVehicleSeatOrAttached = state;
	}

	override bool IsInVehicle()
	{
#ifdef DAYZ_1_25
		if (Expansion_IsAttached())
			return false;
#endif
		
		if (super.IsInVehicle())
			return true;

		if (GetCommand_ExpansionVehicle())
			return true;

		return false;
	}

	bool Expansion_IsDriver()
	{
		return m_IsVehicleSeatDriver;
	}

	override void OnCommandExpansionVehicleStart()
	{
		m_AnimCommandStarting = HumanMoveCommandID.CommandVehicle;
		
		if ( GetInventory() )
			GetInventory().LockInventory(LOCK_FROM_SCRIPT);
		
		ItemBase itemInHand = GetItemInHands();
		EntityAI itemOnHead = FindAttachmentBySlotName("Headgear");

		if ( itemInHand && itemInHand.GetCompEM() )
			itemInHand.GetCompEM().SwitchOff();

		TryHideItemInHands(true);

		if ( itemOnHead && itemOnHead.GetCompEM() )
			itemOnHead.GetCompEM().SwitchOff();
		
		auto hcv = GetCommand_ExpansionVehicle();
		if ( hcv && hcv.GetVehicleSeat() == DayZPlayerConstants.VEHICLESEAT_DRIVER )
			OnVehicleSeatDriverEnter();
	}
	
	override void OnCommandExpansionVehicleFinish()
	{
		if ( GetInventory() )
			GetInventory().UnlockInventory(LOCK_FROM_SCRIPT);
		
		TryHideItemInHands(false, true);
		
		if ( m_IsVehicleSeatDriver )
			OnVehicleSeatDriverLeft();
	}

#ifdef DAYZ_1_25
	override void OnExpansionAttachTo( Object obj, vector transform[4] )
	{
		super.OnExpansionAttachTo(obj, transform);
		
		Expansion_SetIsInVehicleSeatOrAttached( true );
	}

	override void OnExpansionDetachFrom( Object obj )
	{
		super.OnExpansionDetachFrom(obj);

		Expansion_SetIsInVehicleSeatOrAttached( false );
	}
#endif

	override bool HeadingModel( float pDt, SDayZPlayerHeadingModel pModel )
	{
		return super.HeadingModel( pDt, pModel );
	}

	void SetHasCarKey(ExpansionCarKey key, bool state = true)
	{
		int idx = m_Expansion_CarKeys.Find(key);
		if (idx == -1 && state)
			m_Expansion_CarKeys.Insert(key);
		else if (!state)
			m_Expansion_CarKeys.Remove(idx);
	}

	ExpansionCarKey GetKeyForCar(CarScript car)
	{
		return GetKeyForCar((EntityAI) car);
	}

	ExpansionCarKey GetKeyForCar(EntityAI entity)
	{
		auto vehicle = ExpansionVehicle.Get(entity, true);

		if (!vehicle)
			return null;

		return GetKeyForCar(vehicle);
	}
	
	ExpansionCarKey GetKeyForCar(ExpansionVehicleBase vehicle)
	{
		return GetKeyForCar((EntityAI) vehicle);
	}

	ExpansionCarKey GetKeyForCar(ExpansionVehicle vehicle)
	{
		if (!vehicle.HasKey())
			return NULL;

		foreach (ExpansionCarKey key: m_Expansion_CarKeys)
		{
			if (key.IsInherited(ExpansionCarAdminKey))
				return key;

			if (key.IsPairedTo(vehicle))
				return key;
		}

		return NULL;
	}

	bool HasKeyForCar(CarScript car)
	{
		return HasKeyForCar((EntityAI) car);
	}

	bool HasKeyForCar(EntityAI entity)
	{
		if (GetKeyForCar(entity))
			return true;

		return false;
	}

	bool HasKeyForCar(ExpansionVehicle vehicle)
	{
		if (GetKeyForCar(vehicle))
			return true;

		return false;
	}
	
	bool HasKeyForCar(ExpansionVehicleBase vehicle)
	{
		return HasKeyForCar((EntityAI) vehicle);
	}

	/**
	 * @deprecated
	 */
	int GetExpansionSaveVersion()
	{
		return m_ExpansionSaveVersion;
	}

	#ifdef EXPANSION_MODSTORAGE
	override void CF_OnStoreSave(CF_ModStorageMap storage)
	{
		super.CF_OnStoreSave(storage);

		auto ctx = storage[DZ_Expansion_Vehicles];
		if (!ctx) return;

	#ifndef DAYZ_1_25
		//! 1.26+
		if (Expansion_GetParent())
			m_Expansion_WasInVehicleSeatOrAttached = true;
		else
			m_Expansion_WasInVehicleSeatOrAttached = false;
	#endif

		ctx.Write(m_Expansion_WasInVehicleSeatOrAttached);
		ctx.Write(m_Expansion_SessionTimeStamp);
	}
	
	override bool CF_OnStoreLoad(CF_ModStorageMap storage)
	{
		if (!super.CF_OnStoreLoad(storage))
			return false;

		auto ctx = storage[DZ_Expansion_Vehicles];
		if (!ctx) return true;

		if (!ctx.Read(m_Expansion_WasInVehicleSeatOrAttached))
			return false;

		if (!ctx.Read(m_Expansion_SessionTimeStamp))
			return false;

		return true;
	}
	#endif
	
	override void AfterStoreLoad()
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.CE, this, "LongDeferredInit");
#endif

		super.AfterStoreLoad();

		int sessionTimeStamp = m_Expansion_SessionTimeStamp;

		m_Expansion_SessionTimeStamp = GetDayZGame().ExpansionGetStartTimestamp(true);

		if ( m_Expansion_WasInVehicleSeatOrAttached )
		{
			ExpansionPPOGORIVMode mode = GetExpansionSettings().GetVehicle().PlacePlayerOnGroundOnReconnectInVehicle;

			if (mode == ExpansionPPOGORIVMode.Disabled)
				return;

			if (mode == ExpansionPPOGORIVMode.OnlyOnServerRestart && sessionTimeStamp == GetDayZGame().ExpansionGetStartTimestamp(true))
				return;

			//! Temporarily disable fall damage
			//! @note `FallDamage` is just a placeholder we use, it doesn't actually exist as ammo since DayZ 1.21
			Expansion_SetAllowDamageEx("FallDamage", false);

			//! CallLater so vehicle attachment code etc has a chance to run first
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(PlacePlayerOnGround, 1500);
		}
	}

	void PlacePlayerOnGround()
	{
		//todo: branchless ??
		if ( GetGame().IsServer() ) 
		{
			IEntity attachmentParent = Expansion_GetParent();

			EXPrint(ToString() + "::PlacePlayerOnGround - player pos " + GetPosition() + " was in vehicle " + m_Expansion_WasInVehicleSeatOrAttached + " is attached to " + attachmentParent);
			
			if (!attachmentParent)
			{
				vector rayStart = GetPosition() + "0 0.6 0";

				//! Ground or water surface position
				vector ground = ExpansionStatic.GetSurfaceWaterPosition(GetPosition());
				//EXPrint(ToString() + "::PlacePlayerOnGround - ground " + ground);

				//! Move ray end up a bit from ground, so that if (e.g.) we are standing on the LHD,
				//! our 1st raycast doesn't hit the (water) surface below (RaycastRV will ignore the LHD),
				//! which would place the player in the water, inside the LHD's model
				vector rayEnd = ground + "0 1.5 0";

				Object ignoreObj = this;
				
				RaycastRVParams params = new RaycastRVParams( rayStart, rayEnd, ignoreObj, 0.2 );
				params.sorted = true;
				//params.type = ObjIntersectFire;
				params.flags = CollisionFlags.ALLOBJECTS;
				
				array< ref RaycastRVResult > results = new array< ref RaycastRVResult >();
				bool haveValidResult;
				if ( DayZPhysics.RaycastRVProxy( params, results ) )
				{
					//EXPrint(ToString() + "::PlacePlayerOnGround - ray 1 hit results: " + results.Count());
					for (int i = 0; i < results.Count(); ++i)
					{
						RaycastRVResult currResult = results[i];
						if (!currResult)
							continue;
						
						Object obj;
						if (currResult.hierLevel > 0)
						{
							obj = currResult.parent;
						}	
						else
						{
							obj = currResult.obj;
						}
						
						//EXPrint(ToString() + "::PlacePlayerOnGround - ray 1 hit result hierlvl " + currResult.hierLevel + " parent " + currResult.parent + " obj " + currResult.obj + " hit pos " + currResult.pos);
						
						if ( !obj || obj.IsTree() || obj.IsBush() || obj.IsScriptedLight() || obj.GetType() == string.Empty)
							continue;
						
						//EXPrint(ToString() + "::PlacePlayerOnGround - ray 1 hit obj " + obj + " pos " + obj.GetPosition());
						if (obj.IsTransport())
						{
							//! If hit obj is more than 6m above ground, ignore it for next raycast
							if (ground[1] + 6 < obj.GetPosition()[1])
								ignoreObj = obj;
							//! Move up from hit pos for next raycast so that if we are standing on a vehicle,
							//! we don't get placed inside the vehicle's model
							rayStart[1] = currResult.pos[1] + 3;
							haveValidResult = false;
						}
						else
						{
							ground[1] = currResult.pos[1];
							haveValidResult = true;
						}
					}
				}

				if (!haveValidResult)
				{
					//! Do another raycast

					PhxInteractionLayers layerMask;

					layerMask |= PhxInteractionLayers.BUILDING;
					layerMask |= PhxInteractionLayers.VEHICLE;
					layerMask |= PhxInteractionLayers.ROADWAY;
					layerMask |= PhxInteractionLayers.TERRAIN;
					layerMask |= PhxInteractionLayers.ITEM_LARGE;

					vector hitPos;

					if (DayZPhysics.SphereCastBullet(rayStart, ground, 0.2, layerMask, ignoreObj, NULL, hitPos, NULL, NULL))
					{
						//EXPrint(ToString() + "::PlacePlayerOnGround - ray 2 hit pos " + hitPos);
						ground[1] = hitPos[1];
					}
				}

				//! If player is more than 1.5m above ground, place them safely,
				//! else just let them fall (no damage at that height)
				if (ground[1] + 1.5 < GetPosition()[1])
				{
					EXPrint(ToString() + "::PlacePlayerOnGround - placing player " + GetPosition() + " -> " + ground);
					SetPosition( ground );
				}

				m_Expansion_WasInVehicleSeatOrAttached = false;
			}

			//! Enable fall damage again - after a delay or player may still die from it
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(Expansion_SetAllowDamageEx, 1500, false, "FallDamage", true);
		}
	}
};
