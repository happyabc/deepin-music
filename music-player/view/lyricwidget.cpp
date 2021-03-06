/**
 * Copyright (C) 2016 Deepin Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 **/

#include "lyricwidget.h"

#include <QDebug>
#include <QLabel>
#include <QFile>
#include <QScrollArea>
#include <QPushButton>
#include <QHBoxLayout>
#include <QListView>
#include <QPainter>
#include <QResizeEvent>
#include <QPaintEvent>
#include <QStringListModel>
#include <QAbstractItemDelegate>
#include <QLineEdit>

#include <thememanager.h>

#include "../core/util/lyric.h"

#include "widget/cover.h"
#include "widget/searchmetalist.h"
#include "widget/searchmetaitem.h"
#include "widget/lyricview.h"

//DWIDGET_USE_NAMESPACE

static const int lyricLineHeight = 40;
static const QString defaultLyric = "No Lyric";

class LyricWidgetPrivate
{
public:
    LyricWidgetPrivate(LyricWidget *parent): q_ptr(parent) {}

    bool checkSearchMeta();
    void initConnection();
    void adjustLyric();
    void setLyricLines(QString lines);
    void showCover();
    void showSearch();

    MetaPtr             activingMeta;
    DMusic::SearchMeta  searchMeta;

    int                 m_emptyOffset = 0;
    int                 m_currentline = 0;
    Lyric               m_lyriclist;

    SearchMetaList      *searchMetaList     = nullptr;
    QPushButton         *m_showSearch       = nullptr;
    QPushButton         *m_exitSearch       = nullptr;
    QFrame              *searchMetaFrame    = nullptr;
    Cover               *m_cover            = nullptr;

    LyricView           *m_lyric            = nullptr;
    QStringListModel    *m_model            = nullptr;

    QString             defaultCover;

    LyricWidget *q_ptr;
    Q_DECLARE_PUBLIC(LyricWidget)
};

bool LyricWidgetPrivate::checkSearchMeta()
{
    return true;
}

void LyricWidgetPrivate::initConnection()
{
    Q_Q(LyricWidget);/*
    q->connect(m_hideLyric, &QPushButton::clicked,
               q, &LyricView::toggleLyricView);*/
}

void LyricWidgetPrivate::adjustLyric()
{
    Q_Q(LyricWidget);
    auto itemHeight = lyricLineHeight;
    auto contentHeight = q->rect().marginsRemoved(q->contentsMargins()).height();
    auto maxHeight = contentHeight * 92 / 100;
    if (m_model->rowCount() * itemHeight < maxHeight) {
        m_lyric->setFixedHeight(m_model->rowCount() * itemHeight);
    } else {
        m_lyric->setFixedHeight(maxHeight);
    }
}

void LyricWidgetPrivate::setLyricLines(QString str)
{
    Q_Q(LyricWidget);
    m_lyriclist = parseLrc(str);

    QStringList lines;
    if (!m_lyriclist.hasTime) {
        lines = str.split('\n');
    } else {
        for (auto &ele : m_lyriclist.m_lyricElements) {
            lines << ele.content;
        }
    }

    if (lines.length() <= 0) {
        lines << defaultLyric;
    }

    m_currentline = 0;
    auto itemHeight = lyricLineHeight;
    auto maxHeight = q->height() * 9 / 10;
    if (lines.length() > 2) {
        m_emptyOffset = maxHeight / itemHeight / 2;
    } else {
        m_emptyOffset = 0;
    }

    QStringList lyric;

    for (int i = 0; i < m_emptyOffset; ++i) {
        lyric << "";
    }
    for (auto line : lines) {
        lyric << line;
    }
    for (int i = 0; i < m_emptyOffset; ++i) {
        lyric << "";
    }
    m_model->setStringList(lyric);
    m_lyric->setModel(m_model);

    QModelIndex index = m_model->index(
                            m_emptyOffset + m_currentline, 0, m_lyric->rootIndex());
    m_lyric->clearSelection();
    m_lyric->setCurrentIndex(index);
    m_lyric->scrollTo(index, QListView::PositionAtCenter);
    m_lyric->clearSelection();

    adjustLyric();
}

LyricWidget::LyricWidget(QWidget *parent)
    : QFrame(parent), d_ptr(new LyricWidgetPrivate(this))
{
    Q_D(LyricWidget);

    setObjectName("LyricView");
    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 80);

    d->searchMetaFrame = new QFrame;
    d->searchMetaFrame->setObjectName("SearchMetaFrame");
    d->searchMetaFrame->setFixedWidth(300);
    auto searchMetaLayout = new QVBoxLayout(d->searchMetaFrame);
    searchMetaLayout->setSpacing(20);

    auto searchMetaHeader = new QLabel;
    searchMetaHeader->setObjectName("SearchMetaHeader");
    searchMetaHeader->setText(tr("Search Lyric"));

    auto searchMetaTitle = new QLineEdit;
    searchMetaTitle->setObjectName("SearchMetaTitle");
    searchMetaTitle->setPlaceholderText(tr("Title"));
    searchMetaTitle->setFixedHeight(34);

    auto searchMetaArtist = new QLineEdit;
    searchMetaArtist->setObjectName("SearchMetaArtist");
    searchMetaArtist->setPlaceholderText(tr("Artist"));
    searchMetaArtist->setFixedHeight(34);

    auto searchMetaButton = new QPushButton;
    searchMetaButton->setObjectName("SearchMetaButton");
    searchMetaButton->setText(tr("Search"));
    searchMetaButton->setFixedHeight(34);

    d->searchMetaList = new SearchMetaList;
    QSizePolicy spmetalist(QSizePolicy::Expanding, QSizePolicy::Expanding);
    spmetalist.setHorizontalStretch(1000);
    spmetalist.setVerticalStretch(1000);
    d->searchMetaList->setSizePolicy(spmetalist);

    searchMetaLayout->addWidget(searchMetaHeader);
    searchMetaLayout->addWidget(searchMetaTitle);
    searchMetaLayout->addWidget(searchMetaArtist);
    searchMetaLayout->addWidget(searchMetaButton);
    searchMetaLayout->addWidget(d->searchMetaList);

    layout->addWidget(d->searchMetaFrame, 0, Qt::AlignTop | Qt::AlignHCenter);

    d->m_cover = new Cover;
    d->m_cover->setFixedSize(200, 200);
    d->m_cover->setObjectName("LyricCover");

    d->m_lyric = new LyricView;
    d->m_lyric->setObjectName("LyricTextView");

    d->m_lyric->setSelectionMode(QListView::SingleSelection);
    d->m_lyric->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    d->m_lyric->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    d->m_lyric->setEditTriggers(QAbstractItemView::NoEditTriggers);
    d->m_lyric->setFlow(QListView::TopToBottom);

    d->m_model = new QStringListModel;
    d->setLyricLines("");

    d->m_lyric->setModel(d->m_model);

    d->m_showSearch = new QPushButton;
    d->m_showSearch->setObjectName("ShowSearch");

    d->m_exitSearch = new QPushButton;
    d->m_exitSearch->setObjectName("ExitSearch");
    d->m_exitSearch->setText(tr("Back"));

    auto btFrame = new QFrame;
    btFrame->setFixedWidth(50);
    auto btLayout = new QVBoxLayout(btFrame);
    btLayout->addWidget(d->m_showSearch, 0, Qt::AlignRight | Qt::AlignTop);
    btLayout->addWidget(d->m_exitSearch, 0, Qt::AlignRight | Qt::AlignTop);
    btLayout->addStretch();

    QSizePolicy spCover(QSizePolicy::Preferred, QSizePolicy::Preferred);
    spCover.setHorizontalStretch(80);
    d->m_cover->setSizePolicy(spCover);
    spCover.setVerticalStretch(1000);
    d->searchMetaFrame->setSizePolicy(spCover);

    layout->addWidget(d->m_cover, 0, Qt::AlignCenter);

    QSizePolicy spText(QSizePolicy::Preferred, QSizePolicy::Preferred);
    spText.setHorizontalStretch(20);
    spText.setVerticalStretch(100);
    d->m_lyric->setSizePolicy(spText);
    layout->addWidget(d->m_lyric, 0, Qt::AlignVCenter);

    QSizePolicy spBack(QSizePolicy::Preferred, QSizePolicy::Preferred);
    spBack.setHorizontalStretch(20);
    d->m_showSearch->setSizePolicy(spBack);
    d->m_showSearch->setFixedSize(24, 24);

    d->m_cover->show();
    d->searchMetaFrame->hide();
    d->m_showSearch->setDisabled(true);
    d->m_showSearch->show();
    d->m_exitSearch->hide();

    layout->addWidget(btFrame);

    ThemeManager::instance()->regisetrWidget(this);
    ThemeManager::instance()->regisetrWidget(d->m_lyric);

    connect(d->m_showSearch, &QPushButton::clicked,
    this, [ = ](bool) {
        Q_ASSERT(!d->activingMeta.isNull());
        searchMetaTitle->setText(d->activingMeta->title);
        searchMetaArtist->setText(d->activingMeta->artist);
        d->searchMetaList->clear();

        d-> m_cover->hide();
        d->searchMetaFrame->show();
        d->m_showSearch->hide();
        d->m_exitSearch->show();
    });

    connect(d->m_exitSearch, &QPushButton::clicked,
    this, [ = ](bool) {

        d->searchMetaFrame->hide();
        d->m_cover->show();
        d->m_showSearch->show();
        d->m_exitSearch->hide();
    });

    connect(searchMetaButton, &QPushButton::clicked,
    this, [ = ](bool) {
        auto context = searchMetaTitle->text() + " " + searchMetaArtist->text();
        emit requestContextSearch(context);
    });

    connect(d->searchMetaList, &SearchMetaList::itemClicked,
    this, [ = ](QListWidgetItem * item) {
        auto searchItem = qobject_cast<SearchMetaItem *>(d->searchMetaList->itemWidget(item));
        if (!searchItem) {
            qCritical() << "SearchMetaItem is empty" << item << searchItem;
            return;
        }
        // fixme:
        auto search = searchItem->property("musicMeta").value<DMusic::SearchMeta>();
        emit changeMetaCache(d->activingMeta, search);
    });

    connect(d->searchMetaList, &SearchMetaList::currentItemChanged,
    this, [ = ](QListWidgetItem * current, QListWidgetItem * previous) {
        auto itemWidget = qobject_cast<SearchMetaItem *>(d->searchMetaList->itemWidget(previous));
        if (itemWidget) {
            qDebug() << itemWidget;
            itemWidget->setChecked(false);
        }
        itemWidget = qobject_cast<SearchMetaItem *>(d->searchMetaList->itemWidget(current));
        if (itemWidget) {
            qDebug() << itemWidget;
            itemWidget->setChecked(true);
        }
    });


}

LyricWidget::~LyricWidget()
{

}

void LyricWidget::initUI()
{
    Q_D(LyricWidget);
    d->m_cover->setCoverPixmap(QPixmap(d->defaultCover));
    d->initConnection();
}

QString LyricWidget::defaultCover() const
{
    Q_D(const LyricWidget);
    return d->defaultCover;
}

void LyricWidget::checkHiddenSearch(QPoint mousePos)
{
    Q_D(LyricWidget);
    if (!this->isVisible() || !d->searchMetaFrame->isVisible()) {
        return;
    }

    auto geometry = d->searchMetaFrame->geometry().marginsAdded(QMargins(0, 0, 40, 40));
    auto btGeometry = d->m_exitSearch->rect();
    auto btPos = d->m_exitSearch->mapFromGlobal(QCursor::pos());
    qDebug() << btGeometry << btPos;
    if (!geometry.contains(mousePos) && !btGeometry.contains(btPos)) {
        d->searchMetaFrame->hide();
        d->m_cover->show();
        d->m_exitSearch->hide();
        d->m_showSearch->show();
    }
}

void LyricWidget::resizeEvent(QResizeEvent *event)
{
    Q_D(LyricWidget);
    QWidget::resizeEvent(event);

    d->m_lyric->setFixedWidth(event->size().width() * 45 / 100);
    d->adjustLyric();

    auto contentHeight = rect().marginsRemoved(contentsMargins()).height();
    auto maxHeight = contentHeight * 90 / 100;
    d->searchMetaFrame->setFixedHeight(maxHeight);
}

void LyricWidget::paintEvent(QPaintEvent *e)
{
    QFrame::paintEvent(e);

    return;
//    QPainter painter(this);

//    painter.setRenderHint(QPainter::Antialiasing);
//    painter.setRenderHint(QPainter::HighQualityAntialiasing);

//    QBrush brush(QColor(255, 255, 255, 25));
//    auto hcenter = d->m_lyric->y() + d->m_lyric->height() / 2;
//    auto xstart = d->m_lyric->x();
//    auto xend = d->m_lyric->x() + d->m_lyric->width();

//    QPainterPath path;
//    path.moveTo(xstart, hcenter - 4);
//    path.lineTo(xstart, hcenter + 5);
//    path.lineTo(xstart + 9, hcenter);
//    path.lineTo(xstart, hcenter - 4);

//    path.moveTo(xstart + 20, hcenter);
//    path.lineTo(xend - 20, hcenter);
//    path.lineTo(xend - 20, hcenter + 1);
//    path.lineTo(xstart + 20, hcenter + 1);
//    path.lineTo(xstart + 20, hcenter);

//    path.moveTo(xend, hcenter - 4);
//    path.lineTo(xend, hcenter + 5);
//    path.lineTo(xend - 9, hcenter);
//    path.lineTo(xend, hcenter - 4);

//    painter.fillPath(path, brush);
}


void LyricWidget::onMusicPlayed(PlaylistPtr playlist, const MetaPtr meta)
{
    Q_D(LyricWidget);
    Q_UNUSED(playlist);
    d->activingMeta = meta;
    d->m_showSearch->setDisabled(false);
}

void LyricWidget::onMusicStop(PlaylistPtr /*playlist*/, const MetaPtr meta)
{
    Q_D(LyricWidget);
    onLyricChanged(meta, DMusic::SearchMeta(), "");
    onCoverChanged(meta, DMusic::SearchMeta(), "");
    d->m_showSearch->setDisabled(true);
}

void LyricWidget::onProgressChanged(qint64 value, qint64 /*length*/)
{
    Q_D(LyricWidget);
    auto len = d->m_lyriclist.m_lyricElements.length();
    if (!d->m_lyriclist.hasTime) {
        return;
    }
    if (len <= 2) {
        return;
    }

    int i = 0;
    for (i = 0; i < len; ++i) {
        auto curEle = d->m_lyriclist.m_lyricElements.at(i);
        if (value < curEle.start) {
            break;
        }
    }
    QModelIndex index = d->m_model->index(
                            d->m_emptyOffset + i - 1, 0, d->m_lyric->rootIndex());
    d->m_lyric->clearSelection();
    d->m_lyric->setCurrentIndex(index);
    d->m_lyric->scrollTo(index, QListView::PositionAtCenter);
}

void LyricWidget::onLyricChanged(const MetaPtr meta, const DMusic::SearchMeta &search,  const QByteArray &lyricData)
{
    Q_D(LyricWidget);
    if (d->activingMeta != meta) {
        return;
    }
    d->searchMeta = search;

    auto lyricStr = QString::fromUtf8(lyricData);
    d->setLyricLines(lyricStr);
}

void LyricWidget::onCoverChanged(const MetaPtr meta,  const DMusic::SearchMeta &search, const QByteArray &coverData)
{
    Q_D(LyricWidget);
    if (d->activingMeta != meta) {
        return;
    }

    d->searchMeta = search;
    QPixmap coverPixmap = coverData.length() > 1024 ?
                          QPixmap::fromImage(QImage::fromData(coverData)) :
                          QPixmap(d->defaultCover);

    d->m_cover->setCoverPixmap(coverPixmap);
    d->m_cover->update();
}

void LyricWidget::setDefaultCover(QString defaultCover)
{
    Q_D(LyricWidget);
    d->defaultCover = defaultCover;
}

void LyricWidget::onUpdateMetaCodec(const MetaPtr meta)
{
    Q_D(LyricWidget);

//    if (d->m_playingMusic == meta) {
//        d->m_playingMusic.title = meta.title;
//        d->m_playingMusic.artist = meta.artist;
//        d->m_playingMusic.album = meta.album;
//    }
}

void LyricWidget::onContextSearchFinished(const QString &context, const QList<DMusic::SearchMeta> &metalist)
{
    Q_D(LyricWidget);
    d->searchMetaList->clear();

//    qDebug() << d->lyricSearchMeta.searchID;
//    qDebug() << d->coverSearchMeta.searchID;

    QListWidgetItem *current = nullptr;
    for (auto &meta : metalist) {
        ;
        auto item = new QListWidgetItem;
        auto itemWidget = new SearchMetaItem;
        itemWidget->initUI(meta);
        if (d->searchMeta.id == meta.id) {
            current = item;
        }
        itemWidget->setProperty("musicMeta", QVariant::fromValue<DMusic::SearchMeta>(meta));
        d->searchMetaList->addItem(item);
        d->searchMetaList->setItemWidget(item, itemWidget);
    }
    if (current) {
        d->searchMetaList->setCurrentItem(current);
    }
}


