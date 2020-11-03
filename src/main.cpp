#include "Application.hpp"

int main()
{
    Application app;
    app.setVerbose(2);

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Error : " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}