#pragma once

#include <dpp/cluster.h>
#include <gl/config.hpp>
#include <gl/database.hpp>
#include <gl/lobby.hpp>
#include <gl/scheduler.hpp>

namespace gl
{
    class core
    {
    public:
        core(const std::string& token);

        void run();
        void notify(dpp::snowflake user_id, const dpp::message&);
        void del_lobby(uint64_t lobby_id);
        void update_presence();

        gl::database& database() { return database_; }
        dpp::cluster& bot() { return bot_; }
        std::vector<std::unique_ptr<gl::lobby>>& lobbies() { return lobbies_; }
        gl::lobby* lobby(dpp::snowflake guild_id, dpp::snowflake lobby_id);
        std::string str_version() const;

        std::chrono::minutes lobby_max_idle_duration = std::chrono::minutes { 1 };
        std::chrono::minutes lobby_max_alive_duration = std::chrono::minutes { 2 };

    protected:
        static std::tuple<uint64_t, std::string> parse_id(const std::string&);

    private:
        gl::database database_;
        dpp::cluster bot_;
        gl::scheduler reminder_;

        std::unordered_map<dpp::snowflake, gl::config> configs_;
        std::vector<std::unique_ptr<gl::lobby>> lobbies_;
    };
} // gl