#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP


// ! ERROR 200 
#define HTTP_OK                     200

// ! ERROR 300

// ! ERROR 400
#define HTTP_BAD_REQUEST            400
#define HTTP_LENGTH_REQUIRED        411
#define HTTP_PAYLOAD_TOO_LARGE      413
#define HTTP_URI_TOO_LONG           414

// ! ERROR 500
#define HTTP_NOT_IMPLEMENTED        501
#define HTTP_VERSION_NOT_SUPPORTED  505

// ! MAX LIMITS
#define MAX_URI_LENGTH              8192
#define MAX_HEADER_SIZE             8192

#endif