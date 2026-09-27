// Game profile share codes: one line of text a friend can paste into their Project OptM.
// "OPTM-GAME1:" + base64 of a small JSON with the game's settings. Launcher paths aren't included
// (they're personal), and a code can't carry anything but these settings.
#pragma once
#include <string>
#include "data.h"

namespace share {

std::string Encode(const GameProfile& p);
bool Decode(const std::string& code, GameProfile& out, std::string& error);

}  // namespace share
