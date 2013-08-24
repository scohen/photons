#include "Arduino.h"
#include <tlc_fades.h>
#include <tlc_config.h>
#include <tlc_animations.h>
#include <tlc_progmem_utils.h>
#include <Tlc5940.h>
#include <tlc_servos.h>
#include <tlc_shifts.h>

#include "ProtonPack.h"

int increment = 50;
int last_debounce_time = 0;

int pp_increment_to_max(int current, int max_value) {
    int incremented = current += 1;
    if (incremented < max_value) {
        return incremented;
    }
    return 0;
}

void _pp_defaultUpdateCyclotron(Pack* pack, Cyclotron* cyclotron) {

    static int BRIGHTNESS_INCREMENT = 80;
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
    
    if (pack->is_initializing) {        
        cell->current_brightness += increment;
        if (cell->current_brightness >= cell->MAX_BRIGHTNESS || cell->current_brightness <=0) {
            increment *= -1;
        }

        for(int i=0; i < cell->num_leds; i++) {
            cell->leds[i] = cell->current_brightness;
        }        

    } else {
        if (last_updated >= cell->UPDATE_RATE) {
            if (cell->current_led == 0){
                for (int i=0; i<= cell->num_leds; i++) {
                  cell->leds[i] = 0;  
                }
            }
            cell->leds[cell->current_led] = cell->MAX_BRIGHTNESS;            
            cell->current_led = pp_increment_to_max(cell->current_led, cell->num_leds + 1);
            cell->last_updated = millis();
        }
    }
}

ProtonPack::ProtonPack(int power_switch_id,
                       int activate_switch_id,
                       int cyclotron_offset,
                       int powercell_offset) {
    _power_switch_id = power_switch_id;
    _activate_switch_id = activate_switch_id;
    _cyclotron_offset = cyclotron_offset;
    _powercell_offset = powercell_offset;    
    setCyclotronUpdateCallback(_pp_defaultUpdateCyclotron);
    setPowercellUpdateCallback(_pp_defaultUpdatePowercell);
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

void resetPack(Pack* _pack) {
    _pack->now = millis();
    _pack->started_at = _pack->now;
    _pack->is_on = false;
    _pack->is_activated = false;
    _pack->is_initializing = false;
}

void ProtonPack::reset() {
    resetPowercell(&_cell);
    resetCyclotron(&_cyclotron);
    resetPack(&_pack);
    _pack.powercell = _cell;
    _pack.cyclotron = _cyclotron;        
}

void ProtonPack::initialize() {
    Tlc.init();
    Serial.begin(9600);
    pinMode(_power_switch_id, INPUT);
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

void ProtonPack::update() {
    _pack.now = millis();
    int buttonState = digitalRead(_power_switch_id);
    boolean shutting_down = false;
    boolean starting_up = false;
    _pack.is_initializing = (_pack.now - _pack.started_at) < 5000;
    
    if ( millis() - last_debounce_time > 150) {
        if (buttonState == HIGH) {
            if(_pack.is_on) {
                shutting_down = true;
            } else {
                starting_up = true;
            }
            _pack.is_on = !_pack.is_on;
            last_debounce_time = millis();
            _pack.now = last_debounce_time;            
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
        Tlc.update();
    }

}

void ProtonPack::setPowercellUpdateCallback(updatePowercellCallback cb) {
    _update_powercell_cb = cb;
}

void ProtonPack::setCyclotronUpdateCallback(updateCyclotronCallback cb) {
    _update_cyclotron_cb = cb;
}
