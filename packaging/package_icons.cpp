// Rasterize the project's existing SVG for OS-native application icons.
#include <QBuffer>
#include <QCoreApplication>
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QPainter>
#include <QSvgRenderer>

#include <array>
#include <cstdlib>

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    if (argc != 3) return EXIT_FAILURE;
    QSvgRenderer svg(QString::fromLocal8Bit(argv[1]));
    const QDir output(QString::fromLocal8Bit(argv[2]));
    if (!svg.isValid() || !output.mkpath("elektronika-mk61.iconset"))
        return EXIT_FAILURE;
    auto render = [&svg](int size) {
        QImage image(size, size, QImage::Format_ARGB32);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        svg.render(&painter);
        return image;
    };
    constexpr std::array sizes{16, 32, 48, 64, 128, 256};
    std::array<QByteArray, sizes.size()> frames;
    for (std::size_t i = 0; i < sizes.size(); ++i) {
        QBuffer buffer(&frames[i]);
        if (!buffer.open(QIODevice::WriteOnly) || !render(sizes[i]).save(&buffer, "PNG"))
            return EXIT_FAILURE;
    }
    QFile ico(output.filePath("elektronika-mk61.ico"));
    if (!ico.open(QIODevice::WriteOnly)) return EXIT_FAILURE;
    QDataStream stream(&ico);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream << quint16(0) << quint16(1) << quint16(sizes.size());
    quint32 offset = 6 + 16 * static_cast<quint32>(sizes.size());
    for (std::size_t i = 0; i < sizes.size(); ++i) {
        const auto extent = static_cast<quint8>(sizes[i] == 256 ? 0 : sizes[i]);
        stream << extent << extent << quint8(0) << quint8(0)
               << quint16(1) << quint16(32) << quint32(frames[i].size()) << offset;
        offset += static_cast<quint32>(frames[i].size());
    }
    for (const auto& frame : frames)
        if (stream.writeRawData(frame.constData(), static_cast<int>(frame.size())) != frame.size())
            return EXIT_FAILURE;
    if (stream.status() != QDataStream::Ok) return EXIT_FAILURE;
    ico.close();
    for (const int size : {16, 32, 128, 256, 512}) {
        const QString prefix = output.filePath("elektronika-mk61.iconset/icon_%1x%1").arg(size);
        if (!render(size).save(prefix + ".png") || !render(size * 2).save(prefix + "@2x.png"))
            return EXIT_FAILURE;
    }
    QFile stamp(output.filePath("iconset.stamp"));
    return stamp.open(QIODevice::WriteOnly) ? EXIT_SUCCESS : EXIT_FAILURE;
}
