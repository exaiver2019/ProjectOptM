// The optimization engine: detects games, applies their profile, puts everything back.
// Ported from Project OptM 1.1 (Enable-Profile / Invoke-Tick / Restore-All).
#pragma once
#include <cstdint>
#include <functional>
#include <future>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>
#include "data.h"
#include "processes.h"
#include "system_info.h"

class Optimizer {
public:
    using LogFn = std::function<void(const std::string&)>;

    void Init(const SystemInfo* sys, AppData* data, LogFn log);

    // Call every poll interval with a fresh process list.
    void Tick(const ProcessList& procs);

    void RestoreAll();                        // undo everything we changed (session keeps running)
    void EndSession(bool gameClosed = false); // restore + stop tracking the active game
    void Panic();                             // restore now and pause auto-optimize
    void SetAuto(bool on);
    void RecoverLastRun();                    // power plan / services left changed by a crash
    void SyncLaunchPriority();                // IFEO entries match the profiles
    void ClearStandby();                      // runs in the background; result is logged on a later tick

    const GameProfile* Active() const { return active_ ? &*active_ : nullptr; }
    double SessionMinutes() const;
    int SessionCleanups() const { return cleanups_; }
    bool Running(const GameProfile& p, const ProcessList& procs) const;

    // Descriptions for the UI
    std::string CoreMode(const GameProfile& p) const;           // Best / Other / All
    std::string Summary(const GameProfile& p) const;
    std::vector<std::string> Plan(const GameProfile& p) const;  // what this profile would do here
    std::vector<std::string> BackgroundList() const;
    uint64_t BackgroundMask() const;
    int PurgeMinutes(const GameProfile& p) const;
    std::vector<std::string> VendorApps(const std::string& vendor) const;

    // Tweaks page: which tweaks run (fixed for a session - changes apply to the next game)
    bool On(const std::string& id) const { return on_.count(id) > 0; }
    void RefreshTweaks();                     // re-read the active preset (ignored mid-session)
    void SyncPerGameSettings();       // undo per-game FSO / GPU changes if those tweaks were turned off
    void RevertOnExit();                      // launch priority, FSO and GPU preference back to yours
    void ReapplyAtStart();                    // ...and set again when the app starts

    // Hooks for the app (FPS capture, history)
    std::function<void(const GameProfile&)> onSessionStart;
    std::function<void(const GameProfile&, double minutes, bool gameClosed)> onSessionEnd;

private:
    void Enable(const GameProfile& p, const ProcessList& procs);
    void Upkeep(const ProcessList& procs);
    void TuneGame(DWORD pid, const std::string& name, const GameProfile& p);
    void SetPriority(const std::vector<std::string>& names, DWORD cls, const ProcessList& procs);
    int  MoveBackground(const std::vector<std::string>& names, const ProcessList& procs);
    void CloseApps(const std::vector<std::string>& names, const std::vector<std::string>& keep, const ProcessList& procs);
    void ReopenClosedApps();
    std::vector<std::string> PauseList() const;
    void PauseServices();
    void ResumeServices();
    void SetGamingPowerPlan();
    void RestorePowerPlan();
    std::vector<std::string> Expand(const std::vector<std::string>& names) const;
    void CheckPurge();
    bool SoftPin(const GameProfile& p) const;
    int  EcoBackground(const std::vector<std::string>& names, const ProcessList& procs);
    void ApplySessionTweaks(const GameProfile& p, const ProcessList& procs);
    void RevertSessionTweaks();
    void Backup(const std::string& b);
    bool AnyGameUses(const std::string& tweakId) const;
    bool SystemWide() const;       // false in a test copy: launch priority / GPU / FSO are left alone
    void RestoreGpuPreferences();
    bool ForcePriority(const std::string& name, DWORD cls, const ProcessList& procs);   // bypasses the protected list

    const SystemInfo* sys_ = nullptr;
    AppData* data_ = nullptr;
    LogFn log_;

    std::optional<GameProfile> active_;
    uint64_t sessionStart_ = 0, lastPurge_ = 0;   // GetTickCount64
    int cleanups_ = 0;
    std::map<ProcKey, DWORD> savedPriority_;
    std::map<ProcKey, uint64_t> savedAffinity_;
    std::set<ProcKey> tuned_;
    std::vector<DWORD> ecoPids_;
    std::vector<std::wstring> reopenPaths_;
    std::future<uint32_t> purge_;
    mutable std::set<std::string> on_;   // Plan/Summary swap in a game's own tweaks for a moment
    std::set<ProcKey> ecoBg_;      // background apps we put in efficiency mode
    std::set<ProcKey> cpuSetKeys_; // game processes that may have CPU sets from us
    bool timerOn_ = false;
    uint64_t trimAt_ = 0;          // working-set trim, once the game has loaded
};
