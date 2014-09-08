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
    int incremented = current += 1;
    if (incremented >= max_value){
        return 0;
    }
    return incremented;
}

int pp_decrement(int current, int max_value) {    
    int incremented = current -= 1;
    if (incremented < 0 ){
        return max_value;
    }
    return incremented;
}

void _pp_defaultUpdateNozzle(Pack* pack, Nozzle* nozzle) {
    nozzle->led = 4000;
}

void _pp_defaultUpdateCyclotron(Pack* pack, Cyclotron* cyclotron) {
    static int BRIGHTNESS_INCREMENT = 83;
    unsigned long last_updated = pack->now - cyclotron->last_updated;
        if (last_updated >= 1000) {
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
    unsigned long last_updated = pack->now - cell->last_updated;
    if (last_updated >= cell->UPDATE_RATE){
        if (pack->is_initializing) {
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
            cell->current_led = pp_decrement(cell->current_led, cell->num_leds - 1);            
            cell->current_led %= cell->num_leds;
        } else {
            if (cell->current_led == cell->num_leds - 1){
                for (int i=0; i<= cell->num_leds; i++) {
                  cell->leds[i] = 0;  
                }
            }
            cell->leds[cell->current_led] = cell->MAX_BRIGHTNESS;            
            cell->current_led = pp_decrement(cell->current_led, cell->num_leds - 1);            
        }
        cell->last_updated = millis();
    }
}

void _pp_defaultUpdateGraph(Pack* pack, Graph* graph){
    
    int O = 1000;
    int o = 4000;
    int _ = 0;
    const int firing_sequence[][15] = {
      {_, _, _, _, _, _, _, O, _, _, _, _, _, _, _},
      {_, _, _, _, _, _, o, O, o, _, _, _, _, _, _},
      {_, _, _, _, _, o, O, _, O, o, _, _, _, _, _},
      {_, _, _, _, o, O, _, _, _, O, o, _, _, _, _},
      {_, _, _, o, O, _, _, _, _, _, O, o, _, _, _},
      {_, _, o, O, _, _, _, _, _, _, _, O, o, _, _},
      {_, o, O, _, _, _, _, _, _, _, _, _, O, o, _},
      {o, O, _, _, _, _, _, _, _, _, _, _, _, O, o},
    };
    const int normal_sequence[][15] = {
        {o, _, _, _, _, _, _, _, _, _, _, _, _, _, _},
        {O, o, _, _, _, _, _, _, _, _, _, _, _, _, _},
        {O, O, o, _, _, _, _, _, _, _, _, _, _, _, _},
        {O, O, O, o, _, _, _, _, _, _, _, _, _, _, _},
        {O, O, O, O, o, _, _, _, _, _, _, _, _, _, _},
        {O, O, O, O, O, o, _, _, _, _, _, _, _, _, _},
        {O, O, O, O, O, O, o, _, _, _, _, _, _, _, _},
        {O, O, O, O, O, O, O, o, _, _, _, _, _, _, _},
        {O, O, O, O, O, O, O, O, _, _, _, _, _, _, _},
        {O, O, O, O, O, O, O, o, _, _, _, _, _, _, _},
        {O, O, O, O, O, O, o, _, _, _, _, _, _, _, _},
        {O, O, O, O, O, o, _, _, _, _, _, _, _, _, _},
        {O, O, O, O, o, _, _, _, _, _, _, _, _, _, _},
        {O, O, O, o, _, _, _, _, _, _, _, _, _, _, _},
        {O, O, o, _, _, _, _, _, _, _, _, _, _, _, _},
        {O, o, _, _, _, _, _, _, _, _, _, _, _, _, _},
        {o, _, _, _, _, _, _, _, _, _, _, _, _, _, _},
        {_, _, _, _, _, _, _, _, _, _, _, _, _, _, _},
    };
    

    unsigned long last_updated = pack->now - graph->last_updated;
    
    if (last_updated >= 40){
        if(pack->is_firing){
            for(int i=0; i < graph->num_leds; i++){
                graph->leds[i] = firing_sequence[graph->iteration][i];
            }
            graph->iteration++;
            graph->iteration %= 8;            
        } else {
            for(int i=0; i < graph->num_leds; i++){
                graph->leds[i] = normal_sequence[graph->iteration][i];
            }
            graph->iteration++;
            graph->iteration %= 18;            
        }
        graph->last_updated = millis(); 
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
    setNozzleUpdateCallback(_pp_defaultUpdateNozzle);
    reset();
}

void resetNozzle(Nozzle* _nozzle) {
    _nozzle->last_updated = 0;
    _nozzle->led = 0;
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
    resetNozzle(&_nozzle);
    _pack.powercell = _cell;
    _pack.cyclotron = _cyclotron;
    _pack.graph = _graph;
    _pack.nozzle = _nozzle;
}

void ProtonPack::initialize() {
    Tlc.init();
    pinMode(_power_switch_id, INPUT);
    
    
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

void ProtonPack::_updateNozzle() {
    _update_nozzle_cb(&_pack, &_nozzle);
    Tlc.set(_graph_offset + 16, _nozzle.led);
}

void ProtonPack::update() {
    _pack.now = millis();
    
    int latestPowerButtonState = digitalRead(_power_switch_id);;
    int latestActivateButtonState = digitalRead(_activate_switch_id);;

    boolean shutting_down = false;
    boolean starting_up = false;
    boolean is_initializing_now = (_pack.now - _pack.started_at) < 5000;
    if(is_initializing_now and ! _pack.is_initializing){
        resetPowercell(&_cell);
        
    } else if (_pack.is_initializing && !is_initializing_now){
        resetPowercell(&_cell);
    }
    
    _pack.is_initializing = is_initializing_now;

    if (millis() - last_debounce_time > 300) {
        if ( latestPowerButtonState != _power_button_state){
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
        _updateNozzle();
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

void ProtonPack::setNozzleUpdateCallback(updateNozzleCallback cb) {
    _update_nozzle_cb = cb;
}