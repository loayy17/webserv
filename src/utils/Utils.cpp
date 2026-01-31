#include "Utils.hpp"
time_t getCurrentTime() {
    return time(NULL);
}
void updateTime(time_t& t) {
    t = getCurrentTime();
}
time_t getDifferentTime(const time_t& start, const time_t& end) {
    return end - start;
}

std::string toUpperWords(const std::string& str) {
    std::string result = str;
    for (size_t i = 0; i < result.size(); ++i) {
        if (result[i] >= 'a' && result[i] <= 'z')
            result[i] = std::toupper(result[i]);
    }
    return result;
}
std::string toLowerWords(const std::string& str) {
    std::string result = str;
    for (size_t i = 0; i < result.size(); ++i) {
        if (result[i] >= 'A' && result[i] <= 'Z')
            result[i] = std::tolower(result[i]);
    }
    return result;
}

std::string cleanCharEnd(const std::string& v, char c) {
    if (!v.empty() && v[v.size() - 1] == c)
        return v.substr(0, v.size() - 1);
    return v;
}

std::string trimSpaces(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::string trimSpacesComments(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos || s[start] == '#')
        return "";
    size_t end = s.find('#', start);
    if (end != std::string::npos)
        end--;
    else
        end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

bool convertFileToLines(std::string file, std::vector<std::string>& lines) {
    std::ifstream ff(file.c_str());
    if (!ff.is_open()) {
        return false;
    }

    std::string line;
    std::string current;

    while (std::getline(ff, line)) {
        for (size_t i = 0; i < line.size(); ++i) {
            char c = line[i];
            if (c == '{' || c == ';' || c == '}') {
                std::string t = trimSpacesComments(current);
                if (!t.empty())
                    lines.push_back(t + ((c == '{') ? " {" : (c == ';') ? ";" : ""));
                else if (c == '{')
                    lines[lines.size() - 1] += " {";
                if (c == '}')
                    lines.push_back("}");
                current.clear();
            } else {
                current += c;
            }
        }
        std::string t = trimSpacesComments(current);

        if (!t.empty())
            lines.push_back(t);
        current.clear();
    }
    ff.close();
    return true;
}

bool checkAllowedMethods(const std::string& m) {
    std::vector<std::string> methods;
    methods.push_back("GET");
    methods.push_back("POST");
    methods.push_back("DELETE");
    methods.push_back("PUT");
    methods.push_back("PATCH");
    methods.push_back("HEAD");
    methods.push_back("OPTIONS");
    return isStringInVector(m, methods);
}

bool splitByChar(const std::string& line, std::string& key, std::string& value, char endChar) {
    std::string s   = line;
    size_t      pos = s.find(endChar);
    if (pos == std::string::npos)
        return false;
    key   = s.substr(0, pos);
    value = s.substr(pos + 1);
    return true;
}

bool splitByString(const std::string& line, std::vector<std::string>& values, const std::string& delimiter) {
    size_t start = 0;
    size_t end   = line.find(delimiter);
    while (end != std::string::npos) {
        values.push_back(line.substr(start, end - start));
        start = end + delimiter.length();
        end   = line.find(delimiter, start);
    }
    values.push_back(line.substr(start));
    return !values.empty();
}

bool parseKeyValue(const std::string& line, std::string& key, std::vector<std::string>& values) {
    std::stringstream ss(line);
    ss >> key;
    if (key.empty())
        return false;

    std::string v;
    while (ss >> v) {
        values.push_back(cleanCharEnd(v, ';'));
    }
    return !values.empty();
}

size_t convertMaxBodySize(const std::string& maxBody) {
    if (maxBody.empty())
        return 0;

    size_t            size = 0;
    char              unit = maxBody[maxBody.size() - 1];
    std::stringstream ss(std::isdigit(unit) ? maxBody : maxBody.substr(0, maxBody.size() - 1));
    ss >> size;
    if (unit == 'K' || unit == 'k')
        return size * 1024;
    if (unit == 'M' || unit == 'm')
        return size * 1024 * 1024;
    if (unit == 'G' || unit == 'g')
        return size * 1024 * 1024 * 1024;
    return size;
}

bool isStringInVector(const std::string& toFind, const std::vector<std::string>& fromFind) {
    for (size_t i = 0; i < fromFind.size(); i++) {
        if (fromFind[i] == toFind) {
            return true;
        }
    }
    return false;
}

std::string normalizePath(const std::string& path) {
    std::string normalized = path;

    if (normalized.empty())
        return "/";
    std::string result;
    if (normalized[0] != '/')
        result += '/';
    for (size_t i = 0; i < normalized.length(); i++) {
        if (normalized[i] == '/' && i + 1 < normalized.length() && normalized[i + 1] == '/')
            continue;
        result += normalized[i];
    }
    return result;
}

bool pathStartsWith(const std::string& path, const std::string& prefix) {
    if (prefix.length() > path.length())
        return false;
    return path.compare(0, prefix.length(), prefix) == 0;
}