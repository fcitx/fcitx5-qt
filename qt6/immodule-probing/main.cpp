/*
 * SPDX-FileCopyrightText: 2023~2023 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include <QGuiApplication>
#include <iostream>
#include <private/qguiapplication_p.h>
#include <qpa/qplatforminputcontext.h>
#include <qpa/qplatforminputcontextfactory_p.h>
#include <qpa/qplatformintegration.h>

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    std::cout << "QT_QPA_PLATFORM=" << app.platformName().toStdString()
              << std::endl;
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    std::cout << "QT_IM_MODULE=";
    auto qt_im_modules = QPlatformInputContextFactory::requested();
    for (int i = 0; i < qt_im_modules.size(); i++)
    {
        std::cout << qt_im_modules[i].toStdString();
        if (i < (qt_im_modules.size() - 1))
            std::cout << ";";
    }
    std::cout << std::endl;
#else
    std::cout << "QT_IM_MODULE="
              << QPlatformInputContextFactory::requested().toStdString()
              << std::endl;
#endif
    auto inputContext =
        QGuiApplicationPrivate::platformIntegration()->inputContext();
    std::cout << "IM_MODULE_CLASSNAME=";
    if (inputContext) {
        std::cout << inputContext->metaObject()->className();
    }
    std::cout << std::endl;
    return 0;
}
