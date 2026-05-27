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

#include "mainwindow_composition.h"

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "aboutdialog.h"
#include "calendarpane.h"
#include "calendarpanecontroller.h"
#include "contactspane.h"
#include "contactspanecontroller.h"
#include "mainwindowactioncontroller.h"
#include "mainwindowviewstate.h"
#include "paneshelldeps.h"
#include "preferences.h"
#include "preferencescontroller.h"
#include "preferencesdialog.h"
#include "statusbarpresenter.h"
#include "applicationcontext.h"
#include "collectioncatalog.h"
#include "windowsettingsstore.h"
#include "windowservices.h"
#include "futurewatch.h"

#include <QApplication>
#include <QDialog>
#include <QIcon>
#include <QKeySequence>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QSplitter>
#include <QTableView>
#include <QTabWidget>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include <utility>

namespace {

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

} // namespace

MainWindowComposition::MainWindowComposition() = default;
MainWindowComposition::~MainWindowComposition() = default;
MainWindowComposition::MainWindowComposition(MainWindowComposition &&) noexcept = default;
MainWindowComposition &MainWindowComposition::operator=(MainWindowComposition &&) noexcept = default;

class MainWindowCompositionBuilder
{
public:
    MainWindowCompositionBuilder(MainWindow &window, Ui::MainWindow &ui, ApplicationContext &applicationContext)
        : w(window)
        , ui(ui)
        , context(applicationContext)
    {}

    MainWindowCompositionBuilder(MainWindow &window,
                                 Ui::MainWindow &ui,
                                 ApplicationContext &applicationContext,
                                 MainWindowComposition &&partialComposition)
        : composition(std::move(partialComposition))
        , w(window)
        , ui(ui)
        , context(applicationContext)
    {}

    MainWindowComposition compose()
    {
        createControllers();
        setupShellActions();
        setupToolbar();
        return std::move(composition);
    }

    MainWindowComposition complete(const WindowServices &windowServices)
    {
        configurePanes(windowServices);
        wirePaneSignals();
        wirePreferenceSignals();
        buildCentralLayout();
        setupControllers();
        setupStatusPresenter(windowServices);
        wireTabWidgetSignals();
        loadStorageAndSettings(windowServices);
        setupInitialCalendarState(windowServices);
        return std::move(composition);
    }

private:
    MainWindowComposition composition;

    void createControllers()
    {
        composition.windowSettingsStore = std::make_unique<WindowSettingsStore>(&w);
        composition.preferencesController = std::make_unique<PreferencesController>(&w);
        composition.viewState = std::make_unique<MainWindowViewState>(&w);
        composition.reloadCoordinator = std::make_unique<ReloadCoordinator>(
            context.eventReader(), context.taskService(), context.contactService(), &w);
        composition.windowServices = std::make_unique<WindowServices>();
        configureWindowServices();
    }

    void configurePanes(const WindowServices &windowServices)
    {
        PaneShellDeps shellDeps{*composition.reloadCoordinator, *composition.preferencesController, windowServices};

        CalendarPane::Deps calendarDeps{shellDeps};
        composition.calendarPane = new CalendarPane(calendarDeps);
        replaceTabPlaceholder(ui.tabCalendar, composition.calendarPane, QStringLiteral("tabCalendar"));
        CalendarPaneController::Deps calendarControllerDeps{context.eventReader(),
                                                            context.eventService(),
                                                            context.taskService(),
                                                            context.collectionService(),
                                                            *composition.reloadCoordinator,
                                                            windowServices};
        composition.calendarPaneController =
            std::make_unique<CalendarPaneController>(composition.calendarPane, calendarControllerDeps, &w);

        ContactsPane::Deps contactsDeps{shellDeps};
        composition.contactsPane = new ContactsPane(contactsDeps);
        replaceTabPlaceholder(ui.tabContacts, composition.contactsPane, QStringLiteral("tabContacts"));
        ContactsPaneController::Deps contactsControllerDeps{
            context.contactService(), context.collectionService(), *composition.reloadCoordinator};
        composition.contactsPaneController =
            std::make_unique<ContactsPaneController>(composition.contactsPane, contactsControllerDeps, &w);
    }

    void configureWindowServices()
    {
        composition.windowServices->currentLocale = [preferences = composition.preferencesController.get()]() {
            return preferences ? preferences->locale() : QLocale::system();
        };
        composition.windowServices->collectionSummary = [&applicationContext = context](CollectionKind kind,
                                                                                        const QString &collectionId) {
            const std::shared_ptr<const CollectionCatalog> catalog = applicationContext.catalog();
            const QList<Collection> collections =
                kind == CollectionKind::Calendar ? catalog->calendarList() : catalog->addressBookList();
            for (const Collection &candidate : collections)
            {
                if (candidate.id == collectionId)
                {
                    return CollectionSummary{candidate.displayName, candidate.color};
                }
            }
            return CollectionSummary{collectionId, {}};
        };
    }

    void replaceTabPlaceholder(QWidget *placeholder, QWidget *replacement, const QString &objectName)
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

    void wirePaneSignals()
    {
        QObject::connect(composition.calendarPane,
                         &CalendarPane::selectedDateChanged,
                         composition.viewState.get(),
                         &MainWindowViewState::setSelectedDate);
        QObject::connect(composition.viewState.get(),
                         &MainWindowViewState::selectedDateChanged,
                         composition.calendarPane,
                         [calendarPane = composition.calendarPane](const QDate &date) {
                             if (calendarPane->selectedDate() != date)
                             {
                                 calendarPane->setSelectedDate(date);
                             }
                         });
        QObject::connect(composition.viewState.get(),
                         &MainWindowViewState::calendarSplitterStateChanged,
                         &w,
                         [calendarPane = composition.calendarPane,
                          viewState = composition.viewState.get(),
                          &window = w](const QByteArray &state) {
                             QSplitter *splitter = calendarPane->calendarSplitter();
                             auto restoreDefault = [calendarPane, viewState, &window]() {
                                 QTimer::singleShot(0, &window, [calendarPane, viewState]() {
                                     QSplitter *splitter = calendarPane->calendarSplitter();
                                     const int availableWidth = splitter->width();
                                     if (availableWidth <= 0)
                                     {
                                         return;
                                     }
                                     const int leftWidth = qMin(qMax(320, availableWidth / 3), 460);
                                     const QSignalBlocker blocker(splitter);
                                     splitter->setSizes({leftWidth, qMax(240, availableWidth - leftWidth)});
                                     if (viewState)
                                     {
                                         viewState->setCalendarSplitterState(splitter->saveState());
                                     }
                                 });
                             };

                             if (state.isEmpty())
                             {
                                 restoreDefault();
                                 return;
                             }

                             const QSignalBlocker blocker(splitter);
                             if (!splitter->restoreState(state))
                             {
                                 restoreDefault();
                             }
                         });
        QObject::connect(composition.viewState.get(),
                         &MainWindowViewState::contactDetailsVisibleChanged,
                         composition.contactsPane,
                         &ContactsPane::setContactDetailsVisible);
        QObject::connect(composition.viewState.get(),
                         &MainWindowViewState::taskPaneVisibleChanged,
                         composition.calendarPane,
                         &CalendarPane::setTaskPaneVisible);
    }

    void wirePreferenceSignals()
    {
        QObject::connect(composition.preferencesController.get(),
                         &PreferencesController::dayViewRangeChanged,
                         &w,
                         [calendarPane = composition.calendarPane](int startHour, int finishHour) {
                             calendarPane->setDayViewTimeRange(startHour, finishHour);
                         });
        QObject::connect(composition.preferencesController.get(),
                         &PreferencesController::applicationFontSizeChanged,
                         &w,
                         [&window = w](int fontSize) { window.applyApplicationFontSize(fontSize); });
    }

    void buildCentralLayout()
    {
        w.centralWidget()->layout()->setContentsMargins(0, 0, 0, 0);
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
        QObject::connect(composition.calendarPane->calendarSplitter(),
                         &QSplitter::splitterMoved,
                         &w,
                         [calendarPane = composition.calendarPane, viewState = composition.viewState.get()]() {
                             if (viewState)
                             {
                                 viewState->setCalendarSplitterState(calendarPane->calendarSplitter()->saveState());
                             }
                         });
    }

    void setupShellActions()
    {
        w.setWindowTitle(MainWindow::tr("LXQt Organizer"));
        w.setWindowIcon(QIcon::fromTheme(QStringLiteral("lxqt-organizer")));
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
        QObject::connect(ui.actionAbout, &QAction::triggered, &w, [&window = w]() { showAboutDialog(window); });
        QObject::connect(ui.actionPreferences,
                         &QAction::triggered,
                         &w,
                         [&window = w, preferences = composition.preferencesController.get()]() {
                             Q_ASSERT(preferences);
                             presentPreferencesDialog(window, *preferences);
                         });
    }

    void setupToolbar()
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

    void setupControllers()
    {
        composition.actionController =
            std::make_unique<MainWindowActionController>(&ui,
                                                         context.collectionService(),
                                                         *composition.viewState,
                                                         composition.preferencesController.get(),
                                                         composition.calendarPane,
                                                         composition.contactsPane,
                                                         &w);
        composition.actionController->setCommandPanes(composition.calendarPane, composition.contactsPane);
        composition.actionController->setActivePane(composition.calendarPane);
        QObject::connect(composition.actionController.get(), &MainWindowActionController::newWindowRequested, &w, []() {
            if (QMetaObject::invokeMethod(QApplication::instance(), "newWindow"))
            {
                return;
            }

            qFatal("LXQt Organizer application object does not provide newWindow()");
        });
        QObject::connect(composition.actionController.get(),
                         &MainWindowActionController::settingsSaveRequested,
                         &w,
                         [&window = w]() { window.saveSettings(); });
    }

    void setupStatusPresenter(const WindowServices &windowServices)
    {
        composition.statusPresenter = std::make_unique<StatusBarPresenter>(ui.statusBar, ui.itemCountLabel, &w);
        composition.statusPresenter->setLocaleProvider([services = &windowServices]() { return services->locale(); });
        composition.statusPresenter->setActivePane(composition.calendarPane);
        QObject::connect(composition.reloadCoordinator.get(),
                         &ReloadCoordinator::readFailures,
                         composition.statusPresenter.get(),
                         &StatusBarPresenter::showReadFailures);
        QObject::connect(
            composition.viewState.get(),
            &MainWindowViewState::selectedDateChanged,
            composition.statusPresenter.get(),
            static_cast<void (StatusBarPresenter::*)(const QDate &)>(&StatusBarPresenter::setSelectedDate));
        composition.statusPresenter->setSelectedDate(selectedDate());
    }

    void wireTabWidgetSignals()
    {
        QObject::connect(ui.tabWidget,
                         &QTabWidget::currentChanged,
                         &w,
                         [actionController = composition.actionController.get(),
                          statusPresenter = composition.statusPresenter.get(),
                          calendarPane = composition.calendarPane,
                          &uiRef = ui]() {
                             auto *active = dynamic_cast<IActivePane *>(uiRef.tabWidget->currentWidget());
                             if (actionController)
                             {
                                 actionController->setActivePane(active);
                             }
                             if (statusPresenter)
                             {
                                 statusPresenter->setActivePane(active);
                             }
                             calendarPane->clearCalendarFindStatus();
                         });
    }

    void loadStorageAndSettings(const WindowServices &windowServices)
    {
        composition.calendarPane->setTimelineCollectionSummaryProvider(
            [services = &windowServices](const QString &collectionId) {
                return services->summarizeCollection(CollectionKind::Calendar, collectionId);
            });
        loadSettings();
        FutureWatcher::watch(
            &w,
            context.reloadCollections(),
            [&window = w,
             &applicationContext = context,
             reloadCoordinator = composition.reloadCoordinator.get(),
             viewState = composition.viewState.get()](bool loaded) {
                const auto catalog = applicationContext.catalogSnapshot();
                if (!loaded || !catalog || (catalog->calendarList().isEmpty() && catalog->addressBookList().isEmpty()))
                {
                    QMessageBox::warning(&window,
                                         MainWindow::tr("Organizer"),
                                         MainWindow::tr("Could not initialize calendar and contact storage."));
                }
                if (reloadCoordinator)
                {
                    reloadCoordinator->reloadContacts();
                    const QDate date = viewState ? viewState->selectedDate() : QDate::currentDate();
                    const QDate monthStart(date.year(), date.month(), 1);
                    reloadCoordinator->setCalendarWindow(
                        monthStart, monthStart.addMonths(1).addDays(-1), QDate::currentDate());
                    reloadCoordinator->reloadCalendar();
                }
            });
    }

    void loadSettings()
    {
        const Preferences fallbackPreferences =
            composition.preferencesController ? composition.preferencesController->preferences() : Preferences();
        const WindowSettingsStore::Snapshot snapshot = composition.windowSettingsStore->load(fallbackPreferences);
        composition.settingsSnapshot = snapshot;
        if (composition.preferencesController)
        {
            composition.preferencesController->setPreferences(snapshot.preferences);
        }
        composition.windowSettingsStore->restoreWindowGeometry(&w, snapshot.geometry);
        if (composition.viewState)
        {
            composition.viewState->setContactDetailsVisible(snapshot.contactDetailsVisible);
            composition.viewState->setTaskPaneVisible(snapshot.taskPaneVisible);
            composition.viewState->setCalendarSplitterState(snapshot.calendarSplitterState);
        }
    }

    void setupInitialCalendarState(const WindowServices &windowServices)
    {
        const QDate initialDate = QDate::currentDate();
        if (composition.viewState)
        {
            composition.viewState->setSelectedDate(initialDate);
        }
        else
        {
            composition.calendarPane->setSelectedDate(initialDate);
        }
        composition.calendarPane->setTimelineLocale(windowServices.locale());
        composition.calendarPane->setDayViewTimeRange(
            composition.preferencesController->preferences().dayViewTimeStart,
            composition.preferencesController->preferences().dayViewTimeFinish);
        composition.calendarPane->setTaskPaneVisible(composition.viewState
                                                         ? composition.viewState->taskPaneVisible()
                                                         : composition.calendarPane->taskPaneVisible());
        restoreCalendarSplitter();
    }

    void restoreCalendarSplitter()
    {
        QSplitter *splitter = composition.calendarPane->calendarSplitter();
        const QByteArray state = composition.viewState ? composition.viewState->calendarSplitterState() : QByteArray();
        if (state.isEmpty())
        {
            restoreDefaultCalendarSplitter();
            return;
        }

        const QSignalBlocker blocker(splitter);
        if (!splitter->restoreState(state))
        {
            restoreDefaultCalendarSplitter();
        }
    }

    void restoreDefaultCalendarSplitter()
    {
        QTimer::singleShot(0, &w, [calendarPane = composition.calendarPane, viewState = composition.viewState.get()]() {
            QSplitter *splitter = calendarPane->calendarSplitter();
            const int availableWidth = splitter->width();
            if (availableWidth <= 0)
            {
                return;
            }
            const int leftWidth = qMin(qMax(320, availableWidth / 3), 460);
            const QSignalBlocker blocker(splitter);
            splitter->setSizes({leftWidth, qMax(240, availableWidth - leftWidth)});
            if (viewState)
            {
                viewState->setCalendarSplitterState(splitter->saveState());
            }
        });
    }

    QDate selectedDate() const
    {
        return composition.viewState ? composition.viewState->selectedDate() : QDate::currentDate();
    }

    MainWindow &w;
    Ui::MainWindow &ui;
    ApplicationContext &context;
};

MainWindowComposition composeMainWindow(MainWindow &window, Ui::MainWindow &ui, ApplicationContext &context)
{
    return MainWindowCompositionBuilder(window, ui, context).compose();
}

MainWindowComposition completeMainWindowComposition(MainWindowComposition &&composition,
                                                    MainWindow &window,
                                                    Ui::MainWindow &ui,
                                                    ApplicationContext &context,
                                                    const WindowServices &windowServices)
{
    return MainWindowCompositionBuilder(window, ui, context, std::move(composition)).complete(windowServices);
}
