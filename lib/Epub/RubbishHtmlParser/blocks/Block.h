#pragma once

class Renderer;
class Epub;

typedef enum
{
  TEXT_BLOCK,
  IMAGE_BLOCK
} BlockType;

class Block
{
public:
  virtual ~Block() {}
  virtual void layout(Renderer *renderer, Epub *epub, int max_width = -1) = 0;
  virtual void dump() = 0;
  virtual BlockType getType() = 0;
  virtual bool isEmpty() = 0;
  virtual void finish(){};
};
