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

String trimQuotes(const String& s) {
    if (s.size() >= 2 && ((s[0] == '"' && s[s.size() - 1] == '"') || (s[0] == '\'' && s[s.size() - 1] == '\'')))
        return s.substr(1, s.size() - 2);
    return s;
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
    file.close();
    return true;
}
bool fileExists(const String& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0;
}

bool checkAllowedMethods(const String& m) {
    VectorString methods;
    methods.push_back(METHOD_GET);
    methods.push_back(METHOD_POST);
    methods.push_back(METHOD_DELETE);
    methods.push_back(METHOD_PUT);
    methods.push_back(METHOD_PATCH);
    methods.push_back(METHOD_HEAD);
    methods.push_back(METHOD_OPTIONS);
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
    key = trimQuotes(key);
    String v;
    while (ss >> v) {
        values.push_back(trimQuotes(cleanCharEnd(v, ';')));
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
    if (path.compare(0, prefix.length(), prefix) != 0)
        return false;
    if (prefix == "/")
        return true;
    if (path.length() == prefix.length() || path[prefix.length()] == '/')
        return true;

    return false;
}

bool ensureDirectoryExists(const String& dirPath) {
    struct stat st;
    if (stat(dirPath.c_str(), &st) == -1) {
        if (mkdir(dirPath.c_str(), DIR_PERMISSIONS) == -1)
            return false;
    }
    return true;
}

String extractFileExtension(const String& path) {
    size_t dotPos = path.rfind(DOT);
    if (dotPos == String::npos)
        return "";
    return path.substr(dotPos);
}

String extractFilenameFromHeader(const String& contentDisposition) {
    size_t filenamePos = contentDisposition.find("filename=");
    if (filenamePos == String::npos)
        return "";

    size_t start = filenamePos + 9; // length of "filename="
    if (start >= contentDisposition.length())
        return "";

    // Handle quoted filenames
    if (contentDisposition[start] == '"') {
        start++;
        size_t end = contentDisposition.find('"', start);
        if (end == String::npos)
            return "";
        return contentDisposition.substr(start, end - start);
    }

    // Handle unquoted filenames
    size_t end = contentDisposition.find(SEMICOLON, start);
    if (end == String::npos)
        end = contentDisposition.length();
    return trimSpaces(contentDisposition.substr(start, end - start));
}

String extractBoundaryFromContentType(const String& contentType) {
    size_t boundaryPos = contentType.find("boundary=");
    if (boundaryPos == String::npos)
        return "";

    size_t start = boundaryPos + 9; // length of "boundary="
    if (start >= contentType.length())
        return "";

    // Find end of boundary (semicolon or end of string)
    size_t end = contentType.find(SEMICOLON, start);
    if (end == String::npos)
        end = contentType.length();

    return trimSpaces(contentType.substr(start, end - start));
}

bool parseMultipartFormData(const String& body, const String& boundary, String& filename, String& fileContent) {
    if (boundary.empty() || body.empty())
        return false;

    // Construct the boundary markers
    String startBoundary = "--" + boundary;
    String endBoundary   = "--" + boundary + "--";

    // Find the first part
    size_t partStart = body.find(startBoundary);
    if (partStart == String::npos)
        return false;

    partStart += startBoundary.length();

    // Skip CRLF after boundary
    if (partStart + 2 <= body.length() && body.substr(partStart, 2) == CRLF)
        partStart += 2;

    // Find the next boundary (end of this part)
    size_t partEnd = body.find(startBoundary, partStart);
    if (partEnd == String::npos)
        return false;

    String part = body.substr(partStart, partEnd - partStart);

    // Find the separator between headers and content (double CRLF)
    size_t headerEnd = part.find(DOUBLE_CRLF);
    if (headerEnd == String::npos)
        return false;

    String headers = part.substr(0, headerEnd);
    String content = part.substr(headerEnd + 4); // Skip \r\n\r\n

    // Remove trailing CRLF from content
    while (content.length() >= 2 && content.substr(content.length() - 2) == CRLF)
        content = content.substr(0, content.length() - 2);

    // Parse headers to find Content-Disposition
    size_t pos = 0;
    while (pos < headers.length()) {
        size_t lineEnd = headers.find(CRLF, pos);
        if (lineEnd == String::npos)
            lineEnd = headers.length();

        String line = headers.substr(pos, lineEnd - pos);

        if (toLowerWords(line).find("content-disposition") != String::npos) {
            filename = extractFilenameFromHeader(line);
        }

        pos = lineEnd + 2;
    }

    fileContent = content;
    return !filename.empty();
}

String GUID() {
    String guid;
    for (int i = 0; i < GUID_LENGTH; i++) {
        if (i == 8 || i == 12 || i == 16 || i == 20)
            guid += '-';
        int index = std::rand() % (sizeof(GUID_CHARSET) - 1);
        guid += GUID_CHARSET[index];
    }
    return guid;
}