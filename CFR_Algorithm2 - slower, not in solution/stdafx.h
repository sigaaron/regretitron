// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
#endif						

#include <iostream>
#include <stdio.h>
#include <tchar.h>

#include <assert.h>

void createkuhntree();
double preprob(int oppcard, int playerignore, int beti);
double avgutility(int card0, int card1, int beti);

// TODO: reference additional headers your program requires here
