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
#include <QAbstractAnimation>
#include <QPropertyAnimation>

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

    DecorationButton(Type btnType, QWidget *parent = nullptr) :
        QWidget(parent), buttonType(btnType), mOpacity(0.25), isUnderMouse(false)
    {
        setFixedSize(16, 16);
        setMouseTracking(true);

        // Setup opacity animation
        opacityAnimation = new QPropertyAnimation(this, "opacity", this);
        opacityAnimation->setDuration(200);
        opacityAnimation->setEasingCurve(QEasingCurve::OutCubic);
    }

    qreal opacity() const
    {
        return mOpacity;
    }

    void setOpacity(qreal opacity)
    {
        mOpacity = opacity;
        update();
    }

    bool isUnderMouse = false;

  signals:
    void clicked();

  protected:
    void enterEvent(QEnterEvent *event) override
    {
        isUnderMouse = true;
        animateOpacity(0.75);
        qobject_cast<QWidget *>( parent() )->repaint();
        QWidget::enterEvent(event);
    }

    void leaveEvent(QEvent *event) override
    {
        isUnderMouse = false;
        animateOpacity(0.25);
        qobject_cast<QWidget *>( parent() )->repaint();
        QWidget::leaveEvent(event);
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton)
        {
            isPressed = true;
            animateOpacity(1.0);
            update();
        }

        QWidget::mousePressEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent *event) override
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

    void paintEvent(QPaintEvent *event) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // Get the color based on button type
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

        if (isUnderMouse) {
            painter.setPen(QPen(color, 2.0, Qt::SolidLine));
        }

        else {
            painter.setPen(QPen(color, 2.0, Qt::SolidLine));
        }

        // Apply current opacity
        color.setAlphaF(mOpacity);

        // Draw the circle
        QRect circleRect((width() - 16) / 2, (height() - 16)/2, 16, 16);
        painter.setBrush(color);
        painter.drawEllipse(circleRect.adjusted(1.0, 1.0, -1.0, -1.0));

        painter.end();
    }

  private:
    void animateOpacity(qreal targetOpacity)
    {
        if (opacityAnimation->state() == QAbstractAnimation::Running)
        {
            opacityAnimation->stop();
        }

        opacityAnimation->setEndValue(targetOpacity);
        opacityAnimation->start();
    }

    Type buttonType;
    qreal mOpacity;
    bool isPressed = false;
    QPropertyAnimation *opacityAnimation;
};

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
    void mouseMoveEvent(QMouseEvent *event) override;

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
    bool isOverButtons();

    uint32_t wf_id;
    QScrollArea *tabScrollArea;
    QWidget *tabContainer;

    QVBoxLayout *baseLyt;
    QPointer<QWidget> clientArea;

    QPointer<QLabel> iconLbl;
    QPointer<QLabel> titleLbl;

    QPointer<DecorationButton> minBtn;
    QPointer<DecorationButton> maxBtn;
    QPointer<DecorationButton> closeBtn;

    QMap<uint32_t, QPushButton*> tabButtons;
    window_data *wdata;
    bool isGroupParent;
    uint32_t groupId;
    QList<uint32_t> groupOrder;

    Qt::Edges getEdgesAt(const QPoint & pos);
    void updateCursorShape(const QPoint & pos);
    int defaultBorderSize = 2;
};

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
