#pragma once

// The 2B Mod Launcher's version, set in one place: the window title (2BModLauncher.c) and the
// VERSIONINFO resource (Resource.rc) are built from these numbers. Release tags carry
// Major.Minor, as the title does: v1.5.
//
// Only the numbers are set by hand. Each string is one stringized run of tokens, so it is still a
// single string literal for rc.

#define LAUNCHER_VERSION_MAJOR			1
#define LAUNCHER_VERSION_MINOR			5
#define LAUNCHER_VERSION_PATCH			0
#define LAUNCHER_VERSION_HOTFIX			0

#define LAUNCHER_VERSION_STRINGIZE_(x)	#x
#define LAUNCHER_VERSION_STRINGIZE(x)	LAUNCHER_VERSION_STRINGIZE_(x)

// Major.Minor, as in the title and the tag: "1.5"
#define LAUNCHER_VERSION_STRING			LAUNCHER_VERSION_STRINGIZE(LAUNCHER_VERSION_MAJOR.LAUNCHER_VERSION_MINOR)

// All four parts, for VERSIONINFO: "1.5.0.0"
#define LAUNCHER_VERSION_FILE_STRING	LAUNCHER_VERSION_STRINGIZE(LAUNCHER_VERSION_MAJOR.LAUNCHER_VERSION_MINOR.LAUNCHER_VERSION_PATCH.LAUNCHER_VERSION_HOTFIX)
