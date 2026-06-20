#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <esp_log.h>
#include <map>
#include "tinyxml2.h"
#include "../ZipFile/ZipFile.h"
#include "Epub.h"

static const char *TAG = "EPUB";

std::string normalise_path(const std::string &path)
{
  std::vector<std::string> components;
  std::string component;
  for (auto c : path)
  {
    if (c == '/')
    {
      if (!component.empty())
      {
        if (component == "..")
        {
          if (!components.empty())
          {
            components.pop_back();
          }
        }
        else
        {
          components.push_back(component);
        }
        component.clear();
      }
    }
    else
    {
      component += c;
    }
  }
  if (!component.empty())
  {
    components.push_back(component);
  }
  std::string result;
  for (auto &component : components)
  {
    if (result.size() > 0)
    {
      result += "/";
    }
    result += component;
  }
  return result;
}

bool Epub::find_content_opf_file(ZipFile &zip, std::string &content_opf_file)
{
  char *meta_info = (char *)zip.read_file_to_memory("META-INF/container.xml");
  if (!meta_info)
  {
    ESP_LOGE(TAG, "Could not find META-INF/container.xml");
    return false;
  }
  
  tinyxml2::XMLDocument meta_data_doc;
  auto result = meta_data_doc.Parse(meta_info);
  free(meta_info);
  
  if (result != tinyxml2::XML_SUCCESS)
  {
    ESP_LOGE(TAG, "Could not parse META-INF/container.xml");
    return false;
  }
  
  auto container = meta_data_doc.FirstChildElement("container");
  if (!container)
  {
    ESP_LOGE(TAG, "Could not find container element");
    return false;
  }
  
  auto rootfiles = container->FirstChildElement("rootfiles");
  if (!rootfiles)
  {
    ESP_LOGE(TAG, "Could not find rootfiles element");
    return false;
  }
  
  auto rootfile = rootfiles->FirstChildElement("rootfile");
  while (rootfile)
  {
    const char *media_type = rootfile->Attribute("media-type");
    if (media_type && strcmp(media_type, "application/oebps-package+xml") == 0)
    {
      const char *full_path = rootfile->Attribute("full-path");
      if (full_path)
      {
        content_opf_file = full_path;
        return true;
      }
    }
    rootfile = rootfile->NextSiblingElement("rootfile");
  }
  
  ESP_LOGE(TAG, "Could not get path to content.opf file");
  return false;
}

bool Epub::parse_content_opf(ZipFile &zip, std::string &content_opf_file)
{
  char *contents = (char *)zip.read_file_to_memory(content_opf_file.c_str());
  tinyxml2::XMLDocument doc;
  auto result = doc.Parse(contents);
  free(contents);
  
  if (result != tinyxml2::XML_SUCCESS)
  {
    ESP_LOGE(TAG, "Error parsing content.opf - %s", doc.ErrorIDToName(result));
    return false;
  }
  
  auto package = doc.FirstChildElement("package");
  if (!package)
  {
    ESP_LOGE(TAG, "Could not find package element");
    return false;
  }
  
  auto metadata = package->FirstChildElement("metadata");
  if (!metadata)
  {
    ESP_LOGE(TAG, "Missing metadata");
    return false;
  }
  
  auto title = metadata->FirstChildElement("dc:title");
  if (!title)
  {
    ESP_LOGE(TAG, "Missing title");
    return false;
  }
  m_title = title->GetText();
  
  auto cover = metadata->FirstChildElement("meta");
  while (cover && cover->Attribute("name") && strcmp(cover->Attribute("name"), "cover") != 0)
  {
    cover = cover->NextSiblingElement("meta");
  }
  
  auto cover_item = cover ? cover->Attribute("content") : nullptr;
  
  auto manifest = package->FirstChildElement("manifest");
  if (!manifest)
  {
    ESP_LOGE(TAG, "Missing manifest");
    return false;
  }
  
  auto item = manifest->FirstChildElement("item");
  std::map<std::string, std::string> items;
  
  while (item)
  {
    std::string item_id = item->Attribute("id");
    std::string href = m_base_path + item->Attribute("href");
    
    if (cover_item && item_id == cover_item)
    {
      m_cover_image_item = href;
    }
    if (item_id == "ncx")
    {
      m_toc_ncx_item = href;
    }
    items[item_id] = href;
    item = item->NextSiblingElement("item");
  }
  
  auto spine = package->FirstChildElement("spine");
  if (!spine)
  {
    ESP_LOGE(TAG, "Missing spine");
    return false;
  }
  
  auto itemref = spine->FirstChildElement("itemref");
  while (itemref)
  {
    auto id = itemref->Attribute("idref");
    if (items.find(id) != items.end())
    {
      m_spine.push_back(std::make_pair(id, items[id]));
    }
    itemref = itemref->NextSiblingElement("itemref");
  }
  
  return true;
}

bool Epub::parse_toc_ncx_file(ZipFile &zip)
{
  if (m_toc_ncx_item.empty())
  {
    ESP_LOGE(TAG, "No ncx file specified");
    return false;
  }
  
  char *ncx_data = (char *)zip.read_file_to_memory(m_toc_ncx_item.c_str());
  if (!ncx_data)
  {
    ESP_LOGE(TAG, "Could not find %s", m_toc_ncx_item.c_str());
    return false;
  }
  
  tinyxml2::XMLDocument doc;
  auto result = doc.Parse(ncx_data);
  free(ncx_data);
  
  if (result != tinyxml2::XML_SUCCESS)
  {
    ESP_LOGE(TAG, "Error parsing toc %s", doc.ErrorIDToName(result));
    return false;
  }
  
  auto ncx = doc.FirstChildElement("ncx");
  if (!ncx)
  {
    ESP_LOGE(TAG, "Could not find ncx element");
    return false;
  }
  
  auto navMap = ncx->FirstChildElement("navMap");
  if (!navMap)
  {
    ESP_LOGE(TAG, "Could not find navMap element");
    return false;
  }
  
  auto navPoint = navMap->FirstChildElement("navPoint");
  
  while (navPoint)
  {
    auto navLabel = navPoint->FirstChildElement("navLabel")->FirstChildElement("text")->FirstChild();
    std::string title = navLabel->Value();
    auto content = navPoint->FirstChildElement("content");
    std::string href = m_base_path + content->Attribute("src");
    
    size_t pos = href.find('#');
    std::string anchor = "";
    if (pos != std::string::npos)
    {
      anchor = href.substr(pos + 1);
      href = href.substr(0, pos);
    }
    
    m_toc.push_back(EpubTocEntry(title, href, anchor, 0));
    navPoint = navPoint->NextSiblingElement("navPoint");
  }
  
  return true;
}

Epub::Epub(const std::string &path) : m_path(path)
{
}

bool Epub::load()
{
  ZipFile zip(m_path.c_str());
  std::string content_opf_file;
  
  if (!find_content_opf_file(zip, content_opf_file))
  {
    ESP_LOGE(TAG, "Could not open ePub");
    return false;
  }
  
  m_base_path = content_opf_file.substr(0, content_opf_file.find_last_of('/') + 1);
  
  if (!parse_content_opf(zip, content_opf_file))
  {
    return false;
  }
  
  if (!parse_toc_ncx_file(zip))
  {
    return false;
  }
  
  return true;
}

const std::string &Epub::get_title()
{
  return m_title;
}

const std::string &Epub::get_cover_image_item()
{
  return m_cover_image_item;
}

uint8_t *Epub::get_item_contents(const std::string &item_href, size_t *size)
{
  ZipFile zip(m_path.c_str());
  std::string path = normalise_path(item_href);
  auto content = zip.read_file_to_memory(path.c_str(), size);
  if (!content)
  {
    ESP_LOGE(TAG, "Failed to read item %s", path.c_str());
    return nullptr;
  }
  return content;
}

int Epub::get_spine_items_count()
{
  return m_spine.size();
}

std::string &Epub::get_spine_item(int spine_index)
{
  try
  {
    return m_spine.at(spine_index).second;
  }
  catch (const std::out_of_range &oor)
  {
    ESP_LOGI(TAG, "get_spine_item index:%d is out_of_range", spine_index);
    spine_index = 0;
  }
  return m_spine.at(spine_index).second;
}

EpubTocEntry &Epub::get_toc_item(int toc_index)
{
  return m_toc[toc_index];
}

int Epub::get_toc_items_count()
{
  return m_toc.size();
}

int Epub::get_spine_index_for_toc_index(int toc_index)
{
  for (int i = 0; i < m_spine.size(); i++)
  {
    if (m_spine[i].second == m_toc[toc_index].href)
    {
      return i;
    }
  }
  ESP_LOGI(TAG, "Section not found");
  return 0;
}
