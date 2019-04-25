#include "mainwindow.h"
#include <QApplication>
#include <QCommandLineParser>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("Repsnapper");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("file", QCoreApplication::translate("main", "file to open."));
    parser.process(a);

    const QStringList args = parser.positionalArguments();

    MainWindow w;
    w.show();

    if (args.size() > 0){
        for (QString a : args)
            w.openFile(a);
    }

    return a.exec();
}
