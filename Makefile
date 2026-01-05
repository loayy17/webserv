NAME		= webserv
CXX			= c++
CXXFLAGS	= -Wall -Wextra -Werror -std=c++98
OBJ_DIR		= obj
SRC_DIR		= src
SRCS		= src/main.cpp \
			  src/server/Server.cpp \
			  src/server/Client.cpp \
			  src/server/PollManager.cpp \
			  src/utils/Logger.cpp \
			  src/utils/FileManager.cpp \
			  src/http/HttpRequest.cpp \
			  src/http/HttpResponse.cpp \
			  src/http/Router.cpp \
			  src/controllers/BaseController.cpp \
			  src/controllers/ApiController.cpp \
			  src/controllers/StaticController.cpp \
			  src/controllers/ErrorController.cpp \
			  src/controllers/UploadController.cpp

OBJS		= $(SRCS:${SRC_DIR}/%.cpp=$(OBJ_DIR)/%.o)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@
	@echo "✓ Compiled $<"

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
	@echo "✓ $(NAME) compiled"

clean:
	rm -rf $(OBJ_DIR)
	@echo "✓ Clean"

fclean: clean
	rm -f $(NAME)
	@echo "✓ Full clean"

re: fclean all

.PHONY: all clean fclean re
