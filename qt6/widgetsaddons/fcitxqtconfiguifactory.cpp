/*
 * SPDX-FileCopyrightText: 2017~2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "fcitxqtconfiguifactory.h"
#include "fcitxqtconfiguifactory_p.h"
#include "fcitxqtconfiguiplugin.h"
#include <QDebug>
#include <QDir>
#include <QLatin1String>
#include <QLibrary>
#include <QObject>
#include <QPluginLoader>
#include <QStandardPaths>
#include <fcitx-utils/i18n.h>
#include <fcitx-utils/standardpaths.h>
#include <filesystem>

namespace fcitx {

namespace {

constexpr char addonConfigPrefix[] = "fcitx://config/addon/";

QString normalizePath(const QString &file) {
    auto path = file;
    if (path.startsWith(addonConfigPrefix)) {
        path.remove(0, sizeof(addonConfigPrefix) - 1);
    }
    return path;
}

} // namespace

FcitxQtConfigUIFactoryPrivate::FcitxQtConfigUIFactoryPrivate(
    FcitxQtConfigUIFactory *factory)
    : QObject(factory), q_ptr(factory) {}

FcitxQtConfigUIFactoryPrivate::~FcitxQtConfigUIFactoryPrivate() {}

FcitxQtConfigUIFactory::FcitxQtConfigUIFactory(QObject *parent)
    : QObject(parent), d_ptr(new FcitxQtConfigUIFactoryPrivate(this)) {
    Q_D(FcitxQtConfigUIFactory);
    d->scan();
}

FcitxQtConfigUIFactory::~FcitxQtConfigUIFactory() {}

FcitxQtConfigUIWidget *FcitxQtConfigUIFactory::create(const QString &file) {
    Q_D(FcitxQtConfigUIFactory);

    auto path = normalizePath(file);
    auto *loader = d->plugins_.value(path);
    if (!loader) {
        return nullptr;
    }

    auto *instance =
        qobject_cast<FcitxQtConfigUIFactoryInterface *>(loader->instance());
    if (!instance) {
        return nullptr;
    }
    return instance->create(path.section('/', 1));
}

bool FcitxQtConfigUIFactory::test(const QString &file) {
    Q_D(FcitxQtConfigUIFactory);

    auto path = normalizePath(file);
    return d->plugins_.contains(path);
}

void FcitxQtConfigUIFactoryPrivate::scan() {
    auto addonFiles = fcitx::StandardPaths::global().locate(
        fcitx::StandardPathsType::Addon, "qt6",
        [](const std::filesystem::path &path) {
            return QLibrary::isLibrary(QString::fromStdString(path));
        },
        StandardPathsMode::System);

    for (const auto &[_, filePath] : addonFiles) {
        auto *loader =
            new QPluginLoader(QString::fromStdString(filePath), this);
        if (loader->metaData().value("IID") !=
            QLatin1String(FcitxQtConfigUIFactoryInterface_iid)) {
            delete loader;
            continue;
        }
        auto metadata = loader->metaData().value("MetaData").toObject();
        auto files = metadata.value("files").toVariant().toStringList();
        auto addon = metadata.value("addon").toVariant().toString();
        for (const auto &file : files) {
            plugins_[addon + "/" + file] = loader;
        }
    }
}
} // namespace fcitx
