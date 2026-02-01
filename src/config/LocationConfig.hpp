#ifndef LOCATION_CONFIG_HPP
#define LOCATION_CONFIG_HPP
#include <iostream>
#include <map>
#include <vector>
#include "../utils/Logger.hpp"
#include "../utils/Utils.hpp"
class LocationConfig {
   public:
    LocationConfig();
    LocationConfig(const LocationConfig& other);
    LocationConfig& operator=(const LocationConfig& other);
    LocationConfig(const std::string& p);
    ~LocationConfig();

    bool setRoot(const VectorString& r);
    void setRoot(const std::string& r);

    void setAutoIndex(bool v);
    bool setAutoIndex(const VectorString& v);

    bool setIndexes(const VectorString& i);
    void setUploadDir(const std::string& p);
    bool setUploadDir(const VectorString& p);
    bool setCgiPass(const VectorString& c);
    void setRedirect(const std::string& r);
    bool setRedirect(const VectorString& r);

    void setClientMaxBody(const std::string& c);
    bool setClientMaxBody(const VectorString& c);

    void         addAllowedMethod(const std::string& m);
    bool         setAllowedMethods(const VectorString& m);
    std::string  getPath() const;
    std::string  getRoot() const;
    bool         getAutoIndex() const;
    VectorString getIndexes() const;
    std::string  getUploadDir() const;
    const std::map<std::string, std::string>& getCgiPass() const;
    std::string  getCgiInterpreter(const std::string& extension) const;
    bool         hasCgi() const;
    std::string  getRedirect() const;
    std::string  getClientMaxBody() const;
    VectorString getAllowedMethods() const;

   private:
    // required location parameters
    std::string path;
    // optional location parameters
    std::string  root;           // default root of server if not set (be required)
    bool         autoIndex;      // default: false
    bool         autoIndexSet;   // tracks if autoindex directive was used
    VectorString indexes;        // default: root if not set be default "index.html"
    std::string  uploadDir;                        // upload directory path
    std::map<std::string, std::string> cgiPass;   // maps extension to interpreter path
    std::string  redirect;       // default: ""
    std::string  clientMaxBody;  // default: ""
    VectorString allowedMethods; // default: GET
};

#endif