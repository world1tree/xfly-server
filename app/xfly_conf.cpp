//
// Created by zaiheshi on 1/20/23.
//

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "xfly_conf.h"
#include "xfly_func.h"

Config::Config() {}

Config::~Config() {
    for (auto it = m_configItemList.begin(); it != m_configItemList.end(); ++it)
        delete (*it);
    m_configItemList.clear();
}

bool Config::load(const char *pConfName) {
    FILE* fp = fopen(pConfName, "r");
    if (fp == nullptr)
        return false;
    char linebuf[501];

    while (!feof(fp)) {
        if (fgets(linebuf, 500, fp) == nullptr)
            continue;
        if (lineInvalid(linebuf))
            continue;

        char* pEqual = strchr(linebuf, '=');
        if (pEqual != nullptr) {
            // set new item;
            PConfigItem pItem = new ConfigItem{};
            strncpy(pItem->ItemName, linebuf, (int)(pEqual-linebuf));
            strcpy(pItem->ItemContent, pEqual+1);
            lStrip(pItem->ItemName);
            rStrip(pItem->ItemName);
            lStrip(pItem->ItemContent);
            rStrip(pItem->ItemContent);
            m_configItemList.push_back(pItem);
        }
    }
    fclose(fp);
    return true;
}

const char *Config::getString(const char *pItemName) {
    for (auto item: m_configItemList) {
        if (strcasecmp(item->ItemName, pItemName) == 0)
            return item->ItemContent;
    }
    return nullptr;
}

int Config::getIntDefault(const char *pItemName, const int value) {
    for (auto item: m_configItemList) {
        if (strcasecmp(item->ItemName, pItemName) == 0)
            return atoi(item->ItemContent);
    }
    return value;
}

bool Config::lineInvalid(const char *line) const {
    if (strlen(line) == 0)
        return true;
    const char* skipCharacter = "\0#;\n\t[ ";
    if (strchr(skipCharacter, *line))
        return true;
    return false;
}

std::ostream &operator<<(std::ostream &os, const Config &config) {
    os << "m_configItemList: \n";
    for (auto it: config.m_configItemList) {
        os << it->ItemName << " -> " << it->ItemContent << std::endl;
    }
    return os;
}
