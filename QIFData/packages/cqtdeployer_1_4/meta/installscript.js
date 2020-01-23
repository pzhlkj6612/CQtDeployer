var VERSION = "1.4"

function Component()
{
    generateTr();
    component.addDependency("QIF");
}

function generateTr() {
    component.setValue("DisplayName", qsTr("CQtDeployer " + VERSION));
    component.setValue("Description", qsTr("This package contains CQtDeployer version " + VERSION));
}


Component.prototype.createOperations = function()
{
//    // call default implementation to actually install README.txt!
    component.createOperations();
    systemIntegration();

}

function systemIntegration() {
    targetDir = installer.value("TargetDir", "");
    homeDir = installer.value("HomeDir", "");

    console.log("install component")
    console.log("targetDir "  + targetDir)
    console.log("hometDir "  + homeDir)

    if (systemInfo.kernelType === "winnt") {

        component.addOperation('Execute', ["SETX", "cqtdeployer", "\"" + targetDir + "/" + VERSION + "/cqtdeployer.exe\""])


    } else {

        if (!installer.fileExists(homeDir + "/.local/bin")) {

            component.addOperation('Execute', ["mkdir", "-p", homeDir + "/.local/bin"])

            QMessageBox["warning"](qsTr("install in system"), qsTr("Installer"),
                qsTr("The \"~/local/bin\" folder was not initialized, you may need to reboot to work correctly!"),
                                   QMessageBox.Ok);

            var ansver = installer.execute('cat', [homeDir + "/.profile"]);
            var result;
            if (ansver.length >= 2) {
                result = ansver[0];
            }

            if (!result.includes("/.local/bin")) {

                var script = '\n# set PATH so it includes users private bin if it exists (generated by cqtdeployer installer) \n' +
                                'if [ -d "$HOME/.local/bin" ] ; then \n' +
                                '    PATH="$HOME/.local/bin:$PATH" \n' +
                                'fi \n';

                component.addOperation('AppendFile', [homeDir + "/.profile", script])
            }

        }
        component.addOperation('Execute', ["ln", "-sf", targetDir + "/" + VERSION + "/cqtdeployer.sh",
                                           homeDir + "/.local/bin/cqtdeployer"],
                               "UNDOEXECUTE", ["rm", "-f", homeDir + "/.local/bin/cqtdeployer"] )

    }

}
