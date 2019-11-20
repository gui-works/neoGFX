// window.hpp
/*
neoGFX Resource Compiler
Copyright(C) 2019 Leigh Johnston

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
#include <neogfx/tools/nrc/ui_element.hpp>

namespace neogfx::nrc
{
    class window : public ui_element<>
    {
    public:
        window(i_ui_element& aParent) :
            ui_element<>{ aParent, aParent.parser().get<neolib::string>("id"), ui_element_type::Window }
        {
            if (aParent.parser().data_exists("default_size"))
            {
                auto ds = get_lengths("default_size");
                if (!ds.empty())
                    iDefaultSize.emplace(ds[0], ds[std::min<std::size_t>(1u, ds.size() - 1u)]);
                else
                    iDefaultSize.emplace();
            }
        }
    public:
        const neolib::i_string& header() const override
        {
            static const neolib::string sHeader = "neogfx/gui/window/window.hpp";
            return sHeader;
        }
    public:
        void parse(const neolib::i_string& aName, const data_t& aData) override
        {
            ui_element<>::parse(aName, aData);
        }
        void parse(const neolib::i_string& aName, const array_data_t& aData) override
        {
            ui_element<>::parse(aName, aData);
        }
    protected:
        void emit() const override
        {
        }
        void emit_preamble() const override
        {
            emit("  window %1%;\n", id());
            ui_element<>::emit_preamble();
        }
        void emit_ctor() const override
        {
            switch(parent().type())
            {
            case ui_element_type::Widget:
                if (iDefaultSize)
                    emit(",\n"
                        "   %1%{ %2%, %3%, %4% }", id(), parent().id(), iDefaultSize->cx, iDefaultSize->cy);
                else
                    emit(",\n"
                        "   %1%{ %2% }", id(), parent().id());
                break;
            default:
                if (iDefaultSize)
                    emit(",\n"
                        "   %1%{ size{ %2%, %3% } }", id(), iDefaultSize->cx, iDefaultSize->cy);
                break;
            }
            ui_element<>::emit_ctor();
        }
        void emit_body() const override
        {
            ui_element<>::emit_body();
        }
    protected:
        using ui_element<>::emit;
    private:
        std::optional<neogfx::basic_size<length>> iDefaultSize;
    };
}
