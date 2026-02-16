#pragma once

class Interpreter;

// Blocks and handles HTTP requests; for each request calls interp->setRequestData + interp->callHandler
void runHttpServer(Interpreter* interp, int port);
