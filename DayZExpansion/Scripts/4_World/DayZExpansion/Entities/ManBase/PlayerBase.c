/**
 * ExpansionPlayerBase.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2023 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

modded class PlayerBase
{
	Object m_PlayerHeadingDir;

	private bool m_HasCalledKillFeed;

	ExpansionKillFeedModule m_KillfeedModule;
	ItemBase m_Expansion_SuicideItem;

	float m_Expansion_GraveCross_Playtime;

	void PlayerBase()
	{
		m_HasCalledKillFeed = false;
	}

	void ~PlayerBase()
	{
		//! Making sure we remove tha call for CreateGraveCross when ever the player base entity gets destroyed
		if ( GetGame() && GetExpansionSettings().GetGeneral().EnableGravecross )
		{
		#ifdef ENFUSION_AI_PROJECT
			if (!IsAI())
		#endif
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(CreateGraveCross);
		}
	}

	override void Init()
	{
		super.Init();

		CF_Modules<ExpansionKillFeedModule>.Get(m_KillfeedModule);
	}

	override void SetSuicide(bool state)
	{
		super.SetSuicide(state);

		if (state)
			m_Expansion_SuicideItem = GetItemInHands();
	}

	override void EEKilled( Object killer )
	{
		if ( GetExpansionSettings().GetGeneral().EnableGravecross )
		{
			#ifdef ENFUSION_AI_PROJECT
			if (!IsAI())
			{
			#endif

				float playtime = StatGet(AnalyticsManagerServer.STAT_PLAYTIME);
				if (playtime > 0)
					Expansion_SetPlaytimeForGraveCross(playtime);

				EntityAI handEntity = GetHumanInventory().GetEntityInHands();
				GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(CreateGraveCross, GetExpansionSettings().GetGeneral().GravecrossSpawnTimeDelay * 1000, false, handEntity);

			#ifdef ENFUSION_AI_PROJECT
			}
			#endif
		}

		super.EEKilled(killer);

		if (GetExpansionSettings().GetNotification().EnableKillFeed && GetIdentity())
		{
			if ( m_KillfeedModule )
			{
				m_KillfeedModule.OnPlayerKilled( this, killer );
			}
		}
	}

	override void EEHitBy(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		if ( GetExpansionSettings().GetNotification().EnableKillFeed && GetIdentity() )
		{
			if ( m_KillfeedModule && !IPADACK() )
			{
				UpdateIPADACK( !IsAlive() );
				m_KillfeedModule.OnPlayerHitBy( damageType, this, source, ammo );
			}
		}

		super.EEHitBy(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);
	}

	bool IPADACK()
	{
		return m_HasCalledKillFeed;
	}

	void UpdateIPADACK(bool state = true)
	{
		m_HasCalledKillFeed = state;
	}

	void Expansion_SetPlaytimeForGraveCross(float playtime)
	{
		m_Expansion_GraveCross_Playtime = playtime;
	}

	void CreateGraveCross(EntityAI handEntity)
	{
		float lifetimeThreshhold = GetExpansionSettings().GetGeneral().GravecrossTimeThreshold;
		bool deleteBody = GetExpansionSettings().GetGeneral().GravecrossDeleteBody;

		string graveobject = "Expansion_Gravecross";

		//! Offset of 0.6 is to account for cross anchor point not being at the bottom of the cross,
		//! if we change cross object and not using ECE_TRACE this needs to be adjusted!
		float offsetY = 0.6;

	#ifdef DIAG
		EXPrint(ToString() + "::CreateGraveCross playtime " + m_Expansion_GraveCross_Playtime + " threshold " + lifetimeThreshhold);
	#endif

		if (m_Expansion_GraveCross_Playtime <= lifetimeThreshhold)
		{
			graveobject = "Expansion_Gravecross_LowLifetime";

			//! Offset of 1.035 is to account for cross anchor point not being at the bottom of the cross,
			//! if we change cross object and not using ECE_TRACE this needs to be adjusted!
			offsetY = 1.035;
		}

		Expansion_GraveBase grave;

		vector pos = GetPosition();
		vector ground = Vector(pos[0], GetGame().SurfaceY(pos[0], pos[2]), pos[2]);

		//! The idea here is that the gravecross should spawn on top of the thing the player died on if it's a building or large item,
		//! and not below sea level if over water

		PhxInteractionLayers layerMask;

		layerMask |= PhxInteractionLayers.BUILDING;
		layerMask |= PhxInteractionLayers.ROADWAY;
		layerMask |= PhxInteractionLayers.TERRAIN;
		layerMask |= PhxInteractionLayers.WATERLAYER;  //! Doesn't seem to work?
		layerMask |= PhxInteractionLayers.ITEM_LARGE;

		Object hitObject;
		vector hitPosition;
		vector hitNormal;
		float hitFraction;

		if ( DayZPhysics.RayCastBullet( pos, ground, layerMask, this, hitObject, hitPosition, hitNormal, hitFraction ) )
			ground[1] = hitPosition[1];

		float water_depth = GetGame().GetWaterDepth(ground);

		if ( water_depth > 0 )
		{
			//! Add water depth so cross sits above water level (with bottom bit submerged slightly)
			ground[1] = ground[1] + water_depth - 0.5;
		}

		ground[1] = ground[1] + offsetY;

		grave = Expansion_GraveBase.Cast(GetGame().CreateObjectEx(graveobject, ground, ECE_CREATEPHYSICS|ECE_UPDATEPATHGRAPH));
		grave.SetPosition(ground);

		if ( handEntity && handEntity.GetHierarchyRootPlayer() )
			handEntity = NULL;

		grave.MoveAttachmentsFromEntity(this, handEntity, ground, GetOrientation());
		grave.SetOrientation(GetOrientation());

		if (deleteBody)
		{
            SetPosition("0 0 0");
        	GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(Delete, 5000, false);
		}
	}

	void Debug_PlayerForward()
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.PLAYER, this, "Debug_PlayerForward");
#endif

		float speed;
		vector direction;
		GetInputController().GetMovement( speed, direction );

		Transform trans = Transform.GetObject( this );
		direction = direction.Multiply3( trans.GetBasis().data );

		vector pos = GetPosition() + direction;

		m_PlayerHeadingDir.SetPosition( pos );
		m_PlayerHeadingDir.SetDirection( direction );
	}
};
