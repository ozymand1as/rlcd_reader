#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <esp_sleep.h>
#include <esp_log.h>
#include "boards/Board.h"
#include "boards/controls/RLCDButtonControls.h"
#include "../lib/Epub/EpubList/EpubList.h"

#ifdef LOG_ENABLED
#define LOG_LEVEL ESP_LOG_INFO
#else
#define LOG_LEVEL ESP_LOG_NONE
#endif

extern "C"
{
  void app_main();
}

const char *TAG = "main";

typedef enum
{
  SELECTING_EPUB,
  SELECTING_TABLE_CONTENTS,
  READING_EPUB
} UIState;

// UI state
RTC_NOINIT_ATTR UIState ui_state = SELECTING_EPUB;
RTC_DATA_ATTR EpubListState epub_list_state;

// Global objects
static EpubList *epub_list = nullptr;

void handleEpubList(Renderer *renderer, UIAction action, bool needs_redraw);

void main_task(void *param)
{
  ESP_LOGI(TAG, "Powering up the board");
  
  // Create board instance
  Board *board = Board::factory();
  if (!board)
  {
    ESP_LOGE(TAG, "Failed to create board");
    return;
  }
  
  // Power up the board
  board->power_up();
  
  // Create renderer
  ESP_LOGI(TAG, "Creating renderer");
  Renderer *renderer = board->get_renderer();
  if (!renderer)
  {
    ESP_LOGE(TAG, "Failed to create renderer");
    return;
  }
  
  // Start filesystem
  ESP_LOGI(TAG, "Starting file system");
  board->start_filesystem();
  
  // Setup battery monitoring
  ESP_LOGI(TAG, "Starting battery monitor");
  Battery *battery = board->get_battery();
  if (battery)
  {
    battery->setup();
  }
  
  // Configure renderer margins
  renderer->set_margin_top(10);
  renderer->set_margin_left(10);
  renderer->set_margin_right(10);
  renderer->set_margin_bottom(10);
  
  // Create UI event queue
  xQueueHandle ui_queue = xQueueCreate(10, sizeof(UIAction));
  
  // Setup button controls
  ESP_LOGI(TAG, "Setting up controls");
  RLCDButtonControls *button_controls = new RLCDButtonControls(
    BUTTON_UP_GPIO, BUTTON_DOWN_GPIO, BUTTON_SELECT_GPIO,
    BUTTON_ACTIVE_LEVEL, ui_queue
  );
  button_controls->setup();
  
  ESP_LOGI(TAG, "Controls configured");
  
  // Check if we woke from deep sleep
  if (button_controls->did_wake_from_deep_sleep())
  {
    bool hydrate_success = renderer->hydrate();
    UIAction ui_action = button_controls->get_deep_sleep_action();
    handleEpubList(renderer, ui_action, !hydrate_success);
  }
  else
  {
    // Reset screen and show initial state
    renderer->reset();
    handleEpubList(renderer, NONE, true);
  }
  
  // Draw battery level
  if (battery)
  {
    ESP_LOGI(TAG, "Battery: %.2fV (%.0f%%)", 
             battery->get_voltage(), battery->get_percentage());
  }
  
  // Flush display
  renderer->flush_display();
  
  // Main event loop
  int64_t last_user_interaction = esp_timer_get_time();
  while (esp_timer_get_time() - last_user_interaction < 120 * 1000 * 1000)
  {
    UIAction ui_action = NONE;
    
    // Wait for button press (60 second timeout)
    if (xQueueReceive(ui_queue, &ui_action, pdMS_TO_TICKS(60000)) == pdTRUE)
    {
      if (ui_action != NONE)
      {
        last_user_interaction = esp_timer_get_time();
        
        // Show feedback
        handleEpubList(renderer, ui_action, false);
      }
    }
    
    // Update battery level periodically
    if (battery)
    {
      ESP_LOGI(TAG, "Battery: %.2fV (%.0f%%)",
               battery->get_voltage(), battery->get_percentage());
    }
    
    // Flush display
    renderer->flush_display();
  }
  
  ESP_LOGI(TAG, "Saving state");
  renderer->dehydrate();
  
  // Stop filesystem
  board->stop_filesystem();
  
  // Prepare for sleep
  board->prepare_to_sleep();
  
  ESP_LOGI(TAG, "Entering deep sleep");
  button_controls->setup_deep_sleep();
  vTaskDelay(pdMS_TO_TICKS(500));
  esp_deep_sleep_start();
}

void handleEpubList(Renderer *renderer, UIAction action, bool needs_redraw)
{
  // Load epub list if not already loaded
  if (!epub_list)
  {
    ESP_LOGI(TAG, "Creating epub list");
    epub_list = new EpubList(renderer, epub_list_state);
    
    // Try to load from SD card
    if (!epub_list->load("/fs/"))
    {
      ESP_LOGW(TAG, "Failed to load EPUB list from SD card");
    }
  }
  
  if (needs_redraw)
  {
    epub_list->set_needs_redraw();
  }
  
  // Handle user action
  switch (action)
  {
  case UP:
    epub_list->prev();
    break;
  case DOWN:
    epub_list->next();
    break;
  case SELECT:
    // TODO: Open selected EPUB
    ESP_LOGI(TAG, "Selected EPUB: %s", epub_list->get_selected_item().c_str());
    break;
  case NONE:
  default:
    break;
  }
  
  // Render the list
  epub_list->render();
}

void app_main()
{
  // Set log levels
  esp_log_level_set("main", LOG_LEVEL);
  esp_log_level_set("RLCD_Board", LOG_LEVEL);
  esp_log_level_set("RLCDRenderer", LOG_LEVEL);
  esp_log_level_set("EpubList", LOG_LEVEL);
  
  ESP_LOGI(TAG, "Starting RLCD EPUB Reader");
  ESP_LOGI(TAG, "Free heap: %d bytes", esp_get_free_heap_size());
  
  // Start main task on core 1
  xTaskCreatePinnedToCore(main_task, "main_task", 32768, NULL, 1, NULL, 1);
}
