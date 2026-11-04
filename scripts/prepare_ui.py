"""Reproducible, display-only patches for the pinned 50D Nordic interface."""
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent

def replace_once(text,before,after):
    if text.count(before)!=1:raise ValueError('UI patch anchor changed: '+before[:90])
    return text.replace(before,after,1)

def menu_overlay(menu):
    menu='#include "nordic.h"\n'+menu
    menu=replace_once(menu,'static int menu_shown = false;',
        'static int menu_shown = false;\nstatic struct cd_ui_pages nordic_pages;\n'
        'static int nordic_status_visible(void);')
    menu=replace_once(menu,'    menu_shown = 1;\n    //~ menu_hidden_should_display_help = 0;',
        '    menu_shown = 1;\n    cd_ui_pages_open(&nordic_pages,camdesk_ui_enabled());\n    //~ menu_hidden_should_display_help = 0;')
    marker='static EXCLUDES(menu_sem)\nvoid menus_display('
    helper='''static int nordic_status_visible(void)
{
    struct menu *selected=get_selected_toplevel_menu();
    int home=selected && !submenu_level && !edit_mode && streq(selected->name,"Home");
    return cd_ui_pages_status(&nordic_pages,camdesk_ui_enabled(),home);
}

'''
    menu=replace_once(menu,marker,helper+marker)
    menu=replace_once(menu,'    g_submenu_width = 720;\n\n    if (duplicate_check_dirty)',
        '    if(nordic_status_visible()){menu_lv_transparent_mode=0;camdesk_ui_draw_dashboard();return;}\n'
        '    g_submenu_width = 720;\n\n    if (duplicate_check_dirty)')
    menu=replace_once(menu,'    if (handle_rack_focus_menu_overrides(event)==0)',
        '    if (!nordic_status_visible() && handle_rack_focus_menu_overrides(event)==0)')
    key_anchor='    // Find the selected menu or submenu (should be cached?)'
    key_hook='''    if(nordic_status_visible())
    {
        int key=event->param;
        int wheel=key==BGMT_WHEEL_UP || key==BGMT_WHEEL_DOWN || key==BGMT_WHEEL_LEFT || key==BGMT_WHEEL_RIGHT;
        int close=key==BGMT_PRESS_HALFSHUTTER || key==BGMT_TRASH || key==BGMT_MENU;
        struct menu *selected=get_selected_toplevel_menu();
        int home=selected && !submenu_level && !edit_mode && streq(selected->name,"Home");
        int action=cd_ui_pages_key(&nordic_pages,camdesk_ui_enabled(),home,
            wheel?CD_PAGE_WHEEL:close?CD_PAGE_CLOSE:CD_PAGE_OTHER);
        if(action==CD_PAGE_TO_MENUS){
            submenu_level=edit_mode=menu_help_active=menu_lv_transparent_mode=0;
            select_menu_by_name("Expo",0);
            menu_redraw();return 0;
        }
        if(action==CD_PAGE_CONSUMED)return 0;
        if(key==BGMT_MENU){gui_stop_menu();return 0;}
    }

'''
    menu=replace_once(menu,key_anchor,key_hook+key_anchor)
    menu=replace_once(menu,'    menu_find_by_name( "Audio",     ICON_ML_AUDIO   );',
        '    menu_find_by_name( "Home",      \'H\' );\n    menu_find_by_name( "Audio",     ICON_ML_AUDIO   );')
    menu=replace_once(menu,'#define MENU_BG_COLOR_HEADER_FOOTER 42','#define MENU_BG_COLOR_HEADER_FOOTER (camdesk_ui_enabled() ? CD_BG : 42)')
    menu=replace_once(menu,'#define MENU_BAR_COLOR (bmp_color_scheme ? COLOR_LIGHT_BLUE : COLOR_BLUE)',
        '#define MENU_BAR_COLOR (camdesk_ui_enabled() ? CD_PANEL : (bmp_color_scheme ? COLOR_LIGHT_BLUE : COLOR_BLUE))')
    # Native menu keys, actions, scrolling and warning logic remain in charge.
    menu=replace_once(menu,'        int color_left = 45;','        int color_left = camdesk_ui_enabled() ? CD_PANEL : 45;')
    menu=replace_once(menu,'        selection_bar_backend(color_right, COLOR_BLACK, xc, y, x_end-xc, h-1);',
        '        selection_bar_backend(color_right, COLOR_BLACK, xc, y, x_end-xc, h-1);\n'
        '        if(camdesk_ui_enabled() && !customize_mode) bmp_fill(CD_ACCENT, xl-5, y+3, 3, h-7);')
    menu=replace_once(menu,'    int color_on = warn ? COLOR_DARK_GREEN1_MOD : COLOR_GREEN1;',
        '    int color_on = camdesk_ui_enabled() ? CD_TEXT : (warn ? COLOR_DARK_GREEN1_MOD : COLOR_GREEN1);')
    menu=replace_once(menu,'    int color_dis = warn ? 50 : COLOR_RED;',
        '    int color_dis = camdesk_ui_enabled() ? CD_LINE : (warn ? 50 : COLOR_RED);')
    menu=replace_once(menu,'    int color_slider_fg = warn ? COLOR_DARK_CYAN1_MOD : COLOR_CYAN;',
        '    int color_slider_fg = camdesk_ui_enabled() ? CD_ACCENT : (warn ? COLOR_DARK_CYAN1_MOD : COLOR_CYAN);')
    menu=replace_once(menu,'    int x = orig_x + junkie_mode ? 2 : 150;\n    int icon_spacing = junkie_mode ? 716 / num_tabs : (720 - 150) / num_tabs;',
        '    int nordic_tabs = camdesk_ui_enabled() && !junkie_mode;\n'
        '    int x = nordic_tabs ? orig_x : (orig_x + junkie_mode ? 2 : 150);\n'
        '    int icon_spacing = nordic_tabs ? 720 / MAX(num_tabs,1) : (junkie_mode ? 716 / num_tabs : (720 - 150) / num_tabs);')
    menu=replace_once(menu,'        if (!menu_lv_transparent_mode)\n        {\n            if (menu->selected)\n                bmp_fill(bg, x-1, y+2, icon_spacing+3, 38);',
        '        if (!menu_lv_transparent_mode)\n        {\n'
        '            if(nordic_tabs) camdesk_ui_draw_tab(menu->name,menu->selected,x,icon_spacing);\n'
        '            else {\n'
        '            if (menu->selected)\n                bmp_fill(bg, x-1, y+2, icon_spacing+3, 38);')
    menu=replace_once(menu,'            x += icon_spacing;\n        }\n        \n        if (submenu) continue;',
        '            }\n            x += icon_spacing;\n        }\n        \n        if (submenu) continue;')
    return menu

def lens_overlay(lens):
    lens='#include "nordic.h"\n'+lens
    lens=replace_once(lens,'    if (!should_draw_bottom_bar()) return;\n\n    lvinfo_display(0,1);',
        '    if (!should_draw_bottom_bar()) return;\n    if(camdesk_ui_try_monitor(0,1))return;\n\n    lvinfo_display(0,1);')
    lens=replace_once(lens,'    if (!get_global_draw()) return;\n    \n    lvinfo_display(1,0);',
        '    if (!get_global_draw()) return;\n    if(camdesk_ui_try_monitor(1,0))return;\n    \n    lvinfo_display(1,0);')
    lens=replace_once(lens,'static void lens_info_init()',
        'PROP_INT(PROP_WB_MODE_PH, camdesk_wb_mode_ph);\n'+
        (ROOT/'ui/lens_snapshot.inc').read_text(encoding='utf-8')+'\nstatic void lens_info_init()')
    return lens
