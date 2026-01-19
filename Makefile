NAME = webserv
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98
SRC_DIR = src
OBJ_DIR = obj
SRCS_Config = $(SRC_DIR)/config
SRCS_Server = $(SRC_DIR)/server
SRCS_Http = $(SRC_DIR)/http
SRCS_Handlers = $(SRC_DIR)/handlers
SRCS_Utils = $(SRC_DIR)/utils
SRCS = $(wildcard $(SRC_DIR)/*.cpp $(SRCS_Config)/*.cpp $(SRCS_Server)/*.cpp $(SRCS_Http)/*.cpp $(SRCS_Handlers)/*.cpp $(SRCS_Utils)/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))
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