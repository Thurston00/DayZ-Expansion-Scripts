/**
 * ExpansionConstructionKitBase.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

/**@class		ExpansionConstructionKitBase
 * @brief
 **/

class ExpansionConstructionKitBase: ItemBase
{
	const float MAX_PLACEMENT_HEIGHT_DIFF = 1.5;
	ref protected EffectSound m_Expansion_DeployLoopSound;
	protected bool m_Expansion_DeployedRegularly;
	
	void ExpansionConstructionKitBase()
	{
		RegisterNetSyncVariableBool("m_IsSoundSynchRemote");
		RegisterNetSyncVariableBool("m_IsDeploySound");
	}
	
	void ~ExpansionConstructionKitBase()
	{
		SEffectManager.DestroyEffect(m_Expansion_DeployLoopSound);
	}
	
	override void EEInit()
	{
		super.EEInit();

		UpdateVisuals();
	}

	void UpdateVisuals()
	{
		SetAnimationPhase( "Inventory", 0 );
		SetAnimationPhase( "Placing", 1 );
	}
	
	override bool DisassembleOnLastDetach()
	{
		return true;
	}

	override bool IsIgnoredByConstruction()
	{
		return false;
	}
	
	override bool IsBasebuildingKit()
	{
		return true;
	}
	
	override bool CanBeRepairedByCrafting()
	{
		return false;
	}
	
	override bool CanPutIntoHands(EntityAI parent)
	{
		return true;
	}

	override bool CanPutInCargo(EntityAI parent)
	{
		return true;
	}

	override bool CanBeRepairedToPristine()
	{
		return true;
	}
	
	override void OnEndPlacement()
	{
		m_Expansion_DeployedRegularly = true;
		SoundSynchRemote();
	}
	
	override void OnPlacementCancelled(Man player)
	{
		super.OnPlacementCancelled(player);
		m_Expansion_DeployedRegularly = false;
		
		if (GetGame().IsServer())
		{
			//! When placing gets cancled show it again
			ShowAllSelections();
		}
	}
	
	override int GetMeleeTargetType()
	{
		return EMeleeTargetType.NONALIGNABLE;
	}
	
	override void OnVariablesSynchronized()
	{
		super.OnVariablesSynchronized();

		if (IsDeploySound())
		{
			PlayDeploySound();
		}

		if (CanPlayDeployLoopSound())
		{
			PlayDeployLoopSound();
		}

		if (m_Expansion_DeployLoopSound && !CanPlayDeployLoopSound())
		{
			StopDeployLoopSound();
		}
	}
	
	void PlayDeployLoopSound()
	{
		if (!GetGame().IsDedicatedServer())
		{
			if (!m_Expansion_DeployLoopSound || !m_Expansion_DeployLoopSound.IsSoundPlaying())
			{
				m_Expansion_DeployLoopSound = SEffectManager.PlaySound(GetLoopDeploySoundset(), GetPosition());
			}
		}
	}

	void StopDeployLoopSound()
	{
		if ( !GetGame().IsDedicatedServer() )
		{
			m_Expansion_DeployLoopSound.SetSoundFadeOut(0.5);
			m_Expansion_DeployLoopSound.SoundStop();
		}
	}

	override void SetActions()
	{
		super.SetActions();

		AddAction(ActionTogglePlaceObject);
		AddAction(ActionDeployObject);
	}

	override int GetDamageSystemVersionChange()
	{
		return 110;
	}
	
	override void OnItemLocationChanged(EntityAI old_owner, EntityAI new_owner)
	{		
		super.OnItemLocationChanged(old_owner, new_owner);
	}

	override bool CanBePlaced(Man player, vector position)
	{
		vector playerpos = player.GetPosition();
		float delta1 = playerpos[1] - position[1];

		if (delta1 > MAX_PLACEMENT_HEIGHT_DIFF || delta1 < -MAX_PLACEMENT_HEIGHT_DIFF)
			return false;
		
		return true;
	}

	override bool IsDeployable()
	{
		return true;
	}

	override string GetDeploySoundset()
	{
		return "placeCarTent_SoundSet";
	}

	override string GetLoopDeploySoundset()
	{
		return "cartent_deploy_SoundSet";
	}
	
	override bool IsHeavyBehaviour()
	{
		return true;
	}
};

class ExpansionConstructionKitLarge: ExpansionConstructionKitBase {};
class ExpansionConstructionKitSmall: ExpansionConstructionKitBase {};

class ExpansionDeployableConstruction: ItemBase
{
	float m_Expansion_ConstructionKitHealth;
	
	ItemBase CreateConstructionKit()
	{
		ItemBase construction_kit = ItemBase.Cast(GetGame().CreateObjectEx(GetConstructionKitType(), GetPosition(), ECE_PLACE_ON_SURFACE ));
		if (m_Expansion_ConstructionKitHealth > 0)
		{
			construction_kit.SetHealth(m_Expansion_ConstructionKitHealth);
		}
		
		return construction_kit;
	}
	
	void CreateConstructionKitInHands(notnull PlayerBase player)
	{
		ItemBase construction_kit = ItemBase.Cast(player.GetHumanInventory().CreateInHands(GetConstructionKitType()));
		if (m_Expansion_ConstructionKitHealth > 0)
		{
			construction_kit.SetHealth(m_Expansion_ConstructionKitHealth);
		}
	}
	
	override void SetActions()
	{
		super.SetActions();

		AddAction( ActionFoldBaseBuildingObject );
		AddAction( ExpansionActionDamageBaseBuilding );
	}
	
	override void EEHealthLevelChanged(int oldLevel, int newLevel, string zone)
	{
		super.EEHealthLevelChanged(oldLevel,newLevel,zone);
		
		if (m_FixDamageSystemInit)
			return;
		
		if (zone == "" && newLevel == GameConstants.STATE_RUINED && GetGame().IsServer())
			MiscGameplayFunctions.DropAllItemsInInventoryInBounds(this, Vector(0.8, 0.15, 1.3));
	}
	
	string GetConstructionKitType();
	
	void DestroyConstruction()
	{
		GetGame().ObjectDelete(this);
	}	
	
	#ifdef EXPANSION_MODSTORAGE
	override void CF_OnStoreSave(CF_ModStorageMap storage)
	{
		super.CF_OnStoreSave(storage);

		auto ctx = storage[DZ_Expansion_BaseBuilding];
		if (!ctx) return;

		ctx.Write(m_Expansion_ConstructionKitHealth);
	}

	override bool CF_OnStoreLoad(CF_ModStorageMap storage)
	{
		if (!super.CF_OnStoreLoad(storage))
			return false;

		auto ctx = storage[DZ_Expansion_BaseBuilding];
		if (!ctx) return true;

		if (ctx.GetVersion() < 46)
			return true;
		
		if (!ctx.Read(m_Expansion_ConstructionKitHealth))
			return false;

		return true;
	}
	#endif
}