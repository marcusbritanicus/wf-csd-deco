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
#include <QMenu>
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
#include <QDebug>
#include <QAbstractAnimation>
#include <QPropertyAnimation>
#include <QWidgetAction>

class DecorationButton;
class TabDragSource;
class TabDropTarget;

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

    // Group management - like GTK version
    void addTabForWindow(uint32_t cdata_wf_id);
    void refreshGroup(uint32_t group_id);
    void clearGroupTabs(uint32_t group_id);
    void group(uint32_t drop_target_id, uint32_t wf_id);
    void ungroup(uint32_t wf_id, bool notify_server);

  protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *pEvent) override;

  private:
    void setupUI();
    bool isOverButtons();

    uint32_t wf_id;
    QString appId;
    bool isGroupParent;
    uint32_t groupId;
    QList<uint32_t> groupOrder;

    QVBoxLayout *baseLyt;
    QPointer<QWidget> clientArea;

    QPointer<TabDragSource> iconLbl;
    QPointer<QLabel> titleLbl;
    QPointer<TabDropTarget> groupBtn;

    QPointer<DecorationButton> minBtn;
    QPointer<DecorationButton> maxBtn;
    QPointer<DecorationButton> closeBtn;

    Qt::Edges getEdgesAt(const QPoint & pos);
    void updateCursorShape(const QPoint & pos);
    int defaultBorderSize = 1;
};

class DecorationButton : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)

  public:
    enum class Type
    {
        Minimize,
        Maximize,
        Pin,
        Close,
    };

    DecorationButton(Type btnType, QWidget *parent = nullptr);
    qreal opacity() const
    {
        return mOpacity;
    }

    void setOpacity(qreal opacity);
    bool isUnderMouse = false;

  signals:
    void clicked();

  protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

  private:
    void animateOpacity(qreal targetOpacity);
    Type buttonType;
    qreal mOpacity;
    bool isPressed = false;
    QPropertyAnimation *opacityAnimation;
};

class TabDragSource : public QLabel
{
    Q_OBJECT

  public:
    TabDragSource(uint32_t id, QWidget *parent = nullptr);

  protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

  private:
    uint32_t wfId = 0;
    QPoint dragStartPos;
};

class TabDropTarget : public QPushButton
{
    Q_OBJECT

  public:
    TabDropTarget(QWidget *parent = nullptr);

    // Like add_tab_button in GTK version
    void addWindow(uint32_t wf_id, const QString & appId, const QString & title);
    void removeWindow(uint32_t wf_id);
    void clearAll();

  protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

  private:
    void updateButtonState();
};

class GroupEntry : public QWidget
{
    Q_OBJECT

  public:
    GroupEntry(uint32_t wf_id, const QString & appId, const QString & title, QWidget *parent);

    void setTitle(QString& title);
    void setAppId(QString& appId);

    Q_SIGNAL void ungroup();

  private:
    QPointer<TabDragSource> iconLbl;
    QPointer<QLabel> titleLbl;
    QPointer<QToolButton> ungroupBtn;

  protected:
    // void mousePressEvent(QMouseEvent *event) override;
    // void mouseReleaseEvent(QMouseEvent *event) override;
};
