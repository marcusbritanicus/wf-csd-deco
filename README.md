# Wayfire Client Side Decoration Decorator

## Overview

This decorator includes a wayfire plugin `csd-decorator` and two clients: `wf-gtk4-decorator` and `wf-qt6-decorator`. When the plugin is loaded and a client is running, toolkit themed decoration windows are applied to each window for decorations. As an additional feature, the decorations serve to support groups and tabs.

## Demo

https://github.com/user-attachments/assets/51d04184-4c68-4e9a-9b9d-f9a6a4c602fb

## Technical

The plugin and decorator communicate using a custom protocol. The plugin sends an event for each toplevel that should be decorated. The client responds by creating a new window. The content area is 'cut' out so that transparent windows work as expected. For windows that draw decorations regardless of compositor preference, they are double decorated unless the plugin option is toggled to disable decorations on these types of windows. 

## Installation

`meson setup --prefix=/usr build && ninja -C build && sudo -E ninja -C build install`

## Usage

Enable `csd-decorator` plugin by placing it in the list of `plugins` in the `[core]` section on wayfire config. Start a decorator client, either `wf-gtk4-decorator` or `wf-qt6-decorator` with the `[autostart]` plugin, or manually.

## Grouping

gtk4-decorator: Each window decoration has a button. Dragging one button onto another window's button will group the windows. Middle click ungroups them.

qt6-decorator: Each window has an icon in the decorator titlebar on the left as well as a drop down button. Dragging an icon to another window's group button groups the windows together. To ungroup a window, click the button on the right of its name in the drop down menu.

## Road Map

Live well, have fun.
