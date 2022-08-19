#include <functional>
#include "server_http.h"

std::string_view Audio_mime_type::mime_type(std::string_view target)
{
    size_t pos = target.find_last_of('.');
    if (pos == s_w::npos)
    {
        return "application/octet-stream";
    }
    auto target_ext = target.substr(pos + 1);
    auto found = types.find(target_ext);
    if (found != types.end())
    {
        return found->second;
    }
    return "application/octet-stream";
}

bool Find_file::find(std::string_view& target)
{
    if (target.size() == 0 || target.back() == '/' || target.find("..") != std::string_view::npos)
    {
        target = "Illegal target filename";
        return false;
    }
    static std::function<bool(std::string_view)> compare;
    if (target.back() == '*')
    {
        target.remove_suffix(1);
        compare = [&target] (std::string_view curr_file)->bool {
            if (curr_file.starts_with(target))
            {
                target = curr_file;
                return true;
            }
            return false; };
    } else {
        compare = [target] (std::string_view curr_file)->bool { return curr_file == target; };
    }
    std::string_view curr_file;
    for (auto const& dir_entry : std::filesystem::directory_iterator{files_path})
    {
        if (dir_entry.is_regular_file())
        {
            curr_file = dir_entry.path().filename().c_str();
            if (compare(curr_file))
            {
                return true;
            }
        }
    }
    target = "Target file not found";
    return false;
}

