// font.hpp
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
#include <neolib/core/jar.hpp>
#include <neolib/core/variant.hpp>
#include <neolib/app/i_settings.hpp>
#include <neogfx/core/geometrical.hpp>

namespace neogfx
{
    class i_native_font_face;
    struct glyph;
    class i_glyph_texture;

    enum class font_style : uint32_t
    {
        Invalid             = 0x00000000,
        Normal              = 0x00000001,
        Italic              = 0x00000002,
        Bold                = 0x00000004,
        Underline           = 0x00000008,
        Superscript         = 0x00000010,
        Subscript           = 0x00000020,
        BelowAscenderLine   = 0x00000040,
        AboveBaseline       = 0x00000040,
        Emulated            = 0x80000000,
        BoldItalic = Bold | Italic,
        BoldItalicUnderline = Bold | Italic | Underline,
        BoldUnderline = Bold | Underline,
        ItalicUnderline = Italic | Underline,
        EmulatedBold = Bold | Emulated,
        EmulatedItalic = Italic | Emulated,
        EmulatedBoldItalic = Bold | Italic | Emulated
    };

    inline constexpr font_style operator~(font_style aLhs)
    {
        return static_cast<font_style>(~static_cast<uint32_t>(aLhs));
    }

    inline constexpr font_style operator|(font_style aLhs, font_style aRhs)
    {
        return static_cast<font_style>(static_cast<uint32_t>(aLhs) | static_cast<uint32_t>(aRhs));
    }

    inline constexpr font_style operator&(font_style aLhs, font_style aRhs)
    {
        return static_cast<font_style>(static_cast<uint32_t>(aLhs) & static_cast<uint32_t>(aRhs));
    }

    inline constexpr font_style operator^(font_style aLhs, font_style aRhs)
    {
        return static_cast<font_style>(static_cast<uint32_t>(aLhs) ^ static_cast<uint32_t>(aRhs));
    }

    inline constexpr font_style& operator|=(font_style& aLhs, font_style aRhs)
    {
        return aLhs = static_cast<font_style>(static_cast<uint32_t>(aLhs) | static_cast<uint32_t>(aRhs));
    }

    inline constexpr font_style& operator&=(font_style& aLhs, font_style aRhs)
    {
        return aLhs = static_cast<font_style>(static_cast<uint32_t>(aLhs) & static_cast<uint32_t>(aRhs));
    }

    inline constexpr font_style& operator^=(font_style& aLhs, font_style aRhs)
    {
        return aLhs = static_cast<font_style>(static_cast<uint32_t>(aLhs) ^ static_cast<uint32_t>(aRhs));
    }
}

begin_declare_enum(neogfx::font_style)
declare_enum_string(neogfx::font_style, Invalid)
declare_enum_string(neogfx::font_style, Normal)
declare_enum_string(neogfx::font_style, Italic)
declare_enum_string(neogfx::font_style, Bold)
declare_enum_string(neogfx::font_style, Underline)
declare_enum_string(neogfx::font_style, Superscript)
declare_enum_string(neogfx::font_style, Subscript)
declare_enum_string(neogfx::font_style, BelowAscenderLine)
declare_enum_string(neogfx::font_style, AboveBaseline)
declare_enum_string(neogfx::font_style, Emulated)
declare_enum_string(neogfx::font_style, BoldItalic)
declare_enum_string(neogfx::font_style, BoldItalicUnderline)
declare_enum_string(neogfx::font_style, BoldUnderline)
declare_enum_string(neogfx::font_style, ItalicUnderline)
declare_enum_string(neogfx::font_style, EmulatedBold)
declare_enum_string(neogfx::font_style, EmulatedItalic)
declare_enum_string(neogfx::font_style, EmulatedBoldItalic)
end_declare_enum(neogfx::font_style)

namespace neogfx 
{
    enum class font_weight : uint32_t
    {
        Unknown = 0,
        Thin = 100,
        Extralight = 200,
        Ultralight = 200,
        Light = 300,
        Normal = 400,
        Regular = 400,
        Medium = 500,
        Semibold = 600,
        Demibold = 600,
        Bold = 700,
        Extrabold = 800,
        Ultrabold = 800,
        Heavy = 900,
        Black = 900
    };

    class font_info
    {
        // exceptions
    public:
        struct unknown_style : std::logic_error { unknown_style() : std::logic_error("neogfx::font_info::unknown_style") {} };
        struct unknown_style_name : std::logic_error { unknown_style_name() : std::logic_error("neogfx::font_info::unknown_style_name") {} };
        // types
    public:
        typedef font_info abstract_type; // todo
        typedef double point_size;
    private:
        typedef std::optional<font_style> optional_style;
        typedef std::optional<string> optional_style_name;
    private:
        class instance;
    public:
        font_info();
        font_info(std::string const& aFamilyName, font_style aStyle, point_size aSize);
        font_info(std::string const& aFamilyName, std::string const& aStyleName, point_size aSize);
        font_info(std::string const& aFamilyName, font_style aStyle, std::string const& aStyleName, point_size aSize);
        font_info(const font_info& aOther);
        virtual ~font_info();
        font_info& operator=(const font_info& aOther);
    private:
        font_info(std::string const& aFamilyName, const optional_style& aStyle, const optional_style_name& aStyleName, point_size aSize);
    public:
        virtual i_string const& family_name() const;
        virtual bool style_available() const;
        virtual font_style style() const;
        virtual bool style_name_available() const;
        virtual i_string const& style_name() const;
        virtual bool underline() const;
        virtual void set_underline(bool aUnderline);
        virtual font_weight weight() const;
        virtual point_size size() const;
        virtual bool kerning() const;
        virtual void enable_kerning();
        virtual void disable_kerning();
    public:
        font_info with_style(font_style aStyle) const;
        font_info with_style_xor(font_style aStyle) const;
        font_info with_underline(bool aUnderline) const;
        font_info with_size(point_size aSize) const;
    public:
        bool operator==(const font_info& aRhs) const;
        bool operator!=(const font_info& aRhs) const;
        bool operator<(const font_info& aRhs) const;
    public:
        static font_weight weight_from_style(font_style aStyle);
        static font_weight weight_from_style_name(std::string aStyleName, bool aUnknownAsRegular = true);
    private:
        mutable std::shared_ptr<instance> iInstance;
    };

    typedef optional<font_info> optional_font_info;

    typedef neolib::small_cookie font_id;

    // todo: abstract font
    class font : public font_info
    {
        friend class font_manager;
        friend class graphics_context;
        // exceptions
    public:
        struct no_fallback_font : std::logic_error { no_fallback_font() : std::logic_error("neogfx::font::no_fallback_font") {} };
        // types
    public:
        typedef font abstract_type; // todo
    private:
        class instance;
        // construction
    public:
        font();
        font(std::string const& aFamilyName, font_style aStyle, point_size aSize);
        font(std::string const& aFamilyName, std::string const& aStyleName, point_size aSize);
        font(const font_info& aFontInfo);
        font(const font& aOther);
        font(const font& aOther, font_style aStyle, point_size aSize);
        font(const font& aOther, std::string const& aStyleName, point_size aSize);
        static font load_from_file(std::string const& aFileName);
        static font load_from_file(std::string const& aFileName, font_style aStyle, point_size aSize);
        static font load_from_file(std::string const& aFileName, std::string const& aStyleName, point_size aSize);
        static font load_from_memory(const void* aData, std::size_t aSizeInBytes);
        static font load_from_memory(const void* aData, std::size_t aSizeInBytes, font_style aStyle, point_size aSize);
        static font load_from_memory(const void* aData, std::size_t aSizeInBytes, std::string const& aStyleName, point_size aSize);
        ~font();
        font& operator=(const font& aOther);
    private:
        font(i_native_font_face& aNativeFontFace);
        font(i_native_font_face& aNativeFontFace, font_style aStyle);
    public:
        font_id id() const;
    public:
        bool has_fallback() const;
        font fallback() const;
        // operations
    public:
        i_string const& family_name() const override;
        font_style style() const override;
        i_string const& style_name() const override;
        point_size size() const override;
        dimension height() const;
        dimension ascender() const;
        dimension descender() const;
        dimension line_spacing() const;
        using font_info::kerning;
        dimension kerning(uint32_t aLeftGlyphIndex, uint32_t aRightGlyphIndex) const;
        bool is_bitmap_font() const;
        uint32_t num_fixed_sizes() const;
        point_size fixed_size(uint32_t aFixedSizeIndex) const;
    public:
        const i_glyph_texture& glyph_texture(const glyph& aGlyph) const;
    public:
        bool operator==(const font& aRhs) const;
        bool operator!=(const font& aRhs) const;
        bool operator<(const font& aRhs) const;
    public:
        i_native_font_face& native_font_face() const;
        // attributes
    private:
        mutable std::shared_ptr<instance> iInstance;
    };

    typedef optional<font> optional_font;

    template <typename Elem, typename Traits>
    inline std::basic_ostream<Elem, Traits>& operator<<(std::basic_ostream<Elem, Traits>& aStream, const font_info& aFont)
    {
        aStream << "[";
        aStream << aFont.family_name();
        aStream << ",";
        if (aFont.style_available())
            aStream << aFont.style();
        else
            aStream << aFont.style_name();
        aStream << ", ";
        aStream << aFont.size();
        aStream << ", ";
        aStream << aFont.underline();
        aStream << ", ";
        aStream << aFont.kerning();
        aStream << "]";
        return aStream;
    }

    template <typename Elem, typename Traits>
    inline std::basic_istream<Elem, Traits>& operator>>(std::basic_istream<Elem, Traits>& aStream, font_info& aFont)
    {
        std::string familyName;
        std::variant<font_style, std::string> style;
        font_info::point_size size;
        bool underline;
        bool kerning;

        auto previousImbued = aStream.getloc();
        if (typeid(std::use_facet<std::ctype<char>>(previousImbued)) != typeid(neolib::comma_only_whitespace))
            aStream.imbue(std::locale{ previousImbued, new neolib::comma_only_whitespace{} });
        char ignore;
        aStream >> ignore;
        aStream >> familyName;
        std::string string;
        aStream >> string;
        try
        {
            style = neolib::string_to_enum<font_style>(string);
        }
        catch (...)
        {
            style = string;
        }
        aStream.imbue(std::locale{ previousImbued, new neolib::comma_as_whitespace{} });
        aStream >> size;
        aStream >> underline;
        aStream >> kerning;
        aStream >> ignore;
        aStream.imbue(previousImbued);

        if (std::holds_alternative<font_style>(style))
            aFont = font_info{ familyName, std::get<font_style>(style), size };
        else
            aFont = font_info{ familyName, std::get<std::string>(style), size };
        aFont.set_underline(underline);
        if (kerning)
            aFont.enable_kerning();
        else
            aFont.disable_kerning();

        return aStream;
    }
}

define_setting_type_as(neogfx::font_info, neogfx::font)
