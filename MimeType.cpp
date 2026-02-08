#include "src/config/MimeTypes.hpp"
int main(int ac, char** av) {
    if (ac < 2) {
        std::cerr << "Usage: " << av[0] << " <assetsPath>" << std::endl;
        return 1;
    }
    MimeTypes mime;
    std::cout << "MIME type: " << mime.get("index.ico") << std::endl;
    return 0;
}