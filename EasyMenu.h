#ifndef _INCLUDE_METAMOD_SOURCE_STUB_PLUGIN_H_
#define _INCLUDE_METAMOD_SOURCE_STUB_PLUGIN_H_


#include <ISmmPlugin.h>
#include <sh_vector.h>
#include <stdio.h>
#include <fstream>
#include <iserver.h>
#include "igameevents.h"
#include "vector.h"
#include <deque>
#include <random>
#include <functional>
#include <utlstring.h>
#include <keyvalues.h>
#include "CCSPlayerController.h"
#include <ctime>
#include "time.h"
#include <complex>
#include <iomanip>
#include "metamod_oslink.h"
#include "include/menus.h"
#include "schemasystem/schemasystem.h"
#include "algorithm"


using namespace std;

struct MenuItem {
    string text;
    string nextMenu;
    bool isDisabled;
};

struct MenuData {
    string name;
    string title;
    string backMenu;
    vector<MenuItem> items;
};


class EasyMenu final : public ISmmPlugin, public IMetamodListener {
public:
    bool Load(PluginId id, ISmmAPI* ismm, char* error, size_t maxlen, bool late);
    bool Unload(char* error, size_t maxlen);
    void AllPluginsLoaded();
private:
    const char* GetAuthor();
    const char* GetName();
    const char* GetDescription();
    const char* GetURL();
    const char* GetLicense();
    const char* GetVersion();
    const char* GetDate();
    const char* GetLogTag();
};

#endif //_INCLUDE_METAMOD_SOURCE_STUB_PLUGIN_H_