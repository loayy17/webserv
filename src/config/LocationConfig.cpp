#include "LocationConfig.hpp"

LocationConfig::LocationConfig(const std::string &p)
    : path(p),
      root(""),
      autoIndex(false),
      indexes(),
      uploadEnabled(false),
      uploadPath(""),
      cgiEnabled(false),
      cgiPath(""),
      cgiExtension(""),
      redirect(""),
      allowedMethods() {}

LocationConfig::~LocationConfig()
{
    allowedMethods.clear();
}
// setters
bool LocationConfig::setRoot(const std::vector<std::string> &r)
{
    if (!root.empty())
        return Logger::error("duplicate root");
    if (r.size() != 1)
        return Logger::error("root takes exactly one value");
    root = r[0];
    if (root[root.length() - 1] == '/')
        root.erase(root.length() - 1);
    return true;
}

void LocationConfig::setRoot(const std::string &r)
{
    root = r;
}

bool LocationConfig::setAutoIndex(const std::vector<std::string> &v)
{
    if (autoIndex != false)
        return Logger::error("duplicate autoindex");
    if (v.size() != 1)
        return Logger::error("autoindex takes exactly one value");
    if (v[0] != "on" && v[0] != "off")
        return Logger::error("invalid autoindex value");
    autoIndex = (v[0] == "on");
    return true;
}

void LocationConfig::setAutoIndex(bool v)
{
    autoIndex = v;
}
bool LocationConfig::setIndexes(const std::vector<std::string> &i)
{
    if (!indexes.empty())
        return Logger::error("duplicate index");
    if (i.empty())
        return Logger::error("index takes at least one value");
    indexes = i;
    return true;
}
void LocationConfig::setUploadEnabled(bool v)
{
    uploadEnabled = v;
}
void LocationConfig::setUploadPath(const std::string &p)
{
    uploadPath = p;
}

bool LocationConfig::setUploadPath(const std::vector<std::string> &p)
{
    if (!uploadPath.empty())
        return Logger::error("duplicate upload_path");
    if (p.size() != 1)
        return Logger::error("upload_path takes exactly one value");
    if (p[0].empty() || p[0][0] != '/')
        return Logger::error("upload_path must be an absolute path");
    uploadPath = p[0];
    return true;
}
void LocationConfig::setCgiEnabled(bool v)
{
    cgiEnabled = v;
}
void LocationConfig::setCgiPath(const std::string &p)
{
    cgiPath = p;
}

bool LocationConfig::setCgiPath(const std::vector<std::string> &p)
{
    if (!cgiPath.empty())
        return Logger::error("duplicate cgi_path");
    if (p.size() != 1)
        return Logger::error("cgi_path takes exactly one value");
    if (p[0].empty() || p[0][0] != '/')
        return Logger::error("cgi_path must be an absolute path");
    cgiPath = p[0];
    return true;
}
void LocationConfig::setCgiExtension(const std::string &e)
{
    cgiExtension = e;
}

bool LocationConfig::setCgiExtension(const std::vector<std::string> &e)
{
    if (!cgiExtension.empty())
        return Logger::error("duplicate cgi_extension");
    if (e.size() != 1)
        return Logger::error("cgi_extension takes exactly one value");
    if (e[0].empty() || e[0][0] != '.')
        return Logger::error("cgi_extension must start with '.'");
    cgiExtension = e[0];
    return true;
}
void LocationConfig::setRedirect(const std::string &r)
{
    redirect = r;
}

bool LocationConfig::setRedirect(const std::vector<std::string> &r)
{
    if (!redirect.empty())
        return Logger::error("duplicate return");
    if (r.empty() || r.size() > 2)
        return Logger::error("return takes 1 or 2 values: [status_code] url");

    std::string code;
    std::string url;

    if (r.size() == 2)
    {
        code = r[0];
        url = r[1];

        if (code != "301" && code != "302" && code != "303" && code != "307" && code != "308")
            return Logger::error("invalid redirect status code: " + code);
    }
    else
    {
        code = "301";
        url = r[0];
    }

    if (url.empty() || url[0] != '/')
        return Logger::error("redirect url must start with /");

    redirect = code + " " + url;
    return true;
}

bool LocationConfig::setClientMaxBody(const std::vector<std::string> &c)
{
    if (!clientMaxBody.empty())
        return Logger::error("duplicate client_max_body_size");
    if (c.size() != 1)
        return Logger::error("client_max_body_size takes exactly one value");
    clientMaxBody = c[0];
    return true;
}
void LocationConfig::setClientMaxBody(const std::string &c)
{
    clientMaxBody = c;
}

void LocationConfig::addAllowedMethod(const std::string &m)
{
    allowedMethods.push_back(m);
}
bool LocationConfig::setAllowedMethods(const std::vector<std::string> &v)
{
    for (size_t i = 0; i < v.size(); i++)
    {
        std::string m = toUpperWords(v[i]);
        if (!checkAllowedMethods(m))
            return Logger::error("invalid method: " + m);
        for (size_t j = 0; j < allowedMethods.size(); j++)
        {
            if (allowedMethods[j] == m)
                return Logger::error("duplicate method: " + m);
        }
        allowedMethods.push_back(m);
    }
    return true;
}

// getters
std::string LocationConfig::getPath() const
{
    return path;
}
std::string LocationConfig::getRoot() const
{
    return root;
}
bool LocationConfig::getAutoIndex() const
{
    return autoIndex;
}
bool LocationConfig::getUploadEnabled() const
{
    return uploadEnabled;
}
std::string LocationConfig::getUploadPath() const
{
    return uploadPath;
}
bool LocationConfig::getCgiEnabled() const
{
    return cgiEnabled;
}
std::string LocationConfig::getCgiPath() const
{
    return cgiPath;
}
std::string LocationConfig::getCgiExtension() const
{
    return cgiExtension;
}
std::string LocationConfig::getRedirect() const
{
    return redirect;
}
std::vector<std::string> LocationConfig::getAllowedMethods() const
{
    return allowedMethods;
}
std::string LocationConfig::getClientMaxBody() const
{
    return clientMaxBody;
}
std::vector<std::string> LocationConfig::getIndexes() const
{
    return indexes;
}