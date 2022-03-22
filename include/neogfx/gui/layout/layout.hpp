// layout.hpp
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
#include <list>
#include <boost/pool/pool_alloc.hpp>
#include <neolib/core/allocator.hpp>
#include <neolib/core/variant.hpp>
#include <neogfx/core/object.hpp>
#include <neogfx/core/units.hpp>
#include <neogfx/gui/layout/layout_item.hpp>
#include <neogfx/gui/layout/layout_item_cache.hpp>
#include <neogfx/gui/layout/anchor.hpp>
#include <neogfx/gui/layout/anchorable.hpp>
#include <neogfx/gui/layout/i_layout.hpp>

namespace neogfx
{
    class i_spacer;

    class layout : public layout_item<object<i_layout>>
    {
        typedef layout_item<object<i_layout>> base_type;
    public:
        define_declared_event(LayoutCompleted, layout_completed)
        define_declared_event(AlignmentChanged, alignment_changed)
    public:
        struct item_already_added : std::logic_error { item_already_added() : std::logic_error("neogfx::layout::item_already_added") {} };
        struct widget_is_floating : std::logic_error { widget_is_floating() : std::logic_error("neogfx::layout::widget_is_floating") {} };
    public:
        typedef i_layout abstract_type;
    protected:
        typedef layout_item_cache item;
        typedef std::list<item, neolib::fast_pool_allocator<item>> item_list;
        enum item_type_e
        {
            ItemTypeNone = 0x00,
            ItemTypeWidget = 0x01,
            ItemTypeLayout = 0x02,
            ItemTypeSpacer = 0x04
        };
    protected:
        template <typename SpecializedPolicy>
        struct common_axis_policy;
        template <typename Layout>
        struct column_major;
        template <typename Layout>
        struct row_major;
    public:
        layout(neogfx::alignment aAlignment = neogfx::alignment::Center | neogfx::alignment::VCenter);
        layout(i_widget& aOwner, neogfx::alignment aAlignment = neogfx::alignment::Center | neogfx::alignment::VCenter);
        layout(i_layout& aParent, neogfx::alignment aAlignment = neogfx::alignment::Center | neogfx::alignment::VCenter);
        layout(const layout&) = delete;
        ~layout();
    public:
        bool is_layout() const override;
        const i_layout& as_layout() const override;
        i_layout& as_layout() override;
        bool is_spacer() const override;
        const i_spacer& as_spacer() const override;
        i_spacer& as_spacer() override;
        bool is_widget() const override;
        const i_widget& as_widget() const override;
        i_widget& as_widget() override;
    public:
        bool has_parent_layout() const override;
        const i_layout& parent_layout() const override;
        i_layout& parent_layout() override;
        void set_parent_layout(i_layout* aParentLayout) override;
        bool has_layout_owner() const override;
        const i_widget& layout_owner() const override;
        i_widget& layout_owner() override;
        void set_layout_owner(i_widget* aOwner) override;
    public:
        i_layout_item& add(i_layout_item& aItem) override;
        i_layout_item& add_at(layout_item_index aPosition, i_layout_item& aItem) override;
        i_layout_item& add(i_ref_ptr<i_layout_item> const& aItem) override;
        i_layout_item& add_at(layout_item_index aPosition, i_ref_ptr<i_layout_item> const& aItem) override;
        void remove_at(layout_item_index aIndex) override;
        bool remove(i_layout_item& aItem) override;
        void remove_all() override;
        void move_all_to(i_layout& aDestination) override;
        layout_item_index count() const override;
        layout_item_index index_of(const i_layout_item& aItem) const override;
        optional_layout_item_index find(const i_layout_item& aItem) const override;
        bool is_widget_at(layout_item_index aIndex) const override;
        const i_layout_item& item_at(layout_item_index aIndex) const override;
        i_layout_item& item_at(layout_item_index aIndex) override;
        using i_layout::get_widget_at;
        const i_widget& get_widget_at(layout_item_index aIndex) const override;
        i_widget& get_widget_at(layout_item_index aIndex) override;
        const i_layout& get_layout_at(layout_item_index aIndex) const override;
        i_layout& get_layout_at(layout_item_index aIndex) override;
    public:
        bool high_dpi() const override;
        dimension dpi_scale_factor() const override;
    public:
        neogfx::padding padding() const override;
    public:
        layout_direction direction() const override;
        bool has_spacing() const override;
        size spacing() const override;
        void set_spacing(optional_size const& aSpacing, bool aUpdateLayout = true) override;
        bool always_use_spacing() const override;
        void set_always_use_spacing(bool aAlwaysUseSpacing) override;
        neogfx::alignment alignment() const override;
        void set_alignment(neogfx::alignment aAlignment, bool aUpdateLayout = true) override;
        neogfx::autoscale autoscale() const override;
        void set_autoscale(neogfx::autoscale aAutoscale, bool aUpdateLayout = true) override;
        bool ignore_visibility() const override;
        void set_ignore_visibility(bool aIgnoreVisibility, bool aUpdateLayout = true) override;
    public:
        using i_layout::enable;
        void enable(bool aEnable) override;
        bool enabled() const override;
        bool invalidated() const override;
        void invalidate(bool aDeferLayout = true) override;
        void validate() override;
    public:
        void set_extents(const size& aExtents) override;
        neogfx::size_policy size_policy() const override;
        void set_minimum_size(optional_size const& aMinimumSize, bool aUpdateLayout = true) override;
        void set_maximum_size(optional_size const& aMaximumSize, bool aUpdateLayout = true) override;
    public:
        bool device_metrics_available() const override;
        const i_device_metrics& device_metrics() const override;
    public:
        void layout_as(const point& aPosition, const size& aSize) override;
    public:
        void fix_weightings(bool aRecalculate = true) override;
    protected:
        void layout_item_enabled(i_layout_item& aItem) override;
        void layout_item_disabled(i_layout_item& aItem) override;
    public:
        bool visible() const override;
    protected:
        item_list::const_iterator cbegin() const;
        item_list::const_iterator cend() const;
        item_list::const_iterator begin() const;
        item_list::const_iterator end() const;
        item_list::iterator begin();
        item_list::iterator end();
        item_list::const_reverse_iterator rbegin() const;
        item_list::const_reverse_iterator rend() const;
        item_list::reverse_iterator rbegin();
        item_list::reverse_iterator rend();
        item_list::const_iterator find_item(const i_layout_item& aItem) const;
        item_list::iterator find_item(i_layout_item& aItem);
        const item_list& items() const;
        item_list& items();
        virtual void remove(item_list::iterator aItem);
        uint32_t spacer_count() const;
        uint32_t items_visible(item_type_e aItemType = static_cast<item_type_e>(ItemTypeWidget|ItemTypeLayout)) const;
        template <typename AxisPolicy>
        size do_minimum_size(optional_size const& aAvailableSpace) const;
        template <typename AxisPolicy>
        size do_maximum_size(optional_size const& aAvailableSpace) const;
        template <typename AxisPolicy>
        void do_layout_items(const point& aPosition, const size& aSize);
        // helpers
    public:
        using i_layout::add;
        using i_layout::add_at;
    private:
        i_layout* iParent;
        mutable i_widget* iOwner;
        mutable std::optional<const i_device_metrics*> iDeviceMetrics;
        optional_size iSpacing;
        bool iAlwaysUseSpacing;
        neogfx::alignment iAlignment;
        neogfx::autoscale iAutoscale;
        bool iIgnoreVisibility;
        bool iEnabled;
        item_list iItems;
        bool iLayoutStarted;
        bool iInvalidated;
    };
}
