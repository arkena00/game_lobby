#include <gl/core.hpp>

#include <gl/ui.hpp>
#include <dpp/dpp.h>

namespace gl
{
    core::core(const std::string& token)
        : bot_{ token, dpp::i_default_intents | dpp::i_message_content }
        , reminder_{ *this }
    {
        database().load_guilds(configs_);

        bot_.on_log(dpp::utility::cout_logger());

        bot_.on_ready([&](const dpp::ready_t& event) {
            if (dpp::run_once<struct register_bot__commands>())
            {
                std::vector<dpp::slashcommand> commands;
                commands.emplace_back("gl", "Make lobby", bot_.me.id);
                commands.emplace_back("gledit", "Edit the last lobby", bot_.me.id);

                dpp::slashcommand config("glconfig", "Configure", bot_.me.id);
                config.add_option(
                        dpp::command_option(dpp::co_integer, "gmt", "Enter your time zone", true)
                        );

                commands.emplace_back(std::move(config));
                bot_.global_bulk_command_create(commands);
            }
        });

        bot_.on_slashcommand([this](const dpp::slashcommand_t& event) {
            if (event.command.get_command_name() == "gl")
            {
                lobbies_.emplace_back(std::make_unique<gl::lobby>(*this, event));
                lobbies_.back()->settings.gmt = configs_[event.command.guild_id].gmt;
                update_presence();

                event.reply(lobbies_.back()->make_message());
            }
            else if (event.command.get_command_name() == "gledit")
            {
                auto guild_id = event.command.guild_id;
                auto it = std::find_if(lobbies_.rbegin(), lobbies_.rend(), [guild_id](const auto& lobby) { return lobby->guild_id() == guild_id; });
                if (it != lobbies_.rend())
                {
                    auto& lobby = *it;
                    lobby->begin_editing();
                    event.reply(lobby->edit_message());
                }
                else event.reply("No lobby found");
            }
            else if (event.command.get_command_name() == "glconfig")
            {
                auto gmt = std::get<int64_t>(event.get_parameter("gmt"));
                configs_[event.command.guild_id].gmt = gmt;
                database().save_config(event.command.guild_id, gmt);

                dpp::message message;
                message.set_flags(dpp::message_flags::m_ephemeral);
                message.content = "Time zone set to GMT+" + std::to_string(gmt);
                event.reply(message);
            }
        });

        bot_.on_select_click([this](const dpp::select_click_t& event) {
            auto [lobby_id, command_id] = parse_id(event.custom_id);

            if (auto* lobby_ptr = lobby(event.command.guild_id, lobby_id))
            {
                if (command_id == lobby_commands::preset)
                {
                    if (!event.values.empty()) lobby_ptr->load_preset(std::stoll(event.values[0]));
                    lobby_ptr->build_make_message();
                    event.reply(dpp::interaction_response_type::ir_update_message, lobby_ptr->make_message());
                }
                else if (command_id == lobby_commands::pinged_roles)
                {
                    for (const auto& role_id: event.values)
                        lobby_ptr->settings.ping_roles.emplace_back(role_id);
                    event.reply(dpp::interaction_response_type::ir_update_message, lobby_ptr->make_message());
                }
                else if (command_id == lobby_commands::players)
                {
                    for (const auto& player : event.values)
                        lobby_ptr->join(gl::player{ std::stoull(player) });
                    event.reply();
                }
                else if (command_id == lobby_commands::players_remove)
                {
                    for (const auto& player : event.values)
                        lobby_ptr->leave(player);
                    event.reply();
                }
                else event.reply();
            }
        });

        bot_.on_button_click([this](const dpp::button_click_t& event) {
            auto [lobby_id, command_id] = parse_id(event.custom_id);

            if (auto* lobby_ptr = lobby(event.command.guild_id, lobby_id))
            {
                if (command_id == lobby_commands::make) lobby_ptr->make();
                else if (command_id == lobby_commands::notify_options)
                {
                    auto modal = ui::make_notify_options(lobby_ptr, event.custom_id);
                    return event.dialog(modal);
                }
                else if (command_id == lobby_commands::button_preset_save)
                {
                    dpp::interaction_modal_response modal(gl::make_id(lobby_ptr, gl::lobby_commands::action_preset_save), "");
                    modal.add_component(
                        dpp::component().
                                set_id("name").
                                set_label("Save preset").
                                set_type(dpp::cot_text).
                                set_required(true).
                                set_min_length(1).
                                set_max_length(32).
                                set_text_style(dpp::text_short));

                    return event.dialog(modal);
                }
                else if (command_id == lobby_commands::button_preset_delete)
                {
                    lobby_ptr->delete_preset();
                    event.reply(dpp::interaction_response_type::ir_update_message, lobby_ptr->make_message());
                    return;
                }
                else if (command_id == lobby_commands::join) lobby_ptr->join(gl::player{ event.command.usr.id });
                else if (command_id == lobby_commands::join_secondary) lobby_ptr->join(gl::player{ event.command.usr.id , player_group::secondary});
                else if (command_id == lobby_commands::leave) lobby_ptr->leave(event.command.usr.id);
                else if (command_id == lobby_commands::game_options)
                {
                    auto modal = ui::make_game_options(lobby_ptr, event.custom_id);
                    return event.dialog(modal);
                }
                else if (command_id == lobby_commands::lobby_options)
                {
                    auto modal = ui::make_lobby_options(lobby_ptr, event.custom_id);
                    return event.dialog(modal);
                }
                event.reply();
            }
        });

        bot_.on_form_submit([&](const dpp::form_submit_t& event) {

            auto [lobby_id, command_id] = parse_id(event.custom_id);

            if (auto* lobby_ptr = lobby(event.command.guild_id, lobby_id))
            {
                if (command_id == lobby_commands::game_options)
                {
                    for (const auto& comp : event.components)
                    {
                        if (comp.components.empty()) continue;
                        const auto& field = comp.components[0];

                        if (field.custom_id == lobby_commands::game)
                            lobby_ptr->settings.game = std::get<std::string>(field.value);
                        else if (field.custom_id == lobby_commands::game_logo)
                            lobby_ptr->settings.game_logo = std::get<std::string>(field.value);
                        else if (field.custom_id == lobby_commands::game_mod)
                            lobby_ptr->settings.game_mod = std::get<std::string>(field.value);
                        else if (field.custom_id == lobby_commands::game_map)
                            lobby_ptr->settings.map = std::get<std::string>(field.value);
                    }
                }
                else if (command_id == lobby_commands::lobby_options)
                {
                    for (const auto& comp : event.components)
                    {
                        if (comp.components.empty()) continue;
                        const auto& field = comp.components[0];

                        if (field.custom_id == lobby_commands::access)
                        {
                            if (std::get<std::string>(field.value) == "y")
                                lobby_ptr->settings.access = lobby_access::public_;
                            else lobby_ptr->settings.access = lobby_access::private_;
                        }
                        if (field.custom_id == lobby_commands::slots)
                            gl::parse_slots(std::get<std::string>(field.value), lobby_ptr->settings);
                        else if (field.custom_id == lobby_commands::begin_time)
                        {
                            // remove gmt, store as UTC
                            auto gmt = configs_[event.command.guild_id].gmt;
                            lobby_ptr->settings.begin_time = gl::gmt_time(gl::to_time_point(std::get<std::string>(field.value)), -gmt);
                        }
                        else if (field.custom_id == lobby_commands::end_time)
                        {
                            std::string str_time = std::get<std::string>(field.value);
                            if (!str_time.empty())
                            {
                                // remove gmt, store as UTC
                                auto gmt = configs_[event.command.guild_id].gmt;
                                lobby_ptr->settings.end_time = gl::gmt_time(gl::to_time_point(std::get<std::string>(field.value)), -gmt);
                            }
                        }
                        else if (field.custom_id == lobby_commands::host)
                            lobby_ptr->settings.host = std::get<std::string>(field.value);
                    }
                }
                else if (command_id == lobby_commands::action_preset_save)
                {
                     if (!event.components.empty()) lobby_ptr->save_preset(std::get<std::string>(event.components[0].components[0].value));
                }
                else if (command_id == lobby_commands::notify_options)
                {
                    int notify_time = 5;
                    bool notify_primary = false;
                    for (const auto& comp : event.components)
                    {
                        if (comp.components.empty()) continue;
                        const auto& field = comp.components[0];

                        if (field.custom_id == lobby_commands::notify_timer)
                            notify_time = std::stoi(std::get<std::string>(field.value));
                        else if (field.custom_id == lobby_commands::notify_primary)
                            notify_primary = std::get<std::string>(field.value) == "y" ? true : false;
                    }
                     //auto reminder_primary = std::stoi(std::get<std::string>(event.components[0].components[1].value));
                     reminder_.add(lobby_ptr, event.command, notify_time, notify_primary);

                     return event.reply();
                }

                // edit
                if (lobby_ptr->state() == gl::lobby_state::active)
                {
                     lobby_ptr->refresh();
                     event.reply();
                     return;
                }

                lobby_ptr->update_preset();
                lobby_ptr->build_make_message();
                event.reply(dpp::interaction_response_type::ir_update_message, lobby_ptr->make_message());
            }
        });
    }
    
    void core::run()
    {
        std::thread t{ [this]{ reminder_.run(); } };
        t.detach();

        try
        {
            bot_.start(dpp::st_wait);
        } catch (...)
        {
            std::cout << "Bot exception";
            restart();
        }
    }

    void core::notify(dpp::snowflake user_id, const dpp::message& message)
    {
        bot_.direct_message_create(user_id, message);
    }

    void core::del_lobby(uint64_t lobby_id)
    {
        auto it = std::find_if(lobbies_.begin(), lobbies_.end(), [lobby_id](const auto& l) { return l->id() == lobby_id; });
        if (it == lobbies_.end()) return;

        std::iter_swap(it, lobbies_.end() - 1);
        lobbies_.back()->end();
        lobbies_.pop_back();
        update_presence();
    }

    gl::lobby* core::lobby(dpp::snowflake guild_id, dpp::snowflake lobby_id)
    {
        for (const auto& lobby : lobbies_)
        {
            if (lobby->guild_id() == guild_id && lobby->id() == lobby_id) return lobby.get();
        }

        return nullptr;
    }

    std::tuple<uint64_t, std::string> core::parse_id(const std::string& custom_id)
    {
        auto sep = custom_id.find('|');
        uint64_t lobby_id = std::stoull(custom_id.substr(0, sep));
        std::string id = custom_id.substr(sep + 1);
        return { lobby_id, id };
    }

    void core::restart()
    {
        bot_.start();
    }

    void core::update_presence()
    {
        std::string presence_message;

        if (!lobbies().empty()) presence_message = "Lobbies: " + std::to_string(lobbies().size());
        bot_.set_presence(dpp::presence(dpp::presence_status::ps_online, dpp::activity_type::at_game, presence_message));
    }

    std::string core::str_version() const
    {
        return "v1.0.5";
    }

    void core::error(const std::string& message)
    {
        bot_.log(dpp::loglevel::ll_error, message);
    }
    void core::log(const std::string& message)
    {
        bot_.log(dpp::loglevel::ll_info, message);
    }
} // gl