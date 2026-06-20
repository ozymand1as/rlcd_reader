#pragma once

#include <string>
#include <vector>
#include "../Renderer/Renderer.h"

struct EpubListState
{
  int num_epubs = 0;
  int selected_item = 0;
  bool is_loaded = false;
  std::vector<std::string> epub_list;
};

class EpubList
{
private:
  Renderer *m_renderer;
  EpubListState &m_state;
  
public:
  EpubList(Renderer *renderer, EpubListState &state);
  ~EpubList();
  
  bool load(const std::string &path);
  void render();
  void prev();
  void next();
  void set_needs_redraw();
  
  std::string get_selected_item();
};
