#pragma once
#include "rbk/mapExtensor/mapV2.h"
#include <QByteArray>
#include <QString>

const mapV2<QByteArray, bool>& getNationISO2();

const mapV2<QByteArray, QByteArray>& getNationISO3();

const mapV2<QByteArray, QByteArray>& ISO2_Remap();

bool                              validNation(const QByteArray& nation, bool okLatam = false);
const mapV2<QByteArray, quint16>& getNationISO2Adapted();
const QMap<QString, QString>&     getNationsIsoCodes();
QString                           getNationIsoCode(const QString& nation, const QString def = "");
