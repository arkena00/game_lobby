#pragma once

#include <ndb/initializer.hpp>
#include <ndb/engine/sqlite/sqlite.hpp> // engine
#include <ndb/preprocessor.hpp> // database macros
#include <ndb/function.hpp> // ndb::clear
#include <ndb/query.hpp> // query and expression
#include <ndb/option.hpp>
#include <ndb/type.hpp>

#include <gl/config.hpp>
#include <gl/lobby.hpp>
#include <gl/utility.hpp>

namespace ndb
{
    template<class Database>
    struct custom_type<std::chrono::system_clock::time_point, Database> : basic_type<std::chrono::system_clock::time_point, ndb::int64_, Database>
    {
        static auto encode(const auto& v)
        {
            auto seconds = std::chrono::time_point_cast<std::chrono::seconds>(v);
            return seconds.time_since_epoch().count();
        }
        static auto decode(const auto& v)
        {
            return std::chrono::system_clock::time_point{ std::chrono::seconds{ v } };
        }
    };

} // ndb

ndb_table(lobby_preset,
          ndb_field_id,
          ndb_field(guild_id, int64_t),
          ndb_field(name, std::string, ndb::size<255>),
          ndb_field(game, std::string, ndb::size<255>),
          ndb_field(game_logo, std::string),
          ndb_field(mod, std::string),
          ndb_field(map, std::string, ndb::size<255>),
          ndb_field(access, int),
          ndb_field(min_slots, int),
          ndb_field(max_slots, int),
          ndb_field(begin_time, std::chrono::system_clock::time_point),
          ndb_field(end_time, std::chrono::system_clock::time_point),
          ndb_field(hostid, std::string, ndb::size<32>)
)

ndb_table(guild,
          ndb_field_id,
          ndb_field(gmt, int)
)

ndb_model(gl_model, lobby_preset, guild)

ndb_project(gl_project,
            ndb_database(gl, gl_model, ndb::sqlite)
)

// alias
namespace dbs
{
    using gl = ndb::databases::gl_project::gl_;
}

namespace gl
{
    class database
    {
    public:
        database()
        {
            ndb::connect<dbs::gl>();
        }

        void load_guilds(std::unordered_map<dpp::snowflake, gl::config>& configs)
        {
            const auto& guild = ndb::models::gl_model.guild;
            for (const auto& line : ndb::query<dbs::gl>() << (ndb::get() << ndb::source(guild)))
            {
                gl::config config;
                config.gmt = line[guild.gmt];
                configs.emplace(line[guild.id], config);
            }
        }

        void save_config(dpp::snowflake guild_id, int gmt)
        {
            const auto& guild = ndb::models::gl_model.guild;
            ndb::query<dbs::gl>() << ndb::add_replace(guild.id = int64_t(guild_id)
                                     , guild.gmt = gmt
                                      );
        }

        void load(int64_t lobby_id, gl::lobby& lobby)
        {
            const auto& lobby_preset = ndb::models::gl_model.lobby_preset;
            auto line = ndb::query<dbs::gl>() << (ndb::get() << ndb::source(lobby_preset) << ndb::filter(
            lobby_preset.guild_id == int64_t(lobby.guild_id()) &&
            lobby_preset.id == lobby_id));

            lobby.settings.access = static_cast<gl::lobby_access>(line[0][lobby_preset.access]);
            lobby.settings.begin_time = line[0][lobby_preset.begin_time];
            lobby.settings.end_time = line[0][lobby_preset.end_time];
            lobby.settings.game = line[0][lobby_preset.game];
            lobby.settings.game_logo = line[0][lobby_preset.game_logo];
            lobby.settings.game_mod = line[0][lobby_preset.mod];
            lobby.settings.host = line[0][lobby_preset.hostid];
            lobby.settings.map = line[0][lobby_preset.map];
            lobby.settings.max_slots = line[0][lobby_preset.max_slots];
            lobby.settings.min_slots = line[0][lobby_preset.min_slots];
        }

        auto save(const std::string& name, const gl::lobby& lobby)
        {
            const auto& lobby_preset = ndb::models::gl_model.lobby_preset;
            ndb::query<dbs::gl>() << ndb::add_replace(lobby_preset.guild_id = int64_t(lobby.guild_id())
                                     , lobby_preset.name = name
                                     , lobby_preset.game = lobby.settings.game
                                     , lobby_preset.game_logo = lobby.settings.game_logo
                                     , lobby_preset.mod = lobby.settings.game_mod
                                     , lobby_preset.map = lobby.settings.map
                                     , lobby_preset.max_slots = lobby.settings.max_slots
                                     , lobby_preset.min_slots = lobby.settings.min_slots
                                     , lobby_preset.access = static_cast<int>(lobby.settings.access)
                                     , lobby_preset.begin_time = lobby.settings.begin_time
                                     , lobby_preset.end_time = lobby.settings.end_time
                                     , lobby_preset.hostid = lobby.settings.host
                                      );
            return ndb::last_id<dbs::gl>();
        }


        void update(int64_t preset_id, const gl::lobby& lobby)
        {
            const auto& lobby_preset = ndb::models::gl_model.lobby_preset;
            ndb::query<dbs::gl>() << (ndb::set(lobby_preset.game = lobby.settings.game
                                     , lobby_preset.game_logo = lobby.settings.game_logo
                                     , lobby_preset.mod = lobby.settings.game_mod
                                     , lobby_preset.map = lobby.settings.map
                                     , lobby_preset.max_slots = lobby.settings.max_slots
                                     , lobby_preset.min_slots = lobby.settings.min_slots
                                     , lobby_preset.access = static_cast<int>(lobby.settings.access)
                                     , lobby_preset.begin_time = lobby.settings.begin_time
                                     , lobby_preset.end_time = lobby.settings.end_time
                                     , lobby_preset.hostid = lobby.settings.host
                                      )
                                  << ndb::filter(lobby_preset.guild_id == int64_t(lobby.guild_id())
                                                 && lobby_preset.id == preset_id));
        }

        void delete_preset(int64_t preset_id, const gl::lobby& lobby)
        {
            const auto& lobby_preset = ndb::models::gl_model.lobby_preset;
            ndb::query<dbs::gl>() << (ndb::del << ndb::source(lobby_preset) << ndb::filter(lobby_preset.guild_id == int64_t(lobby.guild_id()) && lobby_preset.id == preset_id));
        }

    private:
        ndb::initializer<ndb::sqlite> init_;
    };
} // gl