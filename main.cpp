#include "src/Core/Application.h"
#include "tests/RendererUnitTests.h"

int main() {
    RendererUnitTests::runAll();
    Application app(800, 800, "Scene");

    if (app.init()) {
        app.run();
    }
    return 0;
}
