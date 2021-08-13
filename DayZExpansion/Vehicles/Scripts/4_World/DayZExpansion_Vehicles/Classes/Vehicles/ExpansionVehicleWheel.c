/**
 * ExpansionVehicleWheel.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2021 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

class ExpansionVehicleWheel
{
	private static float s_SUSP_DEBUG_LENGTH = 0.0;

	private string m_InventorySlot;
	private string m_Selection;
	private string m_Name;
	private int m_Index;
	private ExpansionVehicleAxle m_Axle;
	private ExpansionWheelBase m_WheelItem;
	private ExpansionVehicleBase m_Vehicle;

	private ref Transform m_TransformWS;
	private ref Transform m_TransformMS;
	private ref Matrix3 m_RotationMatrix;

	private vector m_InitialWheelPositionMS;

	private vector m_WheelDirectionMS;
	private vector m_WheelAxleMS;

	private vector m_SuspensionOffset;

	// wheel animations
	private string m_AnimTurn;
	private string m_AnimRotation;
	private string m_AnimDamper;
	private string m_WheelHub;

	// Suspension
	private bool m_HasContact;
	private Object m_ContactObject;
	private vector m_ContactPosition;
	private vector m_ContactNormal;
	private float m_ContactFraction;
	private float m_ContactLength;
	
	private vector m_TraceStart;
	private float m_TraceUp;
	private float m_TraceDown;

	private vector m_ContactPositionWS;
	private vector m_ContactNormalWS;

	private float m_SuspensionLength;
	private float m_SuspensionLengthPrevious;
	
	private float m_SuspensionFraction;
	private float m_SuspensionDistance;
	private float m_RestLength;

	private float m_SuspensionForce;

	private float m_SuspensionRelativeVelocity;
	private float m_SuspensionInvContact;

	private vector m_ContactVelocity;

	private vector m_TireLongDir;
	private vector m_TireLatDir;
	private float m_TireLongSpeed;
	private float m_TireLatSpeed;

	private vector m_RayStartMS;
	private vector m_RayStartWS;
	private vector m_RayEndMS;
	private vector m_RayEndWS;

	private vector m_ForwardWS;
	private vector m_AxleWS;

	private float m_AngularVelocity;
	private float m_AngularRotation;
	
	private float m_Mass;

	private float m_WheelTorque;
	private float m_BrakeTorque;
	
	private float m_Steering;
	
	private string m_Surface;
	
	private ref array< vector > m_WheelVertexPositions;

	private vector m_Impulse;
	private vector m_ImpulseTorque;

	private float m_WheelDepth;
	private float m_SurfaceFriction;

	private float m_WheelAcceleration;
	private float m_ForwardImpulse;
	private float m_SideDot;
	private float m_SideImpulse;
	private vector m_ForwardImp;
	private vector m_SideImp;

	void ExpansionVehicleWheel( ExpansionVehicleBase pVehicle, ExpansionVehicleAxle pAxle, string pName, int pIdx )
	{
		m_WheelVertexPositions = new array< vector >();
		
		m_Name = pName;
		m_Index = pIdx;
		m_Vehicle = pVehicle;
		m_Axle = pAxle;

		m_TransformWS = new Transform;
		m_TransformMS = new Transform;
		m_RotationMatrix = new Matrix3;

		m_WheelDirectionMS = "0 -1 0";
		m_WheelAxleMS = "-1 0 0"; // TODO: get from model
		
		string path;

		path = "CfgVehicles " + m_Vehicle.GetType() + " SimulationModule Axles " + m_Axle.GetName() + " Wheels " + m_Name + " inventorySlot";
		m_InventorySlot = GetGame().ConfigGetTextOut( path );

		path = "CfgVehicles " + m_Vehicle.GetType() + " SimulationModule Axles " + m_Axle.GetName() + " Wheels " + m_Name + " animTurn";
		m_AnimTurn = GetGame().ConfigGetTextOut( path );
		
		path = "CfgVehicles " + m_Vehicle.GetType() + " SimulationModule Axles " + m_Axle.GetName() + " Wheels " + m_Name + " animRotation";
		m_AnimRotation = GetGame().ConfigGetTextOut( path );
		
		path = "CfgVehicles " + m_Vehicle.GetType() + " SimulationModule Axles " + m_Axle.GetName() + " Wheels " + m_Name + " animDamper";
		m_AnimDamper = GetGame().ConfigGetTextOut( path );
		
		path = "CfgVehicles " + m_Vehicle.GetType() + " SimulationModule Axles " + m_Axle.GetName() + " Wheels " + m_Name + " wheelHub";
		m_WheelHub = GetGame().ConfigGetTextOut( path );

		m_InitialWheelPositionMS = GetCenterPositionSelection( "geometry", m_WheelHub );
		m_RestLength = m_Axle.GetTravelMax() * 0.5;

		string n_axis_start;
		string n_axis_end;

		m_SuspensionOffset = "0 0 0";

		path = "CfgVehicles " + m_Vehicle.GetType() + " SimulationModule Axles " + m_Axle.GetName() + " Wheels " + m_Name + " axis_start";
		if ( GetGame().ConfigGetText( path, n_axis_start ) )
		{
			path = "CfgVehicles " + m_Vehicle.GetType() + " SimulationModule Axles " + m_Axle.GetName() + " Wheels " + m_Name + " axis_end";
			if ( GetGame().ConfigGetText( path, n_axis_end ) )
			{
				m_SuspensionOffset = m_Vehicle.GetMemoryPointPos( n_axis_start );
				m_WheelDirectionMS = vector.Direction( m_Vehicle.GetMemoryPointPos( n_axis_end ), m_SuspensionOffset ).Normalized();
				m_SuspensionOffset = m_InitialWheelPositionMS - m_SuspensionOffset;
			}
		}
	}

	void ~ExpansionVehicleWheel()
	{
		delete m_TransformWS;
		delete m_TransformMS;
		delete m_RotationMatrix;
		
		delete m_WheelVertexPositions;
	}

	float GetAngularVelocity()
	{
		return m_AngularVelocity;
	}
	
	float GetRPM()
	{
		if (m_WheelItem)
			return ( m_AngularVelocity * 30.0 * 0.10472 ) / ( Math.PI * m_WheelItem.m_Radius * m_WheelItem.m_Radius );
		
		return 0;
	}

	void SetSteering( float pSteering )
	{
		m_Steering = pSteering;
	}

	void ApplyTorque( float pTorque )
	{
		m_WheelTorque = pTorque;
	}

	void ApplyBrake( float pTorque )
	{
		m_BrakeTorque = pTorque;
	}
	
	private vector GetCenterPositionSelection( string pLODName, string pSelectionName )
	{
		LOD lod = m_Vehicle.GetLODByName( pLODName );
		if ( !lod )
			return "0 0 0";

		Selection selection = lod.GetSelectionByName( pSelectionName );
		if ( !selection )
			return "0 0 0";
		
		vector offset = vector.Zero; // m_WheelDirectionMS * m_Axle.GetWheelHubRadius() * -0.5;
		
		vector totalVertices = "0 0 0";
		int count = selection.GetVertexCount();
		for ( int i = 0; i < count; ++i )
		{
			vector vp = lod.GetVertexPosition( selection.GetLODVertexIndex( i ) ) + offset;
			totalVertices += vp;
			m_WheelVertexPositions.Insert( vp );
		}
		
		vector center = Vector( totalVertices[0] / count, totalVertices[1] / count, totalVertices[2] / count );
		m_WheelVertexPositions.Insert( center );
		return center;
	}

	string GetInventorySlot()
	{
		return m_InventorySlot;
	}

	ExpansionVehicleAxle GetAxle()
	{
		return m_Axle;
	}

	float GetSuspensionLength()
	{
		return m_SuspensionLength;
	}

	float GetSuspensionFraction()
	{
		return m_SuspensionFraction;
	}

	vector GetSuspensionContactPosition()
	{
		return m_ContactPosition;
	}

	vector GetSuspensionContactNormal()
	{
		return m_ContactNormal;
	}

	vector GetInitialWheelPosition()
	{
		return m_InitialWheelPositionMS;
	}

	void OnWheelAttach( notnull ExpansionWheelBase pWheel )
	{		
		m_WheelItem = pWheel;
		
		m_Axle.UpdateWheelRadius();
	}

	void OnWheelDetach()
	{
		m_WheelItem = NULL;
	}
	
	ExpansionWheelBase GetItem()
	{
		return m_WheelItem;
	}

	#ifdef CF_DebugUI
	bool CF_OnDebugUpdate(CF_Debug instance, CF_DebugUI_Type type)
	{
		instance.Add("Wheel", m_Axle.GetName() + "::" + m_Name + " | " + m_WheelHub + " | " + m_WheelItem);

		instance.Add("Impulse", m_Impulse);
		instance.Add("Impulse Torque", m_ImpulseTorque);

		instance.Add("Ray Start (MS)", m_RayStartMS );
		instance.Add("Ray End (MS)", m_RayEndMS );
		instance.Add("Ray Start (WS)", m_RayStartWS );
		instance.Add("Ray End (WS)", m_RayEndWS );

		instance.Add("Previous", m_Vehicle.GetAnimationPhase( m_AnimDamper ) );
		instance.Add("New", m_SuspensionFraction );

		instance.Add("Has Contact", m_HasContact );
		instance.Add("Contact Velocity", m_ContactVelocity );

		instance.Add("Wheel Depth", m_WheelDepth );
		instance.Add("Suspension Length", m_SuspensionLength );
		instance.Add("Suspension Fraction", m_SuspensionFraction );
		instance.Add("Suspension Force", m_SuspensionForce );
		instance.Add("Trace Up", m_TraceUp );
		instance.Add("Trace Down", m_TraceDown );
		instance.Add("Travel Max Down", m_Axle.GetTravelMaxDown() );

		instance.Add("[" + m_Index + "] Friction", m_SurfaceFriction );
		instance.Add("[" + m_Index + "] Torque", (m_WheelTorque * m_Mass) );
		instance.Add("[" + m_Index + "] Brake", m_BrakeTorque );

		instance.Add("[" + m_Index + "] Acceleration", m_WheelAcceleration );
		instance.Add("[" + m_Index + "] Force", m_ForwardImpulse );

		instance.Add("[" + m_Index + "] m_SideImp", m_SideImp );

		return true;
	}
	#endif

	void SetupSimulation( float pDt, out int pNumWheelsGrounded )
	{
		if ( !m_WheelItem )
			return;
		
		m_Mass = dBodyGetMass( m_Vehicle );
		
		vector suspensionOffset = (m_RayEndMS - m_InitialWheelPositionMS) + ((m_SuspensionLength + m_WheelItem.m_Radius - m_Axle.GetWheelHubRadius()) * -m_WheelDirectionMS);

		vector wheelTransform[4];
		wheelTransform[0] = -m_WheelAxleMS;
		wheelTransform[1] = -m_WheelDirectionMS;
		wheelTransform[2] = "0 0 1";
		wheelTransform[3] = m_SuspensionOffset;
		
		vector rotationTransform[4];
		Math3D.YawPitchRollMatrix( Vector( m_Steering, 0, 0 ), rotationTransform );
		rotationTransform[3] = m_WheelVertexPositions[m_WheelVertexPositions.Count() - 1] + suspensionOffset;

		Math3D.MatrixMultiply4( rotationTransform, wheelTransform, m_TransformMS.data );

		Suspension( pDt );

		m_TransformWS = m_Vehicle.m_Transform.Multiply( m_TransformMS );

		if ( m_HasContact )
		{
			pNumWheelsGrounded++;
		}

		#ifndef EXPANSION_WHEEL_DEBUG_DISABLE
		for ( int j = 0; j < m_WheelVertexPositions.Count() - 2; ++j )
		{
			vector wvps = m_WheelVertexPositions[j] + suspensionOffset - m_TransformMS[3];
			vector wvpe = m_WheelVertexPositions[j + 1] + suspensionOffset - m_TransformMS[3];
			
			vector pts[2];
			pts[0] = wvps.Multiply4( m_TransformWS.data );
			pts[1] = wvpe.Multiply4( m_TransformWS.data );
			
			int color = 0xFF00FF00;
			if ( !m_HasContact )
				color = 0xFFFF0000;
			
			#ifndef EXPANSION_DEBUG_SHAPES_DISABLE
			m_Vehicle.DBGAddShape( Shape.CreateLines( color, ShapeFlags.NOZBUFFER, pts, 2 ) );
			#endif
		}

		#ifndef EXPANSION_DEBUG_SHAPES_DISABLE
		m_Vehicle.DBGAddShape( Shape.CreateSphere( 0xFF0000FF, ShapeFlags.WIREFRAME | ShapeFlags.NOZBUFFER, m_TransformMS[3].Multiply4( m_Vehicle.m_Transform.data ), 0.05 ) );
		#endif
		#endif
	}

	void Simulate( float pDt, int pNumWheelsGrounded, out vector pImpulse, out vector pImpulseTorque )
	{		
		if ( !m_WheelItem )
			return;

		Suspension( pDt, pNumWheelsGrounded, m_Impulse, m_ImpulseTorque );
		Friction( pDt, pNumWheelsGrounded, m_Impulse, m_ImpulseTorque );

		// convert wheel forces to world space
		m_Impulse = m_Impulse.Multiply3( m_Vehicle.m_Transform.data );
		m_ImpulseTorque = m_ImpulseTorque.Multiply3( m_Vehicle.m_Transform.data );
		
		pImpulse += m_Impulse;
		pImpulseTorque += m_ImpulseTorque;
	}

	void Animate( float pDt, bool pPhysics )
	{
		if ( !pPhysics && m_WheelItem )
		{
			vector rotationTransform[4];
			Math3D.YawPitchRollMatrix( Vector( -m_Steering, 0, 0 ), rotationTransform );
			rotationTransform[3] = m_InitialWheelPositionMS;
			
			m_TraceStart = m_SuspensionOffset.Multiply4(rotationTransform);
			m_SuspensionDistance = Math.Max(-m_Vehicle.GetModelVelocityAt(m_TraceStart)[1] / 40.0, 0) * 2.0; //! checking 2 frames ahead
			
			m_TraceUp = m_Axle.GetTravelMaxUp() + m_WheelItem.m_Radius;
			m_TraceDown = m_Axle.GetTravelMaxDown() + m_WheelItem.m_Radius + m_SuspensionDistance;
			
			m_ContactLength = m_TraceUp + m_TraceDown;

			PhxInteractionLayers collisionLayerMask = PhxInteractionLayers.BUILDING|PhxInteractionLayers.DOOR|PhxInteractionLayers.VEHICLE|PhxInteractionLayers.ROADWAY|PhxInteractionLayers.TERRAIN|PhxInteractionLayers.ITEM_SMALL|PhxInteractionLayers.ITEM_LARGE|PhxInteractionLayers.FENCE;
			
			m_RayStartMS = m_TraceStart - ( m_WheelDirectionMS * (m_TraceUp - m_Axle.GetWheelHubRadius()) );
			m_RayEndMS = m_TraceStart + ( m_WheelDirectionMS * (m_TraceDown - m_Axle.GetWheelHubRadius()) );

			m_RayStartWS = m_RayStartMS.Multiply4(m_Vehicle.m_Transform.data);
			m_RayEndWS = m_RayEndMS.Multiply4(m_Vehicle.m_Transform.data);
			
			m_Vehicle.DBGDrawLineMS( m_TraceStart - ( m_WheelDirectionMS * m_TraceUp ), m_TraceStart + ( m_WheelDirectionMS * m_TraceDown ), 0xFFFFFFFFF );
			
			#ifdef EXPANSION_DEBUG_UI_VEHICLE
			#endif

			m_HasContact = DayZPhysics.SphereCastBullet( m_RayStartWS, m_RayEndWS, m_Axle.GetWheelHubRadius(), collisionLayerMask, m_Vehicle, m_ContactObject, m_ContactPositionWS, m_ContactNormalWS, m_ContactFraction );
			if ( m_HasContact )
			{
				m_ContactNormal = m_ContactNormalWS.InvMultiply3( m_Vehicle.m_Transform.data );

				float wheelDiff = vector.Dot( m_ContactNormal, m_WheelDirectionMS );
				if ( wheelDiff >= -0.1 )
				{
					m_HasContact = false;
				}
			} else
			{
				m_ContactFraction = 1.0;
			}

			m_SuspensionLength = (1.0 - m_ContactFraction) * m_ContactLength;
			m_SuspensionLength = Math.Clamp(m_SuspensionLength, 0, m_Axle.GetTravelMax());
			
			m_SuspensionFraction = (m_SuspensionLength / m_Axle.GetTravelMax());

			m_SuspensionLengthPrevious = m_SuspensionLength;
		}
		
		if ( GetGame().IsServer() || pPhysics )
		{
			m_Vehicle.SetAnimationPhase( m_AnimDamper, m_SuspensionFraction );
			m_Vehicle.SetAnimationPhase( m_AnimTurn, m_Steering * Math.DEG2RAD );
			m_Vehicle.SetAnimationPhase( m_AnimRotation, m_AngularRotation );
		}
	}

	private void Suspension( float pDt )
	{
		vector rotationTransform[4];
		Math3D.YawPitchRollMatrix( Vector( -m_Steering, 0, 0 ), rotationTransform );
		rotationTransform[3] = m_InitialWheelPositionMS;
		
		m_TraceStart = m_SuspensionOffset.Multiply4(rotationTransform);
		m_SuspensionDistance = Math.Max(-m_Vehicle.GetModelVelocityAt(m_TraceStart)[1] / 40.0, 0) * 5.0; //! checking 2 frames ahead
		
		m_TraceUp = m_Axle.GetTravelMaxUp() + m_WheelItem.m_Radius + m_WheelItem.m_Radius;
		m_TraceDown = m_Axle.GetTravelMaxDown() + m_WheelItem.m_Radius + m_SuspensionDistance;
		
		m_ContactLength = m_TraceUp + m_TraceDown;

		PhxInteractionLayers collisionLayerMask = PhxInteractionLayers.BUILDING|PhxInteractionLayers.DOOR|PhxInteractionLayers.VEHICLE|PhxInteractionLayers.ROADWAY|PhxInteractionLayers.TERRAIN|PhxInteractionLayers.ITEM_SMALL|PhxInteractionLayers.ITEM_LARGE|PhxInteractionLayers.FENCE;
		
		m_RayStartMS = m_TraceStart - ( m_WheelDirectionMS * (m_TraceUp - m_Axle.GetWheelHubRadius()) );
		m_RayEndMS = m_TraceStart + ( m_WheelDirectionMS * (m_TraceDown - m_Axle.GetWheelHubRadius()) );

		m_RayStartWS = m_RayStartMS.Multiply4(m_Vehicle.m_Transform.data);
		m_RayEndWS = m_RayEndMS.Multiply4(m_Vehicle.m_Transform.data);
		
		m_Vehicle.DBGDrawLineMS( m_TraceStart - ( m_WheelDirectionMS * m_TraceUp ), m_TraceStart + ( m_WheelDirectionMS * m_TraceDown ), 0xFFFFFFFFF );

		m_HasContact = DayZPhysics.SphereCastBullet( m_RayStartWS, m_RayEndWS, m_Axle.GetWheelHubRadius(), collisionLayerMask, m_Vehicle, m_ContactObject, m_ContactPositionWS, m_ContactNormalWS, m_ContactFraction );
		if ( m_HasContact )
		{
			m_ContactPosition = m_ContactPositionWS.InvMultiply4(m_Vehicle.m_Transform.data);
			m_ContactNormal = m_ContactNormalWS.InvMultiply3( m_Vehicle.m_Transform.data );

			float wheelDiff = vector.Dot( m_ContactNormal, m_WheelDirectionMS );
			if ( wheelDiff >= -0.1 )
			{
				m_HasContact = false;
			} else
			{
				m_ContactVelocity = m_Vehicle.GetModelVelocityAt( m_ContactPosition );
				
				float invWheelDiff = -1.0 / wheelDiff;
				m_SuspensionRelativeVelocity = Math.Clamp(vector.Dot( m_ContactNormal, m_ContactVelocity ) * invWheelDiff, -1, 1);

				m_SuspensionInvContact = invWheelDiff;
				
				//m_ContactNormal = "0 1 0";
				//m_ContactNormalWS = m_ContactNormal.Multiply3( m_Vehicle.m_Transform.data );
			}
		}

		if ( !m_HasContact )
		{
			m_ContactFraction = 1.0;
			
			m_ContactVelocity = "0 0 0";

			m_SuspensionRelativeVelocity = 0.0;
			m_SuspensionInvContact = 10.0;

			m_ContactPosition = m_TraceStart + ( m_WheelDirectionMS * m_TraceDown );
			m_ContactNormal = -m_WheelDirectionMS;
			
			m_ContactPositionWS = m_ContactPosition.Multiply4(m_Vehicle.m_Transform.data);
			m_ContactNormalWS = m_TransformWS.data[1];
			
			m_Surface = "";
		} else
		{
			/*
			vector temp1;
			vector temp2;
			int temp3;
			set< Object > temp4 = new set< Object >();
			DayZPhysics.RaycastRV( m_RayStartWS, m_RayEndWS, temp1, temp2, temp3, temp4, m_Vehicle, m_Vehicle, false, false, ObjIntersectGeom, m_Axle.GetWheelHubRadius() );
			
			if ( temp4.Count() > 0 )
				m_ContactObject = temp4[0];
			else
				m_ContactObject = null;
			
			if ( !m_Surface || !DayZPhysics.GetHitSurface( m_ContactObject, m_RayStartWS, m_RayEndWS, m_Surface ) )
			{
				m_Surface = "";
			}
			*/
			
			m_Surface = "";
		}
		
		#ifndef EXPANSION_WHEEL_DEBUG_DISABLE
		m_Vehicle.DBGDrawImpulseMS( m_ContactPosition, m_ContactVelocity * m_Mass * pDt, 0x9900FF00 );
		
		if ( s_SUSP_DEBUG_LENGTH != 0.0 )
		{
			m_Vehicle.DBGDrawLine( m_ContactPositionWS, m_ContactPositionWS - (m_ContactNormal * s_SUSP_DEBUG_LENGTH), 0x9900FF00 );
			m_Vehicle.DBGDrawLine( m_ContactPosition.Multiply4(m_Vehicle.m_Transform.data), m_ContactPosition.Multiply4(m_Vehicle.m_Transform.data) + (m_ContactNormal * s_SUSP_DEBUG_LENGTH), 0x99FF0000 );
		}
		#endif
	}

	private void Suspension( float pDt, int pNumWheelsGrounded, out vector impulse, out vector impulseTorque )
	{
		m_SuspensionLength = m_ContactLength * (1.0 - m_ContactFraction);// * (1.5 - m_ContactFraction);
		//m_SuspensionLength = Math.Max(m_SuspensionLength, 0);
		m_WheelDepth = Math.Clamp(m_SuspensionLength - m_Axle.GetTravelMax(), 0, m_WheelItem.m_Radius);
		m_WheelDepth *= m_WheelDepth;
		m_SuspensionLength = Math.Clamp(m_SuspensionLength, 0, m_Axle.GetTravelMax());
		
		m_SuspensionFraction = (m_SuspensionLength / m_Axle.GetTravelMax());

		#ifdef EXPANSION_DEBUG_UI_VEHICLE
		#endif
		
		if ( m_HasContact )
		{
			Expansion_Assert_False( pNumWheelsGrounded != 0, "If m_HasContact is true, atleast 1 wheel must be grounded." );
						
			float ks = m_Axle.GetStiffness();
			float kc = m_Axle.GetCompression();
			float kd = m_Axle.GetDamping();

			float compressionDelta = kd * m_SuspensionRelativeVelocity;
			if (m_SuspensionRelativeVelocity < 0)
				compressionDelta = kc * m_SuspensionRelativeVelocity;
			float suspension = ks * m_SuspensionLength;
			
			m_SuspensionForce = suspension - compressionDelta;
		
			vector susp = m_ContactNormal * m_SuspensionForce * pDt;

			impulse += susp;
			impulseTorque += m_ContactPosition * susp;
			
			#ifndef EXPANSION_WHEEL_DEBUG_DISABLE
			m_Vehicle.DBGDrawImpulseMS( m_ContactPosition + Vector( 0, s_SUSP_DEBUG_LENGTH, 0 ), susp, 0xFFC0D000 );
			#endif
		}
		
			float surfaceY = GetGame().SurfaceY(m_TransformWS[3][0], m_TransformWS[3][2]);
			if (surfaceY > m_TransformWS[3][1])
			{
				vector vehicleTransform[4];
				m_Vehicle.GetTransform(vehicleTransform);
				
				vector transform[4];
				transform[0] = vehicleTransform[0];
				transform[1] = vehicleTransform[1];
				transform[2] = vehicleTransform[2];
				transform[3] = m_TransformWS[3];
				transform[3][1] = surfaceY + m_WheelItem.m_Radius;
				Math3D.MatrixInvMultiply4(vehicleTransform, transform, transform);
				transform[3][0] = 0;
				transform[3][2] = 0;
				Math3D.MatrixMultiply4(vehicleTransform, transform, transform);
			
				
				m_Vehicle.SetTransform(transform);
			}
		
		m_SuspensionLengthPrevious = m_SuspensionLength;
	}

	private void Friction( float pDt, int pNumWheelsGrounded, out vector impulse, out vector impulseTorque )
	{
		if ( !m_HasContact )
			return;
		
		m_SurfaceFriction = 0.95;
		
		if ( m_Surface != "" ) 
			m_SurfaceFriction = Surface.GetParamFloat( m_Surface, "friction" );
		
		m_WheelAcceleration = 0;
		
		m_WheelAcceleration += m_WheelItem.m_Radius * m_WheelTorque * m_Mass;
		
		m_WheelAcceleration -= Math.Sign(m_ContactVelocity[2]) * m_BrakeTorque * Math.Lerp(0.0, 1.0, Math.Clamp(Math.AbsFloat(m_ContactVelocity[2]), 0, 1));
		
		m_WheelAcceleration -= (1.0 - m_SurfaceFriction) * m_WheelItem.m_TyreRollResistance * m_ContactVelocity[2] * m_WheelItem.m_Radius * m_Mass;
		
		m_ForwardImpulse = m_WheelAcceleration;
		
		m_AngularVelocity = m_ContactVelocity[2] / m_WheelItem.m_Radius;
		
		m_AngularRotation += m_AngularVelocity * pDt;
		if ( m_AngularRotation > Math.PI2 )
			m_AngularRotation -= Math.PI2;
		else if ( m_AngularRotation < 0 )
			m_AngularRotation += Math.PI2;

		m_SideDot = vector.Dot( m_ContactVelocity.Normalized(), m_TransformMS[0] );
		float sideCoef = 10.0;
		
		vector axle = m_TransformWS[0];
		float proj = vector.Dot(axle, m_ContactNormalWS);
		axle -= m_ContactNormalWS * proj;
		axle.Normalize();
		
		//m_SideImpulse = sideCoef * m_Mass * -m_SideDot * m_ContactVelocity.Length() / pNumWheelsGrounded;
		m_SideImpulse = ExpansionPhysics.ResolveSingleBilateral(m_Vehicle, m_ContactPosition, m_ContactVelocity.Multiply3( m_Vehicle.m_Transform.data ), m_ContactObject, "0 0 0", axle);
		
		m_ForwardImp = m_TransformMS[2] * m_ForwardImpulse * pDt;
		m_SideImp = m_TransformMS[0] * m_SideImpulse * pDt;
	
		#ifndef EXPANSION_WHEEL_DEBUG_DISABLE
		m_Vehicle.DBGDrawImpulseMS( m_ContactPosition + "0 0.1 0", m_ForwardImp, 0xFF00FFFF );
		#endif
		
		impulse += m_ForwardImp;
		impulseTorque += m_ContactPosition * m_ForwardImp;

		#ifndef EXPANSION_WHEEL_DEBUG_DISABLE
		m_Vehicle.DBGDrawImpulseMS( m_ContactPosition + "0 0.1 0", m_SideImp, 0xFF4400AA );
		#endif
		
		impulse += m_SideImp;
		impulseTorque += m_ContactPosition * m_SideImp;
	}
};