#pragma once

#include <gl/lobby.hpp>
#include <string>
#include <iostream>

namespace gl
{
    inline std::string to_string(lobby_access access)
    {
        switch (access)
        {
            case lobby_access::private_: return "private";
            case lobby_access::public_: return "public";
        }
    }

    inline std::string current_date()
    {
        time_t now = time(nullptr);
        struct tm tstruct{};
        char buf[32];
        localtime_s(&tstruct, &now);
        strftime(buf, sizeof(buf), "%d/%m/%Y", &tstruct);

        return buf;
    }

    inline std::string make_id(lobby* lobby_ptr, const std::string& id)
    {
        return std::to_string(lobby_ptr->id()) + "|" + id;
    }

    inline void parse_slots(const std::string& str_slots, gl::lobby_settings& settings)
    {
        auto sep_pos = str_slots.find('-');
        if (sep_pos != std::string::npos)
        {
            settings.min_slots = std::stoi(str_slots.substr(0, sep_pos));
            settings.max_slots = std::stoi(str_slots.substr(sep_pos + 1));
        }
        else
        {
            int slots = std::stoi(str_slots);
            settings.min_slots = slots;
            settings.max_slots = slots;
        }
    }

    inline void parse_time(const std::string& str_time, gl::lobby_settings& settings)
    {
        auto sep_pos = str_time.find('-');

        if (sep_pos != std::string::npos)
        {
            settings.begin_time = str_time.substr(0, sep_pos);
            settings.end_time = str_time.substr(sep_pos + 1);
        }
        else
        {
            settings.begin_time = str_time;
            settings.end_time = "";
        }
    }
} // gl