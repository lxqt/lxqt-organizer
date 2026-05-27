/*
 * LXQt Organizer - personal information manager for LXQt.
 * Copyright (C) 2026 Basil Crow
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "aboutdialog.h"
#include "calendarpane.h"
#include "calendarpanecontroller.h"
#include "collectioncatalog.h"
#include "collectionservice.h"
#include "contactspane.h"
#include "contactspanecontroller.h"
#include "futurewatch.h"
#include "iactivepane.h"
#include "mainwindowservices.h"
#include "paneshelldeps.h"
#include "preferences.h"
#include "preferencescontroller.h"
#include "preferencesdialog.h"
#include "reloadcoordinator.h"
#include "storageerrormessages.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDialog>
#include <QFont>
#include <QFontMetrics>
#include <QIcon>
#include <QKeySequence>
#include <QLabel>
#include <QLocale>
#include <QMessageBox>
#include <QMetaObject>
#include <QPainter>
#include <QPaintEvent>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSplitter>
#include <QStatusBar>
#include <QStringList>
#include <QTabWidget>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include <functional>
#include <utility>

namespace {

void replaceTabPlaceholder(Ui::MainWindow &ui, QWidget *placeholder, QWidget *replacement, const QString &objectName);
void buildCentralLayout(MainWindow &window, Ui::MainWindow &ui, CalendarPane &calendarPane);
void setupShellActions(MainWindow &window, Ui::MainWindow &ui, PreferencesController &prefs);
void setupToolbar(Ui::MainWindow &ui);
void setupStatusBar(MainWindow &window,
                    Ui::MainWindow &ui,
                    PreferencesController &prefs,
                    ReloadCoordinator &reload,
                    QLabel *&itemCountOut,
                    QLabel *&selectedDateOut);
void wireTabWidgetSignals(MainWindow &window,
                          Ui::MainWindow &ui,
                          CalendarPane *calendarPane,
                          const std::function<void(IActivePane *)> &setActivePane);
void wirePreferenceSignals(MainWindow &window, PreferencesController &prefs, CalendarPane &calendarPane);
void restoreCalendarSplitter(MainWindow &window, CalendarPane &calendarPane, const QByteArray &state);
void restoreDefaultCalendarSplitter(MainWindow &window, CalendarPane &calendarPane);
void updateStatusBarSelectedDate(QLabel *selectedDateLabel, const PreferencesController &prefs, const QDate &date);
void showReadFailures(MainWindow &window, const QList<ReadFailure> &readFailures);
void connectContextAlias(MainWindow &window,
                         QAction *alias,
                         const QString &contextActionId,
                         const std::function<void(const QString &)> &triggerContextAction);
void refreshContextAlias(QAction *alias,
                         const QString &contextActionId,
                         MainWindowServices &services,
                         CalendarPane *calendarPane,
                         ContactsPane *contactsPane,
                         IActivePane *activePane);
QAction *contextAction(CalendarPane *calendarPane,
                       ContactsPane *contactsPane,
                       IActivePane *activePane,
                       const QString &contextActionId);
bool contextActionAvailable(MainWindowServices &services, const QAction *action);
void updateViewToggleAction(Ui::MainWindow &ui, const ContactsPane &contactsPane, bool taskPaneVisible);

void showAboutDialog(MainWindow &window)
{
    AboutDialog dialog(QStringLiteral(LXQT_ORGANIZER_VERSION), &window);
    dialog.exec();
}

void presentPreferencesDialog(MainWindow &window, PreferencesController &controller)
{
    Preferences preferences = controller.preferences();
    PreferencesDialog dialog(&window, &preferences);
    dialog.setModal(true);

    if (dialog.exec() == QDialog::Accepted)
    {
        preferences = dialog.preferences();
        window.setWindowTitle(MainWindow::tr("LXQt Organizer"));
        controller.setPreferences(preferences);
    }

    window.applyApplicationFontSize(controller.applicationFontSize());
}

class StatusLabel : public QLabel
{
public:
    explicit StatusLabel(QWidget *parent = nullptr)
        : QLabel(parent)
    {
        setTextFormat(Qt::PlainText);
        setWordWrap(false);
        setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        setMinimumWidth(0);
    }

    QSize sizeHint() const override
    {
        const QFontMetrics metrics(font());
        return {metrics.horizontalAdvance(QStringLiteral("0000")), metrics.height()};
    }

    QSize minimumSizeHint() const override { return {0, sizeHint().height()}; }

    bool hasHeightForWidth() const override { return false; }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        const QFontMetrics metrics(font());
        painter.setPen(palette().color(foregroundRole()));
        painter.drawText(contentsRect(),
                         alignment() | Qt::TextSingleLine,
                         metrics.elidedText(text(), Qt::ElideRight, contentsRect().width()));
    }
};

bool sameReadFailure(const ReadFailure &first, const ReadFailure &second)
{
    return first.ref.collectionId == second.ref.collectionId && first.ref.href == second.ref.href &&
           first.status == second.status;
}

QList<ReadFailure> uniqueFailures(const QList<ReadFailure> &readFailures)
{
    QList<ReadFailure> failures;
    for (const ReadFailure &failure : readFailures)
    {
        if (!failure.isFailure())
        {
            continue;
        }
        bool found = false;
        for (const ReadFailure &existing : std::as_const(failures))
        {
            if (sameReadFailure(existing, failure))
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            failures.append(failure);
        }
    }
    return failures;
}

QStringList contextActionIds(const QAction *action)
{
    if (!action)
    {
        return {};
    }

    QStringList ids = action->property("contextActionIds").toStringList();
    if (ids.isEmpty() && !action->objectName().isEmpty())
    {
        ids.append(action->objectName());
    }
    return ids;
}

QAction *contextActionInPane(IActivePane *pane, const QString &contextActionId)
{
    if (!pane)
    {
        return nullptr;
    }

    for (QAction *action : pane->contextActions())
    {
        if (contextActionIds(action).contains(contextActionId))
        {
            return action;
        }
    }
    return nullptr;
}

bool anyWritable(const QList<Collection> &collections)
{
    for (const Collection &collection : collections)
    {
        if (collection.isWritable())
        {
            return true;
        }
    }
    return false;
}

void replaceTabPlaceholder(Ui::MainWindow &ui, QWidget *placeholder, QWidget *replacement, const QString &objectName)
{
    const int index = ui.tabWidget->indexOf(placeholder);
    if (index < 0)
    {
        return;
    }
    const QString tabText = ui.tabWidget->tabText(index);
    const QIcon tabIcon = ui.tabWidget->tabIcon(index);
    const QString tabToolTip = ui.tabWidget->tabToolTip(index);
    const QString tabWhatsThis = ui.tabWidget->tabWhatsThis(index);
    const bool tabEnabled = ui.tabWidget->isTabEnabled(index);

    replacement->setObjectName(objectName);
    ui.tabWidget->removeTab(index);
    placeholder->deleteLater();
    ui.tabWidget->insertTab(index, replacement, tabIcon, tabText);
    ui.tabWidget->setTabToolTip(index, tabToolTip);
    ui.tabWidget->setTabWhatsThis(index, tabWhatsThis);
    ui.tabWidget->setTabEnabled(index, tabEnabled);
}

void buildCentralLayout(MainWindow &window, Ui::MainWindow &ui, CalendarPane &calendarPane)
{
    window.centralWidget()->layout()->setContentsMargins(0, 0, 0, 0);
    QWidget *mainPane = new QWidget;
    QVBoxLayout *mainPaneLayout = new QVBoxLayout(mainPane);
    mainPaneLayout->setContentsMargins(0, 0, 0, 0);
    mainPaneLayout->setSpacing(0);
    ui.gridLayout_2->removeWidget(ui.tabWidget);
    ui.tabWidget->setParent(mainPane);
    mainPaneLayout->addWidget(ui.tabWidget);
    ui.gridLayout_2->addWidget(mainPane, 0, 0);
    ui.mainToolBar->setFloatable(false);
    ui.mainToolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    ui.tabWidget->setDocumentMode(true);
    QObject::connect(calendarPane.calendarSplitter(), &QSplitter::splitterMoved, &window, [&window, &calendarPane]() {
        window.setCalendarSplitterState(calendarPane.calendarSplitter()->saveState());
    });
}

void setupShellActions(MainWindow &window, Ui::MainWindow &ui, PreferencesController &prefs)
{
    window.setWindowTitle(MainWindow::tr("LXQt Organizer"));
    window.setWindowIcon(QIcon::fromTheme(QStringLiteral("lxqt-organizer")));
    ui.actionClose_Window->setShortcut(QKeySequence::Close);
    ui.actionNew_Event->setIconText(MainWindow::tr("New"));
    ui.actionNew_Task->setIconText(MainWindow::tr("Task"));
    ui.actionExit->setShortcut(QKeySequence::Quit);
    ui.actionNew_Event->setShortcut(QKeySequence::New);
    ui.actionNew_Task->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_N));
    ui.actionNew_Contact->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N));
    ui.actionDelete->setShortcut(QKeySequence::Delete);
    ui.actionFind->setShortcut(QKeySequence::Find);
    ui.actionFindNext->setShortcut(QKeySequence::FindNext);
    ui.actionFindPrev->setShortcut(QKeySequence::FindPrevious);
    ui.actionToday->setIconText(MainWindow::tr("Today"));
    ui.actionToday->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_T));
    ui.actionPrevious_Month->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Left));
    ui.actionNext_Month->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Right));
    ui.actionPreferences->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Comma));
    ui.actionIncrease_Font->setShortcuts(QKeySequence::ZoomIn);
    ui.actionDecrease_Font->setShortcuts(QKeySequence::ZoomOut);
    ui.actionReset_Font->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_0));
    ui.actionQuick_Full_View->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_F));
    ui.actionQuick_Full_View->setCheckable(true);
    QObject::connect(ui.actionAbout, &QAction::triggered, &window, [&window]() { showAboutDialog(window); });
    QObject::connect(ui.actionPreferences, &QAction::triggered, &window, [&window, &prefs]() {
        presentPreferencesDialog(window, prefs);
    });
}

void setupToolbar(Ui::MainWindow &ui)
{
    if (QToolButton *todayButton = qobject_cast<QToolButton *>(ui.mainToolBar->widgetForAction(ui.actionToday)))
    {
        todayButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        todayButton->setText(ui.actionToday->iconText());
    }
    if (QToolButton *newButton = qobject_cast<QToolButton *>(ui.mainToolBar->widgetForAction(ui.actionNew_Event)))
    {
        newButton->setMenu(ui.menuNew);
        newButton->setPopupMode(QToolButton::MenuButtonPopup);
        newButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        newButton->setText(ui.actionNew_Event->iconText());
        newButton->setToolTip(MainWindow::tr("Create an event. Use the menu for tasks and contacts."));
    }
    if (QToolButton *quickFullButton =
            qobject_cast<QToolButton *>(ui.mainToolBar->widgetForAction(ui.actionQuick_Full_View)))
    {
        quickFullButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    }
}

void setupStatusBar(MainWindow &window,
                    Ui::MainWindow &ui,
                    PreferencesController &prefs,
                    ReloadCoordinator &reload,
                    QLabel *&itemCountOut,
                    QLabel *&selectedDateOut)
{
    if (ui.itemCountLabel)
    {
        ui.itemCountLabel->hide();
        ui.itemCountLabel->deleteLater();
    }
    itemCountOut = new StatusLabel(window.statusBar());
    selectedDateOut = new QLabel(window.statusBar());
    selectedDateOut->setTextFormat(Qt::PlainText);
    selectedDateOut->setWordWrap(false);
    window.statusBar()->addWidget(itemCountOut, 1);
    window.statusBar()->addPermanentWidget(selectedDateOut);

    QObject::connect(&reload,
                     &ReloadCoordinator::readFailures,
                     &window,
                     [&window](const QList<ReadFailure> &readFailures) { showReadFailures(window, readFailures); });
    QObject::connect(&window,
                     &MainWindow::selectedDateChanged,
                     &window,
                     [selectedDateLabel = selectedDateOut, &prefs](const QDate &date) {
                         updateStatusBarSelectedDate(selectedDateLabel, prefs, date);
                     });
}

void wireTabWidgetSignals(MainWindow &window,
                          Ui::MainWindow &ui,
                          CalendarPane *calendarPane,
                          const std::function<void(IActivePane *)> &setActivePane)
{
    QObject::connect(ui.tabWidget, &QTabWidget::currentChanged, &window, [&ui, calendarPane, setActivePane]() {
        auto *active = dynamic_cast<IActivePane *>(ui.tabWidget->currentWidget());
        setActivePane(active);
        if (calendarPane)
        {
            calendarPane->clearCalendarFindStatus();
        }
    });
}

void wirePreferenceSignals(MainWindow &window, PreferencesController &prefs, CalendarPane &calendarPane)
{
    QObject::connect(
        &prefs, &PreferencesController::dayViewRangeChanged, &window, [&calendarPane](int startHour, int finishHour) {
            calendarPane.setDayViewTimeRange(startHour, finishHour);
        });
    QObject::connect(&prefs, &PreferencesController::applicationFontSizeChanged, &window, [&window](int fontSize) {
        window.applyApplicationFontSize(fontSize);
    });
}

void restoreCalendarSplitter(MainWindow &window, CalendarPane &calendarPane, const QByteArray &state)
{
    QSplitter *splitter = calendarPane.calendarSplitter();
    if (state.isEmpty())
    {
        restoreDefaultCalendarSplitter(window, calendarPane);
        return;
    }
    const QSignalBlocker blocker(splitter);
    if (!splitter->restoreState(state))
    {
        restoreDefaultCalendarSplitter(window, calendarPane);
    }
}

void restoreDefaultCalendarSplitter(MainWindow &window, CalendarPane &calendarPane)
{
    QTimer::singleShot(0, &window, [&window, &calendarPane]() {
        QSplitter *splitter = calendarPane.calendarSplitter();
        const int availableWidth = splitter->width();
        if (availableWidth <= 0)
        {
            return;
        }
        const int leftWidth = qMin(qMax(320, availableWidth / 3), 460);
        const QSignalBlocker blocker(splitter);
        splitter->setSizes({leftWidth, qMax(240, availableWidth - leftWidth)});
        window.setCalendarSplitterState(splitter->saveState());
    });
}

void updateStatusBarSelectedDate(QLabel *selectedDateLabel, const PreferencesController &prefs, const QDate &date)
{
    if (selectedDateLabel)
    {
        selectedDateLabel->setText(prefs.locale().toString(date, QLocale::LongFormat));
    }
}

void showReadFailures(MainWindow &window, const QList<ReadFailure> &readFailures)
{
    const QList<ReadFailure> failures = uniqueFailures(readFailures);
    if (failures.isEmpty())
    {
        return;
    }
    const QString detail = StorageErrorMessages::storageFailureDetail(failures.first().status);
    const QString message =
        detail.isEmpty()
            ? MainWindow::tr("%n item(s) could not be loaded.", nullptr, failures.size())
            : MainWindow::tr("%n item(s) could not be loaded. First error: %1", nullptr, failures.size()).arg(detail);
    window.statusBar()->showMessage(message, 10000);
}

void connectContextAlias(MainWindow &window,
                         QAction *alias,
                         const QString &contextActionId,
                         const std::function<void(const QString &)> &triggerContextAction)
{
    if (!alias)
    {
        return;
    }
    QObject::connect(alias, &QAction::triggered, &window, [contextActionId, triggerContextAction]() {
        triggerContextAction(contextActionId);
    });
}

QAction *contextAction(CalendarPane *calendarPane,
                       ContactsPane *contactsPane,
                       IActivePane *activePane,
                       const QString &contextActionId)
{
    if (contextActionId == QLatin1String("actionNew_Event") || contextActionId == QLatin1String("actionNew_Task"))
    {
        return contextActionInPane(calendarPane, contextActionId);
    }
    if (contextActionId == QLatin1String("actionNew_Contact"))
    {
        return contextActionInPane(contactsPane, contextActionId);
    }
    return contextActionInPane(activePane, contextActionId);
}

bool contextActionAvailable(MainWindowServices &services, const QAction *action)
{
    if (!action || !action->isEnabled())
    {
        return false;
    }

    const QString objectName = action->objectName();
    if (objectName == QLatin1String("actionNew_Event") || objectName == QLatin1String("actionNew_Task"))
    {
        return anyWritable(services.collectionService().calendarList());
    }
    if (objectName == QLatin1String("actionNew_Contact"))
    {
        return anyWritable(services.collectionService().addressBookList());
    }
    return true;
}

void refreshContextAlias(QAction *alias,
                         const QString &contextActionId,
                         MainWindowServices &services,
                         CalendarPane *calendarPane,
                         ContactsPane *contactsPane,
                         IActivePane *activePane)
{
    if (!alias)
    {
        return;
    }
    QAction *source = contextAction(calendarPane, contactsPane, activePane, contextActionId);
    const bool available = contextActionAvailable(services, source);
    QSignalBlocker blocker(alias);
    alias->setEnabled(available);
    if (!source)
    {
        return;
    }
    alias->setText(source->text());
    alias->setIconText(source->iconText());
    alias->setIcon(source->icon());
    alias->setToolTip(source->toolTip());
    alias->setStatusTip(source->statusTip());
    alias->setWhatsThis(source->whatsThis());
}

void updateViewToggleAction(Ui::MainWindow &ui, const ContactsPane &contactsPane, bool taskPaneVisible)
{
    if (ui.tabWidget->currentWidget() == &contactsPane)
    {
        const bool detailsVisible = contactsPane.contactDetailsVisible();
        ui.actionQuick_Full_View->setChecked(detailsVisible);
        ui.actionQuick_Full_View->setText(MainWindow::tr("Full View"));
        ui.actionQuick_Full_View->setToolTip(detailsVisible ? MainWindow::tr("Show full contact view")
                                                            : MainWindow::tr("Show quick contact view"));
        return;
    }

    ui.actionQuick_Full_View->setChecked(taskPaneVisible);
    ui.actionQuick_Full_View->setText(MainWindow::tr("Tasks"));
    ui.actionQuick_Full_View->setToolTip(taskPaneVisible ? MainWindow::tr("Hide tasks") : MainWindow::tr("Show tasks"));
}

} // namespace

MainWindow::MainWindow(MainWindowServices &services, QWidget *parent)
    : QMainWindow(parent)
    , m_ui(new Ui::MainWindow)
    , m_services(services)
{
    m_ui->setupUi(this);

    m_settingsSaveTimer.setSingleShot(true);
    m_settingsSaveTimer.setInterval(200);
    connect(&m_settingsSaveTimer, &QTimer::timeout, this, &MainWindow::flushSettingsSave);

    m_prefs = std::make_unique<PreferencesController>(this);
    m_reload = std::make_unique<ReloadCoordinator>(m_services.collectionService(),
                                                   m_services.eventService(),
                                                   m_services.taskService(),
                                                   m_services.contactService(),
                                                   this);
    setupShellActions(*this, *m_ui, *m_prefs);
    setupToolbar(*m_ui);
    configurePanes();
    wirePaneSignals();
    wirePreferenceSignals(*this, *m_prefs, *m_calendarPane);
    buildCentralLayout(*this, *m_ui, *m_calendarPane);
    setupStatusBar(*this, *m_ui, *m_prefs, *m_reload, m_itemCountLabel, m_selectedDateLabel);
    setupActionWiring();
    wireTabWidgetSignals(*this, *m_ui, m_calendarPane, [this](IActivePane *pane) { setActivePane(pane); });
    loadStorageAndSettings();
    setupInitialCalendarState();

    applyApplicationFontSize(m_prefs->applicationFontSize());
}

MainWindow::~MainWindow()
{
    if (m_settingsSaveTimer.isActive())
    {
        flushSettingsSave();
    }
    m_calendarPaneController.reset();
    m_contactsPaneController.reset();
    m_reload.reset();
    m_prefs.reset();
    delete m_ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    flushSettingsSave();
    QMainWindow::closeEvent(event);
}

void MainWindow::applyApplicationFontSize(int fontSize)
{
    QFont font = this->font();
    font.setPointSize(qBound(8, fontSize, 36));
    setFont(font);
    m_calendarPane->setCalendarFontSize(fontSize);
}

void MainWindow::saveSettings()
{
    flushSettingsSave();
}

void MainWindow::setSelectedDate(const QDate &date)
{
    if (!date.isValid() || m_selectedDate == date)
    {
        return;
    }
    m_selectedDate = date;
    Q_EMIT selectedDateChanged(m_selectedDate);
}

void MainWindow::setCalendarSplitterState(const QByteArray &state)
{
    if (m_calendarSplitterState == state)
    {
        return;
    }
    m_calendarSplitterState = state;
    Q_EMIT calendarSplitterStateChanged(m_calendarSplitterState);
}

void MainWindow::setContactDetailsVisible(bool visible)
{
    if (m_contactDetailsVisible == visible)
    {
        return;
    }
    m_contactDetailsVisible = visible;
    Q_EMIT contactDetailsVisibleChanged(m_contactDetailsVisible);
}

void MainWindow::setTaskPaneVisible(bool visible)
{
    if (m_taskPaneVisible == visible)
    {
        return;
    }
    m_taskPaneVisible = visible;
    Q_EMIT taskPaneVisibleChanged(m_taskPaneVisible);
}

void MainWindow::configurePanes()
{
    PaneShellDeps shellDeps{*m_reload, *m_prefs, m_services};

    CalendarPane::Deps calendarDeps{shellDeps};
    m_calendarPane = new CalendarPane(calendarDeps);
    replaceTabPlaceholder(*m_ui, m_ui->tabCalendar, m_calendarPane, QStringLiteral("tabCalendar"));
    CalendarPaneController::Deps calendarControllerDeps{m_services.eventService(),
                                                        m_services.taskService(),
                                                        m_services.collectionService(),
                                                        *m_reload,
                                                        *m_prefs,
                                                        m_services};
    m_calendarPaneController = std::make_unique<CalendarPaneController>(m_calendarPane, calendarControllerDeps, this);

    ContactsPane::Deps contactsDeps{shellDeps};
    m_contactsPane = new ContactsPane(contactsDeps);
    replaceTabPlaceholder(*m_ui, m_ui->tabContacts, m_contactsPane, QStringLiteral("tabContacts"));
    ContactsPaneController::Deps contactsControllerDeps{
        m_services.contactService(), m_services.collectionService(), *m_reload};
    m_contactsPaneController = std::make_unique<ContactsPaneController>(m_contactsPane, contactsControllerDeps, this);
}

void MainWindow::wirePaneSignals()
{
    connect(m_calendarPane, &CalendarPane::selectedDateChanged, this, &MainWindow::setSelectedDate);
    connect(this, &MainWindow::selectedDateChanged, m_calendarPane, [this](const QDate &date) {
        if (m_calendarPane->selectedDate() != date)
        {
            m_calendarPane->setSelectedDate(date);
        }
    });
    connect(this, &MainWindow::calendarSplitterStateChanged, this, [this](const QByteArray &state) {
        QSplitter *splitter = m_calendarPane->calendarSplitter();
        if (state.isEmpty())
        {
            restoreDefaultCalendarSplitter(*this, *m_calendarPane);
            return;
        }
        const QSignalBlocker blocker(splitter);
        if (!splitter->restoreState(state))
        {
            restoreDefaultCalendarSplitter(*this, *m_calendarPane);
        }
    });
    connect(this, &MainWindow::contactDetailsVisibleChanged, m_contactsPane, &ContactsPane::setContactDetailsVisible);
    connect(this, &MainWindow::taskPaneVisibleChanged, m_calendarPane, &CalendarPane::setTaskPaneVisible);

    connect(m_prefs.get(), &PreferencesController::preferencesChanged, this, [this](const Preferences &preferences) {
        m_settingsSnapshot.preferences = preferences;
        scheduleSettingsSave();
    });
    connect(this, &MainWindow::calendarSplitterStateChanged, this, [this](const QByteArray &state) {
        m_settingsSnapshot.calendarSplitterState = state;
        scheduleSettingsSave();
    });
    connect(this, &MainWindow::contactDetailsVisibleChanged, this, [this](bool visible) {
        m_settingsSnapshot.contactDetailsVisible = visible;
        scheduleSettingsSave();
    });
    connect(this, &MainWindow::taskPaneVisibleChanged, this, [this](bool visible) {
        m_settingsSnapshot.taskPaneVisible = visible;
        scheduleSettingsSave();
    });
}

void MainWindow::setupActionWiring()
{
    connect(&m_services.collectionService(),
            &CollectionService::collectionsReloaded,
            this,
            &MainWindow::scheduleRefreshActionState);
    connect(this, &MainWindow::contactDetailsVisibleChanged, this, &MainWindow::scheduleRefreshActionState);
    connect(this, &MainWindow::taskPaneVisibleChanged, this, &MainWindow::scheduleRefreshActionState);

    connect(m_ui->actionExit, &QAction::triggered, qApp, &QApplication::quit);
    connect(m_ui->actionNew_Window, &QAction::triggered, this, []() {
        if (QMetaObject::invokeMethod(QApplication::instance(), "newWindow"))
        {
            return;
        }
        qFatal("LXQt Organizer application object does not provide newWindow()");
    });
    connect(m_ui->actionClose_Window, &QAction::triggered, this, [this]() { close(); });
    connect(m_ui->actionNext_Month, &QAction::triggered, this, [this]() { m_calendarPane->gotoNextMonth(); });
    connect(m_ui->actionPrevious_Month, &QAction::triggered, this, [this]() { m_calendarPane->gotoPreviousMonth(); });
    connect(m_ui->actionToday, &QAction::triggered, this, [this]() { m_calendarPane->gotoToday(); });
    connect(m_ui->actionIncrease_Font, &QAction::triggered, this, [this]() {
        m_prefs->setApplicationFontSize(m_prefs->applicationFontSize() + 1);
    });
    connect(m_ui->actionDecrease_Font, &QAction::triggered, this, [this]() {
        m_prefs->setApplicationFontSize(m_prefs->applicationFontSize() - 1);
    });
    connect(m_ui->actionReset_Font, &QAction::triggered, this, [this]() { m_prefs->resetApplicationFontSize(); });
    connect(m_ui->actionQuick_Full_View, &QAction::triggered, this, [this]() {
        if (m_ui->tabWidget->currentWidget() == m_contactsPane)
        {
            setContactDetailsVisible(!m_contactsPane->contactDetailsVisible());
        }
        else
        {
            setTaskPaneVisible(!m_taskPaneVisible);
        }
        saveSettings();
    });

    const auto triggerContextAction = [this](const QString &contextActionId) {
        this->triggerContextAction(contextActionId);
    };
    connectContextAlias(*this, m_ui->actionNew_Event, QStringLiteral("actionNew_Event"), triggerContextAction);
    connectContextAlias(*this, m_ui->actionNew_Task, QStringLiteral("actionNew_Task"), triggerContextAction);
    connectContextAlias(*this, m_ui->actionNew_Contact, QStringLiteral("actionNew_Contact"), triggerContextAction);
    connectContextAlias(*this, m_ui->actionEdit, QStringLiteral("actionEdit"), triggerContextAction);
    connectContextAlias(*this, m_ui->actionDelete, QStringLiteral("actionDelete"), triggerContextAction);
    connectContextAlias(*this, m_ui->actionFind, QStringLiteral("actionFind"), triggerContextAction);
    connectContextAlias(*this, m_ui->actionFindNext, QStringLiteral("actionFindNext"), triggerContextAction);
    connectContextAlias(*this, m_ui->actionFindPrev, QStringLiteral("actionFindPrev"), triggerContextAction);
    connectContextAlias(*this, m_ui->actionMail_To, QStringLiteral("actionMail_To"), triggerContextAction);

    setActivePane(m_calendarPane);
}

void MainWindow::loadStorageAndSettings()
{
    m_calendarPane->setTimelineCollectionSummaryProvider([servicesPtr = &m_services](const QString &collectionId) {
        return summarizeCollection(*servicesPtr->catalogSnapshot(), CollectionKind::Calendar, collectionId);
    });
    loadSettings();
    FutureWatcher::watch(this, m_services.reloadCollections(), [this](bool loaded) {
        const auto catalog = m_services.catalogSnapshot();
        if (!loaded || !catalog || (catalog->calendarList().isEmpty() && catalog->addressBookList().isEmpty()))
        {
            QMessageBox::warning(this, tr("Organizer"), tr("Could not initialize calendar and contact storage."));
        }
        m_reload->reloadContacts();
        const QDate date = selectedDate();
        const QDate monthStart(date.year(), date.month(), 1);
        m_reload->setCalendarWindow(monthStart, monthStart.addMonths(1).addDays(-1), QDate::currentDate());
        m_reload->reloadCalendar();
    });
}

void MainWindow::loadSettings()
{
    const Preferences fallbackPreferences = m_prefs->preferences();
    const settings_io::Snapshot snapshot = settings_io::load(fallbackPreferences);
    m_settingsSnapshot = snapshot;
    m_prefs->setPreferences(snapshot.preferences);
    settings_io::restoreWindowGeometry(this, snapshot.geometry);
    setContactDetailsVisible(snapshot.contactDetailsVisible);
    setTaskPaneVisible(snapshot.taskPaneVisible);
    setCalendarSplitterState(snapshot.calendarSplitterState);
}

void MainWindow::setupInitialCalendarState()
{
    const QDate initialDate = QDate::currentDate();
    setSelectedDate(initialDate);
    m_calendarPane->setTimelineLocale(m_prefs->locale());
    m_calendarPane->setDayViewTimeRange(m_prefs->preferences().dayViewTimeStart,
                                        m_prefs->preferences().dayViewTimeFinish);
    m_calendarPane->setTaskPaneVisible(m_taskPaneVisible);
    restoreCalendarSplitter(*this, *m_calendarPane, m_calendarSplitterState);
}

void MainWindow::setActivePane(IActivePane *pane)
{
    for (const QMetaObject::Connection &connection : std::as_const(m_activePaneConnections))
    {
        disconnect(connection);
    }
    m_activePaneConnections.clear();
    for (const QMetaObject::Connection &connection : std::as_const(m_contextActionConnections))
    {
        disconnect(connection);
    }
    m_contextActionConnections.clear();

    m_activePane = pane;
    m_activePaneObject = dynamic_cast<QObject *>(pane);
    if (m_activePaneObject)
    {
        m_activePaneConnections.append(
            connect(m_activePaneObject, SIGNAL(actionStateChanged()), this, SLOT(refreshActionState())));
        m_activePaneConnections.append(
            connect(m_activePaneObject, SIGNAL(actionStateChanged()), this, SLOT(refreshStatusBar())));
        m_activePaneConnections.append(
            connect(m_activePaneObject, SIGNAL(itemCountChanged()), this, SLOT(refreshStatusBar())));
        m_activePaneConnections.append(connect(m_activePaneObject,
                                               SIGNAL(readFailures(QList<ReadFailure>)),
                                               this,
                                               SLOT(showReadFailures(QList<ReadFailure>))));
        for (QAction *action : m_activePane->contextActions())
        {
            if (action)
            {
                m_contextActionConnections.append(
                    connect(action, &QAction::changed, this, &MainWindow::scheduleRefreshActionState));
            }
        }
    }
    refreshActionState();
    refreshStatusBar();
}

void MainWindow::refreshStatusBar()
{
    m_itemCountLabel->setText(m_activePane ? m_activePane->selectionStatusText() : QString());
}

void MainWindow::showReadFailures(const QList<ReadFailure> &readFailures)
{
    ::showReadFailures(*this, readFailures);
}

void MainWindow::scheduleRefreshActionState()
{
    if (m_refreshActionStateQueued)
    {
        return;
    }
    m_refreshActionStateQueued = true;
    QTimer::singleShot(0, this, [this]() {
        m_refreshActionStateQueued = false;
        refreshActionState();
    });
}

void MainWindow::refreshActionState()
{
    refreshContextAliases();
    updateViewToggleAction(*m_ui, *m_contactsPane, m_taskPaneVisible);
}

void MainWindow::triggerContextAction(const QString &contextActionId)
{
    QAction *action = contextAction(m_calendarPane, m_contactsPane, m_activePane, contextActionId);
    if (action && contextActionAvailable(m_services, action))
    {
        action->trigger();
    }
}

void MainWindow::refreshContextAliases()
{
    refreshContextAlias(m_ui->actionNew_Event,
                        QStringLiteral("actionNew_Event"),
                        m_services,
                        m_calendarPane,
                        m_contactsPane,
                        m_activePane);
    refreshContextAlias(m_ui->actionNew_Task,
                        QStringLiteral("actionNew_Task"),
                        m_services,
                        m_calendarPane,
                        m_contactsPane,
                        m_activePane);
    refreshContextAlias(m_ui->actionNew_Contact,
                        QStringLiteral("actionNew_Contact"),
                        m_services,
                        m_calendarPane,
                        m_contactsPane,
                        m_activePane);
    refreshContextAlias(
        m_ui->actionEdit, QStringLiteral("actionEdit"), m_services, m_calendarPane, m_contactsPane, m_activePane);
    refreshContextAlias(
        m_ui->actionDelete, QStringLiteral("actionDelete"), m_services, m_calendarPane, m_contactsPane, m_activePane);
    refreshContextAlias(
        m_ui->actionFind, QStringLiteral("actionFind"), m_services, m_calendarPane, m_contactsPane, m_activePane);
    refreshContextAlias(m_ui->actionFindNext,
                        QStringLiteral("actionFindNext"),
                        m_services,
                        m_calendarPane,
                        m_contactsPane,
                        m_activePane);
    refreshContextAlias(m_ui->actionFindPrev,
                        QStringLiteral("actionFindPrev"),
                        m_services,
                        m_calendarPane,
                        m_contactsPane,
                        m_activePane);
    refreshContextAlias(
        m_ui->actionMail_To, QStringLiteral("actionMail_To"), m_services, m_calendarPane, m_contactsPane, m_activePane);
}

void MainWindow::scheduleSettingsSave()
{
    m_settingsSaveTimer.start();
}

void MainWindow::flushSettingsSave()
{
    if (m_settingsSaveTimer.isActive())
    {
        m_settingsSaveTimer.stop();
    }
    m_settingsSnapshot.preferences = m_prefs->preferences();
    m_settingsSnapshot.contactDetailsVisible = m_contactDetailsVisible;
    m_settingsSnapshot.taskPaneVisible = m_taskPaneVisible;
    m_settingsSnapshot.calendarSplitterState = m_calendarSplitterState;
    settings_io::saveWindowGeometry(this, &m_settingsSnapshot);
    settings_io::save(m_settingsSnapshot);
}
