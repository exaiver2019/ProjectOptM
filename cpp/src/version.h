// Project OptM version - bump for every release (also used by resource.rc).
#pragma once
#define OPTM_VERSION       "2.0.0"
#define OPTM_VERSION_RC    2, 0, 0, 0

// GitHub "user/repo" the releases live in; empty turns update checks off
#ifndef OPTM_UPDATE_REPO
#define OPTM_UPDATE_REPO   "exaiver2019/ProjectOptM"
#endif
#ifndef OPTM_UPDATE_API    // overridable for testing against a local server
#define OPTM_UPDATE_API    "https://api.github.com/repos/" OPTM_UPDATE_REPO "/releases/latest"
#endif
