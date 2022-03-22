// spin_box.hpp
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
#include <neogfx/gui/layout/horizontal_layout.hpp>
#include <neogfx/gui/widget/line_edit.hpp>
#include <neogfx/gui/layout/vertical_layout.hpp>
#include <neogfx/gui/widget/push_button.hpp>

namespace neogfx
{
    template <typename T>
    class basic_spin_box : public framed_widget<>
    {
        typedef framed_widget<> base_type;
    public:
        define_event(ValueChanged, value_changed)
        define_event(ConstraintsChanged, constraints_changed)
    public:
        typedef T value_type;
    private:
        enum class step_direction
        {
            Up, 
            Down
        };
    public:
        static constexpr size INTERNAL_SPACING = { 2.0, 0.0 };
        static constexpr size SPIN_BUTTON_MINIMUM_SIZE = { 13.0, 9.0 };
    public:
        basic_spin_box();
        basic_spin_box(i_widget& aParent);
        basic_spin_box(i_layout& aLayout);
    public:
        i_string const& text();
        const line_edit& text_box() const;
        line_edit& text_box();
    public:
        value_type minimum() const;
        void set_minimum(value_type aMinimum);
        value_type maximum() const;
        void set_maximum(value_type aMaximum);
        value_type step() const;
        void set_step(value_type aStep);
        value_type value() const;
        void set_value(value_type aValue, bool aNotify = true);
        std::string const& format() const;
        void set_format(std::string const& aFormat);
        const std::optional<size_hint>& text_box_size_hint() const;
        void set_text_box_size_hint(const std::optional<size_hint>& aSizeHint);
    public:
        std::string const& valid_text_characters() const;
        std::string value_to_string() const;
    protected:
        neogfx::size_policy size_policy() const override;
    protected:
        color palette_color(color_role aColorRole) const override;
    protected:
        color frame_color() const override;
    protected:
        bool mouse_wheel_scrolled(mouse_wheel aWheel, const point& aPosition, delta aDelta, key_modifiers_e aKeyModifiers) override;
    protected:
        bool key_pressed(scan_code_e aScanCode, key_code_e aKeyCode, key_modifiers_e aKeyModifiers) override;
    private:
        void do_step(step_direction aDirection, uint32_t aAmount = 1);
        std::optional<value_type> string_to_value(std::string const& aString) const;
        void init();
        void update_size_hint();
        void update_arrows();
    private:
        sink iSink;
        string iText;
        cursor::position_type iTextCursorPos;
        horizontal_layout iPrimaryLayout;
        line_edit iTextBox;
        vertical_layout iSecondaryLayout;
        push_button iStepUpButton;
        push_button iStepDownButton;
        std::optional<widget_timer> iStepper;
        mutable std::optional<std::pair<color, texture>> iUpArrow;
        mutable std::optional<std::pair<color, texture>> iDownArrow;
        value_type iMinimum;
        value_type iMaximum;
        value_type iStep;
        value_type iValue;
        std::string iFormat;
        std::optional<size_hint> iTextBoxSizeHint;
        bool iDontSetText = false;
    };

    extern template class basic_spin_box<int8_t>;
    extern template class basic_spin_box<uint8_t>;
    extern template class basic_spin_box<int16_t>;
    extern template class basic_spin_box<uint16_t>;
    extern template class basic_spin_box<int32_t>;
    extern template class basic_spin_box<uint32_t>;
    extern template class basic_spin_box<int64_t>;
    extern template class basic_spin_box<uint64_t>;
    extern template class basic_spin_box<float>;
    extern template class basic_spin_box<double>;
    
    typedef basic_spin_box<int8_t> int8_spin_box;
    typedef basic_spin_box<uint8_t> uint8_spin_box;
    typedef basic_spin_box<int16_t> int16_spin_box;
    typedef basic_spin_box<uint16_t> uint16_spin_box;
    typedef basic_spin_box<int32_t> int32_spin_box;
    typedef basic_spin_box<uint32_t> uint32_spin_box;
    typedef basic_spin_box<int64_t> int64_spin_box;
    typedef basic_spin_box<uint64_t> uint64_spin_box;
    typedef basic_spin_box<float> float_spin_box;
    typedef basic_spin_box<double> double_spin_box;

    typedef int32_spin_box spin_box;
}