NAME        = webserv
CXX         = c++
CXXFLAGS    = -Wall -Wextra -Werror -std=c++98 -g

CONFIG_TESTER_NAME = config_tester
REQUEST_TESTER_NAME = request_tester
ROUTER_TESTER_NAME  = router_tester

SRC_DIR     = src
OBJ_DIR     = obj
TEST_DIR    = tests


# --------------------------------
# Sources and objects
# -------------------------------
# config sources
SRC_CONFIG = $(SRC_DIR)/config/ConfigLexer.cpp \
				$(SRC_DIR)/config/ConfigParser.cpp \
				$(SRC_DIR)/config/ConfigToken.cpp \
				$(SRC_DIR)/config/ListenAddressConfig.cpp \
				$(SRC_DIR)/config/LocationConfig.cpp \
				$(SRC_DIR)/config/MimeTypes.cpp \
				$(SRC_DIR)/config/ServerConfig.cpp

# handlers sources
SRC_HANDLERS = $(SRC_DIR)/handlers/CgiHandler.cpp \
				$(SRC_DIR)/handlers/CgiProcess.cpp \
				$(SRC_DIR)/handlers/DeleteHandler.cpp \
				$(SRC_DIR)/handlers/DirectoryListingHandler.cpp \
				$(SRC_DIR)/handlers/ErrorPageHandler.cpp \
				$(SRC_DIR)/handlers/FileHandler.cpp \
				$(SRC_DIR)/handlers/StaticFileHandler.cpp \
				$(SRC_DIR)/handlers/UploaderHandler.cpp

# HTTP sources
SRC_HTTP = $(SRC_DIR)/http/HttpRequest.cpp \
			$(SRC_DIR)/http/HttpResponse.cpp \
			$(SRC_DIR)/http/ResponseBuilder.cpp \
			$(SRC_DIR)/http/RouteResult.cpp \
			$(SRC_DIR)/http/Router.cpp


# server sources
SRC_SERVER = $(SRC_DIR)/server/Client.cpp \
				$(SRC_DIR)/server/EpollManager.cpp \
				$(SRC_DIR)/server/PollManager.cpp \
				$(SRC_DIR)/server/Server.cpp \
				$(SRC_DIR)/server/ServerManager.cpp \
				$(SRC_DIR)/server/ServerManagerPoll.cpp

# utils sources
SRC_UTILS = $(SRC_DIR)/utils/Logger.cpp \
			$(SRC_DIR)/utils/SessionManager.cpp \
			$(SRC_DIR)/utils/SessionResult.cpp \
			$(SRC_DIR)/utils/Utils.cpp
# -------------------------------
# Main files (ONLY mains)
# -------------------------------
MAIN_SRC        = $(SRC_DIR)/main.cpp
CONFIG_MAIN     = $(TEST_DIR)/config_tester.cpp
REQUEST_MAIN    = $(TEST_DIR)/request_tester.cpp
ROUTER_MAIN     = $(TEST_DIR)/router_tester.cpp

# -------------------------------
# All project sources EXCEPT main
# -------------------------------
SRCS_NO_MAIN = $(SRC_CONFIG) \
				$(SRC_HANDLERS) \
				$(SRC_HTTP) \
				$(SRC_SERVER) \
				$(SRC_UTILS) 

# -------------------------------
SRCS_MAIN = $(MAIN_SRC) \
			$(SRCS_NO_MAIN)

SRCS_CONFIG_TESTER = $(CONFIG_MAIN) \
				 $(SRCS_NO_MAIN)

SRCS_REQUEST_TESTER = $(REQUEST_MAIN) \
					 $(SRCS_NO_MAIN)

SRCS_ROUTER_TESTER = $(ROUTER_MAIN) \
					$(SRCS_NO_MAIN)


OBJS_MAIN = $(SRCS_MAIN:.cpp=.o)
OBJS_CONFIG_TESTER = $(SRCS_CONFIG_TESTER:.cpp=.o)
OBJS_REQUEST_TESTER = $(SRCS_REQUEST_TESTER:.cpp=.o)
OBJS_ROUTER_TESTER = $(SRCS_ROUTER_TESTER:.cpp=.o)
# =================================================
# DEFAULT TARGET
# =================================================

all: $(NAME)
$(NAME): $(OBJS_MAIN)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS_MAIN)
# =================================================
# TESTERS (same OBJS, different main)
# =================================================
config_tester: $(OBJS_CONFIG_TESTER)
	$(CXX) $(CXXFLAGS) -o $(CONFIG_TESTER_NAME) $(OBJS_CONFIG_TESTER) 

request_tester: $(OBJS_REQUEST_TESTER)
	$(CXX) $(CXXFLAGS) -o $(REQUEST_TESTER_NAME) $(OBJS_REQUEST_TESTER)

router_tester: $(OBJS_ROUTER_TESTER)
	$(CXX) $(CXXFLAGS) -o $(ROUTER_TESTER_NAME) $(OBJS_ROUTER_TESTER)

tests: config_tester request_tester router_tester

# =================================================
# CLEANING
# =================================================
clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME) config_tester request_tester router_tester

re: fclean all

.PHONY: all clean fclean re tests \
        config_tester request_tester router_tester
