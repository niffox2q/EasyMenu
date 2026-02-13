#include "EasyMenu.h"


EasyMenu EasyMenu;

IVEngineServer2* engine = nullptr;
CGlobalVars* gpGlobals = nullptr;
CGameEntitySystem* g_pGameEntitySystem = nullptr;
CEntitySystem* g_pEntitySystem = nullptr;

IUtilsApi* utils;
IMenusApi* menus_api;
IPlayersApi* players_api;


PLUGIN_EXPOSE(EasyMenu,EasyMenu);

static bool pluginLoaded = false;
vector<MenuData> allMenus;
map<string, string> phrases;

string startMenu;

// TODO
// Плагин позволяет обычным юзерам создавать меню через конфиги, без полноценного воссоздания плагина
// Плагин должен считывать каждый файл который находится в addons/configs/EasyMenu/menus, там должны находится меню в виде JSON
// В конфиге должна находится настройка (открытия меню при входе игрока), туда можно будет записать только одно
// Поддерка VIP[CORE] и Admin System чтобы иметь возможность открывать меню только для определнных лиц
// Должен находится основной конфиг по пути addons/configs/EasyMenu
// Структура MenuData должна хранить в себе - название string name, string title, vector<MenuItem> items
// Структура MenuItem должна хранить в себе - string text, string szBack(callback), bool isDisabled?
// "Menu"
// {
//     "OpenCommand"	"!examplemenu1"
//     "AdminAccess"	"@admin/root"
//     "VipAccess"		"owner"
//
//     "Title"		"Пример заголовка меню"
//
//     "Item"
//     {
//         "Text"	"Кнопка №1"
//         "Command"	"say Привет!"
//         "disabled"	"false"
//     }
// }
//
//
// "Config"
// {
//     "JoinMenu" "examplemenu1"
// }

CGameEntitySystem* GameEntitySystem()
{
    return utils->GetCGameEntitySystem();
}

// void LoadTranslations() {
//     phrases.clear();
//     KeyValues* g_kvPhrases = new KeyValues("Phrases");
//     const char *pszPath = "addons/translations/lr_joinAnnounce.phrases.txt";
//
//     if (!g_kvPhrases->LoadFromFile(g_pFullFileSystem, pszPath))
//     {
//         utils->ErrorLog("%s Failed to load %s", g_PLAPI->GetLogTag(), pszPath);
//         return;
//     }
//
//     const char* language = utils->GetLanguage();
//
//     for (KeyValues *pKey = g_kvPhrases->GetFirstTrueSubKey(); pKey; pKey = pKey->GetNextTrueSubKey()) {
//         phrases[string(pKey->GetName())] = string(pKey->GetString(language));
//     }
//     delete g_kvPhrases;
//
// }
//
// const char* GetTranslation(const char* key) {
//     return phrases[string(key)].c_str();
// }

void OpenMenu(int iSlot,string menu_name) {
    Menu hMenu;
    for (auto& m : allMenus) {
        if (m.name == menu_name) {
            menus_api->SetTitleMenu(hMenu, m.title.c_str());
            for (auto& i : m.items) {
                menus_api->AddItemMenu(hMenu,i.nextMenu.c_str(),i.text.c_str(),i.isDisabled ? ITEM_DISABLED : ITEM_DEFAULT);
            }
            menus_api->SetExitMenu(hMenu,true);
            if (m.name != startMenu) menus_api->SetBackMenu(hMenu,true);
            menus_api->SetCallback(hMenu,[m](const char* szBack, const char* szFront, int iItem, int iSlot) {
                CCSPlayerController* player = CCSPlayerController::FromSlot(iSlot);
                if (!player || !player->IsConnected()) return;
                if (!szBack || strcmp(szBack, "") == 0) return;
                if (strcmp(szBack,"back") == 0) {
                    OpenMenu(iSlot, m.backMenu.empty() ? startMenu : m.backMenu);
                }
                OpenMenu(iSlot,szBack);
            });
            menus_api->DisplayPlayerMenu(hMenu,iSlot,true,true);
            break;
        }
    }

}

bool StartMenuOpen(int iSlot, const char* content) {
    CCSPlayerController* player = CCSPlayerController::FromSlot(iSlot);
    if (!player || !player->IsConnected()) return false;

    bool bFound = false;
    for (auto& m : allMenus) {
        if (m.name == startMenu) {
            bFound = true;
            OpenMenu(iSlot,m.name);
            break;
        }
    }
    if (!bFound) {
        utils->PrintToChat(iSlot, "%s | Menu %s not found",g_PLAPI->GetLogTag(),startMenu.c_str());
    }
    return true;
}
void OnPlayerFullConnect(const char* szName, IGameEvent* pEvent, bool bDontBroadcast) {
    StartMenuOpen(pEvent->GetInt("userid"), "");
}

void LoadMenuFromKV(KeyValues* kv, const std::string& path)
{
    std::string filename = path;
    size_t slash = filename.find_last_of("/\\");
    if (slash != std::string::npos)
        filename = filename.substr(slash + 1);

    size_t dot = filename.find_last_of(".");
    if (dot != std::string::npos)
        filename = filename.substr(0, dot);

    META_CONPRINTF("%s | Loaded: %s from menu list\n",g_PLAPI->GetLogTag(),filename.c_str());
    KeyValues* itemsKV = kv->FindKey("Items");
    if (itemsKV) {
        vector<MenuItem> items;
        FOR_EACH_TRUE_SUBKEY(itemsKV, id)
        {
            MenuItem item;
            const char* idName = id->GetName();
            if (strcmp(idName, "Item") != 0) continue;

            item.text = id->GetString("Text", "");
            item.nextMenu = id->GetString("NextMenu", "");
            item.isDisabled = id->GetBool("disabled", false);
            items.push_back(item);
        }
        allMenus.push_back({filename,kv->GetString("Title", ""),kv->GetString("BackMenu", ""),items});
    }
}

void LoadConfig() {
    allMenus.clear();
    KeyValues* config = new KeyValues("Config");
    const char* path = "addons/configs/EasyMenu/config.ini";
    if (!config->LoadFromFile(g_pFullFileSystem, path)) {
        utils->ErrorLog("%s Failed to load: %s",g_PLAPI->GetLogTag(), path);
        delete config;
        return;
    }

    startMenu = config->GetString("start_menu", "");

    KeyValues* menus = config->FindKey("Menus");
    if (!menus) {
        utils->ErrorLog("%s No `Menus` key in config",g_PLAPI->GetLogTag());
        delete config;
        return;
    }
    for (KeyValues* sub = menus->GetFirstSubKey();sub;sub = sub->GetNextKey()) {
        const char* menuName = sub->GetName();
        char menuPath[256];
        g_SMAPI->Format(menuPath, sizeof(menuPath),
            "addons/configs/EasyMenu/menus/%s.cfg", menuName);
        KeyValues* kvMenu = new KeyValues("Menu");
        if (!kvMenu->LoadFromFile(g_pFullFileSystem,menuPath)) {
            utils->ErrorLog("%s | Failed to load menu from %s",g_PLAPI->GetLogTag(),menuPath);
            delete kvMenu;
            continue;
        }

        LoadMenuFromKV(kvMenu,menuPath);
        delete kvMenu;
    }



    delete config;
}


CON_COMMAND_F(mm_easymenu_reload,"Перезапуск файла перевода",FCVAR_SERVER_CAN_EXECUTE) {
    LoadConfig();
    META_CONPRINTF("EasyMenu reload successfully.\n");
}

void StartupServer() {
    g_pGameEntitySystem = GameEntitySystem();
    g_pEntitySystem = utils->GetCEntitySystem();
    gpGlobals = utils->GetCGlobalVars();
    META_CONPRINTF("%s Plugin started successfully.\n", g_PLAPI->GetLogTag());
}

bool EasyMenu::Load(PluginId id, ISmmAPI* ismm, char* error, size_t maxlen, bool late) {
    PLUGIN_SAVEVARS();
    GET_V_IFACE_CURRENT(GetEngineFactory, g_pCVar, ICvar, CVAR_INTERFACE_VERSION);
    GET_V_IFACE_ANY(GetEngineFactory, g_pSchemaSystem, ISchemaSystem, SCHEMASYSTEM_INTERFACE_VERSION);
    GET_V_IFACE_CURRENT(GetFileSystemFactory, g_pFullFileSystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION);
    GET_V_IFACE_CURRENT(GetEngineFactory, engine, IVEngineServer2, SOURCE2ENGINETOSERVER_INTERFACE_VERSION);
    GET_V_IFACE_ANY(GetServerFactory, g_pSource2Server, ISource2Server, SOURCE2SERVER_INTERFACE_VERSION);
    GET_V_IFACE_ANY(GetServerFactory, g_pSource2GameClients, IServerGameClients, SOURCE2GAMECLIENTS_INTERFACE_VERSION);

    ConVar_Register(FCVAR_SERVER_CAN_EXECUTE | FCVAR_GAMEDLL);
    g_SMAPI->AddListener(this,this);

    return true;
}

void EasyMenu::AllPluginsLoaded() {
    int ret;
    utils = (IUtilsApi*)g_SMAPI->MetaFactory(Utils_INTERFACE, &ret, nullptr);
    if (ret == META_IFACE_FAILED) {
        META_CONPRINTF("[EasyMenu] Missing UTILS plugin.");
        engine->ServerCommand(("meta unload " + std::to_string(g_PLID)).c_str());
        return;
    }
    menus_api = (IMenusApi*)g_SMAPI->MetaFactory(Menus_INTERFACE, &ret, nullptr);
    if (ret == META_IFACE_FAILED) {
        META_CONPRINTF("[EasyMenu] Missing UTILS plugin.");
        engine->ServerCommand(("meta unload " + std::to_string(g_PLID)).c_str());
        return;
    }
    players_api = (IPlayersApi*)g_SMAPI->MetaFactory(PLAYERS_INTERFACE, &ret, nullptr);
    if (ret == META_IFACE_FAILED) {
        META_CONPRINTF("[EasyMenu] Missing UTILS plugin.");
        engine->ServerCommand(("meta unload " + std::to_string(g_PLID)).c_str());
        return;
    }

    utils->HookEvent(g_PLID, "player_connect_full", OnPlayerFullConnect);
    utils->RegCommand(g_PLID, {"mm_openmenu"}, {"!openmenu"},StartMenuOpen);
    utils->StartupServer(g_PLID, StartupServer);
    LoadConfig();
    pluginLoaded = true;

}

bool EasyMenu::Unload(char* error, size_t maxlen) {
    utils->ClearAllHooks(g_PLID);
    ConVar_Unregister();

    return true;
}

const char* EasyMenu::GetAuthor() { return "niffox"; }
const char* EasyMenu::GetDate() { return __DATE__; }
const char* EasyMenu::GetDescription() { return "Easy Menu"; }
const char* EasyMenu::GetLicense() { return "Free"; }
const char* EasyMenu::GetLogTag() { return "Easy Menu"; }
const char* EasyMenu::GetName() { return "Easy Menu"; }
const char* EasyMenu::GetURL() { return ""; }
const char* EasyMenu::GetVersion() { return "1.0.0"; }