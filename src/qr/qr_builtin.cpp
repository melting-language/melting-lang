#include "qr_builtin.hpp"
#include "interpreter.hpp"
#include <sstream>
#include <string>

#if defined(USE_QR)
#include <qrencode.h>
#endif

#if defined(USE_QR)
static std::string qrToSvg(QRcode* qr, int cellSize) {
    if (!qr || !qr->data) return "";
    const int w = qr->width;
    const int size = w * cellSize;
    std::ostringstream out;
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 " << size << " " << size << "\" width=\"" << size << "\" height=\"" << size << "\">\n";
    out << "<rect width=\"100%\" height=\"100%\" fill=\"white\"/>\n";
    for (int i = 0; i < w; ++i) {
        for (int j = 0; j < w; ++j) {
            if (qr->data[i * w + j] & 1) {
                out << "<rect x=\"" << (j * cellSize) << "\" y=\"" << (i * cellSize)
                    << "\" width=\"" << cellSize << "\" height=\"" << cellSize << "\" fill=\"black\"/>\n";
            }
        }
    }
    out << "</svg>";
    return out.str();
}
#endif

void registerQrBuiltins(Interpreter* interp) {
#if defined(USE_QR)
    interp->registerBuiltin("qrGenerate", [](Interpreter* i, std::vector<Value> args) -> Value {
        (void)i;
        if (args.empty()) return std::string("");
        std::string text;
        if (std::holds_alternative<std::string>(args[0]))
            text = std::get<std::string>(args[0]);
        else if (std::holds_alternative<double>(args[0]))
            text = std::to_string((int)std::get<double>(args[0]));
        else
            return std::string("");
        int cellSize = 4;
        if (args.size() >= 2 && std::holds_alternative<double>(args[1])) {
            int v = (int)std::get<double>(args[1]);
            if (v >= 1 && v <= 32) cellSize = v;
        }
        QRcode* qr = QRcode_encodeString(text.c_str(), 0, QR_ECLEVEL_L, QR_MODE_8, 1);
        if (!qr) return std::string("");
        std::string svg = qrToSvg(qr, cellSize);
        QRcode_free(qr);
        return svg;
    });
#else
    (void)interp;
#endif
}
