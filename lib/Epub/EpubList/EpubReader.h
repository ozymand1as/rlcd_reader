#pragma once

class Epub;
class Renderer;
class RubbishHtmlParser;

#include "./State.h"

class EpubReader
{
private:
  EpubListItem &state;
  Epub *epub = nullptr;
  Renderer *renderer = nullptr;
  RubbishHtmlParser *parser = nullptr;

  void parse_and_layout_current_section(bool silent = false);

public:
  EpubReader(EpubListItem &state, Renderer *renderer) : state(state), renderer(renderer) {}
  ~EpubReader() {}
  bool load();
  void next();
  void prev();
  void render();
  void setup_parser(bool show_loading = true);
  void set_state_section(uint16_t current_section);
};
