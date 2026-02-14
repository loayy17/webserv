#include "LocationConfig.hpp"

LocationConfig::LocationConfig()
    : path(""),
      root(""),
      autoIndex(false),
      autoIndexSet(false),
      indexes(),
      uploadDir(""),
      cgiPass(),
      clientMaxBody(""),
      allowedMethods(),
      errorPage(),
      hasRedirect(false),
      redirectCode(0),
      redirectValue("") {}

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

LocationConfig::LocationConfig(const String& p)
    : path(p),
      root(""),
      autoIndex(false),
      autoIndexSet(false),
      indexes(),
      uploadDir(""),
      cgiPass(),
      clientMaxBody(""),
      allowedMethods(),
      errorPage(),
      hasRedirect(false),
      redirectCode(0),
      redirectValue("") {}

LocationConfig::~LocationConfig() {
    indexes.clear();
    allowedMethods.clear();
    cgiPass.clear();
    errorPage.clear();
}
// setters
bool LocationConfig::setRoot(const VectorString& r) {
    if (!root.empty())
        return Logger::error("duplicate root");
    if (r.size() != 1)
        return Logger::error("root takes exactly one value");
    root = r[0];
    if (root[root.length() - 1] == '/')
        root.erase(root.length() - 1);
    return true;
}

void LocationConfig::setRoot(const String& r) {
    root = r;
}

bool LocationConfig::setAutoIndex(const VectorString& v) {
    if (autoIndexSet)
        return Logger::error("duplicate autoindex directive");
    if (v.size() != 1)
        return Logger::error("autoindex takes exactly one value");
    if (v[0] != "on" && v[0] != "off")
        return Logger::error("invalid autoindex value");
    autoIndex    = (v[0] == "on");
    autoIndexSet = true;
    return true;
}

void LocationConfig::setAutoIndex(bool v) {
    autoIndex = v;
}
bool LocationConfig::setIndexes(const VectorString& i) {
    if (!indexes.empty())
        return Logger::error("duplicate index");
    if (i.empty())
        return true;
    indexes = i;
    return true;
}
void LocationConfig::setUploadDir(const String& p) {
    uploadDir = p;
}

bool LocationConfig::setUploadDir(const VectorString& p) {
    if (!uploadDir.empty())
        return Logger::error("duplicate upload_dir directive");
    if (p.size() != 1)
        return Logger::error("upload_dir takes exactly one value");
    uploadDir = p[0];
    return true;
}

bool LocationConfig::setCgiPass(const VectorString& c) {
    if (c.size() != 2)
        return Logger::error("cgi_pass requires extension and interpreter");

    String extension   = trimSpaces(c[0]);
    String interpreter = trimSpaces(c[1]);

    if (extension.empty() || extension[0] != '.')
        return Logger::error("cgi_pass extension must start with '.'");

    if (interpreter.empty() || interpreter[0] != '/')
        return Logger::error("cgi_pass interpreter must be an absolute path");

    if (findValueStrInMap(cgiPass, extension) != "")
        return Logger::error("duplicate cgi_pass for extension: " + extension);

    cgiPass[extension] = interpreter;
    return true;
}

bool LocationConfig::setRedirect(const VectorString& r) {
    if (hasRedirect)
        return Logger::error("duplicate return directive");
    if (r.size() != 1 && r.size() != 2)
        return Logger::error("return takes exactly one value");
    redirectCode = stringToType<int>(r[0]);
    if (redirectCode <= 0 || redirectCode > 1000)
        return Logger::error("return code must be between 0 and 1000");
    if (r.size() == 2) 
        redirectValue = r[1];
    hasRedirect = true;
    return true;
}

bool LocationConfig::setClientMaxBody(const VectorString& c) {
    if (!clientMaxBody.empty())
        return Logger::error("duplicate client_max_body_size");
    if (c.size() != 1)
        return Logger::error("client_max_body_size takes exactly one value");
    clientMaxBody = c[0];
    return true;
}
void LocationConfig::setClientMaxBody(const String& c) {
    clientMaxBody = c;
}

void LocationConfig::addAllowedMethod(const String& m) {
    allowedMethods.push_back(m);
}
bool LocationConfig::setAllowedMethods(const VectorString& v) {
    if (!allowedMethods.empty())
        return Logger::error("duplicate methods directive");
    if (v.empty())
        return Logger::error("methods requires at least one value");
    for (size_t i = 0; i < v.size(); i++) {
        String m = toUpperWords(v[i]);
        if (!checkAllowedMethods(m))
            return Logger::error("invalid method: " + m);
        for (size_t j = 0; j < allowedMethods.size(); j++) {
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
        const String& codeStr = values[i];
        int           code    = stringToType<int>(codeStr);
        if (codeStr != typeToString(code)) {
            return Logger::error("Invalid error code: " + codeStr);
        }
        if (code < 100 || code > 599) {
            return Logger::error("Error code must be between 100 and 599: " + codeStr);
        }
        errorPage[code] = pagePath;
    }

    return true;
}

// getters
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
    return findValueStrInMap(cgiPass, extension);
}
bool LocationConfig::hasCgi() const {
    return !cgiPass.empty();
}

VectorString LocationConfig::getAllowedMethods() const {
    return allowedMethods;
}
String LocationConfig::getClientMaxBody() const {
    return clientMaxBody;
}
VectorString LocationConfig::getIndexes() const {
    return indexes;
}
String LocationConfig::getErrorPage(int code) const {
    return findValueIntInMap(errorPage, code);
}

bool LocationConfig::getIsRedirect() const
{
    return hasRedirect;
}
int LocationConfig::getRedirectCode() const
{
    return redirectCode;
}
String LocationConfig::getRedirectValue() const
{
    return redirectValue;
}