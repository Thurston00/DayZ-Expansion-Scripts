//#include "JM/CF/Defines/CFDefines.c"

#define EXPANSIONMODAI
#define ENFUSION_AI_PROJECT

//#define EAI_TRACE

#ifdef DIAG
	#define EAI_DEBUG_TRANSITION
#else
	#ifdef BUILD_EXPERIMENTAL
		#define EAI_DEBUG_TRANSITION
	#else
		//! Disable AI attachment if stable DayZ until server CTD when attaching is fixed
		#define EXPANSION_DISABLE_AI_ATTACHMENT
	#endif
#endif

#define EAI_DEBUG_PATH