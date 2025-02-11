void set_old_st(struct termios oldt) {
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}

#define new_st \
  struct termios oldt; \
  struct termios newt; \
  tcgetattr(STDIN_FILENO, &oldt); \
  memcpy(&newt, &oldt, sizeof(oldt)); \
  newt.c_iflag &= ~(IXON); \
  newt.c_lflag &= ~(ICANON | ECHO | ISIG | IEXTEN); \
  tcsetattr(STDIN_FILENO, TCSANOW, &newt)

#define old_st set_old_st(oldt)
#define old_st_legacy system("stty icanon isig iexten echo");
