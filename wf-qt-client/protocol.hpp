#pragma once

#include <QObject>
#include <QWidget>
#include <QMap>
#include <QSharedPointer>
#include <QString>
#include <QList>
#include <QScrollArea>

struct window_data {
    QWidget *scrolled_window;
    QWidget *tab_box;
    void *drag_source; // QDrag in Qt
    std::string app_id;
    std::string title;
    struct group_data {
        uint32_t id;
        bool parent;
        QList<uint32_t> order;
    } group;
    uint32_t wf_id;
};

// Forward declarations
class DecorationWindow;

// Global data
extern QMap<uint32_t, QWidget*> view_to_decor;
extern QMap<QWidget*, QSharedPointer<window_data>> win_data;

// Protocol functions
void setup_protocol(void *display);
void close_request(uint32_t wf_id);
void update_borders(uint32_t id, uint32_t top, uint32_t bottom, uint32_t left, uint32_t right);
void group_windows(uint32_t parent_id, uint32_t child_id);
void select_window(uint32_t id);
void ungroup_window(uint32_t id);

// Window management
DecorationWindow* create_deco_window(uint32_t wf_id);
void destroy_deco_window(uint32_t wf_id);
void set_title(QWidget *window, const char *title);
void set_app_id(QWidget *window, const char *app_id);
