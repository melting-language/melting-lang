#pragma once

class Interpreter;

// Call from registerBuiltins() to add qrGenerate when USE_QR is defined.
void registerQrBuiltins(Interpreter* interp);
