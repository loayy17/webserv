#ifndef FILE_HANDLER_HPP
#define FILE_HANDLER_HPP
#include <dirent.h>
#include "../utils/Utils.hpp"
class FileHandler {
   public:
    FileHandler();
    FileHandler(struct dirent* entry, const String& basePath, String pathUri);
    FileHandler(const FileHandler& other);
    FileHandler& operator=(const FileHandler& other);
    ~FileHandler();

    String getFileName() const;
    String getFileLink() const;
    String getIcon() const;
    String getSize() const;
    String getLastModifiedDate() const;

   private:
    String name;
    String link;
    String icon;
    String size;
    String lastModified;
};
#endif