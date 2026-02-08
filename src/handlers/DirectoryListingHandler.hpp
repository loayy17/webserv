#ifndef DIRECTORY_LISTING_HANDLER_HPP
#define DIRECTORY_LISTING_HANDLER_HPP

#include <dirent.h>
#include <vector>
#include "../http/HttpResponse.hpp"
#include "../utils/Logger.hpp"
#include "../utils/Utils.hpp"
#include "FileHandler.hpp"
#include "IHandler.hpp"

class DirectoryListingHandler : public IHandler {
   public:
    DirectoryListingHandler();
    DirectoryListingHandler(const DirectoryListingHandler& other);
    DirectoryListingHandler& operator=(const DirectoryListingHandler& other);
    ~DirectoryListingHandler();

    bool handle(const Router& router, HttpResponse& response) const;

   private:
    String buildRow(const FileHandler& fileInfo) const;
    bool   readDirectoryEntries(const String& path, const String& uri, std::vector<FileHandler>& entries) const;
};

#endif
