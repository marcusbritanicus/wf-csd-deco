#pragma once

#include "protocol.hpp"
#include <QApplication>
#include <QWidget>
#include <QPointer>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QToolButton>
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

class DecorationWindow : public QWidget
{
    Q_OBJECT

  public:
    DecorationWindow(uint32_t wf_id, QWidget *parent = nullptr);
    ~DecorationWindow();

    void setWindowTitle(const QString & title);
    void setAppId(const QString & appId);
    uint32_t getWfId() const
    {
        return wf_id;
    }

  protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

    void paintEvent(QPaintEvent *pEvent) override;

  private:
    void setupUI();
    void addTabButton(window_data *wdata, window_data *cdata);
    void clearTabs();
    void refreshTabs();
    void updateTabOrder();
    void scrollSync(uint32_t group_id);
    void ungroup(window_data *wdata, bool notify_server);
    void group(window_data *drop_target_data, uint32_t wf_id);
    void clearGroupTabs(uint32_t group_id);
    void reparentGroup(uint32_t group_id, window_data *last_parent);
    void refreshGroup(uint32_t group_id);

    uint32_t wf_id;
    QScrollArea *tabScrollArea;
    QWidget *tabContainer;

    QVBoxLayout *baseLyt;
    QPointer<QWidget> clientArea;

    QPointer<QLabel> iconLbl;
    QPointer<QLabel> titleLbl;

    QPointer<QToolButton> minBtn;
    QPointer<QToolButton> maxBtn;
    QPointer<QToolButton> closeBtn;

    QMap<uint32_t, QPushButton*> tabButtons;
    window_data *wdata;
    bool isGroupParent;
    uint32_t groupId;
    QList<uint32_t> groupOrder;

    // Flag to prevent recursive resizing
    bool isResizingFromCompositor;
    QSize pendingClientSize;
};
