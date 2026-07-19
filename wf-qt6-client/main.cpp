#include "protocol.hpp"
#include "decorator.hpp"

#include <QApplication>
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QScrollArea>
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QToolTip>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QWindow>
#include <QScreen>
#include <QMap>
#include <QSharedPointer>
#include <QDebug>
#include <QPainter>
#include <QMenu>

#include <qpa/qplatformnativeinterface.h>

// Global data
static QApplication *app;
QMap<uint32_t, QWidget*> view_to_decor;
QMap<QWidget*, QSharedPointer<WindowData>> winData;

// ===== DecorationButton Implementation =====
DecorationButton::DecorationButton(Type btnType, QWidget *parent) :
    QWidget(parent), buttonType(btnType), mOpacity(0.25), isUnderMouse(false)
{
    setFixedSize(16, 16);
    setMouseTracking(true);

    opacityAnimation = new QPropertyAnimation(this, "opacity", this);
    opacityAnimation->setDuration(200);
    opacityAnimation->setEasingCurve(QEasingCurve::OutCubic);
}

void DecorationButton::setOpacity(qreal opacity)
{
    mOpacity = opacity;
    update();
}

void DecorationButton::enterEvent(QEnterEvent *event)
{
    isUnderMouse = true;
    animateOpacity(0.75);
    qobject_cast<QWidget*>(parent())->repaint();
    QWidget::enterEvent(event);
}

void DecorationButton::leaveEvent(QEvent *event)
{
    isUnderMouse = false;
    animateOpacity(0.25);
    qobject_cast<QWidget*>(parent())->repaint();
    QWidget::leaveEvent(event);
}

void DecorationButton::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        isPressed = true;
        animateOpacity(1.0);
        update();
    }

    QWidget::mousePressEvent(event);
}

void DecorationButton::mouseReleaseEvent(QMouseEvent *event)
{
    if ((event->button() == Qt::LeftButton) && isPressed)
    {
        isPressed = false;
        animateOpacity(isUnderMouse ? 0.75 : 0.25);
        update();
        emit clicked();
    }

    QWidget::mouseReleaseEvent(event);
}

void DecorationButton::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QColor color;
    switch (buttonType)
    {
      case Type::Minimize:
        color = Qt::darkYellow;
        break;

      case Type::Maximize:
        color = Qt::darkCyan;
        break;

      case Type::Pin:
        color = Qt::darkGreen;
        break;

      case Type::Close:
        color = Qt::darkRed;
        break;
    }

    painter.setPen(QPen(color, 2.0, Qt::SolidLine));
    color.setAlphaF(mOpacity);
    QRect circleRect((width() - 16) / 2, (height() - 16) / 2, 16, 16);
    painter.setBrush(color);
    painter.drawEllipse(circleRect.adjusted(1.0, 1.0, -1.0, -1.0));
    painter.end();
}

void DecorationButton::animateOpacity(qreal targetOpacity)
{
    if (opacityAnimation->state() == QAbstractAnimation::Running)
    {
        opacityAnimation->stop();
    }

    opacityAnimation->setEndValue(targetOpacity);
    opacityAnimation->start();
}

// ===== TabDragSource Implementation =====
TabDragSource::TabDragSource(WindowData *data, QWidget *parent) :
    QLabel(parent), wdata(data)
{
    setFixedSize(24, 24);
    setScaledContents(true);
    setStyleSheet("QLabel { border: none; background: transparent; }");
}

void TabDragSource::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        dragStartPos = event->pos();
    }

    QLabel::mousePressEvent(event);
}

void TabDragSource::mouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton))
    {
        return;
    }

    if ((event->pos() - dragStartPos).manhattanLength() < QApplication::startDragDistance())
    {
        return;
    }

    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData;
    mimeData->setData("application/x-wf-window-id", QByteArray::number(wdata->wf_id));
    drag->setMimeData(mimeData);
    drag->setPixmap(pixmap(Qt::ReturnByValue).scaled(32, 32));
    drag->setHotSpot(QPoint(16, 16));
    drag->exec(Qt::CopyAction | Qt::MoveAction);
}

void TabDragSource::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        select_window(wdata->wf_id);
    }

    QLabel::mouseReleaseEvent(event);
}

// ===== TabDropTarget Implementation =====
TabDropTarget::TabDropTarget(QWidget *parent) :
    QPushButton(parent)
{
    setAcceptDrops(true);
    setFixedHeight(24);
    setText("+");
    setToolTip("Drop here to group windows");
    setStyleSheet(
        "QPushButton {"
        "  border: 1px solid #888;"
        "  border-radius: 3px;"
        "  padding: 2px 8px;"
        "  background: transparent;"
        "}"
        "QPushButton:hover {"
        "  background: #d8d8d8;"
        "}");
    setMenu(new QMenu(this));
}

void TabDropTarget::addWindow(uint32_t wf_id, const QString & appId, const QString & title)
{
    QAction *action = menu()->addAction(QIcon::fromTheme(appId), title);
    action->setData(wf_id);
    connect(action, &QAction::triggered, this, [wf_id] ()
    {
        select_window(wf_id);
    });
    updateButtonState();
}

void TabDropTarget::removeWindow(uint32_t wf_id)
{
    for (QAction *action : menu()->actions())
    {
        if (action->data().toUInt() == wf_id)
        {
            menu()->removeAction(action);
            delete action;
            break;
        }
    }

    updateButtonState();
}

void TabDropTarget::clearAll()
{
    menu()->clear();
    updateButtonState();
}

void TabDropTarget::updateButtonState()
{
    if (menu()->actions().isEmpty())
    {
        setIcon(QIcon());
        setText("+");
        setToolTip("Drop here to group windows");
    } else
    {
        QAction *first = menu()->actions().first();
        setIcon(first->icon());
        setText(first->text());
        setToolTip(QString("Group of %1 windows").arg(menu()->actions().size()));
    }
}

void TabDropTarget::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-wf-window-id"))
    {
        event->acceptProposedAction();
        setStyleSheet(
            "QPushButton {"
            "  border: 2px solid #4CAF50;"
            "  border-radius: 3px;"
            "  padding: 2px 8px;"
            "  background: #c8e6c9;"
            "}");
    }
}

void TabDropTarget::dragLeaveEvent(QDragLeaveEvent *event)
{
    setStyleSheet(
        "QPushButton {"
        "  border: 1px solid #888;"
        "  border-radius: 3px;"
        "  padding: 2px 8px;"
        "  background: transparent;"
        "}");
    QPushButton::dragLeaveEvent(event);
}

void TabDropTarget::dropEvent(QDropEvent *event)
{
    setStyleSheet(
        "QPushButton {"
        "  border: 1px solid #888;"
        "  border-radius: 3px;"
        "  padding: 2px 8px;"
        "  background: transparent;"
        "}");

    if (event->mimeData()->hasFormat("application/x-wf-window-id"))
    {
        bool ok;
        uint32_t dropped_wf_id = event->mimeData()->data("application/x-wf-window-id").toUInt(&ok);
        if (ok)
        {
            DecorationWindow *targetWindow = qobject_cast<DecorationWindow*>(parent());
            if (targetWindow)
            {
                QWidget *droppedWidget = view_to_decor[dropped_wf_id];
                if (droppedWidget)
                {
                    WindowData *droppedData = winData[droppedWidget].data();
                    if (droppedData)
                    {
                        targetWindow->group(targetWindow->getWindowData(), dropped_wf_id);
                        event->acceptProposedAction();
                    }
                }
            }
        }
    }
}

// ===== DecorationWindow Implementation =====
DecorationWindow::DecorationWindow(uint32_t id, QWidget *parent) :
    QWidget(parent), wf_id(id), isGroupParent(false), groupId(0)
{
    setWindowFlags(
        Qt::Window | Qt::CustomizeWindowHint | Qt::FramelessWindowHint | Qt::BypassWindowManagerHint);

    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
    resize(300, 300);

    wdata = new WindowData();
    wdata->wf_id    = id;
    wdata->group.id = 0;
    wdata->group.parent = false;
    wdata->group.order.clear();

    setupUI();

    winData[this]     = QSharedPointer<WindowData>(wdata);
    view_to_decor[id] = this;
}

DecorationWindow::~DecorationWindow()
{
    close_request(wf_id);
}

void DecorationWindow::setupUI()
{
    baseLyt = new QVBoxLayout();
    baseLyt->setContentsMargins(QMargins(defaultBorderSize, 0, defaultBorderSize, defaultBorderSize));
    baseLyt->setSpacing(0);

    iconLbl = new TabDragSource(wdata, this);
    iconLbl->setFixedSize(QSize(24, 24));
    iconLbl->setPixmap(QIcon::fromTheme("wayfire").pixmap(24));

    titleLbl = new QLabel(this);
    titleLbl->setStyleSheet("QLabel { color: #AAFFFFFF; }");

    minBtn = new DecorationButton(DecorationButton::Type::Minimize, this);
    minBtn->setFixedSize(QSize(24, 24));
    minBtn->setMouseTracking(true);
    connect(minBtn, &DecorationButton::clicked, this, &QWidget::showMinimized);

    maxBtn = new DecorationButton(DecorationButton::Type::Maximize, this);
    maxBtn->setFixedSize(QSize(24, 24));
    maxBtn->setMouseTracking(true);
    connect(maxBtn, &DecorationButton::clicked, [this] ()
    {
        if (isMaximized())
        {
            showNormal();
        } else
        {
            showMaximized();
        }
    });

    closeBtn = new DecorationButton(DecorationButton::Type::Close, this);
    closeBtn->setFixedSize(QSize(24, 24));
    closeBtn->setMouseTracking(true);
    connect(closeBtn, &DecorationButton::clicked, this, &QWidget::close);

    groupBtn = new TabDropTarget(this);
    groupBtn->setFixedHeight(24);

    clientArea = new QWidget();
    clientArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QHBoxLayout *titleLyt = new QHBoxLayout();
    titleLyt->setContentsMargins(QMargins(5, 5, 5, 5));
    titleLyt->setSpacing(5);

    titleLyt->addWidget(iconLbl);
    titleLyt->addWidget(groupBtn);
    titleLyt->addWidget(titleLbl);
    titleLyt->addStretch();
    titleLyt->addWidget(minBtn);
    titleLyt->addWidget(maxBtn);
    titleLyt->addWidget(closeBtn);

    baseLyt->addLayout(titleLyt);
    baseLyt->addWidget(clientArea);

    setLayout(baseLyt);
}

// ===== Group Management - Like GTK version =====

void DecorationWindow::addTabForWindow(WindowData *wdata, WindowData *cdata)
{
    // In Qt version, we add to the TabDropTarget menu instead of a tab box
    // This is called by refreshGroup to populate the menu
    if (this->groupBtn && cdata)
    {
        qCritical() << cdata->app_id;
        qCritical() << cdata->title;
        qCritical() << "-------------";
        this->groupBtn->addWindow(
            cdata->wf_id,
            QString::fromStdString(cdata->app_id),
            QString::fromStdString(cdata->title));
    }
}

void DecorationWindow::clearGroupTabs(uint32_t group_id)
{
    if (!group_id)
    {
        return;
    }

    // Clear all TabDropTarget menus for windows in this group
    for (auto it = winData.begin(); it != winData.end(); ++it)
    {
        if (it.value()->group.id == group_id)
        {
            if (auto *dec = qobject_cast<DecorationWindow*>(view_to_decor[it.value()->wf_id]))
            {
                if (dec->groupBtn)
                {
                    dec->groupBtn->clearAll();
                }
            }
        }
    }
}

void DecorationWindow::refreshGroup(uint32_t group_id)
{
    if (!group_id)
    {
        return;
    }

    // Get the order from the parent
    QList<uint32_t> button_order;
    for (auto it = winData.begin(); it != winData.end(); ++it)
    {
        if ((it.value()->group.id == group_id) && it.value()->group.parent)
        {
            button_order = it.value()->group.order;
            break;
        }
    }

    // Clear all tabs in the group first
    clearGroupTabs(group_id);

    // For each window in the group, add all tabs
    for (auto it = winData.begin(); it != winData.end(); ++it)
    {
        if (it.value()->group.id == group_id)
        {
            if (auto *dec = qobject_cast<DecorationWindow*>(view_to_decor[it.value()->wf_id]))
            {
                // Add each window in order to this window's menu
                for (auto id : button_order)
                {
                    auto cdata = winData[view_to_decor[id]].data();
                    if (cdata && (cdata->group.id == group_id))
                    {
                        dec->addTabForWindow(it.value().data(), cdata);
                    }
                }
            }
        }
    }

    // If only one window in group, clear and reset to single tab
    for (auto it = winData.begin(); it != winData.end(); ++it)
    {
        if (it.value()->group.id == group_id)
        {
            if (auto *dec = qobject_cast<DecorationWindow*>(view_to_decor[it.value()->wf_id]))
            {
                if (dec->groupBtn && (dec->groupBtn->menu()->actions().size() <= 1))
                {
                    dec->groupBtn->clearAll();
                    it.value()->group.id = 0;
                    it.value()->group.parent = false;
                    it.value()->group.order.clear();
                    dec->addTabForWindow(it.value().data(), it.value().data());
                }
            }
        }
    }
}

void DecorationWindow::group(WindowData *drop_target_data, uint32_t wf_id)
{
    auto drag_source_data = winData[view_to_decor[wf_id]].data();
    uint32_t group_id     = 1;

    if (drag_source_data->group.id && (drag_source_data->group.id == drop_target_data->group.id))
    {
        qDebug() << "Cannot add tab to the same group.";
        return;
    }

    // Ungroup the dragged window first
    ungroup(drag_source_data, false);

    // Tell the compositor about the grouping
    group_windows(drop_target_data->wf_id, wf_id);

    // Find or create group ID
    if (drop_target_data->group.id)
    {
        group_id = drop_target_data->group.id;
    } else
    {
        for (auto it = winData.begin(); it != winData.end(); ++it)
        {
            if (it.value()->group.id >= group_id)
            {
                group_id = it.value()->group.id + 1;
            }
        }

        drop_target_data->group.parent = true;
        drop_target_data->group.id     = group_id;
        drop_target_data->group.order.append(drop_target_data->wf_id);
    }

    // Set the dragged window's group
    drag_source_data->group.id = group_id;

    // Add to parent's order
    for (auto it = winData.begin(); it != winData.end(); ++it)
    {
        if ((it.value()->group.id == group_id) && it.value()->group.parent)
        {
            if (!it.value()->group.order.contains(drag_source_data->wf_id))
            {
                it.value()->group.order.append(drag_source_data->wf_id);
            }

            break;
        }
    }

    // Refresh all windows in the group
    refreshGroup(group_id);
}

void DecorationWindow::ungroup(WindowData *wdata, bool notify_server)
{
    auto group_id = wdata->group.id;

    if (group_id)
    {
        // Remove from parent's order
        for (auto it = winData.begin(); it != winData.end(); ++it)
        {
            if ((group_id == it.value()->group.id) && it.value()->group.parent)
            {
                it.value()->group.order.removeAll(wdata->wf_id);
                break;
            }
        }
    }

    // Reset this window's group data
    wdata->group.parent = false;
    wdata->group.order.clear();
    wdata->group.id = 0;

    // If there was a group, refresh it
    if (group_id)
    {
        // Check if group still has members
        bool hasMembers = false;
        for (auto it = winData.begin(); it != winData.end(); ++it)
        {
            if (it.value()->group.id == group_id)
            {
                hasMembers = true;
                break;
            }
        }

        if (hasMembers)
        {
            // Refresh remaining group
            refreshGroup(group_id);
        } else
        {
            // Group is empty, clear all
            clearGroupTabs(group_id);
        }

        // Reset this window's menu to show only itself
        if (groupBtn)
        {
            groupBtn->clearAll();
            groupBtn->addWindow(
                wdata->wf_id,
                QString::fromStdString(wdata->app_id),
                QString::fromStdString(wdata->title));
        }
    }

    if (notify_server)
    {
        ungroup_window(wdata->wf_id);
    }
}

// ===== Event handlers =====

bool DecorationWindow::isOverButtons()
{
    return minBtn->isUnderMouse || maxBtn->isUnderMouse || closeBtn->isUnderMouse;
}

void DecorationWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-wf-window-id"))
    {
        event->acceptProposedAction();
    }
}

void DecorationWindow::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-wf-window-id"))
    {
        bool ok;
        uint32_t id = event->mimeData()->data("application/x-wf-window-id").toUInt(&ok);
        if (ok && (id != wf_id))
        {
            group(wdata, id);
            event->acceptProposedAction();
        }
    }
}

void DecorationWindow::closeEvent(QCloseEvent *event)
{
    close_request(wf_id);
    event->accept();
}

void DecorationWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    QPoint relative_position = clientArea->mapTo(window(), QPoint(0, 0));
    update_borders(wf_id, relative_position.y() - relative_position.x(),
        relative_position.x(), relative_position.x(), relative_position.x());
}

Qt::Edges DecorationWindow::getEdgesAt(const QPoint & pos)
{
    int padding     = 3;
    Qt::Edges edges = {};
    if (pos.x() <= defaultBorderSize + padding)
    {
        edges |= Qt::LeftEdge;
    } else if (pos.x() >= width() - defaultBorderSize - padding)
    {
        edges |= Qt::RightEdge;
    }

    if (pos.y() <= defaultBorderSize + padding)
    {
        edges |= Qt::TopEdge;
    } else if (pos.y() >= height() - defaultBorderSize - padding)
    {
        edges |= Qt::BottomEdge;
    }

    return edges;
}

void DecorationWindow::updateCursorShape(const QPoint & pos)
{
    Qt::Edges edges = getEdgesAt(pos);
    if (edges.testFlag(Qt::LeftEdge) && edges.testFlag(Qt::TopEdge))
    {
        setCursor(Qt::SizeFDiagCursor);
    } else if (edges.testFlag(Qt::RightEdge) && edges.testFlag(Qt::BottomEdge))
    {
        setCursor(Qt::SizeFDiagCursor);
    } else if (edges.testFlag(Qt::LeftEdge) && edges.testFlag(Qt::BottomEdge))
    {
        setCursor(Qt::SizeBDiagCursor);
    } else if (edges.testFlag(Qt::RightEdge) && edges.testFlag(Qt::TopEdge))
    {
        setCursor(Qt::SizeBDiagCursor);
    } else if (edges.testFlag(Qt::LeftEdge) || edges.testFlag(Qt::RightEdge))
    {
        setCursor(Qt::SizeHorCursor);
    } else if (edges.testFlag(Qt::TopEdge) || edges.testFlag(Qt::BottomEdge))
    {
        setCursor(Qt::SizeVerCursor);
    } else
    {
        setCursor(Qt::ArrowCursor);
    }
}

void DecorationWindow::mousePressEvent(QMouseEvent *event)
{
    if ((event->button() == Qt::LeftButton) && !isOverButtons() && !iconLbl->underMouse() &&
        !groupBtn->underMouse())
    {
        Qt::Edges edges = getEdgesAt(event->pos());
        if (edges)
        {
            windowHandle()->startSystemResize(edges);
        } else
        {
            windowHandle()->startSystemMove();
        }
    }

    QWidget::mousePressEvent(event);
}

void DecorationWindow::mouseMoveEvent(QMouseEvent *event)
{
    updateCursorShape(event->pos());
    QWidget::mouseMoveEvent(event);
}

void DecorationWindow::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing);

    if (hasFocus() || isActiveWindow())
    {
        painter.setPen(QPen(palette().color(QPalette::Highlight), 2.0));
    } else
    {
        painter.setPen(QPen(Qt::black, 2.0));
    }

    if (minBtn->isUnderMouse)
    {
        painter.setPen(QPen(Qt::darkYellow, 2.0));
    }

    if (maxBtn->isUnderMouse)
    {
        painter.setPen(QPen(Qt::darkBlue, 2.0));
    }

    if (closeBtn->isUnderMouse)
    {
        painter.setPen(QPen(Qt::darkRed, 2.0));
    }

    painter.setBrush(QColor(0, 0, 0, 120));
    painter.drawRoundedRect(QRect(0, 0, width(), height()).adjusted(1.0, 1.0, -1.0, -1.0), 5.0, 5.0);
    painter.end();
}

void DecorationWindow::setWindowTitle(const QString & title)
{
    QWidget::setWindowTitle(title);
    wdata->title = title.toStdString();
    titleLbl->setText(title);
}

void DecorationWindow::setAppId(const QString & appId)
{
    wdata->app_id = appId.toStdString();

    QPixmap pixmap;
    if (QIcon::hasThemeIcon(appId))
    {
        pixmap = QIcon::fromTheme(appId).pixmap(24);
    } else
    {
        pixmap = QIcon::fromTheme("wayfire").pixmap(24);
    }

    if (iconLbl)
    {
        iconLbl->setPixmap(pixmap);
        iconLbl->setWindowData(wdata);
    }

    // Add initial tab
    if (groupBtn)
    {
        groupBtn->addWindow(wdata->wf_id, appId, QString::fromStdString(wdata->title));
    }
}

// ===== Main =====

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setDesktopFileName("org.wf.sample-decorator");
    app = &a;

    QPlatformNativeInterface *native = QGuiApplication::platformNativeInterface();

    if (native)
    {
        struct wl_display *display =
            reinterpret_cast<wl_display*>(native->nativeResourceForIntegration("display"));

        setup_protocol(display);
        return a.exec();
    } else
    {
        qFatal() << "Unable to get wayland display!";
    }
}

#include "main.moc"
