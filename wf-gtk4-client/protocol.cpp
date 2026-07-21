#include "protocol.hpp"
#include "wf-decorator-client-protocol.h"
#include <wayland-client.h>
#include <string.h>
#include <iostream>

static wf_decorator_manager *decorator_manager;

static void create_new_decoration(void*, wf_decorator_manager*, uint32_t view_id)
{
    std::cout << "create new decoration" << std::endl;
    auto window = create_deco_window(view_id);
    view_to_decor[view_id] = window;
}

static void destroy_decoration(void*, wf_decorator_manager*, uint32_t view_id)
{
    std::cout << "destroy decoration" << std::endl;
    destroy_deco_window(view_id);
}

static void title_changed(void*,
    wf_decorator_manager*, uint32_t view, const char *new_title)
{
    set_title(view_to_decor[view], new_title);
}

static void app_id_changed(void*,
    wf_decorator_manager*, uint32_t view, const char *new_app_id)
{
    set_app_id(view_to_decor[view], new_app_id);
}

static void notify_focus(void*, wf_decorator_manager*, uint32_t view)
{}

const wf_decorator_manager_listener decorator_listener =
{
    create_new_decoration,
    destroy_decoration,
    title_changed,
    app_id_changed,
    notify_focus
};

static uint32_t decorator_global_id;
void registry_add_object(void*, struct wl_registry *registry, uint32_t id,
    const char *interface, uint32_t)
{
    std::cout << "new registry: " << interface << std::endl;
    if ((strcmp(interface, wf_decorator_manager_interface.name) == 0) &&
        (decorator_global_id != id))
    {
        std::cout << "bind it" << std::endl;
        decorator_manager =
            (wf_decorator_manager*)wl_registry_bind(registry, id, &wf_decorator_manager_interface, 1u);

        wf_decorator_manager_add_listener(decorator_manager, &decorator_listener, NULL);
        decorator_global_id = id;
    }
}

void registry_remove_object(void*, struct wl_registry*, uint32_t id)
{
    if (id == decorator_global_id)
    {
        wf_decorator_manager_destroy(decorator_manager);
        decorator_manager = NULL;
    }
}

static struct wl_registry_listener registry_listener =
{
    &registry_add_object,
    &registry_remove_object
};

void close_request(uint32_t id)
{
    if (!decorator_manager)
    {
        return;
    }

    wf_decorator_manager_close_request(decorator_manager, id);
}

void update_borders(uint32_t id, uint32_t top, uint32_t bottom, uint32_t left, uint32_t right)
{
    if (!decorator_manager)
    {
        return;
    }

    wf_decorator_manager_update_borders(decorator_manager, id, top, bottom, left, right);
}

void group_windows(uint32_t parent_id, uint32_t child_id)
{
    if (!decorator_manager)
    {
        return;
    }

    wf_decorator_manager_group_windows(decorator_manager, parent_id, child_id);
}

void select_window(uint32_t id)
{
    if (!decorator_manager)
    {
        return;
    }

    wf_decorator_manager_select_window(decorator_manager, id);
}

void ungroup_window(uint32_t id)
{
    if (!decorator_manager)
    {
        return;
    }

    wf_decorator_manager_ungroup_window(decorator_manager, id);
}

void setup_protocol(GdkDisplay *displ)
{
    auto display  = gdk_wayland_display_get_wl_display(displ);
    auto registry = wl_display_get_registry(display);

    wl_registry_add_listener(registry, &registry_listener, NULL);
    wl_display_roundtrip(display);
}
