#ifndef LOCATION_CONFIG_HPP
#define LOCATION_CONFIG_HPP
#include <iostream>
#include <vector>
#include "../utils/Logger.hpp"
#include "../utils/Utils.hpp"
class LocationConfig {
   public:
    LocationConfig(const std::string& p);
    ~LocationConfig();

    bool setRoot(const std::vector<std::string>& r);
    void setRoot(const std::string& r);

    void setAutoIndex(bool v);
    bool setAutoIndex(const std::vector<std::string>& v);

    bool setIndexes(const std::vector<std::string>& i);
    void setUploadEnabled(bool v);
    void setUploadPath(const std::string& p);
    bool setUploadPath(const std::vector<std::string>& p);
    void setCgiEnabled(bool v);
    void setCgiPath(const std::string& p);
    bool setCgiPath(const std::vector<std::string>& p);
    void setCgiExtension(const std::string& e);
    bool setCgiExtension(const std::vector<std::string>& e);
    void setRedirect(const std::string& r);
    bool setRedirect(const std::vector<std::string>& r);

    void setClientMaxBody(const std::string& c);
    bool setClientMaxBody(const std::vector<std::string>& c);

    void                     addAllowedMethod(const std::string& m);
    bool                     setAllowedMethods(const std::vector<std::string>& m);
    std::string              getPath() const;
    std::string              getRoot() const;
    bool                     getAutoIndex() const;
    std::vector<std::string> getIndexes() const;
    bool                     getUploadEnabled() const;
    std::string              getUploadPath() const;
    bool                     getCgiEnabled() const;
    std::string              getCgiPath() const;
    std::string              getCgiExtension() const;
    std::string              getRedirect() const;
    std::string              getClientMaxBody() const;
    std::vector<std::string> getAllowedMethods() const;

   private:
    LocationConfig();
    // required location parameters
    std::string path;
    // optional location parameters
    std::string              root;           // default root of server if not set (be required)
    bool                     autoIndex;      // default: false
    std::vector<std::string> indexes;        // default: root if not set be default "index.html"
    bool                     uploadEnabled;  // default: false
    std::string              uploadPath;     // be required if uploadEnabled is true
    bool                     cgiEnabled;     // default: false
    std::string              cgiPath;        // be required if cgiEnabled is true
    std::string              cgiExtension;   // be required if cgiEnabled is true
    std::string              redirect;       // default: ""
    std::string              clientMaxBody;  // default: ""
    std::vector<std::string> allowedMethods; // default: GET
};

#endif