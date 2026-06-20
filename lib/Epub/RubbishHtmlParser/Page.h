#pragma once

#include "blocks/TextBlock.h"
#include "blocks/ImageBlock.h"
#include <vector>

class PageElement
{
public:
  int y_pos;
  PageElement(int y_pos) : y_pos(y_pos) {}
  virtual ~PageElement() {}
  virtual void render(Renderer *renderer, Epub *epub) = 0;
};

class PageLine : public PageElement
{
public:
  TextBlock *block;
  int line_break_index;

  PageLine(TextBlock *block, int line_break_index, int y_pos)
      : PageElement(y_pos), block(block), line_break_index(line_break_index) {}
  
  void render(Renderer *renderer, Epub *epub)
  {
    block->render(renderer, line_break_index, 0, y_pos);
  }
};

class PageImage : public PageElement
{
public:
  ImageBlock *block;

  PageImage(ImageBlock *block, int y_pos) : PageElement(y_pos), block(block) {}
  
  void render(Renderer *renderer, Epub *epub)
  {
    block->render(renderer, epub, y_pos);
  }
};

class Page
{
public:
  std::vector<PageElement *> elements;
  
  void render(Renderer *renderer, Epub *epub)
  {
    for (auto element : elements)
    {
      element->render(renderer, epub);
    }
  }
  
  ~Page()
  {
    for (auto element : elements)
    {
      delete element;
    }
  }
};
