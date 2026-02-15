#include "Utils.hpp"

// ============================================================================
// Time Methods
// ============================================================================

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
    // Remove the trailing newline added by ctime
    if (!result.empty() && result[result.size() - 1] == '\n')
        result.erase(result.size() - 1);
    return result;
}

// ============================================================================
// String Methods
// ============================================================================

String toUpperWords(const String& str) {
    String result = str;
    for (size_t i = 0; i < result.size(); ++i) {
        result[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(result[i])));
    }
    return result;
}

String toLowerWords(const String& str) {
    String result = str;
    for (size_t i = 0; i < result.size(); ++i) {
        result[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(result[i])));
    }
    return result;
}

String cleanCharEnd(const String& v, char c) {
    if (!v.empty() && v[v.size() - 1] == c)
        return v.substr(0, v.size() - 1);
    return v;
}

String trimSpaces(const String& s) {
    const char* whitespace = " \t\r\n";
    size_t      start      = s.find_first_not_of(whitespace);
    if (start == String::npos)
        return "";
    size_t end = s.find_last_not_of(whitespace);
    return s.substr(start, end - start + 1);
}

String trimQuotes(const String& s) {
    if (s.size() >= 2 && ((s[0] == '"' && s[s.size() - 1] == '"') || (s[0] == '\'' && s[s.size() - 1] == '\'')))
        return s.substr(1, s.size() - 2);
    return s;
}

String trimSpacesComments(const String& s) {
    const char* whitespace = " \t\r\n";
    size_t      start      = s.find_first_not_of(whitespace);
    if (start == String::npos || s[start] == '#')
        return "";
    size_t end = s.find('#', start);
    if (end != String::npos)
        end--;
    else
        end = s.find_last_not_of(whitespace);
    return s.substr(start, end - start + 1);
}

String htmlEntities(const String& str) {
    String result;
    result.reserve(str.size()); // Optimization: reserve at least input size
    for (size_t i = 0; i < str.size(); ++i) {
        char c = str[i];
        switch (c) {
            case '&':
                result += "&amp;";
                break;
            case '<':
                result += "&lt;";
                break;
            case '>':
                result += "&gt;";
                break;
            case '"':
                result += "&quot;";
                break;
            case '\'':
                result += "&#39;";
                break;
            default:
                result += c;
        }
    }
    return result;
}

String generateGUID() {
    String guid;
    for (int i = 0; i < GUID_LENGTH; i++) {
        if (i == 8 || i == 12 || i == 16 || i == 20)
            guid += '-';
        int index = std::rand() % (sizeof(GUID_CHARSET) - 1);
        guid += GUID_CHARSET[index];
    }
    return guid;
}

bool splitByChar(const String& line, String& key, String& value, char endChar, bool reverse) {
    size_t pos = reverse ? line.rfind(endChar) : line.find(endChar);
    if (pos == String::npos)
        return false;
    key   = line.substr(0, pos);
    value = line.substr(pos + 1);
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

String findValueStrInMap(const MapString& map, const String& key) {
    MapString::const_iterator it = map.find(key);
    return (it != map.end()) ? it->second : "";
}

String findValueIntInMap(const MapIntString& map, int key) {
    MapIntString::const_iterator it = map.find(key);
    return (it != map.end()) ? it->second : "";
}

// ============================================================================
// File System Methods
// ============================================================================

bool convertFileToLines(const String& file, VectorString& lines) {
    std::ifstream ff(file.c_str());
    if (!ff.is_open()) {
        std::cerr << "Error: Could not open file " << file << std::endl;
        return false;
    }

    String line;
    String current;

    // Tokenizer logic specific to the Nginx-like config format
    while (std::getline(ff, line)) {
        for (size_t i = 0; i < line.size(); ++i) {
            char c = line[i];
            if (c == '{' || c == ';' || c == '}') {
                String t = trimSpacesComments(current);
                if (!t.empty())
                    lines.push_back(t + ((c == '{') ? " {" : (c == ';') ? ";" : ""));
                else if (c == '{' && !lines.empty())
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

struct stat getFileStat(const String& path) {
    struct stat st;
    String      actualPath = path;

    // Try path as is
    if (stat(actualPath.c_str(), &st) == 0)
        return st;

    // Remove leading ./ if present
    if (actualPath.size() > 2 && actualPath[0] == DOT && actualPath[1] == SLASH) {
        actualPath = actualPath.substr(2);
        if (stat(actualPath.c_str(), &st) == 0)
            return st;
    }

    // Handle leading slash or ./ prefixing
    if (!actualPath.empty() && actualPath[0] == SLASH) {
        // Try adding . at start
        String dotPrefix = DOT + actualPath;
        if (stat(dotPrefix.c_str(), &st) == 0)
            return st;

        // Try removing the slash
        String noSlash = actualPath.substr(1);
        if (stat(noSlash.c_str(), &st) == 0)
            return st;
    }

    st.st_mode = 0; // Mark invalid
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
    safe.reserve(filename.size());
    for (size_t i = 0; i < filename.size(); ++i) {
        char c = filename[i];
        if (std::isalnum(c) || c == '.' || c == '-' || c == '_')
            safe += c;
        else
            safe += '_';
    }
    return safe;
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
    if (dotPos == String::npos || dotPos == path.length() - 1)
        return "";
    return path.substr(dotPos);
}

String extractDirectoryFromPath(const String& path) {
    size_t lastSlash = path.rfind(SLASH);
    if (lastSlash == String::npos)
        return ".";
    if (lastSlash == 0)
        return "/";
    return path.substr(0, lastSlash);
}

// ============================================================================
// Path Methods
// ============================================================================

String normalizePath(const String& path) {
    if (path.empty())
        return "/";

    // 1. Remove duplicate slashes
    String cleaned;
    cleaned.reserve(path.size());
    if (path[0] != SLASH)
        cleaned += '/';

    for (size_t i = 0; i < path.length(); ++i) {
        if (path[i] == SLASH && i + 1 < path.length() && path[i + 1] == SLASH)
            continue;
        cleaned += path[i];
    }

    // 2. Resolve . and ..
    VectorString segments;
    size_t       start = 0;
    // Iterate through path, splitting by slash
    for (size_t i = 0; i <= cleaned.length(); ++i) {
        if (i == cleaned.length() || cleaned[i] == SLASH) {
            if (i > start) {
                String seg = cleaned.substr(start, i - start);
                if (seg == "..") {
                    if (!segments.empty())
                        segments.pop_back();
                } else if (seg != ".") {
                    segments.push_back(seg);
                }
            }
            start = i + 1;
        }
    }

    // 3. Rebuild
    if (segments.empty())
        return "/";

    String result = "/";
    for (size_t i = 0; i < segments.size(); ++i) {
        result += segments[i];
        if (i + 1 < segments.size())
            result += SLASH;
    }

    // Preserve trailing slash if originally present (and not root)
    if (result.size() > 1 && !cleaned.empty() && cleaned[cleaned.size() - 1] == SLASH)
        result += SLASH;

    return result;
}

String joinPaths(const String& firstPath, const String& secondPath) {
    if (firstPath.empty())
        return secondPath.empty() ? "/" : secondPath;
    if (secondPath.empty())
        return firstPath;

    bool firstEndsSlash    = (firstPath[firstPath.size() - 1] == SLASH);
    bool secondStartsSlash = (secondPath[0] == SLASH);

    if (firstEndsSlash && secondStartsSlash)
        return firstPath + secondPath.substr(1);
    if (!firstEndsSlash && !secondStartsSlash)
        return firstPath + SLASH + secondPath;

    return firstPath + secondPath;
}

bool pathStartsWith(const String& path, const String& prefix) {
    if (prefix.length() > path.length())
        return false;
    if (path.compare(0, prefix.length(), prefix) != 0)
        return false;

    // Exact match or prefix ends with slash or path continues with slash
    if (prefix == "/" || path.length() == prefix.length() || path[prefix.length()] == SLASH)
        return true;

    return false;
}

String getUriRemainder(const String& uri, const String& locPath) {
    String normalUri = normalizePath(uri);
    String normalLoc = normalizePath(locPath);

    if (normalLoc == "/")
        return normalUri;

    if (pathStartsWith(normalUri, normalLoc)) {
        String rest = normalUri.substr(normalLoc.length());
        if (rest.empty() || rest[0] != '/')
            return "/" + rest;
        return rest;
    }

    // Should typically not happen if logic is correct upstream
    return normalUri;
}

// ============================================================================
// HTTP/Network Helpers
// ============================================================================

bool checkAllowedMethods(const String& m) {
    // Optimization: Avoid constructing vector on every call
    static const char* allowed[] = {METHOD_GET, METHOD_POST, METHOD_DELETE, METHOD_PUT, METHOD_PATCH, METHOD_HEAD, METHOD_OPTIONS, NULL};

    for (int i = 0; allowed[i]; ++i) {
        if (m == allowed[i])
            return true;
    }
    return false;
}

bool setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return Logger::error("Failed to get fd flags");
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
        return Logger::error("Failed to set non-blocking mode");
    return true;
}

size_t convertMaxBodySize(const String& maxBody) {
    if (maxBody.empty())
        return 0;

    size_t size       = 0;
    char   unit       = maxBody[maxBody.size() - 1];
    String numberPart = std::isdigit(unit) ? maxBody : maxBody.substr(0, maxBody.size() - 1);

    std::stringstream ss(numberPart);
    ss >> size;
    if (ss.fail())
        return 0;

    switch (unit) {
        case 'K':
        case 'k':
            return size * 1024;
        case 'M':
        case 'm':
            return size * 1024 * 1024;
        case 'G':
        case 'g':
            return size * 1024 * 1024 * 1024;
        default:
            return size;
    }
}

String formatSize(double size) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int         unit    = 0;

    while (size >= 1024.0 && unit < 4) {
        size /= 1024.0;
        unit++;
    }

    size_t num    = static_cast<size_t>(size);
    String result = typeToString<size_t>(num);

    // Add up to 2 decimal places if needed and not B
    if (unit > 0) {
        size_t decimal = static_cast<size_t>((size - num) * 100);
        if (decimal > 0) {
            result += "."; // changed from ' to . for standard notation
            if (decimal < 10)
                result += "0";
            result += typeToString<size_t>(decimal);
        }
    }
    result += units[unit];
    return result;
}

// ============================================================================
// Header/Body Parsing
// ============================================================================

String extractFilenameFromHeader(const String& contentDisposition) {
    // Look for filename=
    size_t filenamePos = contentDisposition.find("filename=");
    if (filenamePos == String::npos)
        return "";

    size_t start = filenamePos + 9;
    if (start >= contentDisposition.length())
        return "";

    // Handle quoted: filename="My File.txt"
    if (contentDisposition[start] == '"') {
        start++;
        size_t end = contentDisposition.find('"', start);
        if (end == String::npos)
            return "";
        return contentDisposition.substr(start, end - start);
    }

    // Handle unquoted: filename=MyFile.txt;
    size_t end = contentDisposition.find(SEMICOLON, start);
    if (end == String::npos)
        end = contentDisposition.length();
    return trimSpaces(contentDisposition.substr(start, end - start));
}

String extractBoundaryFromContentType(const String& contentType) {
    size_t boundaryPos = contentType.find("boundary=");
    if (boundaryPos == String::npos)
        return "";

    size_t start = boundaryPos + 9;
    // Boundary might be end of string or followed by semicolon
    size_t end = contentType.find(SEMICOLON, start);
    if (end == String::npos)
        end = contentType.length();

    return trimSpaces(contentType.substr(start, end - start));
}

bool parseMultipartFormData(const String& body, const String& boundary, String& filename, String& fileContent) {
    if (boundary.empty() || body.empty())
        return false;

    String startBoundary = "--" + boundary;
    // String endBoundary   = "--" + boundary + "--"; // Not explicitly needed for first part

    // 1. Find Start
    size_t partStart = body.find(startBoundary);
    if (partStart == String::npos)
        return false;
    partStart += startBoundary.length();

    // Consume CRLF if present
    if (partStart + 2 <= body.length() && body.substr(partStart, 2) == CRLF)
        partStart += 2;

    // 2. Find End of this part (next boundary)
    size_t partEnd = body.find(startBoundary, partStart);
    if (partEnd == String::npos)
        return false;

    // Extract the raw part
    String part = body.substr(partStart, partEnd - partStart);

    // 3. Separate Headers and Content
    size_t headerEnd = part.find(DOUBLE_CRLF);
    if (headerEnd == String::npos)
        return false;

    String headers = part.substr(0, headerEnd);
    String content = part.substr(headerEnd + 4); // Skip \r\n\r\n

    // 4. Remove trailing CRLF from content (part of multipart protocol)
    if (content.length() >= 2 && content.substr(content.length() - 2) == CRLF)
        content.resize(content.length() - 2);

    // 5. Parse Headers for filename
    filename.clear();
    VectorString lines;
    splitByString(headers, lines, CRLF);

    for (size_t i = 0; i < lines.size(); ++i) {
        if (toLowerWords(lines[i]).find("content-disposition") != String::npos) {
            filename = extractFilenameFromHeader(lines[i]);
            break;
        }
    }

    fileContent = content;
    return !filename.empty();
}

bool getHeaderValue(const String& headers, const String& headerName, String& outValue) {
    outValue.clear();
    if (headerName.empty() || headers.empty())
        return false;

    String search = toLowerWords(headerName);
    if (search.find(':') == String::npos)
        search += ':';

    String lowerHeaders = toLowerWords(headers);
    size_t pos          = lowerHeaders.find(search);
    if (pos == String::npos)
        return false;

    size_t valueStart = pos + search.size();
    size_t endLine    = lowerHeaders.find("\r\n", valueStart);
    if (endLine == String::npos)
        endLine = lowerHeaders.size(); // handle last line without CRLF

    // Extract from original headers to preserve case of value
    outValue = trimSpaces(headers.substr(valueStart, endLine - valueStart));
    return !outValue.empty();
}

bool isChunkedTransferEncoding(const String& headers) {
    String val;
    if (!getHeaderValue(headers, "transfer-encoding", val))
        return false;
    return toLowerWords(val).find("chunked") != String::npos;
}

bool decodeChunkedBody(const String& chunkedBody, String& decodedBody) {
    decodedBody.clear();
    size_t pos      = 0;
    size_t totalLen = chunkedBody.size();

    while (pos < totalLen) {
        // 1. Find line end for chunk size
        size_t lineEnd = chunkedBody.find("\r\n", pos);
        if (lineEnd == String::npos)
            return false;

        String sizeLine = chunkedBody.substr(pos, lineEnd - pos);

        // Remove extensions (e.g., 4A; extension=value)
        size_t semiPos = sizeLine.find(';');
        if (semiPos != String::npos)
            sizeLine = sizeLine.substr(0, semiPos);

        sizeLine = trimSpaces(sizeLine);
        if (sizeLine.empty())
            return false;

        // 2. Parse Hex Size
        unsigned long chunkSize = 0;
        for (size_t i = 0; i < sizeLine.size(); i++) {
            char c = sizeLine[i];
            chunkSize *= 16;
            if (c >= '0' && c <= '9')
                chunkSize += c - '0';
            else if (c >= 'a' && c <= 'f')
                chunkSize += c - 'a' + 10;
            else if (c >= 'A' && c <= 'F')
                chunkSize += c - 'A' + 10;
            else
                return false;
        }

        pos = lineEnd + 2; // skip \r\n

        // 3. Last Chunk (0)
        if (chunkSize == 0)
            return true;

        // 4. Validate limits
        if (pos + chunkSize > totalLen)
            return false; // incomplete data

        // 5. Append Data
        decodedBody.append(chunkedBody, pos, chunkSize);
        pos += chunkSize;

        // 6. Check trailing CRLF
        if (pos + 2 > totalLen || chunkedBody.substr(pos, 2) != "\r\n")
            return false;
        pos += 2;
    }

    // Should end with 0 chunk, so if we exit loop, it might be incomplete
    return false;
}

size_t extractContentLength(const String& headers) {
    String val;
    if (!getHeaderValue(headers, "content-length", val))
        return 0;
    return stringToType<size_t>(val);
}