#include "protocol.hpp"
#include "decorator.hpp"

#include <QApplication>
#include <QWidget>
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

#include <qpa/qplatformnativeinterface.h>

// Global data
static QApplication *app;
QMap<uint32_t, QWidget*> view_to_decor;
QMap<QWidget*, QSharedPointer<window_data>> win_data;

class TabDragSource : public QPushButton
{
    Q_OBJECT

  public:
    TabDragSource(window_data *data, QWidget *parent = nullptr) :
        QPushButton(parent), wdata(data)
    {}

  protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton)
        {
            dragStartPos = event->pos();
            QPushButton::mousePressEvent(event);
        } else if (event->button() == Qt::MiddleButton)
        {
            // ungroup(wdata, true);
            qCritical() << "ungroup";
        }
    }

    void mouseMoveEvent(QMouseEvent *event) override
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
        mimeData->setData("application/x-wf-window-id",
            QByteArray::number(wdata->wf_id));
        drag->setMimeData(mimeData);

        QPixmap pixmap = grab();
        drag->setPixmap(pixmap);
        drag->setHotSpot(event->pos());

        drag->exec(Qt::CopyAction | Qt::MoveAction);
    }

    void mouseReleaseEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton)
        {
            select_window(wdata->wf_id);
        }

        QPushButton::mouseReleaseEvent(event);
    }

  private:
    window_data *wdata;
    QPoint dragStartPos;
};

class DropTarget : public QWidget
{
    Q_OBJECT

  public:
    DropTarget(window_data *data, QWidget *parent = nullptr) :
        QWidget(parent), wdata(data)
    {
        setAcceptDrops(true);
    }

  protected:
    void dragEnterEvent(QDragEnterEvent *event) override
    {
        if (event->mimeData()->hasFormat("application/x-wf-window-id"))
        {
            event->acceptProposedAction();
        }
    }

    void dropEvent(QDropEvent *event) override
    {
        if (event->mimeData()->hasFormat("application/x-wf-window-id"))
        {
            bool ok;
            uint32_t wf_id = event->mimeData()->data("application/x-wf-window-id").toUInt(&ok);
            if (ok && (wdata->wf_id != wf_id))
            {
                // group(wdata, wf_id);
                qCritical() << "Group";
                event->acceptProposedAction();
            }
        }
    }

  private:
    window_data *wdata;
};

DecorationWindow::DecorationWindow(uint32_t id, QWidget *parent) :
    QWidget(parent), wf_id(id), isGroupParent(false), groupId(0)
{
    setWindowFlags(
        Qt::Window | Qt::CustomizeWindowHint | Qt::FramelessWindowHint | Qt::BypassWindowManagerHint);

    setAttribute(Qt::WA_TranslucentBackground);

    setMouseTracking(true);

    qCritical() << "Calling resize(...)";
    resize(250, 250);

    setupUI();

    wdata = new window_data();
    wdata->wf_id = id;
    wdata->scrolled_window = tabScrollArea;
    wdata->tab_box     = tabContainer;
    wdata->drag_source = nullptr;
    wdata->group.id    = 0;
    wdata->group.parent = false;
    wdata->group.order.clear();

    win_data[this]    = QSharedPointer<window_data>(wdata);
    view_to_decor[id] = this;

    // Create initial tab
    addTabButton(wdata, wdata);

    // Setup drop target for tabs
    tabContainer->setAcceptDrops(true);
}

DecorationWindow::~DecorationWindow()
{
    close_request(wf_id);
}

void DecorationWindow::setupUI()
{
    QVBoxLayout *baseLyt = new QVBoxLayout();
    baseLyt->setContentsMargins(QMargins(borderSize, 0, borderSize, borderSize));

    iconLbl = new QLabel(this);
    iconLbl->setFixedSize(QSize(24, 24));
    iconLbl->setPixmap(QIcon::fromTheme("wayfire").pixmap(24));

    titleLbl = new QLabel(this);
    titleLbl->setText("__wf_qt_decorator");

    minBtn = new QToolButton();
    minBtn->setFixedSize(QSize(24, 24));
    minBtn->setIconSize(QSize(24, 24));
    minBtn->setIcon(QIcon::fromTheme("window-minimize"));
    connect(minBtn, &QToolButton::clicked, this, &QWidget::showMinimized);

    maxBtn = new QToolButton();
    maxBtn->setFixedSize(QSize(24, 24));
    maxBtn->setIconSize(QSize(24, 24));
    maxBtn->setIcon(QIcon::fromTheme("window-maximize"));
    connect(
        maxBtn, &QToolButton::clicked, [this] ()
    {
        if (isMaximized())
        {
            showNormal();
            maxBtn->setIcon(QIcon::fromTheme("window-maximize"));
        } else
        {
            showMaximized();
            maxBtn->setIcon(QIcon::fromTheme("window-restore"));
        }
    });

    closeBtn = new QToolButton();
    closeBtn->setFixedSize(QSize(24, 24));
    closeBtn->setIconSize(QSize(24, 24));
    closeBtn->setIcon(QIcon::fromTheme("window-close"));
    connect(closeBtn, &QToolButton::clicked, this, &QWidget::close);

    clientArea = new QWidget(this);
    clientArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QHBoxLayout *titleLyt = new QHBoxLayout();
    titleLyt->setContentsMargins(QMargins(5, 5, 5, 5));

    titleLyt->addWidget(iconLbl);
    titleLyt->addWidget(titleLbl);
    titleLyt->addStretch();
    titleLyt->addWidget(minBtn);
    titleLyt->addWidget(maxBtn);
    titleLyt->addWidget(closeBtn);

    baseLyt->addLayout(titleLyt);
    baseLyt->addWidget(clientArea);

    setMinimumHeight(34);

    tabContainer = new QWidget(this);

    setLayout(baseLyt);

    // Install event filter on client area to detect resize
    clientArea->installEventFilter(this);
}

void DecorationWindow::setClientSize(const QSize & clientSize)
{
    if (clientSize.isEmpty())
    {
        return;
    }

    isResizingFromCompositor = true;
    pendingClientSize = clientSize;

    // Calculate total window size (client + decoration)
    QSize totalSize = clientSize;
    totalSize.setHeight(clientSize.height() + titleBarHeight + borderSize * 2);
    totalSize.setWidth(clientSize.width() + borderSize * 2);

    // Resize the window
    qCritical() << "Calling resize" << totalSize;
    resize(totalSize);

    isResizingFromCompositor = false;
}

void DecorationWindow::addTabButton(window_data *wdata, window_data *cdata)
{
    TabDragSource *button = new TabDragSource(cdata);
    button->setText(QString::fromStdString(cdata->app_id));
    button->setToolTip(QString::fromStdString(cdata->title));
    button->setFixedHeight(24);
    button->setStyleSheet(
        "QPushButton {"
        "  border: 1px solid #888;"
        "  border-radius: 3px;"
        "  padding: 2px 8px;"
        "  background: #f0f0f0;"
        "}"
        "QPushButton:hover {"
        "  background: #e0e0e0;"
        "}"
        "QPushButton:pressed {"
        "  background: #d0d0d0;"
        "}");

    tabButtons[cdata->wf_id] = button;
    // tabLayout->addWidget(button);

    // Setup drop target for the tab container if this is the first tab
    // if (tabLayout->count() == 1)
    // {
    // DropTarget *dropTarget = new DropTarget(wdata, tabContainer);
    //// The drop target will handle drops
    // }
}

void DecorationWindow::clearTabs()
{
    qDeleteAll(tabButtons);
    tabButtons.clear();
}

void DecorationWindow::refreshTabs()
{
    clearTabs();
    // Rebuild tabs based on group order
    refreshGroup(groupId);
}

void DecorationWindow::updateTabOrder()
{
    // Update tab order based on group order
}

void DecorationWindow::scrollSync(uint32_t group_id)
{
    if (!group_id)
    {
        return;
    }

    window_data *pdata = nullptr;
    for (auto it = win_data.begin(); it != win_data.end(); ++it)
    {
        if ((it.value()->group.id == group_id) && it.value()->group.parent)
        {
            pdata = it.value().data();
            break;
        }
    }

    if (!pdata)
    {
        qDebug() << "No parent in group?";
        return;
    }

    // int scrollPos = pdata->scrolled_window->horizontalScrollBar()->value();
    int scrollPos = 0;

    for (auto it = win_data.begin(); it != win_data.end(); ++it)
    {
        if ((it.value()->group.id == group_id) && (it.value().data() != pdata))
        {
            // it.value()->scrolled_window->horizontalScrollBar()->setValue(scrollPos);
        }
    }
}

void DecorationWindow::ungroup(window_data *wdata, bool notify_server)
{
    auto group_id = wdata->group.id;
    clearGroupTabs(group_id);

    if (group_id)
    {
        for (auto it = win_data.begin(); it != win_data.end(); ++it)
        {
            if ((group_id == it.value()->group.id) && it.value()->group.parent)
            {
                it.value()->group.order.removeAll(wdata->wf_id);
                if (wdata->group.parent)
                {
                    reparentGroup(group_id, wdata);
                }

                break;
            }
        }
    }

    wdata->group.parent = false;
    wdata->group.order.clear();
    wdata->group.id = 0;

    if (group_id)
    {
        addTabButton(wdata, wdata);
    }

    refreshGroup(group_id);
    if (notify_server)
    {
        ungroup_window(wdata->wf_id);
    }
}

void DecorationWindow::group(window_data *drop_target_data, uint32_t wf_id)
{
    auto drag_source_data = win_data[view_to_decor[wf_id]].data();
    uint32_t group_id     = 1;

    if (drag_source_data->group.id &&
        (drag_source_data->group.id == drop_target_data->group.id))
    {
        qDebug() << "Cannot add tab to the same group.";
        return;
    }

    ungroup(drag_source_data, false);
    group_windows(drop_target_data->wf_id, wf_id);

    if (drop_target_data->group.id)
    {
        group_id = drop_target_data->group.id;
    } else
    {
        for (auto it = win_data.begin(); it != win_data.end(); ++it)
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

    drag_source_data->group.id = group_id;

    for (auto it = win_data.begin(); it != win_data.end(); ++it)
    {
        if ((it.value()->group.id == group_id) && it.value()->group.parent)
        {
            it.value()->group.order.append(drag_source_data->wf_id);
            break;
        }
    }

    clearGroupTabs(group_id);
    refreshGroup(group_id);
    scrollSync(group_id);
}

void DecorationWindow::clearGroupTabs(uint32_t group_id)
{
    if (!group_id)
    {
        return;
    }

    QList<uint32_t> button_order;
    for (auto it = win_data.begin(); it != win_data.end(); ++it)
    {
        if ((it.value()->group.id == group_id) && it.value()->group.parent)
        {
            button_order = it.value()->group.order;
            break;
        }
    }

    for (auto id : button_order)
    {
        auto wdata = win_data[view_to_decor[id]].data();
        if (wdata->group.id == group_id)
        {
            // Clear tabs for this window
            if (auto *dec = qobject_cast<DecorationWindow*>(view_to_decor[id]))
            {
                dec->clearTabs();
            }
        }
    }
}

void DecorationWindow::reparentGroup(uint32_t group_id, window_data *last_parent)
{
    if (!group_id)
    {
        return;
    }

    for (auto it = win_data.begin(); it != win_data.end(); ++it)
    {
        if ((it.value()->group.id == group_id) && (last_parent != it.value().data()))
        {
            it.value()->group.order = last_parent->group.order;
            it.value()->group.parent = true;
            last_parent->group.parent = false;
            last_parent->group.order.clear();
            last_parent->group.id = 0;
            scrollSync(group_id);
            break;
        }
    }
}

void DecorationWindow::refreshGroup(uint32_t group_id)
{
    if (!group_id)
    {
        return;
    }

    QList<uint32_t> button_order;
    for (auto it = win_data.begin(); it != win_data.end(); ++it)
    {
        if ((it.value()->group.id == group_id) && it.value()->group.parent)
        {
            button_order = it.value()->group.order;
            break;
        }
    }

    for (auto it = win_data.begin(); it != win_data.end(); ++it)
    {
        for (auto id : button_order)
        {
            auto cdata = win_data[view_to_decor[id]].data();
            if ((cdata->group.id == group_id) && (it.value()->group.id == group_id))
            {
                if (auto *dec = qobject_cast<DecorationWindow*>(view_to_decor[id]))
                {
                    dec->addTabButton(it.value().data(), cdata);
                }
            }
        }
    }

    for (auto it = win_data.begin(); it != win_data.end(); ++it)
    {
        if (auto *dec = qobject_cast<DecorationWindow*>(view_to_decor[it.value()->wf_id]))
        {
            if (dec->tabButtons.size() == 1)
            {
                dec->clearTabs();
                it.value()->group.id = 0;
                it.value()->group.parent = false;
                it.value()->group.order.clear();
                dec->addTabButton(it.value().data(), it.value().data());
            }
        }
    }
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
    qCritical() << "resizeEvent(...)";
    QWidget::resizeEvent(event);

    // Update borders
    update_borders(wf_id, titleBarHeight, borderSize, borderSize, borderSize);

    qCritical() << event->size();
}

bool DecorationWindow::eventFilter(QObject *obj, QEvent *event)
{
    if ((obj == this) && (event->type() == QEvent::Resize))
    {
        QResizeEvent *resizeEvent = static_cast<QResizeEvent*>(event);

        qCritical() << "resize event" << resizeEvent->size();

        // Only send update_borders if this resize came from the compositor
        // or if we're not in the middle of a compositor-triggered resize
        if (!isResizingFromCompositor)
        {
            // This resize came from the user or window manager
            // Calculate client area size (total - decoration)
            int clientHeight = resizeEvent->size().height() - titleBarHeight;
            if (clientHeight < 0)
            {
                clientHeight = 0;
            }

            // Send the client area size to the compositor
            // Note: This would be a custom protocol message if needed
            update_borders(wf_id, titleBarHeight + borderSize, borderSize, borderSize, borderSize);

            qDebug() << "Window resized by user. Client area:" <<
                resizeEvent->size().width() << "x" << clientHeight;
        }
    }

    return QWidget::eventFilter(obj, event);
}

void DecorationWindow::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing);

    painter.setPen(QPen(Qt::black, 2.0));

    if (minBtn->underMouse())
    {
        painter.setPen(QPen(Qt::darkYellow, 2.0));
    }

    if (maxBtn->underMouse())
    {
        painter.setPen(QPen(Qt::darkBlue, 2.0));
    }

    if (closeBtn->underMouse())
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
    // Update icon
    iconLbl->setPixmap(QIcon::fromTheme(appId).pixmap(24));
}

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

        // Setup Wayland protocol
        setup_protocol(display);

        return a.exec();
    } else
    {
        qFatal() << "Unable to get wayland dislay!";
    }
}

#include "main.moc"
