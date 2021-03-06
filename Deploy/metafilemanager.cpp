/*
 * Copyright (C) 2018-2021 QuasarApp.
 * Distributed under the lgplv3 software license, see the accompanying
 * Everyone is permitted to copy and distribute verbatim copies
 * of this license document, but changing it is not allowed.
 */

#include "deploycore.h"
#include "metafilemanager.h"

#include <quasarapp.h>
#include <QDir>
#include <configparser.h>
#include "filemanager.h"

#include <assert.h>

bool MetaFileManager::createRunScriptWindows(const QString &target) {

    auto cnf = DeployCore::_config;
    auto targetinfo = cnf->targets().value(target);
    if (!targetinfo.isValid()) {
        return false;
    }
    auto distro = cnf->getDistro(target);

    QFileInfo targetInfo(target);

    QString content;
    auto runScript = cnf->getRunScript(targetInfo.fileName());
    if (runScript.size()) {
        QFile script(runScript);
        if (!script.open(QIODevice::ReadOnly)) {
            return false;
        }
        content = script.readAll();
        script.close();

    } else {

        bool fGui = DeployCore::isGui(_mudulesMap.value(target));
        auto systemLibsDir = distro.getLibOutDir() + DeployCore::systemLibsFolderName();

        content =
                "@echo off \n"
                "SET BASE_DIR=%~dp0\n"
                "SET PATH=%BASE_DIR%" + distro.getLibOutDir() + ";%PATH%;" + systemLibsDir + "\n"
                "SET CQT_PKG_ROOT=%BASE_DIR%\n"
                "SET CQT_RUN_FILE=%BASE_DIR%%0.bat\n"

                "%3\n";

        // Run application as invoke of the console for consle applications
        // And run gui applciation in the detached mode.
        if (fGui) {
            content += "start \"%0\" %4 \"%BASE_DIR%" + distro.getBinOutDir() + "%1\" %2 \n";
        } else {
            content += "call \"%BASE_DIR%" + distro.getBinOutDir() + "%1\" %2 \n";
        }

        content = content.arg(targetInfo.baseName(), targetInfo.fileName(), "%*",
                              generateCustoScriptBlok(true)); // %0 %1 %2 %3

        content = QDir::toNativeSeparators(content);

        if (fGui) {
            content = content.arg("/B"); // %4
        }
    }

    QString fname = DeployCore::_config->getTargetDir(target) + QDir::separator() + targetInfo.baseName()+ ".bat";

    QFile F(fname);
    if (!F.open(QIODevice::WriteOnly)) {
        return false;
    }

    F.write(content.toUtf8());
    F.flush();
    F.close();

    _fileManager->addToDeployed(fname);

    return F.setPermissions(QFileDevice::ExeOther | QFileDevice::WriteOther |
                            QFileDevice::ReadOther | QFileDevice::ExeUser |
                            QFileDevice::WriteUser | QFileDevice::ReadUser |
                            QFileDevice::ExeOwner | QFileDevice::WriteOwner |
                            QFileDevice::ReadOwner);
}

bool MetaFileManager::createRunScriptLinux(const QString &target) {
    auto cnf = DeployCore::_config;

    if (!cnf->targets().contains(target)) {
        return false;
    }
    auto distro = cnf->getDistro(target);

    QFileInfo targetInfo(target);

    QString content;
    auto runScript = cnf->getRunScript(targetInfo.fileName());
    if (runScript.size()) {
        QFile script(runScript);
        if (!script.open(QIODevice::ReadOnly)) {
            return false;
        }
        content = script.readAll();
        script.close();

    } else {

        auto systemLibsDir = distro.getLibOutDir() + DeployCore::systemLibsFolderName();

        content =
                "#!/bin/sh\n"
                "BASE_DIR=$(dirname \"$(readlink -f \"$0\")\")\n"
                "export "
                "LD_LIBRARY_PATH=\"$BASE_DIR\"" + distro.getLibOutDir() +
                ":\"$BASE_DIR\":$LD_LIBRARY_PATH:\"$BASE_DIR\"" + systemLibsDir + "\n"
                "export QML_IMPORT_PATH=\"$BASE_DIR\"" + distro.getQmlOutDir() + ":$QML_IMPORT_PATH\n"
                "export QML2_IMPORT_PATH=\"$BASE_DIR\"" + distro.getQmlOutDir() + ":$QML2_IMPORT_PATH\n"
                "export QT_PLUGIN_PATH=\"$BASE_DIR\"" + distro.getPluginsOutDir() + ":$QT_PLUGIN_PATH\n"
                "export QTWEBENGINEPROCESS_PATH=\"$BASE_DIR\"" + distro.getBinOutDir() + "QtWebEngineProcess\n"
                "export QTDIR=\"$BASE_DIR\"\n"
                "export CQT_PKG_ROOT=\"$BASE_DIR\"\n"
                "export CQT_RUN_FILE=\"$BASE_DIR/%2\"\n"

                "export "
                "QT_QPA_PLATFORM_PLUGIN_PATH=\"$BASE_DIR\"" + distro.getPluginsOutDir() +
                "platforms:$QT_QPA_PLATFORM_PLUGIN_PATH\n"
                ""
                "%1\n"
                "\"$BASE_DIR" + distro.getBinOutDir() + "%0\" \"$@\"\n";

        content = content.arg(targetInfo.fileName()); // %0

        content = content.arg(generateCustoScriptBlok(false),
                              targetInfo.baseName()+ ".sh"); // %1 %2



    }

    QString fname = DeployCore::_config->getTargetDir(target) + QDir::separator() + targetInfo.baseName()+ ".sh";

    QFile F(fname);
    if (!F.open(QIODevice::WriteOnly)) {
        return false;
    }

    F.write(content.toUtf8());
    F.flush();
    F.close();

    _fileManager->addToDeployed(fname);

    return F.setPermissions(QFileDevice::ExeOther | QFileDevice::WriteOther |
                            QFileDevice::ReadOther | QFileDevice::ExeUser |
                            QFileDevice::WriteUser | QFileDevice::ReadUser |
                            QFileDevice::ExeOwner | QFileDevice::WriteOwner |
                            QFileDevice::ReadOwner);
}

QString MetaFileManager::generateCustoScriptBlok(bool bat) const {
    QString res = "";

    QString commentMarker = "# ";
    if (bat) {
        commentMarker = ":: ";
    }

    auto cstSh = QuasarAppUtils::Params::getArg("customScript", "");
    if (cstSh.size()) {
        res = "\n" +
              commentMarker + "Begin Custom Script (generated by customScript flag)\n"
              "%0\n" +
              commentMarker + "End Custom Script\n"
              "\n";

        res = res.arg(cstSh);
    }

    return res;
}

MetaFileManager::MetaFileManager(FileManager *manager):
    _fileManager(manager)
{
    assert(_fileManager);
}

bool MetaFileManager::createRunScript(const QString &target) {

    QFileInfo info(target);
    auto sufix = info.completeSuffix();

    if (sufix.contains("exe", Qt::CaseSensitive)) {
        return createRunScriptWindows(target);
    }

    if (sufix.isEmpty()) {
        return createRunScriptLinux(target);
    }

    return true;

}

bool MetaFileManager::createQConf(const QString &target) {
    auto cnf = DeployCore::_config;

    if (!cnf->targets().contains(target)) {
        return false;
    }
    auto distro = cnf->getDistro(target);

    QString content =
            "[Paths]\n"
            "Prefix= ." + distro.getRootDir(distro.getBinOutDir()) + "\n"
            "Libraries= ." + distro.getLibOutDir() + "\n"
            "Plugins= ." + distro.getPluginsOutDir() + "\n"
            "Imports= ." + distro.getQmlOutDir() + "\n"
            "Translations= ." + distro.getTrOutDir() + "\n"
            "Qml2Imports= ." + distro.getQmlOutDir() + "\n";


    content.replace("//", "/");
    content = QDir::fromNativeSeparators(content);

    QString fname = DeployCore::_config->getTargetDir(target) + distro.getBinOutDir() + "qt.conf";

    QFile F(fname);
    if (!F.open(QIODevice::WriteOnly)) {
        return false;
    }

    F.write(content.toUtf8());
    F.flush();
    F.close();

    _fileManager->addToDeployed(fname);

    return F.setPermissions(QFileDevice::ExeOther | QFileDevice::WriteOther |
                            QFileDevice::ReadOther | QFileDevice::ExeUser |
                            QFileDevice::WriteUser | QFileDevice::ReadUser |
                            QFileDevice::ExeOwner | QFileDevice::WriteOwner |
                            QFileDevice::ReadOwner);
}

void MetaFileManager::createRunMetaFiles(const QHash<QString, DeployCore::QtModule>& modulesMap) {

    _mudulesMap = modulesMap;
    for (auto i = DeployCore::_config->targets().cbegin(); i != DeployCore::_config->targets().cend(); ++i) {

        if (!createRunScript(i.key())) {
            QuasarAppUtils::Params::log("Failed to create a run script: " + i.key(),
                                         QuasarAppUtils::Error);
        }

        if (!createQConf(i.key())) {
            QuasarAppUtils::Params::log("Failed to create the qt.conf file", QuasarAppUtils::Warning);
        }
    }
}
