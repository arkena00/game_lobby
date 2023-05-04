#include <gl/core.hpp>

#include <fstream>

int main()
{
    std::ifstream ifs{ "token.txt" };
    if (ifs.is_open())
    {
        std::string token{ std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>() };
        gl::core core{ token };
        core.run();
    }
    else
    {
        std::cout << "error: token.txt not found";
    }

    return 0;
}