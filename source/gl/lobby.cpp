#include <gl/lobby.hpp>

#include <gl/core.hpp>
#include <gl/database.hpp>
#include <gl/utility.hpp>
#include <dpp/cluster.h>
#include <dpp/colors.h>
#include <utility>

namespace gl
{
    lobby::lobby(gl::core& core, dpp::slashcommand_t source_command)
        : core_{ core }
        , bot_{ core.bot() }
        , database_{ core.database() }
        , source_command_{ std::move(source_command) }
        , make_time_ { std::chrono::utc_clock::now() }
        , id_{ ++lobby_id }
    {
        build_make_message();
    }

    void lobby::build_make_message()
    {
        make_message_ = dpp::message();
        make_message_.channel_id = source_command_.command.channel_id;
        make_message_.content = "# Make a lobby";
        make_message_.set_flags(dpp::message_flags::m_ephemeral);

        // game options
        auto comp_options =
            dpp::component().
                     add_component(dpp::component().set_label("Game options...").
                        set_emoji("⚙️").
                        set_style(dpp::cos_success).
                        set_id(make_id(lobby_commands::game_options))
                    ).add_component(dpp::component().set_label("Lobby options...").
                        set_emoji("⚙️").
                        set_style(dpp::cos_success).
                        set_id(make_id(lobby_commands::lobby_options))
                    );

        // presets
        int presets_count = 0;
        auto comp_select_presets = dpp::component().set_type(dpp::cot_selectmenu).
                    set_label("Select preset...").
                    set_placeholder("Select preset...").
                    set_id(make_id(lobby_commands::preset));

        const auto& preset = ndb::models::gl_model.lobby_preset;
        for (auto& line : ndb::query<dbs::gl>() << (ndb::get() << ndb::source(preset) << ndb::filter(preset.guild_id == int64_t(guild_id()))))
        {
            ++presets_count;
            bool selected = false;
            auto preset_id = line[preset.id];
            std::string preset_name = line[preset.name];
            std::string description = line[preset.game];
            description += line[preset.map];
            if (preset_id == preset_id_) selected = true;
            comp_select_presets.add_select_option(dpp::select_option(line[preset.name], std::to_string(preset_id), description).set_emoji("📁").set_default(selected));
        }

        auto comp_preset =
        dpp::component().add_component(comp_select_presets);

        // pre added players
        auto comp_players =
        dpp::component().add_component(
            dpp::component().set_type(dpp::cot_user_selectmenu).
            set_placeholder("Add players").
            set_max_values(25).
            set_id(make_id(lobby_commands::players))
        );

        // pinged roles
        auto comp_ping_roles =
        dpp::component().add_component(
            dpp::component().set_type(dpp::cot_role_selectmenu).
            set_placeholder("Roles to ping").
            set_max_values(25).
            set_id(make_id(lobby_commands::pinged_roles))
        );

        auto comp_buttons =
            dpp::component().
                     add_component(dpp::component().set_label("Make").
                        set_style(dpp::cos_success).
                        set_id(make_id(lobby_commands::make))
                    ).add_component(
                        dpp::component().set_label("Cancel").
                        set_style(dpp::cos_danger).
                        set_id(make_id(lobby_commands::cancel))
                    ).add_component(
                        dpp::component().set_label("Save preset...").
                        set_style(dpp::cos_primary).
                        set_id(make_id(lobby_commands::button_preset_save))
                    ).add_component(
                        dpp::component().set_label("Delete preset").
                        set_style(dpp::cos_primary).
                        set_id(make_id(lobby_commands::button_preset_delete))
                    );


        if (presets_count > 0) make_message_.add_component(comp_preset);
        make_message_.add_component(comp_options);
        make_message_.add_component(comp_players);
        make_message_.add_component(comp_ping_roles);
        make_message_.add_component(comp_buttons);
    }

    void lobby::build_view_message()
    {
        auto mid = view_message_.id;
        view_message_ = dpp::message{};
        view_message_.id = mid;
        view_message_.channel_id = source_command_.command.channel_id;

        // date
        if (settings.begin_time == std::chrono::utc_clock::time_point{}) settings.begin_time = std::chrono::utc_clock::now();

        std::string begin_date = gl::to_string(gl::gmt_time(settings.begin_time, settings.gmt), "%d/%m/%Y");
        std::string begin_time = gl::to_string(gl::gmt_time(settings.begin_time, settings.gmt), "%H:%M");
        std::string end_date = gl::to_string(gl::gmt_time(settings.end_time, settings.gmt), "%d/%m/%Y");
        std::string end_time = gl::to_string(gl::gmt_time(settings.end_time, settings.gmt), "%H:%M");

        if (settings.end_time == std::chrono::utc_clock::time_point{}) end_time = "";

        auto today = std::chrono::floor<std::chrono::days>(std::chrono::utc_clock::now());
        auto begin_day = std::chrono::floor<std::chrono::days>(settings.begin_time);
        auto end_day = std::chrono::floor<std::chrono::days>(settings.end_time);

        if (today == begin_day) begin_date = "Today";
        if (today == end_day) end_date = "";
        if (!end_time.empty()) end_time = " - " + end_time;

        std::string gmt = "(GMT";
        gmt += (settings.gmt >= 0 ? "+" : "-") + std::to_string(settings.gmt) + ")";

        std::string description;
        if (!settings.game.empty()) description = "- :video_game: **" + settings.game + (settings.game_mod.empty() ? "" : " [" + settings.game_mod + "]") + "**\n";
        description += "- :calendar_spiral: **" + begin_date + "**\n";
        description += "- :clock10: **" + begin_time + end_time + "** " + gmt + "\n";
        if (!settings.map.empty()) description += "- :map: **" + settings.map + "**\n";
        if (!settings.host.empty()) description += "- :bust_in_silhouette: **<@" + settings.host + ">**\n";

        std::string primary_players;
        std::string secondary_players;
        std::string waiting_players;
        int primary_players_count = 0;
        int secondary_players_count = 0;
        int waiting_players_count = 0;
        for (const auto& player : players_)
        {
            if (player->group == player_group::primary)
            {
                if (primary_players_count + waiting_players_count >= settings.max_slots)
                {
                    waiting_players += std::to_string(waiting_players_count + 1) + ". " + player->str() + "\n";
                    ++waiting_players_count;
                }
                else
                {
                    primary_players += std::to_string(primary_players_count + 1) + ". " + player->str() + "\n";
                    ++primary_players_count;
                }
            }
            else
            {
                secondary_players += std::to_string(secondary_players_count + 1) + ". " + player->str() + "\n";
                ++secondary_players_count;
            }
        }
        for (int i = 0; i < settings.min_slots - primary_players_count; ++i)
        {
            primary_players += std::to_string(primary_players_count + i + 1) + ". <available slot>\n";
        }


        std::string fill_status = primary_players_count >= settings.min_slots ? ":white_check_mark:" : ":question:";
        std::string lobby_access = settings.access == lobby_access::private_ ? ":lock:" : "";

        dpp::embed embed = dpp::embed().
        set_color(dpp::colors::blue_green).
        set_title(settings.game + lobby_access).
        //set_author("Among Us Lobby", "", "https://logos-world.net/wp-content/uploads/2021/08/Among-Us-Logo.png").
        //set_author("GameLobby", "https://github.com/arkena00/", "https://avatars.githubusercontent.com/u/4370057?v=4").
        set_description(description).
        set_thumbnail(settings.game_logo).
        add_field(
               fill_status + " Primary (" + std::to_string(primary_players_count) + "/" + std::to_string(settings.max_slots) + ")",
               primary_players,
               true
        ).
        set_footer(dpp::embed_footer().
                                      set_text("GameLobby " + core_.str_version() + "\nJoin as secondary if you are not sure to be present").
                                      set_icon("https://cdn.discordapp.com/app-icons/1100304468244975677/e27284e425960d09cabaa43c30107e57.png?size=256")).
        set_timestamp(time(nullptr));

        if (secondary_players_count > 0)
        {
            embed.add_field(
                ":recycle: Secondary (" + std::to_string(secondary_players_count) + ")",
                secondary_players,
                true
            );
        }
        if (waiting_players_count > 0)
        {
            embed.add_field(
                ":clock2: Waiting (" + std::to_string(waiting_players_count) + ")",
                waiting_players,
                false
            );
        }

        view_message_.add_embed(embed);

        std::string str_ping_roles;
        for (const auto& role : settings.ping_roles)
        {
            str_ping_roles += "<@&" + role + ">";
            view_message_.allowed_mentions.roles.emplace_back(role);
        }
        view_message_.set_content(str_ping_roles);

        if (settings.access == lobby_access::public_)
        {
            view_message_.add_component(
                dpp::component().
                add_component(dpp::component().set_label("Join").set_emoji("🎮").
                    set_style(dpp::cos_success).
                    set_id(make_id("join"))).
                add_component(dpp::component().set_label("Join(secondary)").
                    set_style(dpp::cos_primary).
                    set_id(make_id("join_secondary"))).
                add_component(
                    dpp::component().set_label("Leave").
                    set_style(dpp::cos_danger).
                    set_id(make_id("leave")))
             );

            view_message_.add_component(
                dpp::component().add_component(
                    dpp::component().set_label("Notify me").set_emoji("🕙").
                    set_style(dpp::cos_secondary).
                    set_id(make_id(lobby_commands::notify_options))
                )
            );
        }
    }

    void lobby::refresh()
    {
        if (state_ == lobby_state::idle)
        {
            build_make_message();
            bot_.message_edit(make_message());
        }
        else if (state_ == lobby_state::active)
        {
            build_view_message();
            bot_.message_edit(view_message());
        }
    }

    std::string lobby::make_id(const std::string& id) const
    {
        return std::to_string(id_) + "|" + id;
    }

    void lobby::make()
    {
        build_view_message();
        bot_.message_create(view_message(), [this](const dpp::confirmation_callback_t& r) {
            if (!r.is_error())
            {
                auto& message = std::get<dpp::message>(r.value);
                view_message_.id = message.id;
                state_ = lobby_state::active;
            }
            else std::cout << "make error";
        });
    }

    void lobby::load_preset(int64_t preset_id)
    {
        database_.load(preset_id, *this);
        preset_id_ = preset_id;
        build_make_message();
    }

    void lobby::delete_preset()
    {
        if (preset_id_ == -1) return;

        database_.delete_preset(preset_id_, *this);
        build_make_message();
    }

    void lobby::save_preset(const std::string& name)
    {
        preset_id_ = database_.save(name, *this);
        build_make_message();
    }

    void lobby::update_preset()
    {
        if (preset_id_ == -1) return;
        database_.update(preset_id_, *this);
    }

    void lobby::join(gl::player player)
    {
        // player already joined as primary
        if (player.group == player_group::primary)
        {
            if (players_.end() != std::find_if(players_.begin(), players_.end(),[player](const auto& p) {
                return p->group == player_group::primary && p->id == player.id; }
                                           )) return;
        }

        players_.erase(std::remove_if(players_.begin(), players_.end(), [player](const auto& p) { return p->id == player.id; }), players_.end());
        players_.emplace_back(std::make_unique<gl::player>(player));
        refresh();
    }

    void lobby::leave(dpp::snowflake user_id)
    {
        players_.erase(std::remove_if(players_.begin(), players_.end(), [user_id](const auto& p) { return p->id == user_id; }), players_.end());
        refresh();
    }

    dpp::snowflake lobby::id() const { return id_; }
    dpp::snowflake lobby::guild_id() const { return source_command_.command.guild_id; }

    gl::player* lobby::player(dpp::snowflake user_id) const
    {
        auto it = std::find_if(players_.begin(), players_.end(), [user_id](const auto& p) { return p->id == user_id; });
        if (it != players_.end()) return it->get();

        return nullptr;
    }

    std::vector<std::unique_ptr<gl::player>>& lobby::players() { return players_; }
    const std::chrono::utc_clock::time_point& lobby::make_time() const { return make_time_; }
    lobby_state lobby::state() const { return state_; }
    bool lobby::has_expired() const
    {
        auto now = std::chrono::utc_clock::now();

        return ((state() == lobby_state::idle && now - make_time() > core_.lobby_max_idle_duration)
        || (state() == gl::lobby_state::active && now - make_time() > core_.lobby_max_alive_duration));
    }
} // gl