// drop_list.hpp
/*
  neogfx C++ App/Game Engine
  Copyright (c) 2015, 2020 Leigh Johnston.  All Rights Reserved.
  
  This program is free software: you can redistribute it and / or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <neogfx/neogfx.hpp>
#include <neogfx/gui/window/window.hpp>
#include <neogfx/gui/widget/list_view.hpp>
#include <neogfx/gui/widget/item_presentation_model.hpp>

namespace neogfx
{
    class drop_list;

    template <typename ItemModel = item_model>
    class default_drop_list_presentation_model : public basic_item_presentation_model<ItemModel>
    {
        typedef basic_item_presentation_model<ItemModel> base_type;
    public:
        using item_model_type = ItemModel;
    public:
        default_drop_list_presentation_model(drop_list& aDropList) : base_type{}, iDropList{ aDropList }
        {
        }
        default_drop_list_presentation_model(drop_list& aDropList, item_model_type& aModel) : base_type{ aModel }, iDropList{ aDropList }
        {
        }
    public:
        item_cell_flags column_flags(item_presentation_model_index::value_type aColumn) const override
        {
            return base_type::column_flags(aColumn) & ~item_cell_flags::Editable;
        }
    public:
        optional_color cell_color(item_presentation_model_index const& aIndex, color_role aColorRole) const override
        {
            if (iDropList.view_created())
            {
                if (aColorRole == color_role::Background)
                {
                    if ((base_type::cell_meta(aIndex).selection & item_cell_selection_flags::Current) == item_cell_selection_flags::Current)
                    {
                        auto backgroundColor = iDropList.view().palette_color(color_role::Void);
                        if (backgroundColor == iDropList.view().background_color())
                            backgroundColor = backgroundColor.shaded(0x20);
                        return backgroundColor;
                    }
                    else
                        return iDropList.view().palette_color(color_role::AlternateBase);
                }
            }
            return {};
        }
    private:
        drop_list& iDropList;
    };

    class drop_list_view : public list_view
    {
        friend class drop_list_popup;
        friend class drop_list;
    public:
        drop_list_view(i_layout& aLayout, drop_list& aDropList);
        ~drop_list_view();
    public:
        using list_view::total_item_area;
    protected:
        void items_filtered() override;
    protected:
        void current_index_changed(const optional_item_presentation_model_index& aCurrentIndex, const optional_item_presentation_model_index& aPreviousIndex) override;
    protected:
        void mouse_button_released(mouse_button aButton, const point& aPosition) override;
    protected:
        bool key_pressed(scan_code_e aScanCode, key_code_e aKeyCode, key_modifiers_e aKeyModifiers) override;
    private:
        drop_list& iDropList;
    };

    class drop_list_popup : public window
    {
        friend class drop_list;
        friend class drop_list_view;
    public:
        drop_list_popup(drop_list& aDropList);
        ~drop_list_popup();
    public:
        const drop_list_view& view() const;
        drop_list_view& view();
    protected:
        color frame_color() const override;
    protected:
        double rendering_priority() const override;
    public:
        using window::show;
        bool show(bool aVisible) override;
    protected:
        neogfx::size_policy size_policy() const override;
        size minimum_size(optional_size const& aAvailableSpace = optional_size{}) const override;
    public:
        bool can_dismiss(const i_widget*) const override;
        dismissal_type_e dismissal_type() const override;
        bool dismissed() const override;
        void dismiss() override;
    private:
        size ideal_size() const;
        void update_placement();
    private:
        drop_list& iDropList;
        drop_list_view iView;
    };

    class i_drop_list_input_widget
    {
    public:
        declare_event(text_changed)
    public:
        typedef i_drop_list_input_widget abstract_type;
        class i_visitor
        {
        public:
            virtual void visit(i_drop_list_input_widget& aInputWidget, push_button& aButtonWidget) = 0;
            virtual void visit(i_drop_list_input_widget& aInputWidget, line_edit& aTextWidget) = 0;
        };
    public:
        virtual ~i_drop_list_input_widget() = default;
    public:
        virtual void accept(i_visitor& aVisitor) = 0;
    public:
        virtual const i_widget& as_widget() const = 0;
        virtual i_widget& as_widget() = 0;
    public:
        virtual bool editable() const = 0;
        virtual const i_widget& image_widget() const = 0;
        virtual i_widget& image_widget() = 0;
        virtual const i_widget& text_widget() const = 0;
        virtual i_widget& text_widget() = 0;
        virtual size spacing() const = 0;
        virtual void set_spacing(const size& aSpacing) = 0;
        virtual const i_texture& image() const = 0;
        virtual void set_image(const i_texture& aImage) = 0;
        virtual i_string const& text() const = 0;
        virtual void set_text(i_string const& aText) = 0;
    };

    enum class drop_list_style : uint32_t
    {
        Normal              = 0x0000,
        Editable            = 0x0001,
        ListAlwaysVisible   = 0x0002,
        NoFilter            = 0x0004
    };
}

begin_declare_enum(neogfx::drop_list_style)
declare_enum_string(neogfx::drop_list_style, Normal)
declare_enum_string(neogfx::drop_list_style, Editable)
declare_enum_string(neogfx::drop_list_style, ListAlwaysVisible)
declare_enum_string(neogfx::drop_list_style, NoFilter)
end_declare_enum(neogfx::drop_list_style)

namespace neogfx
{
    inline drop_list_style operator|(drop_list_style aLhs, drop_list_style aRhs)
    {
        return static_cast<drop_list_style>(static_cast<uint32_t>(aLhs) | static_cast<uint32_t>(aRhs));
    }

    inline drop_list_style operator&(drop_list_style aLhs, drop_list_style aRhs)
    {
        return static_cast<drop_list_style>(static_cast<uint32_t>(aLhs) & static_cast<uint32_t>(aRhs));
    }

    inline drop_list_style operator~(drop_list_style aLhs)
    {
        return static_cast<drop_list_style>(~static_cast<uint32_t>(aLhs));
    }

    class drop_list : public widget<>, private i_drop_list_input_widget::i_visitor
    {
        friend class drop_list_view;
    public:
        define_event(SelectionChanged, selection_changed, optional_item_model_index)
    public:
        struct no_selection : std::runtime_error { no_selection() : std::runtime_error("neogfx::drop_list::no_selection") {} };
    private:
        class list_proxy
        {
        private:
            class view_container : public framed_widget<>
            {
                typedef framed_widget<> base_type;
            public:
                view_container(i_layout& aLayout);
            public:
                color palette_color(color_role aColorRole) const override;
                color frame_color() const override;
            };
        public:
            struct no_view : std::logic_error { no_view() : std::logic_error("neogfx::drop_list::list_proxy::no_view") {} };
        public:
            list_proxy(drop_list& aDropList);
            ~list_proxy();
        public:
            bool view_created() const;
            bool view_visible() const;
            drop_list_view& view() const;
            dimension effective_frame_width() const;
        public:
            void show_view();
            void hide_view();
            void close_view();
            void update_view_placement();
        private:
            drop_list& iDropList;
            mutable std::optional<drop_list_popup> iPopup;
            mutable std::optional<view_container> iViewContainer;
            mutable std::optional<drop_list_view> iView;
            sink iSink;
        };
    public:
        drop_list(drop_list_style aStyle = drop_list_style::Normal);
        drop_list(i_widget& aParent, drop_list_style aStyle = drop_list_style::Normal);
        drop_list(i_layout& aLayout, drop_list_style aStyle = drop_list_style::Normal);
        ~drop_list();
    public:
        bool has_model() const;
        const i_item_model& model() const;
        i_item_model& model();
        void set_model(i_item_model& aModel);
        void set_model(i_ref_ptr<i_item_model> const& aModel);
        bool has_presentation_model() const;
        const i_item_presentation_model& presentation_model() const;
        i_item_presentation_model& presentation_model();
        void set_presentation_model(i_item_presentation_model& aPresentationModel);
        void set_presentation_model(i_ref_ptr<i_item_presentation_model> const& aPresentationModel);
        bool has_selection_model() const;
        const i_item_selection_model& selection_model() const;
        i_item_selection_model& selection_model();
        void set_selection_model(i_item_selection_model& aSelectionModel);
        void set_selection_model(i_ref_ptr<i_item_selection_model> const& aSelectionModel);
    public:
        bool has_selection() const;
        const item_model_index& selection() const;
    public:
        bool input_matches_current_item() const;
        bool input_matches_selection() const;
    public:
        bool view_created() const;
        bool view_visible() const;
        void show_view();
        void hide_view();
        void close_view();
        drop_list_view& view() const;
        void accept_selection();
        void cancel_selection(bool aClearInput = false);
        void cancel_and_restore_selection(bool aOnlyRestoreIfViewCreated = false);
    public:
        drop_list_style style() const;
        void set_style(drop_list_style aStyle);
        bool editable() const;
        void set_editable(bool aEditable);
        bool list_always_visible() const;
        void set_list_always_visible(bool aListAlwaysVisible);
        bool filter_enabled() const;
        void enable_filter(bool aEnableFilter);
        bool has_input() const;
        const i_drop_list_input_widget& input_widget() const;
        i_drop_list_input_widget& input_widget();
    public:
        bool changing_text() const;
        bool handling_text_change() const;
        bool accepting_selection() const;
        bool cancelling_selection() const;
    public:
        neogfx::size_policy size_policy() const override;
        size minimum_size(optional_size const& aAvailableSpace = optional_size{}) const override;
    private:
        void visit(i_drop_list_input_widget& aInputWidget, push_button& aButtonWidget) override;
        void visit(i_drop_list_input_widget& aInputWidget, line_edit& aTextWidget) override;
    private:
        void init();
        void update_widgets(bool aForce = false);
        void update_arrow();
        void handle_clicked();
        void handle_cancel_selection(bool aRestoreSavedSelection, bool aUpdateEditor = true);
        bool handle_proxy_key_event(const neogfx::keyboard_event& aEvent);
    private:
        drop_list_style iStyle;
        vertical_layout iLayout0;
        horizontal_layout iLayout1;
        std::unique_ptr<i_drop_list_input_widget> iInputWidget;
        ref_ptr<i_item_model> iModel;
        ref_ptr<i_item_presentation_model> iPresentationModel;
        ref_ptr<i_item_selection_model> iSelectionModel;
        sink iSink;
        sink iSelectionSink;
        mutable std::optional<std::pair<color, texture>> iDownArrowTexture;
        image_widget iDownArrow;
        list_proxy iListProxy;
        optional_item_model_index iSavedSelection;
        optional_item_model_index iSelection;
        bool iChangingText;
        bool iHandlingTextChange;
        bool iAcceptingSelection;
        bool iCancellingSelection;
    };
}