#include "LocationConfig.hpp"

// -----------------------------------------------------------------------------
// Constructors / Destructor
// -----------------------------------------------------------------------------
LocationConfig::LocationConfig()
    : path(),
      root(),
      autoIndex(false),
      autoIndexSet(false),
      indexes(),
      uploadDir(),
      cgiPass(),
      clientMaxBody(-1),
      allowedMethods(),
      errorPage(),
      hasRedirect(false),
      redirectCode(0),
      redirectValue() {}

LocationConfig::LocationConfig(const LocationConfig& other)
    : path(other.path),
      root(other.root),
      autoIndex(other.autoIndex),
      autoIndexSet(other.autoIndexSet),
      indexes(other.indexes),
      uploadDir(other.uploadDir),
      cgiPass(other.cgiPass),
      clientMaxBody(other.clientMaxBody),
      allowedMethods(other.allowedMethods),
      errorPage(other.errorPage),
      hasRedirect(other.hasRedirect),
      redirectCode(other.redirectCode),
      redirectValue(other.redirectValue) {}

LocationConfig::LocationConfig(const String& p)
    : path(p),
      root(),
      autoIndex(false),
      autoIndexSet(false),
      indexes(),
      uploadDir(),
      cgiPass(),
      clientMaxBody(-1),
      allowedMethods(),
      errorPage(),
      hasRedirect(false),
      redirectCode(0),
      redirectValue() {}

LocationConfig& LocationConfig::operator=(const LocationConfig& other) {
    if (this != &other) {
        path           = other.path;
        root           = other.root;
        autoIndex      = other.autoIndex;
        autoIndexSet   = other.autoIndexSet;
        indexes        = other.indexes;
        uploadDir      = other.uploadDir;
        cgiPass        = other.cgiPass;
        clientMaxBody  = other.clientMaxBody;
        allowedMethods = other.allowedMethods;
        errorPage      = other.errorPage;
        hasRedirect    = other.hasRedirect;
        redirectCode   = other.redirectCode;
        redirectValue  = other.redirectValue;
    }
    return *this;
}

LocationConfig::~LocationConfig() {}

// -----------------------------------------------------------------------------
// Setters
// -----------------------------------------------------------------------------
bool LocationConfig::setRoot(const VectorString& r) {
    if (!root.empty())
        return Logger::error("duplicate root");
    if (!requireSingleValue(r, "root"))
        return false;
    String newRoot = r[0];
    if (newRoot.size() > 1 && newRoot[newRoot.size() - 1] == SLASH)
        newRoot.erase(newRoot.size() - 1);
    root = newRoot;
    return true;
}

void LocationConfig::setRoot(const String& r) {
    root = r;
}

bool LocationConfig::setAutoIndex(const VectorString& v) {
    if (autoIndexSet)
        return Logger::error("duplicate autoindex directive");
    if (!requireSingleValue(v, "autoindex"))
        return false;
    if (v[0] != "on" && v[0] != "off")
        return Logger::error("invalid autoindex value (must be 'on' or 'off')");
    autoIndex    = (v[0] == "on");
    autoIndexSet = true;
    return true;
}

void LocationConfig::setAutoIndex(bool v) {
    autoIndex    = v;
    autoIndexSet = true;
}

bool LocationConfig::setIndexes(const VectorString& i) {
    if (!indexes.empty())
        return Logger::error("duplicate index");
    if (!i.empty())
        indexes = i;
    return true;
}

bool LocationConfig::setUploadDir(const VectorString& p) {
    if (!uploadDir.empty())
        return Logger::error("duplicate upload_dir directive");
    if (!requireSingleValue(p, "upload_dir"))
        return false;
    uploadDir = p[0];
    return true;
}

void LocationConfig::setUploadDir(const String& p) {
    uploadDir = p;
}

bool LocationConfig::setCgiPass(const VectorString& c) {
    if (c.size() != 2)
        return Logger::error("cgi_pass requires extension and interpreter");

    String extension   = trimSpaces(c[0]);
    String interpreter = trimSpaces(c[1]);

    if (extension.empty() || extension[0] != DOT)
        return Logger::error("cgi_pass extension must start with '.'");
    if (interpreter.empty() || interpreter[0] != SLASH)
        return Logger::error("cgi_pass interpreter must be an absolute path");
    if (cgiPass.find(extension) != cgiPass.end())
        return Logger::error("duplicate cgi_pass for extension: " + extension);

    cgiPass[extension] = interpreter;
    return true;
}

bool LocationConfig::setRedirect(const VectorString& r) {
    if (hasRedirect)
        return Logger::error("duplicate return directive");
    if (r.empty())
        return Logger::error("return requires at least a status code");

    if (!stringToType<int>(r[0], redirectCode) || redirectCode < 1 || redirectCode > 999)
        return Logger::error("invalid HTTP status code for redirect: " + r[0]);
    redirectValue.clear();
    for (size_t i = 1; i < r.size(); ++i) {
        if (i > 1)
            redirectValue += " ";
        redirectValue += r[i];
    }
    hasRedirect = true;
    return true;
}

bool LocationConfig::setClientMaxBody(const VectorString& c) {
    if (clientMaxBody != -1)
        return Logger::error("Duplicate client_max_body_size");
    if (!requireSingleValue(c, "client_max_body_size"))
        return false;
    if (convertMaxBodySize(c[0]) == 0 && c[0] != "0")
        return Logger::error("invalid client_max_body_size value: " + c[0]);
    clientMaxBody = convertMaxBodySize(c[0]);
    return true;
}

void LocationConfig::setClientMaxBody(ssize_t c) {
    clientMaxBody = c;
}

bool LocationConfig::setAllowedMethods(const VectorString& v) {
    if (!allowedMethods.empty())
        return Logger::error("duplicate methods directive");
    if (v.empty())
        return Logger::error("methods requires at least one value");

    for (size_t i = 0; i < v.size(); ++i) {
        String m = toUpperWords(v[i]);
        if (!checkAllowedMethods(m))
            return Logger::error("invalid method: " + m);
        for (size_t j = 0; j < allowedMethods.size(); ++j) {
            if (allowedMethods[j] == m)
                return Logger::error("duplicate method: " + m);
        }
        allowedMethods.push_back(m);
    }
    return true;
}

bool LocationConfig::setErrorPage(const VectorString& values) {
    if (values.size() < 2)
        return Logger::error("error_page requires at least an error code and a page path");
    const String& pagePath = values.back();
    for (size_t i = 0; i < values.size() - 1; ++i) {
        int code;
        if (!stringToType<int>(values[i], code) || code < 100 || code > 599) {
            return Logger::error("invalid error code (must be 100-599): " + values[i]);
        }
        errorPage[code] = pagePath;
    }
    return true;
}

String LocationConfig::getPath() const {
    return path;
}

String LocationConfig::getRoot() const {
    return root;
}

bool LocationConfig::getAutoIndex() const {
    return autoIndex;
}

String LocationConfig::getUploadDir() const {
    return uploadDir;
}

const std::map<String, String>& LocationConfig::getCgiPass() const {
    return cgiPass;
}

String LocationConfig::getCgiInterpreter(const String& extension) const {
    std::map<String, String>::const_iterator it = cgiPass.find(extension);
    return (it != cgiPass.end()) ? it->second : "";
}

bool LocationConfig::hasCgi() const {
    return !cgiPass.empty();
}

VectorString LocationConfig::getAllowedMethods() const {
    return allowedMethods;
}

ssize_t LocationConfig::getClientMaxBody() const {
    return clientMaxBody;
}

VectorString LocationConfig::getIndexes() const {
    return indexes;
}

String LocationConfig::getErrorPage(int code) const {
    std::map<int, String>::const_iterator it = errorPage.find(code);
    return (it != errorPage.end()) ? it->second : "";
}

bool LocationConfig::getIsRedirect() const {
    return hasRedirect;
}

int LocationConfig::getRedirectCode() const {
    return redirectCode;
}

String LocationConfig::getRedirectValue() const {
    return redirectValue;
}