#ifndef CONFIG_TOKEN_HPP
#define CONFIG_TOKEN_HPP

#include "../utils/Utils.hpp"

class ConfigToken {
   public:
    ConfigToken();
    ConfigToken(Type type, const String& value, int line);
    ConfigToken(const ConfigToken& other);
    ConfigToken& operator=(const ConfigToken& other);
    ~ConfigToken();

    Type          getType() const;
    const String& getValue() const;
    int           getLine() const;

   private:
    Type   _type;
    String _value;
    int    _n_line;
};

#endif