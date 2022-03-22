﻿#include <neolib/neolib.hpp>
#include <csignal>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <neolib/core/random.hpp>
#include <neolib/task/thread_pool.hpp>
#include <neolib/app/i_power.hpp>
#include <neogfx/neogfx.hpp>
#include <neogfx/core/easing.hpp>
#include <neogfx/core/i_transition_animator.hpp>
#include <neogfx/hid/i_surface.hpp>
#include <neogfx/hid/i_surface_manager.hpp>
#include <neogfx/hid/i_game_controllers.hpp>
#include <neogfx/gfx/graphics_context.hpp>
#include <neogfx/gui/widget/item_model.hpp>
#include <neogfx/gui/widget/item_presentation_model.hpp>
#include <neogfx/gui/widget/table_view.hpp>
#include <neogfx/gui/dialog/color_dialog.hpp>
#include <neogfx/gui/dialog/message_box.hpp>
#include <neogfx/gui/dialog/font_dialog.hpp>
#include <neogfx/gui/dialog/game_controller_dialog.hpp>
#include <neogfx/app/file_dialog.hpp>
#include <neogfx/game/ecs.hpp>
#include <neogfx/game/clock.hpp>
#include <neogfx/game/rigid_body.hpp>
#include <neogfx/game/rectangle.hpp>
#include <neogfx/game/game_world.hpp>
#include <neogfx/game/simple_physics.hpp>
#include <neogfx/game/collision_detector.hpp>
#include <neogfx/game/mesh_renderer.hpp>
#include <neogfx/game/mesh_render_cache.hpp>
#include <neogfx/game/animator.hpp>
#include <neogfx/game/time.hpp>

#include "test.ui.hpp"

namespace ng = neogfx;
using namespace ng::string_literals;
using namespace ng::unit_literals;

class my_item_model : public ng::basic_item_model<void*, 9u>
{
public:
    my_item_model()
    {
        set_column_name(0, "Zero");
        set_column_name(1, "One");
        set_column_name(2, "Two");
        set_column_name(3, "Three");
        set_column_name(4, "Four");
        set_column_name(5, "Five");
        set_column_name(6, "Six");
        set_column_name(7, "Empty");
        set_column_name(8, "Eight");
    }
public:
    const ng::item_cell_data& cell_data(const ng::item_model_index& aIndex) const override
    {
        if (aIndex.column() == 4)
        {
            if (aIndex.row() == 4)
            {
                static const ng::item_cell_data sReadOnly = { "** Read Only **" };
                return sReadOnly;
            }
            if (aIndex.row() == 5)
            {
                static const ng::item_cell_data sUnselectable = { "** Unselectable **" };
                return sUnselectable;
            }
        }
        return ng::basic_item_model<void*, 9u>::cell_data(aIndex);
    }
};

template <typename Model>
class my_basic_item_presentation_model : public ng::basic_item_presentation_model<Model>
{
    typedef ng::basic_item_presentation_model<Model> base_type;
public:
    typedef Model model_type;
public:
    my_basic_item_presentation_model(model_type& aModel, bool aSortable = false) :
        base_type{ aModel, aSortable },
        iCellImages{ {
            ng::image{ ":/test/resources/icon.png" }, 
            ng::image{ ":/closed/resources/caw_toolbar.zip#contacts.png" },
            ng::image{ ":/closed/resources/caw_toolbar.zip#favourite.png" },
            ng::image{ ":/closed/resources/caw_toolbar.zip#folder.png" },
            {}
        } }
    {
    }
public:
    ng::item_cell_flags cell_flags(const ng::item_presentation_model_index& aIndex) const override
    {
        if constexpr (model_type::container_traits::is_flat)
        {
            auto const modelIndex = base_type::to_item_model_index(aIndex);
            if (modelIndex.column() == 4)
            {
                if (modelIndex.row() == 4)
                    return base_type::cell_flags(aIndex) & ~ng::item_cell_flags::Editable;
                if (modelIndex.row() == 5)
                    return base_type::cell_flags(aIndex) & ~ng::item_cell_flags::Selectable;
            }
        }
        return base_type::cell_flags(aIndex);
    }
    void set_color_role(const std::optional<ng::color_role>& aColorRole)
    {
        iColorRole = aColorRole;
    }
    ng::optional_color cell_color(const ng::item_presentation_model_index& aIndex, ng::color_role aColorRole) const override
    {
        // use seed to make random color based on row/index ...
        neolib::basic_random<double> prng{ (base_type::to_item_model_index(aIndex).row() << 16) + base_type::to_item_model_index(aIndex).column() };
        auto const textColor = ng::service<ng::i_app>().current_style().palette().color(ng::color_role::Text);
        auto const backgroundColor = ng::service<ng::i_app>().current_style().palette().color(aIndex.row() % 2 == 0 ? ng::color_role::Base : ng::color_role::AlternateBase);
        if (aColorRole == iColorRole)
            return ng::color{ prng(0.0, 1.0), prng(0.0, 1.0), prng(0.0, 1.0) }.with_lightness((aColorRole == ng::color_role::Text ? textColor.light() : backgroundColor.light()) ? 0.85 : 0.15);
        else if (aColorRole == ng::color_role::Base && iColorRole)
            return ng::color::Black;
        else if (aColorRole == ng::color_role::Background)
            return backgroundColor;
        return {};
    }
    ng::optional_texture cell_image(const ng::item_presentation_model_index& aIndex) const override
    {
        if (base_type::column_image_size(aIndex.column()))
        {
            auto const idx = (std::hash<uint32_t>{}(base_type::to_item_model_index(aIndex).row()) + std::hash<uint32_t>{}(base_type::to_item_model_index(aIndex).column())) % 5;
            return iCellImages[idx];
        }
        return ng::optional_texture{};
    }
private:
    std::optional<ng::color_role> iColorRole;
    std::array<ng::optional_texture, 5> iCellImages;
};

typedef my_basic_item_presentation_model<my_item_model> my_item_presentation_model;
typedef my_basic_item_presentation_model<ng::item_tree_model> my_item_tree_presentation_model;

class easing_item_presentation_model : public ng::default_drop_list_presentation_model<ng::basic_item_model<ng::easing>>
{
    typedef ng::default_drop_list_presentation_model<ng::basic_item_model<ng::easing>> base_type;
public:
    easing_item_presentation_model(ng::drop_list& aDropList, ng::basic_item_model<ng::easing>& aModel, bool aLarge = true) : base_type{ aDropList, aModel }, iLarge{ aLarge }
    {
        iSink += ng::service<ng::i_app>().current_style_changed([this](ng::style_aspect)
        {
            iTextures.clear();
            VisualAppearanceChanged.async_trigger();
        });
    }
public:
    ng::optional_texture cell_image(const ng::item_presentation_model_index& aIndex) const override
    {
        auto easingFunction = item_model().item(to_item_model_index(aIndex));
        auto iterTexture = iTextures.find(easingFunction);
        if (iterTexture == iTextures.end())
        {
            ng::dimension const d = iLarge ? 48.0 : 24.0;
            ng::texture newTexture{ ng::size{d, d}, 1.0, ng::texture_sampling::Multisample };
            ng::graphics_context gc{ newTexture };
            ng::scoped_snap_to_pixel snap{ gc };
            auto const textColor = ng::service<ng::i_app>().current_style().palette().color(ng::color_role::Text);
            gc.draw_rect(ng::rect{ ng::point{}, ng::size{d, d} }, ng::pen{ textColor, 1.0 });
            ng::optional_point lastPos;
            ng::pen pen{ textColor, 2.0 };
            for (double x = 0.0; x <= d - 8.0; x += 2.0)
            {
                ng::point pos{ x + 4.0, ng::ease(easingFunction, x / (d - 8.0)) * (d - 8.0) + 4.0 };
                if (lastPos != std::nullopt)
                {
                    gc.draw_line(*lastPos, pos, pen);
                }
                lastPos = pos;
            }
            iterTexture = iTextures.emplace(easingFunction, newTexture).first;
        }
        return iterTexture->second;
    }
private:
    bool iLarge;
    mutable std::map<ng::easing, ng::texture> iTextures;
    ng::sink iSink;
};

class keypad_button : public ng::push_button
{
public:
    keypad_button(ng::text_edit& aTextEdit, uint32_t aNumber) :
        ng::push_button{ boost::lexical_cast<std::string>(aNumber) }, iTextEdit{ aTextEdit }
    {
        clicked([this, aNumber]()
        {
            ng::service<ng::i_app>().change_style("Keypad").
                palette().set_color(ng::color_role::Theme, aNumber != 9 ? ng::color{ aNumber & 1 ? 64 : 0, aNumber & 2 ? 64 : 0, aNumber & 4 ? 64 : 0 } : ng::color::LightGoldenrod);
            if (aNumber == 9)
                iTextEdit.set_default_style(ng::text_edit::character_style{ ng::optional_font{}, ng::gradient{ ng::color::DarkGoldenrod, ng::color::LightGoldenrodYellow, ng::gradient_direction::Horizontal }, ng::color_or_gradient{} });
            else if (aNumber == 8)
                iTextEdit.set_default_style(ng::text_edit::character_style{ ng::font{"SnareDrum One NBP", "Regular", 60.0}, ng::color::White });
            else if (aNumber == 0)
                iTextEdit.set_default_style(ng::text_edit::character_style{ ng::font{"SnareDrum Two NBP", "Regular", 60.0}, ng::color::White });
            else
                iTextEdit.set_default_style(
                    ng::text_edit::character_style{
                        ng::optional_font{},
                        ng::gradient{
                            ng::color{ aNumber & 1 ? 64 : 0, aNumber & 2 ? 64 : 0, aNumber & 4 ? 64 : 0 }.lighter(0x40),
                            ng::color{ aNumber & 1 ? 64 : 0, aNumber & 2 ? 64 : 0, aNumber & 4 ? 64 : 0 }.lighter(0xC0),
                            ng::gradient_direction::Horizontal},
                        ng::color_or_gradient{} });
        });
    }
private:
    ng::text_edit& iTextEdit;
};

ng::game::i_ecs& create_game(ng::i_layout& aLayout);

void signal_handler(int signal)
{
    if (signal == SIGABRT) 
    {
        ng::service<ng::debug::logger>() << "SIGABRT received" << ng::endl;
    }
    else 
    {
        ng::service<ng::debug::logger>() << "Unexpected signal " << signal << " received" << ng::endl;
    }
    std::_Exit(EXIT_FAILURE);
}

inline ng::quad line_to_quad(const ng::vec3& aStart, const ng::vec3& aEnd, double aLineWidth)
{
    auto const vecLine = aEnd - aStart;
    auto const length = vecLine.magnitude();
    auto const halfWidth = aLineWidth / 2.0;
    auto const v1 = ng::vec3{ -halfWidth, -halfWidth, 0.0 };
    auto const v2 = ng::vec3{ -halfWidth, halfWidth, 0.0 };
    auto const v3 = ng::vec3{ length + halfWidth, halfWidth, 0.0 };
    auto const v4 = ng::vec3{ length + halfWidth, -halfWidth, 0.0 };
    auto const r = ng::rotation_matrix(ng::vec3{ 1.0, 0.0, 0.0 }, vecLine);
    return ng::quad{ aStart + r * v1, aStart + r * v2, aStart + r * v3, aStart + r * v4 };
}

int main(int argc, char* argv[])
{
    auto previous_handler = std::signal(SIGABRT, signal_handler);
    if (previous_handler == SIG_ERR) 
    {
        ng::service<ng::debug::logger>() << "SIGABRT handler setup failed" << ng::endl;
        return EXIT_FAILURE;
    }

    /* Yes this is an 800 line (and counting) function and whilst in general such long functions are
    egregious this function is a special case: it is test code which mostly just creates widgets. 
    Most of this code is about to disappear into code auto-generated by the neoGFX resource compiler! */

    test::main_app app{ argc, argv, "neoGFX Test App (Pre-Release)" };

    try
    {
        app.register_style(ng::style("Keypad"));
        app.change_style("Keypad");
        app.current_style().palette().set_color(ng::color_role::Theme, ng::color::Black);
        app.change_style("Dark");

        test::main_window window{ app };

        auto ds = window.textEdit.default_style();
        ds.paragraph().set_padding(ng::padding{ 0, 4.0_dip });
        window.textEdit.set_default_style(ds);

        window.textEdit.ObjectAcceptable([&](ng::i_drag_drop_object const& aObject, ng::drop_operation& aAcceptableAs)
        {
            if (aObject.ddo_type() == ng::i_drag_drop_file_list::otid())
                aAcceptableAs = ng::drop_operation::Copy;
        });

        window.textEdit.ObjectDropped([&](ng::i_drag_drop_object const& aObject)
        {
            auto const& files = static_cast<ng::i_drag_drop_file_list const&>(aObject);
            std::ifstream file{ files.file_paths()[0].to_std_string() };
            window.textEdit.set_plain_text(ng::string{ std::istreambuf_iterator<char>{file}, {} });
        });

        app.actionArcadeMode.checked([&]() { neolib::service<neolib::i_power>().enable_turbo_mode(); });
        app.actionArcadeMode.unchecked([&]() { neolib::service<neolib::i_power>().disable_turbo_mode(); });

        neolib::service<neolib::i_power>().turbo_mode_entered([&]() { app.actionArcadeMode.check(); });
        neolib::service<neolib::i_power>().turbo_mode_left([&]() { app.actionArcadeMode.uncheck(); });

        app.actionFileOpen.triggered([&]()
        {
            auto textFile = ng::open_file_dialog(window, ng::file_dialog_spec{ "Edit Text File", {}, { "*.txt" }, "Text Files" });
            if (textFile)
            {
                std::ifstream file{ (*textFile)[0] };
                window.textEdit.set_plain_text(ng::string{ std::istreambuf_iterator<char>{file}, {} });
            }
        });

        app.actionShowStatusBar.set_checked(true);

        app.actionShowStatusBar.checked([&]()
        {
            window.statusBar.show();
        });
        
        app.actionShowStatusBar.unchecked([&]()
        {
            window.statusBar.hide();
        });

        app.add_action("Goldenrod Style"_s).set_shortcut("Ctrl+Alt+Shift+G").triggered([]()
        {
            ng::service<ng::i_app>().change_style("Keypad").palette().set_color(ng::color_role::Theme, ng::color::LightGoldenrod);
        });

        app.actionContacts.triggered([]()
        {
            ng::service<ng::i_app>().change_style("Keypad").palette().set_color(ng::color_role::Theme, ng::color::White);
        });
        
        neolib::callback_timer ct{ app, [&app](neolib::callback_timer& aTimer)
        {
            aTimer.again();
            if (ng::service<ng::i_clipboard>().sink_active())
            {
                auto& sink = ng::service<ng::i_clipboard>().active_sink();
                if (sink.can_paste())
                {
                    app.actionPasteAndGo.enable();
                }
                else
                {
                    app.actionPasteAndGo.disable();
                }
            }
        }, std::chrono::milliseconds{ 100 } };

        app.actionPasteAndGo.triggered([&app]()
        {
            ng::service<ng::i_clipboard>().paste();
        });

        neolib::random menuPrng{ 0 };
        for (int i = 1; i <= 5; ++i)
        {
            auto& sm = window.menuFavourites.add_sub_menu(ng::string{ "More_" + boost::lexical_cast<std::string>(i) });
            for (int j = 1; j <= 5; ++j)
            {
                auto& sm2 = sm.add_sub_menu(ng::string{ "More_" + boost::lexical_cast<std::string>(i) + "_" + boost::lexical_cast<std::string>(j) });
                int n = menuPrng(100);
                for (int k = 1; k < n; ++k)
                {
                    auto& action = app.add_action(ng::string{ "More_" + boost::lexical_cast<std::string>(i) + "_" + boost::lexical_cast<std::string>(j) + "_" + boost::lexical_cast<std::string>(k) }, ":/closed/resources/caw_toolbar.zip#favourite.png"_s);
                    sm2.add_action(action);
                    action.triggered([&]()
                    {
                        window.textEdit.set_text(action.text());
                    });
                }
            }
        }

        app.actionColorDialog.triggered([&window]()
        {
            ng::color_dialog cd(window);
            cd.exec();
        });

        app.actionManagePlugins.triggered([&]()
            {
                ng::service<ng::i_rendering_engine>().enable_frame_rate_limiter(!ng::service<ng::i_rendering_engine>().frame_rate_limited());
            });

        app.actionNextTab.triggered([&]() { window.tabPages.select_next_tab(); });
        app.actionPreviousTab.triggered([&]() { window.tabPages.select_previous_tab(); });

        auto display_controller_info = [&](ng::i_game_controller const& aController, bool aConnected)
        {
            std::ostringstream oss;
            oss << aController.product_name() << " {" << aController.product_id() << "} " << (aConnected ? " connected." : " disconnected.") << std::endl;
            window.textEdit.append_text(ng::string{ oss.str() }, true);
        };
        for (auto const& controller : ng::service<ng::i_game_controllers>().controllers())
            display_controller_info(*controller, true);
        ng::service<ng::i_game_controllers>().controller_connected([&](ng::i_game_controller& aController) { display_controller_info(aController, true); } );
        ng::service<ng::i_game_controllers>().controller_disconnected([&](ng::i_game_controller& aController) { display_controller_info(aController, false); });
        window.gradientWidget.gradient_changed([&]()
        {
            auto s = window.textEdit.default_style();
            auto s2 = window.textEditEditor.default_style();
            if (!window.editOutline.is_checked())
            {
                s.character().set_glyph_color(window.gradientWidget.gradient());
                s2.character().set_glyph_color(window.gradientWidget.gradient());
                if (window.editGlow.is_checked())
                {
                    s.character().set_text_effect(ng::text_effect{ ng::text_effect_type::Glow, window.gradientWidget.gradient(), window.effectWidthSlider.value(), {}, window.effectAux1Slider.value() });
                    s2.character().set_text_effect(ng::text_effect{ ng::text_effect_type::Glow, window.gradientWidget.gradient(), window.effectWidthSlider.value(), {}, window.effectAux1Slider.value() });
                }
                else if (window.editShadow.is_checked())
                {
                    s.character().set_text_effect(ng::text_effect{ ng::text_effect_type::Shadow, window.gradientWidget.gradient(), window.effectWidthSlider.value(), {}, window.effectAux1Slider.value() });
                    s2.character().set_text_effect(ng::text_effect{ ng::text_effect_type::Shadow, window.gradientWidget.gradient(), window.effectWidthSlider.value(), {}, window.effectAux1Slider.value() });
                }
            }
            else
            {
                s.character().set_text_effect(ng::text_effect{ ng::text_effect_type::Outline, window.gradientWidget.gradient() });
                s2.character().set_text_effect(ng::text_effect{ ng::text_effect_type::Outline, window.gradientWidget.gradient() });
            }
            s2.character().set_paper_color(ng::color_or_gradient{});
            window.textEdit.set_default_style(s);
            window.textEditEditor.set_default_style(s2);
        });

        window.button1.clicked([&window]()
        {
            if ((window.tabPages.tab_container_style() & ng::tab_container_style::TabAlignmentMask) == ng::tab_container_style::TabAlignmentTop)
                window.tabPages.set_tab_container_style(ng::tab_container_style::TabAlignmentBottom);
            else if ((window.tabPages.tab_container_style() & ng::tab_container_style::TabAlignmentMask) == ng::tab_container_style::TabAlignmentBottom)
                window.tabPages.set_tab_container_style(ng::tab_container_style::TabAlignmentLeft);
            else if ((window.tabPages.tab_container_style() & ng::tab_container_style::TabAlignmentMask) == ng::tab_container_style::TabAlignmentLeft)
                window.tabPages.set_tab_container_style(ng::tab_container_style::TabAlignmentRight);
            else if ((window.tabPages.tab_container_style() & ng::tab_container_style::TabAlignmentMask) == ng::tab_container_style::TabAlignmentRight)
                window.tabPages.set_tab_container_style(ng::tab_container_style::TabAlignmentTop);
        });
        window.buttonChina.clicked([&window]() 
        { 
            window.set_title_text(u8"一只敏捷的狐狸跳过一只懒狗。"_t); 
            if (window.buttonChina.has_maximum_size())
                window.buttonChina.set_maximum_size({});
            else 
                window.buttonChina.set_maximum_size(ng::size{ 128_dip, 64_dip });
        });

        for (int32_t i = 1; i <= 100; ++i)
            window.dropList3.model().insert_item(window.dropList3.model().end(), "Example_"_t + boost::lexical_cast<std::string>(i));
        neolib::random prng;
        for (int32_t i = 1; i <= 250; ++i)
        {
            std::string randomString;
            for (uint32_t j = prng(12); j-- > 0;)
                randomString += static_cast<char>('A' + prng('z' - 'A'));
            window.dropList4.model().insert_item(window.dropList4.model().end(), randomString);
        }
        window.toggleEditable.Toggled([&]()
        {
            window.dropList.set_editable(!window.dropList.editable());
            window.dropList2.set_editable(!window.dropList2.editable());
            window.dropList3.set_editable(!window.dropList3.editable());
            window.dropList4.set_editable(!window.dropList4.editable());
        });
        window.buttonGenerateUuid.clicked([&]() { window.textEdit.set_text(to_string(neolib::generate_uuid())); });
        window.dropList.SelectionChanged([&](const ng::optional_item_model_index& aIndex) { window.textEdit.set_text(ng::string{ aIndex != std::nullopt ? window.dropList.model().cell_data(*aIndex).to_string() : std::string{} }); });
        window.dropList2.SelectionChanged([&](const ng::optional_item_model_index& aIndex) { window.textEdit.set_text(ng::string{ aIndex != std::nullopt ? window.dropList2.model().cell_data(*aIndex).to_string() : std::string{} }); });
        window.dropList3.SelectionChanged([&](const ng::optional_item_model_index& aIndex) { window.textEdit.set_text(ng::string{ aIndex != std::nullopt ? window.dropList3.model().cell_data(*aIndex).to_string() : std::string{} }); });
        window.dropList4.SelectionChanged([&](const ng::optional_item_model_index& aIndex) { window.textEdit.set_text(ng::string{ aIndex != std::nullopt ? window.dropList4.model().cell_data(*aIndex).to_string() : std::string{} }); });
        window.textField1.input_box().TextChanged([&window]()
        {
            window.button1.set_text(window.textField1.input_box().text());
        });
        window.spinBox1.ValueChanged([&window]() { window.slider1.set_value(window.spinBox1.value()); });
        bool colorCycle = false;
        window.button6.clicked([&colorCycle]() { colorCycle = !colorCycle; });
        window.buttonArcadeMode.clicked([&app]() { app.actionArcadeMode.toggle(); });
        window.button7.clicked([&app]() { app.actionMute.toggle(); });
        window.button8.clicked([&app]() { if (app.actionContacts.is_enabled()) app.actionContacts.disable(); else app.actionContacts.enable(); });
        prng.seed(3);
//        auto transitionPrng = prng;
//        std::vector<ng::transition_id> transitions;
        for (uint32_t i = 0; i < 10; ++i)
        {
            auto& button = window.layout3.emplace<ng::push_button>(std::string(1, 'A' + i));
            ng::color randomColor = ng::color{ prng(255), prng(255), prng(255) };
            button.set_base_color(randomColor);
            button.clicked([&, randomColor]() { window.textEdit.set_palette_color(ng::color_role::Background, randomColor.same_lightness_as(app.current_style().palette().color(ng::color_role::Background))); });
//            transitions.push_back(ng::service<ng::i_animator>().add_transition(button.Position, ng::easing::OutBounce, transitionPrng.get(1.0, 2.0), false));
        }
//        ng::event<> startAnimation;
//        startAnimation([&window, &transitions, &transitionPrng]()
//        {
//            for (auto t : transitions)
//                ng::service<ng::i_animator>().transition(t).reset(true, true);
//            for (auto i = 0u; i < window.layout3.count(); ++i)
//            {
//                auto& button = window.layout3.get_widget_at(i);
//                auto finalPosition = button.position();
//                button.set_position(ng::point{ finalPosition.x, finalPosition.y - transitionPrng.get(600.0, 800.0) }.ceil());
//                button.set_position(finalPosition);
//            }
//        });
//        window.mainWindow.Window([&startAnimation](const ng::window_event& aEvent)
//        { 
//            if (aEvent.type() == ng::window_event_type::Resized)
//                startAnimation.async_trigger(); 
//        });
        auto showHideTabs = [&window]()
        {
            if (window.checkTriState.is_checked())
                window.tabPages.hide_tab(8);
            else
                window.tabPages.show_tab(8);
            if (window.checkTriState.is_indeterminate())
                window.tabPages.hide_tab(9);
            else
                window.tabPages.show_tab(9);
        };
        window.checkTriState.checked([&window, showHideTabs]()
        {
            static uint32_t n;
            if ((n++)%2 == 1)
                window.checkTriState.set_indeterminate();
            showHideTabs();
        });
        window.checkTriState.Unchecked([&window, showHideTabs]()
        {
            showHideTabs();
        });
        window.checkWordWrap.check();
        window.checkWordWrap.checked([&]()
        {
            window.textEdit.set_word_wrap(true);
        });
        window.checkWordWrap.Unchecked([&]()
        {
            window.textEdit.set_word_wrap(false);
        });
        window.checkPassword.checked([&]()
        {
            window.textField2.hint().set_text("Enter password"_s);
            window.textField2.input_box().set_password(true);
        });
        window.checkPassword.Unchecked([&]()
        {
            window.textField2.hint().set_text("Enter text"_s);
            window.textField2.input_box().set_password(false);
        });
        window.checkGroupBoxCheckable.checked([&window]()
        {
            window.groupBox.set_checkable(true);
            window.labelFPS.show();
        });
        window.checkGroupBoxCheckable.Unchecked([&window]()
        {
            window.groupBox.set_checkable(false);
            window.labelFPS.hide();
        });
        window.labelFPS.hide();
        std::optional<ng::sink> sink1;
        window.checkColumns.checked([&]()
        {
            window.checkPassword.disable();
            window.textEdit.set_columns(3);
            sink1.emplace();
            *sink1 += window.gradientWidget.GradientChanged([&]()
            {
                auto cs = window.textEdit.column(2);
                cs.set_style(ng::text_edit::character_style{ ng::optional_font{}, ng::color_or_gradient{}, ng::color_or_gradient{}, ng::text_effect{ ng::text_effect_type::Outline, ng::color::White } });
                window.textEdit.set_column(2, cs);
            });
        });
        window.checkColumns.Unchecked([&]()
        {
            window.checkPassword.enable();
            window.textEdit.remove_columns();
            sink1 = std::nullopt;
        });
        window.checkKerning.Toggled([&app, &window]()
        {
            auto fi = app.current_style().font_info();
            if (window.checkKerning.is_checked())
            {
                fi.enable_kerning();
                auto newTextEditFont = window.textEdit.font();
                newTextEditFont.enable_kerning();
                window.textEdit.set_font(newTextEditFont);
            }
            else
            {
                fi.disable_kerning();
                auto newTextEditFont = window.textEdit.font();
                newTextEditFont.disable_kerning();
                window.textEdit.set_font(newTextEditFont);
            }
            app.current_style().set_font_info(fi);
        });
        window.checkSubpixel.Toggled([&app, &window]()
        {
            if (window.checkSubpixel.is_checked())
                ng::service<ng::i_rendering_engine>().subpixel_rendering_on();
            else
                ng::service<ng::i_rendering_engine>().subpixel_rendering_off();
        });
        window.editNormal.checked([&]()
        {
            auto s = window.textEdit.default_style();
            s.character().set_text_effect(ng::optional_text_effect{});
            window.textEdit.set_default_style(s);
        });
        window.editOutline.checked([&]()
        {
            window.effectWidthSlider.set_value(1);
            auto s = window.textEdit.default_style();
            s.character().set_text_color(app.current_style().palette().color(ng::color_role::Text).light() ? ng::color::Black : ng::color::White);
            s.character().set_text_effect(ng::text_effect{ ng::text_effect_type::Outline, app.current_style().palette().color(ng::color_role::Text) });
            window.textEdit.set_default_style(s);
            auto s2 = window.textEditEditor.default_style();
            s2.character().set_text_effect(ng::text_effect{ ng::text_effect_type::Outline, ng::color::White });
            window.textEditEditor.set_default_style(s2);
        });
        window.editGlow.checked([&]()
        {
            window.effectWidthSlider.set_value(5);
            window.effectAux1Slider.set_value(1);
            auto s = window.textEdit.default_style();
            s.character().set_text_effect(ng::text_effect{ ng::text_effect_type::Glow, ng::color::Orange, window.effectWidthSlider.value(), {}, window.effectAux1Slider.value() });
            window.textEdit.set_default_style(s);
            s = window.textEditEditor.default_style();
            s.character().set_text_effect(ng::text_effect{ ng::text_effect_type::Glow, ng::color::Orange, window.effectWidthSlider.value(), {}, window.effectAux1Slider.value() });
            window.textEditEditor.set_default_style(s);
        });
        window.effectWidthSlider.ValueChanged([&]()
        {
            auto s = window.textEdit.default_style();
            auto& textEffect = s.character().text_effect();
            if (textEffect == std::nullopt)
                return;
            s.character().set_text_effect(ng::text_effect{ window.editGlow.is_checked() ? ng::text_effect_type::Glow : window.editShadow.is_checked() ? ng::text_effect_type::Shadow : ng::text_effect_type::Outline, textEffect->color(), window.effectWidthSlider.value(), {}, textEffect->aux1() });
            window.textEdit.set_default_style(s);
            std::ostringstream oss;
            oss << window.effectWidthSlider.value() << std::endl << window.effectAux1Slider.value() << std::endl;
            window.textEditSmall.set_text(ng::string{ oss.str() });
        });
        window.effectAux1Slider.ValueChanged([&]()
        {
            window.editGlow.check();
            auto s = window.textEdit.default_style();
            auto& textEffect = s.character().text_effect();
            if (textEffect == std::nullopt)
                return;
            s.character().set_text_effect(ng::text_effect{ ng::text_effect_type::Glow, textEffect->color(), textEffect->width(), {}, window.effectAux1Slider.value() });
            window.textEdit.set_default_style(s);
            std::ostringstream oss;
            oss << window.effectWidthSlider.value() << std::endl << window.effectAux1Slider.value() << std::endl;
            window.textEditSmall.set_text(ng::string{ oss.str() });
        });
        window.editShadow.checked([&]()
        {
            auto s = window.textEdit.default_style();
            s.character().set_text_effect(ng::text_effect{ ng::text_effect_type::Shadow, {} });
            window.textEdit.set_default_style(s);
        });
        window.radioSliderFont.checked([&window, &app]()
        {
            app.current_style().set_font_info(app.current_style().font_info().with_size(window.slider1.normalized_value() * 18.0 + 4));
        });
        auto update_theme_color = [&window]()
        {
            auto themeColor = ng::service<ng::i_app>().current_style().palette().color(ng::color_role::Theme).to_hsv();
            themeColor.set_hue(window.slider1.normalized_value() * 360.0);
            ng::service<ng::i_app>().current_style().palette().set_color(ng::color_role::Theme, themeColor.to_rgb<ng::color>());
        };
        window.slider1.ValueChanged([update_theme_color, &window, &app]()
        {
            window.spinBox1.set_value(window.slider1.value());
            if (window.radioSliderFont.is_checked())
                app.current_style().set_font_info(app.current_style().font_info().with_size(window.slider1.normalized_value() * 18.0 + 4));
            else if (window.radioThemeColor.is_checked())
                update_theme_color();
        });
        window.slider1.set_normalized_value((app.current_style().font_info().size() - 4) / 18.0);
        window.radioThemeColor.checked([update_theme_color, &window, &app]()
        {
            window.slider1.set_normalized_value(ng::service<ng::i_app>().current_style().palette().color(ng::color_role::Theme).to_hsv().hue() / 360.0);
            update_theme_color();
        });

        app.actionGameControllerCalibration.triggered([&window]()
        {
            ng::game_controller_dialog gameControllerDialog(window);
            gameControllerDialog.exec();
        });

        window.themeColor.clicked([&window]()
        {
            static std::optional<ng::color_dialog::custom_color_list> sCustomColors;
            if (sCustomColors == std::nullopt)
            {
                sCustomColors = ng::color_dialog::custom_color_list{};
                std::fill(sCustomColors->begin(), sCustomColors->end(), ng::color::White);
            }
            auto oldColor = ng::service<ng::i_app>().current_style().palette().color(ng::color_role::Theme);
            ng::color_dialog colorPicker(window, ng::service<ng::i_app>().current_style().palette().color(ng::color_role::Theme));
            colorPicker.set_custom_colors(*sCustomColors);
            colorPicker.SelectionChanged([&]()
            {
                ng::service<ng::i_app>().current_style().palette().set_color(ng::color_role::Theme, colorPicker.selected_color());
            });
            if (colorPicker.exec() == ng::dialog_result::Accepted)
                ng::service<ng::i_app>().current_style().palette().set_color(ng::color_role::Theme, colorPicker.selected_color());
            else
                ng::service<ng::i_app>().current_style().palette().set_color(ng::color_role::Theme, oldColor);
            *sCustomColors = colorPicker.custom_colors();
        });

        window.themeFont.clicked([&]()
        {
            bool const changeEditFont = window.textEdit.has_focus();
            ng::font_dialog fontPicker(window, changeEditFont ? window.textEdit.font() : ng::service<ng::i_app>().current_style().font_info());
            if (fontPicker.exec() == ng::dialog_result::Accepted)
            {
                if (changeEditFont)
                {
                    window.textEdit.set_font(fontPicker.selected_font());
                    window.textEditEditor.set_font(fontPicker.selected_font());
                }
                else
                    ng::service<ng::i_app>().current_style().set_font_info(fontPicker.selected_font());
            }
        });

        window.editColor.clicked([&]()
        {
            static std::optional<ng::color_dialog::custom_color_list> sCustomColors;
            static ng::color sInk = ng::service<ng::i_app>().current_style().palette().color(ng::color_role::Text);
            ng::color_dialog colorPicker(window, sInk);
            if (sCustomColors != std::nullopt)
                colorPicker.set_custom_colors(*sCustomColors);
            if (colorPicker.exec() == ng::dialog_result::Accepted)
            {
                sInk = colorPicker.selected_color();
                auto s = window.textEdit.default_style();
                s.character().set_glyph_color(sInk);
                window.textEdit.set_default_style(s);
                auto s2 = window.textField1.input_box().default_style();
                s2.character().set_glyph_color(sInk);
                window.textField1.input_box().set_default_style(s2);
                window.textField2.input_box().set_default_style(s2);
            }
            sCustomColors = colorPicker.custom_colors();
        });

        ng::vertical_spacer spacer1{ window.layout4 };
        window.button9.clicked([&app]()
        {
            if (app.current_style().name() == "Light")
                app.change_style("Dark");
            else
                app.change_style("Light");
        });
        for (uint32_t row = 0u; row < 3u; ++row)
            for (uint32_t col = 0u; col < 3u; ++col)
                window.keypad.add_item_at_position(row, col, ng::make_ref<keypad_button>(window.textEdit, row * 3u + col + 1u));
        window.keypad.add_item_at_position(3u, 1u, ng::make_ref<keypad_button>(window.textEdit, 0u));
        window.keypad.add_span(3u, 1u, 1u, 2u);

        neolib::callback_timer animation(app, [&](neolib::callback_timer& aTimer)
        {
            if (!window.has_native_surface()) // todo: shouldn't need this check
                return;

            aTimer.again();

            std::ostringstream oss;
            oss << window.fps() << "/" << window.potential_fps() << " FPS/PFPS";
            window.labelFPS.set_text(ng::string{ oss.str() });

            if (colorCycle)
            {
                const double PI = 2.0 * std::acos(0.0);
                double brightness = ::sin((app.program_elapsed_ms() / 16 % 360) * (PI / 180.0)) / 2.0 + 0.5;
                neolib::random prng{ app.program_elapsed_ms() / 5000 };
                ng::color randomColor = ng::color{ prng(255), prng(255), prng(255) };
                randomColor = randomColor.to_hsv().with_brightness(brightness).to_rgb<ng::color>();
                window.button6.set_base_color(randomColor);
            }
        }, std::chrono::milliseconds{ 16 });

        app.action_file_new().triggered([&]()
        {
        });

        uint32_t standardButtons = 0;
        uint32_t standardButton = 1;
        while (standardButton != 0)
        {
            auto bd = ng::dialog_button_box::standard_button_details(static_cast<ng::standard_button>(standardButton));
            if (bd.first != ng::button_role::Invalid)
            {
                auto& button = window.groupMessageBoxButtons.item_layout().emplace<ng::check_box>(bd.second);
                button.checked([&standardButtons, standardButton]() { standardButtons |= standardButton; });
                button.Unchecked([&standardButtons, standardButton]() { standardButtons &= ~standardButton; });
            }
            standardButton <<= 1;
        }
        window.buttonOpenMessageBox.clicked([&]()
        {
            ng::standard_button result;
            if (window.radioMessageBoxIconInformation.is_checked())
                result = ng::message_box::information(window, window.lineEditMessageBoxTitle.text(), window.textEditMessageBoxText.text(), window.textEditMessageBoxDetailedText.text(), static_cast<ng::standard_button>(standardButtons));
            else if (window.radioMessageBoxIconQuestion.is_checked())
                result = ng::message_box::question(window, window.lineEditMessageBoxTitle.text(), window.textEditMessageBoxText.text(), window.textEditMessageBoxDetailedText.text(), static_cast<ng::standard_button>(standardButtons));
            else if (window.radioMessageBoxIconWarning.is_checked())
                result = ng::message_box::warning(window, window.lineEditMessageBoxTitle.text(), window.textEditMessageBoxText.text(), window.textEditMessageBoxDetailedText.text(), static_cast<ng::standard_button>(standardButtons));
            else if (window.radioMessageBoxIconStop.is_checked())
                result = ng::message_box::stop(window, window.lineEditMessageBoxTitle.text(), window.textEditMessageBoxText.text(), window.textEditMessageBoxDetailedText.text(), static_cast<ng::standard_button>(standardButtons));
            else if (window.radioMessageBoxIconError.is_checked())
                result = ng::message_box::error(window, window.lineEditMessageBoxTitle.text(), window.textEditMessageBoxText.text(), window.textEditMessageBoxDetailedText.text(), static_cast<ng::standard_button>(standardButtons));
            else if (window.radioMessageBoxIconCritical.is_checked())
                result = ng::message_box::critical(window, window.lineEditMessageBoxTitle.text(), window.textEditMessageBoxText.text(), window.textEditMessageBoxDetailedText.text(), static_cast<ng::standard_button>(standardButtons));
            window.labelMessageBoxResult.set_text("Result = "_s + ng::dialog_button_box::standard_button_details(result).second);
        });

        // Item Views

        ng::service<ng::i_window_manager>().save_mouse_cursor();
        ng::service<ng::i_window_manager>().set_mouse_cursor(ng::mouse_system_cursor::Wait);

        ng::table_view& tableView1 = window.tableView1;
        ng::table_view& tableView2 = window.tableView2;

        tableView1.set_minimum_size(ng::size(128, 128));
        tableView2.set_minimum_size(ng::size(128, 128));
        window.button10.clicked([&tableView1, &tableView2]()
        {
            if (tableView1.column_header().visible())
                tableView1.column_header().hide();
            else
                tableView1.column_header().show();
            if (tableView2.column_header().visible())
                tableView2.column_header().hide();
            else
                tableView2.column_header().show();
        });

        my_item_model itemModel;
        #ifdef NDEBUG
        itemModel.reserve(500);
        #else
        itemModel.reserve(100);
        #endif
        ng::event_processing_context epc{ app };
        for (uint32_t row = 0; row < itemModel.capacity(); ++row)
        {
            #ifdef NDEBUG
            if (row % 100 == 0)
                app.process_events(epc);
            #else
            if (row % 10 == 0)
                app.process_events(epc);
            #endif
            neolib::random prng;
            for (uint32_t col = 0; col < 9; ++col)
            {
                if (col == 0)
                    itemModel.insert_item(ng::item_model_index(row), row + 1);
                else if (col == 1)
                    itemModel.insert_cell_data(ng::item_model_index(row, col), row + 1);
                else if (col != 7)
                {
                    std::string randomString;
                    for (uint32_t j = prng(12); j-- > 0;)
                        randomString += static_cast<char>('A' + prng('z' - 'A'));
                    itemModel.insert_cell_data(ng::item_model_index(row, col), randomString);
                }
            }
        } 

        itemModel.set_column_min_value(0, 0u);
        itemModel.set_column_max_value(0, 9999u);
        itemModel.set_column_min_value(1, 0u);
        itemModel.set_column_max_value(1, 9999u);
        itemModel.set_column_step_value(1, 1u);
        tableView1.set_model(itemModel);
        my_item_presentation_model ipm1{ itemModel, true };
        tableView1.set_presentation_model(ipm1);
        ipm1.set_column_editable_when_focused(4);
        ipm1.set_column_editable_when_focused(5);
        ipm1.set_column_editable_when_focused(6);
        ipm1.set_column_editable_when_focused(7);
        ipm1.set_column_editable_when_focused(8);
        tableView2.set_model(itemModel);
        my_item_presentation_model ipm2{ itemModel, true };
        ipm2.set_column_editable_when_focused(0);
        ipm2.set_column_editable_when_focused(1);
        ipm2.set_column_editable_when_focused(2);
        ipm2.set_column_editable_when_focused(3);
        tableView2.set_presentation_model(ipm2);
        tableView2.column_header().set_expand_last_column(true);
        tableView1.Keyboard([&tableView1](const ng::keyboard_event& ke)
        {
            if (ke.type() == ng::keyboard_event_type::KeyPressed && ke.scan_code() == ng::ScanCode_DELETE && tableView1.model().rows() > 0 && tableView1.selection_model().has_current_index())
                tableView1.model().erase(tableView1.model().begin() + tableView1.presentation_model().to_item_model_index(tableView1.selection_model().current_index()).row());
        });

        ipm1.ItemChecked([&](const ng::item_presentation_model_index& aIndex) { ipm2.check(ipm2.from_item_model_index(ipm1.to_item_model_index(aIndex))); });
        ipm1.ItemUnchecked([&](const ng::item_presentation_model_index& aIndex) { ipm2.uncheck(ipm2.from_item_model_index(ipm1.to_item_model_index(aIndex))); });

        auto update_column5_heading = [&](bool aReadOnly, bool aUnselectable)
        {
            std::string heading;
            if (aReadOnly)
                heading = "Read only";
            if (aUnselectable)
            {
                if (!heading.empty())
                    heading += "/\n";
                heading += "Unselectable";
            }
            if (heading.empty())
                heading = "Five";
            else
                heading = "*****" + heading + "*****";
            itemModel.set_column_name(5, heading);
        };

        ng::item_tree_model treeModel;
        auto entities = treeModel.insert_item(treeModel.send(), "Entity");
        auto components = treeModel.insert_item(treeModel.send(), "Component");
        auto systems = treeModel.insert_item(treeModel.send(), "System");
        auto animals = treeModel.append_item(entities, "Animals");
        treeModel.append_item(animals, "Kitten");
        treeModel.append_item(animals, "Hedgehog");
        auto dolphins = treeModel.append_item(animals, "Dolphins");
        treeModel.append_item(dolphins, "T. truncatus");
        treeModel.append_item(dolphins, "O. orca");
        auto people = treeModel.append_item(entities, "People");
        auto athletes = treeModel.append_item(people, "Athletes (London 2012 Gold Medalists, Running)");
        treeModel.append_item(athletes, "Usain Bolt");
        treeModel.append_item(athletes, "Usain Bolt");
        treeModel.append_item(athletes, "Kirani James");
        treeModel.append_item(athletes, "David Rudisha");
        treeModel.append_item(athletes, "Taoufik Makhloufi");
        treeModel.append_item(athletes, "Mo Farah");
        treeModel.append_item(athletes, "Mo Farah");
        treeModel.append_item(athletes, "Shelly-Ann Fraser-Pryce");
        treeModel.append_item(athletes, "Allyson Felix");
        treeModel.append_item(athletes, "Sanya Richards-Ross");
        treeModel.append_item(athletes, "Caster Semenya");
        treeModel.append_item(athletes, "Maryam Yusuf Jamal");
        treeModel.append_item(athletes, "Meseret Defar Tola");
        treeModel.append_item(athletes, "Tirunesh Dibaba Kenene");

        my_item_tree_presentation_model itpm1{ treeModel };
        my_item_tree_presentation_model itpm2{ treeModel, true };

        window.treeView1.set_model(treeModel);
        window.treeView1.set_presentation_model(itpm1);
        window.treeView2.set_model(treeModel);
        window.treeView2.set_presentation_model(itpm2);

        window.checkTableViewsCheckable.checked([&]() { ipm1.set_column_checkable(0, true); ipm2.set_column_checkable(0, true); ipm1.set_column_checkable(1, true); ipm2.set_column_checkable(1, true); itpm1.set_column_checkable(0, true); itpm2.set_column_checkable(0, true); });
        window.checkTableViewsCheckable.unchecked([&]() { ipm1.set_column_checkable(0, false); ipm2.set_column_checkable(0, false); ipm1.set_column_checkable(1, false); ipm2.set_column_checkable(1, false); itpm1.set_column_checkable(0, false); itpm2.set_column_checkable(0, false); });
        window.checkTableViewsCheckableTriState.checked([&]() { ipm1.set_column_tri_state_checkable(0, true); ipm1.set_column_tri_state_checkable(1, true); ipm2.set_column_tri_state_checkable(0, true); ipm2.set_column_tri_state_checkable(1, true); itpm1.set_column_tri_state_checkable(0, true); itpm2.set_column_tri_state_checkable(0, true); });
        window.checkTableViewsCheckableTriState.unchecked([&]() { ipm1.set_column_tri_state_checkable(0, false); ipm1.set_column_tri_state_checkable(1, false); ipm2.set_column_tri_state_checkable(0, false); ipm2.set_column_tri_state_checkable(1, false); itpm1.set_column_tri_state_checkable(0, false); itpm2.set_column_tri_state_checkable(0, false); });
        window.checkColumn5ReadOnly.checked([&]() { ipm1.set_column_read_only(5, true); ipm2.set_column_read_only(5, true); update_column5_heading(true, window.checkColumn5Unselectable.is_checked()); });
        window.checkColumn5ReadOnly.unchecked([&]() { ipm1.set_column_read_only(5, false); ipm2.set_column_read_only(5, false); update_column5_heading(false, window.checkColumn5Unselectable.is_checked()); });
        window.checkColumn5Unselectable.checked([&]() { ipm1.set_column_selectable(5, false); ipm2.set_column_selectable(5, false); update_column5_heading(window.checkColumn5ReadOnly.is_checked(), true); });
        window.checkColumn5Unselectable.unchecked([&]() { ipm1.set_column_selectable(5, true); ipm2.set_column_selectable(5, true); update_column5_heading(window.checkColumn5ReadOnly.is_checked(), false); });

        window.checkUpperTableViewImages.checked([&] { for (uint32_t c = 0u; c <= 6u; c += 2u) ipm1.set_column_image_size(c, ng::size{ 16_dip }); itpm1.set_column_image_size(0, ng::size{ 16_dip }); });
        window.checkUpperTableViewImages.unchecked([&] { for (uint32_t c = 0u; c <= 6u; c += 2u) ipm1.set_column_image_size(c, {}); itpm1.set_column_image_size(0, {}); });
        window.radioUpperTableViewMonochrome.checked([&] { ipm1.set_color_role({}); window.tableView1.update(); itpm1.set_color_role({}); window.treeView1.update(); });
        window.radioUpperTableViewColoredText.checked([&] { ipm1.set_color_role(ng::color_role::Text); window.tableView1.update(); itpm1.set_color_role(ng::color_role::Text); window.treeView1.update(); });
        window.radioUpperTableViewColoredCells.checked([&] { ipm1.set_color_role(ng::color_role::Background); window.tableView1.update(); itpm1.set_color_role(ng::color_role::Background); window.treeView1.update(); });
        window.checkLowerTableViewImages.checked([&] { for (uint32_t c = 0u; c <= 6u; c += 2u) ipm2.set_column_image_size(c, ng::size{ 16_dip }); itpm2.set_column_image_size(0, ng::size{ 16_dip }); });
        window.checkLowerTableViewImages.unchecked([&] { for (uint32_t c = 0u; c <= 6u; c += 2u) ipm2.set_column_image_size(c, {}); itpm2.set_column_image_size(0, {}); });
        window.radioLowerTableViewMonochrome.checked([&] { ipm2.set_color_role({}); window.tableView2.update(); itpm2.set_color_role({}); window.treeView2.update(); });
        window.radioLowerTableViewColoredText.checked([&] { ipm2.set_color_role(ng::color_role::Text); window.tableView2.update(); itpm2.set_color_role(ng::color_role::Text); window.treeView2.update(); });
        window.radioLowerTableViewColoredCells.checked([&] { ipm2.set_color_role(ng::color_role::Background); window.tableView2.update(); itpm2.set_color_role(ng::color_role::Background); window.treeView2.update(); });

        ng::basic_item_model<ng::easing> easingItemModelUpperTableView;
        window.dropListEasingUpperTableView.SelectionChanged([&](const ng::optional_item_model_index& aIndex) 
        { 
            tableView1.vertical_scrollbar().set_transition(easingItemModelUpperTableView.item(*aIndex), 0.75); 
        });
        for (auto i = 0; i < ng::standard_easings().size(); ++i)
            easingItemModelUpperTableView.insert_item(easingItemModelUpperTableView.end(), ng::standard_easings()[i], ng::to_string(ng::standard_easings()[i]));
        easing_item_presentation_model easingPresentationModelUpperTableView{ window.dropListEasingUpperTableView, easingItemModelUpperTableView, false };
        window.dropListEasingUpperTableView.set_size_policy(ng::size_constraint::Minimum);
        window.dropListEasingUpperTableView.set_model(easingItemModelUpperTableView);
        window.dropListEasingUpperTableView.set_presentation_model(easingPresentationModelUpperTableView);
        window.dropListEasingUpperTableView.selection_model().set_current_index(ng::item_presentation_model_index{ ng::standard_easing_index(ng::easing::One) });
        window.dropListEasingUpperTableView.accept_selection();

        ng::basic_item_model<ng::easing> easingItemModelLowerTableView;
        window.dropListEasingLowerTableView.SelectionChanged([&](const ng::optional_item_model_index& aIndex)
        {
            tableView2.vertical_scrollbar().set_transition(easingItemModelLowerTableView.item(*aIndex), 0.75);
        });
        for (auto i = 0; i < ng::standard_easings().size(); ++i)
            easingItemModelLowerTableView.insert_item(easingItemModelLowerTableView.end(), ng::standard_easings()[i], ng::to_string(ng::standard_easings()[i]));
        easing_item_presentation_model easingPresentationModelLowerTableView{ window.dropListEasingLowerTableView, easingItemModelLowerTableView, false };
        window.dropListEasingLowerTableView.set_size_policy(ng::size_constraint::Minimum);
        window.dropListEasingLowerTableView.set_model(easingItemModelLowerTableView);
        window.dropListEasingLowerTableView.set_presentation_model(easingPresentationModelLowerTableView);
        window.dropListEasingLowerTableView.selection_model().set_current_index(ng::item_presentation_model_index{ ng::standard_easing_index(ng::easing::One) });
        window.dropListEasingLowerTableView.accept_selection();

        window.radioNoTableViewSelection.checked([&]() { window.tableView1.selection_model().set_mode(ng::item_selection_mode::NoSelection); window.tableView2.selection_model().set_mode(ng::item_selection_mode::NoSelection); });
        window.radioSingleTableViewSelection.checked([&]() { window.tableView1.selection_model().set_mode(ng::item_selection_mode::SingleSelection); window.tableView2.selection_model().set_mode(ng::item_selection_mode::SingleSelection); });
        window.radioMultipleTableViewSelection.checked([&]() { window.tableView1.selection_model().set_mode(ng::item_selection_mode::MultipleSelection); window.tableView2.selection_model().set_mode(ng::item_selection_mode::MultipleSelection); });
        window.radioExtendedTableViewSelection.checked([&]() { window.tableView1.selection_model().set_mode(ng::item_selection_mode::ExtendedSelection); window.tableView2.selection_model().set_mode(ng::item_selection_mode::ExtendedSelection); });

        window.radioSingleTableViewSelection.check();

        tableView1.selection_model().current_index_changed([&](const ng::optional_item_presentation_model_index& aCurrentIndex, const ng::optional_item_presentation_model_index&)
        {
            if (aCurrentIndex)
            {
                auto const modelIndex = tableView1.presentation_model().to_item_model_index(*aCurrentIndex);
                tableView2.selection_model().set_current_index(tableView2.presentation_model().from_item_model_index(modelIndex));
            }
        });

        tableView2.selection_model().current_index_changed([&](const ng::optional_item_presentation_model_index& aCurrentIndex, const ng::optional_item_presentation_model_index&)
        {
            if (aCurrentIndex)
            {
                auto const modelIndex = tableView2.presentation_model().to_item_model_index(*aCurrentIndex);
                tableView1.selection_model().set_current_index(tableView1.presentation_model().from_item_model_index(modelIndex));
            }
        });

        ng::service<ng::i_window_manager>().restore_mouse_cursor(window);

        #ifdef NDEBUG
        for (int i = 0; i < 1000; ++i)
        #else
        for (int i = 0; i < 100; ++i)
        #endif
            window.layoutLots.emplace<ng::push_button>(boost::lexical_cast<std::string>(i));

        ng::image hash(":/test/resources/channel_32.png");
        for (uint32_t i = 0; i < 9; ++i)
        {
            auto hashWidget = ng::make_ref<ng::image_widget>(hash, ng::aspect_ratio::Keep, static_cast<ng::cardinal>(i));
            hashWidget->set_size_policy(ng::size_constraint::Expanding);
            hashWidget->set_background_color(i % 2 == 0 ? ng::color::Black : ng::color::White);
            window.gridLayoutImages.add_item_at_position(i / 3, i % 3, hashWidget);
        }
        ng::image smallHash(":/test/resources/channel.png");

        bool gameCreated = false;
        ng::game::i_ecs* gameEcs = nullptr;
        window.pageGame.VisibilityChanged([&]()
        {
            if (window.pageGame.visible() && !gameCreated)
            {
                auto& ecs = create_game(window.layoutGame);
                gameEcs = &ecs;
                auto& worldClock = ecs.shared_component<ng::game::clock>()[0];
                window.sliderBoxTimestep.ValueChanged([&]() 
                { 
                    worldClock.timestep = ng::game::chrono::to_flicks(window.sliderBoxTimestep.value()).count();
                });
                window.sliderBoxTimestepGrowth.ValueChanged([&]() 
                { 
                    worldClock.timestepGrowth = window.sliderBoxTimestepGrowth.value();
                });
                window.sliderBoxMaximumTimestep.ValueChanged([&]() 
                { 
                    worldClock.maximumTimestep = ng::game::chrono::to_flicks(window.sliderBoxMaximumTimestep.value()).count();
                });
                window.sliderBoxYieldAfter.ValueChanged([&]()
                {
                    ecs.system<ng::game::simple_physics>().yield_after(std::chrono::duration<double, std::milli>{window.sliderBoxYieldAfter.value() });
                });
                window.checkCollisions.set_checked(true);
                window.checkCollisions.Toggled([&]()
                {
                    if (window.checkCollisions.is_checked())
                        ecs.system<ng::game::collision_detector>().resume();
                    else
                        ecs.system<ng::game::collision_detector>().pause();
                });
                gameCreated = true;
            }
        });

        neolib::basic_random<uint8_t> rngColor;
        auto random_color = [&]()
        {
            return ng::color{ rngColor(255), rngColor(255), rngColor(255) };
        };

        ng::basic_item_model<ng::easing> easingItemModel;
        for (auto i = 0; i < ng::standard_easings().size(); ++i)
            easingItemModel.insert_item(easingItemModel.end(), ng::standard_easings()[i], ng::to_string(ng::standard_easings()[i]));
        easing_item_presentation_model easingPresentationModel{ window.dropListEasing, easingItemModel };
        window.dropListEasing.set_size_policy(ng::size_constraint::Minimum);
        window.dropListEasing.set_model(easingItemModel);
        window.dropListEasing.set_presentation_model(easingPresentationModel);
        window.dropListEasing.selection_model().set_current_index(ng::item_presentation_model_index{ 0 });
        window.dropListEasing.accept_selection();
        ng::texture logo{ ng::image{ ":/test/resources/neoGFX.png" } };

        std::array<ng::texture, 4> tex = 
        {
            ng::texture{ ng::size{64.0, 64.0}, 1.0, ng::texture_sampling::Multisample },
            ng::texture{ ng::size{64.0, 64.0}, 1.0, ng::texture_sampling::Multisample },
            ng::texture{ ng::size{64.0, 64.0}, 1.0, ng::texture_sampling::Multisample },
            ng::texture{ ng::size{64.0, 64.0}, 1.0, ng::texture_sampling::Multisample }
        };
        std::array<ng::color, 4> texColor =
        {
            ng::color::Red,
            ng::color::Green,
            ng::color::Blue,
            ng::color::White
        };
        ng::font renderToTextureFont{ "Exo 2", ng::font_style::Bold, 11.0 };
        auto test_pattern = [renderToTextureFont](ng::i_graphics_context& aGc, const ng::point& aOrigin, double aDpiScale, const ng::color& aColor, std::string const& aText)
        {
            aGc.draw_circle(aOrigin + ng::point{ 32.0, 32.0 }, 32.0, ng::pen{ aColor, aDpiScale * 2.0 });
            aGc.draw_rect(ng::rect{ aOrigin + ng::point{ 0.0, 0.0 }, ng::size{ 64.0, 64.0 } }, ng::pen{ aColor, 1.0 });
            aGc.draw_line(aOrigin + ng::point{ 0.0, 0.0 }, aOrigin + ng::point{ 64.0, 64.0 }, ng::pen{ aColor, aDpiScale * 4.0 });
            aGc.draw_line(aOrigin + ng::point{ 64.0, 0.0 }, aOrigin + ng::point{ 0.0, 64.0 }, ng::pen{ aColor, aDpiScale * 4.0 });
            aGc.draw_multiline_text(aOrigin + ng::point{ 4.0, 4.0 }, aText, renderToTextureFont, ng::text_appearance{ ng::color::White, ng::text_effect{ ng::text_effect_type::Outline, ng::color::Black, 2.0 } });
            aGc.draw_pixel(aOrigin + ng::point{ 2.0, 2.0 }, ng::color{ 0xFF, 0x01, 0x01, 0xFF });
            aGc.draw_pixel(aOrigin + ng::point{ 3.0, 2.0 }, ng::color{ 0x02, 0xFF, 0x02, 0xFF });
            aGc.draw_pixel(aOrigin + ng::point{ 4.0, 2.0 }, ng::color{ 0x03, 0x03, 0xFF, 0xFF });
        };

        // render to texture demo
        for (std::size_t i = 0; i < 4; ++i)
        {
            ng::graphics_context texGc{ tex[i] };
            ng::scoped_snap_to_pixel snap{ texGc };
            test_pattern(texGc, ng::point{}, 1.0, texColor[i], "Render\nTo\nTexture");
        }

        window.pageDrawing.painting([&](ng::i_graphics_context& aGc)
        {
            aGc.draw_rect(ng::rect{ ng::point{ 5, 5 }, ng::size{ 2, 2 } }, ng::color::White);
            aGc.draw_pixel(ng::point{ 7, 7 }, ng::color::Blue);
            aGc.draw_focus_rect(ng::rect{ ng::point{ 8, 8 }, ng::size{ 16, 16 } });
            aGc.fill_rounded_rect(ng::rect{ ng::point{ 100, 100 }, ng::size{ 100, 100 } }, 10.0, ng::color::Goldenrod);
            aGc.fill_rect(ng::rect{ ng::point{ 300, 250 }, ng::size{ 200, 100 } }, window.gradientWidget.gradient().with_direction(ng::gradient_direction::Horizontal));
            aGc.fill_rounded_rect(ng::rect{ ng::point{ 300, 400 }, ng::size{ 200, 100 } }, 10.0, window.gradientWidget.gradient().with_direction(ng::gradient_direction::Horizontal));
            aGc.draw_rounded_rect(ng::rect{ ng::point{ 300, 400 }, ng::size{ 200, 100 } }, 10.0, ng::pen{ ng::color::Blue4, 2.0 });
            aGc.draw_rounded_rect(ng::rect{ ng::point{ 150, 150 }, ng::size{ 300, 300 } }, 10.0, ng::pen{ ng::color::Red4, 2.0 });
            aGc.fill_rounded_rect(ng::rect{ ng::point{ 500, 500 }, ng::size{ 200, 200 } }, 10.0, window.gradientWidget.gradient().with_direction(ng::gradient_direction::Radial));
            aGc.draw_rounded_rect(ng::rect{ ng::point{ 500, 500 }, ng::size{ 200, 200 } }, 10.0, ng::pen{ ng::color::Black, 1.0 });
            for (int x = 0; x < 3; ++x)
            {
                aGc.fill_rect(ng::rect{ ng::point{ 600.0 + x * 17, 600.0 }, ng::size{ 16, 16 } }, ng::color::Green);
                aGc.draw_rect(ng::rect{ ng::point{ 600.0 + x * 17, 600.0 }, ng::size{ 16, 16 } }, ng::pen{ ng::color::White, 1.0 });
            }
            aGc.fill_arc(ng::point{ 500, 50 }, 75, 0.0, ng::to_rad(45.0), ng::color::Chocolate);
            aGc.draw_arc(ng::point{ 500, 50 }, 75, 0.0, ng::to_rad(45.0), ng::pen{ ng::color::White, 3.0 });
            aGc.draw_arc(ng::point{ 500, 50 }, 50, ng::to_rad(5.0), ng::to_rad(40.0), ng::pen{ ng::color::Yellow, 3.0 });

            for (int x = 0; x < 10; ++x)
                for (int y = 0; y < 10; ++y)
                    if ((x + y % 2) % 2 == 0)
                        aGc.draw_pixel(ng::point{ 32.0 + x, 32.0 + y }, ng::color::Black);
                    else
                        aGc.set_pixel(ng::point{ 32.0 + x, 32.0 + y }, ng::color::Goldenrod);

            // easing function demo
            ng::scalar t = static_cast<ng::scalar>(app.program_elapsed_us());
            auto const d = 1000000.0;
            auto const x = ng::ease(easingItemModel.item(window.dropListEasing.selection()), int(t / d) % 2 == 0 ? std::fmod(t, d) / d : 1.0 - std::fmod(t, d) / d) * (window.pageDrawing.extents().cx - logo.extents().cx);
            aGc.draw_texture(ng::point{ x, (window.pageDrawing.extents().cy - logo.extents().cy) / 2.0 }, logo);

            auto texLocation = ng::point{ (window.pageDrawing.extents().cx - 64.0) / 2.0, (window.pageDrawing.extents().cy - logo.extents().cy) / 4.0 }.ceil();
            aGc.draw_texture(texLocation + ng::point{ 0.0, 0.0 }, tex[0]);
            aGc.draw_texture(texLocation + ng::point{ 0.0, 65.0 }, tex[1]);
            aGc.draw_texture(texLocation + ng::point{ 65.0, 0.0 }, tex[2]);
            aGc.draw_texture(texLocation + ng::point{ 65.0, 65.0 }, tex[3]);

            texLocation.x += 140.0;
            test_pattern(aGc, texLocation + ng::point{ 0.0, 0.0 }, 1.0_dip, texColor[0], "Render\nTo\nScreen");
            test_pattern(aGc, texLocation + ng::point{ 0.0, 65.0 }, 1.0_dip, texColor[1], "Render\nTo\nScreen");
            test_pattern(aGc, texLocation + ng::point{ 65.0, 0.0 }, 1.0_dip, texColor[2], "Render\nTo\nScreen");
            test_pattern(aGc, texLocation + ng::point{ 65.0, 65.0 }, 1.0_dip, texColor[3], "Render\nTo\nScreen");
        });

        window.buttonStyle1.clicked([&window]()
        {
            window.textEditEditor.set_default_style(ng::text_edit::character_style{ ng::optional_font(), ng::gradient(ng::color::Red, ng::color::White, ng::gradient_direction::Horizontal), ng::color_or_gradient() });
        });

        window.buttonStyle2.clicked([&window]()
        {
            window.textEditEditor.set_default_style(ng::text_edit::character_style{ ng::font("SnareDrum One NBP", "Regular", 60.0), ng::color::White });
        });

        window.canvasInstancing.set_logical_coordinate_system(ng::logical_coordinate_system::AutomaticGui);

        window.groupRenderingScheme.set_fill_opacity(0.5);
        window.groupMeshShape.set_fill_opacity(0.5);

        window.radioCircle.check();
        window.checkOutline.check();
        window.checkFill.check();
        window.radioEcsInstancing.disable();

        ng::font infoFont{ "SnareDrum Two NBP", "Regular", 40.0 };

        std::optional<ng::game::ecs> ecs;
        ng::sink sink;

        ng::rect instancingRect;

        auto entity_transformation = [&](ng::game::component<ng::game::mesh_filter>& aComponent, ng::game::component<ng::game::mesh_render_cache>& aCache, ng::game::mesh_filter& aFilter)
        {
            thread_local auto seed = ( neolib::simd_srand(std::this_thread::get_id()), 42);
            if (!aFilter.transformation)
                aFilter.transformation = ng::mat44::identity();
            (*aFilter.transformation)[3][0] = neolib::simd_rand(instancingRect.cx - 1);
            (*aFilter.transformation)[3][1] = neolib::simd_rand(instancingRect.cy - 1);
            ng::game::set_render_cache_dirty_no_lock(aCache, aComponent.entity(aFilter));
        };

        std::atomic<bool> useThreadPool = false;
        window.checkThreadPool.Checked([&]() { useThreadPool = true; });
        window.checkThreadPool.Unchecked([&]() { useThreadPool = false; });

#ifdef USE_AVX_DYNAMIC
        neolib::use_simd() = false;
        window.checkUseSimd.Checked([&]() { neolib::use_simd() = true; });
        window.checkUseSimd.Unchecked([&]() { neolib::use_simd() = false; });
#else
        window.checkUseSimd.disable();
#endif

        auto update_ecs_entities = [&](ng::game::step_time aPhysicsStepTime)
        {
            auto& cache = ecs->component<ng::game::mesh_render_cache>();
            auto update_function = [&](ng::game::component<ng::game::mesh_filter>& aComponent, ng::game::mesh_filter& aFilter)
            {
                entity_transformation(aComponent, cache, aFilter);
            };
            instancingRect = window.pageInstancing.client_rect();
            if (useThreadPool)
                ecs->component<ng::game::mesh_filter>().parallel_apply(update_function);
            else
                ecs->component<ng::game::mesh_filter>().apply(update_function);
        };

        auto configure_ecs = [&]()
        {
            sink.clear();
            bool needEcs = window.radioEcsBatching.is_checked() || window.radioEcsInstancing.is_checked();
            if (!needEcs)
            {
                window.canvasInstancing.set_ecs({});
                ecs = std::nullopt;
            }
            else
            {
                if (!ecs)
                {
                    ecs.emplace(ng::game::ecs_flags::Default | ng::game::ecs_flags::NoThreads);
                    ecs->component<ng::game::mesh_filter>(); // todo: should animator include mesh_filter?
                    ecs->system<ng::game::animator>().start_thread();
                    sink += ~~~~ecs->system<ng::game::animator>().Animate(update_ecs_entities);
                    window.canvasInstancing.set_ecs(*ecs);
                }
                if (window.checkPauseAnimation.is_checked() && !ecs->system<ng::game::animator>().paused())
                {
                    ecs->system<ng::game::time>().pause();
                    ecs->system<ng::game::animator>().pause();
                }
                else if (!window.checkPauseAnimation.is_checked() && ecs->system<ng::game::animator>().paused())
                {
                    ecs->system<ng::game::time>().resume();
                    ecs->system<ng::game::animator>().resume();
                }
                auto const meshesWanted = window.sliderShapeCount.value();
                ng::game::scoped_component_lock<ng::game::mesh_filter> lock{ *ecs };
                auto& meshFilterComponent = ecs->component<ng::game::mesh_filter>();
                auto& meshFilters = meshFilterComponent.component_data();
                auto const& instancingRect = window.pageInstancing.client_rect();
                while (meshFilters.size() != meshesWanted)
                {
                    if (meshFilters.size() < meshesWanted)
                    {
                        auto r = ng::rect{ ng::point{ neolib::simd_rand(instancingRect.cx - 1), neolib::simd_rand(instancingRect.cy - 1) }, ng::size_i32{ window.sliderShapeSize.value() } };
                        ng::game::shape::rectangle
                        {
                            *ecs,
                            ng::vec3{},
                            ng::vec2{ r.cx, r.cy },
                            random_color().with_alpha(random_color().red())
                        }.detach();
                    }
                    else
                        ecs->destroy_entity(meshFilterComponent.entities()[0]);
                }
            }
        };

        window.radioOff.Checked([&]() { configure_ecs(); });
        window.radioImmediateBatching.Checked([&]() { configure_ecs(); });
        window.radioEcsBatching.Checked([&]() { configure_ecs(); });
        window.radioEcsInstancing.Checked([&]() { configure_ecs(); });
        window.sliderShapeCount.ValueChanged([&]() { configure_ecs(); });
        window.sliderShapeSize.ValueChanged([&]() { configure_ecs(); });
        window.checkOutline.Toggled([&]() { configure_ecs(); });
        window.checkFill.Toggled([&]() { configure_ecs(); });
        window.radioCircle.Toggled([&]() { configure_ecs(); });
        window.checkPauseAnimation.Toggled([&]() { configure_ecs(); });

        window.canvasInstancing.PaintingChildren([&](ng::i_graphics_context& aGc)
        {
            if (window.radioOff.is_checked())
            {
                aGc.draw_multiline_text(ng::point{ 32.0, 32.0 }, 
                    "WARNING\n\n\nTHIS TEST CAN POTENTIALLY TRIGGER\nSEIZURES FOR PEOPLE WITH\nPHOTOSENSITIVE EPILEPSY\n\nDISCRETION IS ADVISED", 
                    infoFont, 0.0, ng::text_appearance{ ng::color::Coral, ng::text_effect{ ng::text_effect_type::Outline, ng::color::Black, 2.0 } }, ng::alignment::Center);
                return;
            }
            if (!ecs)
            {
                for (int32_t i = 0; i < window.sliderShapeCount.value(); ++i)
                {
                    if (window.checkOutline.is_checked() && window.checkFill.is_unchecked())
                    {
                        if (window.radioCircle.is_checked())
                            aGc.draw_circle(
                                ng::point{ neolib::simd_rand(window.pageInstancing.client_rect().cx - 1), neolib::simd_rand(window.pageInstancing.client_rect().extents().cy - 1) }, window.sliderShapeSize.value(),
                                ng::pen{ random_color(), 2.0 });
                        else
                            aGc.draw_rect(
                                ng::rect{ ng::point{ neolib::simd_rand(window.pageInstancing.client_rect().cx - 1), neolib::simd_rand(window.pageInstancing.client_rect().extents().cy - 1) }, ng::size_i32{ window.sliderShapeSize.value() } },
                                ng::pen{ random_color(), 2.0 });
                    }
                    else if (window.checkOutline.is_checked() && window.checkFill.is_checked())
                    {
                        if (window.radioCircle.is_checked())
                            aGc.draw_circle(
                                ng::point{ neolib::simd_rand(window.pageInstancing.client_rect().cx - 1), neolib::simd_rand(window.pageInstancing.client_rect().cy - 1) }, window.sliderShapeSize.value(),
                                ng::pen{ random_color(), 2.0 },
                                random_color().with_alpha(random_color().red()));
                        else
                            aGc.draw_rect(
                                ng::rect{ ng::point{ neolib::simd_rand(window.pageInstancing.client_rect().cx - 1), neolib::simd_rand(window.pageInstancing.client_rect().extents().cy - 1) }, ng::size_i32{ window.sliderShapeSize.value() } },
                                ng::pen{ random_color(), 2.0 },
                                random_color().with_alpha(random_color().red()));
                    }
                    else
                    {
                        if (window.radioCircle.is_checked())
                            aGc.fill_circle(
                                ng::point{ neolib::simd_rand(window.pageInstancing.client_rect().cx - 1), neolib::simd_rand(window.pageInstancing.client_rect().cy - 1) }, window.sliderShapeSize.value(),
                                random_color().with_alpha(random_color().red()));
                        else
                            aGc.fill_rect(
                                ng::rect{ ng::point{ neolib::simd_rand(window.pageInstancing.client_rect().cx - 1), neolib::simd_rand(window.pageInstancing.client_rect().extents().cy - 1) }, ng::size_i32{ window.sliderShapeSize.value() } },
                                random_color().with_alpha(random_color().red()));
                    }
                }
            }
            thread_local std::string currentInfoText;
            thread_local std::optional<ng::glyph_text> infoGlyphText;
            bool const mouseOver = window.pageInstancing.client_rect().contains(window.pageInstancing.mouse_position());
            if (mouseOver)
            {
                std::ostringstream newInfoText;
                newInfoText << "Shape count: " << window.sliderShapeCount.value() << "  Shape size: " << window.sliderShapeSize.value() << "\nMesh vertex count: " << "???" << "  #gamedev :D";
                if (currentInfoText != newInfoText.str())
                {
                    currentInfoText = newInfoText.str();
                    infoGlyphText = aGc.to_glyph_text(currentInfoText, infoFont);
                }
                aGc.draw_multiline_glyph_text(ng::point{ 32.0, 32.0 }, *infoGlyphText, 0.0, ng::text_appearance{ ng::color::White, ng::text_effect{ ng::text_effect_type::Outline, ng::color::Black, 2.0 } });
            }
        });

        neolib::callback_timer animator{ app, [&](neolib::callback_timer& aTimer)
        {
            if (!window.has_native_window())
                return;
            bool canUpdateCanvas = !ecs && window.canvasInstancing.can_update();
            aTimer.set_duration(window.pageDrawing.can_update() || canUpdateCanvas ? std::chrono::milliseconds{ 0 } : std::chrono::milliseconds{ 1 });
            aTimer.again();
            window.pageDrawing.update();
            window.canvasInstancing.update();
            bool const mouseOver = window.canvasInstancing.entered(true);
            window.groupRenderingScheme.show(mouseOver);
            window.groupMeshShape.show(mouseOver);
            if (window.groupBox.is_checkable() && window.groupBox.check_box().is_checked())
                window.update();
            if (gameEcs)
            {
                if (gameEcs->system<ng::game::simple_physics>().can_apply())
                    gameEcs->system<ng::game::simple_physics>().apply();
                if (gameEcs->system<ng::game::collision_detector>().can_apply())
                    gameEcs->system<ng::game::collision_detector>().apply();
            }
        }, std::chrono::milliseconds{ 1 } };

        return app.exec();
    }
    catch (std::exception& e)
    {
        app.halt();
        ng::service<ng::debug::logger>() << "neogfx::app::exec: terminating with exception: " << e.what() << ng::endl;
        ng::service<ng::i_surface_manager>().display_error_message(app.name().empty() ? "Abnormal Program Termination" : "Abnormal Program Termination - " + app.name(), std::string("main: terminating with exception: ") + e.what());
        std::exit(EXIT_FAILURE);
    }
    catch (...)
    {
        app.halt();
        ng::service<ng::debug::logger>() << "neogfx::app::exec: terminating with unknown exception" << ng::endl;
        ng::service<ng::i_surface_manager>().display_error_message(app.name().empty() ? "Abnormal Program Termination" : "Abnormal Program Termination - " + app.name(), "main: terminating with unknown exception");
        std::exit(EXIT_FAILURE);
    }
}

