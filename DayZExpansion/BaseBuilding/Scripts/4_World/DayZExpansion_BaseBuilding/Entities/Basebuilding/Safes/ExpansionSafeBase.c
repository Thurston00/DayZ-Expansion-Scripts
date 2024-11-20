/**
 * ExpansionSafeBase.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

class ExpansionSafeBase: Container_Base
{
	protected EffectSound m_Expansion_Sound;
	
	protected bool m_Expansion_WasSynced;
	protected bool m_Expansion_WasLocked;

	// ------------------------------------------------------------
	// Constructor
	// ------------------------------------------------------------
	void ExpansionSafeBase()
	{
		m_Expansion_KnownUIDs = new TStringArray;

		RegisterNetSyncVariableBool( "m_Expansion_Locked" );
		RegisterNetSyncVariableInt( "m_Expansion_CodeLength" );
	}

	// ------------------------------------------------------------
	// SetActions
	// ------------------------------------------------------------
	override void SetActions()
	{
		super.SetActions();

		AddAction( ActionTogglePlaceObject );
		AddAction( ActionPlaceObject );

		AddAction( ExpansionActionOpen );

		AddAction( ExpansionActionCloseAndLockSafe );
		AddAction( ExpansionActionCloseSafe );
		
		AddAction( ExpansionActionEnterSafeLock );
		AddAction( ExpansionActionChangeSafeLock );
	}

	// ------------------------------------------------------------
	// CanBeRepairedToPristine
	// ------------------------------------------------------------
	override bool CanBeRepairedToPristine()
	{
		return true;
	}

	override bool CanClose( string selection )
	{
		return super.CanClose(selection) && !IsRuined();
	}
	
	// ------------------------------------------------------------
	// Expansion ExpansionHasCodeLock
	// ------------------------------------------------------------
	override bool ExpansionHasCodeLock( string selection )
	{
		return (selection == "codelock");
	}

	// ------------------------------------------------------------
	// Expansion Open
	// ------------------------------------------------------------
	override void Expansion_Open( string selection ) 
	{
		//! Door open animation
		SetAnimationPhase( "safe_door", 1 );

		super.Expansion_Open( selection );
	}

	// ------------------------------------------------------------
	// Expansion Close
	// ------------------------------------------------------------
	override void Expansion_Close( string selection ) 
	{
		//! Door close animation
		SetAnimationPhase( "safe_door", 0 );
		
		super.Expansion_Close( selection );
	}
	
	protected void SoundCodeLockLocked()
	{
		if ( !GetGame().IsDedicatedServer() )
		{
			m_Expansion_Sound = SEffectManager.PlaySound("Expansion_CodeLock_Lock3_SoundSet", GetPosition());
			m_Expansion_Sound.SetSoundAutodestroy( true );
		}
	}
	
	protected void SoundCodeLockUnlocked()
	{
		if ( !GetGame().IsDedicatedServer() ) // client side
		{
			// Expansion_Succes_SoundSet
			m_Expansion_Sound = SEffectManager.PlaySound("Expansion_CodeLock_Unlock1_SoundSet", GetPosition());
			m_Expansion_Sound.SetSoundAutodestroy( true );
		}
	}

	override void OnVariablesSynchronized()
	{
		super.OnVariablesSynchronized();

		if ( m_Expansion_WasSynced && m_Expansion_WasLocked != m_Expansion_Locked )
		{
			if ( m_Expansion_Locked )
				SoundCodeLockLocked();
			else
				SoundCodeLockUnlocked();
		}

		m_Expansion_WasLocked = m_Expansion_Locked;
		m_Expansion_WasSynced = true;
	}

	override bool CanUseConstruction()
	{
		return false;
	}
	
	override bool IsDeployable()
	{
		return false;
	}

	#ifdef EXPANSION_MODSTORAGE
	override void CF_OnStoreSave(CF_ModStorageMap storage)
	{
		super.CF_OnStoreSave(storage);

		auto ctx = storage[DZ_Expansion_BaseBuilding];
		if (!ctx) return;

		ctx.Write( m_Expansion_IsOpened );
		ctx.Write( m_Expansion_Locked );
		ctx.Write( m_Expansion_Code );

		ctx.Write( m_Expansion_KnownUIDs.Count() );
		for ( int i = 0; i < m_Expansion_KnownUIDs.Count(); i++ )
		{
			ctx.Write( m_Expansion_KnownUIDs[i] );
		}
	}
	
	override bool CF_OnStoreLoad(CF_ModStorageMap storage)
	{
		if (!super.CF_OnStoreLoad(storage))
			return false;

		auto ctx = storage[DZ_Expansion_BaseBuilding];
		if (!ctx) return true;

		if (!ctx.Read(m_Expansion_IsOpened))
			return false;

		if (!ctx.Read(m_Expansion_Locked))
			return false;

		if (!ctx.Read(m_Expansion_Code))
			return false;

		m_Expansion_CodeLength = m_Expansion_Code.Length();

		if (ctx.GetVersion() < 38)
		{
			bool hasCode;
			if (!ctx.Read(hasCode))
				return false;
		}

		int count;
		if (!ctx.Read(count))
			return false;

		for (int i = 0; i < count; i++)
		{
			string knownUID;
			if (!ctx.Read(knownUID))
				return false;

			m_Expansion_KnownUIDs.Insert( knownUID );
		}

		return true;
	}
	#endif

	// ------------------------------------------------------------
	// Override AfterStoreLoad
	// ------------------------------------------------------------
	override void AfterStoreLoad()
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.CE, this, "AfterStoreLoad");
#endif

		super.AfterStoreLoad();

		if ( m_Expansion_IsOpened )
		{
			SetAnimationPhase( "safe_door", 1 );
		} else
		{
			SetAnimationPhase( "safe_door", 0 );
		}

		SetSynchDirty();
	}

	// ------------------------------------------------------------
	// IsHeavyBehaviour
	// ------------------------------------------------------------
	override bool IsHeavyBehaviour() 
	{
		return true;
	}

	// ------------------------------------------------------------
	// OnPlacementComplete
	// ------------------------------------------------------------
	override void OnPlacementComplete( Man player, vector position = "0 0 0", vector orientation = "0 0 0" )
	{
		if ( IsMissionHost() )
		{			
			SetPosition( position );
			SetOrientation( orientation );

			if( HasCode() && !ExpansionIsLocked() && !ExpansionIsOpened() )
				ExpansionLock();

			SetSynchDirty();
		}	
			
		SetIsPlaceSound( true );
	}

	// ------------------------------------------------------------
	// GetPlaceSoundset
	// ------------------------------------------------------------
	override string GetPlaceSoundset()
	{
		return "placeBarrel_SoundSet";
	}

	// ------------------------------------------------------------
	// CanPutInCargo
	// ------------------------------------------------------------
	override bool CanPutInCargo( EntityAI parent )
	{
		if ( !super.CanPutInCargo(parent) )
		{
			return false;
		}

		if ( GetNumberOfItems() == 0 && !ExpansionIsOpened() )
		{
			return true;
		}

		return false;
	}
	
	// ------------------------------------------------------------
	// CanPutIntoHands
	// ------------------------------------------------------------
	override bool CanPutIntoHands(EntityAI parent)
	{
		if ( !super.CanPutIntoHands( parent ) )
		{
			return false;
		}
		
		if ( GetNumberOfItems() == 0 && !ExpansionIsOpened() )
		{
			return true;
		}

		return false;
	}

	// ------------------------------------------------------------
	// DamageItemInCargo
	// ------------------------------------------------------------
	override bool DamageItemInCargo(float damage)
	{
		return false;
	}

	// ------------------------------------------------------------
	// EEKilled
	// ------------------------------------------------------------
	override void EEKilled( Object killer )
	{
		bool canRaidSafes = GetExpansionSettings().GetRaid().CanRaidSafes;

		if ( !canRaidSafes )
			return;

		super.EEKilled( killer );
	}

	override void ExpansionOnDestroyed( Object killer )
	{
		SetCode("");  //! Will unlock as well
	}
}