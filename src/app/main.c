#include "app/doc.h"
#include "app/def.h"
#include "app/style.h"
#include "app/file_io.h"
#include "app/core_data.h"
#include "app/config.h"

// order is important, io_util & str_util before global
#define IO_UTIL_IMPLEMENTATION       // only define once
#include "global/io_util.h"          // only need to include here, normally included via global.h
#define STR_UTIL_IMPLEMENTATION      // only define once
#include "global/str_util.h"         // only need to include here, normally included via global.h
#include "global/global.h"

#define STB_DS_IMPLEMENTATION   // only define once
#include "stb/stb_ds.h"

#include <ctype.h>
#include <direct.h>
#include <dirent/dirent.h>

typedef enum
{
  SEARCH_DOCUMENTATION  = FLAG(0),
  SEARCH_DEFINITION     = FLAG(1),
  SEARCH_SEARCH         = FLAG(2),
}search_mode_t;

// activated by '-config'
bool print_config_cmd = false;
// activated by -c/-color 
bool color_cmd = false;

int main(int argc, char** argv)
{
  core_data_t* core_data = core_data_get();

  // -- get executable name --
  // P_STR(_getcwd(NULL, 0));
  // @BUGG: this only works on windows
  GetModuleFileName(NULL, core_data->exec_path, CORE_PATH_MAX);   // get executable location
  ERR_CHECK(core_data->exec_path != NULL, "failed getting executable path\n");



  // moved this after it stopped beeing used
  // core_data_init_sheet_path();
  // P_STR(core_data->sheets_path);
 
  // ---- commands ----

  search_mode_t mode = SEARCH_DOCUMENTATION;  // search docs by default
  u32 cmd_count = 0;
  #define WORD_ARR_MAX 24
  char* word_arr[WORD_ARR_MAX] = { NULL }; // all words, aka. not commands, like malloc, git, etc.
  int   word_arr_len = 0;
  for (u32 i = 1; i < argc; ++i)
  {
    // // @NOTE: '-h' or '-help' for help is in term_docs.sheet
    
    // @NOTE: '-config' to print config file
    if (argv[i][0] == '-'  && 
        argv[i][1] == 'c'  && 
        argv[i][2] == 'o'  && 
        argv[i][3] == 'n'  && 
        argv[i][4] == 'f'  && 
        argv[i][5] == 'i'  && 
        argv[i][6] == 'g') 
    {
      cmd_count++;
      print_config_cmd = true;
    }
    // @NOTE: '-c' or '-color' to disable color and styles
    else if ((argv[i][0] == '-'  && 
              argv[i][1] == 'c') ||
             
             (argv[i][0] == '-'  && 
              argv[i][1] == 'c'  && 
              argv[i][2] == 'o'  && 
              argv[i][3] == 'l'  && 
              argv[i][4] == 'o'  && 
              argv[i][5] == 'r') ) 
    {
      cmd_count++;
      color_cmd = true;
    }
    else if (argv[i][0] == '-'  && 
             argv[i][1] == 'l'  && 
             argv[i][2] == 'o'  && 
             argv[i][3] == 'c'  ) 
    {
      cmd_count++;
      core_data->print_loc_act = true;
    }
    
    // -- modes --

    else if (argv[i][0] == '-' && 
             argv[i][1] == 'd')      // -d: definnition
    {
      cmd_count++;
      mode = SEARCH_DEFINITION;
    }

    // if none of the commands its a word, i.e. 'malloc', 'function', etc.
    else
    { 
      word_arr[word_arr_len++] = argv[i]; 
      ERR_CHECK(word_arr_len < WORD_ARR_MAX, "more words than the word_arr arr can hold, max is: %d\n", WORD_ARR_MAX);
    }
  }
  
  // -c / -color modifier
  if (color_cmd) 
  { 
    core_data->style_act       = false; 
    core_data->style_deact_cmd = true; 
  } 
  
  // -- config file --
  
  core_data_init_config_path();
  // P_STR(config_path);
  config_read_config_file(core_data->config_path, print_config_cmd);

  // -- arguments --
  
  // too few arguments, at least 1, i.e. 2 because doc counts
  // not enough arguments for documentation or definition
  if (word_arr_len < 1) 
  { 
    DOC_PF_COLOR(PF_RED); PF("[!]"); DOC_PF_COLOR(PF_WHITE);  
    PF(" provide keyword to search or command, i.e. '"); 
    DOC_PF_COLOR(PF_CYAN); PF("-h"); DOC_PF_COLOR(PF_WHITE);
    PF("' for help.\n"); 
    exit(1);
  } // @NOTE: no actual stuff just '-abc' type stuff

  // ---- keywords ----

  int found_count = 0;  // counts found matches for keyword
  
  if (HAS_FLAG(mode, SEARCH_DOCUMENTATION))
  {
    // // @TMP: print word_arr
    // for (int i = 0; i < word_arr_len; ++i)
    // { P_STR(word_arr[i]); }
    
    // if (core_data->builtin_sheets_act)
    // { doc_search_dir(core_data->sheets_path, (const char**)word_arr, word_arr_len, &found_count); }
    // else
    // { DOC_PF_COLOR(PF_RED); PF("[!]"); DOC_PF_COLOR(PF_WHITE); PF(" searching without builtin sheets.\n"); }
    
    // custom doc paths
    for (int i = 0; i < core_data->sheet_paths_len; ++i)
    {
      // P_STR(core_data->custom_sheet_paths[i]);
      doc_search_dir(core_data->sheet_paths[i], (const char**)word_arr, word_arr_len, &found_count);
    }
  }
  else if (HAS_FLAG(mode, SEARCH_DEFINITION))
  {
    if (word_arr_len < 2) // need dir and keyword
    {
      DOC_PF_COLOR(PF_RED); PF("[!]"); DOC_PF_COLOR(PF_WHITE);  
      PF(" structure for definition search: '"); 
      DOC_PF_COLOR(PF_CYAN); PF("doc [directory] [keyword] -d"); DOC_PF_COLOR(PF_WHITE); PF("'\n");
      exit(1);
    }
    
    char* dir_name = word_arr[0]; // argv[1];
    char* keyword  = word_arr[1]; // argv[2];
    
    def_search_and_print(dir_name, keyword, &found_count);
  } 

  // -- check if nothing was found --
  // DOC_P_INT(found_count);
  if (found_count <= 0)
  { 
    if (HAS_FLAG(mode, SEARCH_DOCUMENTATION))
    {
      DOC_PF_COLOR(PF_RED); PF("[!]"); DOC_PF_COLOR(PF_WHITE);  
      PF(" could not find keyword%s '", word_arr_len > 1 ? "s" : ""); 
      DOC_PF_COLOR(PF_PURPLE);
      for (int i = 0; i < word_arr_len; ++i)
      {
        PF("%s",  word_arr[i]);
        DOC_PF_COLOR(PF_WHITE); 
        PF("%s",  i < word_arr_len -1 ? "', '" : "");
        DOC_PF_COLOR(PF_PURPLE);
      }
      DOC_PF_COLOR(PF_WHITE); 
      PF("' in sheets.\n");
    }
    else if (HAS_FLAG(mode, SEARCH_DEFINITION))
    {
      DOC_PF_COLOR(PF_RED); PF("[!]"); DOC_PF_COLOR(PF_WHITE);  
      PF(" could not find function defenition '"); 
      DOC_PF_COLOR(PF_PURPLE);
      PF("%s", word_arr[1]); 
      DOC_PF_COLOR(PF_WHITE); 
      PF("'\n    in header files under '");
      DOC_PF_COLOR(PF_PURPLE);
      PF("%s", word_arr[0]); 
      DOC_PF_COLOR(PF_WHITE); 
      PF("'\n");
    }
    else { P_ERR("unnkown mode"); }
  }
  
  DOC_PF_COLOR(PF_WHITE);
  DOC_PF_STYLE_RESET();
	return 0;
}

