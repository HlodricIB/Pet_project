#include <iostream>
#include "server_http.h"

int main()
{
    std::string filename;
    filename = full_filename("song2*");
    std::cout << filename << '\t' << mime_type(filename) << std::endl;
    filename = full_filename("song2.wav");
    std::cout << filename << '\t' << mime_type(filename) << std::endl;
    filename = full_filename("song1*");
    std::cout << filename << '\t' << mime_type(filename) << std::endl;
    filename = full_filename("s");
    std::cout << filename << '\t' << mime_type(filename) << std::endl;

    return 0;
}
