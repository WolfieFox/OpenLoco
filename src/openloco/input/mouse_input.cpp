#include "../audio/audio.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../localisation/string_ids.h"
#include "../ui/scrollview.h"
#include "../window.h"
#include "../windowmgr.h"
#include <map>

using namespace openloco::interop;
using namespace openloco::ui;

namespace openloco::input
{

    static void state_resizing(mouse_button button, int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, ui::widget_index widgetIndex);
    static void state_normal(mouse_button state, int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, ui::widget_index widgetIndex);
    static void state_normal_hover(int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, ui::widget_index widgetIndex);
    static void state_normal_left(int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, ui::widget_index widgetIndex);
    static void state_normal_right(int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, ui::widget_index widgetIndex);
    static void state_positioning_window(mouse_button button, int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, ui::widget_index widgetIndex);

    static void window_position_begin(int16_t x, int16_t y, ui::window* window, ui::widget_index widget_index);
    static void window_position_end();

    static void window_resize_begin(int16_t x, int16_t y, ui::window* window, ui::widget_index widget_index);

    static void viewport_drag_begin(window* w);

    static void scroll_begin(int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, int8_t widgetIndex);

    static void scroll_drag_begin(int16_t x, int16_t y, window* pWindow, widget_index index);

    static void widget_over_flatbutton_invalidate();

#pragma mark - Input

    loco_global<uint16_t, 0x0050C19C> time_since_last_tick;

    static loco_global<ui::window_type, 0x0052336F> _pressedWindowType;
    static loco_global<ui::window_number, 0x00523370> _pressedWindowNumber;
    static loco_global<uint32_t, 0x00523372> _pressedWidgetIndex;
    static loco_global<uint16_t, 0x00523376> _clickRepeatTicks;
    static loco_global<uint16_t, 0x00523378> _dragLastX;
    static loco_global<uint16_t, 0x0052337A> _dragLastY;
    static loco_global<ui::window_number, 0x0052337C> _dragWindowNumber;
    static loco_global<ui::window_type, 0x0052337E> _dragWindowType;
    static loco_global<uint8_t, 0x0052337F> _dragWidgetIndex;
    static loco_global<uint8_t, 0x00523380> _dragScrollIndex;
    static loco_global<ui::window_type, 0x00523381> _tooltipWindowType;
    static loco_global<int16_t, 0x00523382> _tooltipWindowNumber;
    static loco_global<int16_t, 0x00523384> _tooltipWidgetIndex;
    static loco_global<uint16_t, 0x00523386> _tooltipCursorX;
    static loco_global<uint16_t, 0x00523388> _tooltipCursorY;
    static loco_global<uint16_t, 0x0052338A> _tooltipTimeout;
    static loco_global<uint16_t, 0x0052338C> _tooltipNotShownTicks;
    static loco_global<uint16_t, 0x0052338E> _ticksSinceDragStart;
    static loco_global<ui::window_number, 0x00523390> _toolWindowNumber;
    static loco_global<ui::window_type, 0x00523392> _toolWindowType;

    loco_global<int16_t, 0x00523394> _toolWidgetIndex;

    static loco_global<uint16_t, 0x00523396> _currentScrollArea;
    static loco_global<uint32_t, 0x00523398> _currentScrollOffset;

    static loco_global<ui::window_type, 0x005233A8> _hoverWindowType;
    static uint8_t _5233A9;
    static loco_global<ui::window_number, 0x005233AA> _hoverWindowNumber;
    static loco_global<uint16_t, 0x005233AC> _hoverWidgetIdx;
    static loco_global<uint32_t, 0x005233AE> _5233AE;
    static loco_global<uint32_t, 0x005233B2> _5233B2;
    static loco_global<ui::window_type, 0x005233B6> _modalWindowType;

    static loco_global<int32_t, 0x01136F98> _currentTooltipStringId;

    static std::map<ui::scrollview::scroll_part, string_id> scroll_widget_tooltips = {
        { ui::scrollview::scroll_part::hscrollbar_button_left, string_ids::tooltip_scroll_left },
        { ui::scrollview::scroll_part::hscrollbar_button_right, string_ids::tooltip_scroll_right },
        { ui::scrollview::scroll_part::hscrollbar_track_left, string_ids::tooltip_scroll_left_fast },
        { ui::scrollview::scroll_part::hscrollbar_track_right, string_ids::tooltip_scroll_right_fast },
        { ui::scrollview::scroll_part::hscrollbar_thumb, string_ids::tooltip_scroll_left_right },
        { ui::scrollview::scroll_part::vscrollbar_button_top, string_ids::tooltip_scroll_up },
        { ui::scrollview::scroll_part::vscrollbar_button_bottom, string_ids::tooltip_scroll_down },
        { ui::scrollview::scroll_part::vscrollbar_track_top, string_ids::tooltip_scroll_up_fast },
        { ui::scrollview::scroll_part::vscrollbar_track_bottom, string_ids::tooltip_scroll_down_fast },
        { ui::scrollview::scroll_part::vscrollbar_thumb, string_ids::tooltip_scroll_up_down },
    };

#pragma mark - Mouse input

    // 0x004C7174
    void handle_mouse(int16_t x, int16_t y, mouse_button button)
    {
        loco_global<uint16_t, 0x001136fa0>() = (uint16_t)button;

        ui::window* window = windowmgr::find_at(x, y);

        ui::widget_index widgetIndex = -1;
        if (window != nullptr)
        {
            widgetIndex = window->find_widget_at(x, y);
        }

        if (*_modalWindowType != ui::window_type::undefined)
        {
            if (window != nullptr)
            {
                if (window->type != *_modalWindowType)
                {
                    if (button == mouse_button::left_pressed)
                    {
                        windowmgr::bring_to_front(_modalWindowType, 0);
                        audio::play_sound(audio::sound_id::sound_14, x);
                    }

                    if (button == mouse_button::right_pressed)
                    {
                        return;
                    }
                }
            }
        }

        ui::widget_t* widget = nullptr;
        if (widgetIndex != -1)
        {
            widget = &window->widgets[widgetIndex];
        }

        registers regs;
        regs.ebp = (int32_t)state();
        regs.esi = (uint32_t)window;
        regs.edx = widgetIndex;
        regs.edi = (uint32_t)widget;
        regs.cx = (uint16_t)button;
        regs.ax = x;
        regs.bx = y;
        switch (state())
        {
            case input_state::reset:
                _tooltipCursorX = x;
                _tooltipCursorY = y;
                _tooltipTimeout = 0;
                _tooltipWindowType = ui::window_type::undefined;
                state(input_state::normal);
                reset_flag(input_flags::flag4);
                state_normal(button, x, y, window, widget, widgetIndex);
                break;

            case input_state::normal:
                state_normal(button, x, y, window, widget, widgetIndex);
                break;

            case input_state::widget_pressed:
            case input_state::dropdown_active:
                call(0x004C7AE7, regs);
                break;

            case input_state::positioning_window:
                state_positioning_window(button, x, y, window, widget, widgetIndex);
                break;

            case input_state::viewport_right:
                call(0x004C74BB, regs);
                break;

            case input_state::viewport_left:
                call(0x004C7334, regs);
                break;

            case input_state::scroll_left:
                call(0x004C71F6, regs);
                break;

            case input_state::resizing:
                state_resizing(button, x, y, window, widget, widgetIndex);
                break;

            case input_state::scroll_right:
                call(0x004C76A7, regs);
                break;
        }
    }

    // 0x004C7722
    static void state_resizing(mouse_button button, int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, ui::widget_index widgetIndex)
    {
        auto w = windowmgr::find(_dragWindowType, _dragWindowNumber);
        if (w == nullptr)
        {
            state(input_state::reset);
            return;
        }

        bool doDefault = false;
        int dx = 0, dy = 0;
        switch (button)
        {
            case mouse_button::released:
                doDefault = true;
                break;

            case mouse_button::left_released:
                state(input_state::normal);
                _tooltipTimeout = 0;
                _tooltipWidgetIndex = _pressedWidgetIndex;
                _tooltipWindowType = _dragWindowType;
                _tooltipWindowNumber = _dragWindowNumber;

                if (w->flags & ui::window_flags::flag_15)
                {
                    doDefault = true;
                    break;
                }

                if (w->flags & ui::window_flags::flag_16)
                {
                    x = window->var_88A - window->width + _dragLastX;
                    y = window->var_88C - window->height + _dragLastY;
                    w->flags &= ~ui::window_flags::flag_16;
                    doDefault = true;
                    break;
                }

                window->var_88A = window->width;
                window->var_88C = window->height;
                x = _dragLastX - window->x - window->width + ui::width();
                y = _dragLastY - window->y - window->height + ui::height() - 27;
                w->flags |= ui::window_flags::flag_16;
                if (y >= ui::height() - 2)
                {
                    _dragLastX = x;
                    _dragLastY = y;
                    return;
                }

                dx = x - _dragLastX;
                dy = y - _dragLastY;

                if (dx == 0 && dy == 0)
                {
                    _dragLastX = x;
                    _dragLastY = y;
                    return;
                }

                break;

            default:
                return;
        }

        if (doDefault)
        {
            if (y >= ui::height() - 2)
            {
                _dragLastX = x;
                _dragLastY = y;
                return;
            }

            dx = x - _dragLastX;
            dy = y - _dragLastY;

            if (dx == 0 && dy == 0)
            {
                _dragLastX = x;
                _dragLastY = y;
                return;
            }

            w->flags &= ~ui::window_flags::flag_16;
        }

        w->invalidate();

        w->width = std::clamp<uint16_t>(w->width + dx, w->min_width, w->max_width);
        w->height = std::clamp<uint16_t>(w->height + dy, w->min_height, w->max_height);
        w->flags |= ui::window_flags::flag_15;
        w->call_on_resize();
        w->call_prepare_draw();
        w->scroll_areas[0].h_right = -1;
        w->scroll_areas[0].v_bottom = -1;
        w->scroll_areas[1].h_right = -1;
        w->scroll_areas[1].v_bottom = -1;
        window->update_scroll_widgets();
        w->invalidate();

        _dragLastX = x;
        _dragLastY = y;
    }

    // 0x004C7903
    static void state_positioning_window(mouse_button button, int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, ui::widget_index widgetIndex)
    {
        auto w = windowmgr::find(_dragWindowType, _dragWindowNumber);
        if (w == nullptr)
        {
            state(input_state::reset);
            return;
        }

        switch (button)
        {
            case mouse_button::released:
            {
                y = std::clamp<int16_t>(y, 29, ui::height() - 29);

                int16_t dx = x - _dragLastX;
                int16_t dy = y - _dragLastY;

                _5233A9 = window->move(dx, dy);

                _dragLastX = x;
                _dragLastY = y;
                break;
            }

            case mouse_button::left_released:
            {
                window_position_end();

                y = std::clamp<int16_t>(y, 29, ui::height() - 29);

                int dx = x - _dragLastX;
                int dy = y - _dragLastY;
                _5233A9 = window->move(dx, dy);

                _dragLastX = x;
                _dragLastY = y;

                if (_5233A9 == false)
                {
                    auto dragWindow = windowmgr::find(_dragWindowType, _dragWindowNumber);
                    if (dragWindow != nullptr)
                    {
                        if (dragWindow->is_enabled(_pressedWidgetIndex))
                        {
                            auto pressedWidget = &dragWindow->widgets[_pressedWidgetIndex];

                            audio::play_sound(audio::sound_id::sound_2, dragWindow->x + pressedWidget->mid_x());
                            dragWindow->call_on_mouse_up(_pressedWidgetIndex);
                        }
                    }
                }

                w->call_on_move(_dragLastX, _dragLastY);
            }
            break;

            default:
                break;
        }
    }

    // 0x004C8048
    static void state_normal(mouse_button state, int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, ui::widget_index widgetIndex)
    {
        switch (state)
        {
            case mouse_button::left_pressed:
                state_normal_left(x, y, window, widget, widgetIndex);
                break;
            case mouse_button::right_pressed:
                state_normal_right(x, y, window, widget, widgetIndex);
                break;
            case mouse_button::released:
                state_normal_hover(x, y, window, widget, widgetIndex);
                break;

            default:
                break;
        }
    }

    // 0x004C8098
    static void state_normal_hover(int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, ui::widget_index widgetIndex)
    {
        ui::window_type windowType = ui::window_type::undefined;
        ui::window_number windowNumber = 0;

        if (window != nullptr)
        {
            windowType = window->type;
            windowNumber = window->number;
        }

        if (windowType != *_hoverWindowType || windowNumber != *_hoverWindowNumber || widgetIndex != *_hoverWidgetIdx)
        {
            widget_over_flatbutton_invalidate();
            _hoverWindowType = window->type;
            _hoverWindowNumber = window->number;
            _hoverWidgetIdx = widgetIndex;
            widget_over_flatbutton_invalidate();
        }

        if (window != nullptr && widgetIndex != -1)
        {
            if (!window->is_disabled(widgetIndex))
            {
                window->call_3(widgetIndex);
            }
        }

        string_id tooltipStringId = string_ids::null;
        if (window != nullptr && widgetIndex != -1)
        {
            if (widget->type == ui::widget_type::scrollview)
            {
                ui::scrollview::scroll_part scrollArea;
                int16_t scrollX, scrollY;
                int32_t scrollIndex;
                ui::scrollview::get_part(window, widget, x, y, &scrollX, &scrollY, &scrollArea, &scrollIndex);

                if (scrollArea == ui::scrollview::scroll_part::none)
                {
                }
                else if (scrollArea == ui::scrollview::scroll_part::view)
                {
                    window->call_scroll_mouse_over(scrollX, scrollY, scrollIndex / sizeof(ui::scroll_area_t));
                }
                else
                {
                    tooltipStringId = scroll_widget_tooltips[scrollArea];
                    if (*_tooltipWindowType != ui::window_type::undefined)
                    {
                        if (tooltipStringId != _currentTooltipStringId)
                        {
                            call(0x4C87B5);
                        }
                    }
                }
            }
        }

        if (*_tooltipWindowType != ui::window_type::undefined)
        {
            if (*_tooltipWindowType == window->type && _tooltipWindowNumber == window->number && _tooltipWidgetIndex == widgetIndex)
            {
                _tooltipTimeout += time_since_last_tick;
                if (_tooltipTimeout >= 8000)
                {
                    windowmgr::close(ui::window_type::tooltip);
                }
            }
            else
            {
                call(0x4C87B5);
            }

            return;
        }

        if (_tooltipNotShownTicks < 500 || (x == _tooltipCursorX && y == _tooltipCursorY))
        {
            _tooltipTimeout += time_since_last_tick;
            int bp = 2000;
            if (_tooltipNotShownTicks <= 1000)
            {
                bp = 0;
            }

            if (bp > _tooltipTimeout)
            {
                return;
            }

            if (tooltipStringId == string_ids::null)
            {
                ui::tooltip::open(window, widgetIndex, x, y);
            }
            else
            {
                ui::tooltip::update(window, widgetIndex, tooltipStringId, x, y);
            }
        }

        _tooltipTimeout = 0;
        _tooltipCursorX = x;
        _tooltipCursorY = y;
    }

    // 0x004C84BE
    static void state_normal_left(int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, ui::widget_index widgetIndex)
    {
        ui::window_type windowType = ui::window_type::undefined;
        ui::window_number windowNumber = 0;

        if (window != nullptr)
        {
            windowType = window->type;
            windowNumber = window->number;
        }

        windowmgr::close(ui::window_type::error);
        windowmgr::close(ui::window_type::tooltip);

        // Window might have changed position in the list, therefore find it again
        window = windowmgr::find(windowType, windowNumber);
        if (window == nullptr)
        {
            return;
        }

        window = windowmgr::bring_to_front(window);
        if (widgetIndex == -1)
        {
            return;
        }

        switch (widget->type)
        {
            case ui::widget_type::caption_22:
            case ui::widget_type::caption_23:
            case ui::widget_type::caption_24:
            case ui::widget_type::caption_25:
                window_position_begin(x, y, window, widgetIndex);
                break;

            case ui::widget_type::panel:
            case ui::widget_type::frame:
                if (window->can_resize() && (x >= (window->x + window->width - 19)) && (y >= (window->y + window->height - 19)))
                {
                    window_resize_begin(x, y, window, widgetIndex);
                }
                else
                {
                    window_position_begin(x, y, window, widgetIndex);
                }
                break;

            case ui::widget_type::viewport:
                state(input_state::viewport_left);
                _dragLastX = x;
                _dragLastY = y;
                _dragWindowType = window->type;
                _dragWindowNumber = window->number;
                if (has_flag(input_flags::tool_active))
                {
                    auto w = windowmgr::find(_toolWindowType, _toolWindowNumber);
                    if (w != nullptr)
                    {
                        w->call_tool_down(_toolWidgetIndex, x, y);
                        set_flag(input_flags::flag4);
                    }
                }
                break;

            case ui::widget_type::scrollview:
                state(input_state::scroll_left);
                _pressedWidgetIndex = widgetIndex;
                _pressedWindowType = window->type;
                _pressedWindowNumber = window->number;
                _tooltipCursorX = x;
                _tooltipCursorY = y;
                scroll_begin(x, y, window, widget, widgetIndex);
                break;

            default:
                if (window->is_enabled(widgetIndex) && !window->is_disabled(widgetIndex))
                {
                    audio::play_sound(audio::sound_id::click_1, window->x + widget->mid_x());

                    // Set new cursor down widget
                    _pressedWidgetIndex = widgetIndex;
                    _pressedWindowType = window->type;
                    _pressedWindowNumber = window->number;
                    set_flag(input_flags::widget_pressed);
                    state(input_state::widget_pressed);
                    _clickRepeatTicks = 1;

                    windowmgr::invalidate_widget(window->type, window->number, widgetIndex);
                    window->call_on_mouse_down(widgetIndex);
                }

                break;
        }
    }

    // 0x004C834A
    static void state_normal_right(int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, ui::widget_index widgetIndex)
    {
        ui::window_type windowType = ui::window_type::undefined;
        ui::window_number windowNumber = 0;

        if (window != nullptr)
        {
            windowType = window->type;
            windowNumber = window->number;
        }

        windowmgr::close(ui::window_type::tooltip);

        // Window might have changed position in the list, therefore find it again
        window = windowmgr::find(windowType, windowNumber);
        if (window == nullptr)
        {
            return;
        }

        window = windowmgr::bring_to_front(window);

        if (widgetIndex == -1)
        {
            return;
        }

        if (*_modalWindowType != ui::window_type::undefined)
        {
            if (*_modalWindowType == window->type)
            {
                scroll_begin(x, y, window, widget, widgetIndex);
            }

            return;
        }

        if (is_title_mode())
        {
            return;
        }

        switch (widget->type)
        {
            default:
                break;

            case ui::widget_type::viewport:
                viewport_drag_begin(window);

                _dragLastX = x;
                _dragLastY = y;

                ui::hide_cursor();
                sub_407218();

                _5233AE = 0;
                _5233B2 = 0;
                set_flag(input_flags::flag5);
                break;

            case ui::widget_type::scrollview:
                scroll_drag_begin(x, y, window, widgetIndex);

                _5233AE = 0;
                _5233B2 = 0;
                set_flag(input_flags::flag5);
                break;
        }
    }

#pragma mark - Window positioning

    // 0x00C877D
    static void window_position_begin(int16_t x, int16_t y, ui::window* window, ui::widget_index widget_index)
    {
        state(input_state::positioning_window);
        _pressedWidgetIndex = widget_index;
        _dragLastX = x;
        _dragLastY = y;
        _dragWindowType = window->type;
        _dragWindowNumber = window->number;
        _5233A9 = false;
    }

    static void window_position_end()
    {
        state(input_state::normal);
        _tooltipTimeout = 0;
        _tooltipWidgetIndex = _pressedWidgetIndex;
        _tooltipWindowType = _dragWindowType;
        _tooltipWindowNumber = _dragWindowNumber;
    }

#pragma mark - Window resizing

    // 0x004C85D1
    static void window_resize_begin(int16_t x, int16_t y, ui::window* window, ui::widget_index widget_index)
    {
        state(input_state::resizing);
        _pressedWidgetIndex = widget_index;
        _dragLastX = x;
        _dragLastY = y;
        _dragWindowType = window->type;
        _dragWindowNumber = window->number;
        window->flags &= ~ui::window_flags::flag_15;
    }

#pragma mark - Viewport dragging

    static void viewport_drag_begin(window* w)
    {
        w->flags &= ~ui::window_flags::scrolling_to_location;
        state(input_state::viewport_right);
        _dragWindowType = w->type;
        _dragWindowNumber = w->number;
        _ticksSinceDragStart = 0;
    }

#pragma mark - Scroll bars

    // 0x004C8689
    void scroll_begin(int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, int8_t widgetIndex)
    {
        ui::scrollview::scroll_part scrollArea;
        int16_t outX, outY;
        int32_t scrollAreaOffset;

        ui::scrollview::get_part(window, widget, x, y, &outX, &outY, &scrollArea, &scrollAreaOffset);

        _currentScrollArea = (uint16_t)scrollArea;
        _currentScrollOffset = scrollAreaOffset;

        if (scrollArea == ui::scrollview::scroll_part::view)
        {
            window->call_scroll_mouse_down(outX, outY, scrollAreaOffset / sizeof(ui::scroll_area_t));
            return;
        }

        // Not implemented for any window
        // window->call_22()

        registers regs;
        regs.eax = outX;
        regs.ebx = outY;
        regs.cx = (int16_t)scrollArea;
        regs.edx = scrollAreaOffset;

        switch (scrollArea)
        {
            case ui::scrollview::scroll_part::hscrollbar_button_left:
                call(0x4c894f, regs);
                break;
            case ui::scrollview::scroll_part::hscrollbar_button_right:
                call(0x4c89ae, regs);
                break;
            case ui::scrollview::scroll_part::hscrollbar_track_left:
                call(0x4c8a36, regs);
                break;
            case ui::scrollview::scroll_part::hscrollbar_track_right:
                call(0x4c8aa6, regs);
                break;

            case ui::scrollview::scroll_part::vscrollbar_button_top:
                call(0x4c8b26, regs);
                break;
            case ui::scrollview::scroll_part::vscrollbar_button_bottom:
                call(0x4c8b85, regs);
                break;
            case ui::scrollview::scroll_part::vscrollbar_track_top:
                call(0x4c8c0d, regs);
                break;
            case ui::scrollview::scroll_part::vscrollbar_track_bottom:
                call(0x4c8c7d, regs);
                break;

            default:
                break;
        }
    }

#pragma mark - Scrollview dragging

    static void scroll_drag_begin(int16_t x, int16_t y, ui::window* window, ui::widget_index widgetIndex)
    {
        state(input_state::scroll_right);
        _dragLastX = x;
        _dragLastY = y;
        _dragWindowType = window->type;
        _dragWindowNumber = window->number;
        _dragWidgetIndex = widgetIndex;
        _ticksSinceDragStart = 0;

        _dragScrollIndex = window->get_scroll_data_index(widgetIndex);

        ui::hide_cursor();
        sub_407218();
    }

#pragma mark Widgets

    static void widget_over_flatbutton_invalidate()
    {
        ui::window_type windowType = _hoverWindowType;
        uint16_t widgetIdx = _hoverWidgetIdx;
        uint16_t windowNumber = _hoverWindowNumber;

        if (windowType == ui::window_type::undefined)
        {
            windowmgr::invalidate_widget(windowType, windowNumber, widgetIdx);
            return;
        }

        ui::window* oldWindow = windowmgr::find(windowType, windowNumber);

        if (oldWindow != nullptr)
        {
            oldWindow->call_prepare_draw();

            ui::widget_t* oldWidget = &oldWindow->widgets[widgetIdx];
            if (
                oldWidget->type == ui::widget_type::wt_16 || oldWidget->type == ui::widget_type::wt_10 || oldWidget->type == ui::widget_type::wt_9)
            {

                windowmgr::invalidate_widget(windowType, windowNumber, widgetIdx);
            }
        }
    }
}
