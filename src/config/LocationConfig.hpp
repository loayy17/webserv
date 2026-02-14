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
    LocationConfig(const String& p);
    ~LocationConfig();

    bool setRoot(const VectorString& r);
    void setRoot(const String& r);

    void setAutoIndex(bool v);
    bool setAutoIndex(const VectorString& v);

    bool setIndexes(const VectorString& i);
    void setUploadDir(const String& p);
    bool setUploadDir(const VectorString& p);
    bool setCgiPass(const VectorString& c);
    bool setRedirect(const VectorString& r);
    bool setErrorPage(const VectorString& values);

    void setClientMaxBody(const String& c);
    bool setClientMaxBody(const VectorString& c);

    void             addAllowedMethod(const String& m);
    bool             setAllowedMethods(const VectorString& m);
    String           getPath() const;
    String           getRoot() const;
    bool             getAutoIndex() const;
    VectorString     getIndexes() const;
    String           getUploadDir() const;
    const MapString& getCgiPass() const;
    String           getCgiInterpreter(const String& extension) const;
    bool             hasCgi() const;
    String           getClientMaxBody() const;
    VectorString     getAllowedMethods() const;
    String           getErrorPage(int code) const;
    bool             getIsRedirect() const;
    int              getRedirectCode() const;
    String           getRedirectValue() const;

   private:
    // required location parameters
    String path;
    // optional location parameters
    String       root;           // default root of server if not set (be required)
    bool         autoIndex;      // default: false
    bool         autoIndexSet;   // tracks if autoindex directive was used
    VectorString indexes;        // default: root if not set be default "index.html"
    String       uploadDir;      // upload directory path
    MapString    cgiPass;        // maps extension to interpreter path
    String       clientMaxBody;  // default: ""
    VectorString allowedMethods; // default: GET
    MapIntString errorPage;      // maps error code to error page path
    bool         hasRedirect;
    int          redirectCode;     
    String       redirectValue;

};

#endif