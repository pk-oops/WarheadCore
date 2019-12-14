/*
 * Copyright (C) 2016+     AzerothCore <www.azerothcore.org>
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <list>
#include <vector>
#include <ace/Configuration_Import_Export.h>
#include <ace/Thread_Mutex.h>
#include <AutoPtr.h>

typedef acore::AutoPtr<ACE_Configuration_Heap, ACE_Null_Mutex> Config;

class ConfigMgr
{
    friend class ConfigLoader;

    ConfigMgr() { }
    ~ConfigMgr() { }

public:

    static ConfigMgr* instance();
    
    /// Method used only for loading main configuration files (authserver.conf and worldserver.conf)
    bool LoadInitial(char const* file);

    /**
     * This method loads additional configuration files
     * It is recommended to use this method in WorldScript::OnConfigLoad hooks
     *
     * @return true if loading was successful
     */
    bool LoadMore(char const* file);

    bool Reload();

    std::string GetStringDefault(std::string const& name, const std::string& def, bool logUnused = true);
    bool GetBoolDefault(std::string const& name, bool def, bool logUnused = true);
    int GetIntDefault(std::string const& name, int def, bool logUnused = true);
    float GetFloatDefault(std::string const& name, float def, bool logUnused = true);

    std::list<std::string> GetKeysByString(std::string const& name);

    bool isDryRun() { return this->dryRun; }
    void setDryRun(bool mode) { this->dryRun = mode; }

private:
    bool dryRun = false;

    bool GetValueHelper(const char* name, ACE_TString &result);
    bool LoadData(char const* file);

    typedef ACE_Thread_Mutex LockType;
    typedef ACE_Guard<LockType> GuardType;

    std::vector<std::string> _confFiles;
    Config _config;
    LockType _configLock;

    ConfigMgr(ConfigMgr const&);
    ConfigMgr& operator=(ConfigMgr const&);
};

#define sConfigMgr ConfigMgr::instance()

#endif
