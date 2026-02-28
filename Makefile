NAME        = webserv
CXX         = c++
CXXFLAGS    = -Wall -Wextra -Werror -std=c++98 -g

SRC_DIR     = src
OBJ_DIR     = obj
TEST_DIR    = tests

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
SRCS = $(wildcard \
	$(SRC_DIR)/config/*.cpp \
	$(SRC_DIR)/server/*.cpp \
	$(SRC_DIR)/http/*.cpp \
	$(SRC_DIR)/handlers/*.cpp \
	$(SRC_DIR)/utils/*.cpp)

OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))

# =================================================
# DEFAULT TARGET
# =================================================
all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) $(MAIN_SRC) -o $@

# =================================================
# OBJECT RULE
# =================================================
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# =================================================
# TESTERS (same OBJS, different main)
# =================================================
config_tester: $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) $(CONFIG_MAIN) -o $@

request_tester: $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) $(REQUEST_MAIN) -o $@

router_tester: $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) $(ROUTER_MAIN) -o $@

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
