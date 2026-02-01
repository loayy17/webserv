#include "LocationConfig.hpp"

LocationConfig::LocationConfig()
    : path(""),
      root(""),
      autoIndex(false),
      autoIndexSet(false),
      indexes(),
      uploadDir(""),
      cgiPass(),
      redirect(""),
      clientMaxBody(""),
      allowedMethods() {}

LocationConfig::LocationConfig(const LocationConfig& other)
    : path(other.path),
      root(other.root),
      autoIndex(other.autoIndex),
      autoIndexSet(other.autoIndexSet),
      indexes(other.indexes),
      uploadDir(other.uploadDir),
      cgiPass(other.cgiPass),
      redirect(other.redirect),
      clientMaxBody(other.clientMaxBody),
      allowedMethods(other.allowedMethods) {}

LocationConfig& LocationConfig::operator=(const LocationConfig& other) {
    if (this != &other) {
        path           = other.path;
        root           = other.root;
        autoIndex      = other.autoIndex;
        autoIndexSet   = other.autoIndexSet;
        indexes        = other.indexes;
        uploadDir      = other.uploadDir;
        cgiPass        = other.cgiPass;
        redirect       = other.redirect;
        clientMaxBody  = other.clientMaxBody;
        allowedMethods = other.allowedMethods;
    }
    return *this;
}

LocationConfig::LocationConfig(const std::string& p)
    : path(p),
      root(""),
      autoIndex(false),
      autoIndexSet(false),
      indexes(),
      uploadDir(""),
      cgiPass(),
      redirect(""),
      clientMaxBody(""),
      allowedMethods() {}

LocationConfig::~LocationConfig() {
    indexes.clear();
    allowedMethods.clear();
    cgiPass.clear();
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

void LocationConfig::setRoot(const std::string& r) {
    root = r;
}

bool LocationConfig::setAutoIndex(const VectorString& v) {
    if (autoIndexSet)
        return Logger::error("duplicate autoindex directive");
    if (v.size() != 1)
        return Logger::error("autoindex takes exactly one value");
    if (v[0] != "on" && v[0] != "off")
        return Logger::error("invalid autoindex value");
    autoIndex = (v[0] == "on");
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
        return Logger::error("index takes at least one value");
    indexes = i;
    return true;
}
void LocationConfig::setUploadDir(const std::string& p) {
    uploadDir = p;
}

bool LocationConfig::setUploadDir(const VectorString& p) {
    if (!uploadDir.empty())
        return Logger::error("duplicate upload_dir directive");
    if (p.size() != 1)
        return Logger::error("upload_dir takes exactly one value");
    if (p[0].empty() || p[0][0] != '/')
        return Logger::error("upload_dir must be an absolute path");
    uploadDir = p[0];
    return true;
}

bool LocationConfig::setCgiPass(const VectorString& c) {
    if (c.size() != 1)
        return Logger::error("cgi_pass takes exactly one value");

    const std::string& value = c[0];
    size_t colonPos = value.find(':');
    if (colonPos == std::string::npos)
        return Logger::error("cgi_pass format must be .extension:/path/to/interpreter");

    std::string extension = value.substr(0, colonPos);
    std::string interpreter = value.substr(colonPos + 1);

    if (extension.empty() || extension[0] != '.')
        return Logger::error("cgi_pass extension must start with '.'");
    if (interpreter.empty() || interpreter[0] != '/')
        return Logger::error("cgi_pass interpreter must be an absolute path");

    if (cgiPass.find(extension) != cgiPass.end())
        return Logger::error("duplicate cgi_pass for extension: " + extension);

    cgiPass[extension] = interpreter;
    return true;
}

void LocationConfig::setRedirect(const std::string& r) {
    redirect = r;
}

bool LocationConfig::setRedirect(const VectorString& r) {
    if (!redirect.empty())
        return Logger::error("duplicate return directive");
    if (r.size() != 1)
        return Logger::error("return takes exactly one value");
    if (r[0].empty() || r[0][0] != '/')
        return Logger::error("return url must start with /");
    redirect = r[0];
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
void LocationConfig::setClientMaxBody(const std::string& c) {
    clientMaxBody = c;
}

void LocationConfig::addAllowedMethod(const std::string& m) {
    allowedMethods.push_back(m);
}
bool LocationConfig::setAllowedMethods(const VectorString& v) {
    if (!allowedMethods.empty())
        return Logger::error("duplicate methods directive");
    if (v.empty())
        return Logger::error("methods requires at least one value");
    for (size_t i = 0; i < v.size(); i++) {
        std::string m = toUpperWords(v[i]);
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

// getters
std::string LocationConfig::getPath() const {
    return path;
}
std::string LocationConfig::getRoot() const {
    return root;
}
bool LocationConfig::getAutoIndex() const {
    return autoIndex;
}
std::string LocationConfig::getUploadDir() const {
    return uploadDir;
}
const std::map<std::string, std::string>& LocationConfig::getCgiPass() const {
    return cgiPass;
}
std::string LocationConfig::getCgiInterpreter(const std::string& extension) const {
    std::map<std::string, std::string>::const_iterator it = cgiPass.find(extension);
    return (it != cgiPass.end()) ? it->second : "";
}
bool LocationConfig::hasCgi() const {
    return !cgiPass.empty();
}
std::string LocationConfig::getRedirect() const {
    return redirect;
}
VectorString LocationConfig::getAllowedMethods() const {
    return allowedMethods;
}
std::string LocationConfig::getClientMaxBody() const {
    return clientMaxBody;
}
VectorString LocationConfig::getIndexes() const {
    return indexes;
}