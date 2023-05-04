#pragma once

#include <cstdint>
#include <gl/player.hpp>
#include <dpp/message.h>
#include <dpp/dispatcher.h>

namespace gl
{
    class core;
    class database;

    enum class lobby_access{ public_, private_ };

    namespace lobby_commands
    {
        static constexpr std::string game_options = "game_options";
        static constexpr std::string lobby_options = "lobby_options";
        static constexpr std::string preset = "preset";
        static constexpr std::string access = "access";
        static constexpr std::string players = "players";
        static constexpr std::string pinged_roles = "pinged_roles";
        static constexpr std::string make = "make";
        static constexpr std::string cancel = "cancel";
        static constexpr std::string button_preset_delete = "bp_delete";
        static constexpr std::string button_preset_save = "bp_save";
        static constexpr std::string action_preset_save = "ap_save";

        //

        static constexpr std::string join = "join";
        static constexpr std::string join_secondary = "join_secondary";
        static constexpr std::string leave = "leave";

        //

        static constexpr std::string game = "game";
        static constexpr std::string game_logo = "game_logo";
        static constexpr std::string game_mod = "game_mod";
        static constexpr std::string game_map = "game_map";

        static constexpr std::string slots = "slots";
        static constexpr std::string date = "date";
        static constexpr std::string time = "time";
        static constexpr std::string host = "host";
    } // lobby_commands

    struct lobby_settings
    {
        std::string game;
        std::string game_logo;
        std::string game_mod;
        int min_slots = 2;
        int max_slots = 10;
        std::string date;
        std::string begin_time;
        std::string end_time;
        std::string map;
        std::string host;
        lobby_access access = lobby_access::public_;
        std::vector<std::string> ping_roles;
        std::vector<dpp::snowflake> players;
    };

    class lobby
    {
    public:
        explicit lobby(gl::core&, dpp::slashcommand_t );

        void build_make_message();
        void build_view_message();

        void make();

        void delete_preset();
        void load_preset(int64_t lobby_id);
        void save_preset(const std::string& name);
        void update_preset();
        void save();

        void join(gl::player);
        void leave(dpp::snowflake);

        void set_access(lobby_access access);
        void set_begin_time(const std::string& begin_time);
        void set_max_slots(int max_slots);

        dpp::snowflake id() const;
        dpp::snowflake guild_id() const;

        const dpp::message& make_message() const { return make_message_; }
        const dpp::message& view_message() const { return view_message_; }

        void refresh();
        std::string make_id(const std::string&) const;

        lobby_settings settings;

    private:
        static inline uint64_t lobby_id = 0;

        gl::database& database_;
        dpp::cluster& bot_;
        dpp::slashcommand_t source_command_;
        dpp::message make_message_;
        dpp::message view_message_;

        uint64_t id_;
        std::vector<gl::player> players_;
        int64_t preset_id_ = -1;
    };
} // gl