// Basic processor functions
struct govnodate {
  short year;
  char  month;
  char  day;
};
typedef struct govnodate govnodate;

govnodate govnodate_convert(unsigned short date) {
  return (govnodate){
    .year = date / 372 + 1970,
    .month = (date % 372) / 31 + 1,
    .day = (date % 372) % 31 + 1
  };
}

U16 GC_GOVNODATE() {
  time_t rawtm;
  struct tm* localtm;

  time(&rawtm);
  localtm = localtime(&rawtm);
  return (localtm->tm_mday - 1) + (localtm->tm_mon * 31) + (localtm->tm_year - 70) * 372;
}

U0 fatal(char* msg) {
  printf("gc16: \033[91mcannot operate\033[0m, error:\n  %s", msg);
}
