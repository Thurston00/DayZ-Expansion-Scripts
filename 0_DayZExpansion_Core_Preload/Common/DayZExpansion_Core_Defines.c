/**
 * DayZExpansion_Core_Defines.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

//! Versioning - LEGACY, consider adding defines for specific features instead when needed
//! GEQ = Greater than or equal
//! Do *not* remove old version defines!
#define EXPANSION_GEQ_1_7_0
#define EXPANSION_GEQ_1_7_17
#define EXPANSION_GEQ_1_7_24
#define EXPANSION_GEQ_1_7_31
#define EXPANSION_GEQ_1_8_0
#define EXPANSION_GEQ_1_8_11
#define EXPANSION_GEQ_1_8_12
#define EXPANSION_GEQ_1_8_18
#define EXPANSION_GEQ_1_8_19
#define EXPANSION_GEQ_1_8_20
#define EXPANSION_GEQ_1_8_21
#define EXPANSION_GEQ_1_8_25
#define EXPANSION_GEQ_1_8_28
#define EXPANSION_GEQ_1_8_33
#define EXPANSION_GEQ_1_8_46

//! Features
#define EXPANSION_GLOBALID_ITEMBASE_CORE

//! Enable it if you want to enable logging with EXPrint
//#define EXPANSIONEXPRINT
//#define EXPANSIONTRACE

// currently is only removed on internal builds and remains on diag/retail. Consider also removing on retail for performance gains
#ifndef DEVELOPER
#define EXTRACE
#endif

#ifdef DIAG
#define EXTRACE_DIAG
#endif

//! Enable it if you want to enable logging with EXLogPrint
//#define EXPANSIONEXLOGPRINT

//! Enable mod load order debug prints
//#define EXPANSION_LOADORDER_DEBUG

//! Enable storage/modstorage debug prints
//#define EXPANSION_STORAGE_DEBUG

//! Enable it if you want to enable item attaching logic
#define EXPANSION_ITEM_ATTACHING_DISABLE
//#define EXPANSION_ITEM_ATTACHING_LOGGING

//#define EXPANSION_PLAYER_ATTACHMENT_LOG

//#define EXPANSION_PLAYER_ATTACHMENT_CANATTACH_OVERRIDE

//! Enable it if you want to enable skinning system debug logs
//#define EXPANSION_SKIN_LOGGING

//! Used for third party mods, if they want to know if expansion is loaded
#define EXPANSIONMODCORE

#ifndef CF_DOUBLYLINKEDNODES
#define EXPANSION_DOUBLYLINKEDNODES
#ifndef JM_COT_DOUBLYLINKEDNODES
#define EXPANSION_DOUBLYLINKEDNODES_WEAKREF
#endif
#endif

#ifdef CF_DebugUI
#define EXPANSION_DEBUG_UI_AI
#define EXPANSION_DEBUG_UI_HOLOGRAM
#define EXPANSION_DEBUG_UI_FALLCOMMAND
#define EXPANSION_DEBUG_UI_VEHICLE
#define EXPANSION_DEBUG_UI_CAMERA
#endif

#ifdef SERVER //! Don't comment these, diag exe server crashes
#define EXPANSION_DEBUG_SHAPES_DISABLE
#endif

#ifdef DabsFramework
#define EXPANSIONUI
#endif

//! Versioning - NEW - do NOT add/change manually, will be added/changed during build with CI.bat!
#define EXPANSION_1_9_11
