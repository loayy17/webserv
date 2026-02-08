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
String formatTime(time_t t) {
    char* raw = ctime(&t);
    if (!raw)
        return "-";
    String result(raw);
    if (!result.empty() && result[result.size() - 1] == '\n')
        result.erase(result.size() - 1);
    return result;
}

String toUpperWords(const String& str) {
    String result = str;
    for (size_t i = 0; i < result.size(); ++i) {
        if (result[i] >= 'a' && result[i] <= 'z')
            result[i] = std::toupper(result[i]);
    }
    return result;
}
String toLowerWords(const String& str) {
    String result = str;
    for (size_t i = 0; i < result.size(); ++i) {
        if (result[i] >= 'A' && result[i] <= 'Z')
            result[i] = std::tolower(result[i]);
    }
    return result;
}

String cleanCharEnd(const String& v, char c) {
    if (!v.empty() && v[v.size() - 1] == c)
        return v.substr(0, v.size() - 1);
    return v;
}

String trimSpaces(const String& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == String::npos)
        return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

String findValueStrInMap(const MapString& map, const String& key) {
    MapString::const_iterator it = map.find(key);
    if (it != map.end()) {
        return it->second;
    }
    return "";
}
String findValueIntInMap(const MapIntString& map, int key) {
    MapIntString::const_iterator it = map.find(key);
    if (it != map.end()) {
        return it->second;
    }
    return "";
}
String findValueInVector(const VectorString& map, const String& key) {
    for (size_t i = 0; i < map.size(); i++) {
        if (map[i] == key) {
            return map[i];
        }
    }
    return "";
}

String trimSpacesComments(const String& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == String::npos || s[start] == '#')
        return "";
    size_t end = s.find('#', start);
    if (end != String::npos)
        end--;
    else
        end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

bool convertFileToLines(String file, VectorString& lines) {
    std::ifstream ff(file.c_str());
    if (!ff.is_open()) {
        std::cerr << "Error: Could not open file " << file << std::endl;
        return false;
    }

    String line;
    String current;

    while (std::getline(ff, line)) {
        for (size_t i = 0; i < line.size(); ++i) {
            char c = line[i];
            if (c == '{' || c == ';' || c == '}') {
                String t = trimSpacesComments(current);
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
        String t = trimSpacesComments(current);

        if (!t.empty())
            lines.push_back(t);
        current.clear();
    }
    ff.close();
    return true;
}
struct stat getFileStat(const String& path) {
    struct stat st;
    String      actualPath = path;

    if (stat(actualPath.c_str(), &st) == 0)
        return st;

    if (actualPath.size() > 2 && actualPath[0] == '.' && actualPath[1] == '/') {
        actualPath = actualPath.substr(2);
        if (stat(actualPath.c_str(), &st) == 0)
            return st;
    }

    if (!actualPath.empty() && actualPath[0] == '/') {
        actualPath = "." + actualPath;
        if (stat(actualPath.c_str(), &st) == 0)
            return st;

        actualPath = actualPath.substr(1);
        if (stat(actualPath.c_str(), &st) == 0)
            return st;
    }

    // mark invalid
    st.st_mode = 0;
    return st;
}

FileType getFileType(const struct stat& st) {
    if (st.st_mode == 0)
        return UNKNOWN;

    if (S_ISDIR(st.st_mode))
        return DIRECTORY;

    if (S_ISREG(st.st_mode))
        return SINGLEFILE;

    return UNKNOWN;
}
FileType getFileType(const String& path) {
    struct stat st = getFileStat(path);
    return getFileType(st);
}

String sanitizeFilename(const String& filename) {
    String safe;
    for (size_t i = 0; i < filename.size(); ++i) {
        char c = filename[i];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '_')
            safe += c;
        else
            safe += '_';
    }
    return safe;
}

String htmlEntities(const String& str) {
    String result;
    for (size_t i = 0; i < str.size(); ++i) {
        char c = str[i];
        if (c == '&')
            result += "&amp;";
        else if (c == '<')
            result += "&lt;";
        else if (c == '>')
            result += "&gt;";
        else if (c == '"')
            result += "&quot;";
        else if (c == '\'')
            result += "&#39;";
        else
            result += c;
    }
    return result;
}

bool readFileContent(const String& filePath, String& content) {
    content.clear();
    std::ifstream file(filePath.c_str(), std::ios::in | std::ios::binary);
    if (!file.is_open())
        return false;

    std::ostringstream ss;
    ss << file.rdbuf();
    content = ss.str();
    return true;
}

bool checkAllowedMethods(const String& m) {
    VectorString methods;
    methods.push_back("GET");
    methods.push_back("POST");
    methods.push_back("DELETE");
    methods.push_back("PUT");
    methods.push_back("PATCH");
    methods.push_back("HEAD");
    methods.push_back("OPTIONS");
    return isKeyInVector(m, methods);
}
// ./www/html/index.html
bool splitByChar(const String& line, String& key, String& value, char endChar, bool reverse) {
    String s   = line;
    size_t pos = reverse ? s.rfind(endChar) : s.find(endChar);
    if (pos == String::npos)
        return false;
    key   = s.substr(0, pos);
    value = s.substr(pos + 1);
    return true;
}

bool splitByString(const String& line, VectorString& values, const String& delimiter) {
    size_t start = 0;
    size_t end   = line.find(delimiter);
    while (end != String::npos) {
        values.push_back(line.substr(start, end - start));
        start = end + delimiter.length();
        end   = line.find(delimiter, start);
    }
    values.push_back(line.substr(start));
    return !values.empty();
}

bool parseKeyValue(const String& line, String& key, VectorString& values) {
    std::stringstream ss(line);
    ss >> key;
    if (key.empty())
        return false;

    String v;
    while (ss >> v) {
        values.push_back(cleanCharEnd(v, ';'));
    }
    return !values.empty();
}

size_t convertMaxBodySize(const String& maxBody) {
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
String formatSize(double size) {
    String units[] = {"B", "KB", "MB", "GB", "TB"};
    int    unit    = 0;

    while (size >= 1024.0 && unit < 4) {
        size /= 1024.0;
        unit++;
    }
    size_t num    = static_cast<size_t>(size);
    String result = typeToString<size_t>(num);
    if (unit) {
        size_t dicemal = static_cast<size_t>(size - num) * 100;
        if (dicemal > 0) {
            result += "'";
            result += typeToString<size_t>(dicemal);
        }
    }
    result += units[unit];
    return result;
}

String normalizePath(const String& path) {
    String normalized = path;

    if (normalized.empty())
        return "/";
    String result;
    if (normalized[0] != '/')
        result += '/';
    for (size_t i = 0; i < normalized.length(); i++) {
        if (normalized[i] == '/' && i + 1 < normalized.length() && normalized[i + 1] == '/')
            continue;
        result += normalized[i];
    }
    return result;
}

String joinPaths(const String& firstPath, const String& secondPath) {
    if (firstPath.empty())
        return secondPath.empty() ? "/" : secondPath;
    if (secondPath.empty())
        return firstPath;
    String result = firstPath;
    if (result[result.size() - 1] == '/' && secondPath[0] == '/')
        result += secondPath.substr(1);
    else if (result[result.size() - 1] != '/' && secondPath[0] != '/')
        result += "/" + secondPath;
    else
        result += secondPath;

    return result;
}

bool pathStartsWith(const String& path, const String& prefix) {
    if (prefix.length() > path.length())
        return false;
    return path.compare(0, prefix.length(), prefix) == 0;
}