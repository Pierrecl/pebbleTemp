#include <pebble.h>


static TextLayer *s_time_layer;
static Window *s_main_window;
static TextLayer *s_weather_layer;
static GFont s_weather_font;
static TextLayer *s_description_layer;

static void update_time(){
  
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  static char s_buffer[8];
  
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style()? "%H:%M" : "%I:%M", tick_time);
  text_layer_set_text(s_time_layer,s_buffer);
  
}

static void main_window_load(Window *window) {
  
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  s_time_layer = text_layer_create(
    GRect(0,PBL_IF_ROUND_ELSE(58, 52), bounds.size.w, 50));
  
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_font(s_time_layer,fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  
  s_weather_layer = text_layer_create(
    GRect(0,PBL_IF_ROUND_ELSE(125, 120), bounds.size.w, 25));
  
  text_layer_set_background_color(s_weather_layer,GColorClear);
  text_layer_set_text_color(s_weather_layer, GColorBlack);
  text_layer_set_text(s_weather_layer, "Chargement...");
  text_layer_set_text_alignment(s_weather_layer,GTextAlignmentCenter);
  
  
  s_weather_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_PERFECT_DOS_20));
  text_layer_set_font(s_weather_layer, s_weather_font);
  
  s_description_layer = text_layer_create(GRect(0,PBL_IF_ROUND_ELSE( 105,100), bounds.size.w, 30));
  text_layer_set_background_color(s_description_layer, GColorClear);
  text_layer_set_text_color(s_description_layer,GColorBlack);
  text_layer_set_text(s_description_layer,"Température intérieure");
  text_layer_set_text_alignment(s_description_layer, GTextAlignmentCenter);
  
  layer_add_child(window_layer,text_layer_get_layer(s_description_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_weather_layer));
}

static void main_window_unload(Window *window){
  
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_weather_layer);
  text_layer_destroy(s_description_layer);
  fonts_unload_custom_font(s_weather_font);

}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed){
  if(tick_time->tm_min % 15 == 0){
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    
    dict_write_uint8(iter,0,0);
    
    app_message_outbox_send();
  }
  update_time();
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context){
  static char temperature_buffer[10];
  static char weather_layer_buffer[32];
  
  Tuple *temp_tuple = dict_find(iterator, MESSAGE_KEY_TEMPERATURE);
  
  if(temp_tuple){
    snprintf(temperature_buffer, sizeof(temperature_buffer),"%d",(int)temp_tuple->value->uint8);
    snprintf(weather_layer_buffer, sizeof(weather_layer_buffer),"%s °C",temperature_buffer);
    text_layer_set_text(s_weather_layer,weather_layer_buffer);
  }
}

static void inbox_droped_callback(AppMessageResult reason, void *context){ 
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message reçus avec succés !");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context){
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message non transmit !");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context){
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message envoyé avec succés !");
}

static void init() {

  s_main_window = window_create();

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  window_stack_push(s_main_window, true);
  update_time();
  
  const int inbox_size = 128;
  const int outbox_size = 128;
  app_message_open(inbox_size,outbox_size);
  
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_droped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
}

static void deinit() {
 window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}

