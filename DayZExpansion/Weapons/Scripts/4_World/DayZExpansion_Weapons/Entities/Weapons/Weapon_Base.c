/**
 * Weapon_Base.c
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
	static int s_Expansion_ConfirmWeaponFire_RPCID;

	private int m_ExShouldFire;
	private autoptr array< int > m_ExMuzzleIndices;
	
	void Weapon_Base()
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.WEAPONS, this, "Weapon_Base");
#endif
	
		m_ExMuzzleIndices = new array< int >;
	}

	void Expansion_FoldOpticsDown()
	{
		TStringArray selectionNames = new TStringArray;
		ConfigGetTextArray( "simpleHiddenSelections", selectionNames );

		SetSimpleHiddenSelectionState( selectionNames.Find( "folding_raised" ), false );
		SetSimpleHiddenSelectionState( selectionNames.Find( "folding_lowered" ), true );
	}

	void Expansion_FoldOpticsUp()
	{
		TStringArray selectionNames = new TStringArray;
		ConfigGetTextArray( "simpleHiddenSelections", selectionNames );

		SetSimpleHiddenSelectionState( selectionNames.Find( "folding_raised" ), true );
		SetSimpleHiddenSelectionState( selectionNames.Find( "folding_lowered" ), false );
	}
	
	void ExpansionHideWeaponPart(string WeaponSelectionS, bool state)
	{
		TStringArray selectionNames = new TStringArray;
		ConfigGetTextArray( "simpleHiddenSelections", selectionNames );
		SetSimpleHiddenSelectionState( selectionNames.Find( WeaponSelectionS ), !state );
	}

	override void OnInventoryExit(Man player)
	{
		super.OnInventoryExit( player );

		UpdateLaser();
	}

	override void UpdateLaser()
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.WEAPONS, this, "UpdateLaser");
#endif
		
		if ( GetGame().IsServer() && GetGame().IsMultiplayer() )
			return;

		if ( !GetInventory() )
			return;

		ItemBase laser = ItemBase.Cast( FindAttachmentBySlotName( "weaponFlashlight" ) );

		if ( laser )
		{
			laser.UpdateLaser();
		}
	}

	float CalculateBarrelLength()
	{
		vector usti_hlavne_position = GetSelectionPositionLS( "usti hlavne" );
		vector konec_hlavne_position = GetSelectionPositionLS( "konec hlavne" );
		return vector.Distance( usti_hlavne_position, konec_hlavne_position );
	}
	
	void ExpansionSetNextFire( int muzzleIndex )
	{
		m_ExShouldFire++;
		m_ExMuzzleIndices.Insert( muzzleIndex );
	}

	vector GetFirePosition( DayZPlayerImplement player )
	{
		if ( !player.IsAlive() )
			return "0 0 0";
		return player.GetBonePositionWS( player.GetBoneIndexByName( "Weapon_Bone_06" ) );
	}

	void ExpansionFire( PlayerBase player, vector direction )
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.WEAPONS, this, "ExpansionFire");
#endif

		if ( m_ExShouldFire <= 0 )
			return;
		
		if ( player )
		{
			int muzzleIndex = m_ExMuzzleIndices[0];

			vector position = GetFirePosition( player ) + ( direction * CalculateBarrelLength() );

			ExpansionWeaponFireBase fireBase = ExpansionWeaponFireBase.Cast( GetExpansionFireType().Spawn() );

			if ( fireBase )
			{
				fireBase.FireServer( this, muzzleIndex, player, position, direction );
			}
		}

		m_ExShouldFire--;
		m_ExMuzzleIndices.Remove( 0 );
	}

	typename GetExpansionFireType()
	{
		return ExpansionWeaponFireBase;
	}
	
	override void SetActions()
	{
		super.SetActions();
		
		AddAction(ExpansionActionCycleOpticsMode);		
	}

	void RPC_Expansion_ConfirmWeaponFire(PlayerIdentity sender, ParamsReadContext ctx)
	{
		auto trace = EXTrace.Start(ExpansionTracing.WEAPONS, this);

		vector direction;
		if (!ctx.Read(direction))
			return;

		PlayerBase player;

		if (GetGame().IsMultiplayer())
		{
			player = PlayerBase.Cast(sender.GetPlayer());
		} 
		else
		{
			player = PlayerBase.Cast(GetGame().GetPlayer());
		}

		if (!player || player != GetHierarchyRootPlayer())
			return;

		ExpansionFire(player, direction);
	}
};
