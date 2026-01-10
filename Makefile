NAME = webserv
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98
SRC_DIR = src
OBJ_DIR = obj
SRCS = $(SRC_DIR)/main.cpp \
 $(SRC_DIR)/server/Server.cpp \
 $(SRC_DIR)/server/ServerManager.cpp \
 $(SRC_DIR)/server/Connection.cpp \
 $(SRC_DIR)/server/PollManager.cpp \
 $(SRC_DIR)/config/ConfigParser.cpp \
 $(SRC_DIR)/config/ServerConfig.cpp \
 $(SRC_DIR)/config/LocationConfig.cpp \
 $(SRC_DIR)/config/MimeTypes.cpp \
 $(SRC_DIR)/http/HttpRequest.cpp \
 $(SRC_DIR)/http/HttpResponse.cpp \
 $(SRC_DIR)/http/Router.cpp \
 $(SRC_DIR)/http/RequestValidator.cpp \
 $(SRC_DIR)/handlers/StaticFileHandler.cpp \
 $(SRC_DIR)/handlers/CgiHandler.cpp \
 $(SRC_DIR)/handlers/UploadHandler.cpp \
 $(SRC_DIR)/handlers/DirectoryListing.cpp \
 $(SRC_DIR)/handlers/ErrorPageHandler.cpp \
 $(SRC_DIR)/utils/Utils.cpp \
 $(SRC_DIR)/utils/SessionManager.cpp \
 $(SRC_DIR)/utils/Logger.cpp
OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
all: $(NAME)
$(NAME): $(OBJS)
$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
@mkdir -p $(dir $@)
$(CXX) $(CXXFLAGS) -c $< -o $@
clean:
rm -rf $(OBJ_DIR)
fclean: clean
rm -f $(NAME)
re: fclean all
.PHONY: all clean fclean re