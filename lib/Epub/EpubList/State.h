#pragma once

#include <stdint.h>

const int MAX_EPUB_LIST_SIZE = 20;
const int MAX_PATH_SIZE = 256;
const int MAX_TITLE_SIZE = 100;

typedef struct
{
  char path[MAX_PATH_SIZE];
  char title[MAX_TITLE_SIZE];
  uint16_t current_section;
  uint16_t current_page;
  uint16_t pages_in_current_section;
} EpubListItem;

typedef struct
{
  int previous_rendered_page;
  int previous_selected_item;
  int selected_item;
  int num_epubs;
  bool is_loaded;
  EpubListItem epub_list[MAX_EPUB_LIST_SIZE];
} EpubListState;

typedef struct
{
  int previous_rendered_page;
  int previous_selected_item;
  int selected_item;
} EpubTocState;
