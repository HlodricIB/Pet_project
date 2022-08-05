#include <map>
#include <filesystem>
#include "server_http.h"

std::string_view mime_type(std::string_view target)
{
    using s_w = std::string_view;
    static const std::map<s_w, s_w> types{
        {"aac", "audio/aac"},
        {"mid", "audio/midi audio/x-midi"},
        {"midi", "audio/midi audio/x-midi"},
        {"mp3", "audio/mpeg"},
        {"oga", "audio/ogg"},
        {"wav", "audio/wav"},
        {"weba", "audio/webm"},
        {"3gp", "audio/3gpp"},
        {"3g2", "audio/3gpp2"}};
    size_t pos = target.find_last_of('.');
    if (pos == s_w::npos)
    {
        return "application/octet-stream";
    } else {
        auto target_ext = target.substr(pos + 1);
        auto found = types.find(target_ext);
        if (found != types.end())
        {
            return found->second;
        } else {
            return "application/octet-stream";
        }
    }
}

std::string full_filename(std::string_view target)
{
    if (target.back() == '*')
    {
        target.remove_suffix(1);
    } else
    {
        return std::string(target);
    }
    namespace f_s = std::filesystem;
    f_s::path songs_dir{"/home/nikita/C++/Pet_project/songs_folder"};
    std::string_view curr_file;
    for (auto const& dir_entry : f_s::directory_iterator{songs_dir})
    {
        if (dir_entry.is_regular_file())
        {
            curr_file = dir_entry.path().filename().c_str();
            if (curr_file.starts_with(target))
            {
                return std::string(curr_file);
            }
        }
    }
    return std::string{};
}

