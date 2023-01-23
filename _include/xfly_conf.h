//
// Created by zaiheshi on 1/20/23.
//

#ifndef XFLY_XFLY_CONF_H
#define XFLY_XFLY_CONF_H

#include <vector>
#include <ostream>
#include "xfly_global.h"


class Config {
private:
    inline static Config *m_instance;
    std::vector<PConfigItem> m_configItemList;
private:
    Config();

    bool lineInvalid(const char *line) const;

    class GCClass {
    public:
        ~GCClass() {
            if (Config::m_instance) {
                delete Config::m_instance;
                Config::m_instance = nullptr;
            }
        }
    };

public:

    ~Config();

    static Config *getInstance() {
        if (m_instance == nullptr) {
            m_instance = new Config{};
            static GCClass gc; // 这里必须这么写，否则存在内存泄漏，因为m_instance这段内存无法释放, 除非程序结尾手动delete
        }
        return m_instance;
    }

    bool load(const char *pConfName);

    friend std::ostream &operator<<(std::ostream &os, const Config &config);

    const char *getString(const char *pItemName);

    int getIntDefault(const char *pItemName, const int value);
};


#endif //XFLY_XFLY_CONF_H
