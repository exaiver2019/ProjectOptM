// Project OptM version - bump for every release (also used by resource.rc).
#pragma once
#define OPTM_VERSION       "2.1.1"
#define OPTM_VERSION_RC    2, 1, 1, 0

// Release channel - which build this is, and who it should go to. Change this one line to switch;
// nothing else in the app needs editing (the badge, its color and its tooltip follow automatically).
//
//   ""              STABLE        Published on GitHub (Publish-Release.bat). Give this to anyone,
//                                  including friends who just want it to work.
//   "experimental"  EXPERIMENTAL  A local build ahead of the next release. Its features have each
//                                 been tried end to end (see the README's per-feature test notes) -
//                                 not everything is proven on real hardware yet. For you, or anyone
//                                 who wants new things early and won't mind an occasional rough edge.
//   "unstable"      UNSTABLE      A local build still being changed with Claude. Something in it may
//                                 be untested, broken, or mid-edit. Never shared - not even with a
//                                 friend who "doesn't mind bugs" - and not for a real game session
//                                 you care about.
#define OPTM_CHANNEL       "experimental"
#define OPTM_VERSION_LABEL OPTM_VERSION " " OPTM_CHANNEL   // "2.1.1 experimental" (just the number when stable)

// GitHub "user/repo" the releases live in; empty turns update checks off
#ifndef OPTM_UPDATE_REPO
#define OPTM_UPDATE_REPO   "exaiver2019/ProjectOptM"
#endif
#ifndef OPTM_UPDATE_API    // overridable for testing against a local server
#define OPTM_UPDATE_API    "https://api.github.com/repos/" OPTM_UPDATE_REPO "/releases/latest"
#endif
