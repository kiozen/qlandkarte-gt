/**********************************************************************************************
    Copyright (C) 2012 Oliver Eichler oliver.eichler@gmx.de

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

**********************************************************************************************/

#include "CTrackFilterWidget.h"
#include "CTrackDB.h"
#include "CTrack.h"
#include "CTrackEditWidget.h"
#include "GeoMath.h"
#include "CSettings.h"
#include "IUnit.h"
#include "CWptDB.h"
#include "config.h"

#include <QtGui>
#include <QMessageBox>
#include <QProgressDialog>
#include <QMenu>
#include <QInputDialog>

enum meter_feet_index
{
    METER_INDEX,
    FEET_INDEX
};

CTrackFilterWidget::CTrackFilterWidget(QWidget *parent)
: QWidget(parent)
{
    setupUi(this);
    setObjectName("CTrackFilterWidget");
    connect(pushApply, SIGNAL(clicked()), this, SLOT(slotApplyFilter()));
    connect(pushResetFilterList, SIGNAL(clicked()), this, SLOT(slotResetFilterList()));
    connect(pushSave, SIGNAL(clicked()), this, SLOT(slotSaveFilter()));
    connect(&CTrackDB::self(), SIGNAL(sigHighlightTrack(CTrack*)), this, SLOT(slotHighlightTrack(CTrack*)));

    connect(comboMeterFeet1, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(slotComboMeterFeet(const QString &)));
    connect(comboMeterFeet2, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(slotComboMeterFeet(const QString &)));
    connect(comboMeterFeet3, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(slotComboMeterFeet(const QString &)));
    connect(comboMeterFeet4, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(slotComboMeterFeet(const QString &)));
    connect(comboMeterFeet5, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(slotComboMeterFeet(const QString &)));

    connect(toolAddHidePoints1, SIGNAL(clicked()), this, SLOT(slotAddFilterHidePoints1()));
    connect(toolAddHidePoints2, SIGNAL(clicked()), this, SLOT(slotAddFilterHidePoints2()));
    connect(toolAddSmoothProfile1, SIGNAL(clicked()), this, SLOT(slotAddFilterSmoothProfile1()));
    connect(toolAddSplit1, SIGNAL(clicked()), this, SLOT(slotAddFilterSplit1()));
    connect(toolAddSplit2, SIGNAL(clicked()), this, SLOT(slotAddFilterSplit2()));
    connect(toolAddSplit3, SIGNAL(clicked()), this, SLOT(slotAddFilterSplit3()));
    connect(toolAddSplit4, SIGNAL(clicked()), this, SLOT(slotAddFilterSplit4()));
    connect(toolAddSplit5, SIGNAL(clicked()), this, SLOT(slotAddFilterSplit5()));
    connect(toolReset, SIGNAL(clicked()), this, SLOT(slotAddFilterReset()));
    connect(toolDelete, SIGNAL(clicked()), this, SLOT(slotAddFilterDelete()));
    connect(toolAddReplaceEle, SIGNAL(clicked()), this, SLOT(slotAddFilterReplaceElevation()));
    connect(toolAddOffsetEle, SIGNAL(clicked()), this, SLOT(slotAddFilterOffsetElevation()));
    connect(toolAddTime1, SIGNAL(clicked()), this, SLOT(slotAddFilterTime1()));
    connect(toolAddTime2, SIGNAL(clicked()), this, SLOT(slotAddFilterTime2()));
    connect(toolAddTime3, SIGNAL(clicked()), this, SLOT(slotAddFilterTime3()));

    connect(toolResetNow, SIGNAL(clicked()), this, SLOT(slotResetNow()));
    connect(toolHidePoints1Now, SIGNAL(clicked()), this, SLOT(slotHidePoints1Now()));
    connect(toolHidePoints2Now, SIGNAL(clicked()), this, SLOT(slotHidePoints2Now()));
    connect(toolDeleteNow, SIGNAL(clicked()), this, SLOT(slotDeleteNow()));
    connect(toolSmoothProfile1Now, SIGNAL(clicked()), this, SLOT(slotSmoothProfile1Now()));
    connect(toolReplaceEleNow, SIGNAL(clicked()), this, SLOT(slotReplaceEleNow()));
    connect(toolOffsetEleNow, SIGNAL(clicked()), this, SLOT(slotOffsetEleNow()));
    connect(toolSplit1Now, SIGNAL(clicked()), this, SLOT(slotSplit1Now()));
    connect(toolSplit2Now, SIGNAL(clicked()), this, SLOT(slotSplit2Now()));
    connect(toolSplit3Now, SIGNAL(clicked()), this, SLOT(slotSplit3Now()));
    connect(toolSplit4Now, SIGNAL(clicked()), this, SLOT(slotSplit4Now()));
    connect(toolSplit5Now, SIGNAL(clicked()), this, SLOT(slotSplit5Now()));
    connect(toolTime1Now, SIGNAL(clicked()), this, SLOT(slotTime1Now()));
    connect(toolTime2Now, SIGNAL(clicked()), this, SLOT(slotTime2Now()));
    connect(toolTime3Now, SIGNAL(clicked()), this, SLOT(slotTime3Now()));

    // ----------- read in GUI configuration -----------
    SETTINGS;
    if(IUnit::self().baseunit == "ft")
    {
        comboMeterFeet1->setCurrentIndex((int)FEET_INDEX);
        comboMeterFeet2->setCurrentIndex((int)FEET_INDEX);
        comboMeterFeet3->setCurrentIndex((int)FEET_INDEX);
        comboMeterFeet4->setCurrentIndex((int)FEET_INDEX);
        comboMeterFeet5->setCurrentIndex((int)FEET_INDEX);
        spinDistance1->setSuffix("ft");
        spinSplit3->setSuffix("ft");
        spinSplit4->setSuffix("ft");
        spinOffset1->setSuffix("ft");
        spinSpeed1->setSuffix("mph");
    }
    else
    {
        comboMeterFeet1->setCurrentIndex((int)METER_INDEX);
        comboMeterFeet2->setCurrentIndex((int)METER_INDEX);
        comboMeterFeet3->setCurrentIndex((int)METER_INDEX);
        comboMeterFeet4->setCurrentIndex((int)METER_INDEX);
        comboMeterFeet5->setCurrentIndex((int)METER_INDEX);
        spinDistance1->setSuffix("m");
        spinSplit3->setSuffix("m");
        spinSplit4->setSuffix("m");
        spinOffset1->setSuffix("m");
        spinSpeed1->setSuffix("km/h");
    }

    // Filter: Hide points 1
    spinDistance1->setValue(cfg.value("trackfilter/HidePoints1/distance",10).toInt());
    spinAzimuthDelta1->setValue(cfg.value("trackfilter/HidePoints1/azimuthdelta",10).toInt());

    // Filter: Hide points 2
    spinDistance2->setValue(cfg.value("trackfilter/HidePoints2/distance",1).toInt());

    // Filter: Smooth profile 1
    spinSmoothProfileTabs1->setValue(cfg.value("trackfilter/SmoothProfile1/tabs",5).toInt());

    // Filter: Split 1
    spinSplit1->setValue(cfg.value("trackfilter/Split1/val",spinSplit1->value()).toInt());

    // Filter: Split 2
    spinSplit2->setValue(cfg.value("trackfilter/Split2/val",spinSplit2->value()).toInt());

    // Filter Split 3
    spinSplit3->setValue(cfg.value("trackfilter/Split3/val",spinSplit3->value()).toInt());

    // Filter: Split 4
    spinSplit4->setValue(cfg.value("trackfilter/Split4/val",spinSplit4->value()).toInt());

    // Filter: Replace Elevation
    radioEleFromLocal->setChecked(cfg.value("trackfilter/ReplaceElevation/fromLocal", radioEleFromLocal->isChecked()).toBool());
    radioEleFromRemote->setChecked(cfg.value("trackfilter/ReplaceElevation/fromRemote", radioEleFromRemote->isChecked()).toBool());
    editGeonamesOrgUsername->setText(cfg.value("geonames/username", "").toString());

    // Filter: Time1
    dateTime1->setDateTime(cfg.value("trackFilter/Time1/time", QDateTime::currentDateTime()).toDateTime());

    // Filter: Time 2
    spinSpeed1->setValue(cfg.value("trackfilter/Time2/speed",spinSpeed1->value()).toInt());

    // Filter: Time 3
    spinTime1->setValue(cfg.value("trackfilter/Time3/delta",spinTime1->value()).toInt());

    // register current track
    slotHighlightTrack(CTrackDB::self().highlightedTrack());

    radioSplitTracks->setChecked(cfg.value("trackfilter/Split/asTrack", radioSplitTracks->isChecked()).toBool());
    radioSplitStages->setChecked(cfg.value("trackfilter/Split/asStages", radioSplitStages->isChecked()).toBool());

    // restore last filter list
    loadFilterList(QDir::home().filePath(CONFIGDIR ".track_filter_current.filter"));

    QDir dir(QDir::home());
    dir.cd(CONFIGDIR);
    QStringList filenames = dir.entryList(QStringList("*.filter"), QDir::Files, QDir::Name);

    foreach(const QString& filename, filenames)
    {
        QListWidgetItem * item = new QListWidgetItem(listStored);
        item->setText(QFileInfo(filename).baseName());
        item->setIcon(QIcon(":/icons/iconFilter16x16.png"));
    }

    connect(listStored, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(slotDoubleClickStoredFilter(QListWidgetItem*)));
    connect(listStored, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotContextMenuStoredFilter(QPoint)));

    contextMenuStoredFilter = new QMenu(this);
    contextMenuStoredFilter->addAction(QIcon(":/icons/iconEdit16x16.png"), tr("Edit name..."), this, SLOT(slotStoredFilterEdit()));
    contextMenuStoredFilter->addAction(QIcon(":/icons/iconClear16x16.png"), tr("Delete"), this, SLOT(slotStoredFilterDelete()));

}


CTrackFilterWidget::~CTrackFilterWidget()
{
    // ----------- store GUI configuration -----------
    SETTINGS;
    // Filter: Hide points 1
    cfg.setValue("trackfilter/HidePoints1/distance",spinDistance1->value());
    cfg.setValue("trackfilter/HidePoints1/azimuthdelta",spinAzimuthDelta1->value());

    // Filter: Hide points 2
    cfg.setValue("trackfilter/HidePoints2/distance",spinDistance2->value());

    // Filter: Smooth profile 1
    cfg.setValue("trackfilter/SmoothProfile1/tabs",spinSmoothProfileTabs1->value());

    // Filter: Split 1
    cfg.setValue("trackfilter/Split1/val",spinSplit1->value());

    // Filter: Split 2
    cfg.setValue("trackfilter/Split2/val",spinSplit2->value());

    // Filter: Split 3
    cfg.setValue("trackfilter/Split3/val",spinSplit3->value());

    // Filter: Split 4
    cfg.setValue("trackfilter/Split4/val",spinSplit4->value());

    // Filter: Replace Elevation
    cfg.setValue("trackfilter/ReplaceElevation/fromLocal", radioEleFromLocal->isChecked());
    cfg.setValue("trackfilter/ReplaceElevation/fromRemote", radioEleFromRemote->isChecked());
    cfg.setValue("geonames/username", editGeonamesOrgUsername->text());

    // Filter: Time 1
    cfg.setValue("trackFilter/Time1/time", dateTime1->dateTime());

    // Filter: Time 2
    cfg.setValue("trackfilter/Time2/speed",spinSpeed1->value());

    // Filter: Time 3
    cfg.setValue("trackfilter/Time3/delta",spinTime1->value());

    cfg.setValue("trackfilter/Split/asTrack", radioSplitTracks->isChecked());
    cfg.setValue("trackfilter/Split/asStages", radioSplitStages->isChecked());

    // store current filter list
    saveFilterList(QDir::home().filePath(CONFIGDIR ".track_filter_current.filter"));
}


void CTrackFilterWidget::showFilterPartMessage(bool show)
{
    if(show)
    {
        labelFilterPartIcon->show();
        labelFilterPartMessage->show();
    }
    else
    {
        labelFilterPartIcon->hide();
        labelFilterPartMessage->hide();
    }

}


void CTrackFilterWidget::saveFilterList(const QString& filename)
{
    QFile file(filename);
    file.open(QIODevice::WriteOnly);
    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_4_5);

    out << qint32(listFilters->count());
    for(int i = 0; i < listFilters->count(); i++)
    {
        QListWidgetItem * item = listFilters->item(i);

        out << item->icon();
        out << item->text();
        out << item->data(Qt::UserRole);
    }

    file.close();
}


void CTrackFilterWidget::loadFilterList(const QString& filename)
{
    QFile file(filename);
    file.open(QIODevice::ReadOnly);
    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_4_5);

    listFilters->clear();

    qint32 N;
    in >> N;
    for(int i = 0; i < N; i++)
    {
        QListWidgetItem * item = new QListWidgetItem(listFilters);

        QIcon icon;
        QString text;
        QVariant data;

        in >> icon >> text >> data;

        item->setIcon(icon);
        item->setText(text);
        item->setData(Qt::UserRole, data);
    }

    file.close();

    if(listFilters->count())
    {
        pushResetFilterList->setEnabled(true);
        pushApply->setEnabled(true);
        pushSave->setEnabled(true);
    }
}


void CTrackFilterWidget::setTrackEditWidget(CTrackEditWidget * w)
{
    trackEditWidget = w;
}


void CTrackFilterWidget::slotUpdate()
{
    QList<CTrack::pt_t> focus;
    track->getPointOfFocus(focus);

    bool isSubSection = focus.size() > 1;
    showFilterPartMessage(isSubSection);

    groupSplit1->setEnabled(!isSubSection);
    groupSplit2->setEnabled(!isSubSection);
    groupSplit3->setEnabled(!isSubSection);
    groupSplit4->setEnabled(!isSubSection);
    groupSplit5->setEnabled(!isSubSection);
    groupTime3->setEnabled(!isSubSection);
}


void CTrackFilterWidget::slotHighlightTrack(CTrack * trk)
{
    if(!track.isNull())
    {
        disconnect(track, SIGNAL(sigChanged()), this, SLOT(slotUpdate()));
        disconnect(track, SIGNAL(sigNeedUpdate()), this, SLOT(slotUpdate()));
    }

    track = trk;
    if(!track.isNull())
    {
        connect(track, SIGNAL(sigChanged()), this, SLOT(slotUpdate()));
        connect(track, SIGNAL(sigNeedUpdate()), this, SLOT(slotUpdate()));
        slotUpdate();
    }

}


void CTrackFilterWidget::slotComboMeterFeet(const QString &text)
{
    if(sender() == comboMeterFeet1)
    {
        spinDistance1->setSuffix(text);
    }
    else if(sender() == comboMeterFeet2)
    {
        spinSplit3->setSuffix(text);
    }
    else if(sender() == comboMeterFeet3)
    {
        spinSplit4->setSuffix(text);
    }
    else if(sender() == comboMeterFeet4)
    {
        spinOffset1->setSuffix(text);
    }
    else if(sender() == comboMeterFeet5)
    {
        spinSpeed1->setSuffix(text);
    }
}


void CTrackFilterWidget::slotDoubleClickStoredFilter(QListWidgetItem * item)
{
    if(item)
    {
        loadFilterList(QDir::home().filePath(CONFIGDIR + item->text() + ".filter"));
    }
}


void CTrackFilterWidget::slotContextMenuStoredFilter( const QPoint & pos)
{
    QPoint p = listStored->mapToGlobal(pos);
    contextMenuStoredFilter->exec(p);
}


void CTrackFilterWidget::slotStoredFilterEdit()
{
    QListWidgetItem * item = listStored->currentItem();
    if(item == 0)
    {
        return;
    }

    QString name = item->text();
    QFile file(QDir::home().filePath(CONFIGDIR + name + ".filter"));

    name = QInputDialog::getText(this, tr("Filter name ..."), tr("Please enter a name for the filter list to store."), QLineEdit::Normal, name);
    if(name.isEmpty())
    {
        return;
    }

    file.rename(QDir::home().filePath(CONFIGDIR + name + ".filter"));
    item->setText(name);
}


void CTrackFilterWidget::slotStoredFilterDelete()
{
    QListWidgetItem * item = listStored->currentItem();
    if(item == 0)
    {
        return;
    }

    QString name = item->text();
    QFile file(QDir::home().filePath(CONFIGDIR + name + ".filter"));

    QMessageBox::StandardButton res = QMessageBox::question(this, tr("Delete track filter..."), tr("Do you really want to delete '%1'?").arg(name), QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes);

    if(res == QMessageBox::No)
    {
        return;
    }

    file.remove();
    delete item;
}


void CTrackFilterWidget::slotResetFilterList()
{
    listFilters->clear();
    pushResetFilterList->setEnabled(false);
    pushApply->setEnabled(false);
    pushSave->setEnabled(false);
}


void CTrackFilterWidget::slotAddFilterHidePoints1()
{
    QByteArray args;
    double d, a;

    readGuiHidePoints1(args, d, a);

    QString name = groupReducePoints1->title() + QString(" (%1%2, %3%4)").arg(d).arg(spinDistance1->suffix()).arg(a).arg(QChar(0260));
    addFilter(name, ":/icons/iconTrack16x16.png", args);
}


void CTrackFilterWidget::readGuiHidePoints1(QByteArray& args, double& d, double& a)
{
    QDataStream stream(&args, QIODevice::WriteOnly);

    d =  spinDistance1->value();
    a = spinAzimuthDelta1->value();

    if(spinDistance1->suffix() == "ft")
    {
        stream << quint32(eHidePoints1) << (d * 0.3048f) << a;
    }
    else
    {
        stream << quint32(eHidePoints1) << d << a;
    }
}


void CTrackFilterWidget::slotAddFilterHidePoints2()
{
    QByteArray args;
    double d;

    readGuiHidePoints2(args, d);
    QString name = groupReducePoints2->title() + QString(" (%1m)").arg(d);
    addFilter(name, ":/icons/iconTrack16x16.png", args);
}


void CTrackFilterWidget::readGuiHidePoints2(QByteArray& args, double& d)
{
    QDataStream stream(&args, QIODevice::WriteOnly);
    d =  spinDistance2->value();
    stream << quint32(eHidePoints2) << d;
}


void CTrackFilterWidget::slotAddFilterSmoothProfile1()
{
    QByteArray args;
    quint32 tabs;

    readGuiSmoothProfile1(args, tabs);
    QString name = groupSmoothProfile1->title() + QString(" (%1 points)").arg(tabs);
    addFilter(name, ":/icons/iconGraph16x16.png", args);
}


void CTrackFilterWidget::readGuiSmoothProfile1(QByteArray& args, quint32& tabs)
{
    QDataStream stream(&args, QIODevice::WriteOnly);
    tabs = spinSmoothProfileTabs1->value();
    stream << quint32(eSmoothProfile1) << tabs;
}


void CTrackFilterWidget::slotAddFilterSplit1()
{
    QByteArray args;
    double val;

    readGuiSplit1(args, val);
    QString name = groupSplit1->title() + QString(" (%1 chunks)").arg(val);
    addFilter(name, ":/icons/editcut.png", args);
}


void CTrackFilterWidget::readGuiSplit1(QByteArray& args, double& val)
{
    QDataStream stream(&args, QIODevice::WriteOnly);

    val = spinSplit1->value();
    stream << quint32(eSplit1) << val;
}


void CTrackFilterWidget::slotAddFilterSplit2()
{
    QByteArray args;
    double val;

    readGuiSplit2(args, val);
    QString name = groupSplit2->title() + QString(" (%1 points)").arg(val);
    addFilter(name, ":/icons/editcut.png", args);
}


void CTrackFilterWidget::readGuiSplit2(QByteArray& args, double& val)
{
    QDataStream stream(&args, QIODevice::WriteOnly);

    val = spinSplit2->value();
    stream << quint32(eSplit2) << val;
}


void CTrackFilterWidget::slotAddFilterSplit3()
{
    QByteArray args;
    double val;

    readGuiSplit3(args, val);
    QString name = groupSplit3->title() + QString(" (%1%2)").arg(val).arg(spinSplit3->suffix());
    addFilter(name, ":/icons/editcut.png", args);
}


void CTrackFilterWidget::readGuiSplit3(QByteArray& args, double& val)
{
    QDataStream stream(&args, QIODevice::WriteOnly);

    val = spinSplit3->value();
    if(spinSplit3->suffix() == "ft")
    {
        stream << quint32(eSplit3) << (val * 0.3048f);
    }
    else
    {
        stream << quint32(eSplit3) << val;
    }
}


void CTrackFilterWidget::slotAddFilterSplit4()
{
    QByteArray args;
    double val;

    readGuiSplit4(args, val);
    QString name = groupSplit4->title() + QString(" (%1%2)").arg(val).arg(spinSplit4->suffix());
    addFilter(name, ":/icons/editcut.png", args);
}


void CTrackFilterWidget::readGuiSplit4(QByteArray& args, double& val)
{
    QDataStream stream(&args, QIODevice::WriteOnly);

    val = spinSplit4->value();
    if(spinSplit4->suffix() == "ft")
    {
        stream << quint32(eSplit4) << (val * 0.3048f);
    }
    else
    {
        stream << quint32(eSplit4) << val;
    }
}


void CTrackFilterWidget::slotAddFilterSplit5()
{
    QByteArray args;

    readGuiSplit5(args);
    QString name = groupSplit5->title();
    addFilter(name, ":/icons/editcut.png", args);
}


void CTrackFilterWidget::readGuiSplit5(QByteArray& args)
{
    QDataStream stream(&args, QIODevice::WriteOnly);
    stream << quint32(eSplit5);
}


void CTrackFilterWidget::slotAddFilterTime1()
{
    QByteArray args;
    QDateTime  time;

    readGuiTime1(args, time);
    QString name = groupTime1->title() + QString(" (%1)").arg(time.toString("dd.MM.yyyy HH:mm:ss"));
    addFilter(name, ":/icons/iconTime16x16.png", args);
}


void CTrackFilterWidget::readGuiTime1(QByteArray& args, QDateTime& time)
{
    QDataStream stream(&args, QIODevice::WriteOnly);

    time = dateTime1->dateTime();
    stream << quint32(eTime1) << time;
}


void CTrackFilterWidget::slotAddFilterTime2()
{
    QByteArray args;
    double speed;

    readGuiTime2(args, speed);
    QString name = groupTime2->title() + QString(" (%1%2)").arg(speed).arg(spinSpeed1->suffix());
    addFilter(name, ":/icons/iconTime16x16.png", args);
}


void CTrackFilterWidget::readGuiTime2(QByteArray& args, double& speed)
{
    QDataStream stream(&args, QIODevice::WriteOnly);

    speed = spinSpeed1->value();

    if(spinSpeed1->suffix() == "mph")
    {
        stream << quint32(eTime2) << (speed * 1.609347);
    }
    else
    {
        stream << quint32(eTime2) << speed;
    }
}


void CTrackFilterWidget::slotAddFilterTime3()
{
    QByteArray args;
    quint32 delta;

    readGuiTime3(args, delta);
    QString name = groupTime3->title() + QString(" (%1sec)").arg(delta);
    addFilter(name, ":/icons/iconTime16x16.png", args);
}


void CTrackFilterWidget::readGuiTime3(QByteArray& args, quint32& delta)
{
    QDataStream stream(&args, QIODevice::WriteOnly);

    delta = spinTime1->value();
    stream << quint32(eTime3) << delta;
}


void CTrackFilterWidget::slotAddFilterReset()
{
    QByteArray args;

    QString name = tr("Reset");
    readGuiReset(args);
    addFilter(name, ":/icons/editundo.png", args);
}


void CTrackFilterWidget::readGuiReset(QByteArray& args)
{
    QDataStream stream(&args, QIODevice::WriteOnly);
    stream << quint32(eReset);
}


void CTrackFilterWidget::slotAddFilterDelete()
{
    QByteArray args;

    QString name = groupDelete->title();
    readGuiDelete(args);
    addFilter(name, ":/icons/iconDelete16x16.png", args);
}


void CTrackFilterWidget::readGuiDelete(QByteArray& args)
{
    QDataStream stream(&args, QIODevice::WriteOnly);
    stream << quint32(eDelete);
}


void CTrackFilterWidget::slotAddFilterReplaceElevation()
{
    QByteArray args;
    quint32 type;

    readGuiReplaceEle(args, type);
    QString name = groupReplaceElevation->title() + (type == eLocal ? tr(" (local)") : tr(" (remote)"));
    addFilter(name, ":/icons/iconGraph16x16.png", args);
}


void CTrackFilterWidget::readGuiReplaceEle(QByteArray& args, quint32& type)
{
    QDataStream stream(&args, QIODevice::WriteOnly);

    type = eLocal;
    if(radioEleFromRemote->isChecked())
    {
        type = eRemote;
    }
    stream << quint32(eReplaceElevation) << type << editGeonamesOrgUsername->text();
}


void CTrackFilterWidget::slotAddFilterOffsetElevation()
{
    QByteArray args;

    double val;
    QString unit;

    readGuiOffsetEle(args, val, unit);
    QString name = groupOffsetElevation->title() + QString(" (%i%i)").arg(val).arg(unit);
    addFilter(name, ":/icons/iconGraph16x16.png", args);
}


void CTrackFilterWidget::readGuiOffsetEle(QByteArray& args, double& val, QString& unit)
{
    QDataStream stream(&args, QIODevice::WriteOnly);

    val     = spinOffset1->value();
    unit    = spinOffset1->suffix();

    if(unit == "ft")
    {
        stream << quint32(eOffsetElevation) << (val / 3.28084);
    }
    else
    {
        stream << quint32(eOffsetElevation) << val;
    }

}


void CTrackFilterWidget::addFilter(const QString& name, const QString& icon, QByteArray& args)
{
    QListWidgetItem * item = new QListWidgetItem(listFilters);
    item->setIcon(QIcon(icon));
    item->setText(name);
    item->setData(Qt::UserRole, args);

    pushApply->setEnabled(true);
    pushResetFilterList->setEnabled(true);
    pushSave->setEnabled(true);
}


void CTrackFilterWidget::slotResetNow()
{
    if(track.isNull()) return;

    quint32 type;
    QByteArray args;
    readGuiReset(args);

    QDataStream stream(&args, QIODevice::ReadOnly);
    stream >> type;

    QList<CTrack*> tracks;
    tracks << track;

    filterReset(stream, tracks);

    postProcessTrack();
}


void CTrackFilterWidget::slotHidePoints1Now()
{
    if(track.isNull()) return;

    quint32 type;
    double d, a;
    QByteArray args;
    readGuiHidePoints1(args, d, a);

    QDataStream stream(&args, QIODevice::ReadOnly);
    stream >> type;

    QList<CTrack*> tracks;
    tracks << track;

    filterHidePoints1(stream, tracks);

    postProcessTrack();
}


void CTrackFilterWidget::slotHidePoints2Now()
{
    if(track.isNull()) return;

    quint32 type;
    double d;
    QByteArray args;
    readGuiHidePoints2(args, d);

    QDataStream stream(&args, QIODevice::ReadOnly);
    stream >> type;

    QList<CTrack*> tracks;
    tracks << track;

    filterHidePoints2(stream, tracks);

    postProcessTrack();
}


void CTrackFilterWidget::slotDeleteNow()
{
    if(track.isNull()) return;

    quint32 type;
    QByteArray args;
    readGuiDelete(args);

    QDataStream stream(&args, QIODevice::ReadOnly);
    stream >> type;

    QList<CTrack*> tracks;
    tracks << track;

    filterDelete(stream, tracks);

    postProcessTrack();
}


void CTrackFilterWidget::slotSmoothProfile1Now()
{
    if(track.isNull()) return;

    quint32 type;
    quint32 tabs;
    QByteArray args;
    readGuiSmoothProfile1(args, tabs);

    QDataStream stream(&args, QIODevice::ReadOnly);
    stream >> type;

    QList<CTrack*> tracks;
    tracks << track;

    filterSmoothProfile1(stream, tracks);

    postProcessTrack();
}


void CTrackFilterWidget::slotReplaceEleNow()
{
    if(track.isNull()) return;

    quint32 type;
    QByteArray args;
    readGuiReplaceEle(args, type);

    QDataStream stream(&args, QIODevice::ReadOnly);
    stream >> type;

    QList<CTrack*> tracks;
    tracks << track;

    filterReplaceElevation(stream, tracks);

    postProcessTrack();
}


void CTrackFilterWidget::slotOffsetEleNow()
{
    if(track.isNull()) return;

    quint32 type;
    double val;
    QString unit;
    QByteArray args;

    readGuiOffsetEle(args, val, unit);

    QDataStream stream(&args, QIODevice::ReadOnly);
    stream >> type;

    QList<CTrack*> tracks;
    tracks << track;

    filterOffsetElevation(stream, tracks);

    postProcessTrack();
}


void CTrackFilterWidget::slotSplit1Now()
{
    if(track.isNull()) return;

    quint32 type;
    double val;
    QByteArray args;
    readGuiSplit1(args, val);

    QDataStream stream(&args, QIODevice::ReadOnly);
    stream >> type;

    QList<CTrack*> tracks;
    tracks << track;

    if(radioSplitTracks->isChecked())
    {
        filterSplit1Tracks(stream, tracks);
    }
    else
    {
        filterSplit1Stages(stream, tracks);
    }

    postProcessTrack();
}


void CTrackFilterWidget::slotSplit2Now()
{
    if(track.isNull()) return;

    quint32 type;
    double val;
    QByteArray args;
    readGuiSplit2(args, val);

    QDataStream stream(&args, QIODevice::ReadOnly);
    stream >> type;

    QList<CTrack*> tracks;
    tracks << track;

    if(radioSplitTracks->isChecked())
    {
        filterSplit2Tracks(stream, tracks);
    }
    else
    {
        filterSplit2Stages(stream, tracks);
    }

    postProcessTrack();
}


void CTrackFilterWidget::slotSplit3Now()
{
    if(track.isNull()) return;

    quint32 type;
    double val;
    QByteArray args;
    readGuiSplit3(args, val);

    QDataStream stream(&args, QIODevice::ReadOnly);
    stream >> type;

    QList<CTrack*> tracks;
    tracks << track;

    if(radioSplitTracks->isChecked())
    {
        filterSplit3Tracks(stream, tracks);
    }
    else
    {
        filterSplit3Stages(stream, tracks);
    }

    postProcessTrack();
}


void CTrackFilterWidget::slotSplit4Now()
{
    if(track.isNull()) return;

    quint32 type;
    double val;
    QByteArray args;
    readGuiSplit4(args, val);

    QDataStream stream(&args, QIODevice::ReadOnly);
    stream >> type;

    QList<CTrack*> tracks;
    tracks << track;

    if(radioSplitTracks->isChecked())
    {
        filterSplit4Tracks(stream, tracks);
    }
    else
    {
        filterSplit4Stages(stream, tracks);
    }

    postProcessTrack();
}


void CTrackFilterWidget::slotSplit5Now()
{
    if(track.isNull()) return;

    quint32 type;
    QByteArray args;
    readGuiSplit5(args);

    QDataStream stream(&args, QIODevice::ReadOnly);
    stream >> type;

    QList<CTrack*> tracks;
    tracks << track;

    filterSplit5Tracks(stream, tracks);

    postProcessTrack();
}


void CTrackFilterWidget::slotTime1Now()
{
    if(track.isNull()) return;

    quint32    type;
    QDateTime  time;
    QByteArray args;
    readGuiTime1(args, time);

    QDataStream stream(&args, QIODevice::ReadOnly);
    stream >> type;

    QList<CTrack*> tracks;
    tracks << track;

    filterTime1(stream, tracks);

    postProcessTrack();
}


void CTrackFilterWidget::slotTime2Now()
{
    if(track.isNull()) return;

    quint32    type;
    double     speed;
    QByteArray args;
    readGuiTime2(args, speed);

    QDataStream stream(&args, QIODevice::ReadOnly);
    stream >> type;

    QList<CTrack*> tracks;
    tracks << track;

    filterTime2(stream, tracks);

    postProcessTrack();
}


void CTrackFilterWidget::slotTime3Now()
{
    if(track.isNull()) return;

    quint32    type;
    quint32    delta;
    QByteArray args;
    readGuiTime3(args, delta);

    QDataStream stream(&args, QIODevice::ReadOnly);
    stream >> type;

    QList<CTrack*> tracks;
    tracks << track;

    filterTime3(stream, tracks);

    postProcessTrack();
}


void CTrackFilterWidget::slotSaveFilter()
{
    QString name = QInputDialog::getText(this, tr("Filter name ..."), tr("Please enter a name for the filter list to store."));

    if(name.isEmpty())
    {
        return;
    }

    QString filename = QDir::home().filePath(CONFIGDIR + name + ".filter");

    saveFilterList(filename);

    QListWidgetItem * item = new QListWidgetItem(listStored);
    item->setText(name);
    item->setIcon(QIcon(":/icons/iconFilter16x16.png"));
}


void CTrackFilterWidget::slotApplyFilter()
{
    if(track.isNull()) return;

    QList<CTrack*> tracks;
    tracks << track;

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    CTrackDB::self().setPointOfFocusByIdx(-1);

    const int N = listFilters->count();
    for(int i = 0; i < N; i++)
    {
        bool cancelled = true;
        quint32 type;
        QListWidgetItem * item = listFilters->item(i);
        QByteArray data = item->data(Qt::UserRole).toByteArray();
        QDataStream args(&data, QIODevice::ReadOnly);

        args >> type;
        switch(type)
        {
            case eHidePoints1:
                cancelled = filterHidePoints1(args, tracks);
                break;

            case eHidePoints2:
                cancelled = filterHidePoints2(args, tracks);
                break;

            case eSmoothProfile1:
                cancelled = filterSmoothProfile1(args, tracks);
                break;

            case eSplit1:
                if(radioSplitTracks->isChecked())
                {
                    cancelled = filterSplit1Tracks(args, tracks);
                }
                else
                {
                    cancelled = filterSplit1Stages(args, tracks);
                }
                break;

            case eSplit2:
                if(radioSplitTracks->isChecked())
                {
                    cancelled = filterSplit2Tracks(args, tracks);
                }
                else
                {
                    cancelled = filterSplit2Stages(args, tracks);
                }
                break;

            case eSplit3:
                if(radioSplitTracks->isChecked())
                {
                    cancelled = filterSplit3Tracks(args, tracks);
                }
                else
                {
                    cancelled = filterSplit3Stages(args, tracks);
                }
                break;

            case eSplit4:
                if(radioSplitTracks->isChecked())
                {
                    cancelled = filterSplit4Tracks(args, tracks);
                }
                else
                {
                    cancelled = filterSplit4Stages(args, tracks);
                }
                break;

            case eReset:
                cancelled = filterReset(args, tracks);
                break;

            case eDelete:
                cancelled = filterDelete(args, tracks);
                break;

            case eReplaceElevation:
                cancelled = filterReplaceElevation(args, tracks);
                break;

            case eOffsetElevation:
                cancelled = filterOffsetElevation(args, tracks);
                break;

            case eTime1:
                cancelled = filterTime1(args, tracks);
                break;

            case eTime2:
                cancelled = filterTime2(args, tracks);
                break;

            case eTime3:
                cancelled = filterTime3(args, tracks);
                break;

            default:
                qDebug() << "unknown filter" << type;
                cancelled = true;

        }

        if(cancelled)
        {
            break;
        }
    }

    postProcessTrack();
    QApplication::restoreOverrideCursor();
}


void CTrackFilterWidget::postProcessTrack()
{
    track->rebuild(true);
    track->slotScaleWpt2Track();
    QString key = track->getKey();
    CTrackDB::self().highlightTrack(key);
    CTrackDB::self().emitSigModified(key);

    trackEditWidget->slotResetAllZoom();
}


bool CTrackFilterWidget::filterHidePoints1(QDataStream& args, QList<CTrack*>& tracks)
{
    double minDistance, minAzimuthDelta;
    args >> minDistance >> minAzimuthDelta;

    foreach(CTrack * trk, tracks)
    {

        //QList<CTrack::pt_t>& trkpts = trk->getTrackPoints();
        //int npts = trk->getTrackPoints().count();

        QProgressDialog progress(groupReducePoints1->title(), tr("Abort filter"), 0, tracks.size(), this);
        progress.setWindowTitle(groupReducePoints1->title());
        progress.setWindowModality(Qt::WindowModal);

        QList<CTrack::pt_t>::iterator trkpt, end;
        trk->setupIterators(trkpt, end);

        int i               = 0;
        double lastEle      = trkpt->ele;
        double lastAzimuth  = trkpt->azimuth;
        double deltaAzimuth = 0;

        projXY p1, p2;
        p1.u = DEG_TO_RAD * trkpt->lon;
        p1.v = DEG_TO_RAD * trkpt->lat;
        ++trkpt;

        while(trkpt != end)
        {
            p2.u = DEG_TO_RAD * trkpt->lon;
            p2.v = DEG_TO_RAD * trkpt->lat;
            double a1, a2;

            double delta = distance(p1,p2,a1,a2);

            if (abs(trkpt->azimuth) <= 180)
            {
                if(abs(trkpt->azimuth - lastAzimuth) > 180)
                {
                    deltaAzimuth = 360 - abs(trkpt->azimuth - lastAzimuth);
                }
                else
                {
                    deltaAzimuth = abs(trkpt->azimuth - lastAzimuth);
                }
            }
            else
            {
                deltaAzimuth = 0;
            }

            double deltaEle = abs(lastEle - trkpt->ele);

            if (delta < minDistance || (deltaAzimuth < minAzimuthDelta))
            {
                if(deltaEle < 3)
                {
                    trkpt->flags |= CTrack::pt_t::eDeleted;
                }
                else
                {
                    lastEle = trkpt->ele;
                }

            }
            else
            {
                p1 = p2;
                progress.setValue(i);
                if(deltaAzimuth >= minAzimuthDelta)
                {
                    lastAzimuth = trkpt->azimuth;
                }

                lastEle = trkpt->ele;
            }
            ++trkpt;
            if (progress.wasCanceled())
            {
                return true;
            }
        }
        trk->rebuild(false);
        progress.setValue(++i);
    }

    return false;
}


bool CTrackFilterWidget::filterHidePoints2(QDataStream& args, QList<CTrack*>& tracks)
{
    double d;
    args >> d;

    int prog = 0;
    QProgressDialog progress(groupReducePoints2->title(), tr("Abort filter"), 0, tracks.size() * 3, this);
    progress.setWindowTitle(groupReducePoints2->title());
    progress.setWindowModality(Qt::WindowModal);

    foreach(CTrack * trk, tracks)
    {

        // convert track points into a vector of pointDP (Douglas-Peucker points)
        QList<CTrack::pt_t>::iterator trkpt, end;
        trk->setupIterators(trkpt, end);
        int npts    = (end - trkpt);
        int idx     = 0;
        QVector<pointDP> line(npts);

        // the used projection will be mercator thus all values are meter
        projPJ pjsrc   = pj_init_plus("+proj=longlat +a=6378137.0000 +b=6356752.3142 +towgs84=0,0,0,0,0,0,0,0 +units=m  +no_defs");
        projPJ pjtar   = pj_init_plus("+proj=merc +a=6378137.0000 +b=6356752.3142 +towgs84=0,0,0,0,0,0,0,0 +units=m  +no_defs");

        while(trkpt != end)
        {
            pointDP& point = line[idx];

            point.x = trkpt->lon * DEG_TO_RAD;
            point.y = trkpt->lat * DEG_TO_RAD;
            point.z = trkpt->ele;

            pj_transform(pjsrc, pjtar, 1, 0, &point.x, &point.y, 0);

            idx++;trkpt++;
        }

        pj_free(pjsrc);
        pj_free(pjtar);

        progress.setValue(prog++);

        GPS_Math_DouglasPeucker(line, d);

        progress.setValue(prog++);

        // now read back the the "used" flags
        trk->setupIterators(trkpt, end);
        foreach(const pointDP& pt, line)
        {
            if(pt.used)
            {
                trkpt->flags &= ~CTrack::pt_t::eDeleted;
            }
            else
            {
                trkpt->flags |= CTrack::pt_t::eDeleted;
            }

            idx++;trkpt++;
        }

        trk->rebuild(false);

        if(progress.wasCanceled())
        {
            return true;
        }
        progress.setValue(prog++);

    }

    return false;
}


bool CTrackFilterWidget::filterSmoothProfile1(QDataStream &args, QList<CTrack *> &tracks)
{
    quint32 tabs;
    args >> tabs;

    foreach(CTrack * trk, tracks)
    {

        QList<CTrack::pt_t>& trkpts = trk->getTrackPoints();
        int npts = trkpts.count();

        QProgressDialog progress(groupSmoothProfile1->title(), tr("Abort filter"), 0, npts, this);
        progress.setWindowTitle(groupSmoothProfile1->title());
        progress.setWindowModality(Qt::WindowModal);

        trk->medianFilter(tabs, progress);
        trk->rebuild(false);

        if(progress.wasCanceled())
        {
            return true;
        }

    }
    return false;
}


bool CTrackFilterWidget::filterSplit1Stages(QDataStream &args, QList<CTrack *> &tracks)
{
    double val;
    args >> val;

    foreach(CTrack * trk, tracks)
    {
        QList<CTrack::pt_t>& trkpts = trk->getTrackPoints();
        int npts        = trkpts.count();
        int totalCnt    = 0;
        int trkptCnt    = 0;
        int trkCnt      = 1;
        int chunk       = (trkpts.size() + val) / val;

        QProgressDialog progress(groupSplit1->title(), tr("Abort filter"), 0, npts, this);
        progress.setWindowTitle(groupSplit1->title());
        progress.setWindowModality(Qt::WindowModal);

        foreach(const CTrack::pt_t& trkpt, trkpts)
        {
            progress.setValue(totalCnt++);

            if(++trkptCnt >= chunk)
            {
                trkptCnt = 0;
                CWptDB::self().newWpt(trkpt.lon * DEG_TO_RAD, trkpt.lat * DEG_TO_RAD, trkpt.ele, QString("S%1").arg(trkCnt++));
            }

            if(progress.wasCanceled())
            {
                return true;
            }
        }
    }
    return false;

}


bool CTrackFilterWidget::filterSplit1Tracks(QDataStream &args, QList<CTrack *> &tracks)
{
    double val;
    args >> val;

    QList<CTrack *> newTracks;

    foreach(CTrack * trk, tracks)
    {
        QList<CTrack::pt_t>& trkpts = trk->getTrackPoints();
        int npts        = trkpts.count();
        int totalCnt    = 0;
        int trkptCnt    = 0;
        int trkCnt      = 1;
        int chunk       = (trkpts.size() + val) / val;

        QProgressDialog progress(groupSplit1->title(), tr("Abort filter"), 0, npts, this);
        progress.setWindowTitle(groupSplit1->title());
        progress.setWindowModality(Qt::WindowModal);

        CTrack * newTrack = new CTrack(&CTrackDB::self());
        newTrack->setName(trk->getName() + QString("_%1").arg(trkCnt++));
        newTrack->setColor(trk->getColorIdx());
        newTracks << newTrack;

        foreach(const CTrack::pt_t& trkpt, trkpts)
        {
            progress.setValue(totalCnt++);

            *newTrack << trkpt;
            if(++trkptCnt >= chunk)
            {
                CTrackDB::self().addTrack(newTrack, true);

                trkptCnt = 0;
                newTrack = new CTrack(&CTrackDB::self());
                newTrack->setName(trk->getName() + QString("_%1").arg(trkCnt++));
                newTrack->setColor(trk->getColorIdx());

                newTracks << newTrack;
            }

            if(progress.wasCanceled())
            {
                return true;
            }
        }
        CTrackDB::self().addTrack(newTrack, true);
    }
    tracks = newTracks;
    return false;
}


bool CTrackFilterWidget::filterSplit2Stages(QDataStream &args, QList<CTrack *> &tracks)
{
    double val;
    args >> val;

    foreach(CTrack * trk, tracks)
    {
        QList<CTrack::pt_t>& trkpts = trk->getTrackPoints();
        int npts        = trkpts.count();
        int totalCnt    = 0;
        int trkptCnt    = 0;
        int trkCnt      = 1;
        int chunk       = val;

        QProgressDialog progress(groupSplit2->title(), tr("Abort filter"), 0, npts, this);
        progress.setWindowTitle(groupSplit2->title());
        progress.setWindowModality(Qt::WindowModal);

        foreach(const CTrack::pt_t& trkpt, trkpts)
        {
            progress.setValue(totalCnt++);

            if(++trkptCnt >= chunk)
            {
                trkptCnt = 0;
                CWptDB::self().newWpt(trkpt.lon * DEG_TO_RAD, trkpt.lat * DEG_TO_RAD, trkpt.ele, QString("S%1").arg(trkCnt++));
            }

            if(progress.wasCanceled())
            {
                return true;
            }
        }
    }
    return false;
}


bool CTrackFilterWidget::filterSplit2Tracks(QDataStream &args, QList<CTrack *> &tracks)
{
    double val;
    args >> val;

    QList<CTrack *> newTracks;

    foreach(CTrack * trk, tracks)
    {
        QList<CTrack::pt_t>& trkpts = trk->getTrackPoints();
        int npts        = trkpts.count();
        int totalCnt    = 0;
        int trkptCnt    = 0;
        int trkCnt      = 1;
        int chunk       = val;

        QProgressDialog progress(groupSplit2->title(), tr("Abort filter"), 0, npts, this);
        progress.setWindowTitle(groupSplit2->title());
        progress.setWindowModality(Qt::WindowModal);

        CTrack * newTrack = new CTrack(&CTrackDB::self());
        newTrack->setName(trk->getName() + QString("_%1").arg(trkCnt++));
        newTrack->setColor(trk->getColorIdx());
        newTracks << newTrack;

        foreach(const CTrack::pt_t& trkpt, trkpts)
        {
            progress.setValue(totalCnt++);

            *newTrack << trkpt;
            if(++trkptCnt >= chunk)
            {
                CTrackDB::self().addTrack(newTrack, true);

                trkptCnt = 0;
                newTrack = new CTrack(&CTrackDB::self());
                newTrack->setName(trk->getName() + QString("_%1").arg(trkCnt++));
                newTrack->setColor(trk->getColorIdx());

                newTracks << newTrack;
            }

            if(progress.wasCanceled())
            {
                return true;
            }
        }
        CTrackDB::self().addTrack(newTrack, true);
    }

    tracks = newTracks;
    return false;
}


bool CTrackFilterWidget::filterSplit3Stages(QDataStream &args, QList<CTrack *> &tracks)
{
    double val;
    args >> val;

    foreach(CTrack * trk, tracks)
    {
        QList<CTrack::pt_t>& trkpts = trk->getTrackPoints();
        int npts        = trkpts.count();
        int totalCnt    = 0;
        int trkCnt      = 1;
        double offset   = 0;

        QProgressDialog progress(groupSplit3->title(), tr("Abort filter"), 0, npts, this);
        progress.setWindowTitle(groupSplit3->title());
        progress.setWindowModality(Qt::WindowModal);

        foreach(const CTrack::pt_t& trkpt, trkpts)
        {
            progress.setValue(totalCnt++);

            if((trkpt.distance - offset) >= val)
            {
                offset = trkpt.distance;
                CWptDB::self().newWpt(trkpt.lon * DEG_TO_RAD, trkpt.lat * DEG_TO_RAD, trkpt.ele, QString("S%1").arg(trkCnt++));
            }

            if(progress.wasCanceled())
            {
                return true;
            }
        }
    }
    return false;
}


bool CTrackFilterWidget::filterSplit3Tracks(QDataStream &args, QList<CTrack *> &tracks)
{
    double val;
    args >> val;

    QList<CTrack *> newTracks;

    foreach(CTrack * trk, tracks)
    {
        QList<CTrack::pt_t>& trkpts = trk->getTrackPoints();
        int npts        = trkpts.count();
        int totalCnt    = 0;
        int trkCnt      = 1;
        double offset   = 0;

        QProgressDialog progress(groupSplit3->title(), tr("Abort filter"), 0, npts, this);
        progress.setWindowTitle(groupSplit3->title());
        progress.setWindowModality(Qt::WindowModal);

        CTrack * newTrack = new CTrack(&CTrackDB::self());
        newTrack->setName(trk->getName() + QString("_%1").arg(trkCnt++));
        newTrack->setColor(trk->getColorIdx());
        newTracks << newTrack;

        foreach(const CTrack::pt_t& trkpt, trkpts)
        {
            progress.setValue(totalCnt++);

            *newTrack << trkpt;
            if((trkpt.distance - offset) >= val)
            {
                offset = trkpt.distance;

                CTrackDB::self().addTrack(newTrack, true);

                newTrack = new CTrack(&CTrackDB::self());
                newTrack->setName(trk->getName() + QString("_%1").arg(trkCnt++));
                newTrack->setColor(trk->getColorIdx());

                newTracks << newTrack;
            }

            if(progress.wasCanceled())
            {
                return true;
            }
        }
        CTrackDB::self().addTrack(newTrack, true);
    }

    tracks = newTracks;
    return false;
}


bool CTrackFilterWidget::filterSplit4Stages(QDataStream &args, QList<CTrack *> &tracks)
{
    double val;
    args >> val;

    foreach(CTrack * trk, tracks)
    {
        QList<CTrack::pt_t>& trkpts = trk->getTrackPoints();
        int npts        = trkpts.count();
        int totalCnt    = 0;
        int trkCnt      = 1;
        double offset   = 0;

        QProgressDialog progress(groupSplit4->title(), tr("Abort filter"), 0, npts, this);
        progress.setWindowTitle(groupSplit4->title());
        progress.setWindowModality(Qt::WindowModal);

        foreach(const CTrack::pt_t& trkpt, trkpts)
        {
            progress.setValue(totalCnt++);

            if((trkpt.ascend - offset) >= val)
            {
                offset = trkpt.ascend;
                CWptDB::self().newWpt(trkpt.lon * DEG_TO_RAD, trkpt.lat * DEG_TO_RAD, trkpt.ele, QString("S%1").arg(trkCnt++));
            }

            if(progress.wasCanceled())
            {
                return true;
            }
        }
    }
    return false;
}


bool CTrackFilterWidget::filterSplit4Tracks(QDataStream &args, QList<CTrack *> &tracks)
{
    double val;
    args >> val;

    QList<CTrack *> newTracks;

    foreach(CTrack * trk, tracks)
    {
        QList<CTrack::pt_t>& trkpts = trk->getTrackPoints();
        int npts        = trkpts.count();
        int totalCnt    = 0;
        int trkCnt      = 1;
        double offset   = 0;

        QProgressDialog progress(groupSplit4->title(), tr("Abort filter"), 0, npts, this);
        progress.setWindowTitle(groupSplit4->title());
        progress.setWindowModality(Qt::WindowModal);

        CTrack * newTrack = new CTrack(&CTrackDB::self());
        newTrack->setName(trk->getName() + QString("_%1").arg(trkCnt++));
        newTrack->setColor(trk->getColorIdx());
        newTracks << newTrack;

        foreach(const CTrack::pt_t& trkpt, trkpts)
        {
            progress.setValue(totalCnt++);

            *newTrack << trkpt;
            if((trkpt.ascend - offset) >= val)
            {
                offset = trkpt.ascend;
                CTrackDB::self().addTrack(newTrack, true);

                newTrack = new CTrack(&CTrackDB::self());
                newTrack->setName(trk->getName() + QString("_%1").arg(trkCnt++));
                newTrack->setColor(trk->getColorIdx());

                newTracks << newTrack;
            }

            if(progress.wasCanceled())
            {
                return true;
            }
        }
        CTrackDB::self().addTrack(newTrack, true);
    }

    tracks = newTracks;
    return false;
}


bool CTrackFilterWidget::filterSplit5Tracks(QDataStream &args, QList<CTrack *> &tracks)
{
    QList<CTrack *> newTracks;

    foreach(CTrack * trk, tracks)
    {
        const QList<CTrack::wpt_t>& wpts = trk->getStageWaypoints();
        if(wpts.isEmpty())
        {
            continue;
        }

        QList<CTrack::pt_t>& trkpts = trk->getTrackPoints();
        int npts        = trkpts.count();
        int totalCnt    = 0;
        int trkCnt      = 1;

        QProgressDialog progress(groupSplit5->title(), tr("Abort filter"), 0, npts, this);
        progress.setWindowTitle(groupSplit5->title());
        progress.setWindowModality(Qt::WindowModal);

        CTrack * newTrack = new CTrack(&CTrackDB::self());
        newTrack->setName(trk->getName() + QString("_%1").arg(trkCnt++));
        newTrack->setColor(trk->getColorIdx());
        newTracks << newTrack;

        QList<CTrack::wpt_t>::const_iterator wpt = wpts.begin();
        foreach(const CTrack::pt_t& trkpt, trkpts)
        {
            progress.setValue(totalCnt++);

            *newTrack << trkpt;

            if(wpt != wpts.end() &&  trkpt == wpt->trkpt)
            {
                wpt++;

                CTrackDB::self().addTrack(newTrack, true);
                newTrack = new CTrack(&CTrackDB::self());
                newTrack->setName(trk->getName() + QString("_%1").arg(trkCnt++));
                newTrack->setColor(trk->getColorIdx());

                newTracks << newTrack;
            }

            if(progress.wasCanceled())
            {
                return true;
            }

        }
        CTrackDB::self().addTrack(newTrack, true);
    }

    tracks = newTracks;
    return false;
}


bool CTrackFilterWidget::filterReset(QDataStream &args, QList<CTrack *> &tracks)
{
    trackEditWidget->slotReset();
    return false;
}


bool CTrackFilterWidget::filterDelete(QDataStream &args, QList<CTrack *> &tracks)
{
    trackEditWidget->slotDelete();
    return false;
}


bool CTrackFilterWidget::filterReplaceElevation(QDataStream &args, QList<CTrack *> &tracks)
{
    QString username;
    quint32 type;
    args >> type >> username;

    SETTINGS;
    cfg.setValue("geonames/username", username);

    foreach(CTrack* trk, tracks)
    {
        if(type == eLocal)
        {
            trk->replaceElevationByLocal(false);
        }
        if(type == eRemote)
        {
            trk->replaceElevationByRemote(false);
        }
    }

    return false;
}


bool CTrackFilterWidget::filterOffsetElevation(QDataStream &args, QList<CTrack *> &tracks)
{
    double offset;
    args >> offset;

    foreach(CTrack * trk, tracks)
    {
        trk->offsetElevation(offset);
    }

    return false;
}


bool CTrackFilterWidget::filterTime1(QDataStream &args, QList<CTrack *> &tracks)
{
    QDateTime time;
    args >> time;

    foreach(CTrack * trk, tracks)
    {
        trk->changeStartTime(time);
    }

    return false;
}


bool CTrackFilterWidget::filterTime2(QDataStream &args, QList<CTrack *> &tracks)
{
    double speed;
    args >> speed;

    foreach(CTrack * trk, tracks)
    {
        trk->changeSpeed(speed/3.6);
    }

    return false;
}


bool CTrackFilterWidget::filterTime3(QDataStream &args, QList<CTrack *> &tracks)
{

    quint32 delta;
    args >> delta;

    foreach(CTrack * trk, tracks)
    {
        if(!trk->unifyTimestamps(delta))
        {
            return false;
        }
    }

    return false;
}
