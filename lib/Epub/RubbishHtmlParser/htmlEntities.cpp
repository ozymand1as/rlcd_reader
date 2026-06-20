#include "htmlEntities.h"
#include <string>
#include <unordered_map>

const int MAX_ENTITY_LENGTH = 10;

static std::unordered_map<std::string, std::string> entity_lookup(
    {{"&quot;", "\""},
     {"&amp;", "&"},
     {"&lt;", "<"},
     {"&gt;", ">"},
     {"&nbsp;", " "},
     {"&apos;", "'"},
     {"&copy;", "©"},
     {"&reg;", "®"},
     {"&trade;", "™"},
     {"&euro;", "€"},
     {"&pound;", "£"},
     {"&yen;", "¥"},
     {"&ndash;", "–"},
     {"&mdash;", "—"},
     {"&lsquo;", "'"},
     {"&rsquo;", "'"},
     {"&ldquo;", ""\""},
     {"&rdquo;", ""\""},
     {"&hellip;", "…"},
     {"&bull;", "•"},
     {"&middot;", "·"},
     {"&times;", "×"},
     {"&divide;", "÷"},
     {"&plusmn;", "±"},
     {"&micro;", "µ"},
     {"&para;", "¶"},
     {"&sect;", "§"},
     {"&deg;", "°"},
     {"&frac14;", "¼"},
     {"&frac12;", "½"},
     {"&frac34;", "¾"},
     {"&iquest;", "¿"},
     {"&iexcl;", "¡"},
     {"&laquo;", "«"},
     {"&raquo;", "»"}});

void convert_to_utf8(int code, std::string &res)
{
  if (code < 0x80)
  {
    res += static_cast<char>(code);
  }
  else if (code < 0x800)
  {
    res += static_cast<char>(0xc0 | (code >> 6));
    res += static_cast<char>(0x80 | (code & 0x3f));
  }
  else if (code < 0x10000)
  {
    res += static_cast<char>(0xe0 | (code >> 12));
    res += static_cast<char>(0x80 | ((code >> 6) & 0x3f));
    res += static_cast<char>(0x80 | (code & 0x3f));
  }
}

bool process_numeric_entity(const std::string &entity, std::string &res)
{
  int code = 0;
  if (entity[2] == 'x' || entity[2] == 'X')
  {
    code = strtol(entity.substr(3, entity.size() - 3).c_str(), nullptr, 16);
  }
  else
  {
    code = strtol(entity.substr(2, entity.size() - 3).c_str(), nullptr, 10);
  }
  if (code != 0)
  {
    if (code == 0xA0)
    {
      res += " ";
    }
    else
    {
      convert_to_utf8(code, res);
    }
    return true;
  }
  return false;
}

bool process_string_entity(const std::string &entity, std::string &res)
{
  auto it = entity_lookup.find(entity);
  if (it != entity_lookup.end())
  {
    res += it->second;
    return true;
  }
  return false;
}

std::string replace_html_entities(const std::string &text)
{
  std::string res;
  res.reserve(text.size());
  for (int i = 0; i < text.size(); ++i)
  {
    bool flag = false;
    if (text[i] == '&')
    {
      int j = i + 1;
      while (j < text.size() && text[j] != ';' && j - i < MAX_ENTITY_LENGTH)
      {
        j++;
      }
      if (j - i > 2)
      {
        std::string entity = text.substr(i, j - i + 1);
        if (entity[1] == '#')
        {
          flag = process_numeric_entity(entity, res);
        }
        else
        {
          flag = process_string_entity(entity, res);
        }
        if (flag)
        {
          i = j;
        }
      }
    }
    if (!flag)
    {
      res += text[i];
    }
  }
  return res;
}
