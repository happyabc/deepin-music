/**
 * Copyright (C) 2016 Deepin Technology Co., Ltd.
 *
 * q program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 **/

#include "mainframe.h"

#include <QDebug>
#include <QAction>
#include <QProcess>
#include <QStackedLayout>
#include <QStyleFactory>
#include <QFileDialog>
#include <QStandardPaths>
#include <QApplication>
#include <QKeyEvent>

#include <DUtil>
#include <dutility.h>
#include <dthememanager.h>
#include <daboutdialog.h>
#include <dsettingsdialog.h>
#include <ddialog.h>

#include "../presenter/presenter.h"
#include "../core/metasearchservice.h"
#include "../core/settings.h"
#include "../core/player.h"
#include "../musicapp.h"

#include "widget/titlebarwidget.h"
#include "widget/infodialog.h"
#include "helper/widgethellper.h"
#include "helper/thememanager.h"

#include "titlebar.h"
#include "importwidget.h"
#include "musiclistwidget.h"
#include "playlistwidget.h"
#include "lyricwidget.h"
#include "footer.h"

const QString s_PropertyViewname = "viewname";
const QString s_PropertyViewnameLyric = "lyric";
static const int FooterHeight = 60;
static const int AnimationDelay = 400; //ms

class MainFramePrivate
{
public:
    MainFramePrivate(MainFrame *parent) : q_ptr(parent) {}

    void initUI();
    void initMenu();

    void setPlaylistVisible(bool visible);
    void toggleLyricView();
    void togglePlaylist();
    void slideToImportView();
    void slideToLyricView();
    void slideToMusicListView(bool keepPlaylist);
    void disableControl(int delay = 350);
    void updateViewname(const QString &vm);

    //! ui: show info dialog
    void showInfoDialog(const MetaPtr meta);


    QWidget         *currentWidget  = nullptr;
    Titlebar        *titlebar       = nullptr;
    TitleBarWidget  *titlebarwidget = nullptr;
    ImportWidget    *importWidget   = nullptr;
    MusicListWidget *musicList      = nullptr;
    LyricWidget       *lyricWidget    = nullptr;
    PlaylistWidget  *playlistWidget = nullptr;
    Footer          *footer         = nullptr;

    InfoDialog      *infoDialog     = nullptr;

    QAction         *newSonglistAction      = nullptr;
    QAction         *colorModeAction        = nullptr;
    QString         coverBackground         = ":/common/image/cover_max.png";
    QString         viewname                = "";

    MainFrame *q_ptr;
    Q_DECLARE_PUBLIC(MainFrame)
};

void MainFramePrivate::initMenu()
{
    Q_Q(MainFrame);
    newSonglistAction = new QAction(MainFrame::tr("New playlist"), q);
    q->connect(newSonglistAction, &QAction::triggered, q, [ = ](bool) {
//        qDebug() << "" <<  newSonglistAction;
//        setPlaylistVisible(true);
//        emit q->addPlaylist(true);
    });

    auto addmusic = new QAction(MainFrame::tr("Add music"), q);
    q->connect(addmusic, &QAction::triggered, q, [ = ](bool) {
        q->onSelectImportDirectory();
    });

    auto addmusicfiles = new QAction(MainFrame::tr("Add music file"), q);
    q->connect(addmusicfiles, &QAction::triggered, q, [ = ](bool) {
        q->onSelectImportFiles();
    });

    auto settings = new QAction(MainFrame::tr("Settings"), q);
    q->connect(settings, &QAction::triggered, q, [ = ](bool) {
        auto configDialog = new Dtk::Widget::DSettingsDialog(q);
        configDialog->setStyle(QStyleFactory::create("dlight"));
        configDialog->setFixedSize(720, 520);
        configDialog->updateSettings(Settings::instance());
        Dtk::Widget::DUtility::moveToCenter(configDialog);
        configDialog->exec();
        Settings::instance()->sync();
    });

    colorModeAction = new QAction(MainFrame::tr("Deep color mode"), q);
    colorModeAction->setCheckable(true);
    colorModeAction->setChecked(Settings::instance()->value("base.play.theme").toString() == "dark");

    q->connect(colorModeAction, &QAction::triggered, q, [ = ](bool) {
        if (Dtk::Widget::DThemeManager::instance()->theme() == "light") {
            colorModeAction->setChecked(true);
            Dtk::Widget::DThemeManager::instance()->setTheme("dark");
            ThemeManager::instance()->setTheme("dark");
        } else {
            colorModeAction->setChecked(false);
            Dtk::Widget::DThemeManager::instance()->setTheme("light");
            ThemeManager::instance()->setTheme("light");
        }
        Settings::instance()->setOption("base.play.theme", Dtk::Widget::DThemeManager::instance()->theme());
    });

    auto about = new QAction(MainFrame::tr("About"), q);
    q->connect(about, &QAction::triggered, q, [ = ](bool) {
        QString descriptionText = MainFrame::tr("Deepin Music Player is a beautiful design and "
                                                "simple function local music player. "
                                                "It supports viewing lyrics when playing, "
                                                "playing lossless music and creating customizable songlist, etc.");
        QString acknowledgementLink = "https://www.deepin.org/acknowledgments/deepin-music#thanks";

        auto *aboutDlg = new Dtk::Widget::DAboutDialog(q);
        aboutDlg->setWindowModality(Qt::WindowModal);
        aboutDlg->setWindowIcon(QPixmap("::/common/image/logo.png"));
        aboutDlg->setProductIcon(QPixmap(":/common/image/logo_96.png"));
        aboutDlg->setProductName("Deepin Music");
        aboutDlg->setVersion(MainFrame::tr("Version: 3.0"));
        aboutDlg->setDescription(descriptionText + "\n");
        aboutDlg->setAcknowledgementLink(acknowledgementLink);
        aboutDlg->show();
    });

    QAction *help = new QAction(MainFrame::tr("Help"), q);
    q->connect(help, &QAction::triggered,
    q, [ = ](bool) {
        static QProcess *m_manual = nullptr;
        if (NULL == m_manual) {
            m_manual =  new QProcess(q);
            const QString pro = "dman";
            const QStringList args("deepin-music");
            m_manual->connect(m_manual, static_cast<void(QProcess::*)(int)>(&QProcess::finished), q, [ = ](int) {
                m_manual->deleteLater();
                m_manual = nullptr;
            });
            m_manual->start(pro, args);
        }
    });

    QAction *m_close = new QAction(MainFrame::tr("Quit"), q);
    q->connect(m_close, &QAction::triggered, q, [ = ](bool) {
        q->close();
    });

    auto titleMenu = new QMenu;
    titleMenu->setStyle(QStyleFactory::create("dlight"));

    titleMenu->addAction(newSonglistAction);
    titleMenu->addAction(addmusic);
    titleMenu->addAction(addmusicfiles);
    titleMenu->addSeparator();

    titleMenu->addAction(colorModeAction);
    titleMenu->addAction(settings);
    titleMenu->addSeparator();

    titleMenu->addAction(about);
    titleMenu->addAction(help);
    titleMenu->addAction(m_close);

    titlebar->setMenu(titleMenu);
}

void MainFramePrivate::initUI()
{
    Q_Q(MainFrame);

    q->setFocusPolicy(Qt::ClickFocus);
    q->setObjectName("PlayerFrame");

    titlebar = new Titlebar;
    titlebar->setObjectName("MainWindowTitlebar");
    titlebarwidget = new TitleBarWidget;
    titlebarwidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
//    titlebar->setFixedHeight(titleBarHeight);
    titlebar->setCustomWidget(titlebarwidget , Qt::AlignLeft, false);

    importWidget = new ImportWidget;
    musicList = new MusicListWidget;
    musicList->setContentsMargins(0, titlebar->height(), 0, FooterHeight);

    lyricWidget = new LyricWidget;
    lyricWidget->setContentsMargins(0, titlebar->height(), 0, FooterHeight);

    playlistWidget = new PlaylistWidget;

    footer = new Footer;
    footer->setFixedHeight(FooterHeight);

    auto contentLayout = new QStackedLayout();
    q->setContentLayout(contentLayout);
    contentLayout->setContentsMargins(20, 20, 20, 20);
    contentLayout->setMargin(0);
    contentLayout->setSpacing(0);

    contentLayout->addWidget(titlebar);
    contentLayout->addWidget(importWidget);
    contentLayout->addWidget(musicList);
    contentLayout->addWidget(lyricWidget);
    contentLayout->addWidget(playlistWidget);
    contentLayout->addWidget(footer);

    infoDialog = new InfoDialog;
    infoDialog->setStyle(QStyleFactory::create("dlight"));
    infoDialog->hide();

    titlebarwidget->setSearchEnable(false);
    importWidget->show();
    footer->show();
    footer->setFocus();
}

void MainFramePrivate::slideToLyricView()
{
    Q_Q(MainFrame);

    auto current = currentWidget ? currentWidget : musicList;
//    lyricView->resize(current->size());

    WidgetHelper::slideBottom2TopWidget(
        current,  lyricWidget, AnimationDelay);

//    q->disableControl();
//    setPlaylistVisible(false);
    currentWidget = lyricWidget;
    titlebar->raise();
    footer->raise();

    updateViewname(s_PropertyViewnameLyric);
}

void MainFramePrivate:: slideToImportView()
{
    Q_Q(MainFrame);

    if (importWidget->isVisible()) {
        importWidget->showImportHint();
        return;
    }

//    setPlaylistVisible(false);
    auto current = currentWidget ? currentWidget : musicList;
    importWidget->showImportHint();
    importWidget->setFixedSize(current->size());

    qDebug() << "show importWidget" << current << importWidget;

    WidgetHelper::slideRight2LeftWidget(
        current, importWidget, AnimationDelay);
    footer->enableControl(false);
    currentWidget = importWidget;
    titlebar->raise();
    footer->raise();

    titlebarwidget->setSearchEnable(false);
    newSonglistAction->setDisabled(true);
    updateViewname("");
}

void MainFramePrivate:: slideToMusicListView(bool keepPlaylist)
{
    Q_Q(MainFrame);

    auto current = currentWidget ? currentWidget : importWidget;
    qDebug() << "changeToMusicListView"
             << current << musicList << keepPlaylist;

    if (musicList->isVisible()) {
        musicList->raise();
        titlebar->raise();
        footer->raise();
//        setPlaylistVisible(keepPlaylist);
        return;
    }
    musicList->setFixedSize(current->size());
    WidgetHelper::slideTop2BottomWidget(
        current, musicList, AnimationDelay);
    q->update();
//    q->disableControl();
    currentWidget = musicList;
//    setPlaylistVisible(keepPlaylist);
    titlebar->raise();
    footer->raise();

    titlebarwidget->setSearchEnable(true);
    newSonglistAction->setDisabled(false);

    updateViewname("");
}

void MainFramePrivate::toggleLyricView()
{
    if (lyricWidget->isVisible()) {
        slideToMusicListView(false);
    } else {
        slideToLyricView();
    }
}

void MainFramePrivate::togglePlaylist()
{
    setPlaylistVisible(!playlistWidget->isVisible());
}

void MainFramePrivate::setPlaylistVisible(bool visible)
{
    Q_Q(MainFrame);
    if (playlistWidget->isVisible() == visible) {
        if (visible) {
            playlistWidget->setFocus();
            playlistWidget->show();
            playlistWidget->raise();
        }
        return;
    }

    auto ismoving = playlistWidget->property("moving").toBool();
    if (ismoving) {
        return;
    }

    playlistWidget->setProperty("moving", true);
    auto titleBarHeight = titlebar->height();

    double factor = 0.6;
    QRect start(q->width(), titleBarHeight,
                playlistWidget->width(), playlistWidget->height());
    QRect end(q->width() - playlistWidget->width() - q->shadowWidth() * 2, titleBarHeight,
              playlistWidget->width(), playlistWidget->height());
    if (!visible) {
        WidgetHelper::slideEdgeWidget(playlistWidget, end, start, AnimationDelay * factor, true);
        footer->setFocus();
    } else {
        playlistWidget->setFocus();
        WidgetHelper::slideEdgeWidget(playlistWidget, start, end, AnimationDelay * factor);
        playlistWidget->raise();
    }
    disableControl(AnimationDelay * factor);
    titlebar->raise();
    footer->raise();

    QTimer::singleShot(AnimationDelay * factor*1, q, [ = ]() {
        playlistWidget->setProperty("moving", false);
    });
}

void MainFramePrivate::disableControl(int delay)
{
    Q_Q(MainFrame);
    footer->enableControl(false);
    QTimer::singleShot(delay, q, [ = ]() {
        footer->enableControl(true);
    });
}

void MainFramePrivate::updateViewname(const QString &vm)
{
    Q_Q(MainFrame);
    DUtil::TimerSingleShot(AnimationDelay / 2, [this, q, vm]() {
//        q->setViewname(vm);
        titlebar->setViewname(vm);
        titlebarwidget->setViewname(vm);
    });
}

void MainFramePrivate::showInfoDialog(const MetaPtr meta)
{
    infoDialog->show();
    infoDialog->updateInfo(meta);
}

MainFrame::MainFrame(QWidget *parent) :
    ThinWindow(parent), d_ptr(new MainFramePrivate(this))
{
    Q_D(MainFrame);
    ThemeManager::instance()->regisetrWidget(this, QStringList() << s_PropertyViewname);

    d->initUI();
    d->initMenu();
}

MainFrame::~MainFrame()
{

}

void MainFrame::binding(Presenter *presenter)
{
    Q_D(MainFrame);

    connect(this, &MainFrame::importSelectFiles, presenter, &Presenter::onImportFiles);

    connect(d->titlebar, &Titlebar::mouseMoving, this, &MainFrame::moveWindow);
    connect(d->footer, &Footer::mouseMoving, this, &MainFrame::moveWindow);

    connect(d->titlebarwidget, &TitleBarWidget::locateMusicInAllMusiclist,
            presenter, &Presenter::onLocateMusicAtAll);
    connect(d->titlebarwidget, &TitleBarWidget::search,
            presenter, &Presenter::onSearchText);
    connect(d->titlebarwidget, &TitleBarWidget::exitSearch,
            presenter, &Presenter::onExitSearch);

    connect(d->importWidget, &ImportWidget::scanMusicDirectory,
            presenter, &Presenter::onScanMusicDirectory);
    connect(d->importWidget, &ImportWidget::importFiles,
            this, &MainFrame::onSelectImportDirectory);
    connect(d->importWidget, &ImportWidget::importSelectFiles,
    this, [ = ](const QStringList & urllist) {
        d->importWidget->showWaitHint();
        emit importSelectFiles(urllist);
    });

    connect(presenter, &Presenter::showMusicList,
    this, [ = ](PlaylistPtr playlist) {
        auto current = d->currentWidget ? d->currentWidget : d->importWidget;
        d->musicList->resize(current->size());
        d->musicList->show();
        d->importWidget->hide();
        d->musicList->onMusiclistChanged(playlist);
        d->disableControl(false);
        d->titlebarwidget->setSearchEnable(true);
    });

    connect(presenter, &Presenter::coverSearchFinished,
    this, [ = ](const MetaPtr, const DMusic::SearchMeta &, const QByteArray & coverData) {
        if (coverData.length() < 32) {
            setCoverBackground(coverBackground());
            this->update();
            return;
        }
        QImage image = QImage::fromData(coverData);
        if (image.isNull()) {
            setCoverBackground(coverBackground());
            this->update();
            return;
        }

        image = WidgetHelper::cropRect(image, this->size());
        setBackgroundImage(WidgetHelper::blurImage(image, 50));
        this->update();
    });

    connect(presenter, &Presenter::musicStoped,
    this, [ = ](PlaylistPtr, const MetaPtr) {
        setCoverBackground(coverBackground());
    });

    connect(presenter, &Presenter::notifyMusciError,
    this, [ = ](PlaylistPtr playlist, const MetaPtr  meta, int /*error*/) {
        Dtk::Widget::DDialog warnDlg(this);
        warnDlg.setIcon(QIcon(":/common/image/dialog_warning.png"));
        warnDlg.setTextFormat(Qt::RichText);
        warnDlg.setTitle(tr("File invalid or does not exist, load failed!"));
        warnDlg.addButtons(QStringList() << tr("I got it"));
        if (0 == warnDlg.exec()) {
            if (playlist->canNext()) {
                emit d->footer->next(playlist, meta);
            }
        }
    });

    connect(presenter, &Presenter::metaLibraryClean,
    this, [ = ]() {
        qDebug() << "metaLibraryClean ----------------";
        d->slideToImportView();
        d->titlebarwidget->clearSearch();
    });

    connect(presenter, &Presenter::scanFinished,
    this, [ = ](const QString & /*jobid*/, int mediaCount) {
        qDebug() << "scanFinished----------------";
        if (0 == mediaCount) {
            QString message = QString(tr("No local music"));
            Dtk::Widget::DDialog warnDlg;
            warnDlg.setIcon(QIcon(":/common/image/dialog_warning.png"));
            warnDlg.setTextFormat(Qt::AutoText);
            warnDlg.setTitle(message);
            warnDlg.addButtons(QStringList() << tr("Ok"));
            warnDlg.setDefaultButton(0);
            if (0 == warnDlg.exec()) {
                return;
            }
        }
    });

    connect(presenter, &Presenter::musicPlayed,
    this, [ = ](PlaylistPtr playlist, const MetaPtr meta) {
        Q_ASSERT(!playlist.isNull());
        Q_ASSERT(!meta.isNull());

        qApp->setApplicationDisplayName(playlist->displayName());
        this->setWindowTitle(meta->title);
    });

    connect(presenter, &Presenter::meidaFilesImported,
    this, [ = ](PlaylistPtr playlist, const MetaPtrList) {
        d->slideToMusicListView(false);
        d->musicList->onMusiclistChanged(playlist);
    });


    connect(d->musicList, &MusicListWidget::showInfoDialog,
    this, [ = ](const MetaPtr meta) {
        d->showInfoDialog(meta);
    });

    // MusicList
    connect(d->musicList, &MusicListWidget::playall,
            presenter, &Presenter::onPlayall);
    connect(d->musicList, &MusicListWidget::resort,
            presenter, &Presenter::onResort);
    connect(d->musicList, &MusicListWidget::playMedia,
            presenter, &Presenter::onSyncMusicPlay);
    connect(d->musicList, &MusicListWidget::updateMetaCodec,
            presenter, &Presenter::onUpdateMetaCodec);
    connect(d->musicList, &MusicListWidget::requestCustomContextMenu,
            presenter, &Presenter::onRequestMusiclistMenu);
    connect(d->musicList, &MusicListWidget::addToPlaylist,
            presenter, &Presenter::onAddToPlaylist);
    connect(d->musicList, &MusicListWidget::musiclistRemove,
            presenter, &Presenter::onMusiclistRemove);
    connect(d->musicList, &MusicListWidget::musiclistDelete,
            presenter, &Presenter::onMusiclistDelete);

    connect(d->musicList, &MusicListWidget::importSelectFiles,
    this, [ = ](PlaylistPtr playlist, QStringList urllist) {
        presenter->requestImportPaths(playlist, urllist);
    });

    connect(presenter, &Presenter::musicListResorted,
            d->musicList, &MusicListWidget::onMusiclistChanged);
    connect(presenter, &Presenter::requestMusicListMenu,
            d->musicList,  &MusicListWidget::onCustomContextMenuRequest);
    connect(presenter, &Presenter::currentMusicListChanged,
            d->musicList,  &MusicListWidget::onMusiclistChanged);
    connect(presenter, &Presenter::musicPlayed,
            d->musicList,  &MusicListWidget::onMusicPlayed);
    connect(presenter, &Presenter::musicError,
            d->musicList,  &MusicListWidget::onMusicError);
    connect(presenter, &Presenter::musicListAdded,
            d->musicList,  &MusicListWidget::onMusicListAdded);
    connect(presenter, &Presenter::musicListRemoved,
            d->musicList,  &MusicListWidget::onMusicListRemoved);
    connect(presenter, &Presenter::locateMusic,
            d->musicList,  &MusicListWidget::onLocate);

    connect(presenter, &Presenter::musicPlayed,
            d->lyricWidget, &LyricWidget::onMusicPlayed);
    connect(presenter, &Presenter::coverSearchFinished,
            d->lyricWidget, &LyricWidget::onCoverChanged);
    connect(presenter, &Presenter::lyricSearchFinished,
            d->lyricWidget, &LyricWidget::onLyricChanged);
    connect(presenter, &Presenter::contextSearchFinished,
            d->lyricWidget, &LyricWidget::onContextSearchFinished);


    connect(d->lyricWidget,  &LyricWidget::requestContextSearch,
            presenter, &Presenter::requestContextSearch);
    connect(d->lyricWidget, &LyricWidget::changeMetaCache,
            presenter, &Presenter::onChangeSearchMetaCache);


    // footer
    connect(d->footer, &Footer::toggleLyricView,
    this, [ = ]() {
        d->toggleLyricView();
    });
    connect(d->footer, &Footer::togglePlaylist,
    this, [ = ]() {
        d->togglePlaylist();
    });

    connect(d->footer,  &Footer::locateMusic,
            d->musicList, &MusicListWidget::onLocate);
    connect(d->footer,  &Footer::play,
            presenter, &Presenter::onSyncMusicPlay);
    connect(d->footer,  &Footer::resume,
            presenter, &Presenter::onMusicResume);
    connect(d->footer,  &Footer::pause,
            presenter, &Presenter::onMusicPause);
    connect(d->footer,  &Footer::next,
            presenter, &Presenter::onMusicNext);
    connect(d->footer,  &Footer::prev,
            presenter, &Presenter::onMusicPrev);
    connect(d->footer,  &Footer::changeProgress,
            presenter, &Presenter::onChangeProgress);
    connect(d->footer,  &Footer::volumeChanged,
            presenter, &Presenter::onVolumeChanged);
    connect(d->footer,  &Footer::toggleMute,
            presenter, &Presenter::onToggleMute);
    connect(d->footer,  &Footer::modeChanged,
            presenter, &Presenter::onPlayModeChanged);
    connect(d->footer,  &Footer::toggleFavourite,
            presenter, &Presenter::onToggleFavourite);

    connect(presenter, &Presenter::modeChanged,
            d->footer,  &Footer::onModeChange);
    connect(presenter, &Presenter::musicListAdded,
            d->footer,  &Footer::onMusicListAdded);
    connect(presenter, &Presenter::musicListRemoved,
            d->footer,  &Footer::onMusicListRemoved);
    connect(presenter, &Presenter::musicPlayed,
            d->footer,  &Footer::onMusicPlayed);
    connect(presenter, &Presenter::musicPaused,
            d->footer,  &Footer::onMusicPause);
    connect(presenter, &Presenter::musicStoped,
            d->footer,  &Footer::onMusicStoped);
    connect(presenter, &Presenter::progrossChanged,
            d->footer,  &Footer::onProgressChanged);
    connect(presenter, &Presenter::coverSearchFinished,
            d->footer,  &Footer::onCoverChanged);
    connect(presenter, &Presenter::volumeChanged,
            d->footer,  &Footer::onVolumeChanged);
    connect(presenter, &Presenter::mutedChanged,
            d->footer,  &Footer::onMutedChanged);
//    connect(presenter, &Presenter::,
//            d->footer,  &Footer::onUpdateMetaCodec);
//    connect(presenter, &Presenter::coverSearchFinished,
//            d->footer,  &Footer::setDefaultCover);

    // playlist
    connect(presenter, &Presenter::playlistAdded,
            d->playlistWidget,  &PlaylistWidget::onPlaylistAdded);
    connect(presenter, &Presenter::musicPlayed,
            d->playlistWidget,  &PlaylistWidget::onMusicPlayed);
    connect(presenter, &Presenter::currentMusicListChanged,
            d->playlistWidget,  &PlaylistWidget::onCurrentChanged);

    connect(d->playlistWidget,  &PlaylistWidget::addPlaylist,
            presenter, &Presenter::onPlaylistAdd);
    connect(d->playlistWidget,  &PlaylistWidget::selectPlaylist,
            presenter, &Presenter::onCurrentPlaylistChanged);
    connect(d->playlistWidget,  &PlaylistWidget::playall,
            presenter, &Presenter::onPlayall);
    connect(d->playlistWidget,  &PlaylistWidget::hidePlaylist,
    this, [ = ]() {
        d->setPlaylistVisible(false);
    });
}

void MainFrame::focusMusicList()
{
    Q_D(const MainFrame);
    d->musicList->setFocus();
}

QString MainFrame::coverBackground() const
{
    Q_D(const MainFrame);
    return d->coverBackground;
}

void MainFrame::setCoverBackground(QString coverBackground)
{
    Q_D(MainFrame);
    d->coverBackground = coverBackground;
    QImage image = QImage(coverBackground);
    image = WidgetHelper::cropRect(image, QWidget::size());
    setBackgroundImage(WidgetHelper::blurImage(image, 50));
}

void MainFrame::onSelectImportDirectory()
{
    Q_D(const MainFrame);
    QFileDialog fileDlg(this);

    auto lastImportPath = Settings::instance()->value("base.play.last_import_path").toString();

    auto lastImportDir = QDir(lastImportPath);
    if (!lastImportDir.exists()) {
        lastImportPath =  QStandardPaths::standardLocations(QStandardPaths::MusicLocation).first();
    }
    fileDlg.setDirectory(lastImportPath);

    fileDlg.setViewMode(QFileDialog::Detail);
    fileDlg.setFileMode(QFileDialog::Directory);
    if (QFileDialog::Accepted == fileDlg.exec()) {
        d->importWidget->showWaitHint();
        Settings::instance()->setOption("base.play.last_import_path",  fileDlg.directory().path());
        emit importSelectFiles(fileDlg.selectedFiles());
    }
}

void MainFrame::onSelectImportFiles()
{
    Q_D(const MainFrame);
    QFileDialog fileDlg(this);

    auto lastImportPath = Settings::instance()->value("base.play.last_import_path").toString();

    auto lastImportDir = QDir(lastImportPath);
    if (!lastImportDir.exists()) {
        lastImportPath =  QStandardPaths::standardLocations(QStandardPaths::MusicLocation).first();
    }
    fileDlg.setDirectory(lastImportPath);

    QString selfilter = tr("Music (%1)");
    selfilter = selfilter.arg(Player::instance()->supportedSuffixList().join(" "));
    fileDlg.setViewMode(QFileDialog::Detail);
    fileDlg.setFileMode(QFileDialog::ExistingFiles);

    fileDlg.setNameFilter(selfilter);
    fileDlg.selectNameFilter(selfilter);
    if (QFileDialog::Accepted == fileDlg.exec()) {
        d->importWidget->showWaitHint();
        Settings::instance()->setOption("base.play.last_import_path",  fileDlg.directory().path());
        emit importSelectFiles(fileDlg.selectedFiles());
    }
}

bool MainFrame::eventFilter(QObject *obj, QEvent *e)
{
    Q_D(const MainFrame);

    if (e->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(e);

        auto keyModifiers = ke->modifiers();
        auto key = static_cast<Qt::Key>(ke->key());

        QStringList sclist;
        sclist << "shortcuts.all.next"
               << "shortcuts.all.play_pause"
               << "shortcuts.all.previous"
               << "shortcuts.all.volume_down"
               << "shortcuts.all.volume_up";

        for (auto optkey : sclist) {
            auto shortcut = Settings::instance()->value(optkey).toStringList();
            auto modifiersstr = shortcut.value(0);
            auto scmodifiers = static_cast<Qt::KeyboardModifier>(modifiersstr.toInt());
            auto keystr = shortcut.value(1);
            auto sckey = static_cast<Qt::Key>(keystr.toInt());

            if (scmodifiers == keyModifiers && key == sckey && !ke->isAutoRepeat()) {
                qDebug() << "match " << optkey << ke->count() << ke->isAutoRepeat();
                MusicApp::instance()->triggerShortcutAction(optkey);
                return true;
            }
        }
    }
    if (e->type() == QEvent::MouseButtonPress) {
        QMouseEvent *me = static_cast<QMouseEvent *>(e);
        //        qDebug() << obj << me->pos();
        if (obj->objectName() == this->objectName() || this->objectName() + "Window" == obj->objectName()) {
            //            qDebug() << me->pos() << QCursor::pos();
            QPoint mousePos = me->pos();
            auto geometry = d->playlistWidget->geometry().marginsAdded(QMargins(0, 0, 40, 40));
            //            qDebug() << geometry << mousePos;
            if (!geometry.contains(mousePos)) {
//                qDebug() << "hide playlist" << me->pos() << QCursor::pos() << obj;
                DUtil::TimerSingleShot(50, [this]() {
                    this->d_func()->setPlaylistVisible(false);
                });
            }
        }
    }

    if (e->type() == QEvent::MouseButtonPress) {
        QMouseEvent *me = static_cast<QMouseEvent *>(e);
        //        qDebug() << obj << me->pos();
        if (obj->objectName() == this->objectName() || this->objectName() + "Window" == obj->objectName()) {
            QPoint mousePos = me->pos();
//            qDebug() << "lyricView checkHiddenSearch" << me->pos() << QCursor::pos() << obj;
            d->lyricWidget->checkHiddenSearch(mousePos);
        }

    }

//    if (e->type() == QEvent::Close) {
//        if (obj->objectName() == this->objectName()) {
//            exit(0);
//        }
//    }
    return qApp->eventFilter(obj, e);


}

void MainFrame::resizeEvent(QResizeEvent *e)
{
    Q_D(const MainFrame);

    ThinWindow::resizeEvent(e);
    QSize newSize = ThinWindow::size();

    auto titleBarHeight =  d->titlebar->height();
    d->titlebar->raise();
    d->titlebar->move(0, 0);
    d->titlebar->resize(newSize.width(), titleBarHeight);
    d->titlebarwidget->setFixedSize(newSize.width() - d->titlebar->buttonAreaWidth() - 60, titleBarHeight);

    d->importWidget->resize(newSize);

    d->lyricWidget->resize(newSize);
    d->musicList->setFixedSize(newSize);

    d->playlistWidget->setFixedSize(220, newSize.height() - FooterHeight - titleBarHeight);

    d->footer->raise();
    d->footer->resize(newSize.width(), FooterHeight);
    d->footer->move(0, newSize.height() - FooterHeight);

//    if (d->tips) {
//        d->tips->hide();
    //    }
}

void MainFrame::closeEvent(QCloseEvent *event)
{
    Settings::instance()->setOption("base.play.geometry", saveGeometry());
    Settings::instance()->setOption("base.play.state", int(windowState()));
    Settings::instance()->sync();

    qDebug() << this->geometry() << this->windowState();
    DUtil::TimerSingleShot(300, [this, event]() {
        ThinWindow::closeEvent(event);
    });
}
