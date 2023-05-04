#include <gl/core.hpp>

#include <gl/ui.hpp>
#include <dpp/dpp.h>

namespace gl
{
    core::core(const std::string& token)
        : bot_{ token, dpp::i_default_intents | dpp::i_message_content }
    {
        bot_.on_log(dpp::utility::cout_logger());

        bot_.on_ready([&](const dpp::ready_t & event) {
            if (dpp::run_once<struct register_bot__commands>()) {
                bot_.global_command_create(dpp::slashcommand("gl", "GameLobby creation menu", bot_.me.id));
            }
        });

        bot_.on_slashcommand([this](const dpp::slashcommand_t& event) {
            if (event.command.get_command_name() == "gl")
            {
                lobbies_.emplace_back(std::make_unique<gl::lobby>(*this, event));

                event.reply(lobbies_.back()->make_message());
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
                    event.reply(dpp::interaction_response_type::ir_update_message, lobby_ptr->make_message());
                }
                else event.reply();
            }
        });

        bot_.on_button_click([this](const dpp::button_click_t& event) {
            auto [lobby_id, command_id] = parse_id(event.custom_id);

            if (auto* lobby_ptr = lobby(event.command.guild_id, lobby_id))
            {
                if (command_id == lobby_commands::make) lobby_ptr->make();
                else if (command_id == lobby_commands::cancel) event.delete_original_response();
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
                else if (command_id == lobby_commands::join_secondary) lobby_ptr->join(gl::player{ event.command.usr.id , player_priority::secondary});
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
                        else if (field.custom_id == lobby_commands::time)
                            gl::parse_time(std::get<std::string>(field.value), lobby_ptr->settings);
                        else if (field.custom_id == lobby_commands::date)
                        {
                            lobby_ptr->settings.date = std::get<std::string>(field.value);
                            if (lobby_ptr->settings.date.empty())
                            {
                                lobby_ptr->settings.date = gl::current_date();
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

                lobby_ptr->update_preset();
                lobby_ptr->build_make_message();
                event.reply(dpp::interaction_response_type::ir_update_message, lobby_ptr->make_message());
            }
        });
    }
    
    void core::run()
    {
        bot_.start(dpp::st_wait);
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
} // gl