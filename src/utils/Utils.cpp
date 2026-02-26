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

bool isLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

String formatDateTime(time_t t) {
    static const int   MONTH_DAYS[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    static const char* WEEKDAYS[]   = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    static const char* MONTHS[]     = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    long daysSinceEpoch = t / SECONDS_PER_DAY;
    long secsToday      = t % SECONDS_PER_DAY;
    int  weekday        = (4 + daysSinceEpoch) % 7;
    if (weekday < 0)
        weekday += 7;

    // Year calculation
    int  year          = 1970;
    long remainingDays = daysSinceEpoch;
    while (true) {
        int daysInYear = isLeapYear(year) ? 366 : 365;
        if (remainingDays >= daysInYear) {
            remainingDays -= daysInYear;
            ++year;
        } else {
            break;
        }
    }

    // Month and day calculation

    int month = 0;
    while (month < 12) {
        int daysInMonth = MONTH_DAYS[month];
        // Adjust February for leap year
        if (month == 1 && isLeapYear(year))
            daysInMonth = 29;
        if (remainingDays >= daysInMonth) {
            remainingDays -= daysInMonth;
            ++month;
        } else
            break;
    }
    int day = static_cast<int>(remainingDays) + 1;

    // Time of day
    int hour   = static_cast<int>(secsToday / SECONDS_PER_HOUR);
    int minute = static_cast<int>((secsToday % SECONDS_PER_HOUR) / SECONDS_PER_MIN);
    int second = static_cast<int>(secsToday % SECONDS_PER_MIN);

    String hourStr = hour < 10 ? "0" + typeToString(hour) : typeToString(hour);
    String minStr  = minute < 10 ? "0" + typeToString(minute) : typeToString(minute);
    String secStr  = second < 10 ? "0" + typeToString(second) : typeToString(second);
    String dayStr  = day < 10 ? "0" + typeToString(day) : typeToString(day);
    // Build the HTTP date string
    String date = WEEKDAYS[weekday];
    date += ", " + dayStr;
    date += " " + String(MONTHS[month]);
    date += " " + typeToString(year);
    date += " " + hourStr + ":" + minStr + ":" + secStr;
    date += " GMT";
    return date;
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
    result.reserve(str.size());
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
    static const char hex[] = "0123456789abcdef";
    String            id;
    id.reserve(SESSION_ID_LENGTH);

    const int     numBytes = SESSION_ID_LENGTH / 2;
    unsigned char bytes[SESSION_ID_LENGTH / 2];

    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) {
        Logger::error("CRITICAL: /dev/urandom unavailable");
        throw std::runtime_error("Secure random source unavailable");
    }

    ssize_t n = read(fd, bytes, numBytes);
    close(fd);
    if (n != numBytes) {
        throw std::runtime_error("Failed to read from /dev/urandom");
    }

    for (int i = 0; i < numBytes; ++i) {
        id += hex[(bytes[i] >> 4) & 0x0f];
        id += hex[bytes[i] & 0x0f];
    }
    return id;
}

bool splitByChar(const String& line, String& key, String& value, char endChar, bool reverse) {
    size_t pos = reverse ? line.rfind(endChar) : line.find(endChar);
    if (pos == String::npos || pos >= line.size()) {
        return false;
    }
    String left;
    String right;
    left = line.substr(0, pos);
    if (pos + 1 <= line.size())
        right = line.substr(pos + 1);
    else
        right = "";

    key   = left;
    value = right;
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
    if (!ff.is_open())
        return Logger::error("Failed to open file: " + file);

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
    if (stat(path.c_str(), &st) == 0)
        return st;

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
    if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
        return Logger::error("Failed to set non-blocking mode");
    return true;
}

size_t convertMaxBodySize(const String& clientMaxBodySize) {
    if (clientMaxBodySize.empty())
        return 0;

    size_t            size       = 0;
    char              unit       = clientMaxBodySize[clientMaxBodySize.size() - 1];
    String            numberPart = (unit >= '9' || unit <= '0') ? clientMaxBodySize : clientMaxBodySize.substr(0, clientMaxBodySize.size() - 1);
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
    static const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int                unit    = 0;

    while (size >= 1024.0 && unit < 4) {
        size /= 1024.0;
        unit++;
    }

    size_t num    = static_cast<size_t>(size);
    String result = typeToString<size_t>(num);
    if (unit > 0) {
        size_t decimal = static_cast<size_t>((size - num) * 100);
        if (decimal > 0) {
            result += ".";
            if (decimal < 10)
                result += "0";
            result += typeToString<size_t>(decimal);
        }
    }
    result += units[unit];
    return result;
}

String getHttpStatusMessage(int code) {
    switch (code) {
        case 100:
            return "Continue";
        case 101:
            return "Switching Protocols";
        case 200:
            return "OK";
        case 201:
            return "Created";
        case 202:
            return "Accepted";
        case 203:
            return "Non-Authoritative Information";
        case 204:
            return "No Content";
        case 205:
            return "Reset Content";
        case 206:
            return "Partial Content";
        case 300:
            return "Multiple Choices";
        case 301:
            return "Moved Permanently";
        case 302:
            return "Found";
        case 303:
            return "See Other";
        case 304:
            return "Not Modified";
        case 305:
            return "Use Proxy";
        case 307:
            return "Temporary Redirect";
        case 400:
            return "Bad Request";
        case 401:
            return "Unauthorized";
        case 402:
            return "Payment Required";
        case 403:
            return "Forbidden";
        case 404:
            return "Not Found";
        case 405:
            return "Method Not Allowed";
        case 406:
            return "Not Acceptable";
        case 407:
            return "Proxy Authentication Required";
        case 408:
            return "Request Timeout";
        case 409:
            return "Conflict";
        case 410:
            return "Gone";
        case 411:
            return "Length Required";
        case 412:
            return "Precondition Failed";
        case 413:
            return "Payload Too Large";
        case 414:
            return "URI Too Long";
        case 415:
            return "Unsupported Media Type";
        case 416:
            return "Range Not Satisfiable";
        case 417:
            return "Expectation Failed";
        case 500:
            return "Internal Server Error";
        case 501:
            return "Not Implemented";
        case 502:
            return "Bad Gateway";
        case 503:
            return "Service Unavailable";
        case 504:
            return "Gateway Timeout";
        case 505:
            return "HTTP Version Not Supported";
        default:
            return "Unknown Error";
    }
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
        size_t lineEnd = chunkedBody.find("\r\n", pos);
        if (lineEnd == String::npos)
            return false;

        String sizeLine = chunkedBody.substr(pos, lineEnd - pos);
        size_t semiPos  = sizeLine.find(';');
        if (semiPos != String::npos)
            sizeLine = sizeLine.substr(0, semiPos);

        sizeLine = trimSpaces(sizeLine);
        if (sizeLine.empty())
            return false;

        // 2. Parse hex chunk size manually
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
                return false; // invalid hex digit
        }

        pos = lineEnd + 2; // skip \r\n

        // 3. Handle last chunk (0)
        if (chunkSize == 0) {
            // Expect a final CRLF (or optional trailer headers + CRLF)
            if (pos + 2 <= totalLen && chunkedBody.substr(pos, 2) == "\r\n")
                pos += 2;
            return pos == totalLen; // ensure all input consumed
        }

        // 4. Validate chunk size fits remaining data
        if (pos + chunkSize + 2 > totalLen)
            return false;

        // 5. Append chunk data
        decodedBody.append(chunkedBody, pos, chunkSize);
        pos += chunkSize;

        // 6. Check trailing CRLF after chunk
        if (chunkedBody.substr(pos, 2) != "\r\n")
            return false;
        pos += 2;
    }

    return false; // should never reach here without a zero chunk
}

bool extractContentLength(ssize_t& contentLength, const String& headers) {
    String val;
    if (!getHeaderValue(headers, "content-length", val) || val.empty() || !stringToType<ssize_t>(val, contentLength))
        return false;
    return true;
}

String urlDecode(const String& input) {
    String result;
    result.reserve(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '%' && i + 2 < input.size() && std::isxdigit(static_cast<unsigned char>(input[i + 1])) &&
            std::isxdigit(static_cast<unsigned char>(input[i + 2]))) {
            int hi = std::isdigit(static_cast<unsigned char>(input[i + 1])) ? input[i + 1] - '0'
                                                                            : std::toupper(static_cast<unsigned char>(input[i + 1])) - 'A' + 10;
            int lo = std::isdigit(static_cast<unsigned char>(input[i + 2])) ? input[i + 2] - '0'
                                                                            : std::toupper(static_cast<unsigned char>(input[i + 2])) - 'A' + 10;
            result += static_cast<char>((hi << 4) | lo);
            i += 2;
        } else {
            result += input[i];
        }
    }
    return result;
}

bool requireSingleValue(const VectorString& v, const String& directive) {
    if (v.size() != 1)
        return Logger::error(directive + " takes exactly one value");
    return true;
}
