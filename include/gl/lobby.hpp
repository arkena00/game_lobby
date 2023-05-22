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
    enum class lobby_state{ idle, active, inactive };

    namespace lobby_commands
    {
        static std::string game_options = "game_options";
        static std::string lobby_options = "lobby_options";
        static std::string preset = "preset";
        static std::string access = "access";
        static std::string players = "players";
        static std::string players_remove = "players_remove";
        static std::string pinged_roles = "pinged_roles";
        static std::string make = "make";
        static std::string edit = "edit";
        static std::string cancel = "cancel";
        static std::string button_preset_delete = "bp_delete";
        static std::string button_preset_save = "bp_save";
        static std::string action_preset_save = "ap_save";

        //

        static std::string join = "join";
        static std::string join_secondary = "join_secondary";
        static std::string leave = "leave";
        static std::string notify_options = "notify_options";

        //

        static std::string game = "game";
        static std::string game_logo = "game_logo";
        static std::string game_mod = "game_mod";
        static std::string game_map = "game_map";

        static std::string slots = "slots";
        static std::string begin_time = "begin_time";
        static std::string end_time = "end_time";
        static std::string host = "host";

        static std::string notify_timer = "notify_timer";
        static std::string notify_primary = "notify_primary";
    } // lobby_commands

    struct lobby_settings
    {
        std::string game;
        std::string game_logo;
        std::string game_mod;
        int min_slots = 2;
        int max_slots = 10;
        std::chrono::utc_clock::time_point begin_time;
        std::chrono::utc_clock::time_point end_time;
        std::string map;
        std::string host;
        lobby_access access = lobby_access::public_;
        std::vector<std::string> ping_roles;
        std::vector<dpp::snowflake> players;
        int gmt = 0;
    };

    class lobby
    {
    public:
        explicit lobby(gl::core&, dpp::slashcommand_t );

        void build_make_message();
        void build_edit_message();
        void build_view_message();

        void make();
        void end();

        void begin_editing();

        void delete_preset();
        void load_preset(int64_t lobby_id);
        void save_preset(const std::string& name);
        void update_preset();

        void join(gl::player);
        void leave(dpp::snowflake);

        [[nodiscard]] dpp::snowflake id() const;
        [[nodiscard]] dpp::snowflake guild_id() const;
        [[nodiscard]] gl::player* player(dpp::snowflake) const;
        std::vector<std::unique_ptr<gl::player>>& players();
        const std::chrono::utc_clock::time_point& make_time() const;
        lobby_state state() const;
        bool has_expired() const;
        std::chrono::utc_clock::time_point expiration_time() const;

        [[nodiscard]] const dpp::message& make_message() const { return make_message_; }
        [[nodiscard]] const dpp::message& edit_message() const { return edit_message_; }
        [[nodiscard]] const dpp::message& view_message() const { return view_message_; }

        void refresh();
        [[nodiscard]] std::string make_id(const std::string&) const;

        lobby_settings settings;

    private:
        static inline uint64_t lobby_id = 0;

        gl::database& database_;
        gl::core& core_;
        dpp::cluster& bot_;
        dpp::slashcommand_t source_command_;
        dpp::message make_message_;
        dpp::message edit_message_;
        dpp::message view_message_;

        uint64_t id_;
        std::vector<std::unique_ptr<gl::player>> players_;
        int64_t preset_id_ = -1;
        lobby_state state_ = lobby_state::idle;
        std::chrono::utc_clock::time_point make_time_;
    };
} // gl