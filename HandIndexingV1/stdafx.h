// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
#endif						

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#include <iostream>
#include <algorithm> //required for sort
#include <assert.h> //required for assert
#include "poker_defs.h" //required to use PokerEval tables and datatypes

//pop up a message box on error
#define REPORT(chars) do{ \
	std::cout << chars << std::endl; \
	_asm {int 3}; \
	exit(1); \
}while(0)
