#include "Arduino.h"
#include <tlc_fades.h>
#include <tlc_config.h>
#include <tlc_animations.h>
#include <tlc_progmem_utils.h>
#include <Tlc5940.h>
#include <tlc_servos.h>
#include <tlc_shifts.h>

#include "ProtonPack.h"
#define INPUT_PULLUP 0x20


int increment = 50;
int last_debounce_time = 0;
int direction = 1;

int pp_increment_to_max(int current, int max_value) {    
    int incremented = current -= 1;
    if (incremented < 0 ){
        return max_value;
    }
    return incremented;
}

void _pp_defaultUpdateCyclotron(Pack* pack, Cyclotron* cyclotron) {

    static int BRIGHTNESS_INCREMENT = 83;
    int last_updated = pack->now - cyclotron->last_updated;
    if (last_updated > 1000) {
        cyclotron->current_brightness = 0;
        cyclotron->current_led = pp_increment_to_max(cyclotron->current_led, 4);
        cyclotron->last_updated = pack->now;
    } else if ( last_updated < 500 ) {
        cyclotron->current_brightness += BRIGHTNESS_INCREMENT;
        cyclotron->leds[cyclotron->current_led] = cyclotron->current_brightness;
    } else if (cyclotron->current_brightness > 0) {
        cyclotron->current_brightness -= BRIGHTNESS_INCREMENT;
        cyclotron->leds[cyclotron->current_led] = cyclotron->current_brightness;
    }
}

void _pp_defaultUpdatePowercell(Pack* pack, Powercell* cell) {
    int last_updated = pack->now - cell->last_updated;
    
    if (last_updated >= cell->UPDATE_RATE){
        if (pack->is_initializing) {
            int r = random(0, cell->num_leds);
            for(int i=0; i < cell->num_leds; i++){
                if (i == cell->current_led){
                    cell->leds[i] = cell->MAX_BRIGHTNESS;
                } else {
                    cell->leds[i] = 0;
                }
                
            }
            if (cell->current_led > 0){
                cell->leds[cell->current_led - 1] = cell->MAX_BRIGHTNESS / 2;
            }
                
            if (cell->current_led > 1) {
                cell->leds[cell->current_led] = cell->MAX_BRIGHTNESS / 4;
            }
            cell->current_led = pp_increment_to_max(cell->current_led, cell->num_leds - 1);            
            cell->current_led %= cell->num_leds;
        } else {
            if (cell->current_led == cell->num_leds - 1){
                for (int i=0; i<= cell->num_leds; i++) {
                  cell->leds[i] = 0;  
                }
            }
            cell->leds[cell->current_led] = cell->MAX_BRIGHTNESS;            
            cell->current_led = pp_increment_to_max(cell->current_led, cell->num_leds - 1);            
        }
        cell->last_updated = millis();
    }
}

void _pp_defaultUpdateGraph(Pack* pack, Graph* graph){    
    int M = 1000;
    int H = 4000;
    int O = 0;
    const int firing_sequence[][15] = {
      {O, O, O, O, O, O, O, H, O, O, O, O, O, O, O},
      {O, O, O, O, O, O, M, H, M, O, O, O, O, O, O},
      {O, O, O, O, O, M, H, O, H, M, O, O, O, O, O},
      {O, O, O, O, M, H, O, O, O, H, M, O, O, O, O},
      {O, O, O, M, H, O, O, O, O, O, H, M, O, O, O},
      {O, O, M, H, O, O, O, O, O, O, O, H, M, O, O},
      {O, M, H, O, O, O, O, O, O, O, O, O, H, M, O},
      {M, H, O, O, O, O, O, O, O, O, O, O, O, H, M},
    };
    const int normal_sequence[][15] = {
        {O, O, O, O, O, O, O, O, O, O, O, O, O, O, M},
        {O, O, O, O, O, O, O, O, O, O, O, O, O, M, H},
        {O, O, O, O, O, O, O, O, O, O, O, O, M, H, H},
        {O, O, O, O, O, O, O, O, O, O, O, M, H, H, H},
        {O, O, O, O, O, O, O, O, O, O, M, H, H, H, H},
        {O, O, O, O, O, O, O, O, O, M, H, H, H, H, H},
        {O, O, O, O, O, O, O, O, M, H, H, H, H, H, H},
        {O, O, O, O, O, O, O, M, H, H, H, H, H, H, H},
        {O, O, O, O, O, O, O, H, H, H, H, H, H, H, H},
        {O, O, O, O, O, O, O, M, H, H, H, H, H, H, H},
        {O, O, O, O, O, O, O, O, M, H, H, H, H, H, H},
        {O, O, O, O, O, O, O, O, O, M, H, H, H, H, H},
        {O, O, O, O, O, O, O, O, O, O, M, H, H, H, H},
        {O, O, O, O, O, O, O, O, O, O, O, M, H, H, H},
        {O, O, O, O, O, O, O, O, O, O, O, O, M, H, H},
        {O, O, O, O, O, O, O, O, O, O, O, O, O, M, H},
        {O, O, O, O, O, O, O, O, O, O, O, O, O, O, M},
        {O, O, O, O, O, O, O, O, O, O, O, O, O, O, O},
    };
    

    int last_updated = pack->now - graph->last_updated;
    
    /*if (last_updated >= 50){
        graph->leds[14] = 2000;

        if(pack->is_firing){
            for(int i=0; i < graph->num_leds; i++){
                graph->leds[i] = firing_sequence[graph->iteration][i];
            }                        
            graph->iteration %= 8;            
        } else {
            for(int i=0; i < graph->num_leds; i++){
                graph->leds[i] = normal_sequence[graph->iteration][i];
            }                        
            graph->iteration %= 18;            
        }
        graph->last_updated = millis();
        graph->iteration++;
    }
    */
    for(int i=0; i < graph->num_leds; i++){
        if (rand() % 3 == 0){
            graph->leds[i] = H;
        } else{
            graph->leds[i] = O;
        }
    }
}

ProtonPack::ProtonPack(int power_switch_id,
                       int activate_switch_id,
                       int cyclotron_offset,
                       int powercell_offset,
                       int graph_offset) {
    _power_switch_id = power_switch_id;
    _activate_switch_id = activate_switch_id;
    _cyclotron_offset = cyclotron_offset;
    _powercell_offset = powercell_offset;
    _graph_offset = graph_offset;
    setCyclotronUpdateCallback(_pp_defaultUpdateCyclotron);
    setPowercellUpdateCallback(_pp_defaultUpdatePowercell);
    setGraphUpdateCallback(_pp_defaultUpdateGraph);
    reset();
}

void resetCyclotron(Cyclotron* _cyclotron) {
    _cyclotron->last_updated = 0;
    _cyclotron->current_led = -1;
    _cyclotron->current_brightness = 0;
    for (int i=0; i < 4; i++) {
        _cyclotron->leds[i] = 0;
    }
}

void resetPowercell(Powercell* _cell) {
    _cell->last_updated = 0;
    _cell->current_led = 0;
    _cell->initializing = true;
    _cell->current_brightness = 0;
    for(int i=0; i < _cell->num_leds; i++) {
        _cell->leds[i] = 0;
    }
}

void resetGraph(Graph* _graph){
    _graph->last_updated = 0;
    _graph->iteration = 0;    
    for(int i=0; i < _graph->num_leds; i++) {
        _graph->leds[i] = 0;
    }
}

void resetPack(Pack* _pack) {
    _pack->now = millis();
    _pack->started_at = _pack->now;
    _pack->is_on = false;
    _pack->is_firing = false;
    _pack->is_initializing = false;    
}

void ProtonPack::reset() {
    Tlc.clear();
    resetPowercell(&_cell);
    resetCyclotron(&_cyclotron);
    resetPack(&_pack);
    resetGraph(&_graph);
    _pack.powercell = _cell;
    _pack.cyclotron = _cyclotron;
    _pack.graph = _graph;
}

void ProtonPack::initialize() {
    Tlc.init();    
    pinMode(_power_switch_id, INPUT);
    digitalWrite(_power_switch_id, HIGH);
    
    pinMode(_activate_switch_id, INPUT);
    digitalWrite(_activate_switch_id, HIGH);
    
    _power_button_state = 0;
    _activate_button_state = 0;
}

void ProtonPack::_updatePowercell() {
    _update_powercell_cb(&_pack, &_cell);
    for(int i=0; i < _cell.num_leds; i++) {
        Tlc.set(_powercell_offset + i, _cell.leds[i]);
    }
}

void ProtonPack::_updateCyclotron() {
    _update_cyclotron_cb(&_pack, &_cyclotron);
    for(int i=0; i < 4; i++) {
        Tlc.set(_cyclotron_offset + i, _cyclotron.leds[i]);
    }
}

void ProtonPack::_updateGraph(){
    _update_graph_cb(&_pack, &_graph);
    for(int i=0; i < 48; i++){
        Tlc.set(_graph_offset + i, _graph.leds[i]);        
    }
}

void ProtonPack::update() {
    _pack.now = millis();
    int powerRead = analogRead(_power_switch_id);
    int activityRead = analogRead(_activate_switch_id);
    int latestPowerButtonState = powerRead > 1010;
    int latestActivateButtonState = activityRead > 1010;
    boolean shutting_down = false;
    boolean starting_up = false;
    boolean is_initializing_now = (_pack.now - _pack.started_at) < 5000;
    if(is_initializing_now and ! _pack.is_initializing){
        resetPowercell(&_cell);
        
    } else if (_pack.is_initializing && !is_initializing_now){
        resetPowercell(&_cell);
    }
    
    _pack.is_initializing = is_initializing_now;
    /*
    Serial.print("Activate: ");
    Serial.print(activityRead);
    Serial.print("\n");
    Serial.print("Power: ");
    Serial.print(powerRead);
    Serial.print("\n");
*/
    if ( millis() - last_debounce_time > 300) {
        if ( latestPowerButtonState != _power_button_state){
            Serial.print("SWITCH\n\tpower:");
            Serial.print(powerRead);
            Serial.print("\n\tactivity:");
            Serial.print(activityRead);
            Serial.print("\n");
            if(_pack.is_on) {
                shutting_down = true;
            } else {
                starting_up = true;
            }
            _pack.is_on = !_pack.is_on;
            last_debounce_time = millis();
            _pack.now = last_debounce_time;                    
            _power_button_state = latestPowerButtonState;
        }
        if (_pack.is_on){
            if (latestActivateButtonState != _activate_button_state){            
                _pack.is_firing = !_pack.is_firing;
                Serial.print("Firing ");
                Serial.println(_pack.is_firing);
                _activate_button_state = latestActivateButtonState;
                last_debounce_time = millis();
            }
        }
    }

    if (starting_up) {
        _pack.started_at = _pack.now;
        _cell.last_updated = _pack.now;
        _cyclotron.last_updated = _pack.now;
        
    } else if (shutting_down) {
        resetPack(&_pack);
        resetCyclotron(&_cyclotron);
        resetPowercell(&_cell);
        Tlc.clear();
        Tlc.update();
    }

    if (_pack.is_on) {
        _updatePowercell();
        _updateCyclotron();
        _updateGraph();
        Tlc.update();
    }

}

void ProtonPack::setPowercellUpdateCallback(updatePowercellCallback cb) {
    _update_powercell_cb = cb;
}

void ProtonPack::setCyclotronUpdateCallback(updateCyclotronCallback cb) {
    _update_cyclotron_cb = cb;
}

void ProtonPack::setGraphUpdateCallback(updateGraphCallback cb) {
    _update_graph_cb = cb;
}
