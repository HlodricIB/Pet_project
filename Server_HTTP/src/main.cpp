#include <iostream>
#include <vector>
#include "server_http.h"

int main()
{
    std::shared_ptr<Mime_types> mime_type = std::make_shared<Audio_mime_type>();
    auto find_file = std::make_shared<Find_file>("/home/nikita/C++/Pet_project/songs_folder");
    std::vector<std::string_view> filenames {"song2*", "song2.wav", "s", "25", "11"};
    for (auto filename : filenames)
    {
        if (find_file->find(filename))
        {
            std::cout << filename << '\t' << mime_type->mime_type(filename) << std::endl;
        } else {
            std::cerr << filename << std::endl;
        }
    }
}
