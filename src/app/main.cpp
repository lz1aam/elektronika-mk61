#include "calculator_widget.hpp"

#include <QApplication>
#include <QIcon>
#include <QPixmap>
#include <QTimer>

#include <cstdlib>

int main(int argc, char** argv) {
    QApplication::setApplicationName(QStringLiteral("elektronika-mk61"));
    QApplication::setApplicationDisplayName(
        QStringLiteral("Elektronika MK-61 Emulator"));
    QApplication::setApplicationVersion(QStringLiteral(PMK_VERSION));
    QApplication app(argc, argv);
    const QIcon applicationIcon(
        QStringLiteral(":/icons/elektronika-mk61.svg"));
    QApplication::setWindowIcon(applicationIcon);

    CalculatorWidget widget;
    widget.setWindowIcon(applicationIcon);
    widget.show();

    const QStringList arguments = app.arguments();
    qsizetype renderIndex = arguments.indexOf(QStringLiteral("--render"));
    const qsizetype stackRenderIndex =
        arguments.indexOf(QStringLiteral("--render-stack"));
    if (stackRenderIndex >= 0) {
        renderIndex = stackRenderIndex;
        widget.setStackDisplayExpanded(true);
    }
    if (renderIndex >= 0 && renderIndex + 1 < arguments.size()) {
        const QString outputPath = arguments.at(renderIndex + 1);
        QTimer::singleShot(250, &app, [&app, &widget, outputPath] {
            const bool saved = widget.grab().save(outputPath, "PNG");
            app.exit(saved ? EXIT_SUCCESS : EXIT_FAILURE);
        });
    } else if (arguments.contains(QStringLiteral("--smoke-test"))) {
        QTimer::singleShot(150, &app, &QCoreApplication::quit);
    }

    return app.exec();
}

